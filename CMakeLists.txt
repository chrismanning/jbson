project(jbson CXX)
cmake_minimum_required(VERSION 2.8)

option(JBSON_ENABLE_TESTING "Enables testing of jbson" OFF)

FILE(GLOB_RECURSE jbson_headers ${CMAKE_CURRENT_SOURCE_DIR}/include/jbson/*.hpp)

add_library(jbson_headers_lib STATIC ${jbson_headers})
set_target_properties(jbson_headers_lib PROPERTIES LINKER_LANGUAGE CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -stdlib=libc++ -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")

if(${JBSON_ENABLE_TESTING})
enable_testing()
endif()

get_cmake_property(CACHE_VARS CACHE_VARIABLES)
foreach(CACHE_VAR ${CACHE_VARS})
  get_property(CACHE_VAR_HELPSTRING CACHE ${CACHE_VAR} PROPERTY HELPSTRING)
  if(CACHE_VAR_HELPSTRING STREQUAL "No help, variable specified on the command line.")
    get_property(CACHE_VAR_TYPE CACHE ${CACHE_VAR} PROPERTY TYPE)
    if(CACHE_VAR_TYPE STREQUAL "UNINITIALIZED")
      set(CACHE_VAR_TYPE)
    else()
      set(CACHE_VAR_TYPE :${CACHE_VAR_TYPE})
    endif()
    set(PARENT_CMAKE_ARGS ${PARENT_CMAKE_ARGS} "-D${CACHE_VAR}${CACHE_VAR_TYPE}=${${CACHE_VAR}}")
  endif()
endforeach()

include(ExternalProject)
ExternalProject_Add(
    gtest_${PROJECT_NAME}
    SVN_REPOSITORY http://googletest.googlecode.com/svn/trunk/
    SVN_REVISION -r 660
    UPDATE_COMMAND ""
    TIMEOUT 10
    CMAKE_ARGS --no-warn-unused-cli ${PARENT_CMAKE_ARGS} "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}" -DBUILD_SHARED_LIBS=OFF
    # Disable install step
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON)
ExternalProject_Get_Property(gtest_${PROJECT_NAME} source_dir)
set(GTEST_INCLUDE_DIRS ${source_dir}/include)
message(STATUS "Gtest include dirs: ${GTEST_INCLUDE_DIRS}")

include_directories(${GTEST_INCLUDE_DIRS})
add_library(${PROJECT_NAME}_test_main OBJECT test/test_main.cpp)

add_dependencies(${PROJECT_NAME}_test_main gtest_${PROJECT_NAME})

ExternalProject_Get_Property(gtest_${PROJECT_NAME} binary_dir)
set(GTEST_LIBRARIES "-L${binary_dir} -lgtest")
message(STATUS "Gtest libs: ${GTEST_LIBRARIES}")

add_definitions(-DJBSON_FILES="${CMAKE_CURRENT_SOURCE_DIR}/test/")

function(cxx_test test_name)
  add_executable(${PROJECT_NAME}_${test_name} test/${test_name}.cpp $<TARGET_OBJECTS:${PROJECT_NAME}_test_main>)
  target_link_libraries(${PROJECT_NAME}_${test_name} ${GTEST_LIBRARIES})
  add_test(${PROJECT_NAME}_${test_name} ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}_${test_name})
endfunction()

cxx_test(element_test)
cxx_test(document_test)
cxx_test(builder_test)
cxx_test(json_reader_test)
cxx_test(json_writer_test)

add_executable(${PROJECT_NAME}_perf_test test/perf_test.cpp $<TARGET_OBJECTS:${PROJECT_NAME}_test_main>)
target_link_libraries(${PROJECT_NAME}_perf_test ${GTEST_LIBRARIES})
