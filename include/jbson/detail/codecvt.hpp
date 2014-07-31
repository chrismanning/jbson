//          Copyright Christian Manning 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_CODECVT_HPP
#define JBSON_CODECVT_HPP

#include <cassert>

#include "config.hpp"

#ifndef BOOST_NO_CXX11_HDR_CODECVT
#include <codecvt>
#elif BOOST_GNU_STDLIB
#include <ext/codecvt_specializations.h>
#endif

namespace jbson {
namespace detail {

#ifdef BOOST_GNU_STDLIB
using state_t = std::encoding_state;

template <typename CharT>
inline state_t create_state() {
    return state_t{};
}

template <>
inline state_t create_state<char16_t>() {
    return state_t{"UTF-16", "UTF-8"};
}

template <>
inline state_t create_state<char32_t>() {
    return state_t{"UTF-32", "UTF-8"};
}

inline bool state_test(const state_t* ps) {
    assert(ps);
    return ps->good();
}
#else
using state_t = std::mbstate_t;

inline state_t create_state() {
    return state_t{};
}

inline bool state_test(const state_t* (ps)) {
    assert(ps);
    return std::mbsinit(ps);
}
#endif

template <typename CharT> struct codecvt : std::codecvt<CharT, char, state_t> { using type = codecvt<CharT>; };

#ifndef BOOST_NO_CXX11_HDR_CODECVT
template <> struct codecvt<wchar_t> { using type = std::codecvt_utf8<wchar_t>; };
#endif // BOOST_NO_CXX11_HDR_CODECVT

template <typename CharT> using codecvt_t = typename codecvt<CharT>::type;

} // namespace detail
} // namespace jbson

#endif // JBSON_CODECVT_HPP
