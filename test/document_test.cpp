//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <string>
using namespace std::literals;
#include <list>
#include <fstream>

#include <jbson/element.hpp>
#include <jbson/document.hpp>
using namespace jbson;

#include <gtest/gtest.h>

TEST(DocumentTest, DocumentParseTest1) {
    const auto test_bson = "\x16\x00\x00\x00\x02hello\x00\x06\x00\x00\x00world\x00\x00"s;
    auto doc = document{test_bson};

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

TEST(DocumentTest, DocumentParseTest2) {
    auto test_bson = "x02hello\x00\x06\x00\x00\x00world\x00\x00"s;
    ASSERT_THROW(document{test_bson}, invalid_document_size);
}

TEST(DocumentTest, DocumentParseTest3) {
    auto ifs = std::ifstream{JBSON_FILES"test.bson", std::ios::binary};
    ASSERT_FALSE(ifs.fail());
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    ASSERT_GT(n, 0);
    auto binstring = std::vector<char>(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(binstring.data(), n);

    auto doc = basic_document<boost::iterator_range<std::vector<char>::const_iterator>>{binstring};

    auto it = doc.begin();
    auto end = doc.end();
    ASSERT_NE(it, end);

    auto arr_el = *it++;
    ASSERT_EQ(it, end);

    EXPECT_EQ(element_type::array_element, arr_el.type());
    auto arr = get<element_type::array_element>(arr_el);
    static_assert(detail::is_document<decltype(arr)>::value, "");
    static_assert(std::is_same<basic_document<boost::iterator_range<std::vector<char>::const_iterator>>, decltype(arr)>::value, "");

    auto i = int32_t{0};
    for(auto&& e : arr) {
        EXPECT_LT(e.size(), arr.size());
        EXPECT_EQ(std::to_string(i), e.name());
        ++i;
    }

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
