//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <string>
using namespace std::literals;
#include <list>

#include <jbson/element.hpp>
#include <jbson/document.hpp>
using jbson::element_type;

#include <gtest/gtest.h>

TEST(ElementTest, ElementParseTest1) {
    auto el1 = jbson::element{"\x02hello\x00\x06\x00\x00\x00world\x00"s};
    ASSERT_EQ(element_type::string_element, el1.type());
    EXPECT_EQ("hello", el1.name());
    EXPECT_EQ("world", jbson::get<element_type::string_element>(el1));
    el1.value("test");
    EXPECT_EQ("test", jbson::get<element_type::string_element>(el1));
}

TEST(ElementTest, ElementParseTest2) {
    auto bson = "\x02hello\x00\x06\x00\x00\x00world\x00"s;
    auto el1 = jbson::element{bson};
    EXPECT_EQ(bson.size(), el1.size());
    ASSERT_EQ(element_type::string_element, el1.type());
    EXPECT_EQ("hello", el1.name());
    EXPECT_EQ("world", jbson::get<element_type::string_element>(el1));
    el1.name("some name");
    EXPECT_EQ("some name", el1.name());
    el1.value("some value");
    EXPECT_EQ("some value", jbson::get<element_type::string_element>(el1));
    el1.value(element_type::int32_element, 1234);
    ASSERT_EQ(element_type::int32_element, el1.type());
    EXPECT_EQ(1234, jbson::get<element_type::int32_element>(el1));
    EXPECT_EQ(15, el1.size());
}

TEST(ElementTest, ElementParseTest3) {
    auto el1 = jbson::element{"Pi 6dp", element_type::double_element, 3.141592};
    ASSERT_EQ(element_type::double_element, el1.type());
    EXPECT_EQ("Pi 6dp", el1.name());
    EXPECT_EQ(3.141592, jbson::get<element_type::double_element>(el1));
    auto val = 44.854;
    el1.value(val);
    ASSERT_EQ(element_type::double_element, el1.type());
    EXPECT_EQ(val, jbson::get<element_type::double_element>(el1));
    //    el1.value("test"s);
}
