//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_TRAITS_HPP
#define JBSON_TRAITS_HPP

#include <type_traits>

#include "./config.hpp"

JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
#include <boost/range/iterator_range.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/and.hpp>
#include <boost/utility/string_ref_fwd.hpp>
#include <boost/tti/has_member_function.hpp>
#include <boost/range/metafunctions.hpp>
#include <boost/mpl/same_as.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/quote.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/at.hpp>
JBSON_CLANG_POP_WARNINGS

#include "../element_fwd.hpp"

JBSON_PUSH_DISABLE_DEPRECATED_WARNING

namespace jbson {
namespace detail {

namespace mpl = boost::mpl;

/*!
 * \brief Turns an element_type into a boost::mpl constant.
 */
template <element_type EType> using element_type_c = mpl::integral_c<element_type, EType>;

/*!
 * \brief Trait to determine if an iterator is a pointer, or pointer in disguise.
 * \internal
 * Types such as std::vector and std::string use thin wrappers around pointers as iterators.
 * Specialisations for these inherit from std::true_type.
 *
 * Totally non-portable; only tested with libc++ and libstdc++.
 */
template <typename Iterator, typename Enable = void> struct is_iterator_pointer : std::false_type {};

#ifndef DOXYGEN_SHOULD_SKIP_THIS

template <typename Iterator> struct is_iterator_pointer<Iterator*, bool> : std::true_type {};

BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator_type)

// libc++, libstdc++
template <typename Iterator>
struct is_iterator_pointer<Iterator, std::enable_if_t<has_iterator_type<Iterator>::value>>
    : mpl::or_<std::is_pointer<typename Iterator::iterator_type>,
               std::is_constructible<Iterator, typename std::iterator_traits<Iterator>::pointer>> {};

template <typename Iterator>
struct is_iterator_pointer<Iterator, std::enable_if_t<!has_iterator_type<Iterator>::value>>
    : std::is_constructible<Iterator, typename std::iterator_traits<Iterator>::pointer> {};

template <typename Iterator>
struct is_iterator_pointer<boost::iterator_range<Iterator>> : is_iterator_pointer<Iterator> {};

#endif // DOXYGEN_SHOULD_SKIP_THIS

/*!
 * \brief Compile-time map. Maps element_types to C++ type for serialisation/deserialisation.
 * \internal
 * \tparam Container Some container or range type used for basic_element storage.
 * \tparam set bool condition to determine whether to use the map for serialisataion or deserialisation.
 *
 * Internal mapped types depend on the container given.
 * When used for finding serialisation types (set == true) it will use the container as-is.
 * Otherwise it uses a boost::iterator_range.
 *
 * When Container's iterators are pointers (according to is_iterator_pointer) all string types will be
 * boost::string_ref.
 * Otherwise strings are all std::string.
 *
 * \sa is_iterator_pointer ElementTypeMap ElementTypeMapSet
 */
template <typename Container, bool set = false> class TypeMap {
    using container_type =
        std::conditional_t<set, Container, boost::iterator_range<typename Container::const_iterator>>;
    using string_type = std::conditional_t < !set && is_iterator_pointer<typename container_type::iterator>::value,
          boost::string_ref, std::string > ;
public:
    /*!
     * \brief boost::mpl map.
     */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
    typedef boost::mpl::map<...> map_type;
#else // DOXYGEN_SHOULD_SKIP_THIS
    typedef typename mpl::map<
        mpl::pair<element_type_c<element_type::double_element>, double>,
        mpl::pair<element_type_c<element_type::string_element>, string_type>,
        mpl::pair<element_type_c<element_type::document_element>, basic_document<container_type, container_type>>,
        mpl::pair<element_type_c<element_type::array_element>, basic_array<container_type, container_type>>,
        mpl::pair<element_type_c<element_type::binary_element>, container_type>,
        mpl::pair<element_type_c<element_type::undefined_element>, void>,
        mpl::pair<element_type_c<element_type::oid_element>, std::array<char, 12>>,
        mpl::pair<element_type_c<element_type::boolean_element>, bool>,
        mpl::pair<element_type_c<element_type::date_element>, int64_t>,
        mpl::pair<element_type_c<element_type::null_element>, void>,
        mpl::pair<element_type_c<element_type::regex_element>, std::tuple<string_type, string_type>>,
        mpl::pair<element_type_c<element_type::db_pointer_element>, std::tuple<string_type, std::array<char, 12>>>,
        mpl::pair<element_type_c<element_type::javascript_element>, string_type>,
        mpl::pair<element_type_c<element_type::symbol_element>, string_type>,
        mpl::pair<element_type_c<element_type::scoped_javascript_element>,
                  std::tuple<string_type, basic_document<container_type, container_type>>>,
        mpl::pair<element_type_c<element_type::int32_element>, int32_t>,
        mpl::pair<element_type_c<element_type::timestamp_element>, int64_t>,
        mpl::pair<element_type_c<element_type::int64_element>, int64_t>,
        mpl::pair<element_type_c<element_type::min_key>, void>,
        mpl::pair<element_type_c<element_type::max_key>, void>>::type map_type;
#endif // DOXYGEN_SHOULD_SKIP_THIS
};

/*!
 * \brief Type alias to perform boost::mpl::at on TypeMap::map_type
 * \sa TypeMap ElementTypeMapSet
 * \internal
 * Maps an element_type to a type for deserialisation.
 */
template <element_type EType, typename Container>
using ElementTypeMap = typename mpl::at<typename TypeMap<Container>::map_type, element_type_c<EType>>::type;

/*!
 * \brief Type alias to perform boost::mpl::at on TypeMap::map_type
 * \sa TypeMap ElementTypeMap
 * \internal
 * Same as ElementTypeMap, but for serialising types rather than deserialising.
 */
template <element_type EType, typename Container>
using ElementTypeMapSet = typename mpl::at<typename TypeMap<Container, true>::map_type, element_type_c<EType>>::type;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
BOOST_TTI_HAS_MEMBER_FUNCTION(push_back);
#endif

/*!
 * \brief Type trait to determine whether a type is a boost::iterator_range
 */
template <typename T> struct is_iterator_range : std::false_type {};

#ifndef DOXYGEN_SHOULD_SKIP_THIS
template <typename IteratorT> struct is_iterator_range<boost::iterator_range<IteratorT>> : std::true_type {};
#endif // DOXYGEN_SHOULD_SKIP_THIS

BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator)
BOOST_MPL_HAS_XXX_TRAIT_DEF(const_iterator)

template <typename Container, typename Arg>
using container_has_push_back_arg_impl = has_member_function_push_back<Container, void, mpl::vector<Arg>>;

template <typename Container> struct container_has_push_back_impl {
    using type =
        mpl::or_<container_has_push_back_arg_impl<Container, typename boost::range_value<Container>::type>,
                 container_has_push_back_arg_impl<Container, const typename boost::range_value<Container>::type&>>;
};

/*!
 * \brief Type trait to determine if type is a container with a push_back() function.
 * \internal
 * Evaluates to std::false_type for all incompatible types (SFINAE friendly).
 *
 * \sa has_iterator has_const_iterator
 */
template <typename Container>
using container_has_push_back =
    typename mpl::eval_if<has_iterator<std::decay_t<Container>>, container_has_push_back_impl<std::decay_t<Container>>,
                          std::false_type>::type;

static_assert(container_has_push_back<std::vector<char>>::value, "");
static_assert(container_has_push_back<std::string>::value, "");
static_assert(!container_has_push_back<std::set<char>>::value, "");
static_assert(!container_has_push_back<char>::value, "");
static_assert(!container_has_push_back<double>::value, "");

#ifndef DOXYGEN_SHOULD_SKIP_THIS
template <typename RangeT>
using is_range = typename mpl::and_<has_iterator<std::decay_t<RangeT>>, has_const_iterator<std::decay_t<RangeT>>>::type;
#endif // DOXYGEN_SHOULD_SKIP_THIS

/*!
 * \brief Type trait to apply a unary metafunction to the result of a Range trait in a SFINAE safe manner.
 * \sa is_range_of_value is_range_of_same_value is_range_of_iterator
 */
template <typename RangeT, typename ElementTrait, typename RangeTrait>
using is_range_of = typename mpl::apply<
    typename mpl::eval_if<is_range<RangeT>, mpl::identity<ElementTrait>, mpl::identity<mpl::always<mpl::false_>>>::type,
    typename mpl::eval_if<is_range<RangeT>,
                          mpl::apply<RangeTrait, std::decay_t<RangeT>>, mpl::identity<void>>::type>::type;

/*!
 * \brief Type trait to apply a unary metafunction trait to the value_type of a Range.
 * \sa is_range_of_same_value is_range_of
 */
template <typename RangeT, typename ElementTrait>
using is_range_of_value = is_range_of<RangeT, ElementTrait, mpl::quote1<boost::range_value>>;

/*!
 * \brief Type trait to determine equivalence of the value_type of a Range.
 * \sa is_range_of_value is_range_of
 */
template <typename RangeT, typename ElementT>
using is_range_of_same_value = is_range_of_value<RangeT, mpl::bind2<mpl::quote2<std::is_same>, ElementT, mpl::_1>>;

static_assert(is_range_of_same_value<std::vector<char>, char>::value, "");
static_assert(!is_range_of_same_value<char, char>::value, "");
static_assert(!is_range_of_same_value<double, char>::value, "");

/*!
 * \brief Type trait to apply a unary metafunction trait to the iterator type of a Range.
 * \sa is_range_of_same_value is_range_of
 */
template <typename RangeT, typename ElementTrait>
using is_range_of_iterator = is_range_of<RangeT, ElementTrait, mpl::quote1<boost::range_mutable_iterator>>;

/*!
 * \brief Variadic version of boost::mpl::quoteN.
 */
template <template <typename...> class Fun, typename...> struct quote {
    //! apply enclosed metafunction
    template <typename... Args> using apply = typename Fun<Args...>::type;
};

static_assert(is_range_of_iterator<std::vector<char>,
                                   mpl::bind<quote<std::is_constructible>, std::vector<char>, mpl::_1, mpl::_1>>::value,
              "");

template <typename T> constexpr bool is_nothrow_swappable_impl() {
    using std::swap;
    return noexcept(swap(std::declval<std::decay_t<T>&>(), std::declval<std::decay_t<T>&>()));
}

/*!
 * \brief Type trait to determine if a type is nothrow/noexcept swappable.
 * \internal
 * Takes ADL into account.
 */
template <typename T> struct is_nothrow_swappable : std::integral_constant<bool, is_nothrow_swappable_impl<T>()> {};

} // namespace detail
} // namespace jbson

JBSON_POP_WARNINGS

#endif // JBSON_TRAITS_HPP
