//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_ELEMENT_FWD_HPP
#define JBSON_ELEMENT_FWD_HPP

#include <vector>
#include <set>

namespace jbson {

template <class Container> struct basic_element;
using element = basic_element<std::vector<char>>;

enum class element_type : uint8_t {
    double_element = 0x01,
    string_element = 0x02,
    document_element = 0x03,
    array_element = 0x04,
    binary_element = 0x05,
    undefined_element = 0x06,
    oid_element = 0x07,
    boolean_element = 0x08,
    date_element = 0x09,
    null_element = 0x0A,
    regex_element = 0x0B,
    db_pointer_element = 0x0C,
    javascript_element = 0x0D,
    symbol_element = 0x0E,
    scoped_javascript_element = 0x0F,
    int32_element = 0x10,
    timestamp_element = 0x11,
    int64_element = 0x12,
    min_key = 0xFF,
    max_key = 0x7F
};

namespace detail {

struct elem_string_compare;

template <typename> struct is_element : std::false_type {};
template <typename Container> struct is_element<basic_element<Container>> : std::true_type {};

} // namespace detail

template <typename Container>
using basic_document_set = std::multiset<basic_element<Container>, detail::elem_string_compare>;

using document_set = basic_document_set<std::vector<char>>;

struct jbson_error;
struct invalid_element_type;
struct incompatible_element_conversion;
struct incompatible_type_conversion;
struct invalid_element_size;

} // namespace jbson

#endif // JBSON_ELEMENT_FWD_HPP
