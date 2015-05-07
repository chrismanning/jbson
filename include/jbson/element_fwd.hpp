//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_ELEMENT_FWD_HPP
#define JBSON_ELEMENT_FWD_HPP

#include <vector>
#include <set>

#include "detail/config.hpp"

/*!
 * \namespace jbson
 * \brief The jbson namespace contains the main classes and functions for viewing,
 *        creating and manipulating BSON data.
 *
 * The jbson projects defines nothing outside of this namespace
 */
namespace jbson {

/*!
 * \brief BSON element
 */
template <class Container> struct basic_element;

/*!
 * \brief Default basic_element type alias for owned BSON data
 */
using element = basic_element<std::vector<char>>;

/*!
 * \brief The element_type enum represents a BSON data type
 */
enum class element_type : uint8_t {
    double_element = 0x01,                      //!< double
    string_element = 0x02,                      //!< std::string or std::string_view (string_type)
    document_element = 0x03,                    //!< basic_document<boost::iterator_range<...>> (document_type)
    array_element = 0x04,                       //!< basic_array<boost::iterator_range<...>>
    binary_element = 0x05,                      //!< \unimplemented binary data storage
    undefined_element JBSON_DEPRECATED = 0x06,  //!< void \deprecated use null_element instead
    oid_element = 0x07,                         //!< std::array<char, 12>
    boolean_element = 0x08,                     //!< bool
    date_element = 0x09,                        //!< int64_t
    null_element = 0x0A,                        //!< void
    regex_element = 0x0B,                       //!< std::tuple<string_type, string_type>
    db_pointer_element JBSON_DEPRECATED = 0x0C, //!< std::tuple<string_type, std::array<char, 12>>
                                                //!< \deprecated use a string_element + oid_element
    javascript_element = 0x0D,                  //!< string_type
    symbol_element JBSON_DEPRECATED = 0x0E,     //!< string_type
                                                //!< \deprecated use a string_element or javascript_element
    scoped_javascript_element = 0x0F,           //!< std::tuple<string_type, document_type>
    int32_element = 0x10,                       //!< int32_t
    timestamp_element = 0x11,                   //!< int64_t
    int64_element = 0x12,                       //!< int64_t
    min_key = 0xFF,                             //!< void
    max_key = 0x7F                              //!< void
};

namespace detail {
inline bool valid_type(element_type etype) {
    return ((uint8_t)etype >= 0x01 && (uint8_t)etype <= 0x12) || (uint8_t)etype == 0xFF || (uint8_t)etype == 0x7F;
}
} // namespace detail

/*!
 * \namespace jbson::detail
 * \brief namespace detail contains internal functions and classes
 * \internal
 */
namespace detail {

struct elem_compare;

/*!
 * \brief Type trait to determine whether a type is a basic_element
 */
template <typename> struct is_element : std::false_type {};

#ifndef DOXYGEN_SHOULD_SKIP_THIS
template <typename Container> struct is_element<basic_element<Container>> : std::true_type {};
#endif

} // namespace detail

/*!
 * \brief BSON document in the form of a std::set for ease of manipulation
 */
template <typename Container> using basic_document_set = std::multiset<basic_element<Container>, detail::elem_compare>;

/*!
 * \brief Default basic_document_set type alias
 */
using document_set = basic_document_set<std::vector<char>>;

struct jbson_error;
struct invalid_element_type;
struct incompatible_element_conversion;
struct incompatible_type_conversion;
struct invalid_element_size;

} // namespace jbson

#endif // JBSON_ELEMENT_FWD_HPP
