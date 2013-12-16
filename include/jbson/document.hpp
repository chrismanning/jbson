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
#include <boost/optional.hpp>
#include <boost/range/metafunctions.hpp>

#include <jbson/element.hpp>

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
    boost::optional<typename std::decay<Value>::type> m_cur;
};

struct elem_string_compare {
    using is_transparent = std::true_type;
    template <typename EContainer, typename EContainer2>
    bool operator()(const basic_element<EContainer>& lhs, const basic_element<EContainer2>& rhs) const {
        return lhs.name() < rhs.name();
    }
    template <typename EContainer> bool operator()(const basic_element<EContainer>& lhs, boost::string_ref rhs) const {
        return lhs.name() < rhs;
    }
    template <typename EContainer> bool operator()(boost::string_ref lhs, const basic_element<EContainer>& rhs) const {
        return lhs < rhs.name();
    }
};

template <typename T> struct is_iterator_range : std::false_type {};

template <typename IteratorT> struct is_iterator_range<boost::iterator_range<IteratorT>> : std::true_type {};

} // namespace detail

template <typename Container>
using basic_document_set = std::multiset<basic_element<Container>, detail::elem_string_compare>;

using document_set = basic_document_set<element::container_type>;

struct doc_builder;

template <class Container, class ElementContainer = Container> class basic_document {
    template <class, class> friend struct detail::document_iter;

  public:
    using container_type = Container;
    using element_type = basic_element<ElementContainer>;
    using iterator = typename detail::document_iter<element_type, typename container_type::const_iterator>;
    using const_iterator = iterator;

    basic_document() = default;

    basic_document(const basic_document&) = default;
    basic_document& operator=(const basic_document&) = default;

    basic_document(basic_document&&) = default;
    basic_document& operator=(basic_document&&) = default;

    explicit basic_document(container_type&& c) : m_data(std::move(c)) {
        if(m_data.size() <= sizeof(int32_t) ||
           m_data.size() != detail::little_endian_to_native<int32_t>(m_data.begin(), m_data.end()))
            BOOST_THROW_EXCEPTION(invalid_document_size{});
    }

    template <typename ForwardRange,
              typename = std::enable_if_t<!std::is_same<std::decay_t<ForwardRange>, doc_builder>::value>>
    explicit basic_document(const ForwardRange& rng)
        : basic_document(std::begin(rng), std::end(rng)) {}

    template <typename ForwardIterator>
    basic_document(ForwardIterator first, ForwardIterator last,
                   std::enable_if_t<std::is_same<typename boost::iterator_value<ForwardIterator>::type, char>::value>* =
                       nullptr)
        : m_data(first, last) {
        if(m_data.size() <= sizeof(int32_t) ||
           m_data.size() != detail::little_endian_to_native<int32_t>(m_data.begin(), m_data.end()))
            BOOST_THROW_EXCEPTION(invalid_document_size{});
    }

    template <typename ForwardIterator>
    basic_document(ForwardIterator first, ForwardIterator last,
                   std::enable_if_t<detail::is_element<typename boost::iterator_value<ForwardIterator>::type>::value &&
                                    !detail::is_iterator_range<container_type>::value>* = nullptr) {
        std::array<char, 4> arr{0, 0, 0, 0};
        boost::range::push_back(m_data, arr);
        for(auto&& e : boost::make_iterator_range(first, last)) {
            e.write_to_container(m_data);
        }
        m_data.push_back('\0');
        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_data.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_data.begin());
    }

    const_iterator begin() const {
        if(m_data.size() <= sizeof(int32_t))
            BOOST_THROW_EXCEPTION(invalid_document_size{});
        return {std::next(m_data.begin(), sizeof(int32_t)), std::prev(m_data.end())};
    }

    const_iterator end() const {
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

    int32_t size() const { return m_data.size(); }

    const container_type& data() const& {
        if(m_data.size() <= sizeof(int32_t))
            BOOST_THROW_EXCEPTION(invalid_document_size{});
        return m_data;
    }

    container_type&& data() && {
        if(m_data.size() <= sizeof(int32_t))
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
};

using document = basic_document<std::vector<char>>;

template <class Container, class ElementContainer = Container>
class basic_array : basic_document<Container, ElementContainer> {
    using base = basic_document<Container, ElementContainer>;

  public:
    using typename base::container_type;
    using typename base::element_type;
    using typename base::iterator;
    using typename base::const_iterator;

    using base::basic_document;
    using base::begin;
    using base::end;
    using base::data;
    using base::size;

    const_iterator find(int32_t idx) const { return base::find(std::to_string(idx)); }

    template <typename RandomAccessContainer,
              typename =
                  std::enable_if_t<std::is_same<typename boost::iterator_category_to_traversal<
                                                    typename boost::range_category<RandomAccessContainer>::type>::type,
                                                boost::random_access_traversal_tag>::value>>
    explicit operator RandomAccessContainer() const {
        auto vec = RandomAccessContainer{};
        for(auto&& e : *this)
            vec.emplace_back(e);
        boost::range::stable_sort(vec,
                                  [](auto&& e1, auto&& e2) { return std::stoi(e1.name()) < std::stoi(e2.name()); });
        return std::move(vec);
    }
};

using array = basic_array<std::vector<char>>;

namespace detail {

template <typename T, typename Container>
struct is_valid_func<T, Container,
                     std::enable_if_t<std::is_convertible<typename std::decay<T>::type, document>::value>> {
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

template <typename T>
struct set_impl<T, std::enable_if_t<std::is_convertible<typename std::decay<T>::type, document>::value>> {
    template <typename Container> static void call(Container& data, T&& val) {
        using doc_type = basic_document<std::vector<char>, std::vector<char>>;
        static_assert(std::is_convertible<T, doc_type>::value, "");
        data = doc_type(std::forward<T>(val)).data();
    }
};

} // namespace detail
} // namespace jbson

#endif // JBSON_DOCUMENT_HPP
