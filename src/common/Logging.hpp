#pragma once

#include "cppdatastream/DataStream.hpp"

#include <spdlog/spdlog.h>

#define LOG_TRACE(...) spdlog::trace(__VA_ARGS__)
#define LOG_DEBUG(...) spdlog::debug(__VA_ARGS__)
#define LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define LOG_WARN(...) spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#define LOG_CRITICAL(...) spdlog::critical(__VA_ARGS__)

/// ILogger for cppdatastream
class CdsSpdLogger : public cppdatastream::ILogger
{
public:
    CdsSpdLogger() = default;

    virtual ~CdsSpdLogger() = default;

    virtual void log(cppdatastream::LogLevel level, const std::string& message) override
    {
        switch(level) {
            case cppdatastream::LogLevel::Trace:
                LOG_TRACE(message);
                break;
            case cppdatastream::LogLevel::Debug:
                LOG_DEBUG(message);
                break;
            case cppdatastream::LogLevel::Info:
                LOG_INFO(message);
                break;
            case cppdatastream::LogLevel::Warn:
                LOG_WARN(message);
                break;
            case cppdatastream::LogLevel::Error:
                LOG_ERROR(message);
                break;
            case cppdatastream::LogLevel::Fatal:
                LOG_CRITICAL(message);
                break;
            default:
                LOG_INFO(message);
                break;
        }
    }
};
