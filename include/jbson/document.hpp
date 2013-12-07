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

namespace detail {

template <typename Value, typename BaseIterator>
struct document_iter
    : boost::iterator_facade<document_iter<Value, BaseIterator>, Value, boost::forward_traversal_tag, Value> {
    document_iter() = default;

    document_iter(BaseIterator it1, BaseIterator it2) : m_start(it1), m_end(it2) { reset_value(); }

    template <class OtherValue, typename OtherIt>
    document_iter(const document_iter<OtherValue, OtherIt>& other,
                  typename std::enable_if<std::is_convertible<OtherValue, Value>::value&&
                                              std::is_convertible<OtherIt, BaseIterator>::value>::type* = nullptr)
        : m_start(other.m_start), m_end(other.m_end), m_cur(other.m_cur) {}

  private:
    friend class boost::iterator_core_access;
    template <typename, typename> friend struct document_iter;

    template <typename OtherValue, typename OtherIterator>
    bool equal(const document_iter<OtherValue, OtherIterator>& other) const {
        return other.m_start == m_start;
    }

    void increment() {
        std::advance(m_start, m_cur.size() - m_cur.name().size() - sizeof(int32_t) - sizeof('\0'));
        reset_value();
    }

    Value dereference() const { return m_cur; }

    void reset_value() {
        if(m_start == m_end)
            return;
        auto val = *m_start;
        if(val == 0) {
            m_start = m_end;
            return;
        }
        ++m_start;
        auto type = static_cast<element_type>(val);
        auto name = std::string{m_start, std::find(m_start, m_end, '\0')};
        std::advance(m_start, name.size() + 1);

        m_cur = {name, type, m_start, m_end};
    }

    BaseIterator m_start;
    const BaseIterator m_end;
    mutable typename std::decay<Value>::type m_cur;
};
} // namespace detail

template <class Container, class ElementContainer = Container> class basic_document {
    template <class, class> friend struct detail::document_iter;
    using container_type = Container;
    using allocator_type = typename container_type::allocator_type;

  public:
    using element_type = basic_element<ElementContainer>;
    using iterator = typename detail::document_iter<element_type, typename container_type::iterator>;
    using const_iterator = typename detail::document_iter<const element_type, typename container_type::const_iterator>;

    basic_document() = default;

    basic_document(const basic_document&) = default;
    basic_document& operator=(const basic_document&) = default;

    basic_document(basic_document&&) = default;
    basic_document& operator=(basic_document&&) = default;

    template <typename ForwardRange>
    basic_document(const ForwardRange& rng)
        : basic_document(std::begin(rng), std::end(rng)) {}

    template <typename ForwardIterator>
    basic_document(ForwardIterator first, ForwardIterator last)
        : m_data(first, last) {
        if(m_data.size() > 4)
            m_size = detail::little_endian_to_native<int32_t>(m_data.begin(), m_data.end());
    }

    iterator begin() {
        assert(m_data.size() > sizeof(int32_t));
        return {std::next(m_data.begin(), sizeof(int32_t)), std::prev(m_data.end())};
    }
    const_iterator begin() const {
        assert(m_data.size() > sizeof(int32_t));
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
