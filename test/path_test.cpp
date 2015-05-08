//          Copyright Christian Manning 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <string>
using namespace std::literals;

#include <boost/exception/diagnostic_information.hpp>

#include <gtest/gtest.h>

#include <jbson/path.hpp>
#include <jbson/json_reader.hpp>
using namespace jbson;

TEST(PathTest, JbsonPathRefTest1) {
    auto res = path_select("{}"_json_doc, "$");
    static_assert(!std::is_rvalue_reference<decltype("{}"_json_doc)>::value, "");
    static_assert(std::is_same<decltype(res), std::vector<basic_element<std::vector<char>>>>::value, "");
    ASSERT_EQ(1, res.size());
}

TEST(PathTest, JbsonPathRefTest2) {
    auto res = path_select(document("{}"_json_doc), "$");
    static_assert(std::is_same<decltype(res), std::vector<basic_element<std::vector<char>>>>::value, "");
    ASSERT_EQ(1, res.size());
}

TEST(PathTest, JbsonPathRefTest3) {
    auto doc = document("{}"_json_doc);
    auto res = path_select(doc, "$");
    static_assert(!std::is_same<decltype(res), std::vector<basic_element<std::vector<char>>>>::value, "");
    ASSERT_EQ(1, res.size());
}

TEST(PathTest, JbsonPathTest1) {
    auto res = path_select("{}"_json_doc, "$");
    ASSERT_EQ(1, res.size());
}

TEST(PathTest, JbsonPathTestRoot) {
    auto res = path_select(R"({"elem": 123})"_json_doc, "$");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::document_element, res.front().type());
}

TEST(PathTest, JbsonPathTestRootDescend) {
    auto res = path_select(R"({"elem": 123})"_json_doc, "$.elem");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
}

TEST(PathTest, JbsonPathTestRootDescendNoMatch) {
    auto res = path_select(R"({"elem": 123})"_json_doc, "$.nothing");
    ASSERT_TRUE(res.empty());
}

TEST(PathTest, JbsonPathTestAll) {
    auto res = path_select(R"({"elem": 123})"_json_doc, "*");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
}

TEST(PathTest, JbsonPathTestRootDescendAll) {
    auto res = path_select(R"({"elem": 123})"_json_doc, "$.*");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
}

TEST(PathTest, JbsonPathTestRoot2) {
    auto res = path_select(R"({"elem": {"nest" : 321}})"_json_doc, "$");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::document_element, res.front().type());
}

TEST(PathTest, JbsonPathTestRootNest) {
    auto res = path_select(R"({"elem": {"nest" : 321}})"_json_doc, "$.elem.nest");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
    ASSERT_EQ(321, res.front().value<int32_t>());
}

TEST(PathTest, JbsonPathTestRootNestAltSyntax) {
    auto res = path_select(R"({"elem": {"nest" : 321}})"_json_doc, R"($.elem['nest'])");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(321, res.front().value<int32_t>());
}

TEST(PathTest, JbsonPathTestRootNestAltSyntax2) {
    auto res = path_select(R"({"elem": {"nest" : 321}})"_json_doc, R"($["elem"]["nest"])");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(321, res.front().value<int32_t>());
}

TEST(PathTest, JbsonPathTestRootNestAltSyntax3) {
    auto res = path_select(R"({"elem": {"nest1":321, "nest2":123, "nest3":456}})"_json_doc, R"($["elem"]["nest2"])");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(123, res.front().value<int32_t>());
}

TEST(PathTest, JbsonPathTestRootNestAltSyntaxMultiple) {
    auto res = path_select(R"({"nest1":321, "nest2":123, "nest3":456})"_json_doc, R"($["nest2","nest3"])");
    ASSERT_EQ(2, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(123, res.front().value<int32_t>());
    ASSERT_EQ(element_type::int32_element, res.back().type());
    EXPECT_EQ(456, res.back().value<int32_t>());
}

TEST(PathTest, JbsonPathTestRootNestAltSyntaxMultiple2) {
    auto res = path_select(R"({"elem": {"nest1":321, "nest2":123, "nest3":456}})"_json_doc, R"($["elem"]["nest2","nest3"])");
    ASSERT_EQ(2, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(123, res.front().value<int32_t>());
    ASSERT_EQ(element_type::int32_element, res.back().type());
    EXPECT_EQ(456, res.back().value<int32_t>());
}

TEST(PathTest, JbsonPathTestRootNestAltSyntaxMix) {
    auto res = path_select(R"({"elem": {"nest" : 321}})"_json_doc, R"($["elem"].nest)");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(321, res.front().value<int32_t>());
}

TEST(PathTest, JbsonPathTestArrayIndex) {
    auto res = path_select(R"({"arr": [76, 923, 12]})"_json_doc, "$.arr[1]");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(923, res.front().value<int32_t>());
}

TEST(PathTest, JbsonPathTestArrayIndexUnion) {
    auto res = path_select(R"({"arr": [76, 923, 12]})"_json_doc, "$.arr[1,2]");
    ASSERT_EQ(2, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(923, res.front().value<int32_t>());
    EXPECT_EQ(element_type::int32_element, res.back().type());
    EXPECT_EQ(12, res.back().value<int32_t>());
}

TEST(PathTest, JbsonPathTestArrayIndexUnion2) {
    auto res = path_select(R"({"arr": [76, [923], {"obj":12}]})"_json_doc, "$.arr[1,2]");
    ASSERT_EQ(2, res.size());
    EXPECT_EQ(element_type::array_element, res.front().type());
    EXPECT_EQ(element_type::document_element, res.back().type());
}

TEST(PathTest, JbsonPathTestArrayWildcardAccess) {
    auto res = path_select(R"({"arr": [76, [923], {"obj":12}]})"_json_doc, "$.arr[*].obj");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(12, res.front().value<int32_t>());
}

TEST(PathTest, JbsonPathTestArrayWildcardAccess2) {
    auto res = path_select(R"({"arr": [76, [923], {"obj":12}, 7654, {"obj":96}]})"_json_doc, "$.arr[*].obj");
    ASSERT_EQ(2, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(12, res.front().value<int32_t>());
    EXPECT_EQ(element_type::int32_element, res.back().type());
    EXPECT_EQ(96, res.back().value<int32_t>());
}

TEST(PathTest, DISABLED_JbsonPathTestRecurse1) {
    auto res = path_select(R"({"arr": [76, [923], {"obj":12}, 765]})"_json_doc, "$.arr..obj");
    ASSERT_EQ(1, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(12, res.front().value<int32_t>());
}

TEST(PathTest, DISABLED_JbsonPathTestRecurse2) {
    auto res = path_select(R"({"arr": [76, [923], {"obj":12}, [765, {"obj": 24}]]})"_json_doc, "$.arr..obj");
    ASSERT_EQ(2, res.size());
    EXPECT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(12, res.front().value<int32_t>());
    EXPECT_EQ(element_type::int32_element, res.back().type());
    EXPECT_EQ(24, res.back().value<int32_t>());
}

TEST(PathTest, JbsonPathExprTest_equality) {
    auto res = path_select(R"({"arr": [76, [923], {"obj": "some string"}, {"obj": "some other string"}]})"_json_doc,
                           R"($.arr[?(@.obj == "some string")])");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::document_element, res.front().type());
    auto doc = get<element_type::document_element>(res.front());
    EXPECT_EQ(1, boost::distance(doc));
    EXPECT_EQ(element_type::string_element, doc.begin()->type());
    EXPECT_EQ("some string", get<element_type::string_element>(*doc.begin()));
}

TEST(PathTest, JbsonPathExprTest_inEquality) {
    auto res = path_select(R"({"arr": [76, [923], {"obj": "some string"}, {"obj": "some other string"}]})"_json_doc,
                           R"($.arr[?(@.obj != "some string")])");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::document_element, res.front().type());
    auto doc = get<element_type::document_element>(res.front());
    EXPECT_EQ(1, boost::distance(doc));
    EXPECT_EQ(element_type::string_element, doc.begin()->type());
    EXPECT_EQ("some other string", get<element_type::string_element>(*doc.begin()));
}

TEST(PathTest, JbsonPathExprTest_equalityAccess) {
    try {
    auto res = path_select(R"({"arr": [76, [923], {"obj": "some string"}]})"_json_doc, R"($.arr[?(@.obj == "some string")].obj)");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::string_element, res.front().type());
    EXPECT_EQ("some string", get<element_type::string_element>(res.front()));
    }
    catch(...) {
        FAIL() << boost::current_exception_diagnostic_information();
    }
}

TEST(PathTest, JbsonPathExprTest_objectAccessInequality) {
    auto res = path_select(R"({"arr": [76, [923], {"obj": "some string"}]})"_json_doc, R"($.arr[?(@.obj != "ghgf")].obj)");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::string_element, res.front().type());
    EXPECT_EQ("some string", get<element_type::string_element>(res.front()));
}

TEST(PathTest, JbsonPathExprTest_more) {
    auto res = path_select(R"({"arr": [76, [923], {"obj": "some string"}]})"_json_doc, R"($.arr[?(@ > 70)])");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(76, get<element_type::int32_element>(res.front()));
}

TEST(PathTest, JbsonPathExprTest_equals) {
    auto res = path_select(R"({"arr": [76, [923], {"obj": "some string"}]})"_json_doc, R"($.arr[?(@ == 76)])");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(76, get<element_type::int32_element>(res.front()));
}

TEST(PathTest, JbsonPathExprTest_equalsArith) {
    auto res = path_select(R"({"arr": [76, [923], {"obj": "some string"}]})"_json_doc, R"($.arr[?(@ == 25*3+1)])");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(76, get<element_type::int32_element>(res.front()));
}

TEST(PathTest, JbsonPathExprTest_equalsLogic) {
    auto res = path_select(R"({"arr": [76, [923], {"obj": "some string"}]})"_json_doc, R"($.arr[?(@ > 70 && @ < 80)])");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(76, get<element_type::int32_element>(res.front()));
}

TEST(PathTest, JbsonPathExprTest_false) {
    auto res = path_select(R"({"arr": [76, [923], {"obj": "some string"}]})"_json_doc, R"($.arr[?(false)])");
    ASSERT_EQ(0, res.size());
}

TEST(PathTest, JbsonPathExprTest_true) {
    auto doc = R"({"arr": [76, [923], {"obj": "some string"}]})"_json_doc;
    auto res = path_select(R"({"arr": [76, [923], {"obj": "some string"}]})"_json_doc, R"($.arr[?(true)])");
    auto arr = get<element_type::array_element>(*std::begin(doc));
    ASSERT_EQ(boost::distance(arr), res.size());
    EXPECT_TRUE(boost::equal(arr, res));
}

TEST(PathTest, JbsonPathExprTest_indexExpr) {
    auto doc = R"({"arr": [76, [923], {"obj": "some string"}]})"_json_doc;
    auto res = path_select(doc, R"($.arr[(6/3-1)])");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::array_element, res.front().type());
    auto arr = get<element_type::array_element>(res.front());
    ASSERT_EQ(boost::distance(arr), 1);
    EXPECT_EQ(923, get<element_type::int32_element>(*std::begin(arr)));
}

TEST(PathTest, JbsonPathExprTest_strIndexExpr) {
    auto doc = R"({"arr": [76, [923], {"obj": "some string"}]})"_json_doc;
    auto res = path_select(doc, R"($.arr[("1")])");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::array_element, res.front().type());
    auto arr = get<element_type::array_element>(res.front());
    ASSERT_EQ(boost::distance(arr), 1);
    EXPECT_EQ(923, get<element_type::int32_element>(*std::begin(arr)));
}

//TEST(PathTest, JbsonPathExprTest_array) {
//    try {
//    auto res = path_select(R"({"arr": [76, [923], {"obj": "some string"}]})"_json_doc,
//                           R"($.arr[?(@[?(@ == 923)].length > 0)])");
//    ASSERT_EQ(1, res.size());
//    ASSERT_EQ(element_type::array_element, res.front().type());
//    auto arr = get<element_type::array_element>(res.front());
//    ASSERT_EQ(1, boost::distance(arr));
//    EXPECT_EQ(923, get<element_type::int32_element>(*std::begin(arr)));
//    }
//    catch(...) {
//        FAIL() << boost::current_exception_diagnostic_information();
//    }
//}

TEST(PathTest, JbsonPathExprTest2) {
    auto res = path_select(R"({"arr": [{"key":"firstname", "value":"Chris"},
                                       {"key":"surname", "value":"Manning"}]})"_json_doc,
                           R"($.arr[?(@.key == "firstname")].value)");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::string_element, res.front().type());
    EXPECT_EQ("Chris", get<element_type::string_element>(res.front()));
}

TEST(PathTest, JbsonPathExprTest3) {
    auto res = path_select(R"({"arr": [{"key":"name", "value":"Chris"},
                                       {"key":"ame", "value":"Chris"},
                                       {"key":"name", "value":"Tom"},
                                       {"key":"ame", "value":"Tom"}]})"_json_doc,
                           R"($.arr[?(@.key == "name")].value)");
    ASSERT_EQ(2, res.size());
    ASSERT_EQ(element_type::string_element, res.front().type());
    EXPECT_EQ("Chris", get<element_type::string_element>(res.front()));
    ASSERT_EQ(element_type::string_element, res.back().type());
    EXPECT_EQ("Tom", get<element_type::string_element>(res.back()));
}

TEST(PathTest, JbsonPathExprTest4) {
    auto res = path_select(R"({"arr": [{"key":"year", "value":2014},
                                       {"key":"year", "value":2012},
                                       {"key":"year", "value":2013}]})"_json_doc,
                           R"($.arr[?(@.value != 2014)].value)");
    ASSERT_EQ(2, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(2012, get<element_type::int32_element>(res.front()));
    ASSERT_EQ(element_type::int32_element, res.back().type());
    EXPECT_EQ(2013, get<element_type::int32_element>(res.back()));
}

TEST(PathTest, JbsonPathExprTest5) {
    auto res = path_select(R"({"arr": [{"key":"year", "value":2014},
                                       {"key":"year", "value":2012},
                                       {"key":"year", "value":2013}]})"_json_doc,
                           R"($.arr[?((@.value < 2014) && (@.value > 2012))].value)");
    ASSERT_EQ(1, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(2013, get<element_type::int32_element>(res.front()));

    res = path_select(R"({"arr": [{"key":"year", "value":2014},
                                  {"key":"year", "value":2012},
                                  {"key":"year", "value":2013}]})"_json_doc,
                           R"($.arr[?(@.value <= 2013)].value)");
    ASSERT_EQ(2, res.size());
    ASSERT_EQ(element_type::int32_element, res.front().type());
    EXPECT_EQ(2012, get<element_type::int32_element>(res.front()));
    ASSERT_EQ(element_type::int32_element, res.back().type());
    EXPECT_EQ(2013, get<element_type::int32_element>(res.back()));
}
