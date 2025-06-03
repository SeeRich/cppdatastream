#ifndef CPPDS_DATASTREAM_H
#define CPPDS_DATASTREAM_H

#pragma once

#ifdef _MSC_VER
    #pragma warning(push, 1)
#endif
#include "moodycamel/readerwritercircularbuffer.h"
#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#ifdef WIN32
    #define NOMINMAX
    #undef min
    #undef max
#endif

#include <any>
#include <format>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#ifndef _MSC_VER
    #define HAS_CXX_DEMANGLE
    #include <cxxabi.h>
#endif

namespace cppdatastream {

namespace detail {

/// @brief Designed to be called as demangleName(typeid(...).name())
inline ::std::string demangleName(const ::std::string& name)
{
#ifdef HAS_CXX_DEMANGLE
    int status{0};
    return abi::__cxa_demangle(name.c_str(), 0, 0, &status);
#else
    // Typeid(...).name() is already demangled on windows.
    return name;
#endif
}

}  // namespace detail

}  // namespace cppdatastream

namespace cppdatastream {

enum class LogLevel
{
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

// Virtual logger interface
class ILogger
{
public:
    virtual ~ILogger() = default;

    virtual void log(LogLevel level, const std::string& message) = 0;
};

// NullLogger implementation
class NullLogger : public ILogger
{
public:
    inline void log(LogLevel, const std::string&) override {}
};

// Global logger
static std::mutex loggerMutex;
static std::shared_ptr<ILogger> currentLogger = std::make_shared<NullLogger>();

// Getter/Setter for the global logger
std::shared_ptr<ILogger> getLogger()
{
    std::lock_guard lock(loggerMutex);
    return currentLogger;
}

void setLogger(std::shared_ptr<ILogger> logger)
{
    std::lock_guard lock(loggerMutex);
    currentLogger = logger;
}

}  // namespace cppdatastream

#ifdef ENABLE_CPPDATASTREAM_LOGGING
    #define CDS_LOG_TRACE(...) cppdatastream::getLogger()->log(cppdatastream::LogLevel::Trace, std::format(__VA_ARGS__))
    #define CDS_LOG_DEBUG(...) cppdatastream::getLogger()->log(cppdatastream::LogLevel::Debug, std::format(__VA_ARGS__))
    #define CDS_LOG_INFO(...) cppdatastream::getLogger()->log(cppdatastream::LogLevel::Info, std::format(__VA_ARGS__))
    #define CDS_LOG_WARN(...) cppdatastream::getLogger()->log(cppdatastream::LogLevel::Warn, std::format(__VA_ARGS__))
    #define CDS_LOG_ERROR(...) cppdatastream::getLogger()->log(cppdatastream::LogLevel::Error, std::format(__VA_ARGS__))
    #define CDS_LOG_FATAL(...) cppdatastream::getLogger()->log(cppdatastream::LogLevel::Fatal, std::format(__VA_ARGS__))

#else
    #define CDS_LOG_TRACE(...)
    #define CDS_LOG_DEBUG(...)
    #define CDS_LOG_INFO(...)
    #define CDS_LOG_WARNING(...)
    #define CDS_LOG_ERROR(...)
    #define CDS_LOG_FATAL(...)
#endif

#ifdef ENABLE_CPPDATASTREAM_DTOR_LOGGING
    #define CDS_LOG_DTOR(...) cppdatastream::getLogger()->log(cppdatastream::LogLevel::Trace, std::format(__VA_ARGS__))
    #define CDS_LOG_DTOR_VFUNC(name) \
        virtual ~name() { CDS_LOG_DTOR("{} DTOR", className()); }
#else
    #define CDS_LOG_DTOR(...)
    #define CDS_LOG_DTOR_VFUNC(name)
#endif

#define CPPDATASTREAM_CLASS_NAME() \
    std::string className() const { return cppdatastream::detail::demangleName(typeid(*this).name()); }

namespace cppdatastream {

// End of processing status
// An upstream element can fill this out (e.g. if there was an exception/unhandled error)
// and it will be passed all the way down the pipeline so that all elements can handle it
// appropriately and the final element can report it to the user (i.e. via wait())
enum EopStatusType
{
    EopSuccess,
    EopCancelled,
    EopError,
};

struct EopStatus
{
    // Type indicating success or failure
    EopStatusType type{EopSuccess};
    // Message associated with the status (should always be set for errors)
    std::string message;

    std::string toString()
    {
        switch(type) {
            case EopSuccess:
                return "Success";
            case EopCancelled:
                return "Cancelled";
            case EopError: {
                return "Error: " + message;
            }
            default: {
                return "Unknown";
            }
        }
    }
};

template <typename T>
class SharedDataBlock
{
public:
    SharedDataBlock() = default;

    virtual ~SharedDataBlock() = default;

    auto data() const -> const T& { return *_data; }

    bool isEmpty() const { return _data.operator bool(); }

    bool isEndOfProcessing() const { return _eop.has_value(); }

    auto eopStatus() const -> const std::optional<EopStatus> { return _eop; }

protected:
    /// Shared data block
    std::shared_ptr<T> _data;
    /// @brief End of processing
    std::optional<EopStatus> _eop;
};

template <typename T>
class WritableDataBlock : public SharedDataBlock<T>
{
public:
    WritableDataBlock() = default;

    virtual ~WritableDataBlock() = default;

    void setEndOfProcessing(const EopStatus& eop) { this->_eop = eop; }

    void setData(const T& data) { SharedDataBlock<T>::_data = std::make_shared<T>(data); }

    auto asShared() const -> SharedDataBlock<T> { return *this; }

    static auto createEos(const EopStatus& eop) -> WritableDataBlock
    {
        WritableDataBlock block;
        block.setEndOfProcessing(eop);
        return block;
    }
};

template <typename IN_T>
class StreamPushable
{
public:
    CPPDATASTREAM_CLASS_NAME();

    virtual ~StreamPushable() {}

    virtual void pushData(const SharedDataBlock<IN_T>& sdb) = 0;
};

inline std::string version() { return CPPDATASTREAM_VERSION; }

template <typename IN_T>
class StreamVisitor
{
public:
    CPPDATASTREAM_CLASS_NAME();

    virtual ~StreamVisitor() {}

    virtual bool visitData(const SharedDataBlock<IN_T>& sdb) = 0;
};

template <typename IN_T, typename OUT_T>
class StreamProcessor : public StreamPushable<IN_T>
{
public:
    CPPDATASTREAM_CLASS_NAME();

    CDS_LOG_DTOR_VFUNC(StreamProcessor);

    // Derived classes must implement this method
    virtual auto processData(const SharedDataBlock<IN_T>& sdb) -> SharedDataBlock<OUT_T> = 0;

    // Connects a StreamPushable to this processor
    virtual void connect(const std::shared_ptr<StreamPushable<IN_T>>& processor) { processors.push_back(processor); }

    // Connects a StreamVisitor to this processor
    virtual void connect(const std::shared_ptr<StreamVisitor<IN_T>>& visitor) { visitors.push_back(visitor); }

    virtual void pushData(const SharedDataBlock<IN_T>& sdb) override
    {
        // If we've already had an error, don't process any more data
        if(_had_error)
            return;

        // Process the incoming dataBlock
        auto output = processData(sdb);

        // Handle error condition
        if(output.isEndOfProcessing() && output.eopStatus().value().type == EopError) {
            CDS_LOG_ERROR("{}: failed to process dataBlock type: {}",
                          detail::demangleName(typeid(*this).name()),
                          detail::demangleName(typeid(sdb).name()));
            _had_error = true;
        }

        // Allow visitors to visit
        for(auto& visitor : visitors) {
            if(!visitor->visitData(output))
                CDS_LOG_ERROR("{}: failed to visit dataBlock type: {}",
                              detail::demangleName(typeid(visitor).name()),
                              detail::demangleName(typeid(sdb).name()));
        }

        // Push the processed data to the connected processors
        for(auto& processor : processors) processor->pushData(output);
    }

protected:
    std::string _name;
    bool _had_error{false};
    std::vector<std::shared_ptr<StreamVisitor<IN_T>>> visitors;
    std::vector<std::shared_ptr<StreamPushable<IN_T>>> processors;
};

template <typename OUT_T>
class StreamSource
{
public:
    CPPDATASTREAM_CLASS_NAME();

    CDS_LOG_DTOR_VFUNC(StreamSource);

    // Start processing (occurs in a background thread, i.e. this doesn't block)
    virtual bool start() = 0;

    // Stop processing
    virtual bool stop() = 0;

    // Cancel processing
    virtual bool cancel() = 0;

    // Connect to downstream
    virtual void connect(const std::shared_ptr<StreamPushable<OUT_T>>& downstream) = 0;
};

/// Passthrough processor, useful to bridge specific elements that interface only with
/// StreamProcessors
template <typename T>
class StreamNoopProcessor : public StreamProcessor<T, T>
{
public:
    CPPDATASTREAM_CLASS_NAME();
    CDS_LOG_DTOR_VFUNC(StreamNoopProcessor);

protected:
    virtual auto processData(const SharedDataBlock<T>& sdb) -> SharedDataBlock<T> override { return sdb; }
};

template <typename T>
class StreamThreadedBuffer : public StreamProcessor<T, T>
{
public:
    CPPDATASTREAM_CLASS_NAME();

    explicit StreamThreadedBuffer(size_t maxBlocks = 100) : _queue(maxBlocks) {}

    virtual ~StreamThreadedBuffer()
    {
        if(_thread.joinable())
            _thread.join();
        CDS_LOG_DTOR("{} DTOR", className());
    }

    virtual void pushData(const SharedDataBlock<T>& sdb) override
    {
        // If we haven't started the thread yet, do it now
        if(!_thread_started) {
            _thread = std::thread(&StreamThreadedBuffer::run, this);
            _thread_started = true;
        }

        // Wait for the queue to have space
        while(!_queue.wait_enqueue_timed(sdb, std::chrono::milliseconds(1))) {
        }
    }

protected:
    virtual auto processData(const SharedDataBlock<T>& sdb) -> SharedDataBlock<T> override final { return sdb; }

private:
    void run()
    {
        SharedDataBlock<T> sdb;
        do {
            // We assume there will always be an EOP block at the end
            _queue.wait_dequeue(sdb);
            StreamProcessor<T, T>::pushData(sdb);
            if(sdb.isEndOfProcessing())
                break;
        } while(true);
    }

    /// @brief SPSC thread-safe queue used to buffer SharedDataBlocks
    moodycamel::BlockingReaderWriterCircularBuffer<SharedDataBlock<T>> _queue;
    /// Thread used to push blocks downstream
    std::thread _thread;
    /// Background thread started
    bool _thread_started{false};
};

template <typename T>
class OnEosVistor : public StreamVisitor<T>
{
public:
    CPPDATASTREAM_CLASS_NAME();

    OnEosVistor(std::function<void()> func) { _func = func; }

    CDS_LOG_DTOR_VFUNC(OnEosVistor);

private:
    virtual bool visitData(const SharedDataBlock<T>& sdb) override
    {
        if(sdb.isEndOfProcessing())
            _func();
        return true;
    };

    std::function<void()> _func;
};

template <typename T>
class AnonymousVisitor : public StreamVisitor<T>
{
public:
    using AnonymousFunc = std::function<bool(const SharedDataBlock<T>&)>;

    CPPDATASTREAM_CLASS_NAME();

    AnonymousVisitor(AnonymousFunc func) { _func = func; }

    CDS_LOG_DTOR_VFUNC(AnonymousVisitor);

private:
    virtual bool visitData(const SharedDataBlock<T>& sdb) override { return _func(sdb); };

    AnonymousFunc _func;
};

/// @brief StreamSink is a visitor that waits for an EndOfProcessing block and reports the status
template <typename T>
class StreamSink final : public StreamVisitor<T>
{
public:
    CPPDATASTREAM_CLASS_NAME();

    CDS_LOG_DTOR_VFUNC(StreamSink);

    StreamSink()
    {
        auto promise = std::make_shared<std::promise<EopStatus>>();
        _future = promise->get_future().share();
        _func = [promise](const EopStatus& eop) {
            CDS_LOG_DEBUG("StreamSink: EOS");
            promise->set_value(eop);
        };
    }

    virtual bool visitData(const SharedDataBlock<T>& sdb) override
    {
        if(sdb.isEndOfProcessing())
            _func(sdb.eopStatus().value());
        return true;
    };

    EopStatus wait()
    {
        // Make copy of shared future
        auto future = _future;
        // Wait for the future to be set
        return future.get();
    }

private:
    std::shared_future<EopStatus> _future;
    std::function<void(const EopStatus&)> _func;
};

}  // namespace cppdatastream

#endif