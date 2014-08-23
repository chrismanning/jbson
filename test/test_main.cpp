//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "gtest/gtest.h"

GTEST_API_ int main(int argc, char **argv) {
    std::locale loc{"en_GB.UTF-8"};
    std::locale::global(loc.combine<std::numpunct<char>>(std::locale("C")));
    loc = std::locale{};
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
