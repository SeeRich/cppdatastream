#include "cppdatastream/AlignedAllocator.hpp"
#include "cppdatastream/DataStream.hpp"

#include <cppdatastream/external/CLI11.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <list>
#include <new>
#include <string>

using BaseDataT = std::any;

/// @brief Pretty prints number of bytes
std::string prettyPrintBytes(uint64_t bytes)
{
    constexpr std::array<char[3], 7> suffix = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};

    uint32_t i = 0;
    auto dblBytes = static_cast<long double>(bytes);

    if(bytes > 1024) {
        for(i = 0; (bytes / 1024) > 0 && i < suffix.size() - 1; i++, bytes /= 1024)
            dblBytes = static_cast<double>(bytes) / 1024.0;
    }

    return fmt::format("{:02f} {}", dblBytes, suffix[i]);
}

using namespace std;

using DataT = AlignedVector<uint8_t>;

std::atomic_flag cancel = ATOMIC_FLAG_INIT;

/// @brief Handler function for SigInterrupt
void sigIntHandler(int s)
{
    std::signal(s, SIG_DFL);
    logWarn("Received signal interrupt, shutting down...\n");
    cancel.test_and_set();
}

class SimpleDataStreamProcessor : public DataStream
{
public:
    virtual ~SimpleDataStreamProcessor() { logDtor("{} DTOR: sum = {}\n", className(), _index, _sum); }

    virtual auto processData(const SharedDataBlock& sdb) -> SharedDataBlock override
    {
        if(sdb.isEndOfProcessing())
            return sdb;

        // Get a read-only copy of the data
        const auto& data = sdb.data<DataT>();
        // Calculate the sum of the data
        _sum += std::accumulate(data.cbegin(), data.cend(), 0ul);
        return sdb;
    }

private:
    uint64_t _sum{0};
};

// Pass through datastream that does no processing
class DataStreamPassThrough : public DataStream
{
public:
    explicit DataStreamPassThrough(uint64_t index) : _index(index) {}

    virtual ~DataStreamPassThrough() { logDtor("{} DTOR: index: {}\n", className(), _index); }

    virtual auto processData(const SharedDataBlock& sdb) -> SharedDataBlock override { return sdb; }

private:
    [[maybe_unused]] size_t _index{0};
};

class DataStreamThroughputMonitor : public DataStream
{
public:
    virtual ~DataStreamThroughputMonitor() { logDtor("{} DTOR\n", className()); }

    virtual auto processData(const SharedDataBlock& sdb) -> SharedDataBlock override
    {
        if(sdb.isEndOfProcessing())
            return sdb;

        // Get a read-only copy of the data
        const auto& data = sdb.data<DataT>();
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
            logInfo("Data Throughput: {}/s\n", prettyPrintBytes(_throughput_bytes));
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
    // Let us handle CTRL-C (i.e. SIGINT) specially.
    std::signal(SIGINT, sigIntHandler);

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
    logInfo("Processing {} blocks of {} bytes each\n", numBlocks, numBytesPerBlock);

    // Create a DataStreamSource that we will push data into.
    auto tb = DataStreamThreadedBuffer(1'000, false);

    // Connect downstream datastreams to the buffer
    DataStream* downstream = &tb;
    for(uint32_t i = 0; i < numStreams; ++i)
        downstream = downstream->connect(std::make_unique<DataStreamPassThrough>(i));

    // Add simple processor datastream
    downstream = downstream->connect(std::make_unique<SimpleDataStreamProcessor>());

    // Add throughput monitor
    downstream->connect(std::make_unique<DataStreamThroughputMonitor>());

    auto vec = DataT(numBytesPerBlock, 1);

    auto start = std::chrono::steady_clock::now();

    auto numThreads = 2u;
    vector<thread> threads;
    for(size_t i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread([&]() {
            // Push the block through the pipeline
            for(uint64_t i = 0; i < numBlocks; ++i) {
                if(cancel.test())
                    break;
                // Create a writable data block;
                auto wb = WritableDataBlock();
                // Set the data
                wb.setData(vec);
                // Push the data
                tb.pushData(wb);
            }
        }));
    }

    for(auto& t : threads) t.join();

    // Push an EndOfProcessing block
    auto wb = WritableDataBlock();
    wb.setEndOfProcessing();
    tb.pushData(wb);

    // Log the total amount of data processed
    std::chrono::duration<float> duration = std::chrono::steady_clock::now() - start;
    logInfo("Processed {} in {} seconds\n",
            prettyPrintBytes(numBlocks * static_cast<uint64_t>(numBytesPerBlock)),
            duration.count());

    return 0;
}