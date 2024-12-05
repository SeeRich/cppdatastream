#include "Utilities.hpp"

#include <cpptrace/cpptrace.hpp>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <array>
#include <csignal>
#include <cstring>
#include <format>
#include <iostream>
#include <tuple>

namespace detail {

// CPPTRACE details for signal safe stack trace, SEE:
// https://github.com/jeremy-rifkin/cpptrace/blob/main/docs/signal-safe-tracing.md

// This is just a utility I like, it makes the pipe API more expressive.
// struct pipe_t
// {
//     union {
//         struct
//         {
//             int read_end;
//             int write_end;
//         };
//         int data[2];
//     };
// };

// void do_signal_safe_trace(cpptrace::frame_ptr* buffer, std::size_t count)
// {
//     // Setup pipe and spawn child
//     pipe_t input_pipe;
//     std::ignore = pipe(input_pipe.data);
//     const pid_t pid = fork();
//     if(pid == -1) {
//         const char* fork_failure_message = "fork() failed\n";
//         std::ignore = write(STDERR_FILENO, fork_failure_message, strlen(fork_failure_message));
//         return;
//     }
//     if(pid == 0) {  // child
//         dup2(input_pipe.read_end, STDIN_FILENO);
//         close(input_pipe.read_end);
//         close(input_pipe.write_end);
//         execl("signal_tracer", "signal_tracer", nullptr);
//         const char* exec_failure_message =
//             "exec(signal_tracer) failed: Make sure the signal_tracer executable is in "
//             "the current working directory and the binary's permissions are correct.\n";
//         std::ignore = write(STDERR_FILENO, exec_failure_message, strlen(exec_failure_message));
//         _exit(1);
//     }
//     // Resolve to safe_object_frames and write those to the pipe
//     for(std::size_t i = 0; i < count; i++) {
//         cpptrace::safe_object_frame frame;
//         cpptrace::get_safe_object_frame(buffer[i], &frame);
//         std::ignore = write(input_pipe.write_end, &frame, sizeof(frame));
//     }
//     close(input_pipe.read_end);
//     close(input_pipe.write_end);
//     // Wait for child
//     waitpid(pid, nullptr, 0);
// }

void handler(int signo)
{
    // Print basic message
    auto message = std::format("SIGNAL: {}\n", strsignal(signo));
    std::ignore = write(STDERR_FILENO, message.c_str(), message.size());
    // Generate trace
    cpptrace::generate_trace().print();
    // WHY DOESN'T THE FOLLOWING WORK?
    // constexpr std::size_t N = 100;
    // cpptrace::frame_ptr buffer[N];
    // std::size_t count = cpptrace::safe_generate_raw_trace(buffer, N);
    // message = std::format("Generated {} frames\n", count);
    // std::ignore = write(STDERR_FILENO, message.c_str(), message.size());
    // do_signal_safe_trace(buffer, count);
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

void sigIntHandler(int signo)
{
    auto message = std::format("SIGNAL: {}\n", strsignal(signo));
    std::ignore = write(STDERR_FILENO, message.c_str(), message.size());
    std::exit(signo);
}

void registerSignalHandlers()
{
    // Let us handle CTRL-C (i.e. SIGINT) specially.
    std::signal(SIGINT, sigIntHandler);

    detail::warmup_cpptrace();

    // Setup other signal handlers
    std::signal(SIGSEGV, detail::handler);
    std::signal(SIGABRT, detail::handler);
    std::signal(SIGFPE, detail::handler);
    std::signal(SIGILL, detail::handler);
    std::signal(SIGBUS, detail::handler);
    std::signal(SIGTRAP, detail::handler);
    std::signal(SIGSYS, detail::handler);
    std::signal(SIGXCPU, detail::handler);
    std::signal(SIGXFSZ, detail::handler);
    std::signal(SIGPIPE, detail::handler);
}