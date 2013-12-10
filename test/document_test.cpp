//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <string>
using namespace std::literals;
#include <list>

#include <jbson/element.hpp>
#include <jbson/document.hpp>
using namespace jbson;

#include <gtest/gtest.h>

TEST(DocumentTest, DocumentParseTest1) {
    const auto test_bson = "\x16\x00\x00\x00\x02hello\x00\x06\x00\x00\x00world\x00\x00"s;
    auto doc = document{document::container_type{test_bson.data(), test_bson.data()+test_bson.size()}};

    auto begin = doc.begin();
    static_assert(std::is_same<decltype(*begin), document::element_type>::value, "");
    auto e = *begin;
    ASSERT_EQ(element_type::string_element, e.type());
    EXPECT_EQ("hello", e.name());
    EXPECT_EQ("world", get<element_type::string_element>(e));
    for(auto e : doc) {
        ASSERT_EQ(element_type::string_element, e.type());
        EXPECT_EQ("hello", e.name());
        EXPECT_EQ("world", get<element_type::string_element>(e));
    }
}
