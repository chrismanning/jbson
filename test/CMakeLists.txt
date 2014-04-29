cxx_test(traits)
cxx_test(element_test)
cxx_test(document_test)
cxx_test(builder_test)
cxx_test(json_reader_test)
cxx_test(json_writer_test)
cxx_test(path_test)

add_executable(${PROJECT_NAME}_perf_test perf_test.cpp $<TARGET_OBJECTS:${PROJECT_NAME}_test_main>)
target_link_libraries(${PROJECT_NAME}_perf_test ${GTEST_LIBRARIES})

add_subdirectory(bson_tests)
