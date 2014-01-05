//          Copyright Christian Manning 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <gtest/gtest.h>

#include <jbson/json_reader.hpp>
using namespace jbson;

class PerfTest : public ::testing::Test {
public:
    virtual void SetUp() {
        FILE *fp = fopen(filename_ = JBSON_FILES"rapidjson_sample.json", "rb");
        ASSERT_NE(nullptr, fp);

        fseek(fp, 0, SEEK_END);
        json_.resize((size_t)ftell(fp)+1);
        fseek(fp, 0, SEEK_SET);
        ASSERT_EQ(json_.size()-1, fread(json_.data(), 1, json_.size()-1, fp));
        json_.back() = '\0';
        fclose(fp);

        // whitespace test
        whitespace_.resize((1024 * 1024) + 4);
        char *p = whitespace_.data();
        for(size_t i = 0; i < whitespace_.size()-4; i += 4) {
            *p++ = ' ';
            *p++ = '\n';
            *p++ = '\r';
            *p++ = '\t';
        }
        *p++ = '[';
        *p++ = '0';
        *p++ = ']';
        *p++ = '\0';
    }

    virtual void TearDown() {
    }

protected:
    const char* filename_;
    std::vector<char> json_;
    std::vector<char> whitespace_;

    static const size_t kTrialCount = 1000;
};

TEST_F(PerfTest, ParseTest) {
    for (size_t i = 0; i < kTrialCount; i++) {
        json_reader reader;
        EXPECT_NO_THROW(reader.parse(json_));
    }
}

TEST_F(PerfTest, WhitespaceTest) {
    for (size_t i = 0; i < kTrialCount; i++) {
        json_reader reader;
        EXPECT_NO_THROW(reader.parse(whitespace_));
    }
}
