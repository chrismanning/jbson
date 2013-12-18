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
