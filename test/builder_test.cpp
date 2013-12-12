//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <string>
using namespace std::literals;
#include <list>
#include <fstream>

#include <jbson/document.hpp>
#include <jbson/builder.hpp>
//using namespace jbson;

#include <gtest/gtest.h>

TEST(BuilderTest, BuildTest1) {
    jbson::doc_builder doc_builder;
    doc_builder("hello", jbson::element_type::string_element, "world");

    auto raw_data = std::vector<char>(4, '\0');
    for(auto&& e : doc_builder.m_elements) {
        boost::range::push_back(raw_data, static_cast<std::vector<char>>(e));
    }
    raw_data.push_back('\0');
    auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(raw_data.size()));
    ASSERT_EQ(4, size.size());

    ::memmove(raw_data.data(), size.data(), size.size());

    auto doc = jbson::document{raw_data};

    bool v{false};
    for(auto&& e : doc) {
        v = true;
        EXPECT_EQ(jbson::element_type::string_element, e.type());
        EXPECT_EQ("hello", e.name());
        EXPECT_EQ("world", jbson::get<jbson::element_type::string_element>(e));
    }
    ASSERT_TRUE(v);
}

TEST(BuilderTest, BuildTest2) {
    jbson::doc_builder doc_builder;
    doc_builder("hello", jbson::element_type::string_element);

    auto v = std::vector<char>{};
    EXPECT_THROW(v = std::move(static_cast<decltype(v)>(doc_builder.m_elements.front())), jbson::invalid_element_size);
}
