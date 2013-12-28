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
#include <boost/predef/other/endian.h>

#include "traits.hpp"
#include "endian.hpp"
#include "error.hpp"
#include "detect_size.hpp"

namespace jbson {
namespace detail {

template <typename ReturnT, typename Enable = void> struct get_impl;

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

// getters
// number
template <typename ReturnT> struct get_impl<ReturnT, std::enable_if_t<std::is_arithmetic<ReturnT>::value>> {
    template <typename RangeT> static ReturnT call(const RangeT& data) {
        if(boost::distance(data) != sizeof(ReturnT))
            BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(boost::distance(data))
                                                         << expected_size(sizeof(ReturnT)));
        return detail::little_endian_to_native<ReturnT>(data.begin(), data.end());
    }
};

// string
template <typename ReturnT>
struct get_impl<ReturnT, std::enable_if_t<std::is_convertible<std::decay_t<ReturnT>, boost::string_ref>::value>> {
    template <typename RangeT> static std::decay_t<ReturnT> call(const RangeT& data) {
        auto first = data.begin(), last = data.end();
        if(std::distance(first, last) <= static_cast<ptrdiff_t>(sizeof(int32_t)))
            BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(std::distance(first, last))
                                                         << expected_size(sizeof(int32_t)));
        std::advance(first, sizeof(int32_t));
        auto length = little_endian_to_native<int32_t>(data.begin(), first) - 1;
        last = std::find(first, last, '\0');
        if(std::distance(first, last) != length)
            BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(std::distance(first, last))
                                                         << expected_size(length));
        return make_string<std::decay_t<ReturnT>>::call(first, last);
    }
};

// embedded document
template <typename ReturnT>
struct get_impl<ReturnT, typename std::enable_if<is_document<std::decay_t<ReturnT>>::value>::type> {
    template <typename RangeT> static ReturnT call(const RangeT& data) {
        return ReturnT{data};
    }
};

// binary data
template <typename ReturnT>
struct get_impl<ReturnT, std::enable_if_t<std::is_same<std::decay_t<ReturnT>, std::vector<char>>::value>> {
    template <typename RangeT> static ReturnT call(const RangeT& data) {
        ReturnT vec;
        boost::range::push_back(vec, data);
        return vec;
    }
};

// ditto
template <template <typename> class ReturnT, typename Iterator>
struct get_impl<
    ReturnT<Iterator>,
    std::enable_if_t<std::is_same<std::decay_t<ReturnT<Iterator>>, typename boost::iterator_range<Iterator>>::value>> {
    template <typename RangeT> static ReturnT<Iterator> call(const RangeT& data) { return data; }
};

// oid
template <typename ReturnT>
struct get_impl<ReturnT, std::enable_if_t<std::is_same<std::decay_t<ReturnT>, std::array<char, 12>>::value>> {
    template <typename RangeT> static ReturnT call(const RangeT& data) {
        if(boost::distance(data) != 12)
            BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(boost::distance(data)) << expected_size(12));
        ReturnT arr;
        std::copy(data.begin(), data.end(), arr.data());
        return arr;
    }
};

// date
template <typename ReturnT>
struct get_impl<
    ReturnT,
    std::enable_if_t<std::is_same<
        std::decay_t<ReturnT>, std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>>::value>> {
    template <typename RangeT> static ReturnT call(const RangeT& data) {
        if(boost::distance(data) != 8)
            BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(boost::distance(data)) << expected_size(8));
        return ReturnT{std::chrono::milliseconds{get_impl<int64_t>::call(data)}};
    }
};

template <size_t N, typename TupleT> using tuple_element_t = typename std::tuple_element<N, std::decay_t<TupleT>>::type;

// regex
template <typename TupleT>
struct get_impl<TupleT,
                std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                                 std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value&&
                                     std::is_same<tuple_element_t<0, TupleT>, tuple_element_t<1, TupleT>>::value>> {
    template <typename RangeT> static std::decay_t<TupleT> call(const RangeT& data) {
        using string_maker = make_string<std::decay_t<tuple_element_t<1, TupleT>>>;
        auto first = std::find(data.begin(), data.end(), '\0');
        auto str = string_maker::call(data.begin(), first);
        auto last = std::find(++first, data.end(), '\0');
        return std::make_tuple(str, string_maker::call(first, last));
    }
};

// db pointer
template <typename TupleT>
struct get_impl<
    TupleT, std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                             std::is_same<std::array<char, 12>, std::decay_t<tuple_element_t<1, TupleT>>>::value&&
                                 std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value>> {
    template <typename RangeT> static std::decay_t<TupleT> call(const RangeT& data) {
        auto str = get_impl<tuple_element_t<0, TupleT>>::call(data);
        auto oid = get_impl<std::array<char, 12>>::call(boost::make_iterator_range(
            std::next(data.begin(), detect_size(element_type::string_element, data.begin(), data.end())),
            data.end()));
        return std::make_tuple(str, oid);
    }
};

// scoped javascript
template <typename TupleT>
struct get_impl<
    TupleT, std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                             is_document<std::decay_t<tuple_element_t<1, TupleT>>>::value&&
                                 std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value>> {
    template <typename RangeT> static TupleT call(const RangeT& data) {
        auto str = get_impl<std::decay_t<tuple_element_t<0, TupleT>>>::call(data);
        auto doc = get_impl<std::decay_t<tuple_element_t<1, TupleT>>>::call(boost::make_iterator_range(
            std::next(data.begin(), detect_size(element_type::string_element, data.begin(), data.end())),
            data.end()));
        return std::make_tuple(str, doc);
    }
};

template <typename ReturnT> struct get_impl<ReturnT, std::enable_if_t<std::is_void<ReturnT>::value>> {
    static_assert(std::is_void<ReturnT>::value, "cannot get void");
};

} // namespace detail
} // namespace jbson

#endif // JBSON_GET_HPP
