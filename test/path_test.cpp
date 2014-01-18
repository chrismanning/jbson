//          Copyright Christian Manning 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <string>
using namespace std::literals;

#include <boost/exception/diagnostic_information.hpp>
#include <boost/range/as_array.hpp>

#include <gtest/gtest.h>

#include <jbson/path.hpp>
#include <jbson/json_reader.hpp>
using namespace jbson;

TEST(PathTest, JbsonPathRefTest1) {
    auto res = path_select("{}"_json_doc, "$");
    static_assert(!std::is_rvalue_reference<decltype("{}"_json_doc)>::value, "");
    static_assert(std::is_same<decltype(res), std::vector<basic_element<std::vector<char>>>>::value, "");
    ASSERT_TRUE(res.empty());
}

TEST(PathTest, JbsonPathRefTest2) {
    auto res = path_select(document("{}"_json_doc), "$");
    static_assert(std::is_same<decltype(res), std::vector<basic_element<std::vector<char>>>>::value, "");
    ASSERT_TRUE(res.empty());
}

TEST(PathTest, JbsonPathRefTest3) {
    auto doc = document("{}"_json_doc);
    auto res = path_select(doc, "$");
    static_assert(!std::is_same<decltype(res), std::vector<basic_element<std::vector<char>>>>::value, "");
    ASSERT_TRUE(res.empty());
}

TEST(PathTest, JbsonPathTest1) {
    auto res = path_select("{}"_json_doc, "$");
    ASSERT_TRUE(res.empty());
}

TEST(PathTest, JbsonPathTest2) {
    auto res = path_select(R"({"elem": 123})"_json_doc, "$");
    ASSERT_EQ(1, res.size());

    res = path_select(R"({"elem": 123})"_json_doc, "$.elem");
    ASSERT_EQ(1, res.size());

    res = path_select(R"({"elem": 123})"_json_doc, "$.nothing");
    ASSERT_TRUE(res.empty());

    res = path_select(R"({"elem": 123})"_json_doc, "*");
    ASSERT_EQ(1, res.size());

    res = path_select(R"({"elem": 123})"_json_doc, "$.*");
    ASSERT_EQ(1, res.size());
}

TEST(PathTest, JbsonPathTest3) {
    auto res = path_select(R"({"elem": {"nest" : 321}})"_json_doc, "$");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::document_element, res.front().type());

    res = path_select(R"({"elem": {"nest" : 321}})"_json_doc, "$.elem.nest");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(321, res.front().value<int32_t>());
}

TEST(PathTest, JbsonPathTest4) {
    auto res = path_select(R"({"elem": {"nest" : 321}})"_json_doc, "$");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::document_element, res.front().type());

    res = path_select(R"({"elem": {"nest" : 321}})"_json_doc, R"($.elem["nest"])");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(321, res.front().value<int32_t>());
}

TEST(PathTest, JbsonPathTest5) {
    auto res = path_select(R"({"elem": {"nest" : 321}})"_json_doc, "$");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::document_element, res.front().type());

    res = path_select(R"({"elem": {"nest" : 321}})"_json_doc, R"($["elem"]["nest"])");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(321, res.front().value<int32_t>());

    res = path_select(R"({"elem": {"nest1":321, "nest2":123, "nest3":456}})"_json_doc, R"($["elem"]["nest2"])");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(123, res.front().value<int32_t>());

    res = path_select(R"({"elem": {"nest1":321, "nest2":123, "nest3":456}})"_json_doc, R"($["elem"]["nest2","nest3"])");
    ASSERT_EQ(2, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(123, res.front().value<int32_t>());
    EXPECT_EQ(element_type::int32_element, res.back().type());
    EXPECT_EQ(456, res.back().value<int32_t>());

    res = path_select(R"({"elem": {"nest" : 321}})"_json_doc, R"($["elem"].nest)");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(321, res.front().value<int32_t>());
}

TEST(PathTest, JbsonPathTest6) {
    auto res = path_select(R"({"arr": [76, 923, 12]})"_json_doc, "$.arr[1]");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(923, res.front().value<int32_t>());
}

TEST(PathTest, JbsonPathTest7) {
    auto res = path_select(R"({"arr": [76, 923, 12]})"_json_doc, "$.arr[1,2]");
    ASSERT_EQ(2, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(923, res.front().value<int32_t>());
    EXPECT_EQ(element_type::int32_element, res.back().type());
    EXPECT_EQ(12, res.back().value<int32_t>());
}

TEST(PathTest, JbsonPathTest8) {
    auto res = path_select(R"({"arr": [76, [923], {"obj":12}]})"_json_doc, "$.arr[1,2]");
    ASSERT_EQ(2, res.size());
    EXPECT_EQ(element_type::array_element, res.front().type());
    EXPECT_EQ(element_type::document_element, res.back().type());
}

TEST(PathTest, JbsonPathTest9) {
    auto res = path_select(R"({"arr": [76, [923], {"obj":12}]})"_json_doc, "$.arr[*].obj");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(12, res.front().value<int32_t>());
}

TEST(PathTest, JbsonPathTest10) {
    auto res = path_select(R"({"arr": [76, [923], {"obj":12}, 765]})"_json_doc, "$.arr..obj");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(12, res.front().value<int32_t>());

    res = path_select(R"({"arr": [76, [923], {"obj":12}, [765, {"obj": 24}]]})"_json_doc, "$.arr..obj");
    ASSERT_EQ(2, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(12, res.front().value<int32_t>());
    EXPECT_EQ(element_type::int32_element, res.back().type());
    EXPECT_EQ(24, res.back().value<int32_t>());
}
