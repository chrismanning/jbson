//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_DOCUMENT_HPP
#define JBSON_DOCUMENT_HPP

#include <memory>
#include <vector>

#include <boost/iterator/iterator_adaptor.hpp>

#include <jbson/element.hpp>

namespace jbson {

struct invalid_document_size : jbson_error {
    const char* what() const noexcept override { return "invalid_document_size"; }
};

namespace detail {

template <typename Value, typename BaseIterator>
struct document_iter
    : boost::iterator_facade<document_iter<Value, BaseIterator>, Value, boost::forward_traversal_tag, Value> {
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

    Value dereference() const { return *m_cur; }

    BaseIterator m_start;
    BaseIterator m_end;
    boost::optional<typename std::decay<Value>::type> m_cur;
};

} // namespace detail

template <class Container, class ElementContainer = Container> class basic_document {
    template <class, class> friend struct detail::document_iter;

  public:
    using container_type = Container;
    using element_type = basic_element<ElementContainer>;
    using iterator = typename detail::document_iter<element_type, typename container_type::iterator>;
    using const_iterator = typename detail::document_iter<const element_type, typename container_type::const_iterator>;

    basic_document() = default;

    basic_document(const basic_document&) = default;
    basic_document& operator=(const basic_document&) = default;

    basic_document(basic_document&&) = default;
    basic_document& operator=(basic_document&&) = default;

    template <typename ForwardRange>
    explicit basic_document(const ForwardRange& rng)
        : basic_document(std::begin(rng), std::end(rng)) {}

    template <typename ForwardIterator>
    basic_document(ForwardIterator first, ForwardIterator last)
        : m_data(first, last) {
        if(m_data.size() > 4)
            m_size = detail::little_endian_to_native<int32_t>(m_data.begin(), m_data.end());
        if(m_data.size() != m_size)
            BOOST_THROW_EXCEPTION(invalid_document_size{});
    }

    iterator begin() {
        if(m_data.size() <= sizeof(int32_t))
            BOOST_THROW_EXCEPTION(invalid_document_size{});
        return {std::next(m_data.begin(), sizeof(int32_t)), std::prev(m_data.end())};
    }
    const_iterator begin() const {
        if(m_data.size() <= sizeof(int32_t))
            BOOST_THROW_EXCEPTION(invalid_document_size{});
        return {std::next(m_data.begin(), sizeof(int32_t)), std::prev(m_data.end())};
    }

    iterator end() {
        auto last = std::prev(m_data.end());
        return {last, last};
    }
    const_iterator end() const {
        auto last = std::prev(m_data.end());
        return {last, last};
    }

    int32_t size() const { return m_size; }

  private:
    container_type m_data;
    mutable int32_t m_size{0};
};

using document = basic_document<std::vector<char>>;

} // namespace jbson

#endif // JBSON_DOCUMENT_HPP
