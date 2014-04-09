//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <string>
using namespace std::literals;

#include <boost/exception/diagnostic_information.hpp>
#include <boost/range/as_array.hpp>

#include <gtest/gtest.h>

#include <jbson/json_reader.hpp>
#include <jbson/json_writer.hpp>
#include <jbson/builder.hpp>
using namespace jbson;


TEST(JsonWriterTest, StringifyTest0) {
    auto json = std::string{};
    detail::stringify("some string", std::back_inserter(json));
    EXPECT_EQ(R"("some string")", json);
}

TEST(JsonWriterTest, StringifyTest1) {
    auto json = std::string{};
    auto doc = R"({ "hello"
               : "world" })"_json_doc;
    detail::stringify(doc, std::back_inserter(json));
    EXPECT_EQ(R"({ "hello" : "world" })", json);
}

TEST(JsonWriterTest, StringifyTest2) {
    auto json = std::string{};
    auto arr = R"(["hello"   , "world" ])"_json_arr;
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
    EXPECT_EQ("4.54321", json);
}

TEST(JsonWriterTest, StringifyTest7) {
    auto json = std::string{};
    detail::stringify("some\n\t\"string\"", std::back_inserter(json));
    EXPECT_EQ(R"("some\n\t\"string\"")", json);
}

TEST(JsonWriterTest, StringifyTest8) {
    auto json = std::string{};
    detail::stringify(0.123456789e-12, std::back_inserter(json));
    ::feclearexcept(FE_ALL_EXCEPT);
    auto dbl = std::stold("0.123456789e-12"s);
    EXPECT_FALSE(::fetestexcept(FE_ALL_EXCEPT));
    ::feclearexcept(FE_ALL_EXCEPT);
    EXPECT_FLOAT_EQ(0.123456789e-12, dbl);
    EXPECT_FLOAT_EQ(23456789012E66, std::stold("23456789012E66"s));

    std::array<char,std::numeric_limits<double>::digits10+2> buf;
    auto n = std::snprintf(buf.data(), buf.size(), "%.8e", 23456789012E66);
    ASSERT_GT(n, 0);
    EXPECT_EQ("2.34567890e+76", boost::string_ref(buf.data(), static_cast<size_t>(n)));
    int64_t i = std::llrint(std::stold("23456789012E66"s));
    EXPECT_TRUE(::fetestexcept(FE_ALL_EXCEPT) & FE_INEXACT);
    ::feclearexcept(FE_ALL_EXCEPT);
    (void) i;
}

TEST(JsonWriterTest, StringifyTest9) {
    auto json = std::string{};
    detail::stringify("\x06", std::back_inserter(json));
    EXPECT_EQ(R"("\u0006")", json);
}

TEST(JsonWriterTest, StringifyTest10) {
    auto json = std::string{};
    detail::stringify("\x1c", std::back_inserter(json));
    EXPECT_EQ(R"("\u001c")", json);
}

TEST(JsonWriterTest, UnicodeTest1) {
    auto json = R"(["6U閆崬밺뀫颒myj츥휘:$薈mY햚#rz飏+玭V㭢뾿愴YꖚX亥ᮉ푊\u0006垡㐭룝\"厓ᔧḅ^Sqpv媫\"⤽걒\"˽Ἆ?ꇆ䬔未tv{DV鯀Tἆl凸g\\㈭ĭ즿UH㽤"])"_json_set;
    auto it = json.find("0");
    ASSERT_NE(json.end(), it);
    auto str = std::string{};
    detail::stringify(it->value<std::string>(), std::back_inserter(str));
    EXPECT_EQ(R"("6U閆崬밺뀫颒myj츥휘:$薈mY햚#rz飏+玭V㭢뾿愴YꖚX亥ᮉ푊\u0006垡㐭룝\"厓ᔧḅ^Sqpv媫\"⤽걒\"˽Ἆ?ꇆ䬔未tv{DV鯀Tἆl凸g\\㈭ĭ즿UH㽤")",
              str);
}

TEST(JsonWriterTest, JsonWriteTest1) {
    auto json = std::string{};
    auto doc = R"({ "hello" : "world" })"_json_doc;
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
    auto doc = R"({ "hello" : "world" })"_json_doc;
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

TEST(JsonWriterTest, JsonWriteTest8) {
    auto ifs = std::ifstream{JBSON_FILES"json_checker_test_suite/pass1.json", std::ios::in};
    auto json = std::vector<char>{};
    ifs.seekg(0, std::ios::end);
    auto n = static_cast<std::streamoff>(ifs.tellg());
    json.resize(n);
    ifs.seekg(0, std::ios::beg);
    ifs.read(json.data(), n);

    auto reader = json_reader{};
    ASSERT_NO_THROW(reader.parse(json));
    auto json_str = std::string{};

    auto arr = array(reader);
    ASSERT_NO_THROW(write_json(arr, std::back_inserter(json_str)));

//    EXPECT_EQ(R"([ "JSON Test Pattern pass1", { "object with 1 member" : [ "array with 1 element" ] }, {  }, [  ], -42, true, false, null, { "integer" : 1234567890, "real" : -9876.543210, "e" : 0.123456789e-12, "E ": 1.234567890E+34, "" : 23456789012E66, "zero" : 0, "one" : 1, "space" : " ", "quote" : "\"", "backslash" : "\\", "controls" : "\b\f\n\r\t", "slash" : "/ & \/", "alpha" : "abcdefghijklmnopqrstuvwyz", "ALPHA" : "ABCDEFGHIJKLMNOPQRSTUVWYZ", "digit" : "0123456789", "0123456789" : "digit", "special" : "`1~!@#$%^&*()_+-={':[,]}|;.</>?", "hex" : "\u0123\u4567\u89AB\uCDEF\uabcd\uef4A", "true" : true, "false" : false, "null" : null, "array" : [  ], "object" : {  }, "address" : "50 St. James Street", "url" : "http://www.JSON.org/", "comment" : "// /* <!-- --", "# -- --> */" : " ", " s p a c e d " :[1,2 , 3 , 4 , 5 , 6 ,7 ],"compact":[1,2,3,4,5,6,7], "jsontext" : "{\"object with 1 member\":[ \"array with 1 element\" ] }", "quotes" : "&#34; \u0022 %22 0x22 034 &#x22;", "\/\\\"\uCAFE\uBABE\uAB98\uFCDE\ubcda\uef4A\b\f\n\r\t`1~!@#$%^&*()_+-=[]{}|;:',./<>?" : "A key can be any string" }, 0.5 ,98.6, 99.44, 1066, 1e1, 0.1e1, 1e-1, 1e00,2e+00,2e-00, "rosebud" ])",
//              json_str);
    auto varr = std::vector<element>(arr);
    ASSERT_EQ(20u, varr.size());
    auto d = varr[8];
    ASSERT_EQ(element_type::document_element, d.type());
    auto set = document_set(d.value<document>());
    auto it = set.find("E");
    ASSERT_NE(set.end(), it);
    ASSERT_EQ(element_type::double_element, it->type());
}