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
template <element_type EType, typename Container, typename T, typename Enable = void> struct set_impl;

// arithmetic
template <element_type EType, typename Container, typename T>
struct set_impl<EType, Container, T, std::enable_if_t<std::is_arithmetic<std::decay_t<T>>::value>> {
    template <typename OutIterator> static OutIterator call(OutIterator out, T val) {
        BOOST_CONCEPT_ASSERT((boost::OutputIterator<OutIterator, char>));
        return boost::range::copy(detail::native_to_little_endian(static_cast<ElementTypeMap<EType, Container>>(val)),
                                  out);
    }
};

// string
template <element_type EType, typename Container, typename T>
struct set_impl<EType, Container, T,
                std::enable_if_t<std::is_constructible<boost::string_ref, std::decay_t<T>>::value &&
                                 !is_document<std::decay_t<T>>::value>> {
    template <typename OutIterator> static OutIterator call(OutIterator out, boost::string_ref val) {
        BOOST_CONCEPT_ASSERT((boost::OutputIterator<OutIterator, char>));
        out = boost::range::copy(detail::native_to_little_endian(static_cast<int32_t>(val.size() + 1)), out);
        out = boost::range::copy(val, out);
        *out++ = '\0';
        return out;
    }
};

// oid
template <element_type EType, typename Container, typename T>
struct set_impl<EType, Container, T, std::enable_if_t<std::is_same<std::decay_t<T>, std::array<char, 12>>::value>> {
    template <typename OutIterator> static OutIterator call(OutIterator out, T val) {
        BOOST_CONCEPT_ASSERT((boost::OutputIterator<OutIterator, char>));
        return boost::range::copy(val, out);
    }
};

// regex
template <element_type EType, typename Container, typename TupleT>
struct set_impl<EType, Container, TupleT,
                std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                                 std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value&&
                                     std::is_same<tuple_element_t<0, TupleT>, tuple_element_t<1, TupleT>>::value>> {
    template <typename OutIterator> static OutIterator call(OutIterator out, TupleT&& val) {
        BOOST_CONCEPT_ASSERT((boost::OutputIterator<OutIterator, char>));
        out = boost::copy(std::get<0>(val), out);
        *out++ = '\0';
        out = boost::copy(std::get<1>(val), out);
        *out++ = '\0';
        return out;
    }
};

// db_pointer
template <element_type EType, typename Container, class TupleT>
struct set_impl<
    EType, Container, TupleT,
    std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                     std::is_same<std::array<char, 12>, std::decay_t<tuple_element_t<1, TupleT>>>::value&&
                         std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value>> {
    template <typename OutIterator> static OutIterator call(OutIterator out, TupleT&& val) {
        BOOST_CONCEPT_ASSERT((boost::OutputIterator<OutIterator, char>));
        out = set_impl<element_type::string_element, Container, std::decay_t<tuple_element_t<0, TupleT>>>::call(
            out, std::get<0>(val));
        out = set_impl<element_type::oid_element, Container, std::decay_t<tuple_element_t<1, TupleT>>>::call(
            out, std::get<1>(val));
        return out;
    }
};

// scoped javascript
template <element_type EType, typename Container, class TupleT>
struct set_impl<EType, Container, TupleT,
                std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                                 is_document<std::decay_t<tuple_element_t<1, TupleT>>>::value&& std::is_constructible<
                                     std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value>> {
    template <typename OutIterator> static void call(OutIterator&&, TupleT&&) {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
    }
};

// binary
template <typename Container, typename T> struct set_impl<element_type::binary_element, Container, T> {
    template <typename OutIterator> static void call(OutIterator&&, T&&) {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
    }
};

// void
template <typename Container, element_type EType, typename T>
struct set_impl<EType, Container, T, std::enable_if_t<std::is_void<T>::value>> {
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
struct set_visitor<EType, C, A, std::enable_if_t<EType == element_type::document_element>> {
    using Container = std::decay_t<C>;
    template <typename T>
    void operator()(Container& data, T&& val,
                    std::enable_if_t<std::is_constructible<basic_document<Container, Container>, T>::value>* = nullptr)
        const {
        data = basic_document<Container, Container>(std::forward<T>(val)).data();
    }
    template <typename... Args> void operator()(Args&&...) const {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
    }
};

// array special handling
template <element_type EType, typename C, typename A>
struct set_visitor<EType, C, A, std::enable_if_t<EType == element_type::array_element>> {
    using Container = std::decay_t<C>;
    template <typename T>
    void
    operator()(Container& data, T&& val,
               std::enable_if_t<std::is_constructible<basic_array<Container, Container>, T>::value>* = nullptr) const {
        data = basic_array<Container, Container>(std::forward<T>(val)).data();
    }
    template <typename... Args> void operator()(Args&&...) const {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
    }
};

// everything else
template <element_type EType, typename Container, typename A>
struct set_visitor<
    EType, Container, A,
    std::enable_if_t<detail::has_member_function_push_back<std::remove_reference_t<Container>, void,
                                                           boost::mpl::vector<const char&>>::
                         value&&(EType != element_type::array_element&& EType != element_type::document_element) &&
                     !std::is_convertible<size_func<EType, void*>, int>::value>> {

    using set_type = ElementTypeMap<EType, std::decay_t<Container>>;

    void operator()(std::decay_t<Container>& data, set_type val) const {
        set_impl<EType, std::decay_t<Container>, set_type>::call(std::back_inserter(data), std::move(val));
    }

    template <typename T>
    void operator()(std::decay_t<Container>& data, T&& val,
                    std::enable_if_t<std::is_constructible<set_type, T>::value>* = nullptr) const {
        set_impl<EType, std::decay_t<Container>, set_type>::call(std::back_inserter(data),
                                                                 set_type(std::forward<T>(val)));
    }

    template <typename T>
    void operator()(std::decay_t<Container>&, T&&,
                    std::enable_if_t<!std::is_constructible<set_type, T>::value>* = nullptr,
                    std::enable_if_t<!std::is_convertible<std::decay_t<T>, set_type>::value>* = nullptr) const {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
    }
};

template <element_type EType, typename Container, typename A>
struct set_visitor<EType, Container, A,
                   std::enable_if_t<!detail::has_member_function_push_back<std::remove_reference_t<Container>, void,
                                                                           boost::mpl::vector<const char&>>::value &&
                                    (EType != element_type::array_element && EType != element_type::document_element) &&
                                    !std::is_convertible<size_func<EType, void*>, int>::value>> {
    static_assert(detail::has_member_function_push_back<std::remove_reference_t<Container>, void,
                                    boost::mpl::vector<const char&>>::value,
                  "Cannot set value of an element without a modifiable container");
};

} // namespace detail
} // namespace jbson

#endif // JBSON_SET_HPP
