#pragma once

#include <print>
#include <string>

enum class LogLevel
{
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR
};

inline void
LOG(std::string msg, LogLevel level = LogLevel::INFO) noexcept
{
    std::string prefix;
    switch (level)
    {
        case LogLevel::DEBUG:
            prefix = "DEBUG";
            break;

        case LogLevel::INFO:
            prefix = "INFO";
            break;

        case LogLevel::WARNING:
            prefix = "WARNING";
            break;

        case LogLevel::ERROR:
            prefix = "ERROR";
            break;

        default:
            prefix = "UNKNOWN";
            break;
    }
    std::println("{}: {}", prefix, msg);
}
