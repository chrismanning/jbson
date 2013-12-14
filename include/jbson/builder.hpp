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
        m_elements.emplace_back(name, type);
    }

    template <typename T>
    doc_builder(const std::string& name, element_type type, T&& val) {
        m_elements.emplace_back(name, type, std::forward<T>(val));
    }

    doc_builder& operator()(const std::string& name, element_type type) {
        m_elements.emplace_back(name, type);
        return *this;
    }

    template <typename T>
    doc_builder& operator()(const std::string& name, element_type type, T&& val) {
        m_elements.emplace_back(name, type, std::forward<T>(val));
        return *this;
    }

    template <typename Container>
    operator basic_document<Container>() const {
        auto raw_data = Container(4, '\0');
        for(auto&& e : m_elements) {
            boost::range::push_back(raw_data, static_cast<std::vector<char>>(e));
        }
        raw_data.push_back('\0');
        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(raw_data.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, raw_data.begin());
        return basic_document<Container>{raw_data};
    }

    template <typename Container>
    operator basic_array<Container>() const {
        return basic_array<Container>(*this);
    }

private:
    std::deque<element> m_elements;
};

} // namespace jbson

#endif // JBSON_BUILDER_HPP
