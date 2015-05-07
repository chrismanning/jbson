//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_GET_HPP
#define JBSON_GET_HPP

#include <type_traits>
#include <array>
#include <chrono>

#include "./config.hpp"

JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
//
JBSON_CLANG_POP_WARNINGS

#include "traits.hpp"
#include "endian.hpp"
#include "error.hpp"
#include "detect_size.hpp"

namespace jbson {

template <typename Container>
void value_get(const basic_element<Container>&, ...) {
    static_assert(std::is_void<Container>::value,
                  "A valid overload of value_get must be supplied for user-defined types.");
}

namespace detail {

template <typename StringT> struct make_string {
    static_assert(!std::is_same<std::decay_t<StringT>, const char*>::value, "");
    static_assert(!std::is_same<std::decay_t<StringT>, char*>::value, "");
    template <typename Iterator> static StringT call(const Iterator& first, const Iterator& last) {
        return StringT{first, last};
    }
};

template <> struct make_string<std::string_view> {
    template <typename Iterator> static std::string_view call(const Iterator& first, const Iterator& last) {
        static_assert(is_iterator_pointer<Iterator>::value, "can only use string_ref for raw memory");
        return std::string_view{&*first, static_cast<size_t>(std::distance(first, last))};
    }
};

// number
template <typename RangeT, typename ArithT>
void deserialise(const RangeT& data, ArithT& num, std::enable_if_t<std::is_arithmetic<ArithT>::value>* = nullptr) {
    if(boost::distance(data) != sizeof(ArithT))
        BOOST_THROW_EXCEPTION(invalid_element_size{} << detail::actual_size(boost::distance(data))
                                                     << detail::expected_size(sizeof(ArithT)));
    num = detail::little_endian_to_native<ArithT>(data.begin(), data.end());
}

// string
template <typename RangeT, typename StringT>
void deserialise(const RangeT& data, StringT& str,
                 std::enable_if_t<std::is_convertible<std::decay_t<StringT>, std::string_view>::value>* = nullptr) {
    auto first = data.begin(), last = data.end();
    if(std::distance(first, last) <= static_cast<ptrdiff_t>(sizeof(int32_t)))
        BOOST_THROW_EXCEPTION(invalid_element_size{} << detail::actual_size(std::distance(first, last))
                                                     << detail::expected_size(sizeof(int32_t)));
    std::advance(first, sizeof(int32_t));
    const auto length = detail::little_endian_to_native<int32_t>(data.begin(), first) - 1;
    if(length < 0)
        BOOST_THROW_EXCEPTION(invalid_element_size{} << detail::actual_size(length));
    last = std::find(first, last, '\0');
    if(std::distance(first, last) != length)
        BOOST_THROW_EXCEPTION(invalid_element_size{} << detail::actual_size(std::distance(first, last))
                                                     << detail::expected_size(length));

    str = detail::make_string<StringT>::call(first, last);
}

// embedded document
template <typename RangeT, typename Container, typename EContainer>
void deserialise(const RangeT& data, basic_document<Container, EContainer>& doc) {
    doc = basic_document<Container, EContainer>{data};
}

template <typename RangeT, typename Container, typename EContainer>
void deserialise(const RangeT& data, basic_array<Container, EContainer>& arr) {
    arr = basic_array<Container, EContainer>{data};
}

// binary data
template <typename RangeT> void deserialise(const RangeT& data, std::vector<char>& vec) {
    boost::range::push_back(vec, data);
}

// ditto
template <typename RangeT> void deserialise(const RangeT& data, RangeT& vec) { vec = data; }

// oid
template <typename RangeT> void deserialise(const RangeT& data, std::array<char, 12>& oid) {
    if(boost::distance(data) != 12)
        BOOST_THROW_EXCEPTION(invalid_element_size{} << detail::actual_size(boost::distance(data))
                                                     << detail::expected_size(12));
    std::copy(data.begin(), data.end(), oid.data());
}

// regex
template <typename RangeT, typename StringT>
void deserialise(const RangeT& data, std::tuple<StringT, StringT>& tuple,
                 std::enable_if_t<std::is_constructible<std::string, std::decay_t<StringT>>::value>* = nullptr) {
    using string_maker = detail::make_string<std::decay_t<StringT>>;

    auto first = std::find(data.begin(), data.end(), '\0');
    if(first == data.end())
        BOOST_THROW_EXCEPTION(invalid_element_size{} << detail::actual_size(boost::distance(data)));
    std::get<0>(tuple) = string_maker::call(data.begin(), first);

    auto last = std::find(++first, data.end(), '\0');
    if(last == data.end())
        BOOST_THROW_EXCEPTION(invalid_element_size{} << detail::actual_size(boost::distance(data)));
    std::get<1>(tuple) = string_maker::call(first, last);
}

// db pointer
template <typename RangeT, typename StringT>
void deserialise(const RangeT& data, std::tuple<StringT, std::array<char, 12>>& tuple,
                 std::enable_if_t<std::is_constructible<std::string, std::decay_t<StringT>>::value>* = nullptr) {
    deserialise(data, std::get<0>(tuple));
    deserialise(boost::make_iterator_range(std::next(data.begin(), detail::detect_size(element_type::string_element,
                                                                                       data.begin(), data.end())),
                                           data.end()),
                std::get<1>(tuple));
}

// scoped javascript
template <typename RangeT, typename StringT, typename DocContainerT, typename DocEContainerT>
void deserialise(const RangeT& data, std::tuple<StringT, basic_document<DocContainerT, DocEContainerT>>& tuple,
                 std::enable_if_t<std::is_constructible<std::string, std::decay_t<StringT>>::value>* = nullptr) {
    int32_t length;
    auto it = data.begin();
    deserialise(boost::make_iterator_range(it, std::next(it, 4)), length);
    if(length != boost::distance(data))
        BOOST_THROW_EXCEPTION(invalid_element_size{} << detail::actual_size(boost::distance(data))
                                                     << detail::expected_size(length));
    std::advance(it, 4);
    deserialise(boost::make_iterator_range(it, data.end()), std::get<0>(tuple));
    deserialise(boost::make_iterator_range(
                    std::next(it, detail::detect_size(element_type::string_element, it, data.end())), data.end()),
                std::get<1>(tuple));
}

} // namespace detail

template <typename Container>
void value_get(const basic_element<Container>& elem, std::string& str) {
    str = std::string(elem.template value<detail::ElementTypeMap<element_type::string_element, Container>>());
}

} // namespace jbson

#endif // JBSON_GET_HPP
