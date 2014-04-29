//          Copyright Christian Manning 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <vector>

#include <jbson/element.hpp>
#include <jbson/document.hpp>
#include <jbson/builder.hpp>
#include <jbson/detail/traits.hpp>
using namespace jbson;
using namespace jbson::detail;

#include <gtest/gtest.h>

// builder conversions
static_assert(std::is_convertible<builder, document>::value, "");
static_assert(std::is_convertible<builder, basic_document<std::vector<char>, std::vector<char>>>::value, "");
static_assert(std::is_convertible<array_builder, array>::value, "");
static_assert(std::is_convertible<array_builder, basic_array<std::vector<char>, std::vector<char>>>::value, "");

// container_has_push_back
static_assert(container_has_push_back<std::vector<char>>::value, "");
static_assert(container_has_push_back<std::string>::value, "");
static_assert(!container_has_push_back<std::set<char>>::value, "");
static_assert(!container_has_push_back<char>::value, "");
static_assert(!container_has_push_back<double>::value, "");

// is_range_of_value
static_assert(detail::is_range_of_value<document_set, boost::mpl::quote1<detail::is_element>>::value, "");

// is_range_of_same_value
static_assert(is_range_of_same_value<std::vector<char>, char>::value, "");
static_assert(!is_range_of_same_value<char, char>::value, "");
static_assert(!is_range_of_same_value<double, char>::value, "");

// is_range_of_iterator
static_assert(is_range_of_iterator<std::vector<char>,
                                   mpl::bind<quote<std::is_constructible>, std::vector<char>, mpl::_1, mpl::_1>>::value,
              "");

// find_second
//static_assert(std::is_same<find_second<TypeMap<std::vector<char>>::map_type, double>,
//                           mpl::begin<TypeMap<std::vector<char>>::map_type>::type>::value, "");

// find_second_if
//static_assert(std::is_same<find_if_second<TypeMap<std::vector<char>>::map_type,
//                            mpl::bind<quote<std::is_same>, double, mpl::_1>>,
//              mpl::begin<TypeMap<std::vector<char>>::map_type>::type>::value, "");

//static_assert(std::is_same<find_if_second<TypeMap<std::vector<char>>::map_type,
//                            mpl::bind<quote<std::is_same>, int32_t, mpl::_1>>,
//              mpl::advance_c<mpl::begin<TypeMap<std::vector<char>>::map_type>::type, 0x10-1>::type>::value, "");

static_assert(std::is_same<find_if_second<TypeMap<std::vector<char>>::map_type,
                            mpl::bind<quote<std::is_same>, std::chrono::milliseconds, mpl::_1>>,
              mpl::end<TypeMap<std::vector<char>>::map_type>::type>::value, "");

// is_valid_element_value_type
static_assert(is_valid_element_value_type<std::vector<char>, double>::value, "");
static_assert(is_valid_element_value_type<std::vector<char>, bool>::value, "");
static_assert(is_valid_element_value_type<std::vector<char>, float>::value, "");
static_assert(is_valid_element_value_type<std::vector<char>, int32_t>::value, "");
static_assert(is_valid_element_value_type<std::vector<char>, int64_t>::value, "");
static_assert(is_valid_element_value_type<std::vector<char>, int>::value, "");
static_assert(is_valid_element_value_type<std::vector<char>, long long>::value, "");
static_assert(is_valid_element_value_type<std::vector<char>, short>::value, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>>::map_type,
              mpl::bind<quote<is_convertible>, double, mpl::_1>>>::type>::type::value
              == element_type::double_element, "");
static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>>::map_type,
              mpl::bind<quote<is_convertible>, float, mpl::_1>>>::type>::type::value
              == element_type::double_element, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>>::map_type,
              mpl::bind<quote<is_convertible>, int, mpl::_1>>>::type>::type::value
              == element_type::int32_element, "");
static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>>::map_type,
              mpl::bind<quote<is_convertible>, int32_t, mpl::_1>>>::type>::type::value
              == element_type::int32_element, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>>::map_type,
              mpl::bind<quote<is_convertible>, long long, mpl::_1>>>::type>::type::value
              == element_type::int64_element, "");
static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>>::map_type,
              mpl::bind<quote<is_convertible>, int64_t, mpl::_1>>>::type>::type::value
              == element_type::int64_element, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>>::map_type,
              mpl::bind<quote<is_convertible>, short, mpl::_1>>>::type>::type::value
              == element_type::int32_element, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>>::map_type,
              mpl::bind<quote<is_convertible>, decltype(std::make_tuple("","")), mpl::_1>>>::type>::type::value
              == element_type::regex_element, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>>::map_type,
              mpl::bind<quote<is_convertible>, decltype(builder()), mpl::_1>>>::type>::type::value
              == element_type::document_element, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>>::map_type,
              mpl::bind<quote<is_convertible>, decltype(std::make_tuple("",document())), mpl::_1>>>::type>::type::value
              == element_type::scoped_javascript_element, "");

// is_valid_element_set_type
static_assert(is_valid_element_set_type<std::vector<char>, double>::value, "");
static_assert(is_valid_element_set_type<std::vector<char>, bool>::value, "");
static_assert(is_valid_element_set_type<std::vector<char>, float>::value, "");
static_assert(is_valid_element_set_type<std::vector<char>, int32_t>::value, "");
static_assert(is_valid_element_set_type<std::vector<char>, int64_t>::value, "");
static_assert(is_valid_element_set_type<std::vector<char>, int>::value, "");
static_assert(is_valid_element_set_type<std::vector<char>, long long>::value, "");
static_assert(is_valid_element_set_type<std::vector<char>, short>::value, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>, true>::map_type,
              mpl::bind<quote<is_constructible>, mpl::_1, double>>>::type>::type::value
              == element_type::double_element, "");
static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>, true>::map_type,
              mpl::bind<quote<is_constructible>, mpl::_1, float>>>::type>::type::value
              == element_type::double_element, "");
static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>, true>::map_type,
              mpl::bind<quote<is_constructible>, mpl::_1, const double>>>::type>::type::value
              == element_type::double_element, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>, true>::map_type,
              mpl::bind<quote<is_constructible>, mpl::_1, int>>>::type>::type::value
              == element_type::int32_element, "");
static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>, true>::map_type,
              mpl::bind<quote<is_constructible>, mpl::_1, int32_t>>>::type>::type::value
              == element_type::int32_element, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>, true>::map_type,
              mpl::bind<quote<is_constructible>, mpl::_1, long long>>>::type>::type::value
              == element_type::int64_element, "");
static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>, true>::map_type,
              mpl::bind<quote<is_constructible>, mpl::_1, int64_t>>>::type>::type::value
              == element_type::int64_element, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>, true>::map_type,
              mpl::bind<quote<is_constructible>, mpl::_1, short>>>::type>::type::value
              == element_type::int32_element, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>, true>::map_type,
              mpl::bind<quote<is_constructible>, mpl::_1, decltype(std::make_tuple("",""))>>>::type>::type::value
              == element_type::regex_element, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>, true>::map_type,
              mpl::bind<quote<is_constructible>, mpl::_1, decltype(builder())>>>::type>::type::value
              == element_type::document_element, "");

static_assert(mpl::first<mpl::deref<find_if_second<TypeMap<std::vector<char>, true>::map_type,
              mpl::bind<quote<is_constructible>, mpl::_1, decltype(std::make_tuple("",document()))>>>::type>::type::value
              == element_type::scoped_javascript_element, "");

TEST(TraitsTest, Test1) {
    SUCCEED();
}
