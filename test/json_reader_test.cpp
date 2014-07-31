//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <string>
using namespace std::literals;

#include <boost/exception/diagnostic_information.hpp>

#include <gtest/gtest.h>

#define private public
#include <jbson/json_reader.hpp>
using namespace jbson;

TEST(JsonReaderTest, JsonParseTest1) {
    auto json = boost::string_ref{R"({})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonParseTest2) {
    auto json = boost::string_ref{R"({{})"};
    auto reader = json_reader{};
    ASSERT_THROW(reader.parse(json), json_parse_error);
}

TEST(JsonReaderTest, JsonParseTest3) {
    auto json = boost::string_ref{R"([])"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonParseTest4) {
    auto json = R"({       "key"       :   "value"  })";
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    EXPECT_EQ("key", elements.begin()->name());
    ASSERT_EQ(element_type::string_element, elements.begin()->type());
    EXPECT_EQ("value", get<element_type::string_element>(*elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest5) {
    auto json = boost::string_ref{"{\n\"key\"\t:\"value\"}"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    EXPECT_EQ("key", elements.begin()->name());
    ASSERT_EQ(element_type::string_element, elements.begin()->type());
    EXPECT_EQ("value", get<element_type::string_element>(*elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest6) {
    auto json = std::array<char, 13>{};
    boost::copy(boost::as_literal(R"({"key":true})"), json.data());
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json.data()));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    EXPECT_EQ("key", elements.begin()->name());
    ASSERT_EQ(element_type::boolean_element, elements.begin()->type());
    EXPECT_TRUE(get<element_type::boolean_element>(*elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest7) {
    auto json = boost::string_ref{R"({"key":123})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    EXPECT_EQ("key", elements.begin()->name());
    ASSERT_EQ(element_type::int32_element, elements.begin()->type());
    EXPECT_EQ(123, get<element_type::int32_element>(*elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest8) {
    auto json = boost::string_ref{R"({"key":null})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    EXPECT_EQ("key", elements.begin()->name());
    ASSERT_EQ(element_type::null_element, elements.begin()->type());
}

TEST(JsonReaderTest, JsonParseTest9) {
    auto json = boost::string_ref{R"({"key":false})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    EXPECT_EQ("key", elements.begin()->name());
    ASSERT_EQ(element_type::boolean_element, elements.begin()->type());
    EXPECT_FALSE(get<element_type::boolean_element>(*elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest10) {
    auto json = boost::string_ref{R"({"key":3.141})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    EXPECT_EQ("key", elements.begin()->name());
    ASSERT_EQ(element_type::double_element, elements.begin()->type());
    EXPECT_EQ(3.141, get<element_type::double_element>(*elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest11) {
    auto json = boost::string_ref{R"({"key":-123})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    EXPECT_EQ("key", elements.begin()->name());
    ASSERT_EQ(element_type::int32_element, elements.begin()->type());
    EXPECT_EQ(-123, get<element_type::int32_element>(*elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest12) {
    auto json = boost::string_ref{R"({"key": 4294967296})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    EXPECT_EQ("key", elements.begin()->name());
    ASSERT_EQ(element_type::int64_element, elements.begin()->type());
    EXPECT_EQ(4294967296, get<element_type::int64_element>(*elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest13) {
    auto json = boost::string_ref{R"({"key": {"nested key" : "nested value"}})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    EXPECT_EQ("key", elements.begin()->name());
    ASSERT_EQ(element_type::document_element, elements.begin()->type());
    auto set = static_cast<document_set>(get<element_type::document_element>(*elements.begin()));
    EXPECT_EQ("nested key", set.begin()->name());
    EXPECT_EQ("nested value", get<element_type::string_element>(*set.begin()));
}

TEST(JsonReaderTest, JsonParseTest14) {
    auto json = boost::string_ref{R"({"ke\ny":"value"})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    EXPECT_EQ("ke\ny", elements.begin()->name());
    ASSERT_EQ(element_type::string_element, elements.begin()->type());
    EXPECT_EQ("value", get<element_type::string_element>(*elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest15) {
    auto json = boost::string_ref{R"(["key": 4294967296])"};
    auto reader = json_reader{};
    ASSERT_THROW(reader.parse(json), json_parse_error);
}

TEST(JsonReaderTest, JsonParseTest16) {
    auto json = boost::string_ref{R"([4294967296, "some string", true])"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(3, elements.size());

    auto it = elements.begin();
    EXPECT_EQ("0", it->name());
    ASSERT_EQ(element_type::int64_element, it->type());
    EXPECT_EQ(4294967296, get<element_type::int64_element>(*it));
    ++it;
    ASSERT_NE(elements.end(), it);
    EXPECT_EQ("1", it->name());
    ASSERT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("some string", get<element_type::string_element>(*it));
    ++it;
    ASSERT_NE(elements.end(), it);
    EXPECT_EQ("2", it->name());
    ASSERT_EQ(element_type::boolean_element, it->type());
    EXPECT_TRUE(get<element_type::boolean_element>(*it));
    ++it;
    ASSERT_EQ(elements.end(), it);
}

TEST(JsonReaderTest, JsonParseTest16_utf32) {
    auto json = boost::u32string_ref{U"[4294967296, \"some \U0001D11E string\", true]"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(3, elements.size());

    auto it = elements.begin();
    EXPECT_EQ("0", it->name());
    ASSERT_EQ(element_type::int64_element, it->type());
    EXPECT_EQ(4294967296, get<element_type::int64_element>(*it));
    ++it;
    ASSERT_NE(elements.end(), it);
    EXPECT_EQ("1", it->name());
    ASSERT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("some \xF0\x9D\x84\x9E string", get<element_type::string_element>(*it));
    ++it;
    ASSERT_NE(elements.end(), it);
    EXPECT_EQ("2", it->name());
    ASSERT_EQ(element_type::boolean_element, it->type());
    EXPECT_TRUE(get<element_type::boolean_element>(*it));
    ++it;
    ASSERT_EQ(elements.end(), it);
}

TEST(JsonReaderTest, JsonParseTest16_utf16) {
    auto json = boost::u16string_ref{u"[4294967296, \"some \xD834\xDD1E string\", true]"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(3, elements.size());

    auto it = elements.begin();
    EXPECT_EQ("0", it->name());
    ASSERT_EQ(element_type::int64_element, it->type());
    EXPECT_EQ(4294967296, get<element_type::int64_element>(*it));
    ++it;
    ASSERT_NE(elements.end(), it);
    EXPECT_EQ("1", it->name());
    ASSERT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("some \xF0\x9D\x84\x9E string", get<element_type::string_element>(*it));
    ++it;
    ASSERT_NE(elements.end(), it);
    EXPECT_EQ("2", it->name());
    ASSERT_EQ(element_type::boolean_element, it->type());
    EXPECT_TRUE(get<element_type::boolean_element>(*it));
    ++it;
    ASSERT_EQ(elements.end(), it);
}

TEST(JsonReaderTest, JsonParseTest17) {
    auto json = boost::string_ref{R"({"bindata" : {"$binary": false}})"};
    auto reader = json_reader{};
    ASSERT_THROW(reader.parse(json), json_parse_error);
}

TEST(JsonReaderTest, JsonParseTest18) {
    auto json = boost::string_ref{R"({"_id" : {"$oid": ""}})"};
    auto reader = json_reader{};
    ASSERT_THROW(reader.parse(json), json_parse_error);
}

TEST(JsonReaderTest, JsonParseTest19) {
    auto json = boost::string_ref{R"({"_id" : {"$oid": "507f1f77bcf86cd799439011"}})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    auto e = *elements.begin();
    ASSERT_EQ(element_type::oid_element, e.type());

    std::array<char, 12> oid{{static_cast<char>(0x50),
                    static_cast<char>(0x7f),
                    static_cast<char>(0x1f),
                    static_cast<char>(0x77),
                    static_cast<char>(0xbc),
                    static_cast<char>(0xf8),
                    static_cast<char>(0x6c),
                    static_cast<char>(0xd7),
                    static_cast<char>(0x99),
                    static_cast<char>(0x43),
                    static_cast<char>(0x90),
                    static_cast<char>(0x11)
                             }};
    EXPECT_EQ(oid, get<element_type::oid_element>(e));
}

TEST(JsonReaderTest, JsonParseTest20) {
    auto json = boost::string_ref{R"({"dollar" : "\u0024"})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    auto e = *elements.begin();
    ASSERT_EQ(element_type::string_element, e.type());

    EXPECT_EQ("$", get<element_type::string_element>(e));
}

TEST(JsonReaderTest, JsonParseTest21) {
    auto json = boost::string_ref{R"({"bad hex" : "\u002G"})"};
    auto reader = json_reader{};
    ASSERT_THROW(reader.parse(json), json_parse_error);
}

TEST(JsonReaderTest, JsonParseTest22) {
    auto json = boost::string_ref{R"({"str" : "\a"})"};
    auto reader = json_reader{};
    ASSERT_THROW(reader.parse(json), json_parse_error);
}

TEST(JsonReaderTest, JsonParseTest23) {
    auto json = boost::string_ref{R"({"utf" : "κ"})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(1, elements.size());
    auto e = *elements.begin();
    ASSERT_EQ(element_type::string_element, e.type());

    EXPECT_EQ(2, get<element_type::string_element>(e).size());
    EXPECT_EQ("κ", get<element_type::string_element>(e));
}

TEST(JsonReaderTest, JsonParseTest24) {
    auto json = boost::string_ref{R"({"a doc" : {}, "some int": 123})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(2, elements.size());
    auto e = *elements.begin();
    ASSERT_EQ(element_type::document_element, e.type());
    EXPECT_EQ(5, get<element_type::document_element>(e).size());

    e = *++elements.begin();
    ASSERT_EQ(element_type::int32_element, e.type());
    EXPECT_EQ(123, get<element_type::int32_element>(e));
}

TEST(JsonReaderTest, JsonParseTest25) {
    auto json = boost::string_ref{R"({"a doc" : [], "some int": 123})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(2, elements.size());
    auto e = *elements.begin();
    ASSERT_EQ(element_type::array_element, e.type());
    EXPECT_EQ(5, get<element_type::array_element>(e).size());

    e = *++elements.begin();
    ASSERT_EQ(element_type::int32_element, e.type());
    EXPECT_EQ(123, get<element_type::int32_element>(e));
}

TEST(JsonReaderTest, JsonParseTest26) {
    auto json = boost::string_ref{R"({"a doc" : [123, "str"], "some int": 123})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    auto elements = basic_document_set<json_reader::range_type>(reader);
    ASSERT_EQ(2, elements.size());
    auto e = *elements.begin();
    ASSERT_EQ(element_type::array_element, e.type());
    EXPECT_EQ(23, get<element_type::array_element>(e).size());

    e = *++elements.begin();
    ASSERT_EQ(element_type::int32_element, e.type());
    EXPECT_EQ(123, get<element_type::int32_element>(e));
}

TEST(JsonReaderTest, JsonLiteralTest1) {
    auto elements = R"({"utf" : "κ"})"_json_set;
//    static_assert(std::is_same<decltype(elements), document_set>::value,"");

    ASSERT_EQ(1, elements.size());
    auto e = *elements.begin();
    ASSERT_EQ(element_type::string_element, e.type());

    EXPECT_EQ(2, get<element_type::string_element>(e).size());
    EXPECT_EQ("κ", get<element_type::string_element>(e));
}

TEST(JsonReaderTest, JsonParseErrorTest1) {
    auto json = boost::string_ref{R"({{})"};
    auto reader = json_reader{};
    ASSERT_THROW(reader.parse(json), json_parse_error);
}

TEST(JsonReaderTest, JsonParseUnicode) {
    auto json = boost::string_ref{R"([""])"};
    auto reader = json_reader{};
    auto str = std::string{};
    str.resize(2);
    using line_it = line_pos_iterator<boost::string_ref::const_iterator>;
    ASSERT_NO_THROW(reader.parse(json));
    auto doc = document(reader);
    str = doc.begin()->value<std::string>();
    EXPECT_EQ("", str);
}

TEST(JsonReaderTest, JsonParseSurrogateUnicode1) {
    auto json = boost::string_ref{R"(["\uD834\uDD1E"])"};
    auto reader = json_reader{};
    auto str = std::string{};
    str.resize(2);
    ASSERT_NO_THROW(reader.parse(json));
    auto doc = document(reader);
    str = doc.begin()->value<std::string>();
    EXPECT_EQ("\xF0\x9D\x84\x9E", str);
}

TEST(JsonReaderTest, JsonParseSurrogateUnicode2) {
    auto json = boost::string_ref{R"(["\uD834\uDB00"])"};
    auto reader = json_reader{};
    ASSERT_THROW(reader.parse(json), json_parse_error);
}

TEST(JsonReaderTest, JsonParseSurrogateUnicode3) {
    auto json = boost::string_ref{R"(["\uDEAD"])"};
    auto reader = json_reader{};
    ASSERT_THROW(reader.parse(json), json_parse_error);
}

TEST(JsonReaderTest, JsonCheckerFail1) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail1.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail2) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail2.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail3) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail3.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail4) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail4.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail5) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail5.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail6) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail6.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail7) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail7.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail8) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail8.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail9) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail9.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail10) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail10.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail11) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail11.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail12) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail12.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail13) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail13.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail14) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail14.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail15) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail15.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail16) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail16.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail17) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail17.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

// this test assumes some maximum nesting depth
//TEST(JsonReaderTest, JsonCheckerFail18) {
//    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail18.json", std::ios::in};
//    auto json = std::vector<char>{};
//    ifs.seekg(0, std::ios::end);
//    auto n = static_cast<std::streamoff>(ifs.tellg());
//    json.resize(n);
//    ifs.seekg(0, std::ios::beg);
//    ifs.read(json.data(), n);

//    auto reader = json_reader{};
//    EXPECT_ANY_THROW(reader.parse(json));
//}

TEST(JsonReaderTest, JsonCheckerFail19) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail19.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail20) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail20.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail21) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail21.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail22) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail22.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail23) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail23.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail24) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail24.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail25) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail25.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail26) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail26.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail27) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail27.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail28) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail28.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail29) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail29.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail30) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail30.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail31) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail31.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail32) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail32.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerFail33) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/fail33.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_ANY_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerPass1) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/pass1.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_NO_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerPass2) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/pass2.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_NO_THROW(reader.parse(json));
}

TEST(JsonReaderTest, JsonCheckerPass3) {
    std::ifstream ifs{JBSON_FILES"/json_checker_test_suite/pass3.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    EXPECT_NO_THROW(reader.parse(json));
}
