#pragma once

#include <cstdint>
#include <functional>
#include <string>

/// @brief Pretty prints number of bytes
std::string prettyPrintBytes(uint64_t bytes);

/// @brief register common signal handlers
void registerSignalHandlers();

/// @brief register program interrupt handler (CTRL-C)
void registerProgramInterruptHandler(std::function<void()> handler);
