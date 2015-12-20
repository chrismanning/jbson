//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_VISIT_HPP
#define JBSON_VISIT_HPP

#include "../element_fwd.hpp"

JBSON_PUSH_DISABLE_DEPRECATED_WARNING

namespace jbson {
namespace detail {

/*!
 * \brief void visit.
 *
 * \throw invalid_element_type When \p type is invalid.
 */
template <template <element_type EType, typename... VArgs> class Visitor, typename... Args>
std::enable_if_t<std::is_void<
    decltype(std::declval<Visitor<element_type::int64_element, Args...>>()(std::declval<Args&&>()...))>::value>
visit(element_type type, Args&&... args) {
    switch(type) {
        case element_type::double_element:
            Visitor<element_type::double_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::string_element:
            Visitor<element_type::string_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::document_element:
            Visitor<element_type::document_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::array_element:
            Visitor<element_type::array_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::binary_element:
            //            Visitor<element_type::binary_element, Args...> {}
            //            (std::forward<Args>(args)...);
            return;
        case element_type::undefined_element:
            Visitor<element_type::undefined_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::oid_element:
            Visitor<element_type::oid_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::boolean_element:
            Visitor<element_type::boolean_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::date_element:
            Visitor<element_type::date_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::null_element:
            Visitor<element_type::null_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::regex_element:
            Visitor<element_type::regex_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::db_pointer_element:
            Visitor<element_type::db_pointer_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::javascript_element:
            Visitor<element_type::javascript_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::symbol_element:
            Visitor<element_type::symbol_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::scoped_javascript_element:
            Visitor<element_type::scoped_javascript_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::int32_element:
            Visitor<element_type::int32_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::timestamp_element:
            Visitor<element_type::timestamp_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::int64_element:
            Visitor<element_type::int64_element, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::min_key:
            Visitor<element_type::min_key, Args...>{}(std::forward<Args>(args)...);
            return;
        case element_type::max_key:
            Visitor<element_type::max_key, Args...>{}(std::forward<Args>(args)...);
            return;
        default:
            BOOST_THROW_EXCEPTION(invalid_element_type{});
    };
}

/*!
 * \brief Non-void visit.
 *
 * \throw invalid_element_type When \p type is invalid.
 */
template <template <element_type EType, typename... VArgs> class Visitor, typename... Args>
std::enable_if_t<!std::is_void<decltype(
                     std::declval<Visitor<element_type::int64_element, Args...>>()(std::declval<Args&&>()...))>::value,
                 decltype(std::declval<Visitor<element_type::int64_element, Args...>>()(std::declval<Args&&>()...))>
visit(element_type type, Args&&... args) {
    switch(type) {
        case element_type::double_element:
            return Visitor<element_type::double_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::string_element:
            return Visitor<element_type::string_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::document_element:
            return Visitor<element_type::document_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::array_element:
            return Visitor<element_type::array_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::binary_element:
        //            return Visitor<element_type::binary_element, Args...> {}
        //            (std::forward<Args>(args)...);
        case element_type::undefined_element:
            return Visitor<element_type::undefined_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::oid_element:
            return Visitor<element_type::oid_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::boolean_element:
            return Visitor<element_type::boolean_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::date_element:
            return Visitor<element_type::date_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::null_element:
            return Visitor<element_type::null_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::regex_element:
            return Visitor<element_type::regex_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::db_pointer_element:
            return Visitor<element_type::db_pointer_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::javascript_element:
            return Visitor<element_type::javascript_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::symbol_element:
            return Visitor<element_type::symbol_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::scoped_javascript_element:
            return Visitor<element_type::scoped_javascript_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::int32_element:
            return Visitor<element_type::int32_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::timestamp_element:
            return Visitor<element_type::timestamp_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::int64_element:
            return Visitor<element_type::int64_element, Args...>{}(std::forward<Args>(args)...);
        case element_type::min_key:
            return Visitor<element_type::min_key, Args...>{}(std::forward<Args>(args)...);
        case element_type::max_key:
            return Visitor<element_type::max_key, Args...>{}(std::forward<Args>(args)...);
        default:
            BOOST_THROW_EXCEPTION(invalid_element_type{}
                                  << detail::actual_type(typeid(Visitor<element_type::min_key, Args...>)));
    };
}

} // namespace detail
} // namespace jbson

JBSON_POP_WARNINGS
#endif // JBSON_VISIT_HPP
