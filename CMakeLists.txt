# cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install -G Ninja
# cmake --build build --config Release --clean-first
cmake_minimum_required(VERSION 3.30)

# Define the project
project(datastream VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION $<$<CONFIG:Debug>,ON, OFF>)

# Always link the multi-threaded C runtime on windows as all our dependencies are doing this as well.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

include(GNUInstallDirs)

# Are we the top-level project?
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(IS_TOP_LEVEL_PROJECT TRUE)
else()
    set(IS_TOP_LEVEL_PROJECT FALSE)
endif()

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(src/common)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(src/examples)
endif()