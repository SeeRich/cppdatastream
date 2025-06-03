#include "cppdatastream/DataStream.hpp"

#include "AlignedAllocator.hpp"
#include "CLI11.hpp"
#include "Logging.hpp"
#include "Utilities.hpp"

#include <fmt/color.h>
#include <spdlog/spdlog.h>

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <list>
#include <new>
#include <string>

using namespace std;
namespace cds = cppdatastream;

// Aligned datablock of bytes
using DataT = AlignedVector<uint8_t>;

class SimpleSource : public cds::StreamSource<DataT>
{
public:
    CPPDATASTREAM_CLASS_NAME();

    SimpleSource(uint64_t numBlocks, uint64_t numBytesPerBlock)
        : _num_blocks(numBlocks), _num_bytes_per_block(numBytesPerBlock)
    {}

    virtual ~SimpleSource()
    {
        if(_thread.joinable())
            _thread.join();
        CDS_LOG_DTOR("{} DTOR", className());
    }

    virtual bool start() override
    {
        LOG_DEBUG("Starting source");
        // Move the downstream into a separate thread that will allocate and push the data blocks
        _thread = std::thread(&SimpleSource::run, this, std::move(_downstream));
        return true;
    }

    virtual bool stop() override
    {
        LOG_DEBUG("Stopping source");
        _stop_flag.store(1);
        return true;
    }

    virtual bool cancel() override
    {
        LOG_DEBUG("Cancelling source");
        _stop_flag.store(2);
        return true;
    }

    virtual void connect(const std::shared_ptr<cds::StreamPushable<DataT>>& downstream) override
    {
        LOG_DEBUG("Connecting source to downstream");
        _downstream = downstream;
    }

private:
    void run(std::shared_ptr<cds::StreamPushable<DataT>>&& downstream)
    {
        LOG_INFO("Processing {} blocks of {} bytes each", _num_blocks, _num_bytes_per_block);
        uint64_t num_blocks{0};
        while(num_blocks < _num_blocks && _stop_flag.load() == 0) {
            // Allocate an aligned vector of bytes filled with ones
            auto vec = DataT(_num_bytes_per_block, 1);
            // Create a writable data block;
            auto wb = cds::WritableDataBlock<DataT>();
            // Set the data (copy the vector)
            wb.setData(vec);
            // Push the data block
            downstream->pushData(wb);
            num_blocks++;
        }

        // Push an EndOfProcessing block
        auto wb = cds::WritableDataBlock<DataT>();
        auto stopFlag = _stop_flag.load();
        if(0 == stopFlag || 1 == stopFlag) {
            wb.setEndOfProcessing(cds::EopStatus{cds::EopStatusType::EopSuccess});
        } else if(2 == stopFlag) {
            wb.setEndOfProcessing(cds::EopStatus{cds::EopStatusType::EopCancelled});
        } else {
            wb.setEndOfProcessing(cds::EopStatus{cds::EopStatusType::EopError, "Unknown stop flag"});
        }
        downstream->pushData(wb);
    }

    uint64_t _num_blocks{0};
    uint64_t _num_bytes_per_block{0};
    std::atomic_uint8_t _stop_flag{0};
    std::thread _thread;
    std::shared_ptr<cds::StreamPushable<DataT>> _downstream;
};

class SimpleAccumulator : public cds::StreamProcessor<DataT, DataT>
{
public:
    CPPDATASTREAM_CLASS_NAME();

    virtual ~SimpleAccumulator() { CDS_LOG_DTOR("{} DTOR: sum = {}", className(), _sum); }

    virtual auto processData(const cds::SharedDataBlock<DataT>& sdb) -> cds::SharedDataBlock<DataT> override
    {
        if(sdb.isEndOfProcessing())
            return sdb;

        // Get a read-only copy of the data
        const auto& data = sdb.data();
        // Calculate the sum of the data
        _sum += std::accumulate(data.cbegin(), data.cend(), 0ul);
        return sdb;
    }

private:
    uint64_t _sum{0};
};

// Pass through datastream that does no processing
class DataStreamPassThrough : public cds::StreamProcessor<DataT, DataT>
{
public:
    CPPDATASTREAM_CLASS_NAME();

    explicit DataStreamPassThrough(uint64_t index) : _index(index) {}

    virtual ~DataStreamPassThrough() { CDS_LOG_DTOR("{} DTOR: index: {}", className(), _index); }

    virtual auto processData(const cds::SharedDataBlock<DataT>& sdb) -> cds::SharedDataBlock<DataT> override
    {
        return sdb;
    }

private:
    [[maybe_unused]] size_t _index{0};
};

template <typename T>
class DataStreamThroughputMonitor : public cds::StreamProcessor<T, T>
{
public:
    CPPDATASTREAM_CLASS_NAME();

    CDS_LOG_DTOR_VFUNC(DataStreamThroughputMonitor);

    virtual auto processData(const cds::SharedDataBlock<T>& sdb) -> cds::SharedDataBlock<T> override
    {
        if(sdb.isEndOfProcessing())
            return sdb;

        // Get a read-only copy of the data
        const auto& data = sdb.data();
        _throughput_bytes += data.size();

        processThroughput();

        return sdb;
    }

private:
    void processThroughput(bool flush = false)
    {
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - _begin).count();
        if(ms > 1000 || flush) {
            LOG_INFO("Data Throughput: {}/s", prettyPrintBytes(_throughput_bytes));
            _begin = now;
            _throughput_bytes = 0;
        }
    }
    // Starting time
    std::chrono::steady_clock::time_point _begin{std::chrono::steady_clock::now()};
    // Throughput bytes
    uint64_t _throughput_bytes{0};
};

// This example demonstrates how to create a more realistic data processing pipeline using cppdatastream.
// Compared to the simple example, this example includes a DataStreamSource and a DataStreamSink with some a some simple
// processing in between.

int main(int argc, char* argv[])
{
    // Setup signal handler
    registerSignalHandlers();

    // Setup logging
    spdlog::set_level(spdlog::level::trace);
    // Set the logger for cppdatastream
    auto cdsLogger = std::make_shared<CdsSpdLogger>();
    cppdatastream::setLogger(cdsLogger);

    // CLI options
    CLI::App app("DataStream processing pipeline prototype (source)");
    app.set_version_flag(
        "--version",
        fmt::format("Version: {}", fmt::format(fg(fmt::terminal_color::green), "{}", cppdatastream::version())));
    // Number of data blocks to pass through the pipeline
    uint32_t numBlocks = 1'000'000;
    app.add_option("-d,--datablocks", numBlocks, "Number of data blocks");
    // Number of datastreams to use in the pipeline
    bool withAccumulator{false};
    app.add_flag("-a,--accumulator", withAccumulator, "Add an accumulator to the pipeline");
    // Number of bytes in each block
    uint32_t numBytesPerBlock = 32'768;
    app.add_option("-b,--bytes", numBytesPerBlock, "Number of bytes per block");
    // Parse the CLI string
    CLI11_PARSE(app, argc, argv);

    // Create the source
    auto source = std::make_shared<SimpleSource>(numBlocks, numBytesPerBlock);

    // Register a interrupt handler to stop the source
    registerProgramInterruptHandler([&]() {
        LOG_INFO("Interrupting source");
        source->cancel();
    });

    // Add a thread buffer after the source
    auto tb = std::make_shared<cds::StreamThreadedBuffer<DataT>>();
    source->connect(tb);

    // Treat as base class StreamProcessor
    std::shared_ptr<cds::StreamProcessor<DataT, DataT>> ds = tb;

    // Add simple processor datastream
    if(withAccumulator) {
        auto accum = std::make_shared<SimpleAccumulator>();
        ds->connect(accum);
        ds = accum;
    }

    // Add throughput monitor
    auto throughputMonitor = std::make_shared<DataStreamThroughputMonitor<DataT>>();
    ds->connect(throughputMonitor);
    ds = throughputMonitor;

    // Add a sink to wait for the end of processing
    auto sink = std::make_shared<cds::StreamSink<DataT>>();
    ds->connect(sink);

    // Start the source
    source->start();

    // Wait for the sink to finish
    auto eopStatus = sink->wait();
    LOG_INFO("EOP status: {}", eopStatus.toString());

    return 0;
}