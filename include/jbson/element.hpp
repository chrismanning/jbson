//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_ELEMENT_HPP
#define JBSON_ELEMENT_HPP

#include <string>
#include <vector>
#include <chrono>
#include <array>

#include <boost/predef/other/endian.h>
#include <boost/mpl/map.hpp>
#include <boost/mpl/at.hpp>
#include <boost/optional.hpp>

namespace jbson {

enum class element_type : uint8_t {
    double_element = 0x01,
    string_element = 0x02,
    document_element = 0x03,
    array_element = 0x04,
    binary_element = 0x05,
    undefined_element = 0x06,
    oid_element = 0x07,
    boolean_element = 0x08,
    date_element = 0x09,
    null_element = 0x0A,
    regex_element = 0x0B,
    db_pointer_element = 0x0C,
    javascript_element = 0x0D,
    symbol_element = 0x0E,
    scoped_javascript_element = 0x0F,
    int32_element = 0x10,
    timestamp_element = 0x11,
    int64_element = 0x12,
    min_key = 0xFF,
    max_key = 0x7F
};

std::ostream& operator<<(std::ostream&, element_type);

template <class Container> class basic_document;
template <typename Container> struct basic_element;

namespace detail {
template <typename T, typename ForwardIterator> T little_endian_to_native(ForwardIterator, ForwardIterator);
template <typename T> std::array<char, sizeof(T)> native_to_little_endian(T&&);

namespace mpl = boost::mpl;

template <element_type EType> using element_type_c = mpl::integral_c<element_type, EType>;

template <element_type EType, typename Container>
using ElementTypeMap = typename mpl::at<
    typename mpl::map<
        mpl::pair<element_type_c<element_type::double_element>, double>,
        mpl::pair<element_type_c<element_type::string_element>, std::string>,
        mpl::pair<element_type_c<element_type::document_element>, basic_document<Container>>,
        mpl::pair<element_type_c<element_type::array_element>, basic_document<Container>>,
        mpl::pair<element_type_c<element_type::binary_element>, std::vector<char>>,
        mpl::pair<element_type_c<element_type::undefined_element>, void>,
        mpl::pair<element_type_c<element_type::oid_element>, std::array<char, 12>>,
        mpl::pair<element_type_c<element_type::boolean_element>, bool>,
        mpl::pair<element_type_c<element_type::date_element>,
                  std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>>,
        mpl::pair<element_type_c<element_type::null_element>, void>,
        mpl::pair<element_type_c<element_type::regex_element>, std::tuple<std::string, std::string>>,
        mpl::pair<element_type_c<element_type::db_pointer_element>, std::tuple<std::string, std::array<char, 12>>>,
        mpl::pair<element_type_c<element_type::javascript_element>, std::string>,
        mpl::pair<element_type_c<element_type::symbol_element>, std::string>,
        mpl::pair<element_type_c<element_type::scoped_javascript_element>,
                  std::tuple<std::string, basic_document<Container>>>,
        mpl::pair<element_type_c<element_type::int32_element>, int32_t>,
        mpl::pair<element_type_c<element_type::timestamp_element>, int64_t>,
        mpl::pair<element_type_c<element_type::int64_element>, int64_t>,
        mpl::pair<element_type_c<element_type::min_key>, void>,
        mpl::pair<element_type_c<element_type::max_key>, void>>::type,
    element_type_c<EType>>::type;

template <element_type EType, typename ReturnT, typename Enable = void> struct get_impl;

} // namespace detail

template <class Container> struct basic_element {
    using container_type = typename std::decay<Container>::type;
    static_assert(std::is_same<typename container_type::value_type, char>::value, "");

    basic_element() = default;

    basic_element(const basic_element&) = default;
    basic_element& operator=(const basic_element&) = default;

    basic_element(basic_element&&) = default;
    basic_element& operator=(basic_element&&) = default;

    basic_element(container_type&& c) : m_data(std::move(c)) {}
    basic_element(const container_type& c) : m_data(c) {}

    template <typename ForwardRange> basic_element(const ForwardRange&);
    template <typename ForwardIterator> basic_element(ForwardIterator, ForwardIterator);

    template <typename T>
    basic_element(const std::string& name, element_type type, T&& val)
        : basic_element(name, type) {
        value(type, std::forward<T>(val));
    }
    template <typename ForwardIterator>
    basic_element(const std::string&, element_type, ForwardIterator, ForwardIterator);
    basic_element(const std::string&, element_type);

    // field name
    const std::string& name() const { return m_name; }
    // size in bytes
    size_t size() const;

    element_type type() const { return m_type; }
    void type(element_type type) { m_type = type; }

    template <typename T> boost::optional<T> value() const;

    template <typename T>
    void value(T&& val, typename std::enable_if_t<std::is_pod<typename std::decay<T>::type>::value>* = nullptr) {
        auto arr = detail::native_to_little_endian(std::forward<T>(val));
        m_data.assign(arr.begin(), arr.end());
    }

    void value(std::string str) {
        assert(valid_type<typename std::decay<std::string>::type>(type()));
        m_data.assign(str.begin(), str.end());
    }

    template <element_type EType, typename T> void value(T&& val) noexcept {
        using T2 = ElementTypeMap<EType>;
        static_assert(std::is_convertible<typename std::decay<T>::type, T2>::value, "");
        value(EType, std::forward<T2>(val));
    }

    template <typename T> void value(element_type type, T&& val) {
        assert(valid_type<typename std::decay<T>::type>(type));
        this->type(type);
        value(std::forward<T>(val));
    }

    explicit operator bool() const { return !m_data.empty(); }

    bool operator==(const basic_element& other) const {
        return m_name == other.m_name && m_type == other.m_type &&
               std::equal(m_data.begin(), m_data.end(), other.m_data.begin(), other.m_data.end());
    }
    bool operator!=(const basic_element& other) const { return !(*this == other); }

  private:
    std::string m_name;
    element_type m_type;
    container_type m_data;

    template <element_type EType> using ElementTypeMap = detail::ElementTypeMap<EType, container_type>;
    template <typename ForwardIterator> static size_t detect_size(element_type, ForwardIterator, ForwardIterator);
    template <typename T> static bool valid_type(element_type);

    template <element_type EType, typename ReturnT, typename> friend struct detail::get_impl;
};

using element = basic_element<std::vector<char>>;

template <class Container>
template <typename ForwardRange>
basic_element<Container>::basic_element(const ForwardRange& range)
    : basic_element(std::begin(range), std::end(range)) {}

template <class Container>
template <typename ForwardIterator>
basic_element<Container>::basic_element(ForwardIterator first, ForwardIterator last) {
    m_type = static_cast<element_type>(*first++);
    auto str_end = std::find(first, last, '\0');
    m_name.assign(first, str_end++);
    first = str_end;
    last = std::next(first, detect_size(m_type, first, last));
    m_data = Container{first, last};
}

template <class Container>
template <typename ForwardIterator>
basic_element<Container>::basic_element(const std::string& name, element_type type, ForwardIterator first,
                                        ForwardIterator last)
    : m_name(name), m_type(type) {
    last = std::next(first, detect_size(m_type, first, last));
    m_data = Container{first, last};
}

template <class Container>
basic_element<Container>::basic_element(const std::string& name, element_type type)
    : m_name(name), m_type(type) {}

template <class Container> size_t basic_element<Container>::size() const {
    return m_data.size() + m_name.size() + sizeof(int32_t) + sizeof('\0');
}

template <class Container>
template <typename ForwardIterator>
size_t basic_element<Container>::detect_size(element_type e, ForwardIterator first, ForwardIterator last) {
    switch(e) {
    case element_type::double_element:
        return sizeof(double);
    case element_type::string_element:
        assert(sizeof(int32_t) <= std::distance(first, last));
        return sizeof(int32_t) + detail::little_endian_to_native<int32_t>(first, last);
    case element_type::document_element:
        assert(sizeof(int32_t) <= std::distance(first, last));
        return detail::little_endian_to_native<int32_t>(first, last);
    case element_type::array_element:
        assert(sizeof(int32_t) <= std::distance(first, last));
        return detail::little_endian_to_native<int32_t>(first, last);
    case element_type::binary_element:
        assert(sizeof(int32_t) <= std::distance(first, last));
        return sizeof(int32_t) + 1 + detail::little_endian_to_native<int32_t>(first, last);
    case element_type::undefined_element:
        break;
    case element_type::oid_element:
        return 12;
    case element_type::boolean_element:
        return sizeof(bool);
    case element_type::date_element:
        return sizeof(ElementTypeMap<element_type::date_element>);
    case element_type::null_element:
        break;
    case element_type::regex_element:
        return std::distance(first, std::find(std::find(first, last, '\0'), last, '\0'));
    case element_type::db_pointer_element:
        return detect_size(element_type::string_element, first, last) +
               detect_size(element_type::oid_element, first, last);
    case element_type::javascript_element:
    case element_type::symbol_element:
        return detect_size(element_type::string_element, first, last);
    case element_type::scoped_javascript_element:
        assert(sizeof(int32_t) <= std::distance(first, last));
        return sizeof(int32_t) + detail::little_endian_to_native<int32_t>(first, last);
    case element_type::int32_element:
        return sizeof(int32_t);
    case element_type::timestamp_element:
        return sizeof(int64_t);
    case element_type::int64_element:
        return sizeof(int64_t);
    case element_type::min_key:
        break;
    case element_type::max_key:
        break;
    default:
        assert(false);
    }
    return 0;
}

template <class Container> template <typename T> bool basic_element<Container>::valid_type(element_type type) {
    switch(type) {
    case element_type::double_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::string_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::document_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::array_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::binary_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::undefined_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::oid_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::boolean_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::date_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::null_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::regex_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::db_pointer_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::javascript_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::symbol_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::scoped_javascript_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::int32_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::timestamp_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::int64_element:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::min_key:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    case element_type::max_key:
        return std::is_convertible<T, ElementTypeMap<element_type::double_element>>::value;
    default:
        assert(false);
    };
}

template <element_type EType, typename Container>
auto get(const basic_element<Container>& elem) -> detail::ElementTypeMap<EType, Container>;

namespace detail {

template <element_type EType, typename ReturnT>
struct get_impl<EType, ReturnT, std::enable_if_t<std::is_arithmetic<ReturnT>::value>> {
    static_assert(std::is_arithmetic<ReturnT>::value, "return type not arithmetic");
    template <typename Container> static ReturnT call(const basic_element<Container>& elem) {
        return detail::little_endian_to_native<ReturnT>(elem.m_data.begin(), elem.m_data.end());
    }
};

template <element_type EType, typename ReturnT>
struct get_impl<EType, ReturnT, std::enable_if_t<std::is_same<ReturnT, std::string>::value>> {
    static_assert(std::is_same<ReturnT, std::string>::value, "return type not string");
    template <typename Container> static ReturnT call(const basic_element<Container>& elem) {
        auto first = elem.m_data.begin(), last = elem.m_data.end();
        std::advance(first, sizeof(int32_t));
        last = std::find(first, last, '\0');
        return ReturnT{first, last};
    }
};

template <typename> struct is_document : std::false_type {};

template <typename Container> struct is_document<basic_document<Container>> : std::true_type {};

template <typename> struct is_element : std::false_type {};

template <typename Container> struct is_element<basic_element<Container>> : std::true_type {};

template <element_type EType, typename ReturnT>
struct get_impl<EType, ReturnT, std::enable_if_t<is_document<ReturnT>::value>> {
    static_assert(is_document<ReturnT>::value, "element type not array/document");
    template <typename Container> static ReturnT call(const basic_element<Container>& elem) {
        return ReturnT{elem.m_data};
    }
};

template <typename T, typename ForwardIterator> T little_endian_to_native(ForwardIterator first, ForwardIterator last) {
    static_assert(std::is_pod<T>::value, "Can only byte swap POD types");
    assert(std::distance(first, last) >= sizeof(T));
    last = std::next(first, sizeof(T));
    assert(std::distance(first, last) == sizeof(T));
    union {
        T u;
        unsigned char u8[sizeof(T)];
    } source;

    std::copy(first, last, source.u8);

#if BOOST_ENDIAN_BIG_BYTE
    decltype(source) dest;
    for(size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];
    return dest.u;
#elif !BOOST_ENDIAN_LITTLE_BYTE
#error "unsupported endianness"
#else
    return source.u;
#endif
}

template <typename T> std::array<char, sizeof(T)> native_to_little_endian(T&& val) {
    using T2 = typename std::decay<T>::type;
    static_assert(std::is_pod<T2>::value, "Can only byte swap POD types");

    union {
        T2 u;
        std::array<char, sizeof(T2)> u8;
    } source;

    source.u = val;

#if BOOST_ENDIAN_BIG_BYTE
    decltype(source) dest;
    for(size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];
    return dest.u8;
#elif !BOOST_ENDIAN_LITTLE_BYTE
#error "unsupported endianness"
#else
    return source.u8;
#endif
}
} // namespace detail

template <element_type EType, typename Container>
auto get(const basic_element<Container>& elem) -> detail::ElementTypeMap<EType, Container> {
    using ReturnT = detail::ElementTypeMap<EType, Container>;
    assert(EType == elem.type());

    return detail::get_impl<EType, ReturnT>::call(elem);
}

std::ostream& operator<<(std::ostream& os, element_type e) {
    switch(e) {
    case element_type::double_element:
        os << "double_element";
        break;
    case element_type::string_element:
        os << "string_element";
        break;
    case element_type::document_element:
        os << "document_element";
        break;
    case element_type::array_element:
        os << "array_element";
        break;
    case element_type::binary_element:
        os << "binary_element";
        break;
    case element_type::undefined_element:
        os << "undefined_element";
        break;
    case element_type::oid_element:
        os << "oid_element";
        break;
    case element_type::boolean_element:
        os << "boolean_element";
        break;
    case element_type::date_element:
        os << "date_element";
        break;
    case element_type::null_element:
        os << "null_element";
        break;
    case element_type::regex_element:
        os << "regex_element";
        break;
    case element_type::db_pointer_element:
        os << "db_pointer_element";
        break;
    case element_type::javascript_element:
        os << "javascript_element";
        break;
    case element_type::symbol_element:
        os << "symbol_element";
        break;
    case element_type::scoped_javascript_element:
        os << "scoped_javascript_element";
        break;
    case element_type::int32_element:
        os << "int32_element";
        break;
    case element_type::timestamp_element:
        os << "timestamp_element";
        break;
    case element_type::int64_element:
        os << "int64_element";
        break;
    case element_type::min_key:
        os << "min_key";
        break;
    case element_type::max_key:
        os << "max_key";
        break;
    default:
        assert(false);
    };

    return os;
}

} // namespace jbson

#endif // JBSON_ELEMENT_HPP
