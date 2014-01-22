//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_DOCUMENT_HPP
#define JBSON_DOCUMENT_HPP

#include <memory>
#include <vector>
#include <set>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/optional.hpp>
#include <boost/range/as_literal.hpp>

#include "document_fwd.hpp"
#include "element_fwd.hpp"
#include "detail/traits.hpp"
#include "element.hpp"

namespace jbson {

struct invalid_document_size : jbson_error {
    const char* what() const noexcept override { return "invalid_document_size"; }
};

namespace detail {

template <typename Value, typename BaseIterator>
struct document_iter : boost::iterator_facade<document_iter<Value, BaseIterator>, Value, boost::forward_traversal_tag,
                                              std::add_lvalue_reference_t<std::add_const_t<Value>>> {
    document_iter() = default;
    document_iter(document_iter&&) = default;
    document_iter& operator=(document_iter&&) = default;
    document_iter(const document_iter&) = default;
    document_iter& operator=(const document_iter&) = default;

    document_iter(BaseIterator it1, BaseIterator it2) : m_start(it1), m_end(it2) {
        if(m_start == m_end)
            return;
        m_cur = Value{m_start, m_end};
    }

    template <class OtherValue, typename OtherIt>
    document_iter(const document_iter<OtherValue, OtherIt>& other,
                  std::enable_if_t<std::is_convertible<OtherValue, Value>::value&&
                                       std::is_convertible<OtherIt, BaseIterator>::value>* = nullptr)
        : m_start(other.m_start), m_end(other.m_end) {
        if(m_start == m_end)
            return;
        m_cur = Value{m_start, m_end};
    }

  private:
    friend class boost::iterator_core_access;
    template <typename, typename> friend struct document_iter;

    template <typename OtherValue, typename OtherIterator>
    bool equal(const document_iter<OtherValue, OtherIterator>& other) const {
        return other.m_start == m_start;
    }

    void increment() {
        if(m_start == m_end) {
            m_cur = boost::none;
            return;
        }

        std::advance(m_start, m_cur->size());
        if(m_start == m_end)
            m_cur = boost::none;
        else
            m_cur = Value{m_start, m_end};
    }

    std::add_lvalue_reference_t<std::add_const_t<Value>> dereference() const { return *m_cur; }

    BaseIterator m_start;
    BaseIterator m_end;
    boost::optional<std::decay_t<Value>> m_cur;
};

} // namespace detail

static_assert(detail::is_range_of_value<document_set, boost::mpl::quote1<detail::is_element>>::value, "");

struct builder;

template <class Container, class ElementContainer> class basic_document {
    template <class, class> friend struct detail::document_iter;

  public:
    using container_type = Container;
    using element_type = basic_element<ElementContainer>;
    using iterator = typename detail::document_iter<element_type, typename container_type::const_iterator>;
    using const_iterator = iterator;
    using value_type = element_type;

    basic_document() = default;

    basic_document(const basic_document&) = default;
    basic_document& operator=(const basic_document&) = default;

    basic_document(basic_document&&) = default;
    basic_document& operator=(basic_document&&) = default;

    template <typename SomeType>
    explicit basic_document(SomeType&& c,
                            std::enable_if_t<std::is_same<container_type, std::decay_t<SomeType>>::value>* = nullptr)
        : m_data(std::forward<SomeType>(c)) {
        if(static_cast<ptrdiff_t>(boost::distance(m_data)) !=
               detail::little_endian_to_native<int32_t>(m_data.begin(), m_data.end()))
            BOOST_THROW_EXCEPTION(invalid_document_size{}
                                  << expected_size(detail::little_endian_to_native<int32_t>(m_data.begin(), m_data.end()))
                                  << actual_size(boost::distance(m_data)));
        if(static_cast<ptrdiff_t>(boost::distance(m_data)) <= static_cast<ptrdiff_t>(sizeof(int32_t)))
            BOOST_THROW_EXCEPTION(invalid_document_size{});
    }

    template <typename OtherContainer>
    basic_document(const basic_document<OtherContainer>& other,
                   std::enable_if_t<std::is_constructible<Container, OtherContainer>::value>* = nullptr)
        : m_data(other.m_data) {}

    template <typename OtherContainer>
    basic_document(const basic_document<OtherContainer>& other,
                   std::enable_if_t<!std::is_constructible<Container, OtherContainer>::value>* = nullptr) {
        boost::range::push_back(m_data, other.m_data);
    }

    template <typename CharT, size_t N> explicit basic_document(CharT (&rng)[N]) = delete;

    template <typename CharT, size_t N> explicit basic_document(const CharT (&rng)[N]) = delete;

    template <
        typename ForwardRange, typename = std::enable_if_t<!std::is_same<std::decay_t<ForwardRange>, builder>::value>,
        typename = std::enable_if_t<detail::is_range_of_same_value<ForwardRange, char>::value>,
        typename =
            std::enable_if_t<!std::is_constructible<container_type, ForwardRange>::value &&
                             detail::is_range_of_iterator<
                                 ForwardRange, boost::mpl::bind<detail::quote<std::is_constructible>, container_type,
                                                                boost::mpl::_1, boost::mpl::_1>>::value>>
    explicit basic_document(ForwardRange&& rng)
        : basic_document(container_type(std::begin(rng), std::end(rng))) {}

    template <
        typename ForwardRange, typename = std::enable_if_t<!std::is_same<std::decay_t<ForwardRange>, builder>::value>,
        typename = std::enable_if_t<!std::is_same<std::decay_t<ForwardRange>, container_type>::value>,
        typename = std::enable_if_t<detail::is_range_of_same_value<ForwardRange, char>::value>,
        typename =
            std::enable_if_t<std::is_constructible<container_type, ForwardRange>::value&& detail::is_range_of_iterator<
                ForwardRange, boost::mpl::bind<detail::quote<std::is_constructible>, typename container_type::iterator,
                                               boost::mpl::_1>>::value>>
    explicit basic_document(ForwardRange&& rng, std::enable_if_t<true>* = nullptr)
        : basic_document(container_type(rng)) {}

    template <typename ForwardIterator>
    basic_document(
        ForwardIterator first, ForwardIterator last,
        std::enable_if_t<detail::is_range_of_same_value<boost::iterator_range<ForwardIterator>, char>::value>* =
            nullptr,
        std::enable_if_t<std::is_constructible<container_type, ForwardIterator, ForwardIterator>::value>* = nullptr)
        : basic_document(container_type{first, last}) {}

    template <typename ForwardRange>
    explicit basic_document(
        ForwardRange&& rng,
        std::enable_if_t<
            boost::mpl::and_<detail::is_range_of_value<ForwardRange, boost::mpl::quote1<detail::is_element>>,
                             boost::mpl::not_<detail::is_iterator_range<container_type>>,
                             boost::mpl::not_<detail::is_document<std::decay_t<ForwardRange>>>>::type::value>* = nullptr) {
        static_assert(!detail::is_document<std::decay_t<ForwardRange>>::value,"");
        std::array<char, 4> arr{{0, 0, 0, 0}};
        boost::range::push_back(m_data, arr);
        for(auto&& e : rng) {
            e.write_to_container(m_data);
        }
        m_data.push_back('\0');
        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_data.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_data.begin());
    }

    template <typename ForwardRange>
    explicit basic_document(
        ForwardRange&& rng,
        std::enable_if_t<
            boost::mpl::and_<detail::is_range_of_value<ForwardRange, boost::mpl::quote1<detail::is_element>>,
                             detail::is_iterator_range<container_type>,
                             boost::mpl::not_<detail::is_document<std::decay_t<ForwardRange>>>>::type::value>* = nullptr) {
        static_assert(!detail::is_document<std::decay_t<ForwardRange>>::value,"");
        std::array<char, 4> arr{{0, 0, 0, 0}};
        auto out = boost::range::copy(arr, m_data.begin());
        for(auto&& e : rng) {
            e.write_to_container(m_data);
        }
        *out++ = '\0';
        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_data.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_data.begin());
    }

    template <typename ForwardIterator>
    basic_document(ForwardIterator first, ForwardIterator last,
                   std::enable_if_t<detail::is_element<typename boost::iterator_value<ForwardIterator>::type>::value &&
                                    !detail::is_iterator_range<container_type>::value>* = nullptr)
        : basic_document(boost::make_iterator_range(first, last)) {}

    const_iterator begin() const {
        if(boost::distance(m_data) <= static_cast<ptrdiff_t>(sizeof(int32_t)))
            BOOST_THROW_EXCEPTION(invalid_document_size{});
        return {std::next(m_data.begin(), sizeof(int32_t)), std::prev(m_data.end())};
    }

    const_iterator end() const noexcept {
        auto last = std::prev(m_data.end());
        return {last, last};
    }

    const_iterator find(boost::string_ref elem_name) const {
        auto end = this->end();
        for(auto i = begin(); i != end; ++i) {
            if(i->name() == elem_name)
                return i;
        }
        return end;
    }

    int32_t size() const noexcept { return boost::distance(m_data); }

    const container_type& data() const& {
        if(static_cast<ptrdiff_t>(m_data.size()) <= static_cast<ptrdiff_t>(sizeof(int32_t)))
            BOOST_THROW_EXCEPTION(invalid_document_size{});
        return m_data;
    }

    container_type&& data() && {
        if(static_cast<ptrdiff_t>(m_data.size()) <= static_cast<ptrdiff_t>(sizeof(int32_t)))
            BOOST_THROW_EXCEPTION(invalid_document_size{});
        return std::move(m_data);
    }

    template <typename EContainer> explicit operator basic_document_set<EContainer>() const {
        auto set = basic_document_set<EContainer>{};
        for(auto&& e : *this) {
            set.emplace(e);
        }
        return std::move(set);
    }

  private:
    container_type m_data;
    template <typename, typename> friend class basic_document;
    template <typename, typename> friend class basic_array;
    static_assert(!std::is_constructible<basic_document, int>::value, "");
    static_assert(!std::is_constructible<basic_document, double>::value, "");
    static_assert(!std::is_constructible<basic_document, builder>::value, "");
};

template <class Container, class ElementContainer>
class basic_array : basic_document<Container, ElementContainer> {
    using base = basic_document<Container, ElementContainer>;
    template <typename, typename> friend class basic_array;

  public:
    using typename base::container_type;
    using typename base::element_type;
    using typename base::iterator;
    using typename base::const_iterator;
    using typename base::value_type;

    using base::m_data;
    using base::begin;
    using base::end;
    using base::data;
    using base::size;

    basic_array() = default;

    basic_array(const basic_array&) = default;
    basic_array& operator=(const basic_array&) = default;

    basic_array(basic_array&&) = default;
    basic_array& operator=(basic_array&&) = default;

    template <typename SomeType>
    explicit basic_array(SomeType&& c,
                         std::enable_if_t<std::is_same<container_type, std::decay_t<SomeType>>::value>* = nullptr)
        : base(std::forward<SomeType>(c)) {
        if(static_cast<ptrdiff_t>(boost::distance(m_data)) <= static_cast<ptrdiff_t>(sizeof(int32_t)) ||
           static_cast<ptrdiff_t>(boost::distance(m_data)) !=
               detail::little_endian_to_native<int32_t>(m_data.begin(), m_data.end()))
            BOOST_THROW_EXCEPTION(invalid_document_size{});
    }

    template <typename OtherContainer>
    basic_array(const basic_array<OtherContainer>& other,
                std::enable_if_t<std::is_constructible<Container, OtherContainer>::value>* = nullptr)
        : base(other.m_data) {}

    template <typename OtherContainer>
    basic_array(const basic_array<OtherContainer>& other,
                std::enable_if_t<!std::is_constructible<Container, OtherContainer>::value>* = nullptr) {
        boost::range::push_back(m_data, other.m_data);
    }

    template <typename CharT, size_t N> explicit basic_array(CharT (&rng)[N]) = delete;

    template <typename CharT, size_t N> explicit basic_array(const CharT (&rng)[N]) = delete;

    template <
        typename ForwardRange, typename = std::enable_if_t<!std::is_same<std::decay_t<ForwardRange>, builder>::value>,
        typename = std::enable_if_t<detail::is_range_of_same_value<ForwardRange, char>::value>,
        typename =
            std::enable_if_t<!std::is_constructible<container_type, ForwardRange>::value &&
                             detail::is_range_of_iterator<
                                 ForwardRange, boost::mpl::bind<detail::quote<std::is_constructible>, container_type,
                                                                boost::mpl::_1, boost::mpl::_1>>::value>>
    explicit basic_array(ForwardRange&& rng)
        : basic_array(container_type(std::begin(rng), std::end(rng))) {}

    template <
        typename ForwardRange, typename = std::enable_if_t<!std::is_same<std::decay_t<ForwardRange>, builder>::value>,
        typename = std::enable_if_t<!std::is_same<std::decay_t<ForwardRange>, container_type>::value>,
        typename = std::enable_if_t<detail::is_range_of_same_value<ForwardRange, char>::value>,
        typename =
            std::enable_if_t<std::is_constructible<container_type, ForwardRange>::value&& detail::is_range_of_iterator<
                ForwardRange, boost::mpl::bind<detail::quote<std::is_constructible>, typename container_type::iterator,
                                               boost::mpl::_1>>::value>>
    explicit basic_array(ForwardRange&& rng, std::enable_if_t<true>* = nullptr)
        : basic_array(container_type(rng)) {}

    template <typename ForwardIterator>
    basic_array(ForwardIterator first, ForwardIterator last,
                std::enable_if_t<detail::is_range_of_same_value<boost::iterator_range<ForwardIterator>, char>::value>* =
                    nullptr,
                std::enable_if_t<std::is_constructible<container_type, ForwardIterator, ForwardIterator>::value>* =
                    nullptr)
        : basic_array(container_type{first, last}) {}

    template <typename ForwardRange>
    explicit basic_array(
        ForwardRange&& rng,
        std::enable_if_t<
            boost::mpl::and_<detail::is_range_of_value<ForwardRange, boost::mpl::quote1<detail::is_element>>,
                             boost::mpl::not_<detail::is_iterator_range<container_type>>,
                             boost::mpl::not_<detail::is_document<std::decay_t<ForwardRange>>>>::type::value>* = nullptr) {
        std::array<char, 4> arr{{0, 0, 0, 0}};
        boost::range::push_back(m_data, arr);
        for(auto&& e : rng) {
            e.write_to_container(m_data);
        }
        m_data.push_back('\0');
        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_data.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_data.begin());
    }

  template <typename ForwardRange>
  explicit basic_array(
      ForwardRange&& rng,
      std::enable_if_t<
          boost::mpl::and_<detail::is_range_of_value<ForwardRange, boost::mpl::quote1<detail::is_element>>,
                           detail::is_iterator_range<container_type>,
                           boost::mpl::not_<detail::is_document<std::decay_t<ForwardRange>>>>::type::value>* = nullptr) {
      std::array<char, 4> arr{{0, 0, 0, 0}};
      auto out = boost::range::copy(arr, m_data.begin());
      for(auto&& e : rng) {
          e.write_to_container(m_data);
      }
      *out++ = '\0';
      auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_data.size()));
      static_assert(4 == size.size(), "");

      boost::range::copy(size, m_data.begin());
  }

    template <typename ForwardIterator>
    basic_array(ForwardIterator first, ForwardIterator last,
                std::enable_if_t<detail::is_element<typename boost::iterator_value<ForwardIterator>::type>::value &&
                                 !detail::is_iterator_range<container_type>::value>* = nullptr)
        : basic_array(boost::make_iterator_range(first, last)) {}

    const_iterator find(int32_t idx) const { return base::find(std::to_string(idx)); }

    template <typename RandomAccessContainer,
              typename =
                  std::enable_if_t<std::is_same<typename boost::iterator_category_to_traversal<
                                                    typename boost::range_category<RandomAccessContainer>::type>::type,
                                                boost::random_access_traversal_tag>::value>>
    explicit operator RandomAccessContainer() const {
        auto vec = RandomAccessContainer{};
        for(auto&& e : *this)
            vec.push_back(e);
        boost::range::stable_sort(vec, [](auto&& e1, auto&& e2) {
            return std::strtol(e1.name().begin(), nullptr, 0) < std::strtol(e2.name().begin(), nullptr, 0);
        });
        return std::move(vec);
    }
};

namespace detail {

template <typename T, typename Container>
struct is_valid_func<T, Container,
                     std::enable_if_t<std::is_convertible<std::decay_t<T>, document>::value>> {
    template <element_type EType, typename... Args> struct inner : std::false_type {
        static_assert(sizeof...(Args) == 0, "");
    };
    template <typename... Args> struct inner<element_type::document_element, Args...> : std::true_type {
        static_assert(sizeof...(Args) == 0, "");
    };
    template <typename... Args> struct inner<element_type::array_element, Args...> : std::true_type {
        static_assert(sizeof...(Args) == 0, "");
    };
};

} // namespace detail
} // namespace jbson

#endif // JBSON_DOCUMENT_HPP
