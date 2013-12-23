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

    builder(const std::string& name, element_type type) {
        emplace(name, type);
    }

    template <typename T>
    builder(const std::string& name, element_type type, T&& val) {
        emplace(name, type, std::forward<T>(val));
    }

    builder& operator()(const std::string& name, element_type type) {
        m_elements.emplace(name, type);
        return *this;
    }

    template <typename T>
    builder& operator()(const std::string& name, element_type type, T&& val) {
        m_elements.emplace(name, type, std::forward<T>(val));
        return *this;
    }

    template <typename Container>
    builder& operator()(basic_element<Container>&& e) {
        m_elements.emplace(std::forward<basic_element<Container>>(e));
        return *this;
    }

    template <typename Container>
    builder& operator()(const basic_element<Container>& e) {
        m_elements.emplace(e);
        return *this;
    }

    builder& emplace(const std::string& name, element_type type) {
        m_elements.emplace(name, type);
        return *this;
    }

    template <typename T>
    builder& emplace(const std::string& name, element_type type, T&& val) {
        m_elements.emplace(name, type, std::forward<T>(val));
        return *this;
    }

    template <typename Container>
    builder& emplace(basic_element<Container>&& e) {
        m_elements.emplace(std::forward<basic_element<Container>>(e));
        return *this;
    }

    template <typename Container>
    builder& emplace(const basic_element<Container>& e) {
        m_elements.emplace(e);
        return *this;
    }

    template <typename Container>
    operator basic_document<Container>() const {
        return basic_document<Container>{m_elements};
    }

    template <typename Container>
    operator basic_array<Container>() const {
        return basic_array<Container>{m_elements};
    }

private:
    document_set m_elements;
};

} // namespace jbson

#endif // JBSON_BUILDER_HPP
