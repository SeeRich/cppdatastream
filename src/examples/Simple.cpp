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

using BaseDataT = std::any;

using namespace std;
namespace cds = cppdatastream;

using DataT = AlignedVector<uint8_t>;

std::atomic_flag cancel = ATOMIC_FLAG_INIT;

class SimpleDataStreamProcessor : public cds::StreamProcessor
{
public:
    CPPDATASTREAM_CLASS_NAME_OVERRIDE();

    virtual ~SimpleDataStreamProcessor() { CDS_LOG_DTOR("{} DTOR: sum = {}", className(), _sum); }

    virtual auto processData(const cds::SharedDataBlock& sdb) -> cds::SharedDataBlock override
    {
        if(sdb.isEndOfProcessing())
            return sdb;

        // Get a read-only copy of the data
        auto data = sdb.asType<DataT>();
        // Calculate the sum of the data
        _sum += std::accumulate(data.cbegin(), data.cend(), 0ul);
        return sdb;
    }

private:
    uint64_t _sum{0};
};

// Pass through datastream that does no processing
class DataStreamPassThrough : public cds::StreamProcessor
{
public:
    CPPDATASTREAM_CLASS_NAME_OVERRIDE();

    explicit DataStreamPassThrough(uint64_t index) : _index(index) {}

    virtual ~DataStreamPassThrough() { CDS_LOG_DTOR("{} DTOR: index: {}", className(), _index); }

    virtual auto processData(const cds::SharedDataBlock& sdb) -> cds::SharedDataBlock override { return sdb; }

private:
    [[maybe_unused]] size_t _index{0};
};

class DataStreamThroughputMonitor : public cds::StreamProcessor
{
public:
    CPPDATASTREAM_CLASS_NAME_OVERRIDE();

    CDS_LOG_DTOR_VFUNC(DataStreamThroughputMonitor);

    virtual auto processData(const cds::SharedDataBlock& sdb) -> cds::SharedDataBlock override
    {
        if(sdb.isEndOfProcessing())
            return sdb;

        // Get a read-only copy of the data
        const auto& data = sdb.asType<DataT>();
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
    CLI::App app("DataStream processing pipeline prototpye");
    app.set_version_flag("--version",
                         fmt::format("Version: {}", fmt::format(fg(fmt::terminal_color::green), "{}", "0.1.0")));
    // Number of data blocks to pass through the pipeline
    uint32_t numBlocks = 10'000'000;
    app.add_option<uint32_t>("-d,--datablocks", numBlocks, "Number of data blocks");
    // Number of datastreams to use in the pipeline
    uint32_t numStreams = 30;
    app.add_option<uint32_t>("-s,--streams", numStreams, "Number of data streams");
    // Number of bytes in each block
    uint32_t numBytesPerBlock = 16384;
    app.add_option<uint32_t>("-b,--bytes", numBytesPerBlock, "Number of bytes per block");
    // Parse the CLI string
    CLI11_PARSE(app, argc, argv);

    // Allocate the number of data blocks up front
    LOG_INFO("Processing {} blocks of {} bytes each", numBlocks, numBytesPerBlock);

    // Create a DataStreamSource that we will push data into.
    auto tb = std::make_shared<cds::StreamThreadedBuffer>(1'000, true);

    // Connect downstream datastreams to the buffer
    std::shared_ptr<cds::StreamProcessor> downstream = tb;
    for(uint32_t i = 0; i < numStreams; ++i)
        downstream = downstream->connect(std::make_unique<DataStreamPassThrough>(i));

    // Add simple processor datastream
    // downstream = downstream->connect(std::make_unique<SimpleDataStreamProcessor>());

    // Another threaded buffer
    downstream = downstream->connect(std::make_unique<cds::StreamThreadedBuffer>(1'000, true));

    // Add throughput monitor
    downstream->connect(std::make_unique<DataStreamThroughputMonitor>());

    // Add a sink to wait for the end of processing
    auto sink = std::make_shared<cds::StreamSink>();
    downstream->connect(sink);

    auto vec = DataT(numBytesPerBlock, 1);
    // Create a writable data block;
    auto wb = cds::WritableDataBlock();
    // Set the data
    wb.setData(vec);

    auto start = std::chrono::steady_clock::now();

    // Push the block through the pipeline
    for(uint64_t i = 0; i < numBlocks; ++i) {
        if(cancel.test())
            break;
        tb->pushData(wb);
    }

    // Push an EndOfProcessing block
    wb = cds::WritableDataBlock();
    wb.setEndOfProcessing(cds::EopStatus{});
    tb->pushData(wb);

    sink->wait();

    // Log the total amount of data processed
    std::chrono::duration<float> duration = std::chrono::steady_clock::now() - start;
    LOG_INFO("Processed {} in {} seconds\n",
             prettyPrintBytes(numBlocks * static_cast<uint64_t>(numBytesPerBlock)),
             duration.count());

    return 0;
}