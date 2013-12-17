//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_JSON_READER_HPP
#define JBSON_JSON_READER_HPP

#include <type_traits>
#include <iterator>
#include <deque>

#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/lexical_cast.hpp>

#include <jbson/element.hpp>
#include <jbson/builder.hpp>

namespace jbson {

struct json_reader {
    explicit json_reader(const std::locale& locale = std::locale()) : m_locale(locale) {}

    explicit json_reader(const std::string& str, const std::locale& locale = std::locale()) : m_locale(locale) {
        parse(str);
    }

    template <typename ForwardIterator> void parse(ForwardIterator, ForwardIterator);
    template <typename ForwardRange> void parse(const ForwardRange& range) {
        parse(std::begin(range), std::end(range));
    }

  private:
    template <typename ForwardIterator> void parse_document(ForwardIterator&, ForwardIterator&, document_set&);
    template <typename ForwardIterator> void parse_array(ForwardIterator&, ForwardIterator&, document_set&);

    template <typename ForwardIterator> std::string parse_string(ForwardIterator&, ForwardIterator&);
    template <typename ForwardIterator> void parse_number(ForwardIterator&, ForwardIterator&, element&);
    template <typename ForwardIterator> bool parse_int(ForwardIterator&, ForwardIterator&, element&);
    template <typename ForwardIterator> bool parse_float(ForwardIterator&, ForwardIterator&, element&);

    template <typename ForwardIterator> void skip_space(ForwardIterator&, ForwardIterator&);

  private:
    std::locale m_locale;
    document_set m_elements;
};

template <typename ForwardIterator> void json_reader::parse(ForwardIterator first, ForwardIterator last) {
    static_assert(std::is_same<typename std::iterator_traits<ForwardIterator>::value_type, char>::value, "");
    skip_space(first, last);
    if(first == last)
        BOOST_THROW_EXCEPTION(std::exception());
    switch(*first) {
        case '{':
            parse_document(first, last, m_elements);
            return;
        case '[':
            parse_array(first, last, m_elements);
            return;
        case '\0':
            BOOST_THROW_EXCEPTION(std::exception());
        default:
            BOOST_THROW_EXCEPTION(std::exception());
    };
}

template <typename ForwardIterator>
void json_reader::parse_document(ForwardIterator& first, ForwardIterator& last, document_set& elements) {
    if(*first++ != '{')
        BOOST_THROW_EXCEPTION(std::exception());
    skip_space(first, last);
    if(first == last)
        BOOST_THROW_EXCEPTION(std::exception());
    if(*first == '}')
        return;

    while(true) {
        auto field_name = parse_string(first, last);
        element el{field_name, element_type::null_element};
        skip_space(first, last);

        if(*first++ != ':')
            BOOST_THROW_EXCEPTION(std::exception());
        skip_space(first, last);

        switch(*first) {
            case '{': {
                document_set edoc;
                parse_document(first, last, edoc);
                el.value<element_type::document_element>(document{edoc});
                break;
            }
            case '[': {
                document_set edoc;
                parse_array(first, last, edoc);
                el.value<element_type::array_element>(array{edoc});
                break;
            }
            case '"':
                el.value<element_type::string_element>(parse_string(first, last));
                break;
            case 't':
                if(boost::equal("true"s, boost::make_iterator_range(first, std::next(first, 4)))) {
                    el.value<element_type::boolean_element>(true);
                    std::advance(first, 4);
                    break;
                }
                BOOST_THROW_EXCEPTION(std::exception());
            case 'f':
                if(boost::equal("false"s, boost::make_iterator_range(first, std::next(first, 5)))) {
                    el.value<element_type::boolean_element>(false);
                    std::advance(first, 5);
                    break;
                }
                BOOST_THROW_EXCEPTION(std::exception());
            case 'n':
                if(!boost::equal("null"s, boost::make_iterator_range(first, std::next(first, 4))))
                    BOOST_THROW_EXCEPTION(std::exception());
                std::advance(first, 4);
                break;
            default:
                parse_number(first, last, el);
        }
        elements.emplace(std::move(el));

        skip_space(first, last);
        if(*first == ',')
            continue;
        if(*first != '}')
            BOOST_THROW_EXCEPTION(std::exception());
        else
            return;
    }
}

template <typename ForwardIterator>
void json_reader::parse_array(ForwardIterator& first, ForwardIterator& last, document_set& elements) {
    if(*first++ != '[')
        BOOST_THROW_EXCEPTION(std::exception());
    skip_space(first, last);
    if(*first == ']')
        return;
}

template <typename ForwardIterator>
std::string json_reader::parse_string(ForwardIterator& first, ForwardIterator& last) {
    if(*first++ != '"')
        BOOST_THROW_EXCEPTION(std::exception());
    if(first == last)
        BOOST_THROW_EXCEPTION(std::exception());
    std::string str;
    while(true) {
        if(*first++ == '"')
            break;

        if(*first == '\\') {
            std::next(first);
            break;
        } else
            str.push_back(*first);
        if(first == last)
            BOOST_THROW_EXCEPTION(std::exception());
    }
    return str;
}

template <typename ForwardIterator>
void json_reader::parse_number(ForwardIterator& first, ForwardIterator& last, element& e) {
    if(parse_int(first, last, e))
        return;
    if(parse_float(first, last, e))
        return;
    BOOST_THROW_EXCEPTION(std::exception());
}

template <typename ForwardIterator>
bool json_reader::parse_int(ForwardIterator& first, ForwardIterator& last, element& e) {
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
bool json_reader::parse_float(ForwardIterator& first, ForwardIterator& last, element& e) {
    try {
        auto val = boost::lexical_cast<double>(boost::make_iterator_range(first, last));
        e.value<element_type::double_element>(val);
        return true;
    }
    catch(boost::bad_lexical_cast&) {
        return false;
    }
}

template <typename ForwardIterator> void json_reader::skip_space(ForwardIterator& first, ForwardIterator& last) {
    first = std::find_if_not(first, last, [this](char c) { return std::isspace(c, m_locale); });
}

} // namesapce jbson

#endif // JBSON_JSON_READER_HPP
