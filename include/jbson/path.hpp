//          Copyright Christian Manning 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_PATH_HPP
#define JBSON_PATH_HPP

#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm.hpp>

#include "document.hpp"

namespace jbson {

template <typename Container, typename EContainer, typename StrRngT>
auto path_select(const basic_document<Container, EContainer>& doc, StrRngT&& path_rng);

namespace detail {

template <typename ElemRangeT, typename StrRngT, typename OutIterator>
void select(ElemRangeT&& doc, StrRngT path, OutIterator out);

template <typename ElemRangeT, typename StrRngT, typename OutIterator>
void select_name(ElemRangeT&& doc, StrRngT path, StrRngT name, OutIterator out) {
    if(name.empty())
        return;

    using namespace boost;

    if(range::equal(name, as_literal("*")) || range::equal(name, as_literal(".."))) {
        if(!path.empty()) { // descend
            for(auto&& e : doc) {
                if(e.type() == element_type::document_element)
                    select(get<element_type::document_element>(e), path, out);
                else if(e.type() == element_type::array_element)
                    select(get<element_type::array_element>(e), path, out);
            }
        } else
            out = range::copy(doc, out);
    }

    auto doc_it = range::find_if(doc, [&](auto&& e) { return range::equal(name, e.name()); });

    if(doc_it != doc.end()) {
        if(!path.empty()) { // descend
            if(doc_it->type() == element_type::document_element)
                select(get<element_type::document_element>(*doc_it), path, out);
            else if(doc_it->type() == element_type::array_element)
                select(get<element_type::array_element>(*doc_it), path, out);
        } else
            *out++ = *doc_it;
    } else
        return;
}

template <typename ElemRangeT, typename StrRngT, typename OutIterator>
void select_sub(ElemRangeT&& doc, StrRngT path, StrRngT subscript, OutIterator out) {
    using namespace boost;

    path.advance_begin(subscript.size() + 2);
    if(!subscript.empty() && subscript.back() == ']')
        throw "error";

    std::vector<typename std::decay_t<ElemRangeT>::element_type> vec;
    while(!subscript.empty()) {
        StrRngT elem_name;
        if(subscript.front() == '"') {
            subscript.pop_front();
            elem_name = range::find<return_begin_found>(subscript, '"');
            subscript.advance_begin(elem_name.size() + 1);
        } else if(::isdigit(subscript.front())) {
            elem_name = range::find<return_begin_found>(subscript, ',');
            subscript.advance_begin(elem_name.size());
        } else if(range::binary_search("(?", subscript.front())) {
            elem_name = range::find<return_begin_found>(subscript, ')');
            subscript.advance_begin(elem_name.size() + 1);
        } else if(subscript.front() == '*') {
            elem_name = {subscript.begin(), std::next(subscript.begin())};
            subscript.pop_front();
        } else
            throw "error";

        select_name(std::forward<ElemRangeT>(doc), path, elem_name, std::back_inserter(vec));

        if(!subscript.empty() && range::binary_search(",]", subscript.front()))
            subscript.pop_front();
    }
    out = range::unique_copy(vec, out);
}

template <typename ElemRangeT, typename StrRngT, typename OutIterator>
void select(ElemRangeT&& doc, StrRngT path, OutIterator out) {
    static_assert(is_document<std::decay_t<ElemRangeT>>::value, "");
    static_assert(is_iterator_range<std::decay_t<StrRngT>>::value, "");
    using namespace boost;

    if(path.empty()) {
        out = range::copy(doc, out);
        return;
    }

    StrRngT elem_name;
    if(!starts_with(path, as_literal("..")))
        path = range::find_if<boost::return_found_end>(path, [](auto c) { return c != '.'; });

    if(path.front() == '[') {
        elem_name = range::find<return_begin_found>(path, ']');
        elem_name.pop_front();
        select_sub(std::forward<ElemRangeT>(doc), path, elem_name, out);
    } else {
        if(starts_with(path, as_literal(".."))) {
            elem_name = {path.begin(), std::next(path.begin(), 2)};
            select_name(std::forward<ElemRangeT>(doc), path, elem_name, out);
            path.advance_begin(2);
        }
        elem_name = range::find_if<return_begin_found>(path, is_any_of(".["));
        path.advance_begin(elem_name.size());
        if(!path.empty() && path.front() == '.' && !starts_with(path, as_literal("..")))
            path.pop_front();
        select_name(std::forward<ElemRangeT>(doc), path, elem_name, out);
    }
}

} // namespace detail

template <typename Container, typename EContainer, typename StrRngT>
auto path_select(const basic_document<Container, EContainer>& doc, StrRngT&& path_rng) {
    auto path = boost::as_literal(path_rng);
    std::vector<basic_element<EContainer>> vec;

    path = boost::range::find_if<boost::return_found_end>(path, [](auto c) { return c != '$'; });

    detail::select(doc, path, std::back_inserter(vec));

    return std::move(vec);
}

template <typename Container, typename EContainer, typename StrRngT>
auto path_select(basic_document<Container, EContainer>&& doc, StrRngT&& path_rng) {
    auto path = boost::as_literal(path_rng);
    std::vector<basic_element<Container>> vec;

    path = boost::range::find_if<boost::return_found_end>(path, [](auto c) { return c != '$'; });

    detail::select(doc, path, std::back_inserter(vec));

    return std::move(vec);
}

} // namespace jbson

#endif // JBSON_PATH_HPP
