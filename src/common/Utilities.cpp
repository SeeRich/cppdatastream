#include "Utilities.hpp"

#include <cpptrace/cpptrace.hpp>

#include <signal.h>
#include <array>
#include <csignal>
#include <cstring>
#include <format>
#include <iostream>
#include <tuple>

#ifdef _WIN32
    #include <io.h>
    #include <windows.h>
#else
    #include <unistd.h>
#endif

namespace detail {

void handler(int signo)
{
    // Print basic message
#ifdef _WIN32
    auto message = std::format("SIGNAL: {}\n", signo);
    std::ignore = _write(_fileno(stderr), message.c_str(), static_cast<unsigned int>(message.size()));
#else
    auto message = std::format("SIGNAL: {}\n", strsignal(signo));
    std::ignore = write(STDERR_FILENO, message.c_str(), message.size());
#endif
    // Generate trace (this is definitely not signal safe)
    cpptrace::generate_trace().print();
    // Up to you if you want to exit or continue or whatever
    std::exit(signo);
}

void warmup_cpptrace()
{
    // This is done for any dynamic-loading shenanigans
    cpptrace::frame_ptr buffer[10];
    cpptrace::safe_generate_raw_trace(buffer, 10);
    cpptrace::safe_object_frame frame;
    cpptrace::get_safe_object_frame(buffer[0], &frame);
}

}  // namespace detail

std::string prettyPrintBytes(uint64_t bytes)
{
    constexpr std::array<char[3], 7> suffix = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};

    uint32_t i = 0;
    auto dblBytes = static_cast<long double>(bytes);

    if(bytes > 1024) {
        for(i = 0; (bytes / 1024) > 0 && i < suffix.size() - 1; i++, bytes /= 1024)
            dblBytes = static_cast<double>(bytes) / 1024.0;
    }

    return std::format("{:02f} {}", dblBytes, suffix[i]);
}

// Static handler for SIGINT
std::function<void(int)> sigIntHandlerFunc;

void sigIntHandler(int signo) { sigIntHandlerFunc(signo); }

void registerSignalHandlers()
{
    // Default handler for SIGINT
    sigIntHandlerFunc = [](int signo) {
#ifdef _WIN32
        auto message = std::format("SIGNAL: {}\n", signo);
        std::ignore = _write(_fileno(stderr), message.c_str(), static_cast<unsigned int>(message.size()));
#else
        auto message = std::format("SIGNAL: {}\n", strsignal(signo));
        std::ignore = write(STDERR_FILENO, message.c_str(), message.size());
#endif
        std::signal(SIGINT, SIG_DFL);
    };
    std::signal(SIGINT, sigIntHandler);

    detail::warmup_cpptrace();

    // Setup other signal handlers - only register signals that are supported on the current platform
    std::signal(SIGSEGV, detail::handler);
    std::signal(SIGABRT, detail::handler);
    std::signal(SIGFPE, detail::handler);
    std::signal(SIGILL, detail::handler);
    std::signal(SIGTERM, detail::handler);

#ifndef _WIN32
    // These signals are not available on Windows
    std::signal(SIGBUS, detail::handler);
    std::signal(SIGTRAP, detail::handler);
    std::signal(SIGSYS, detail::handler);
    std::signal(SIGXCPU, detail::handler);
    std::signal(SIGXFSZ, detail::handler);
    std::signal(SIGPIPE, detail::handler);
#endif
}

void registerProgramInterruptHandler(std::function<void()> handler)
{
    // Override previous handler if present
    sigIntHandlerFunc = [handler](int) {
        handler();
        std::signal(SIGINT, SIG_DFL);
    };
}
