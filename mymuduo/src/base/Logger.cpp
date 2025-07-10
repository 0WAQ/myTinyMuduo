#include "mymuduo/base/Logger.h"
#include "mymuduo/base/Timestamp.h"

namespace mymuduo {

        // 加速日志时间的拼接
__thread char t_time[64];
__thread time_t t_lastSeconds;

const char* LogLevelName[Logger::NUM_LOG_LEVELS] = {
    "[DEBUG]: ",
    "[INFO ]: ",
    "[WARN ]: ",
    "[ERROR]: ",
    "[UNKN ]: "
};

void default_output(const char* data, size_t len) {
    size_t n = std::fwrite(data, 1, len, stdout);
    (void)n;
}

void default_flush() {
    std::fflush(stdout);
}

using namespace std::placeholders;

Logger::OutputFunc g_output = std::bind(default_output, _1, _2);
Logger::FlushFunc g_flush = default_flush;
Logger::LogLevel g_level = Logger::NUM_LOG_LEVELS;  // 默认不启用日志


} // namespace mymuduo

using namespace mymuduo;

Logger::Logger(Logger::LogLevel level)
    : _impl(level)
{ }

Logger::Impl::Impl(Logger::LogLevel level)
    : _level(level)
    , _ts(Timestamp::now())
{
    _format << LogLevelName[level];
    format_time();
}

void Logger::Impl::format_time() {
    using namespace std::chrono;
    auto duration = _ts.time_since_epoch();
    auto secs = duration_cast<seconds>(duration);
    auto microsecs = duration_cast<microseconds>(duration - secs);

    if (secs.count() != t_lastSeconds) {
        t_lastSeconds = secs.count();

        std::time_t t = system_clock::to_time_t(system_clock::time_point(secs));
        std::tm* tm = std::localtime(&t);

        std::snprintf(t_time, sizeof(t_time), "%04d-%02d-%02d %02d:%02d:%02d",
                  tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                  tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
    _format << T(t_time, 19);

    Fmt us(".%06d ", microsecs.count());
    assert(us.size() == 8);
    _format << T(us.data(), 8);
}

void Logger::Impl::finish() {
    _format.format("\n");
}

Logger::~Logger() {
    _impl.finish();
    const LogFormat::Buffer& buf(_impl._format.buffer());
    g_output(buf.data(), buf.size());
    if (_impl._level == ERROR) {
        g_flush();
        abort();
    }
}

Logger::LogLevel Logger::log_level() {
    return g_level;
}

void Logger::set_log_level(LogLevel level) {
    g_level = level;
}

void Logger::set_output(OutputFunc func) {
    g_output = std::move(func);
}

void Logger::set_flush(FlushFunc func) {
    g_flush = std::move(func);
}
