//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_DOCUMENT_FWD_HPP
#define JBSON_DOCUMENT_FWD_HPP

#include <vector>

namespace jbson {

template <class Container, class ElementContainer = boost::iterator_range<typename Container::const_iterator>>
class basic_document;
template <class Container, class ElementContainer = boost::iterator_range<typename Container::const_iterator>>
class basic_array;

using document = basic_document<std::vector<char>>;
using array = basic_array<std::vector<char>>;

struct invalid_document_size;

namespace detail {

template <typename> struct is_document : std::false_type {};

template <typename Container, typename ElementContainer>
struct is_document<basic_document<Container, ElementContainer>> : std::true_type {};

template <typename Container, typename ElementContainer>
struct is_document<basic_array<Container, ElementContainer>> : std::true_type {};

} // namespace detail
} // namespace jbson

#endif // JBSON_DOCUMENT_FWD_HPP
