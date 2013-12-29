//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_SET_HPP
#define JBSON_SET_HPP

#include "../element_fwd.hpp"

namespace jbson {
namespace detail {

// setters
template <element_type EType, typename T, typename Enable = void> struct set_impl;

// arithmetic
template <element_type EType, typename T>
struct set_impl<EType, T, std::enable_if_t<std::is_arithmetic<std::decay_t<T>>::value>> {
    template <typename Container> static void call(Container& data, T val) {
        boost::range::push_back(data,
                                detail::native_to_little_endian(static_cast<ElementTypeMap<EType, Container>>(val)));
    }
};

// string
template <element_type EType, typename T>
struct set_impl<EType, T, std::enable_if_t<std::is_constructible<boost::string_ref, std::decay_t<T>>::value &&
                                           !is_document<typename std::decay<T>::type>::value>> {
    template <typename Container> static void call(Container& data, boost::string_ref val) {
        auto old_size = data.size();
        boost::range::push_back(data, detail::native_to_little_endian(static_cast<int32_t>(val.size() + 1)));
        boost::range::push_back(data, val);
        data.push_back('\0');
        if(data.size() - old_size != val.size() + sizeof(int32_t) + sizeof('\0'))
            BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(data.size() - old_size)
                                                         << expected_size(val.size() + sizeof(int32_t) + sizeof('\0')));
    }
};

// date
template <element_type EType, typename DateT>
struct set_impl<EType, DateT,
                std::enable_if_t<std::is_arithmetic<typename std::decay_t<DateT>::rep>::value&&
                                     std::is_arithmetic<typename std::decay_t<DateT>::clock::rep>::value>> {
    template <typename Container> static void call(Container& data, DateT val) {
        auto old_size = data.size();
        boost::range::push_back(data,
                                detail::native_to_little_endian(static_cast<int64_t>(val.time_since_epoch().count())));
        if(data.size() - old_size != sizeof(int64_t))
            BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(data.size() - old_size)
                                                         << expected_size(sizeof(int64_t)));
    }
};

// oid
template <element_type EType, typename T>
struct set_impl<EType, T, std::enable_if_t<std::is_same<std::decay_t<T>, std::array<char, 12>>::value>> {
    template <typename Container> static void call(Container& data, T val) {
        auto old_size = data.size();
        boost::range::push_back(data, val);
        if(data.size() - old_size != val.size())
            BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(data.size() - old_size)
                                                         << expected_size(val.size()));
    }
};

// regex
template <element_type EType, typename TupleT>
struct set_impl<EType, TupleT,
                std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                                 std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value&&
                                     std::is_same<tuple_element_t<0, TupleT>, tuple_element_t<1, TupleT>>::value>> {
    template <typename Container> static void call(Container& data, TupleT&& val) {
        boost::push_back(data, std::get<0>(val));
        data.push_back('\0');
        boost::push_back(data, std::get<1>(val));
        data.push_back('\0');
    }
};

// db_pointer
template <element_type EType, class TupleT>
struct set_impl<
    EType, TupleT,
    std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                     std::is_same<std::array<char, 12>, std::decay_t<tuple_element_t<1, TupleT>>>::value&&
                         std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value>> {
    template <typename Container> static void call(Container& data, TupleT&& val) {
        set_impl<element_type::string_element, std::decay_t<tuple_element_t<0, TupleT>>>::call(data, std::get<0>(val));
        set_impl<element_type::oid_element, std::decay_t<tuple_element_t<1, TupleT>>>::call(data, std::get<1>(val));
    }
};

// scoped javascript
template <element_type EType, class TupleT>
struct set_impl<EType, TupleT,
                std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                                 is_document<std::decay_t<tuple_element_t<1, TupleT>>>::value&& std::is_constructible<
                                     std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value>> {
    template <typename Container> static void call(Container&, TupleT&&) {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
    }
};

// binary
template <typename T> struct set_impl<element_type::binary_element, T> {
    template <typename Container> static void call(Container&, T&&) {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
    }
};

// void
template <element_type EType, typename T> struct set_impl<EType, T, std::enable_if_t<std::is_void<T>::value>> {
    static_assert(std::is_void<T>::value, "cannot set void");
};

// set visitor
template <element_type EType, typename C, typename A = void, typename Enable = void> struct set_visitor;

// voids
template <element_type EType, typename C, typename A>
struct set_visitor<EType, C, A, std::enable_if_t<size_func<EType, void*>::value == 0>> {
    template <typename... Args> void operator()(Args&&...) const {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
    }
};

// doc special handling
template <element_type EType, typename C, typename A>
struct set_visitor<EType, C, A,
                   std::enable_if_t<EType == element_type::document_element&& has_member_function_push_back<
                                                 std::decay_t<C>, void, boost::mpl::vector<const char&>>::value>> {
    using Container = std::decay_t<C>;
    template <typename T>
    void operator()(Container& data, T&& val,
                    std::enable_if_t<std::is_constructible<basic_document<Container, Container>, T>::value>* =
                        nullptr) const {
        data = basic_document<Container, Container>(std::forward<T>(val)).data();
    }
    template <typename... Args> void operator()(Args&&...) const {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
    }
};

// array special handling
template <element_type EType, typename C, typename A>
struct set_visitor<EType, C, A,
                   std::enable_if_t<EType == element_type::array_element&& has_member_function_push_back<
                                                 std::decay_t<C>, void, boost::mpl::vector<const char&>>::value>> {
    using Container = std::decay_t<C>;
    template <typename T>
    void operator()(Container& data, T&& val,
                    std::enable_if_t<std::is_constructible<basic_array<Container, Container>, T>::value>* =
                        nullptr) const {
        data = basic_array<Container, Container>(std::forward<T>(val)).data();
    }
    template <typename... Args> void operator()(Args&&...) const {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
    }
};

// everything else
template <element_type EType, typename Container, typename A>
struct set_visitor<EType, Container, A,
                   std::enable_if_t<(EType != element_type::array_element&& EType != element_type::document_element) &&
                                    !std::is_convertible<size_func<EType, void*>, int>::value>> {
    using set_type = ElementTypeMap<EType, std::decay_t<Container>>;
    void operator()(std::decay_t<Container>& data, set_type val) const {
        set_impl<EType, set_type>::call(data, std::move(val));
    }
    template <typename T>
    void operator()(std::decay_t<Container>& data, T&& val,
                    std::enable_if_t<std::is_constructible<set_type, T>::value>* = nullptr) const {
        set_impl<EType, set_type>::call(data, set_type(std::forward<T>(val)));
    }
    template <typename T>
    void operator()(std::decay_t<Container>&, T&&,
                    std::enable_if_t<!std::is_constructible<set_type, T>::value>* = nullptr,
                    std::enable_if_t<!std::is_convertible<std::decay_t<T>, set_type>::value>* = nullptr) const {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
    }
};

} // namespace detail
} // namespace jbson

#endif // JBSON_SET_HPP
