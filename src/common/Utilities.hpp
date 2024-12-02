#pragma once

#include <cstdint>
#include <string>

/// @brief Pretty prints number of bytes
std::string prettyPrintBytes(uint64_t bytes);

/// @brief register common signal handlers
void registerSignalHandlers();
