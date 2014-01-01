//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_JSON_READER_HPP
#define JBSON_JSON_READER_HPP

#include <type_traits>
#include <iterator>
#include <memory>
#include <deque>

#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/exception/exception.hpp>
#include <boost/spirit/home/support/iterators/line_pos_iterator.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <jbson/element.hpp>
#include <jbson/builder.hpp>

namespace jbson {

using boost::spirit::line_pos_iterator;

struct json_parse_error;
enum class json_error_num;

struct json_reader {
    explicit json_reader(const std::locale& locale = std::locale()) : m_locale(locale) {}

    json_reader(const json_reader&) = delete;
    json_reader& operator=(const json_reader&) = delete;

    json_reader(json_reader&&) = default;
    json_reader& operator=(json_reader&&) = default;

    ~json_reader() {}

    template <typename ForwardIterator> void parse(ForwardIterator, ForwardIterator);

    template <typename ForwardRange_> void parse(ForwardRange_&& range_) {
        auto range = boost::as_literal(std::forward<ForwardRange_>(range_));
        using ForwardRange = decltype(range);
        BOOST_CONCEPT_ASSERT((boost::ForwardRangeConcept<ForwardRange>));
        using line_it = line_pos_iterator<typename boost::range_const_iterator<std::decay_t<ForwardRange>>::type>;
        parse(line_it{std::begin(range)}, line_it{std::end(range)});
    }

    operator document_set() const { return m_elements; }

  private:
    template <typename ForwardIterator>
    void parse_document(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&, document_set&);
    template <typename ForwardIterator>
    void parse_array(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&, document_set&);

    template <typename ForwardIterator>
    void parse_value(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&, element&);

    bool parse_extended_value(element&, const document_set&);

    template <typename ForwardIterator>
    std::string parse_string(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&);
    template <typename ForwardIterator>
    void parse_number(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&, element&);
    template <typename ForwardIterator>
    bool parse_int(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&, element&);
    template <typename ForwardIterator>
    bool parse_float(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&, element&);

    template <typename ForwardIterator>
    void skip_space(line_pos_iterator<ForwardIterator>&, const line_pos_iterator<ForwardIterator>&);

    template <typename ForwardIterator>
    json_parse_error make_parse_exception(json_error_num, const line_pos_iterator<ForwardIterator>& current,
                                          const line_pos_iterator<ForwardIterator>& last,
                                          const std::string& expected = {}) const;
    json_parse_error make_parse_exception(json_error_num, const std::string& expected = {}) const;

  private:
    std::locale m_locale;
    document_set m_elements;
    std::shared_ptr<void> m_start;
};

enum class json_error_num {
    invalid_root_element,
    unexpected_end_of_range,
    unexpected_token,
};
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
    auto e = make_parse_exception(err, expected);
    if(m_start) {
        auto start = *static_cast<line_pos_iterator<ForwardIterator>*>(m_start.get());
        e << current_line(boost::lexical_cast<std::string>(boost::spirit::get_current_line(start, current, last)));
        e << line_pos(boost::spirit::get_column(start, current));
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

template <typename ForwardIterator> void json_reader::parse(ForwardIterator first_, ForwardIterator last_) {
    BOOST_CONCEPT_ASSERT((boost::ForwardIterator<ForwardIterator>));
    static_assert(std::is_same<typename std::iterator_traits<ForwardIterator>::value_type, char>::value, "");

    line_pos_iterator<ForwardIterator> first{first_}, last{last_};
    m_start = std::make_shared<line_pos_iterator<ForwardIterator>>(first);
    skip_space(first, last);
    if(first == last || *first == '\0')
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    switch(*first) {
        case '{':
            parse_document(first, last, m_elements);
            return;
        case '[':
            parse_array(first, last, m_elements);
            return;
        default:
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::invalid_root_element, first, last));
    };
}

template <typename ForwardIterator>
void json_reader::parse_document(line_pos_iterator<ForwardIterator>& first,
                                 const line_pos_iterator<ForwardIterator>& last, document_set& elements) {
    if(*first != '{')
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "{"));
    skip_space(++first, last);
    if(first == last)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    if(*first == '}')
        return;

    while(true) {
        auto field_name = parse_string(first, last);
        element el{field_name, element_type::null_element};
        skip_space(first, last);

        if(*first != ':')
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, ":"));
        skip_space(++first, last);
        parse_value(first, last, el);
        elements.emplace(std::move(el));

        skip_space(first, last);
        if(*first == ',') {
            ++first;
            continue;
        }
        if(*first != '}')
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "}"));
        else
            return;
    }
}

template <typename ForwardIterator>
void json_reader::parse_array(line_pos_iterator<ForwardIterator>& first, const line_pos_iterator<ForwardIterator>& last,
                              document_set& elements) {
    if(*first != '[')
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "["));
    skip_space(++first, last);
    if(first == last)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    if(*first == ']')
        return;

    int32_t idx{0};
    while(true) {
        element el{std::to_string(idx++), element_type::null_element};
        skip_space(first, last);

        parse_value(first, last, el);
        elements.emplace(std::move(el));

        skip_space(first, last);
        if(*first == ',') {
            ++first;
            continue;
        }
        if(*first != ']')
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, ", or ]"));
        else
            return;
    }
}

template <typename ForwardIterator>
void json_reader::parse_value(line_pos_iterator<ForwardIterator>& first, const line_pos_iterator<ForwardIterator>& last,
                              element& el) {
    switch(*first) {
        case '{': {
            document_set edoc;
            parse_document(first, last, edoc);
            if(!edoc.empty()) {
                auto name = edoc.begin()->name();
                if(!name.empty() && name[0] == '$' && parse_extended_value(el, edoc)) {
                } else {
                    auto doc = document{edoc};
                    el.value<element_type::document_element>(std::move(doc));
                }
            }
            break;
        }
        case '[': {
            document_set earr;
            parse_array(first, last, earr);
            auto arr = array{earr};
            el.value<element_type::array_element>(std::move(arr));
            break;
        }
        case '"':
            el.value<element_type::string_element>(parse_string(first, last));
            break;
        case 't':
            if(boost::equal(boost::as_literal("true"), boost::make_iterator_range(first, std::next(first, 4)))) {
                el.value<element_type::boolean_element>(true);
                std::advance(first, 4);
                break;
            }
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "true"));
        case 'f':
            if(boost::equal(boost::as_literal("false"), boost::make_iterator_range(first, std::next(first, 5)))) {
                el.value<element_type::boolean_element>(false);
                std::advance(first, 5);
                break;
            }
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "false"));
        case 'n':
            if(!boost::equal(boost::as_literal("null"), boost::make_iterator_range(first, std::next(first, 4))))
                BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "null"));
            std::advance(first, 4);
            break;
        default:
            parse_number(first, last, el);
    }
}

bool json_reader::parse_extended_value(element& e, const document_set& edoc) {
    if(edoc.empty())
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "extended json value"));
    auto name = edoc.begin()->name();
    if(name == "$binary" || name == "$type") {
        if(edoc.size() != 2)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "binary element"));
        // TODO: implement binary value
    } else if(name == "$date") {
        if(edoc.size() != 1 ||
           (edoc.begin()->type() != element_type::int32_element && edoc.begin()->type() != element_type::int64_element))
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "date element"));
        using DateT = detail::ElementTypeMap<element_type::date_element, element::container_type>;
        if(edoc.begin()->type() == element_type::int32_element)
            e.value<element_type::date_element>(
                DateT{std::chrono::milliseconds{get<element_type::int32_element>(*edoc.begin())}});
        else if(edoc.begin()->type() == element_type::int64_element)
            e.value<element_type::date_element>(
                DateT{std::chrono::milliseconds{get<element_type::int64_element>(*edoc.begin())}});
        else
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "date element"));
    } else if(name == "$timestamp") {
        if(edoc.size() != 1)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "timestamp element"));
        // TODO: implement timestamp
    } else if(name == "$regex" || name == "$options") {
        if(edoc.size() != 2)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "regex element"));
        auto it = edoc.find("$regex");
        if(it == edoc.end() || it->type() != element_type::string_element)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "regex element"));
        auto re = get<element_type::string_element>(*it);
        it = edoc.find("$options");
        if(it == edoc.end() || it->type() != element_type::string_element)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "regex element"));
        auto options = get<element_type::string_element>(*it);
        e.value<element_type::regex_element>(std::make_tuple(re, options));
    } else if(name == "$oid") {
        if(edoc.size() != 1 || edoc.begin()->type() != element_type::string_element)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "oid element"));
        auto str = get<element_type::string_element>(*edoc.begin());
        if(str.size() != 24)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "oid element"));
        std::array<char, 12> oid;
        for(auto i = 0; i < 24; i += 2)
            oid[i / 2] = static_cast<char>(std::stoi(str.substr(i, 2).to_string(), nullptr, 16));

        e.value<element_type::oid_element>(oid);
    } else if(name == "$ref" || name == "$id") {
        if(edoc.size() != 2)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "ref element"));
        auto it = edoc.find("$id");
        if(it == edoc.end() || it->type() != element_type::string_element)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "ref element"));
        auto str = get<element_type::string_element>(*it);
        if(str.size() != 24)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "oid element"));
        std::array<char, 12> oid;
        for(auto i = 0; i < 24; i += 2)
            oid[i / 2] = static_cast<char>(std::stoi(str.substr(i, 2).to_string(), nullptr, 16));

        it = edoc.find("$ref");
        if(it == edoc.end() || it->type() != element_type::string_element)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "ref element"));
        auto coll = get<element_type::string_element>(*it);
        e.value<element_type::db_pointer_element>(std::make_tuple(coll, oid));
    } else if(name == "$undefined") {
        if(edoc.size() != 1)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "undefined element"));
        e.type(element_type::undefined_element);
    } else if(name == "$minkey") {
        if(edoc.size() != 1)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "minkey element"));
        e.type(element_type::min_key);
    } else if(name == "$maxkey") {
        if(edoc.size() != 1)
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, "maxkey element"));
        e.type(element_type::max_key);
    } else
        return false;
    return true;
}

template <typename ForwardIterator>
std::string json_reader::parse_string(line_pos_iterator<ForwardIterator>& first,
                                      const line_pos_iterator<ForwardIterator>& last) {
    if(*first != '"')
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "\""));
    if(first == last)
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    ++first;
    std::string str;
    while(true) {
        if(*first == '"') {
            ++first;
            break;
        }

        auto c = *first;
        if(*first == '\\') {
            ++first;
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
            else if(c == 'u') {
                ++first;
                auto idx = size_t{0};
                auto codepoint = std::stoi(std::string{first, std::next(first, 4)}, &idx, 16);
                if(idx != 4)
                    BOOST_THROW_EXCEPTION(
                        make_parse_exception(json_error_num::unexpected_token, std::next(first, idx), last));
                c = std::char_traits<char>::to_char_type(codepoint);
                std::advance(first, 3);
            } else if(std::iscntrl(c, m_locale))
                BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last));
            else
                break;
        }
        str.push_back(c);
        ++first;
        if(first == last || *first == '\0')
            BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_end_of_range, first, last));
    }
    return str;
}

template <typename ForwardIterator>
void json_reader::parse_number(line_pos_iterator<ForwardIterator>& first,
                               const line_pos_iterator<ForwardIterator>& last_, element& e) {
    if(!std::isdigit(*first) && *first != '-')
        BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last_, "number"));
    auto last = std::find_if_not(first, last_, boost::algorithm::is_any_of(boost::as_literal(".+-eE")) ||
                                                   boost::algorithm::is_digit());

    if(parse_int(first, last, e) || parse_float(first, last, e)) {
        first = last;
        return;
    }
    BOOST_THROW_EXCEPTION(make_parse_exception(json_error_num::unexpected_token, first, last, "number"));
}

template <typename ForwardIterator>
bool json_reader::parse_int(line_pos_iterator<ForwardIterator>& first, const line_pos_iterator<ForwardIterator>& last,
                            element& e) {
    try {
        auto val = boost::lexical_cast<int64_t>(boost::make_iterator_range(first, last));
        if(val > std::numeric_limits<int32_t>::min() && val < std::numeric_limits<int32_t>::max())
            e.value<element_type::int32_element>(static_cast<int32_t>(val));
        else
            e.value<element_type::int64_element>(val);
        return true;
    }
    catch(boost::bad_lexical_cast&) {
        return false;
    }
}

template <typename ForwardIterator>
bool json_reader::parse_float(line_pos_iterator<ForwardIterator>& first, const line_pos_iterator<ForwardIterator>& last,
                              element& e) {
    try {
        auto val = boost::lexical_cast<double>(boost::make_iterator_range(first, last));
        e.value<element_type::double_element>(val);
        return true;
    }
    catch(boost::bad_lexical_cast&) {
        return false;
    }
}

template <typename ForwardIterator>
void json_reader::skip_space(line_pos_iterator<ForwardIterator>& first,
                             const line_pos_iterator<ForwardIterator>& last) {
    first = std::find_if_not(first, last, [this](char c) { return std::isspace(c, m_locale); });
}

document_set operator"" _json(const char* str, size_t len) {
    auto reader = json_reader{};
    reader.parse(str, str + len);
    return reader;
}

} // namesapce jbson

#endif // JBSON_JSON_READER_HPP
