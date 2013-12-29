//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_DETECT_SIZE_HPP
#define JBSON_DETECT_SIZE_HPP

#include "../element_fwd.hpp"
#include "error.hpp"
#include "traits.hpp"
#include "visit.hpp"

namespace jbson {
namespace detail {

template <element_type EType, typename ForwardIterator, typename = ForwardIterator> struct size_func {
    size_t operator()(ForwardIterator, ForwardIterator) const {
        return sizeof(detail::ElementTypeMap<EType, std::string>);
    }
};

template <typename ForwardIterator>
struct size_func<element_type::null_element, ForwardIterator> : std::integral_constant<int, 0> {
    template <typename... Args> constexpr size_t operator()(Args&&...) const { return 0; }
};

template <typename ForwardIterator>
struct size_func<element_type::min_key, ForwardIterator> : std::integral_constant<int, 0> {
    template <typename... Args> constexpr size_t operator()(Args&&...) const { return 0; }
};

template <typename ForwardIterator>
struct size_func<element_type::max_key, ForwardIterator> : std::integral_constant<int, 0> {
    template <typename... Args> constexpr size_t operator()(Args&&...) const { return 0; }
};

template <typename ForwardIterator>
struct size_func<element_type::undefined_element, ForwardIterator> : std::integral_constant<int, 0> {
    template <typename... Args> constexpr size_t operator()(Args&&...) const { return 0; }
};

template <typename ForwardIterator> struct size_func<element_type::string_element, ForwardIterator> {
    size_t operator()(ForwardIterator first, ForwardIterator last) const {
        if(static_cast<ptrdiff_t>(sizeof(int32_t)) > std::distance(first, last))
            BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(std::distance(first, last))
                                                         << expected_size(sizeof(int32_t)));
        return sizeof(int32_t) + detail::little_endian_to_native<int32_t>(first, last);
    }
};

template <typename ForwardIterator>
struct size_func<element_type::javascript_element, ForwardIterator> : size_func<element_type::string_element,
                                                                                ForwardIterator> {};

template <typename ForwardIterator>
struct size_func<element_type::symbol_element, ForwardIterator> : size_func<element_type::string_element,
                                                                            ForwardIterator> {};

template <typename ForwardIterator>
struct size_func<element_type::binary_element, ForwardIterator> : private size_func<element_type::string_element,
                                                                                    ForwardIterator> {
    size_t operator()(ForwardIterator first, ForwardIterator last) const {
        return size_func<element_type::string_element, ForwardIterator>::operator()(first, last) + 1;
    }
};

template <typename ForwardIterator> struct size_func<element_type::document_element, ForwardIterator> {
    size_t operator()(ForwardIterator first, ForwardIterator last) const {
        if(static_cast<ptrdiff_t>(sizeof(int32_t)) > std::distance(first, last))
            BOOST_THROW_EXCEPTION(invalid_element_size{} << actual_size(std::distance(first, last))
                                                         << expected_size(sizeof(int32_t)));
        return detail::little_endian_to_native<int32_t>(first, last);
    }
};

template <typename ForwardIterator>
struct size_func<element_type::array_element, ForwardIterator> : size_func<element_type::document_element,
                                                                           ForwardIterator> {};

template <typename ForwardIterator>
struct size_func<element_type::scoped_javascript_element, ForwardIterator> : size_func<element_type::document_element,
                                                                                       ForwardIterator> {};

template <typename ForwardIterator>
struct size_func<element_type::db_pointer_element,
                 ForwardIterator> : private size_func<element_type::oid_element, ForwardIterator>,
                                    private size_func<element_type::string_element, ForwardIterator> {
    size_t operator()(ForwardIterator first, ForwardIterator last) const {
        return size_func<element_type::string_element, ForwardIterator>::operator()(first, last) +
               size_func<element_type::oid_element, ForwardIterator>::operator()(first, last);
    }
};

template <typename ForwardIterator> struct size_func<element_type::regex_element, ForwardIterator> {
    size_t operator()(ForwardIterator first, ForwardIterator last) const {
        auto it = std::next(std::find(first, last, '\0'));
        it = std::next(std::find(it, last, '\0'));
        return std::distance(first, it);
    }
};

template <typename ForwardIterator>
ptrdiff_t detect_size(element_type e, ForwardIterator first, ForwardIterator last) {
    return detail::visit<size_func>(e, first, last);
}

} // namespace detail
} // namespace jbson

#endif // JBSON_DETECT_SIZE_HPP
