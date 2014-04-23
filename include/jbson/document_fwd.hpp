//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_DOCUMENT_FWD_HPP
#define JBSON_DOCUMENT_FWD_HPP

#include <vector>

#include "detail/config.hpp"

JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
#include <boost/range/iterator_range.hpp>
JBSON_CLANG_POP_WARNINGS

namespace jbson {

/*!
 * \brief BSON document
 */
template <class Container, class ElementContainer = boost::iterator_range<typename Container::const_iterator>>
class basic_document;
/*!
 * \brief BSON array
 */
template <class Container, class ElementContainer = boost::iterator_range<typename Container::const_iterator>>
class basic_array;

/*!
 * \brief Default basic_document type alias for owned BSON data
 */
using document = basic_document<std::vector<char>>;
/*!
 * \brief Default basic_array type alias for owned BSON data
 */
using array = basic_array<std::vector<char>>;

struct builder;
struct array_builder;

struct invalid_document_size;

namespace detail {

/*!
 * \brief Type trait to determine whether a type is a basic_document (or basic_array)
 */
template <typename> struct is_document : std::false_type {};

#ifndef DOXYGEN_SHOULD_SKIP_THIS

template <typename Container, typename ElementContainer>
struct is_document<basic_document<Container, ElementContainer>> : std::true_type {};

template <typename Container, typename ElementContainer>
struct is_document<basic_array<Container, ElementContainer>> : std::true_type {};

#endif // DOXYGEN_SHOULD_SKIP_THIS

} // namespace detail
} // namespace jbson

#endif // JBSON_DOCUMENT_FWD_HPP
