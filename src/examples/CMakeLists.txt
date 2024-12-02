add_executable(ex_simple Simple.cpp)
target_link_libraries(ex_simple PRIVATE datastream_obj)



# target_include_directories(${MAIN_TARGET} PRIVATE 
#     $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)

# target_link_libraries(${MAIN_TARGET} PRIVATE 
#     CONAN_PKG::fmt CONAN_PKG::boost)

# # Include some platform specific optimization
# if(MSVC)
#     target_compile_options(${MAIN_TARGET} PRIVATE -W4 -WX /arch:AVX2 $<$<CONFIG:Release>:/GL>)
#     target_link_options(${MAIN_TARGET} PRIVATE /arch:AVX2 $<$<CONFIG:Release>:/LTCG:STATUS>)
# else()
#     target_compile_options(${MAIN_TARGET} PRIVATE -Wall -Wextra -pedantic -Werror -Wsuggest-override -march=native)
#     target_link_options(${MAIN_TARGET} PRIVATE -march=native)
# endif()
