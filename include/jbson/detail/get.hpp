//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_GET_HPP
#define JBSON_GET_HPP

#include <type_traits>
#include <array>
#include <chrono>

#include <boost/utility/string_ref.hpp>

#include "traits.hpp"
#include "endian.hpp"
#include "error.hpp"
#include "detect_size.hpp"

namespace jbson {
namespace detail {

template <typename StringT> struct make_string {
    template <typename Iterator> static StringT call(const Iterator& first, const Iterator& last) {
        return StringT{first, last};
    }
};

template <> struct make_string<boost::string_ref> {
    template <typename Iterator> static boost::string_ref call(const Iterator& first, const Iterator& last) {
        static_assert(is_iterator_pointer<Iterator>::value, "can only use string_ref for raw memory");
        return boost::string_ref{&*first, static_cast<size_t>(std::distance(first, last))};
    }
};

} // namespace detail

// number
template <typename RangeT, typename ArithT>
void deserialise(const RangeT& data, ArithT& num, std::enable_if_t<std::is_arithmetic<ArithT>::value>* = nullptr) {
    if(boost::distance(data) != sizeof(ArithT))
        BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(boost::distance(data))
                                                     << expected_size(sizeof(ArithT)));
    num = detail::little_endian_to_native<ArithT>(data.begin(), data.end());
}

// string
template <typename RangeT, typename StringT>
void deserialise(const RangeT& data, StringT& str,
                 std::enable_if_t<std::is_convertible<std::decay_t<StringT>, boost::string_ref>::value>* = nullptr) {
    auto first = data.begin(), last = data.end();
    if(std::distance(first, last) <= static_cast<ptrdiff_t>(sizeof(int32_t)))
        BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(std::distance(first, last))
                                                     << expected_size(sizeof(int32_t)));
    std::advance(first, sizeof(int32_t));
    const auto length = detail::little_endian_to_native<int32_t>(data.begin(), first) - 1;
    last = std::find(first, last, '\0');
    if(std::distance(first, last) != length)
        BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(std::distance(first, last))
                                                     << expected_size(length));

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
        BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(boost::distance(data)) << expected_size(12));
    std::copy(data.begin(), data.end(), oid.data());
}

template <size_t N, typename TupleT> using tuple_element_t = typename std::tuple_element<N, std::decay_t<TupleT>>::type;

// regex
template <typename RangeT, typename TupleT>
void deserialise(
    const RangeT& data, TupleT& tuple,
    std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                     std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value&&
                         std::is_same<tuple_element_t<0, TupleT>, tuple_element_t<1, TupleT>>::value>* = nullptr) {
    using string_maker = detail::make_string<std::decay_t<tuple_element_t<1, TupleT>>>;
    auto first = std::find(data.begin(), data.end(), '\0');
    std::get<0>(tuple) = string_maker::call(data.begin(), first);
    auto last = std::find(++first, data.end(), '\0');
    std::get<1>(tuple) = string_maker::call(first, last);
}

// db pointer
template <typename RangeT, typename TupleT>
void
deserialise(const RangeT& data, TupleT& tuple,
            std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                             std::is_same<std::array<char, 12>, std::decay_t<tuple_element_t<1, TupleT>>>::value&&
                                 std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value>* =
                nullptr) {
    deserialise(data, std::get<0>(tuple));
    deserialise(boost::make_iterator_range(std::next(data.begin(), detail::detect_size(element_type::string_element,
                                                                                       data.begin(), data.end())),
                                           data.end()),
                std::get<1>(tuple));
}

// scoped javascript
template <typename RangeT, typename TupleT>
void deserialise(
    const RangeT& data, TupleT& tuple,
    std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                     detail::is_document<std::decay_t<tuple_element_t<1, TupleT>>>::value&& std::is_constructible<
                         std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value>* = nullptr) {
    deserialise(data, std::get<0>(tuple));
    deserialise(boost::make_iterator_range(std::next(data.begin(), detail::detect_size(element_type::string_element,
                                                                                       data.begin(), data.end())),
                                           data.end()),
                std::get<1>(tuple));
}

namespace detail {

template <typename ReturnT> struct get_impl {
    static_assert(!std::is_void<ReturnT>::value, "cannot get void");
    template <typename RangeT> static ReturnT call(const RangeT& data) {
        ReturnT ret{};
        deserialise(data, ret);
        return std::move(ret);
    }
};

} // namespace detail
} // namespace jbson

#endif // JBSON_GET_HPP
