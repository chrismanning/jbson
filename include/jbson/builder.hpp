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

struct doc_builder {
    doc_builder() = default;

    doc_builder(const doc_builder&) = default;
    doc_builder& operator=(const doc_builder&) = default;

    doc_builder(doc_builder&&) = default;
    doc_builder& operator=(doc_builder&&) = default;

    doc_builder(const std::string& name, element_type type) {
        emplace(name, type);
    }

    template <typename T>
    doc_builder(const std::string& name, element_type type, T&& val) {
        emplace(name, type, std::forward<T>(val));
    }

    doc_builder& operator()(const std::string& name, element_type type) {
        m_elements.emplace(name, type);
        return *this;
    }

    template <typename T>
    doc_builder& operator()(const std::string& name, element_type type, T&& val) {
        m_elements.emplace(name, type, std::forward<T>(val));
        return *this;
    }

    doc_builder& emplace(const std::string& name, element_type type) {
        m_elements.emplace(name, type);
        return *this;
    }

    template <typename T>
    doc_builder& emplace(const std::string& name, element_type type, T&& val) {
        m_elements.emplace(name, type, std::forward<T>(val));
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
