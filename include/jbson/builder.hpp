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

    template <typename Container, typename EContainer,
              typename = std::enable_if_t<
                  detail::has_member_function_push_back<Container, void, boost::mpl::vector<const char&>>::value>>
    operator basic_document<Container, EContainer>() const {
        return basic_document<Container, EContainer>(m_elements);
    }

    template <typename Container, typename EContainer,
              typename = std::enable_if_t<
                  detail::has_member_function_push_back<Container, void, boost::mpl::vector<const char&>>::value>>
    operator basic_array<Container, EContainer>() const {
        return basic_array<Container, EContainer>(m_elements);
    }

  private:
    basic_document_set<std::deque<char>> m_elements;
};
static_assert(std::is_convertible<builder, document>::value, "");
static_assert(std::is_convertible<builder, basic_document<std::vector<char>, std::vector<char>>>::value, "");
static_assert(!std::is_convertible<builder, basic_document<boost::iterator_range<const char*>>>::value, "");

} // namespace jbson

#endif // JBSON_BUILDER_HPP
