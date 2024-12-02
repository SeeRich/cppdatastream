# Find dependencies that the common code uses
find_package(cpptrace REQUIRED)
find_package(fmt REQUIRED)
find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)

# Add a obj library for all the examples/tests/benchmarks to make use of
add_library(datastream_obj OBJECT)
target_include_directories(datastream_obj PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_sources(datastream_obj PRIVATE Utilities.cpp)
target_link_libraries(datastream_obj PUBLIC cpptrace::cpptrace)
target_link_libraries(datastream_obj PUBLIC fmt::fmt Boost::boost spdlog::spdlog)
target_compile_definitions(datastream_obj PUBLIC ENABLE_CPPDATASTREAM_LOGGING ENABLE_CPPDATASTREAM_DTOR_LOGGING)
target_compile_definitions(datastream_obj PUBLIC CPPTRACE_UNWIND_WITH_UNWIND)

# Add executable to Signal Safe Stack tracing
# add_executable(signal_tracer SignalTracer.cpp)
# target_link_libraries(signal_tracer PRIVATE datastream_obj)