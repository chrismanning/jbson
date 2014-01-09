//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_JSON_READER_HPP
#define JBSON_JSON_READER_HPP

#include <type_traits>
#include <iterator>
#include <memory>
#include <codecvt>

#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/exception/exception.hpp>
#include <boost/spirit/home/support/iterators/line_pos_iterator.hpp>

#include "element.hpp"
#include "builder.hpp"
#include "detail/traits.hpp"

namespace jbson {

using boost::spirit::line_pos_iterator;

struct json_parse_error;
enum class json_error_num;

struct json_reader {
    json_reader() = default;

    using container_type = std::vector<char>;
    using range_type = boost::iterator_range<container_type::const_iterator>;

    json_reader(const json_reader&) = delete;
    json_reader& operator=(const json_reader&) = delete;

    json_reader(json_reader&&) = default;
    json_reader& operator=(json_reader&&) = default;

    ~json_reader() {}

    template <typename ForwardIterator> void parse(ForwardIterator, ForwardIterator);
    template <typename ForwardIterator>
    void parse(line_pos_iterator<ForwardIterator>, line_pos_iterator<ForwardIterator>);

    template <typename ForwardRange_> void parse(ForwardRange_&& range_) {
        auto range = boost::as_literal(std::forward<ForwardRange_>(range_));
        using ForwardRange = decltype(range);
        BOOST_CONCEPT_ASSERT((boost::ForwardRangeConcept<ForwardRange>));
        using line_it = line_pos_iterator<typename boost::range_const_iterator<std::decay_t<ForwardRange>>::type>;
        parse(line_it{std::cbegin(range)}, line_it{std::cend(range)});
    }

    operator basic_document_set<range_type>() const& {
        if(m_data.size() < 5)
            return basic_document_set<range_type>{};
        return basic_document_set<range_type>(basic_document<range_type>(*this));
    }

    operator basic_document<range_type>() const& {
        if(m_data.size() < 5)
            return basic_document<range_type>{};
        return basic_document<range_type>{boost::make_iterator_range(m_data)};
    }

    operator basic_array<range_type>() const& {
        if(m_data.size() < 5)
            return basic_array<range_type>{};
        return basic_array<range_type>{boost::make_iterator_range(m_data)};
    }

    operator document() const& {
        if(m_data.size() < 5)
            return document{};
        return document{m_data};
    }

    operator document() && {
        if(m_data.size() < 5)
            return document{};
        return document{std::move(m_data)};
    }

    operator array() const& {
        if(m_data.size() < 5)
            return array{};
        return array{m_data};
    }

    operator array() && {
        if(m_data.size() < 5)
            return array{};
        return array{std::move(m_data)};
    }

  private:
    template <element_type EType, typename T> using setter = detail::set_impl<EType, element::container_type, T>;

    template <typename ForwardIterator, typename OutputIterator>
    OutputIterator parse_document(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&,
                                  OutputIterator);
    template <typename ForwardIterator, typename OutputIterator>
    OutputIterator parse_array(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&,
                               OutputIterator);

    template <typename ForwardIterator, typename OutputIterator>
    std::tuple<OutputIterator, element_type> parse_value(line_pos_iterator<ForwardIterator>&,
                                                         const line_pos_iterator<ForwardIterator>&, OutputIterator);

    template <typename OutputIterator>
    std::tuple<boost::optional<OutputIterator>, element_type> parse_extended_value(const basic_document<range_type>&,
                                                                                   OutputIterator);

    template <typename ForwardIterator, typename OutputIterator>
    OutputIterator parse_name(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&,
                              OutputIterator);

    template <typename ForwardIterator, typename OutputIterator>
    OutputIterator parse_string(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&,
                                OutputIterator);
    template <typename ForwardIterator, typename OutputIterator>
    std::tuple<OutputIterator, element_type> parse_number(line_pos_iterator<ForwardIterator>&,
                                                          const line_pos_iterator<ForwardIterator>&, OutputIterator);

    template <typename ForwardIterator>
    void skip_space(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&);

    template <typename ForwardIterator>
    json_parse_error make_parse_exception(json_error_num, const line_pos_iterator<ForwardIterator>& current,
                                          const line_pos_iterator<ForwardIterator>& last,
                                          const std::string& expected = {}) const;
    json_parse_error make_parse_exception(json_error_num, const std::string& expected = {}) const;

  private:
    std::shared_ptr<void> m_start;
    container_type m_data;
};

enum class json_error_num { invalid_root_element, unexpected_end_of_range, unexpected_token, };

template <typename CharT, typename TraitsT>
std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& os, json_error_num err) {
    switch(err) {
        case json_error_num::invalid_root_element:
            os << "invalid_root_element";
            break;
        case json_error_num::unexpected_end_of_range:
            os << "unexpected_end_of_range";
            break;
        case json_error_num::unexpected_token:
            os << "unexpected_token";
            break;
        default:
            os << "unknown error";
    }
    return os;
}

using parse_error = boost::error_info<struct err_val_, json_error_num>;
using expected_token = boost::error_info<struct token_, std::string>;
using current_line = boost::error_info<struct line_, std::string>;
using line_pos = boost::error_info<struct line_pos_, size_t>;

struct json_parse_error : jbson_error {
    const char* what() const noexcept override { return "json_parse_error"; }
};

std::string to_string(const json_parse_error&);

template <typename ForwardIterator>
json_parse_error
json_reader::make_parse_exception(json_error_num err, const line_pos_iterator<ForwardIterator>& current,
                                  const line_pos_iterator<ForwardIterator>& last, const std::string& expected) const {
    BOOST_CONCEPT_ASSERT((boost::ForwardIterator<ForwardIterator>));
    auto e = make_parse_exception(err, expected);
    if(m_start) {
        auto start = *static_cast<line_pos_iterator<ForwardIterator>*>(m_start.get());
        auto begin = boost::spirit::get_line_start(start, current);
        if(begin != current && begin != last && (*begin == '\n' || *begin == '\r'))
            std::advance(begin, 1);
        auto range = boost::range::find_first_of<boost::return_begin_found>(boost::make_iterator_range(begin, last),
                                                                            boost::as_literal("\n\r"));

        e << current_line(boost::lexical_cast<std::string>(range));
        e << line_pos(boost::spirit::get_column(begin, current));
    } else
        std::abort();
    return e;
}

json_parse_error json_reader::make_parse_exception(json_error_num err, const std::string& expected) const {
    auto e = json_parse_error{};
    e << parse_error(err);
    if(!expected.empty())
        e << expected_token(expected);
    return e;
}

template <typename ForwardIterator> void json_reader::parse(ForwardIterator first, ForwardIterator last) {
    BOOST_CONCEPT_ASSERT((boost::ForwardIterator<ForwardIterator>));
    static_assert(std::is_same<typename std::iterator_traits<ForwardIterator>::value_type, char>::value, "");
    parse(line_pos_iterator<ForwardIterator>{first}, line_pos_iterator<ForwardIterator>{last});
}

template <typename ForwardIterator>
void json_reader::parse(line_pos_iterator<ForwardIterator> first, line_pos_iterator<ForwardIterator> last) {
    BOOST_CONCEPT_ASSERT((boost::ForwardIterator<ForwardIterator>));
    static_assert(std::is_same<typename std::iterator_traits<ForwardIterator>::value_type, char>::value, "");
    container_type c;
    m_data.swap(c);
    m_data.reserve(+std::distance(&*first, &*last));

    m_start = std::make_shared<line_pos_iterator<ForwardIterator>>(first);
    skip_space(first, last);
    if(first == last || *first == '\0')
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    switch(*first) {
        case '{':
            parse_document(first, last, m_data.end());
            break;
        case '[':
            parse_array(first, last, m_data.end());
            break;
        default:
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::invalid_root_element, first, last));
    };
    m_data.shrink_to_fit();

    skip_space(first, last);
    if(first != last)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "end of input"));
}

template <typename ForwardIterator, typename OutputIterator>
OutputIterator json_reader::parse_document(line_pos_iterator<ForwardIterator>& first,
                                           const line_pos_iterator<ForwardIterator>& last, OutputIterator out) {
    if(first == last)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    assert(last != first);
    assert(&*first != nullptr);

    auto start_idx = std::distance(m_data.begin(), out);

    assert(out >= m_data.begin() && out <= m_data.end());
    out = std::next(m_data.insert(out, 4, '\0'), 4);

    if(*first != '{')
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "{"));
    skip_space(++first, last);
    if(first == last)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    if(*first == '}') {
        ++first;

        assert(out >= m_data.begin() && out <= m_data.end());
        out = std::next(m_data.insert(out, '\0'));

        int32_t size = std::distance(std::next(m_data.begin(), start_idx), out);
        if(size != 5)
            BOOST_THROW_EXCEPTION(invalid_document_size{} << expected_size(5) << actual_size(size));
        boost::range::copy(detail::native_to_little_endian(size), std::next(m_data.begin(), start_idx));
        return out;
    }

    while(true) {
        if(first == last)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));

        assert(out >= m_data.begin() && out <= m_data.end());
        out = m_data.insert(out, static_cast<char>(element_type::null_element));

        auto type_idx = std::distance(m_data.begin(), out);
        ++out;
        skip_space(first, last);
        out = parse_name(first, last, out);
        skip_space(first, last);

        if(*first != ':')
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, ":"));
        ++first;
        skip_space(first, last);
        element_type type;
        std::tie(out, type) = parse_value(first, last, out);
        m_data[type_idx] = static_cast<char>(type);

        skip_space(first, last);
        if(*first == ',') {
            ++first;
            continue;
        }
        if(*first != '}')
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "}"));
        else {
            ++first;

            assert(out >= m_data.begin() && out <= m_data.end());
            out = std::next(m_data.insert(out, '\0'));

            int32_t size = std::distance(std::next(m_data.begin(), start_idx), out);
            if(size < 5)
                BOOST_THROW_EXCEPTION(invalid_document_size{} << expected_size(5) << actual_size(size));
            boost::range::copy(detail::native_to_little_endian(size), std::next(m_data.begin(), start_idx));
            return out;
        }
    }
}

template <typename ForwardIterator, typename OutputIterator>
OutputIterator json_reader::parse_array(line_pos_iterator<ForwardIterator>& first,
                                        const line_pos_iterator<ForwardIterator>& last, OutputIterator out) {
    if(first == last)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    assert(last != first);
    assert(&*first != nullptr);

    auto start_idx = std::distance(m_data.begin(), out);

    assert(out >= m_data.begin() && out <= m_data.end());
    out = std::next(m_data.insert(out, 4, '\0'), 4);

    if(*first != '[')
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "["));
    skip_space(++first, last);
    if(first == last)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    if(*first == ']') {
        ++first;

        assert(out >= m_data.begin() && out <= m_data.end());
        out = std::next(m_data.insert(out, '\0'));

        int32_t size = std::distance(std::next(m_data.begin(), start_idx), out);
        if(size != 5)
            BOOST_THROW_EXCEPTION(invalid_document_size{} << expected_size(5) << actual_size(size));
        boost::range::copy(detail::native_to_little_endian(size), std::next(m_data.begin(), start_idx));
        return out;
    }

    int32_t idx{0};
    while(true) {
        if(first == last)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));

        assert(out >= m_data.begin() && out <= m_data.end());
        out = m_data.insert(out, static_cast<char>(element_type::null_element));
        auto type_idx = std::distance(m_data.begin(), out);
        auto sidx = std::to_string(idx++);
        ++out;

        assert(out >= m_data.begin() && out <= m_data.end());
        out = m_data.insert(out, sidx.size() + 1, '\0');
        out = std::next(boost::range::copy(sidx, out));
        assert(out >= m_data.begin() && out <= m_data.end());

        skip_space(first, last);

        element_type type;
        std::tie(out, type) = parse_value(first, last, out);
        m_data[type_idx] = static_cast<char>(type);

        skip_space(first, last);

        if(*first == ',') {
            ++first;
            continue;
        }
        if(*first != ']')
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, ", or ]"));
        else {
            ++first;

            assert(out >= m_data.begin() && out <= m_data.end());
            out = std::next(m_data.insert(out, '\0'));

            int32_t size = std::distance(std::next(m_data.begin(), start_idx), out);
            if(size < 5)
                BOOST_THROW_EXCEPTION(invalid_document_size{} << expected_size(5) << actual_size(size));
            boost::range::copy(detail::native_to_little_endian(size), std::next(m_data.begin(), start_idx));
            return out;
        }
    }
}

template <typename ForwardIterator, typename OutputIterator>
std::tuple<OutputIterator, element_type> json_reader::parse_value(line_pos_iterator<ForwardIterator>& first,
                                                                  const line_pos_iterator<ForwardIterator>& last,
                                                                  OutputIterator out) {
    if(first == last)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    assert(last != first);
    assert(&*first != nullptr);
    auto type = element_type::null_element;
    switch(*first) {
        case '"':
            type = element_type::string_element;
            out = parse_string(first, last, out);
            break;
        case '[':
            type = element_type::array_element;
            out = parse_array(first, last, out);
            break;
        case 'f':
            type = element_type::boolean_element;
            if(boost::equal(boost::as_literal("false"), boost::make_iterator_range(first, std::next(first, 5)))) {
                assert(out >= m_data.begin() && out <= m_data.end());
                out = std::next(m_data.insert(out, false));
                std::advance(first, 5);
                break;
            }
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "false"));
        case 'n':
            type = element_type::null_element;
            if(!boost::equal(boost::as_literal("null"), boost::make_iterator_range(first, std::next(first, 4))))
                BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "null"));
            std::advance(first, 4);
            break;
        case 't':
            type = element_type::boolean_element;
            if(boost::equal(boost::as_literal("true"), boost::make_iterator_range(first, std::next(first, 4)))) {
                assert(out >= m_data.begin() && out <= m_data.end());
                out = std::next(m_data.insert(out, true));
                std::advance(first, 4);
                break;
            }
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "true"));
        case '{': {
            auto idx = std::distance(m_data.begin(), out);
            auto r = parse_document(first, last, out);

            basic_document<range_type> doc{std::next(m_data.begin(), idx), r};
            assert(std::distance(std::next(m_data.begin(), idx), r) >= 5);
            assert(doc.size() >= 5);

            boost::string_ref name;
            auto it = doc.begin();
            if(it != doc.end()) {
                name = it->name();
            }
            if(!name.empty() && name[0] == '$') {
                boost::optional<OutputIterator> o_out;
                std::tie(o_out, type) = parse_extended_value(doc, std::next(m_data.begin(), idx));
                if(o_out) {
                    out = *o_out;
                    m_data.resize(std::distance(m_data.begin(), out));
                    break;
                }
            }
            type = element_type::document_element;
            out = r;
        } break;
        default:
            std::tie(out, type) = parse_number(first, last, out);
    }
    if(first == last)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    return std::make_tuple(out, type);
}

template <typename OutputIterator>
std::tuple<boost::optional<OutputIterator>, element_type>
json_reader::parse_extended_value(const basic_document<range_type>& doc, OutputIterator out) {
    if(doc.size() < 5)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "extended json value")
                              << expected_size(5) << actual_size(doc.size()));
    auto type = element_type::null_element;
    auto name = doc.begin()->name();
    if(name == "$binary" || name == "$type") {
        if(doc.size() != 2)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "binary element"));
        type = element_type::binary_element;
        // TODO: implement binary value
    } else if(name == "$date") {
        if(doc.size() != 1 ||
           (doc.begin()->type() != element_type::int32_element && doc.begin()->type() != element_type::int64_element))
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "date element"));
        using DateT = detail::ElementTypeMap<element_type::date_element, element::container_type>;
        if(doc.begin()->type() == element_type::int32_element)
            out = setter<element_type::date_element, DateT>::call(
                out, DateT{std::chrono::milliseconds{get<element_type::int32_element>(*doc.begin())}});
        else if(doc.begin()->type() == element_type::int64_element)
            out = setter<element_type::date_element, DateT>::call(
                out, DateT{std::chrono::milliseconds{get<element_type::int64_element>(*doc.begin())}});
        else
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "date element"));
        type = element_type::date_element;
    } else if(name == "$timestamp") {
        if(doc.size() != 1)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "timestamp element"));
        type = element_type::timestamp_element;
        // TODO: implement timestamp
    } else if(name == "$regex" || name == "$options") {
        if(doc.size() != 2)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "regex element"));
        auto it = doc.find("$regex");
        if(it == doc.end() || it->type() != element_type::string_element)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "regex element"));
        auto re = get<element_type::string_element>(*it);
        it = doc.find("$options");
        if(it == doc.end() || it->type() != element_type::string_element)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "regex element"));
        auto options = get<element_type::string_element>(*it);

        type = element_type::regex_element;
        out = setter<element_type::regex_element, decltype(std::make_tuple(re, options))>::call(
            out, std::make_tuple(re, options));
    } else if(name == "$oid") {
        if(doc.size() != 40 || doc.begin()->type() != element_type::string_element)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "oid element"));
        auto str = get<element_type::string_element>(*doc.begin());
        if(str.size() != 24)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "oid element"));
        std::array<char, 12> oid;
        for(auto i = 0; i < 24; i += 2)
            oid[i / 2] = static_cast<char>(std::stoi(boost::lexical_cast<std::string>(str.substr(i, 2)), nullptr, 16));

        type = element_type::oid_element;
        out = setter<element_type::oid_element, decltype(oid)>::call(out, oid);
    } else if(name == "$ref" || name == "$id") {
        if(doc.size() != 2)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "ref element"));
        auto it = doc.find("$id");
        if(it == doc.end() || it->type() != element_type::string_element)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "ref element"));
        auto str = get<element_type::string_element>(*it);
        if(str.size() != 24)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "oid element"));
        std::array<char, 12> oid;
        for(auto i = 0; i < 24; i += 2)
            oid[i / 2] = static_cast<char>(std::stoi(boost::lexical_cast<std::string>(str.substr(i, 2)), nullptr, 16));

        it = doc.find("$ref");
        if(it == doc.end() || it->type() != element_type::string_element)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "ref element"));
        auto coll = get<element_type::string_element>(*it);
        type = element_type::db_pointer_element;
        out = setter<element_type::db_pointer_element, decltype(std::make_tuple(coll, oid))>::call(
            out, std::make_tuple(coll, oid));
    } else if(name == "$undefined") {
        if(doc.size() != 1)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "undefined element"));
        type = element_type::undefined_element;
    } else if(name == "$minkey") {
        if(doc.size() != 1)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "minkey element"));
        type = element_type::min_key;
    } else if(name == "$maxkey") {
        if(doc.size() != 1)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "maxkey element"));
        type = element_type::max_key;
    } else
        return std::make_tuple(boost::none, (element_type)0);
    return std::make_tuple(out, type);
}

template <typename ForwardIterator, typename OutputIterator>
OutputIterator json_reader::parse_string(line_pos_iterator<ForwardIterator>& first,
                                         const line_pos_iterator<ForwardIterator>& last, OutputIterator out) {
    assert(last != first);
    assert(&*first != nullptr);

    assert(out >= m_data.begin() && out <= m_data.end());
    out = m_data.insert(out, 4, '\0');

    auto size = std::distance(m_data.begin(), std::next(out, 4));
    out = parse_name(first, last, std::next(out, 4));
    boost::range::copy(detail::native_to_little_endian<int32_t>(m_data.size() - size),
                       std::next(m_data.begin(), size - 4));
    return out;
}

template <typename ForwardIterator, typename OutputIterator>
OutputIterator json_reader::parse_name(line_pos_iterator<ForwardIterator>& first,
                                       const line_pos_iterator<ForwardIterator>& last, OutputIterator out) {
    assert(last != first);
    assert(&*first != nullptr);
    if(*first != '"')
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "\""));
    if(first == last)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    std::advance(first, 1);

    while(true) {
        if(first == last)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));

        char c = *first;
        if(c == '"') {
            std::advance(first, 1);
            break;
        }

        if(c == '\\') {
            std::advance(first, 1);
            if(first == last || *first == '\0')
                BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
            c = *first;
            if(c == '"')
                c = '"';
            else if(c == '\\')
                c = '\\';
            else if(c == '/')
                c = '/';
            else if(c == 'b')
                c = '\b';
            else if(c == 'f')
                c = '\f';
            else if(c == 'n')
                c = '\n';
            else if(c == 'r')
                c = '\r';
            else if(c == 't')
                c = '\t';
            else if(c == 'u') {
                std::advance(first, 1);
                auto idx = size_t{0};
                auto codepoint = std::stoi(std::string{first, std::next(first, 4)}, &idx, 16);
                if(idx != 4)
                    BOOST_THROW_EXCEPTION(
                        make_parse_exception(json_error_num::unexpected_token, std::next(first, idx), last));
                std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt{};
                std::string str;
                if(codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                    // Handle UTF-16 surrogate pair. Adapted from rapidjson
                    std::advance(first, 4);
                    if(*first++ != '\\' || *first++ != 'u')
                        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last));
                    auto codepoint2 = std::stoi(std::string{first, std::next(first, 4)}, &idx, 16);
                    if(codepoint2 < 0xDC00 || codepoint2 > 0xDFFF)
                        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last));
                    codepoint2 = (((codepoint - 0xD800) << 10) | (codepoint2 - 0xDC00)) + 0x10000;
                    str = cvt.to_bytes(codepoint2);
                } else if(codepoint == 0x0000)
                    str = std::string{R"(\u0000)"};
                else
                    str = cvt.to_bytes(codepoint);
                std::advance(first, 4);
                out = m_data.insert(out, str.size(), '\0');
                out = boost::range::copy(str, out);
                continue;
            } else
                BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last));
        } else if(::iscntrl(c) && c != '\x7f')
            BOOST_THROW_EXCEPTION(
                make_parse_exception(json_error_num::unexpected_token, first, last, "non-control char"));

        assert(out >= m_data.begin() && out <= m_data.end());
        out = std::next(m_data.insert(out, c));

        std::advance(first, 1);
    }
    assert(out >= m_data.begin() && out <= m_data.end());
    return std::next(m_data.insert(out, '\0'));
}

template <typename ForwardIterator, typename OutputIterator>
std::tuple<OutputIterator, element_type> json_reader::parse_number(line_pos_iterator<ForwardIterator>& first,
                                                                   const line_pos_iterator<ForwardIterator>& last_,
                                                                   OutputIterator out) {
    static_assert(detail::is_iterator_pointer<std::decay_t<ForwardIterator>>::value, "");
    assert(last_ != first);
    assert(&*first != nullptr);
    if(!std::isdigit(*first) && *first != '-')
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last_, "number"));
    auto last = std::find_if_not(
        first, last_, [](char c) { return ::isdigit(c) || c == '.' || c == '+' || c == '-' || c == 'e' || c == 'E'; });

    if(*first == '0' && std::distance(first, last) > 1 && *std::next(first) != '.')
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last_, "number"));
    auto type = element_type::null_element;
    using namespace boost::spirit;
    char* pos;
    auto val = ::strtold(&*first, &pos);

    if(val == HUGE_VALL || pos == &*first || pos != &*last)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "number"));
    std::advance(first, std::distance(const_cast<const char*>(&*first), const_cast<const char*>(pos)));

    ::feclearexcept(FE_ALL_EXCEPT);
    int64_t i = std::llrint(val);
    if(::fetestexcept(FE_ALL_EXCEPT)) {
        assert(out >= m_data.begin() && out <= m_data.end());
        out = m_data.insert(out, sizeof(double), '\0');
        out = detail::set_impl<element_type::double_element, decltype(m_data), double>::call(out, val);
        type = element_type::double_element;
    } else if(i > std::numeric_limits<int32_t>::min() && i < std::numeric_limits<int32_t>::max()) {
        assert(out >= m_data.begin() && out <= m_data.end());
        out = m_data.insert(out, sizeof(int32_t), '\0');
        out = detail::set_impl<element_type::int32_element, decltype(m_data), int32_t>::call(out, i);
        type = element_type::int32_element;
    } else {
        assert(out >= m_data.begin() && out <= m_data.end());
        out = m_data.insert(out, sizeof(int64_t), '\0');
        out = detail::set_impl<element_type::int64_element, decltype(m_data), int64_t>::call(out, i);
        type = element_type::int64_element;
    }
    ::feclearexcept(FE_ALL_EXCEPT);

    return std::make_tuple(out, type);
}

template <typename ForwardIterator>
void json_reader::skip_space(line_pos_iterator<ForwardIterator>& first,
                             const line_pos_iterator<ForwardIterator>& last) {
    first = std::find_if_not(first, last, &::isspace);
}

inline namespace literal {

document_set operator"" _json_set(const char* str, size_t len) {
    auto reader = json_reader{};
    reader.parse(str, str + len);
    return document_set(document(std::move(reader)));
}

document operator"" _json_doc(const char* str, size_t len) {
    auto reader = json_reader{};
    reader.parse(str, str + len);
    return std::move(reader);
}

array operator"" _json_arr(const char* str, size_t len) {
    auto reader = json_reader{};
    reader.parse(str, str + len);
    return std::move(reader);
}

} // namespace literal

} // namesapce jbson

#endif // JBSON_JSON_READER_HPP
