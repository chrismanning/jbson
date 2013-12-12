//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_BUILDER_HPP
#define JBSON_BUILDER_HPP

#include <deque>

#include <jbson/element.hpp>

namespace jbson {

struct doc_builder {
    template <typename ...Args>
    doc_builder& operator()(Args&&... args) {
        m_elements.emplace_back(std::forward<Args>(args)...);
        return *this;
    }
//private:
    std::deque<element> m_elements;
};

} // namespace jbson

#endif // JBSON_BUILDER_HPP
