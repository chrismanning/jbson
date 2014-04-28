//          Copyright Christian Manning 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <chrono>
using namespace std::literals;

#include <jbson/document.hpp>
#include <jbson/builder.hpp>
#include <jbson/json_reader.hpp>
using namespace jbson;

#include "gtest/gtest.h"

#ifndef BSON_BINARY_DIR
#error "location of test files unknown"
#endif

class BsonTest : public ::testing::Test {
public:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }

    void init_bson(std::string filename) {
        std::ifstream ifs(std::string{BSON_BINARY_DIR"/"}+filename);
        ASSERT_FALSE(ifs.fail());

        ifs.seekg(0, std::ios::end);
        auto n = static_cast<std::streamoff>(ifs.tellg());
        std::vector<char> data;
        ASSERT_NO_THROW(data.resize(n));
        ifs.seekg(0, std::ios::beg);
        ifs.read(data.data(), n);
        ASSERT_NO_THROW((doc = document{std::move(data)}));
    }

protected:
    document doc;
};

TEST_F(BsonTest, FileTest1) {
    init_bson("test1.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("int", it->name());
    ASSERT_EQ(element_type::int32_element, it->type());
    EXPECT_EQ(1, it->value<int32_t>());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest2) {
    init_bson("test2.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("int64", it->name());
    ASSERT_EQ(element_type::int64_element, it->type());
    EXPECT_EQ(1, it->value<int64_t>());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest3) {
    init_bson("test3.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("double", it->name());
    ASSERT_EQ(element_type::double_element, it->type());
    EXPECT_DOUBLE_EQ(1.123, it->value<double>());

    ++it;
    ASSERT_EQ(end, it);
}

namespace std {
template <typename Container, typename RepT, typename RatioT>
void value_get(const basic_element<Container>& elem, std::chrono::duration<RepT, RatioT>& dur) {
    dur = std::chrono::duration_cast
          <std::chrono::duration<RepT, RatioT>>(std::chrono::duration<RepT, std::milli>{elem.template value<RepT>()});
}

template <typename Container, typename IteratorT, typename RepT, typename RatioT>
void serialise(Container& data, IteratorT& it, const std::chrono::duration<RepT, RatioT>& dur) {
    using jbson::serialise;
    serialise(data, it, dur.count());
}
}

TEST_F(BsonTest, FileTest4) {
    init_bson("test4.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("utc", it->name());
    ASSERT_EQ(element_type::date_element, it->type());
    EXPECT_EQ(1319285594123, it->value<int64_t>());
    EXPECT_EQ(1319285594123ms, it->value<std::chrono::milliseconds>());
    try {
    auto b = builder("date", element_type::date_element, 1319285594123ms);
    auto e = element("date", element_type::date_element, 1319285594123ms);
    }
    catch(...) {
        std::clog << boost::current_exception_diagnostic_information();
        FAIL();
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest5) {
    init_bson("test5.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("string", it->name());
    ASSERT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("some string", it->value<std::string>());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest6) {
    init_bson("test6.bson");
    ASSERT_TRUE(doc.valid(document_validity::array_indices));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("array[int]", it->name());
    ASSERT_EQ(element_type::array_element, it->type());
    decltype(get<element_type::array_element>(*it)) arr;
    {
        ASSERT_NO_THROW(arr = get<element_type::array_element>(*it));

        auto it = arr.begin();
        const auto end = arr.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("0", it->name());
        ASSERT_EQ(element_type::int32_element, it->type());
        EXPECT_EQ(1, it->value<int32_t>());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("1", it->name());
        ASSERT_EQ(element_type::int32_element, it->type());
        EXPECT_EQ(2, it->value<int32_t>());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("2", it->name());
        ASSERT_EQ(element_type::int32_element, it->type());
        EXPECT_EQ(3, it->value<int32_t>());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("3", it->name());
        ASSERT_EQ(element_type::int32_element, it->type());
        EXPECT_EQ(4, it->value<int32_t>());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("4", it->name());
        ASSERT_EQ(element_type::int32_element, it->type());
        EXPECT_EQ(5, it->value<int32_t>());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("5", it->name());
        ASSERT_EQ(element_type::int32_element, it->type());
        EXPECT_EQ(6, it->value<int32_t>());

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest7) {
    init_bson("test7.bson");
    ASSERT_TRUE(doc.valid(document_validity::array_indices));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("array[double]", it->name());
    ASSERT_EQ(element_type::array_element, it->type());
    decltype(get<element_type::array_element>(*it)) arr;
    {
        ASSERT_NO_THROW(arr = get<element_type::array_element>(*it));

        auto it = arr.begin();
        const auto end = arr.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("0", it->name());
        ASSERT_EQ(element_type::double_element, it->type());
        EXPECT_DOUBLE_EQ(1.123, it->value<double>());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("1", it->name());
        ASSERT_EQ(element_type::double_element, it->type());
        EXPECT_DOUBLE_EQ(2.123, it->value<double>());

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest8) {
    init_bson("test8.bson");
    ASSERT_TRUE(doc.valid(document_validity::array_indices));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("document", it->name());
    ASSERT_EQ(element_type::document_element, it->type());
    decltype(get<element_type::document_element>(*it)) subdoc;
    {
        ASSERT_NO_THROW(subdoc = get<element_type::document_element>(*it));

        ASSERT_EQ(1, boost::distance(subdoc));

        auto it = subdoc.begin();
        const auto end = subdoc.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("int", it->name());
        ASSERT_EQ(element_type::int32_element, it->type());
        EXPECT_EQ(1, it->value<int32_t>());

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest9) {
    init_bson("test9.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("null", it->name());
    ASSERT_EQ(element_type::null_element, it->type());
    EXPECT_THROW(it->value<std::string>(), incompatible_type_conversion);
    EXPECT_THROW(it->value<int32_t>(), incompatible_type_conversion);
    EXPECT_THROW(it->value<bool>(), incompatible_type_conversion);

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest10) {
    init_bson("test10.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("regex", it->name());
    ASSERT_EQ(element_type::regex_element, it->type());
    EXPECT_EQ(std::make_tuple("1234", "i"), get<element_type::regex_element>(*it));

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest11) {
    init_bson("test11.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("hello", it->name());
    ASSERT_EQ(element_type::string_element, it->type());
    EXPECT_EQ("world", it->value<std::string>());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest12) {
    init_bson("test12.bson");
    ASSERT_TRUE(doc.valid(document_validity::array_indices));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("BSON", it->name());
    ASSERT_EQ(element_type::array_element, it->type());
    decltype(get<element_type::array_element>(*it)) arr;
    {
        ASSERT_NO_THROW(arr = get<element_type::array_element>(*it));

        auto it = arr.begin();
        const auto end = arr.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("0", it->name());
        ASSERT_EQ(element_type::string_element, it->type());
        EXPECT_EQ("awesome", it->value<std::string>());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("1", it->name());
        ASSERT_EQ(element_type::double_element, it->type());
        EXPECT_DOUBLE_EQ(5.05, it->value<double>());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("2", it->name());
        ASSERT_EQ(element_type::int32_element, it->type());
        EXPECT_EQ(1986, it->value<int32_t>());

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest13) {
    init_bson("test13.bson");
    ASSERT_TRUE(doc.valid(document_validity::array_indices));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("array[bool]", it->name());
    ASSERT_EQ(element_type::array_element, it->type());
    decltype(get<element_type::array_element>(*it)) arr;
    {
        ASSERT_NO_THROW(arr = get<element_type::array_element>(*it));

        auto it = arr.begin();
        const auto end = arr.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("0", it->name());
        ASSERT_EQ(element_type::boolean_element, it->type());
        EXPECT_EQ(true, it->value<bool>());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("1", it->name());
        ASSERT_EQ(element_type::boolean_element, it->type());
        EXPECT_EQ(false, it->value<bool>());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("2", it->name());
        ASSERT_EQ(element_type::boolean_element, it->type());
        EXPECT_EQ(true, it->value<bool>());

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest14) {
    init_bson("test14.bson");
    ASSERT_TRUE(doc.valid(document_validity::array_indices));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("array[string]", it->name());
    ASSERT_EQ(element_type::array_element, it->type());
    decltype(get<element_type::array_element>(*it)) arr;
    {
        ASSERT_NO_THROW(arr = get<element_type::array_element>(*it));

        auto it = arr.begin();
        const auto end = arr.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("0", it->name());
        ASSERT_EQ(element_type::string_element, it->type());
        EXPECT_EQ("hello", it->value<std::string>());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("1", it->name());
        ASSERT_EQ(element_type::string_element, it->type());
        EXPECT_EQ("world", it->value<std::string>());

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest15) {
    init_bson("test15.bson");
    ASSERT_TRUE(doc.valid(document_validity::array_indices));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("array[datetime]", it->name());
    ASSERT_EQ(element_type::array_element, it->type());
    decltype(get<element_type::array_element>(*it)) arr;
    {
        ASSERT_NO_THROW(arr = get<element_type::array_element>(*it));

        auto it = arr.begin();
        const auto end = arr.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("0", it->name());
        ASSERT_EQ(element_type::date_element, it->type());
        EXPECT_EQ(0ms, it->value<std::chrono::milliseconds>());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("1", it->name());
        ASSERT_EQ(element_type::date_element, it->type());
        EXPECT_EQ(1319285594123ms, it->value<std::chrono::milliseconds>());

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest16) {
    init_bson("test16.bson");
    ASSERT_TRUE(doc.valid(document_validity::array_indices));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("array[null]", it->name());
    ASSERT_EQ(element_type::array_element, it->type());
    decltype(get<element_type::array_element>(*it)) arr;
    {
        ASSERT_NO_THROW(arr = get<element_type::array_element>(*it));

        auto it = arr.begin();
        const auto end = arr.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("0", it->name());
        ASSERT_EQ(element_type::null_element, it->type());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("1", it->name());
        ASSERT_EQ(element_type::null_element, it->type());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("2", it->name());
        ASSERT_EQ(element_type::null_element, it->type());

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest17) {
    init_bson("test17.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(4, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("_id", it->name());
    ASSERT_EQ(element_type::oid_element, it->type());

    ++it;
    ASSERT_NE(end, it);

    EXPECT_EQ("document", it->name());
    ASSERT_EQ(element_type::document_element, it->type());
    {
        decltype(get<element_type::document_element>(*it)) subdoc;
        ASSERT_NO_THROW(subdoc = get<element_type::document_element>(*it));
        ASSERT_EQ(4, boost::distance(subdoc));

        auto it = subdoc.begin();
        const auto end = subdoc.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("_id", it->name());
        ASSERT_EQ(element_type::oid_element, it->type());

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("tags", it->name());
        ASSERT_EQ(element_type::array_element, it->type());
        {
            decltype(get<element_type::array_element>(*it)) arr;
            ASSERT_NO_THROW(arr = get<element_type::array_element>(*it));
            ASSERT_EQ(4, boost::distance(arr));

            auto it = arr.begin();
            const auto end = arr.end();
            ASSERT_NE(end, it);

            EXPECT_EQ("0", it->name());
            ASSERT_EQ(element_type::string_element, it->type());
            EXPECT_EQ("1", get<element_type::string_element>(*it));

            ++it;
            ASSERT_NE(end, it);

            EXPECT_EQ("1", it->name());
            ASSERT_EQ(element_type::string_element, it->type());
            EXPECT_EQ("2", get<element_type::string_element>(*it));

            ++it;
            ASSERT_NE(end, it);

            EXPECT_EQ("2", it->name());
            ASSERT_EQ(element_type::string_element, it->type());
            EXPECT_EQ("3", get<element_type::string_element>(*it));

            ++it;
            ASSERT_NE(end, it);

            EXPECT_EQ("3", it->name());
            ASSERT_EQ(element_type::string_element, it->type());
            EXPECT_EQ("4", get<element_type::string_element>(*it));

            ++it;
            ASSERT_EQ(end, it);
        }

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("text", it->name());
        ASSERT_EQ(element_type::string_element, it->type());
        EXPECT_EQ("asdfanother", get<element_type::string_element>(*it));

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("source", it->name());
        ASSERT_EQ(element_type::document_element, it->type());
        {
            decltype(get<element_type::document_element>(*it)) subdoc;
            ASSERT_NO_THROW(subdoc = get<element_type::document_element>(*it));
            ASSERT_EQ(1, boost::distance(subdoc));

            auto it = subdoc.begin();
            const auto end = subdoc.end();
            ASSERT_NE(end, it);

            EXPECT_EQ("name", it->name());
            ASSERT_EQ(element_type::string_element, it->type());
            EXPECT_EQ("blah", get<element_type::string_element>(*it));

            ++it;
            ASSERT_EQ(end, it);
        }

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_NE(end, it);

    EXPECT_EQ("type", it->name());
    ASSERT_EQ(element_type::array_element, it->type());
    {
        decltype(get<element_type::array_element>(*it)) arr;
        ASSERT_NO_THROW(arr = get<element_type::array_element>(*it));
        ASSERT_EQ(1, boost::distance(arr));

        auto it = arr.begin();
        const auto end = arr.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("0", it->name());
        ASSERT_EQ(element_type::string_element, it->type());
        EXPECT_EQ("source", get<element_type::string_element>(*it));

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_NE(end, it);

    EXPECT_EQ("missing", it->name());
    ASSERT_EQ(element_type::array_element, it->type());
    {
        decltype(get<element_type::array_element>(*it)) arr;
        ASSERT_NO_THROW(arr = get<element_type::array_element>(*it));
        ASSERT_EQ(1, boost::distance(arr));

        auto it = arr.begin();
        const auto end = arr.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("0", it->name());
        ASSERT_EQ(element_type::string_element, it->type());
        EXPECT_EQ("server_created_at", get<element_type::string_element>(*it));

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest18) {
    init_bson("test18.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("hello", it->name());
    ASSERT_EQ(element_type::null_element, it->type());
    EXPECT_THROW(it->value<std::string>(), incompatible_type_conversion);
    EXPECT_THROW(it->value<int32_t>(), incompatible_type_conversion);
    EXPECT_THROW(it->value<bool>(), incompatible_type_conversion);

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest19) {
    init_bson("test19.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("bool", it->name());
    ASSERT_EQ(element_type::boolean_element, it->type());
    EXPECT_DOUBLE_EQ(true, it->value<bool>());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest20) {
    init_bson("test20.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("double", it->name());
    ASSERT_EQ(element_type::double_element, it->type());
    EXPECT_DOUBLE_EQ(123.4567, it->value<double>());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest21) {
    init_bson("test21.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("document", it->name());
    ASSERT_EQ(element_type::document_element, it->type());
    decltype(get<element_type::document_element>(*it)) subdoc;
    {
        ASSERT_NO_THROW(subdoc = get<element_type::document_element>(*it));
        EXPECT_EQ(0, boost::distance(subdoc));

        auto it = subdoc.begin();
        const auto end = subdoc.end();
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest22) {
    init_bson("test22.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("oid", it->name());
    ASSERT_EQ(element_type::oid_element, it->type());
    auto oid = get<element_type::oid_element>(*it);
    EXPECT_PRED2(([](auto&& a, auto&& b) { return boost::range::equal(a, b); }),
                 boost::as_literal("\x12\x34\x56\x78\x90\xab\xcd\xef\x12\x34\xab\xcd"),
                 boost::make_iterator_range(oid));

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest23) {
    init_bson("test23.bson");
    ASSERT_TRUE(doc.valid(document_validity::array_indices));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("array", it->name());
    ASSERT_EQ(element_type::array_element, it->type());
    decltype(get<element_type::array_element>(*it)) arr;
    {
        ASSERT_NO_THROW(arr = get<element_type::array_element>(*it));
        EXPECT_EQ(2, boost::distance(arr));

        auto it = arr.begin();
        const auto end = arr.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("0", it->name());
        ASSERT_EQ(element_type::string_element, it->type());
        EXPECT_EQ("hello", get<element_type::string_element>(*it));

        ++it;
        ASSERT_NE(end, it);

        EXPECT_EQ("1", it->name());
        ASSERT_EQ(element_type::string_element, it->type());
        EXPECT_EQ("world", get<element_type::string_element>(*it));

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, DISABLED_FileTest24) {
    init_bson("test24.bson");
    FAIL() << "binary elements not implemented";
}

JBSON_PUSH_DISABLE_DEPRECATED_WARNING
TEST_F(BsonTest, FileTest25) {
    init_bson("test25.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("undefined", it->name());
    ASSERT_EQ(element_type::undefined_element, it->type());
    EXPECT_THROW(it->value<std::string>(), incompatible_type_conversion);
    EXPECT_THROW(it->value<int32_t>(), incompatible_type_conversion);
    EXPECT_THROW(it->value<bool>(), incompatible_type_conversion);

    ++it;
    ASSERT_EQ(end, it);
}
JBSON_POP_WARNINGS

TEST_F(BsonTest, FileTest26) {
    init_bson("test26.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("time_t", it->name());
    ASSERT_EQ(element_type::date_element, it->type());
    EXPECT_EQ(1234567890000ms, it->value<std::chrono::milliseconds>());
    EXPECT_EQ(1234567890s, it->value<std::chrono::seconds>());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest27) {
    init_bson("test27.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    boost::string_ref regex, options;
    EXPECT_EQ("regex", it->name());
    ASSERT_EQ(element_type::regex_element, it->type());

    std::tie(regex, options) = get<element_type::regex_element>(*it);
    EXPECT_EQ("^abcd", regex);
    EXPECT_EQ("ilx", options);

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest27b) {
    init_bson("test27.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    boost::string_ref regex, options;
    EXPECT_EQ("regex", it->name());
    ASSERT_EQ(element_type::regex_element, it->type());

    std::tie(regex, options) = it->value<std::tuple<boost::string_ref,boost::string_ref>>();
    EXPECT_EQ("^abcd", regex);
    EXPECT_EQ("ilx", options);

    ++it;
    ASSERT_EQ(end, it);
}

JBSON_PUSH_DISABLE_DEPRECATED_WARNING
TEST_F(BsonTest, FileTest28) {
    init_bson("test28.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    boost::string_ref collection;
    std::array<char, 12> oid;
    EXPECT_EQ("dbpointer", it->name());
    ASSERT_EQ(element_type::db_pointer_element, it->type());

    std::tie(collection, oid) = get<element_type::db_pointer_element>(*it);
    EXPECT_EQ("foo", collection);
    EXPECT_PRED2(([](auto&& a, auto&& b) { return boost::range::equal(a, b); }),
                 boost::as_literal("\x01\x23\xab\xcd\x01\x23\xab\xcd\x01\x23\xab\xcd"),
                 boost::make_iterator_range(oid));

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest28b) {
    init_bson("test28.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    boost::string_ref collection;
    std::array<char, 12> oid;
    EXPECT_EQ("dbpointer", it->name());
    ASSERT_EQ(element_type::db_pointer_element, it->type());

    std::tie(collection, oid) = it->value<std::tuple<boost::string_ref,std::array<char,12>>>();
    EXPECT_EQ("foo", collection);
    EXPECT_PRED2(([](auto&& a, auto&& b) { return boost::range::equal(a, b); }),
                 boost::as_literal("\x01\x23\xab\xcd\x01\x23\xab\xcd\x01\x23\xab\xcd"),
                 boost::make_iterator_range(oid));

    ++it;
    ASSERT_EQ(end, it);
}
JBSON_POP_WARNINGS

TEST_F(BsonTest, FileTest29) {
    init_bson("test29.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("code", it->name());
    ASSERT_EQ(element_type::javascript_element, it->type());
    EXPECT_EQ("var a = {};", it->value<boost::string_ref>());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest30) {
    init_bson("test30.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("code", it->name());
    ASSERT_EQ(element_type::javascript_element, it->type());
    EXPECT_EQ("var a = {};", it->value<boost::string_ref>());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest31) {
    init_bson("test31.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    decltype(get<element_type::document_element>(*it)) scope;
    boost::string_ref code;
    EXPECT_EQ("code", it->name());
    ASSERT_EQ(element_type::scoped_javascript_element, it->type());

    try {
    std::tie(code, scope) = get<element_type::scoped_javascript_element>(*it);
    EXPECT_EQ("var a = {};", code);
    }
    catch(...) {
        std::clog << boost::current_exception_diagnostic_information();
        FAIL();
    }

    {
        ASSERT_EQ(1, boost::distance(scope));

        auto it = scope.begin();
        const auto end = scope.end();
        ASSERT_NE(end, it);

        EXPECT_EQ("foo", it->name());
        ASSERT_EQ(element_type::string_element, it->type());
        EXPECT_EQ("bar", it->value<boost::string_ref>());

        ++it;
        ASSERT_EQ(end, it);
    }

    ++it;
    ASSERT_EQ(end, it);
}

JBSON_PUSH_DISABLE_DEPRECATED_WARNING
TEST_F(BsonTest, FileTest32) {
    init_bson("test32.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("hello", it->name());
    ASSERT_EQ(element_type::symbol_element, it->type());
    EXPECT_EQ("world", it->value<boost::string_ref>());

    ++it;
    ASSERT_EQ(end, it);
}
JBSON_POP_WARNINGS

TEST_F(BsonTest, FileTest33) {
    init_bson("test33.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(3, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("a", it->name());
    ASSERT_EQ(element_type::int32_element, it->type());
    EXPECT_EQ(-123, it->value<int32_t>());

    ++it;
    ASSERT_NE(end, it);

    EXPECT_EQ("c", it->name());
    ASSERT_EQ(element_type::int32_element, it->type());
    EXPECT_EQ(0, it->value<int32_t>());

    ++it;
    ASSERT_NE(end, it);

    EXPECT_EQ("b", it->name());
    ASSERT_EQ(element_type::int32_element, it->type());
    EXPECT_EQ(123, it->value<int32_t>());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest34) {
    init_bson("test34.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("a", it->name());
    ASSERT_EQ(element_type::int64_element, it->type());
    EXPECT_EQ(100000000000000ULL, it->value<int64_t>());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest35) {
    init_bson("test35.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("timestamp", it->name());
    ASSERT_EQ(element_type::timestamp_element, it->type());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest36) {
    init_bson("test36.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("minkey", it->name());
    ASSERT_EQ(element_type::min_key, it->type());
    EXPECT_THROW(it->value<std::string>(), incompatible_type_conversion);
    EXPECT_THROW(it->value<int32_t>(), incompatible_type_conversion);
    EXPECT_THROW(it->value<bool>(), incompatible_type_conversion);

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest37) {
    init_bson("test37.bson");
    ASSERT_TRUE(doc.valid(document_validity::unicode_valid));
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("maxkey", it->name());
    ASSERT_EQ(element_type::max_key, it->type());
    EXPECT_THROW(it->value<std::string>(), incompatible_type_conversion);
    EXPECT_THROW(it->value<int32_t>(), incompatible_type_conversion);
    EXPECT_THROW(it->value<bool>(), incompatible_type_conversion);

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, DISABLED_FileTest38) {
    init_bson("test38.bson");
    ASSERT_EQ(100, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    for(auto i = 0; i < 100 && it != end; i++, it++) {
        decltype(get<element_type::document_element>(*it)) subdoc;
        ASSERT_NO_THROW(subdoc = get<element_type::document_element>(*it));
        EXPECT_EQ(0, boost::distance(subdoc)) << i;
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, DISABLED_FileTest39) {
    init_bson("test39.bson");
    ASSERT_EQ(100, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    for(auto i = 0; i < 100 && it != end; i++, it++) {
        decltype(get<element_type::document_element>(*it)) subdoc;
        ASSERT_NO_THROW(subdoc = get<element_type::document_element>(*it));
        EXPECT_EQ(0, boost::distance(subdoc)) << i;
    }

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest40) {
    init_bson("test40.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest41) {
    init_bson("test41.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest42) {
    init_bson("test42.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest43) {
    init_bson("test43.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest44) {
    init_bson("test44.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest45) {
    init_bson("test45.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest46) {
    init_bson("test46.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest47) {
    init_bson("test47.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest48) {
    init_bson("test48.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest49) {
    init_bson("test49.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest50) {
    init_bson("test50.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest51) {
    init_bson("test51.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest52) {
    init_bson("test52.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest53) {
    init_bson("test53.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest54) {
    init_bson("test54.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest55) {
    init_bson("test55.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}

TEST_F(BsonTest, FileTest56) {
    init_bson("test56.bson");
    ASSERT_FALSE(doc.valid(document_validity::unicode_valid));
}

TEST_F(BsonTest, FileTest57) {
    init_bson("test57.bson");
    ASSERT_FALSE(doc.valid(document_validity::element_construct));
}
