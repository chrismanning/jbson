//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_JSON_WRITER_HPP
#define JBSON_JSON_WRITER_HPP

#include <string>
#include <iterator>

#include <boost/utility/string_ref.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/concept_check.hpp>

#include "element.hpp"
#include "document.hpp"
#include "detail/visit.hpp"

namespace jbson {

struct json_writer;

template <typename OutputIterator, typename Container>
std::decay_t<OutputIterator> write_json(OutputIterator, const basic_array<Container>&);
template <typename OutputIterator, typename Container>
std::decay_t<OutputIterator> write_json(OutputIterator, const basic_document<Container>&);

namespace detail {

template <element_type EType, typename Element, typename OutputIterator> struct json_element_visitor;

namespace {

template <typename T, typename OutputIterator>
std::decay_t<OutputIterator> stringify(T&& v, OutputIterator out,
                                       std::enable_if_t<std::is_arithmetic<std::decay_t<T>>::value>* = nullptr) {
    if(std::is_same<std::decay_t<T>, bool>::value)
        out = boost::range::copy(boost::as_literal(!!v ? "true" : "false"), out);
    else
        out = boost::range::copy(std::to_string(v), out);
    return out;
}

template <typename OutputIterator> std::decay_t<OutputIterator> stringify(boost::string_ref v, OutputIterator out) {
    *out++ = '"';
    out = boost::range::copy(v, out);
    *out++ = '"';
    return out;
}

template <typename C, typename EC, typename OutputIterator>
std::decay_t<OutputIterator> stringify(const basic_document<C, EC>& doc, OutputIterator out) {
    out = boost::range::copy(boost::as_literal("{ "), out);

    auto end = doc.end();
    for(auto it = doc.begin(); it != end;) {
        *out++ = '"';
        out = boost::range::copy(it->name(), out);
        *out++ = '"';
        out = boost::range::copy(boost::as_literal(" : "), out);
        out = detail::visit<detail::json_element_visitor>(it->type(), *it, out);
        if(++it != end)
            out = boost::range::copy(boost::as_literal(", "), out);
    }

    return boost::range::copy(boost::as_literal(" }"), out);
}

template <typename C, typename EC, typename OutputIterator>
std::decay_t<OutputIterator> stringify(const basic_array<C, EC>& arr, OutputIterator out) {
    out = boost::range::copy(boost::as_literal("[ "), out);

    auto end = arr.end();
    for(auto it = arr.begin(); it != end;) {
        out = detail::visit<detail::json_element_visitor>(it->type(), *it, out);
        if(++it != end)
            out = boost::range::copy(boost::as_literal(", "), out);
    }

    return boost::range::copy(boost::as_literal(" ]"), out);
}

} // namespace

template <element_type EType, typename Element, typename OutputIterator> struct json_element_visitor {
    static_assert(detail::is_element<std::decay_t<Element>>::value, "");
    BOOST_CONCEPT_ASSERT((boost::OutputIterator<OutputIterator, char>));

    json_element_visitor() = default;

    std::decay_t<OutputIterator> operator()(Element&& e, OutputIterator out) const {
        return stringify(get<EType>(e), out);
    }
};

// oid
template <typename Element, typename OutputIterator>
struct json_element_visitor<element_type::oid_element, Element, OutputIterator> {
    std::decay_t<OutputIterator> operator()(Element&& e, std::decay_t<OutputIterator> out) const {
        auto oid = get<element_type::oid_element>(e);
        std::array<char, 25> buf;
        for(size_t i = 0; i < oid.size(); ++i)
            std::snprintf(&buf[i*2], 3, "%x", static_cast<unsigned char>(oid[i]));
        return stringify(static_cast<document>(builder("$oid", element_type::string_element,
                                                       boost::string_ref(buf.data(), 24))), out);
    }
};

// db pointer
template <typename Element, typename OutputIterator>
struct json_element_visitor<element_type::db_pointer_element, Element, OutputIterator> {
    std::decay_t<OutputIterator> operator()(Element&& e, std::decay_t<OutputIterator> out) const {
        using string_type = decltype(get<element_type::string_element>(e));
        string_type ref;
        using oid_type = decltype(get<element_type::oid_element>(e));
        oid_type oid;
        std::tie(ref, oid) = get<element_type::db_pointer_element>(e);
        return stringify(static_cast<document>(
                             builder("$ref", element_type::string_element, ref)("$id", element_type::oid_element, oid)),
                         out);
    }
};

// date
template <typename Element, typename OutputIterator>
struct json_element_visitor<element_type::date_element, Element, OutputIterator> {
    std::decay_t<OutputIterator> operator()(Element&& e, std::decay_t<OutputIterator> out) const {
        auto date = get<element_type::date_element>(e);
        return stringify(date.time_since_epoch().count(), out);
    }
};

// regex
template <typename Element, typename OutputIterator>
struct json_element_visitor<element_type::regex_element, Element, OutputIterator> {
    std::decay_t<OutputIterator> operator()(Element&& e, std::decay_t<OutputIterator> out) const {
        using string_type = decltype(get<element_type::string_element>(e));
        string_type regex, options;
        std::tie(regex, options) = get<element_type::regex_element>(e);
        return stringify(static_cast<document>(builder("$regex", element_type::string_element,
                                                       regex)("$options", element_type::string_element, options)),
                         out);
    }
};

// scoped code
template <typename Element, typename OutputIterator>
struct json_element_visitor<element_type::scoped_javascript_element, Element, OutputIterator> {
    std::decay_t<OutputIterator> operator()(Element&& e, std::decay_t<OutputIterator> out) const { return out; }
};

// voids
template <typename Element, typename OutputIterator>
struct json_element_visitor<element_type::null_element, Element, OutputIterator> {
    std::decay_t<OutputIterator> operator()(Element&& e, std::decay_t<OutputIterator> out) const {
        return boost::range::copy(boost::as_literal("null"), out);
    }
};

template <typename Element, typename OutputIterator>
struct json_element_visitor<element_type::undefined_element, Element, OutputIterator> {
    std::decay_t<OutputIterator> operator()(Element&& e, std::decay_t<OutputIterator> out) const {
        return boost::range::copy(boost::as_literal("null"), out);
    }
};

template <typename Element, typename OutputIterator>
struct json_element_visitor<element_type::max_key, Element, OutputIterator> {
    std::decay_t<OutputIterator> operator()(Element&& e, std::decay_t<OutputIterator> out) const {
        return boost::range::copy(boost::as_literal("null"), out);
    }
};

template <typename Element, typename OutputIterator>
struct json_element_visitor<element_type::min_key, Element, OutputIterator> {
    std::decay_t<OutputIterator> operator()(Element&& e, std::decay_t<OutputIterator> out) const {
        return boost::range::copy(boost::as_literal("null"), out);
    }
};

} // namespace detail

template <typename OutputIterator, typename Container>
std::decay_t<OutputIterator> write_json(const basic_array<Container>& arr, OutputIterator out) {
    return detail::stringify(arr, out);
}

template <typename OutputIterator, typename Container>
std::decay_t<OutputIterator> write_json(const basic_document<Container>& doc, OutputIterator out) {
    return detail::stringify(doc, out);
}

} // namespace jbson

#endif // JBSON_JSON_WRITER_HPP
