//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <string>
using namespace std::literals;

#include <boost/exception/diagnostic_information.hpp>

#include <jbson/json_reader.hpp>
using namespace jbson;

#include <gtest/gtest.h>

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
    auto json = boost::string_ref{R"({       "key"       :   "value"  })"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    ASSERT_EQ(1, reader.m_elements.size());
    EXPECT_EQ("key", reader.m_elements.begin()->name());
    ASSERT_EQ(element_type::string_element, reader.m_elements.begin()->type());
    EXPECT_EQ("value", get<element_type::string_element>(*reader.m_elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest5) {
    auto json = boost::string_ref{"{\n\"key\"\t:\"value\"}"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    ASSERT_EQ(1, reader.m_elements.size());
    EXPECT_EQ("key", reader.m_elements.begin()->name());
    ASSERT_EQ(element_type::string_element, reader.m_elements.begin()->type());
    EXPECT_EQ("value", get<element_type::string_element>(*reader.m_elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest6) {
    auto json = boost::string_ref{R"({"key":true})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    ASSERT_EQ(1, reader.m_elements.size());
    EXPECT_EQ("key", reader.m_elements.begin()->name());
    ASSERT_EQ(element_type::boolean_element, reader.m_elements.begin()->type());
    EXPECT_TRUE(get<element_type::boolean_element>(*reader.m_elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest7) {
    auto json = boost::string_ref{R"({"key":123})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    ASSERT_EQ(1, reader.m_elements.size());
    EXPECT_EQ("key", reader.m_elements.begin()->name());
    ASSERT_EQ(element_type::int32_element, reader.m_elements.begin()->type());
    EXPECT_EQ(123, get<element_type::int32_element>(*reader.m_elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest8) {
    auto json = boost::string_ref{R"({"key":null})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    ASSERT_EQ(1, reader.m_elements.size());
    EXPECT_EQ("key", reader.m_elements.begin()->name());
    ASSERT_EQ(element_type::null_element, reader.m_elements.begin()->type());
}

TEST(JsonReaderTest, JsonParseTest9) {
    auto json = boost::string_ref{R"({"key":false})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    ASSERT_EQ(1, reader.m_elements.size());
    EXPECT_EQ("key", reader.m_elements.begin()->name());
    ASSERT_EQ(element_type::boolean_element, reader.m_elements.begin()->type());
    EXPECT_FALSE(get<element_type::boolean_element>(*reader.m_elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest10) {
    auto json = boost::string_ref{R"({"key":3.141})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    ASSERT_EQ(1, reader.m_elements.size());
    EXPECT_EQ("key", reader.m_elements.begin()->name());
    ASSERT_EQ(element_type::double_element, reader.m_elements.begin()->type());
    EXPECT_EQ(3.141, get<element_type::double_element>(*reader.m_elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest11) {
    auto json = boost::string_ref{R"({"key":-123})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    ASSERT_EQ(1, reader.m_elements.size());
    EXPECT_EQ("key", reader.m_elements.begin()->name());
    ASSERT_EQ(element_type::int32_element, reader.m_elements.begin()->type());
    EXPECT_EQ(-123, get<element_type::int32_element>(*reader.m_elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest12) {
    auto json = boost::string_ref{R"({"key": 4294967296})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    ASSERT_EQ(1, reader.m_elements.size());
    EXPECT_EQ("key", reader.m_elements.begin()->name());
    ASSERT_EQ(element_type::int64_element, reader.m_elements.begin()->type());
    EXPECT_EQ(4294967296, get<element_type::int64_element>(*reader.m_elements.begin()));
}

TEST(JsonReaderTest, JsonParseTest13) {
    auto json = boost::string_ref{R"({"key": {"nested key" : "nested value"}})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    ASSERT_EQ(1, reader.m_elements.size());
    EXPECT_EQ("key", reader.m_elements.begin()->name());
    ASSERT_EQ(element_type::document_element, reader.m_elements.begin()->type());
    auto set = static_cast<document_set>(get<element_type::document_element>(*reader.m_elements.begin()));
    EXPECT_EQ("nested key", set.begin()->name());
    EXPECT_EQ("nested value", get<element_type::string_element>(*set.begin()));
}

TEST(JsonReaderTest, JsonParseTest14) {
    auto json = boost::string_ref{R"({"ke\ny":"value"})"};
    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));

    ASSERT_EQ(1, reader.m_elements.size());
    EXPECT_EQ("ke\ny", reader.m_elements.begin()->name());
    ASSERT_EQ(element_type::string_element, reader.m_elements.begin()->type());
    EXPECT_EQ("value", get<element_type::string_element>(*reader.m_elements.begin()));
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

    ASSERT_EQ(3, reader.m_elements.size());

    auto it = reader.m_elements.begin();
    EXPECT_EQ("0", it->name());
    ASSERT_EQ(element_type::int64_element, it->type());
    EXPECT_EQ(4294967296, get<element_type::int64_element>(*it));
    ++it;
    ASSERT_NE(reader.m_elements.end(), it);
    EXPECT_EQ("1", it->name());
    ASSERT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("some string", get<element_type::string_element>(*it));
    ++it;
    ASSERT_NE(reader.m_elements.end(), it);
    EXPECT_EQ("2", it->name());
    ASSERT_EQ(element_type::boolean_element, it->type());
    EXPECT_TRUE(get<element_type::boolean_element>(*it));
    ++it;
    ASSERT_EQ(reader.m_elements.end(), it);
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

    ASSERT_EQ(1, reader.m_elements.size());
    auto e = *reader.m_elements.begin();
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
