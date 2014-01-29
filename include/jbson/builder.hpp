//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_BUILDER_HPP
#define JBSON_BUILDER_HPP

#include <deque>

#include <jbson/element.hpp>
#include <jbson/document.hpp>

namespace jbson {

struct builder {
    builder() = default;

    builder(const builder&) = default;
    builder& operator=(const builder&) = default;

    builder(builder&&) = default;
    builder& operator=(builder&&) = default;

    template <typename... Args> builder(Args&&... args) { emplace(std::forward<Args>(args)...); }

    template <typename... Args> builder& operator()(Args&&... args) {
        emplace(std::forward<Args>(args)...);
        return *this;
    }

    template <typename... Args> builder& emplace(Args&&... args) {
        m_elements.emplace(std::forward<Args>(args)...);
        return *this;
    }

    template <typename Container, typename EContainer> operator basic_document<Container, EContainer>() const {
        return basic_document<Container, EContainer>(m_elements);
    }

  private:
    document_set m_elements;
};
static_assert(std::is_convertible<builder, document>::value, "");
static_assert(std::is_convertible<builder, basic_document<std::vector<char>, std::vector<char>>>::value, "");

struct array_builder {
    array_builder() = default;

    array_builder(const array_builder&) = default;
    array_builder& operator=(const array_builder&) = default;

    array_builder(array_builder&&) = default;
    array_builder& operator=(array_builder&&) = default;

    template <typename... Args> array_builder(Args&&... args) { emplace(std::forward<Args>(args)...); }

    template <typename... Args> array_builder& operator()(Args&&... args) {
        emplace(std::forward<Args>(args)...);
        return *this;
    }

    template <typename... Args> array_builder& emplace(Args&&... args) {
        m_elements.emplace_back(std::to_string(m_elements.size()), std::forward<Args>(args)...);
        return *this;
    }

    template <typename... Args> array_builder& emplace(size_t idx, Args&&... args) {
        m_elements.emplace_back(std::to_string(idx), std::forward<Args>(args)...);
        return *this;
    }

    template <typename Container, typename EContainer> operator basic_array<Container, EContainer>() const {
        auto fun = [](auto&& a, auto&& b) {
#ifdef _GNU_SOURCE
            // natural sort
            return strverscmp(a.name().data(), b.name().data());
#else
            return a.name().compare(b.name());
#endif
        };
        std::sort(m_elements.begin(), m_elements.end(), [fun](auto&& a, auto&& b) { return fun(a, b) < 0; });
        assert(std::unique(m_elements.begin(), m_elements.end(),
                           [fun](auto&& a, auto&& b) { return fun(a, b) == 0; }) == m_elements.end());
        return basic_array<Container, EContainer>(m_elements);
    }

  private:
    mutable std::deque<element> m_elements;
};
static_assert(std::is_convertible<array_builder, array>::value, "");
static_assert(std::is_convertible<array_builder, basic_array<std::vector<char>, std::vector<char>>>::value, "");

} // namespace jbson

#endif // JBSON_BUILDER_HPP
