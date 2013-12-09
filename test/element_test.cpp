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

    el1.value(element_type::boolean_element, false);
    EXPECT_FALSE(jbson::get<element_type::boolean_element>(el1));
    el1.value(true);
    ASSERT_EQ(element_type::boolean_element, el1.type());
    EXPECT_TRUE(jbson::get<element_type::boolean_element>(el1));
    EXPECT_EQ(8, el1.size());
    el1.value(432);
    ASSERT_EQ(element_type::boolean_element, el1.type());
    EXPECT_TRUE(jbson::get<element_type::boolean_element>(el1));
    EXPECT_EQ(8, el1.size());
    el1.value(0);
    ASSERT_EQ(element_type::boolean_element, el1.type());
    EXPECT_FALSE(jbson::get<element_type::boolean_element>(el1));
    EXPECT_EQ(8, el1.size());
}

TEST(ElementTest, ElementParseTest2) {
    auto bson = "\x02hello\x00\x06\x00\x00\x00world\x00"s;
    auto el1 = jbson::basic_element<std::list<char>>{bson};
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

TEST(ElementTest, ElementConstructTest1) {
    auto el1 = jbson::element{"Pi 6dp", element_type::double_element, 3.141592};
    ASSERT_EQ(element_type::double_element, el1.type());
    EXPECT_EQ("Pi 6dp", el1.name());
    EXPECT_EQ(3.141592, jbson::get<element_type::double_element>(el1));
    auto val = 44.854;
    el1.value(val);
    ASSERT_EQ(element_type::double_element, el1.type());
    EXPECT_EQ(val, jbson::get<element_type::double_element>(el1));
}

TEST(ElementTest, ElementCopyTest1) {
    auto el1 = jbson::element{"Pi 6dp", element_type::double_element, 3.141592};
    ASSERT_EQ(element_type::double_element, el1.type());
    EXPECT_EQ("Pi 6dp", el1.name());
    EXPECT_EQ(3.141592, jbson::get<element_type::double_element>(el1));
    jbson::element el2 = el1;
    ASSERT_EQ(element_type::double_element, el2.type());
    EXPECT_EQ("Pi 6dp", el2.name());
    EXPECT_EQ(3.141592, jbson::get<element_type::double_element>(el2));
    EXPECT_EQ(el1, el2);
}

TEST(ElementTest, ElementCopyTest2) {
    auto el1 = jbson::element{"Pi 6dp", element_type::double_element, 3.141592};
    ASSERT_EQ(element_type::double_element, el1.type());
    EXPECT_EQ("Pi 6dp", el1.name());
    EXPECT_EQ(3.141592, jbson::get<element_type::double_element>(el1));
    jbson::element el2 = el1;
    auto val = 44.854;
    el2.value(val);
    ASSERT_EQ(element_type::double_element, el2.type());
    EXPECT_EQ("Pi 6dp", el2.name());
    EXPECT_EQ(val, jbson::get<element_type::double_element>(el2));
    EXPECT_NE(el1, el2);
}

TEST(ElementTest, ElementCopyConvertTest1) {
    auto el1 = jbson::element{"Pi 6dp", element_type::double_element, 3.141592};
    ASSERT_EQ(element_type::double_element, el1.type());
    EXPECT_EQ("Pi 6dp", el1.name());
    EXPECT_EQ(3.141592, jbson::get<element_type::double_element>(el1));
    jbson::basic_element<std::list<char>> el2 = el1;
    EXPECT_EQ(el1, el2);
    auto val = 44.854;
    el2.value(val);
    ASSERT_EQ(element_type::double_element, el2.type());
    EXPECT_EQ("Pi 6dp", el2.name());
    EXPECT_EQ(val, jbson::get<element_type::double_element>(el2));
    EXPECT_NE(el1, el2);
}

TEST(ElementTest, ElementMoveTest1) {
    auto el1 = jbson::element{"Pi 6dp", element_type::double_element, 3.141592};
    ASSERT_EQ(element_type::double_element, el1.type());
    EXPECT_EQ("Pi 6dp", el1.name());
    EXPECT_EQ(3.141592, jbson::get<element_type::double_element>(el1));
    auto old_size = el1.size();
    jbson::element el2 = std::move(el1);
    ASSERT_EQ(2, el1.size());
    ASSERT_EQ(old_size, el2.size());
    ASSERT_EQ(element_type::double_element, el2.type());
    EXPECT_EQ("Pi 6dp", el2.name());
    EXPECT_EQ(3.141592, jbson::get<element_type::double_element>(el2));
    EXPECT_NE(el1, el2);
}

TEST(ElementTest, ElementMoveConvertTest1) {
    auto el1 = jbson::element{"Pi 6dp", element_type::double_element, 3.141592};
    ASSERT_EQ(element_type::double_element, el1.type());
    EXPECT_EQ("Pi 6dp", el1.name());
    EXPECT_EQ(3.141592, jbson::get<element_type::double_element>(el1));
    auto old_size = el1.size();
    jbson::basic_element<std::list<char>> el2(std::move(el1));
    ASSERT_EQ(2, el1.size());
    ASSERT_EQ(old_size, el2.size());
    ASSERT_EQ(element_type::double_element, el2.type());
    EXPECT_EQ("Pi 6dp", el2.name());
    EXPECT_EQ(3.141592, jbson::get<element_type::double_element>(el2));
    EXPECT_NE(el1, el2);
}

TEST(ElementTest, ElementVoidTest) {
    auto el1 = jbson::element{"null element", element_type::null_element};
    ASSERT_EQ(element_type::null_element, el1.type());
    EXPECT_EQ("null element", el1.name());
    ASSERT_EQ(el1.name().size() + sizeof('\0') + 1, el1.size());
    //    el1.value<bool>();
}

TEST(ElementTest, ElementRefTest1) {
    static_assert(std::is_same<boost::string_ref, decltype(jbson::get<element_type::string_element>(
                                                      jbson::basic_element<boost::string_ref>{}))>::value,
                  "");
    static_assert(std::is_same<boost::string_ref, decltype(jbson::get<element_type::string_element>(
                                                      jbson::basic_element<std::vector<char>>{}))>::value,
                  "");
    static_assert(std::is_same<std::string, decltype(jbson::get<element_type::string_element>(
                                                jbson::basic_element<std::list<char>>{}))>::value,
                  "");
    ASSERT_TRUE(
        (std::is_same<boost::string_ref, decltype(jbson::get<element_type::string_element>(jbson::element{}))>::value));
}

TEST(ElementTest, ElementRefTest2) {
    boost::string_ref bson = "\x02hello\x00\x06\x00\x00\x00world\x00"s;
    auto el1 = jbson::basic_element<boost::string_ref>{bson};
    EXPECT_EQ(bson.size(), el1.size());
    ASSERT_EQ(element_type::string_element, el1.type());
    EXPECT_EQ("hello", el1.name());
    EXPECT_EQ("world", jbson::get<element_type::string_element>(el1));
    auto str_ref = jbson::get<element_type::string_element>(el1);
    EXPECT_TRUE(str_ref.data() >= bson.data() && str_ref.data() < (bson.data() + bson.size()));
    el1.name("some name");
    EXPECT_EQ("some name", el1.name());
}

// test elements one type at a time
template <typename ElemType> struct VoidVisitor {
    ElemType m_v;

    explicit VoidVisitor(ElemType v) : m_v(v) {}

    void operator()(ElemType v, element_type e) {
        EXPECT_EQ(element_type::double_element, e);
        EXPECT_EQ(m_v, v);
    }

    template <typename T> void operator()(T, element_type) {}
    void operator()(element_type) {}
};

TEST(ElementTest, ElementVisitTest1) {
    auto el1 = jbson::element{"Pi 6dp", element_type::double_element, 3.141592};
    ASSERT_EQ(element_type::double_element, el1.type());
    EXPECT_EQ("Pi 6dp", el1.name());
    EXPECT_EQ(3.141592, jbson::get<element_type::double_element>(el1));
    el1.visit(VoidVisitor<double>(3.141592));
}

template <typename ElemType> struct BoolVisitor {
    ElemType m_v;

    explicit BoolVisitor(ElemType v) : m_v(v) {}

    bool operator()(ElemType v, element_type e) {
        EXPECT_EQ(m_v, v);
        return element_type::double_element == e;
    }

    template <typename T> bool operator()(T, element_type) { return false; }
    bool operator()(element_type) { return false; }
};

TEST(ElementTest, ElementVisitTest2) {
    auto el1 = jbson::element{"Pi 6dp", element_type::double_element, 3.141592};
    ASSERT_EQ(element_type::double_element, el1.type());
    EXPECT_EQ("Pi 6dp", el1.name());
    EXPECT_EQ(3.141592, jbson::get<element_type::double_element>(el1));
    EXPECT_TRUE(el1.visit(BoolVisitor<double>(3.141592)));
}
