project(jbson CXX)
cmake_minimum_required(VERSION 2.8)

FILE(GLOB_RECURSE jbson_headers ${CMAKE_CURRENT_SOURCE_DIR}/include/jbson/*.hpp)

add_library(jbson_headers_lib STATIC ${jbson_headers})
set_target_properties(jbson_headers_lib PROPERTIES LINKER_LANGUAGE CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SRC_LIST test/test1.cpp)

enable_testing()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -stdlib=libc++ -fsanitize=address")
add_executable(${PROJECT_NAME}_test ${SRC_LIST})
