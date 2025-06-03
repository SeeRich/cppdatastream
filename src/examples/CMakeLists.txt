add_executable(ex_simple Simple.cpp)
target_link_libraries(ex_simple PRIVATE datastream_obj)

add_executable(ex_source Source.cpp)
target_link_libraries(ex_source PRIVATE datastream_obj)