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
#include <jbson/json_reader.hpp>
using namespace jbson;

#include <gtest/gtest.h>

TEST(BuilderTest, EmptyBuild) {
    auto builder_ = builder{};

    document doc(builder_);
    // int32_t + 0x00
    EXPECT_EQ(5, doc.size());
}

TEST(BuilderTest, BuildTest1) {
    auto builder_ = builder("hello", element_type::string_element, "world");

    document doc(builder_);

    auto it = doc.begin();
    ASSERT_NE(doc.end(), it);
    auto e = *it;
    EXPECT_EQ(element_type::string_element, e.type());
    EXPECT_EQ("hello", e.name());
    EXPECT_EQ("world", get<jbson::element_type::string_element>(e));
}

JBSON_PUSH_DISABLE_DEPRECATED_WARNING
TEST(BuilderTest, BuildTest2) {
    builder builder_;
    EXPECT_THROW(builder_ = builder("hello", element_type::string_element), incompatible_type_conversion);
    ASSERT_NO_THROW(builder_ = builder("hello", element_type::null_element));
    EXPECT_NO_THROW((void)document(builder_));
    EXPECT_THROW(builder_ = builder("hello", element_type::undefined_element, 0), incompatible_type_conversion);
}
JBSON_POP_WARNINGS

TEST(BuilderTest, BuildTest3) {
    auto doc = static_cast<document>(
                       builder
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

TEST(BuilderTest, ArrayBuildTest1) {
    static_assert(std::is_same<decltype(array_builder(123)(321)), array_builder&&>::value, "");
    auto arrb = array_builder(15)("str")(4.6);
    static_assert(std::is_same<decltype(arrb), array_builder>::value, "");
    array arr;
    EXPECT_NO_THROW(arr = array(arrb));

    auto it = arr.begin();
    const auto end = arr.end();
    ASSERT_NE(end, it);
    EXPECT_EQ("0", it->name());
    EXPECT_EQ(element_type::int32_element, it->type());
    EXPECT_EQ(15, get<element_type::int32_element>(*it));
    it++;
    EXPECT_EQ("1", it->name());
    EXPECT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("str", get<element_type::string_element>(*it));
    it++;
    EXPECT_EQ("2", it->name());
    EXPECT_EQ(element_type::double_element, it->type());
    EXPECT_DOUBLE_EQ(4.6, get<element_type::double_element>(*it));
    it++;
    ASSERT_EQ(it, end);
}

TEST(BuilderTest, BuildNestTest1) {
    document doc;
    doc = document(builder
                        ("hello", element_type::string_element, "world")
                        ("embedded array", element_type::array_element, array_builder
                         ("awesome")(5.05)(1986)
                        ));

    auto it = doc.begin();
    auto end = doc.end();
    ASSERT_NE(it, end);

    auto e = *it;
    EXPECT_EQ(element_type::string_element, e.type());
    EXPECT_EQ("hello", e.name());
    EXPECT_EQ("world", get<jbson::element_type::string_element>(e));

    it++;
    ASSERT_NE(it, end);
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

TEST(BuilderTest, BuildNestTest2) {
    auto root_doc = document(builder
                        ("hello", element_type::string_element, "world")
                        ("embedded doc", element_type::document_element, builder
                         ("a", "awesome")
                         ("b", 5.05)
                         ("c", 1986)
                        ));

    auto it = root_doc.begin();
    auto end = root_doc.end();
    ASSERT_NE(it, end);

    auto e = *it;
    EXPECT_EQ(element_type::string_element, e.type());
    EXPECT_EQ("hello", e.name());
    EXPECT_EQ("world", get<jbson::element_type::string_element>(e));

    it++;
    ASSERT_NE(it, end);
    auto doc_el = *it++;
    ASSERT_EQ(it, end);

    EXPECT_EQ(element_type::document_element, doc_el.type());
    auto doc = get<element_type::document_element>(doc_el);
    static_assert(detail::is_document<decltype(doc)>::value, "");
    static_assert(
        std::is_same<basic_document<boost::iterator_range<std::vector<char>::const_iterator>>, decltype(doc)>::value, "");

    it = doc.begin();
    end = doc.end();
    EXPECT_EQ("a", it->name());
    EXPECT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("awesome", get<element_type::string_element>(*it));
    it++;
    EXPECT_EQ("b", it->name());
    EXPECT_EQ(element_type::double_element, it->type());
    EXPECT_EQ(5.05, it->value<double>());
    it++;
    EXPECT_EQ("c", it->name());
    EXPECT_EQ(element_type::int32_element, it->type());
    EXPECT_EQ(1986, it->value<int32_t>());
    it++;
    ASSERT_EQ(it, end);
}

TEST(BuilderTest, BuildNestTest3) {
    auto root_doc = document(builder
                        ("hello", element_type::string_element, "world")
                        ("embedded doc", element_type::array_element, array_builder
                         ("awesome")
                         (5.05)
                         (element_type::document_element, builder
                          ("y", "sauce")
                          ("z", 57)
                         )
                        ));

    auto it = root_doc.begin();
    auto end = root_doc.end();
    ASSERT_NE(it, end);

    auto e = *it;
    EXPECT_EQ(element_type::string_element, e.type());
    EXPECT_EQ("hello", e.name());
    EXPECT_EQ("world", get<jbson::element_type::string_element>(e));

    it++;
    ASSERT_NE(it, end);
    auto arr_el = *it++;
    ASSERT_EQ(it, end);

    EXPECT_EQ(element_type::array_element, arr_el.type());
    auto doc = get<element_type::array_element>(arr_el);
    static_assert(detail::is_document<decltype(doc)>::value, "");
    static_assert(
        std::is_same<basic_array<boost::iterator_range<std::vector<char>::const_iterator>>, decltype(doc)>::value, "");

    it = doc.begin();
    end = doc.end();
    EXPECT_EQ("0", it->name());
    EXPECT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("awesome", get<element_type::string_element>(*it));
    it++;
    EXPECT_EQ("1", it->name());
    EXPECT_EQ(element_type::double_element, it->type());
    EXPECT_EQ(5.05, it->value<double>());
    it++;
    EXPECT_EQ("2", it->name());
    EXPECT_EQ(element_type::document_element, it->type());
    ASSERT_NO_THROW(doc = get<element_type::document_element>(*it));
    it++;
    ASSERT_EQ(it, end);

    it = doc.begin();
    end = doc.end();

    EXPECT_EQ("y", it->name());
    EXPECT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("sauce", get<element_type::string_element>(*it));
    it++;
    EXPECT_EQ("z", it->name());
    EXPECT_EQ(element_type::int32_element, it->type());
    EXPECT_EQ(57, get<element_type::int32_element>(*it));
    it++;
    ASSERT_EQ(it, end);
}

TEST(BuilderTest, BuildNestTest4) {
    auto root_doc = document(builder
                        ("hello", element_type::string_element, "world")
                        ("embedded doc", element_type::document_element, builder
                         ("a", "awesome")
                         ("b", 5.05)
                         ("c", element_type::document_element, builder
                          ("y", "sauce")
                          ("z", 57)
                         )
                        ));

    auto it = root_doc.begin();
    auto end = root_doc.end();
    ASSERT_NE(it, end);

    auto e = *it;
    EXPECT_EQ(element_type::string_element, e.type());
    EXPECT_EQ("hello", e.name());
    EXPECT_EQ("world", get<jbson::element_type::string_element>(e));

    it++;
    ASSERT_NE(it, end);
    auto doc_el = *it++;
    ASSERT_EQ(it, end);

    EXPECT_EQ(element_type::document_element, doc_el.type());
    auto doc = get<element_type::document_element>(doc_el);
    static_assert(detail::is_document<decltype(doc)>::value, "");
    static_assert(
        std::is_same<basic_document<boost::iterator_range<std::vector<char>::const_iterator>>, decltype(doc)>::value, "");

    it = doc.begin();
    end = doc.end();
    EXPECT_EQ("a", it->name());
    EXPECT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("awesome", get<element_type::string_element>(*it));
    it++;
    EXPECT_EQ("b", it->name());
    EXPECT_EQ(element_type::double_element, it->type());
    EXPECT_EQ(5.05, it->value<double>());
    it++;
    EXPECT_EQ("c", it->name());
    EXPECT_EQ(element_type::document_element, it->type());
    ASSERT_NO_THROW(doc = get<element_type::document_element>(*it));
    it++;
    ASSERT_EQ(it, end);

    it = doc.begin();
    end = doc.end();

    EXPECT_EQ("y", it->name());
    EXPECT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("sauce", get<element_type::string_element>(*it));
    it++;
    EXPECT_EQ("z", it->name());
    EXPECT_EQ(element_type::int32_element, it->type());
    EXPECT_EQ(57, get<element_type::int32_element>(*it));
    it++;
    ASSERT_EQ(it, end);
}

TEST(BuilderTest, BuildNestTest5) {
    document root_doc;
    EXPECT_NO_THROW(root_doc = builder
                               ("hello", element_type::string_element, "world")
                               ("embedded doc", element_type::document_element, builder
                                ("a", "awesome")
                                ("b", 5.05)
                                ("c", element_type::document_element, R"({"y":"sauce","z":57})"_json_set)
                               ));

    EXPECT_NO_THROW((void)document(R"({"y":"sauce","z":57})"_json_set));
    EXPECT_NO_THROW(root_doc = builder
                               ("hello", element_type::string_element, "world")
                               ("embedded doc", element_type::document_element, builder
                                ("a", "awesome")
                                ("b", 5.05)
                                ("c", element_type::document_element, R"({"y":"sauce","z":57})"_json_doc)
                               ));
    std::vector<element> elems;
    elems.emplace_back("y", "sauce");
    elems.emplace_back("z", 57);
    EXPECT_NO_THROW(root_doc = builder
                               ("hello", element_type::string_element, "world")
                               ("embedded doc", element_type::document_element, builder
                                ("a", "awesome")
                                ("b", 5.05)
                                ("c", element_type::document_element, document(elems))
                               ));
    auto it = root_doc.find("embedded doc");
    ASSERT_NE(root_doc.end(), it);
    ASSERT_EQ(element_type::document_element, it->type());
    auto doc = get<element_type::document_element>(*it);
    it = doc.find("c");
    ASSERT_NE(doc.end(), it);
    ASSERT_EQ(element_type::document_element, it->type());
    ASSERT_NO_THROW(doc = get<element_type::document_element>(*it));

    it = doc.begin();
    ASSERT_NE(doc.end(), it);
    EXPECT_EQ("y", it->name());
    EXPECT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("sauce", get<element_type::string_element>(*it));
    it++;
    EXPECT_EQ("z", it->name());
    EXPECT_EQ(element_type::int32_element, it->type());
    EXPECT_EQ(57, get<element_type::int32_element>(*it));
    it++;
    ASSERT_EQ(it, doc.end());
}
