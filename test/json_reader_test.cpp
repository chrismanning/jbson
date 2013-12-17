//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <string>
using namespace std::literals;

#include <boost/exception/diagnostic_information.hpp>

#include <jbson/json_reader.hpp>
using namespace jbson;

#include <gtest/gtest.h>

TEST(JsonReaderTest, JsonParseTest1) {
    try {
    auto json = boost::string_ref{R"({"key":"value"})"};
    auto reader = json_reader{};
    reader.parse(json);
    }
    catch(...) {
        std::clog << boost::current_exception_diagnostic_information() << std::endl;
        std::abort();
    }
}
