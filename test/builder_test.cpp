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
using namespace jbson;

#include <gtest/gtest.h>

TEST(BuilderTest, BuildTest1) {
    auto builder = doc_builder("hello", element_type::string_element, "world");

    document doc(builder);

    auto it = doc.begin();
    ASSERT_NE(doc.end(), it);
    auto e = *it;
    EXPECT_EQ(element_type::string_element, e.type());
    EXPECT_EQ("hello", e.name());
    EXPECT_EQ("world", get<jbson::element_type::string_element>(e));
}

TEST(BuilderTest, BuildTest2) {
    auto builder = doc_builder("hello", element_type::string_element);
    EXPECT_THROW((void)document(builder), invalid_element_size);
    builder = doc_builder("hello", element_type::null_element);
    EXPECT_NO_THROW((void)document(builder));
    EXPECT_THROW(builder = doc_builder("hello", element_type::undefined_element, 0), incompatible_type_conversion);
}

TEST(BuilderTest, BuildTest3) {
    auto doc = static_cast<document>(
                       doc_builder
                       ("first name", element_type::string_element, "Chris")
                       ("surname", element_type::string_element, "Manning")
                       ("yob", element_type::int32_element, 1991)
                    );
    static_assert(std::is_same<decltype(doc), document>::value, "");
    auto it = doc.begin();
    auto end = doc.end();
    ASSERT_NE(end, it);
    EXPECT_EQ("first name", it->name());
    EXPECT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("Chris", get<element_type::string_element>(*it));
    it++;
    EXPECT_EQ("surname", it->name());
    EXPECT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("Manning", get<element_type::string_element>(*it));
    it++;
    EXPECT_EQ("yob", it->name());
    EXPECT_EQ(element_type::int32_element, it->type());
    EXPECT_EQ(1991, it->value<int32_t>());
    it++;
    ASSERT_EQ(it, end);
}

TEST(BuilderTest, BuildNestTest1) {
    auto doc = document(doc_builder
                        ("hello", element_type::string_element, "world")
                        ("embedded document", element_type::array_element, doc_builder
                         ("0", element_type::string_element, "awesome")
                         ("1", element_type::double_element, 5.05)
                         ("2", element_type::int32_element, 1986)
                        ));

    auto it = doc.begin();
    auto end = doc.end();
    ASSERT_NE(it, end);

    auto e = *it++;
    EXPECT_EQ(element_type::string_element, e.type());
    EXPECT_EQ("hello", e.name());
    EXPECT_EQ("world", get<jbson::element_type::string_element>(e));

    auto arr_el = *it++;
    ASSERT_EQ(it, end);

    EXPECT_EQ(element_type::array_element, arr_el.type());
    auto arr = get<element_type::array_element>(arr_el);
    static_assert(detail::is_document<decltype(arr)>::value, "");
    static_assert(
        std::is_same<basic_array<boost::iterator_range<std::vector<char>::const_iterator>>, decltype(arr)>::value, "");

    it = arr.begin();
    end = arr.end();
    EXPECT_EQ("0", it->name());
    EXPECT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("awesome", get<element_type::string_element>(*it));
    it++;
    EXPECT_EQ("1", it->name());
    EXPECT_EQ(element_type::double_element, it->type());
    EXPECT_EQ(5.05, it->value<double>());
    it++;
    EXPECT_EQ("2", it->name());
    EXPECT_EQ(element_type::int32_element, it->type());
    EXPECT_EQ(1986, it->value<int32_t>());
    it++;
    ASSERT_EQ(it, end);
}
