//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_SET_HPP
#define JBSON_SET_HPP

#include "../element_fwd.hpp"
#include "traits.hpp"

namespace jbson {

template <typename Container> void value_set(basic_element<Container>&, ...) {
    static_assert(std::is_void<Container>::value,
                  "A valid overload of value_set must be supplied for user-defined types.");
}

namespace detail {

// setters

// arithmetic
template <typename Container, typename IteratorT, typename T>
void serialise(Container& c, IteratorT& it, T val, std::enable_if_t<std::is_arithmetic<T>::value>* = nullptr) {
    auto data = detail::native_to_little_endian(val);
    it = c.insert(it, std::begin(data), std::end(data));
    if(it != std::end(c))
        std::advance(it, boost::distance(data));
}

// string
template <typename Container, typename IteratorT>
void serialise(Container& c, IteratorT& it, std::experimental::string_view val) {
    serialise(c, it, static_cast<int32_t>(val.size() + 1));
    it = c.insert(it, std::begin(val), std::end(val));
    if(it != std::end(c))
        std::advance(it, boost::distance(val));
    it = std::next(c.insert(it, '\0'));
}

// document
template <typename Container, typename IteratorT, typename DocContainer, typename DocEContainer>
void serialise(Container& c, IteratorT& it, const basic_document<DocContainer, DocEContainer>& val) {
    if(c.empty()) {
        c = Container{val.data().begin(), val.data().end()};
        it = c.end();
        return;
    }
    it = c.insert(it, std::begin(val.data()), std::end(val.data()));
    if(it != std::end(c))
        std::advance(it, boost::distance(val.data()));
}

// array
template <typename Container, typename IteratorT, typename DocContainer, typename DocEContainer>
void serialise(Container& c, IteratorT& it, const basic_array<DocContainer, DocEContainer>& val) {
    if(c.empty()) {
        c = Container{val.data().begin(), val.data().end()};
        it = c.end();
        return;
    }
    it = c.insert(it, std::begin(val.data()), std::end(val.data()));
    if(it != std::end(c))
        std::advance(it, boost::distance(val.data()));
}

// oid
template <typename Container, typename IteratorT>
void serialise(Container& c, IteratorT& it, const std::array<char, 12>& val) {
    it = c.insert(it, std::begin(val), std::end(val));
    if(it != std::end(c))
        std::advance(it, 12);
}

// regex
template <typename Container, typename IteratorT, typename StringT>
void serialise(Container& c, IteratorT& it, const std::tuple<StringT, StringT>& val,
               std::enable_if_t<std::is_convertible<StringT, std::experimental::string_view>::value>* = nullptr) {
    std::experimental::string_view str1 = std::get<0>(val);
    it = c.insert(it, std::begin(str1), std::end(str1));
    if(it != std::end(c))
        std::advance(it, boost::distance(str1));
    it = std::next(c.insert(it, '\0'));

    std::experimental::string_view str2 = std::get<1>(val);
    it = c.insert(it, std::begin(str2), std::end(str2));
    if(it != std::end(c))
        std::advance(it, boost::distance(str2));
    it = std::next(c.insert(it, '\0'));
}

// db_pointer
template <typename Container, typename IteratorT, typename StringT>
void serialise(Container& c, IteratorT& it, const std::tuple<StringT, std::array<char, 12>>& val) {
    serialise(c, it, std::get<0>(val));
    serialise(c, it, std::get<1>(val));
}

// scoped javascript
template <typename Container, typename IteratorT, typename StringT, typename DocContainer, typename DocEContainer>
void serialise(Container&, IteratorT&, const std::tuple<StringT, basic_document<DocContainer, DocEContainer>>&,
               std::enable_if_t<std::is_convertible<StringT, std::experimental::string_view>::value>* = nullptr) {
    assert(false);
    BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
}

// set visitor
template <element_type EType, typename C, typename It, typename A, typename Enable = void> struct set_visitor;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// voids
template <element_type EType, typename C, typename It, typename A>
struct set_visitor<EType, C, It, A, std::enable_if_t<std::is_void<ElementTypeMapSet<EType, std::decay_t<C>>>::value>> {
    template <typename... Args> void operator()(Args&&...) const {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
    }
};

// everything else
template <element_type EType, typename Container, typename IteratorT, typename A>
struct set_visitor<EType, Container, IteratorT, A,
                   std::enable_if_t<!std::is_void<ElementTypeMapSet<EType, std::decay_t<Container>>>::value>> {
    static_assert(detail::container_has_push_back<Container>::value,
                  "Cannot set value of an element without a modifiable container");

    using container_type = std::decay_t<Container>;
    using set_type = ElementTypeMapSet<EType, container_type>;

    template <typename T> void operator()(container_type& data, T&& val) const {
        (*this)(data, std::end(data), std::forward<T>(val));
    }

    void operator()(container_type& data, typename container_type::const_iterator it, set_type val) const {
        serialise(data, it, std::move(val));
    }

    template <typename T>
    void
    operator()(container_type& data, typename container_type::const_iterator it, T&& val,
               std::enable_if_t<std::is_constructible<set_type, T>::value || std::is_convertible<T, set_type>::value>* =
                   nullptr) const {
        serialise(data, it, set_type(std::forward<T>(val)));
    }

    template <typename T>
    void operator()(container_type&, typename container_type::const_iterator const&, T&&,
                    std::enable_if_t<!std::is_constructible<set_type, T>::value>* = nullptr,
                    std::enable_if_t<!std::is_convertible<std::decay_t<T>, set_type>::value>* = nullptr) const {
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{} << expected_type(typeid(set_type))
                                                             << actual_type(typeid(T)));
    }
};

#endif // DOXYGEN_SHOULD_SKIP_THIS

} // namespace detail
} // namespace jbson

#endif // JBSON_SET_HPP
