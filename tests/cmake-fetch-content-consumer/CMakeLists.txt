cmake_minimum_required(VERSION 3.10)

project(cmake-fetch-content-consumer)

include(FetchContent)

FetchContent_Declare(
  cppdatastream
  GIT_REPOSITORY https://github.com/SeeRich/cppdatastream.git
  GIT_TAG        4a227dc3d68280ba1c70474965f7540948f3ca78
)
FetchContent_MakeAvailable(cppdatastream)

add_executable(cmake-fetch-content-consumer main.cpp)
target_link_libraries(cmake-fetch-content-consumer cppdatastream)