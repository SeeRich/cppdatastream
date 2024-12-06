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

std::atomic_flag cancel = ATOMIC_FLAG_INIT;

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

// This simple example demonstrates how to create a simple data processing pipeline using cppdatastream.
// The pipeline consists of a StreamThreadedBuffer -> SimpleAccumulator (optional, default=off) ->
// DataStreamThroughputMonitor -> StreamSink Data blocks are pushed manually into the StreamThreadedBuffer and the
// pipeline processes them. This example is intentionally simple and doesn't include a DataStreamSource since that is
// usually the hardest part to implement. Also, this example isn't optimized for performance. Each data block is
// allocated by the main thread before passing it to the pipeline. Ideally, the data blocks would be pre-allocated and
// reused to avoid the overhead of memory allocation.

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

    // Allocate the number of data blocks up front
    LOG_INFO("Processing {} blocks of {} bytes each", numBlocks, numBytesPerBlock);

    // Create a DataStreamSource that we will push data into.
    auto tb = std::make_shared<cds::StreamThreadedBuffer<DataT>>(1'000);

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

    // Allocate an aligned vector of bytes filled with ones1
    auto vec = DataT(numBytesPerBlock, 1);

    auto start = std::chrono::steady_clock::now();

    // Push the block through the pipeline
    for(uint64_t i = 0; i < numBlocks; ++i) {
        if(cancel.test())
            break;
        // Create a writable data block;
        auto wb = cds::WritableDataBlock<DataT>();
        // Set the data (copy the vector)
        wb.setData(vec);
        // Push the data block
        tb->pushData(wb);
    }

    // Push an EndOfProcessing block
    auto wb = cds::WritableDataBlock<DataT>();
    wb = cds::WritableDataBlock<DataT>();
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