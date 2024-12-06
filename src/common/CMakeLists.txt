# Include Conan
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/deps)

# Dependencies
find_package(fmt REQUIRED)
find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(libpcap REQUIRED)
find_package(jemalloc REQUIRED)
find_package(Threads REQUIRED)
find_package(cpptrace REQUIRED)

# Add a obj library for all the examples/tests/benchmarks to make use of
add_library(datastream_obj OBJECT)
target_include_directories(datastream_obj PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
target_sources(datastream_obj PRIVATE Utilities.cpp)
target_link_libraries(datastream_obj PUBLIC cppdatastream)
target_link_libraries(datastream_obj PUBLIC cpptrace::cpptrace)
target_link_libraries(datastream_obj PUBLIC fmt::fmt Boost::boost spdlog::spdlog)
target_link_libraries(datastream_obj PUBLIC libpcap::libpcap jemalloc::jemalloc)
target_compile_definitions(datastream_obj PUBLIC ENABLE_CPPDATASTREAM_LOGGING ENABLE_CPPDATASTREAM_DTOR_LOGGING)
target_compile_definitions(datastream_obj PUBLIC CPPTRACE_UNWIND_WITH_UNWIND)

# Include some platform specific optimization
if(MSVC)
    target_compile_options(datastream_obj PRIVATE -W4 -WX /arch:AVX2 $<$<CONFIG:Release>:/GL>)
    target_link_options(datastream_obj PRIVATE /arch:AVX2 $<$<CONFIG:Release>:/LTCG:STATUS>)
else()
    target_compile_options(datastream_obj PRIVATE -Wall -Wextra -pedantic -Wsuggest-override -march=native)
    target_link_options(datastream_obj PRIVATE -march=native)
endif()

# Custom target for formatting code
file(GLOB_RECURSE FORMAT_SOURCES
    LIST_DIRECTORIES true
    ${CMAKE_SOURCE_DIR}/include/*.hpp
    ${CMAKE_SOURCE_DIR}/src/*.cpp)
# message(STATUS "Formatting sources: ${FORMAT_SOURCES}")
add_custom_target(format_code
  COMMAND clang-format -i -style=file ${FORMAT_SOURCES}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Add executable to Signal Safe Stack tracing
# add_executable(signal_tracer SignalTracer.cpp)
# target_link_libraries(signal_tracer PRIVATE datastream_obj)