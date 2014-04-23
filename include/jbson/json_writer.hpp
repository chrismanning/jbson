//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_JSON_WRITER_HPP
#define JBSON_JSON_WRITER_HPP

#include <string>
#include <iterator>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdocumentation"
#include <boost/utility/string_ref.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/concept_check.hpp>
#pragma GCC diagnostic pop

#include "element.hpp"
#include "document.hpp"
#include "builder.hpp"
#include "detail/visit.hpp"

JBSON_PUSH_DISABLE_DEPRECATED_WARNING

namespace jbson {

struct json_writer;

template <typename OutputIterator, typename Container>
std::decay_t<OutputIterator> write_json(const basic_array<Container>&, OutputIterator);
template <typename OutputIterator, typename Container>
std::decay_t<OutputIterator> write_json(const basic_document<Container>&, OutputIterator);

namespace detail {

template <element_type EType, typename Element, typename OutputIterator> struct json_element_visitor;

namespace {

template <typename T, typename OutputIterator>
std::decay_t<OutputIterator> stringify(T&& v, OutputIterator out,
                                       std::enable_if_t<std::is_integral<std::decay_t<T>>::value>* = nullptr) {
    if(std::is_same<T, bool>::value)
        return boost::range::copy(boost::as_literal(!!v ? "true" : "false"), out);
    std::array<char, std::numeric_limits<T>::digits10 + 2> int_str;
    auto n = std::snprintf(int_str.data(), int_str.size(), "%zd", static_cast<ptrdiff_t>(v));
    assert(n > 0);
    assert(static_cast<size_t>(n) <= int_str.size());
    return boost::range::copy(boost::string_ref{int_str.data(), static_cast<size_t>(n)}, out);
}

template <typename T, typename OutputIterator>
std::decay_t<OutputIterator> stringify(T&& v, OutputIterator out,
                                       std::enable_if_t<std::is_floating_point<std::decay_t<T>>::value>* = nullptr) {
    std::array<char, std::numeric_limits<T>::digits10 + 2> float_str;
    auto n = std::snprintf(float_str.data(), float_str.size(), "%.8g", v);
    assert(n > 0);
    assert(static_cast<size_t>(n) <= float_str.size());
    out = boost::range::copy(boost::string_ref{float_str.data(), static_cast<size_t>(n)}, out);
    return out;
}

template <typename OutputIterator> std::decay_t<OutputIterator> stringify(boost::string_ref v, OutputIterator out) {
    auto str = std::vector<char>(v.begin(), v.end());
    for(auto i = str.begin(); i != str.end(); ++i) {
        switch(*i) {
            case '"':
                i = ++str.insert(i, '\\');
                break;
            case '\\':
                i = ++str.insert(i, '\\');
                break;
            case '/':
                i = ++str.insert(i, '\\');
                break;
            case '\b':
                i = ++str.insert(i, '\\');
                *i = 'b';
                break;
            case '\f':
                i = ++str.insert(i, '\\');
                *i = 'f';
                break;
            case '\n':
                i = ++str.insert(i, '\\');
                *i = 'n';
                break;
            case '\r':
                i = ++str.insert(i, '\\');
                *i = 'r';
                break;
            case '\t':
                i = ++str.insert(i, '\\');
                *i = 't';
                break;
            default:
                if(::iscntrl(*i)) {
                    std::array<char, 7> hex;
                    auto n = std::snprintf(hex.data(), hex.size(), "\\u%04x", std::char_traits<char>::to_int_type(*i));
                    assert(n == 6);
                    i = str.erase(i);
                    i = str.insert(i, hex.begin(), std::next(hex.begin(), n));
                }
                break;
        }
    }
    *out++ = '"';
    out = boost::range::copy(str, out);
    *out++ = '"';
    return out;
}

template <typename C, typename EC, typename OutputIterator>
std::decay_t<OutputIterator> stringify(const basic_document<C, EC>& doc, OutputIterator out) {
    out = boost::range::copy(boost::as_literal("{ "), out);

    auto end = doc.end();
    for(auto it = doc.begin(); it != end;) {
        out = stringify(it->name(), out);
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace concepts {
#include <boost/concept/detail/concept_def.hpp>
BOOST_concept(OutputIterator, (TT)(ValueT)) {
    BOOST_CONCEPT_USAGE(OutputIterator) {
        ++i;      // require preincrement operator
        i++;      // require postincrement operator
        *i++ = t; // require postincrement and assignment
    }

  private:
    TT i{std::declval<TT>()}, j{std::declval<TT>()};
    ValueT t = ValueT();
};
#include <boost/concept/detail/concept_undef.hpp>
} // namespace concepts

#endif // DOXYGEN_SHOULD_SKIP_THIS

template <element_type EType, typename Element, typename OutputIteratorT> struct json_element_visitor {
    static_assert(detail::is_element<std::decay_t<Element>>::value, "");
    BOOST_CONCEPT_ASSERT((concepts::OutputIteratorConcept<OutputIteratorT, char>));

    json_element_visitor() = default;

    std::decay_t<OutputIteratorT> operator()(Element&& e, OutputIteratorT out) const {
        return stringify(get<EType>(e), out);
    }
};

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// oid
template <typename Element, typename OutputIterator>
struct json_element_visitor<element_type::oid_element, Element, OutputIterator> {
    std::decay_t<OutputIterator> operator()(Element&& e, std::decay_t<OutputIterator> out) const {
        auto oid = get<element_type::oid_element>(e);
        std::array<char, 25> buf;
        for(size_t i = 0; i < oid.size(); ++i)
            std::snprintf(&buf[i * 2], 3, "%0.2x", static_cast<unsigned char>(oid[i]));
        return stringify(
            static_cast<document>(builder("$oid", element_type::string_element, boost::string_ref(buf.data(), 24))),
            out);
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
        return stringify(static_cast<document>(builder("$date", e.template value<int64_t>())), out);
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

#endif // DOXYGEN_SHOULD_SKIP_THIS

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

JBSON_POP_WARNINGS

#endif // JBSON_JSON_WRITER_HPP
