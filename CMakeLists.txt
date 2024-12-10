# Only 3.10 is required for consumers
cmake_minimum_required(VERSION 3.10)

# Define the project
project(cppdatastream VERSION 0.0.1 LANGUAGES CXX)
set(CPPDATASTREAM_VERSION ${PROJECT_VERSION} CACHE INTERNAL "cppdatastream version")
message(STATUS "cppdatastream version: ${CPPDATASTREAM_VERSION}")

include(GNUInstallDirs)

# Main header-only library
add_library(cppdatastream INTERFACE)
target_compile_definitions(cppdatastream INTERFACE CPPDATASTREAM_VERSION="${CPPDATASTREAM_VERSION}")
target_include_directories(cppdatastream INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/cppdatastream>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/cppdatastream>
)

# Are we the top-level project?
# If so, we setup development settings
set(IS_TOP_LEVEL_PROJECT FALSE)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(IS_TOP_LEVEL_PROJECT TRUE)

    # Much newer cmake is required for development
    cmake_minimum_required(VERSION 3.30)

    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION $<$<CONFIG:Debug>,ON, OFF>)    
endif()

if(IS_TOP_LEVEL_PROJECT)
    add_subdirectory(src/common)
endif()

if(BUILD_EXAMPLES AND IS_TOP_LEVEL_PROJECT)
    add_subdirectory(src/examples)
endif()