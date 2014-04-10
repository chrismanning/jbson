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
void deserialise(const Container& data, std::chrono::duration<RepT, RatioT>& dur) {
    using jbson::deserialise;
    RepT rep;
    deserialise(data, rep);
    dur = std::chrono::duration<RepT, RatioT>{rep};
}
}

TEST_F(BsonTest, FileTest4) {
    init_bson("test4.bson");
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("utc", it->name());
    ASSERT_EQ(element_type::date_element, it->type());
    EXPECT_EQ(1319285594123, it->value<int64_t>());
    EXPECT_EQ(1319285594123ms, it->value<std::chrono::milliseconds>());

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest5) {
    init_bson("test5.bson");
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
    ASSERT_EQ(1, boost::distance(doc));

    auto it = doc.begin();
    const auto end = doc.end();
    ASSERT_NE(end, it);

    EXPECT_EQ("null", it->name());
    ASSERT_EQ(element_type::null_element, it->type());
    EXPECT_THROW(it->value<std::string>(), invalid_element_size);
    EXPECT_THROW(it->value<int32_t>(), invalid_element_size);
    EXPECT_THROW(it->value<bool>(), invalid_element_size);

    ++it;
    ASSERT_EQ(end, it);
}

TEST_F(BsonTest, FileTest10) {
    init_bson("test10.bson");
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

