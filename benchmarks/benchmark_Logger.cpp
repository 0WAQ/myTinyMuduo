#include "mymuduo/base/AsyncLogging.h"
#include "mymuduo/base/Logger.h"

#include <cstddef>
#include <filesystem>
#include <thread>
#include <vector>
#include <format>
#include <barrier>
#include <print>
#include <benchmark/benchmark.h>

using namespace mymuduo;
using namespace std::chrono_literals;
using namespace std::placeholders;

namespace fs = std::filesystem;
namespace bm = benchmark;


void BM_Logger(bm::State& state) {
    const int64_t ThreadCount = state.threads();         // 线程数
    const int64_t MsgSize = state.range(0);         // 每条日志字节数
    const int64_t LogCount = state.range(1);        // 每个线程输出日志条数
    const int64_t MsgCountPerThread = LogCount / state.threads();

    static std::shared_ptr<AsyncLogging> async;
    static fs::path filepath { "/tmp/mymuduo_bm_Logger" };
    static std::string filename;
    static std::string msg;

    static std::barrier sync_point { static_cast<std::ptrdiff_t>(ThreadCount) };

    // 初始化
    if (state.thread_index() == 0) {
        filename = std::format("{}-{}-{}", ThreadCount, LogCount, MsgSize);
        if (fs::exists(filepath)) {
            fs::remove_all(filepath);
        }

        async = std::make_shared<AsyncLogging>(filepath, filename);
        msg = std::string(MsgSize, 'A');
        Logger::set_log_level(Logger::INFO);
        Logger::set_output([](const char* data, size_t len) {
            async->append(data, len);
        });
        async->start();
    }

    /////////////////////////////////////////////////////////
    sync_point.arrive_and_wait();
    for (auto _ : state) {
        for (int64_t i = 0; i < MsgCountPerThread; ++i) {
            LOG_INFO("{}", msg);
        }
    }
    sync_point.arrive_and_wait();
    /////////////////////////////////////////////////////////

    if (state.thread_index() == 0) {
        async.reset();
        fs::remove_all(filepath);
    }

    const double total_bytes = static_cast<double>(LogCount * MsgSize);

    // 吞吐量
    state.counters["Throughput"] = bm::Counter(total_bytes,
                                               bm::Counter::kIsRate,
                                               bm::Counter::OneK::kIs1024);
    // 消息速率
    state.counters["MsgRate"] = bm::Counter(LogCount,
                                            bm::Counter::kIsRate);
}


// TAG
BENCHMARK(BM_Logger)
    ->ArgsProduct({
            { 110 },
            { 1'000'000 }
        })
    ->Unit(bm::kSecond)
    ->ThreadRange(1, std::thread::hardware_concurrency())
    ->ThreadPerCpu()
    ->MeasureProcessCPUTime()
    ->UseRealTime();
