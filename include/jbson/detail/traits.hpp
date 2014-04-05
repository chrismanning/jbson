//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_TRAITS_HPP
#define JBSON_TRAITS_HPP

#include <type_traits>

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

#include "../element_fwd.hpp"

namespace jbson {
namespace detail {

namespace mpl = boost::mpl;

template <element_type EType> using element_type_c = mpl::integral_c<element_type, EType>;

template <typename Iterator, typename Enable = void> struct is_iterator_pointer : std::false_type {};

template <typename Iterator> struct is_iterator_pointer<Iterator*, bool> : std::true_type {};

template <typename Iterator>
struct is_iterator_pointer<
    Iterator,
    std::enable_if_t<std::is_constructible<Iterator, typename std::iterator_traits<Iterator>::value_type*>::value>>
    : std::true_type {};

template <typename Iterator>
struct is_iterator_pointer<Iterator, std::enable_if_t<std::is_pointer<typename Iterator::iterator_type>::value>>
    : std::true_type {};

template <typename Iterator>
struct is_iterator_pointer<boost::iterator_range<Iterator>> : is_iterator_pointer<Iterator> {};

template <typename Container, bool set = false> struct TypeMap {
    using container_type =
        std::conditional_t<set, Container, boost::iterator_range<typename Container::const_iterator>>;
    using string_type = std::conditional_t < !set && is_iterator_pointer<typename container_type::iterator>::value,
          boost::string_ref, std::string > ;
    typedef typename mpl::map<
        mpl::pair<element_type_c<element_type::double_element>, double>,
        mpl::pair<element_type_c<element_type::string_element>, string_type>,
        mpl::pair<element_type_c<element_type::document_element>, basic_document<container_type, container_type>>,
        mpl::pair<element_type_c<element_type::array_element>, basic_array<container_type, container_type>>,
        mpl::pair<element_type_c<element_type::binary_element>, container_type>,
        mpl::pair<element_type_c<element_type::undefined_element>, void>,
        mpl::pair<element_type_c<element_type::oid_element>, std::array<char, 12>>,
        mpl::pair<element_type_c<element_type::boolean_element>, bool>,
        mpl::pair<element_type_c<element_type::date_element>,
                  std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>>,
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
};

template <element_type EType, typename Container>
using ElementTypeMap = typename mpl::at<typename TypeMap<Container>::map_type, element_type_c<EType>>::type;

template <element_type EType, typename Container>
using ElementTypeMapSet = typename mpl::at<typename TypeMap<Container, true>::map_type, element_type_c<EType>>::type;

BOOST_TTI_HAS_MEMBER_FUNCTION(push_back);

template <typename T> struct is_iterator_range : std::false_type {};

template <typename IteratorT> struct is_iterator_range<boost::iterator_range<IteratorT>> : std::true_type {};

template <typename T, typename T2> using lazy_enable_if = typename boost::lazy_enable_if<T, T2>;

template <typename T> struct is_string_literal : std::false_type {};

template <typename CharT, size_t N> struct is_string_literal<CharT (&)[N]> : std::true_type {};

template <typename CharT, size_t N> struct is_string_literal<const CharT (&)[N]> : std::true_type {};

template <typename RandomAccessContainer>
using is_random_access_container = typename std::is_same<
    typename boost::iterator_category_to_traversal<typename boost::range_category<RandomAccessContainer>::type>::type,
    boost::random_access_traversal_tag>;

BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator)
BOOST_MPL_HAS_XXX_TRAIT_DEF(const_iterator)

template <typename RangeT, typename ElementTrait, typename RangeTrait>
using is_range_of = typename boost::mpl::apply<
    ElementTrait, typename boost::mpl::eval_if<
                      boost::mpl::and_<has_iterator<std::decay_t<RangeT>>, has_const_iterator<std::decay_t<RangeT>>>,
                      boost::mpl::apply<RangeTrait, std::decay_t<RangeT>>, boost::mpl::identity<void>>::type>::type;

template <typename RangeT, typename ElementTrait>
using is_range_of_value = is_range_of<RangeT, ElementTrait, boost::mpl::quote1<boost::range_value>>;

template <typename RangeT, typename ElementT>
using is_range_of_same_value =
    is_range_of_value<RangeT, boost::mpl::bind2<boost::mpl::quote2<std::is_same>, ElementT, boost::mpl::_1>>;

static_assert(is_range_of_same_value<std::vector<char>, char>::value, "");
static_assert(!is_range_of_same_value<char, char>::value, "");
static_assert(!is_range_of_same_value<double, char>::value, "");

template <typename RangeT, typename ElementTrait>
using is_range_of_iterator = is_range_of<RangeT, ElementTrait, boost::mpl::quote1<boost::range_mutable_iterator>>;

template <template <typename...> class Fun, typename...> struct quote {
    template <typename... Args> using apply = typename Fun<Args...>::type;
};

static_assert(is_range_of_iterator<std::vector<char>, boost::mpl::bind<quote<std::is_constructible>, std::vector<char>,
                                                                       boost::mpl::_1, boost::mpl::_1>>::value,
              "");

template <typename T> constexpr bool is_nothrow_swappable_impl() {
    using std::swap;
    return noexcept(swap(std::declval<std::decay_t<T>&>(), std::declval<std::decay_t<T>&>()));
}

template <typename T> struct is_nothrow_swappable {
    using type = is_nothrow_swappable<T>;
    static constexpr bool value = is_nothrow_swappable_impl<T>();
};

} // namespace detail
} // namespace jbson

#endif // JBSON_TRAITS_HPP
