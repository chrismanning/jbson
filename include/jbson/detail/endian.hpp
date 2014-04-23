//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_ENDIAN_HPP
#define JBSON_ENDIAN_HPP

#include <type_traits>

#include "./config.hpp"

JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
#include <boost/predef/other/endian.h>
JBSON_CLANG_POP_WARNINGS

namespace jbson {
namespace detail {

// endian shit
template <typename T, typename ForwardIterator> T little_endian_to_native(ForwardIterator first, ForwardIterator last) {
    static_assert(std::is_pod<T>::value, "Can only byte swap POD types");
    assert(std::distance(first, last) >= static_cast<ptrdiff_t>(sizeof(T)));
    last = std::next(first, sizeof(T));
    assert(std::distance(first, last) == static_cast<ptrdiff_t>(sizeof(T)));
    union {
        T u;
        unsigned char u8[sizeof(T)];
    } source;

    std::copy(first, last, source.u8);

#if BOOST_ENDIAN_BIG_BYTE
#warning "big endian untested"
    std::reverse(std::begin(source.u8), std::end(source.u8));
#elif !BOOST_ENDIAN_LITTLE_BYTE
#error "unsupported endianness"
#endif
    return source.u;
}

template <typename T> std::array<char, sizeof(T)> native_to_little_endian(T val) {
    using T2 = std::decay_t<T>;
    static_assert(std::is_pod<T2>::value, "Can only byte swap POD types");

    union {
        T2 u;
        std::array<char, sizeof(T2)> u8;
    } source;

    source.u = val;

#if BOOST_ENDIAN_BIG_BYTE
#warning "big endian untested"
    std::reverse(std::begin(source.u8), std::end(source.u8));
#elif !BOOST_ENDIAN_LITTLE_BYTE
#error "unsupported endianness"
#endif
    return source.u8;
}

} // namespace detail
} // namespace jbson

#endif // JBSON_ENDIAN_HPP
