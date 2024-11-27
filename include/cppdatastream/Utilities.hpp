#pragma once

#include <fmt/color.h>
#include <fmt/format.h>

// Helpful logging macros
#define logInfo(...) fmt::print(__VA_ARGS__)
#define logDebug(...) fmt::print(fg(fmt::terminal_color::cyan), __VA_ARGS__)
#define logWarn(...) fmt::print(fg(fmt::terminal_color::yellow), __VA_ARGS__)
#define logError(...) fmt::print(fg(fmt::terminal_color::red), __VA_ARGS__)
// Turn off DTOR logging for example
#define logDtor(...)  // fmt::print(fg(fmt::terminal_color::bright_magenta), __VA_ARGS__)

/// @brief Designed to be called as demangleName(typeid(...).name())
std::string demangleName(const std::string& name);