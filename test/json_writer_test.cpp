//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <string>
using namespace std::literals;

#include <boost/exception/diagnostic_information.hpp>
#include <boost/range/as_array.hpp>

#include <jbson/json_reader.hpp>
#include <jbson/json_writer.hpp>
#include <jbson/builder.hpp>
using namespace jbson;

#include <gtest/gtest.h>

TEST(JsonWriterTest, StringifyTest0) {
    auto json = std::string{};
    detail::stringify("some string", std::back_inserter(json));
    EXPECT_EQ(R"("some string")", json);
}

TEST(JsonWriterTest, StringifyTest1) {
    auto json = std::string{};
    auto doc = document{R"({ "hello"
               : "world" })"_json};
    detail::stringify(doc, std::back_inserter(json));
    EXPECT_EQ(R"({ "hello" : "world" })", json);
}

TEST(JsonWriterTest, StringifyTest2) {
    auto json = std::string{};
    auto arr = array{R"(["hello"   , "world" ])"_json};
    detail::stringify(arr, std::back_inserter(json));
    EXPECT_EQ(R"([ "hello", "world" ])", json);
}

TEST(JsonWriterTest, StringifyTest3) {
    auto json = std::string{};
    detail::stringify(123, std::back_inserter(json));
    EXPECT_EQ("123", json);
}

TEST(JsonWriterTest, StringifyTest4) {
    auto json = std::string{};
    detail::stringify(0, std::back_inserter(json));
    EXPECT_EQ("0", json);
}

TEST(JsonWriterTest, StringifyTest5) {
    auto json = std::string{};
    detail::stringify(false, std::back_inserter(json));
    EXPECT_EQ("false", json);
    json.clear();
    detail::stringify(true, std::back_inserter(json));
    EXPECT_EQ("true", json);
}

TEST(JsonWriterTest, StringifyTest6) {
    auto json = std::string{};
    detail::stringify(4.543210, std::back_inserter(json));
    EXPECT_EQ("4.543210", json);
}

TEST(JsonWriterTest, JsonWriteTest1) {
    auto json = std::string{};
    auto doc = document{R"({ "hello" : "world" })"_json};
    write_json(doc, std::back_inserter(json));

    EXPECT_EQ(R"({ "hello" : "world" })", json);
}

TEST(JsonWriterTest, JsonWriteTest2) {
    auto json = std::string{};
    write_json(static_cast<document>(builder("some value", element_type::int32_element, 123)
               ("some other value", element_type::boolean_element, true)),
               std::back_inserter(json));

    EXPECT_EQ(R"({ "some other value" : true, "some value" : 123 })", json);
}

TEST(JsonWriterTest, JsonWriteTest3) {
    auto json = std::array<char, 22>{};
    auto doc = document{R"({ "hello" : "world" })"_json};
    write_json(doc, json.begin());

    EXPECT_EQ(boost::as_array(R"({ "hello" : "world" })"), json);
}

std::string demangle(boost::string_ref t) {
    const auto str = abi::__cxa_demangle(t.data(), nullptr, nullptr, nullptr);
    std::string out;
    if(str) {
        out = str;
        ::free(str);
    }
    return std::move(out);
}

TEST(JsonWriterTest, JsonWriteTest4) {
    auto json = std::string{};
    write_json(static_cast<document>(builder("some value", element_type::int32_element, 123)
               ("some regex", element_type::regex_element, std::make_tuple(".*", "i"))),
               std::back_inserter(json));

    EXPECT_EQ(R"({ "some regex" : { "$options" : "i", "$regex" : ".*" }, "some value" : 123 })", json);
}

TEST(JsonWriterTest, JsonWriteTest5) {
    auto json = std::string{};
    std::array<char, 12> oid{{static_cast<char>(0x50),
                    static_cast<char>(0x7f),
                    static_cast<char>(0x1f),
                    static_cast<char>(0x77),
                    static_cast<char>(0xbc),
                    static_cast<char>(0xf8),
                    static_cast<char>(0x6c),
                    static_cast<char>(0xd7),
                    static_cast<char>(0x99),
                    static_cast<char>(0x43),
                    static_cast<char>(0x90),
                    static_cast<char>(0x11)
                             }};
    write_json(static_cast<document>(builder("some oid", element_type::oid_element, oid)),
               std::back_inserter(json));
    EXPECT_EQ(R"({ "some oid" : { "$oid" : "507f1f77bcf86cd799439011" } })", json);
}

TEST(JsonWriterTest, JsonWriteTest6) {
    auto json = std::string{};
    std::array<char, 12> oid{{0,1,2,3,4,5,6,7,8,9,10,11}};
    write_json(static_cast<document>(builder("some oid", element_type::oid_element, oid)),
               std::back_inserter(json));
    EXPECT_EQ(R"({ "some oid" : { "$oid" : "000102030405060708090a0b" } })", json);
}

TEST(JsonWriterTest, JsonWriteTest7) {
    auto json = std::string{};
    std::array<char, 12> oid{{static_cast<char>(0x50),
                    static_cast<char>(0x7f),
                    static_cast<char>(0x1f),
                    static_cast<char>(0x77),
                    static_cast<char>(0xbc),
                    static_cast<char>(0xf8),
                    static_cast<char>(0x6c),
                    static_cast<char>(0xd7),
                    static_cast<char>(0x99),
                    static_cast<char>(0x43),
                    static_cast<char>(0x90),
                    static_cast<char>(0x11)
                             }};
    write_json(static_cast<document>(builder("some ref", element_type::db_pointer_element,
                                                                       std::make_tuple("some collection", oid))),
               std::back_inserter(json));
    EXPECT_EQ(R"({ "some ref" : { "$id" : { "$oid" : "507f1f77bcf86cd799439011" }, "$ref" : "some collection" } })", json);
}
