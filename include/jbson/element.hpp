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
#include <exception>

#include <boost/predef/other/endian.h>
#include <boost/mpl/map.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/and.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include <boost/utility/string_ref.hpp>

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

template <class, class> class basic_document;
template <class, class> class basic_array;
template <typename Container> struct basic_element;

struct jbson_error : virtual std::exception, virtual boost::exception {
    const char* what() const noexcept override { return "jbson_error"; }
};

struct invalid_element_type : jbson_error {
    const char* what() const noexcept override { return "invalid_element_type"; }
};

struct incompatible_element_conversion : jbson_error {
    const char* what() const noexcept override { return "incompatible_element_conversion"; }
};

struct incompatible_type_conversion : jbson_error {
    const char* what() const noexcept override { return "incompatible_type_conversion"; }
};

struct invalid_element_size : jbson_error {
    const char* what() const noexcept override { return "invalid_element_size"; }
};

namespace detail {
template <typename T, typename ForwardIterator> T little_endian_to_native(ForwardIterator, ForwardIterator);
template <typename T> std::array<char, sizeof(T)> native_to_little_endian(T&&);

namespace mpl = boost::mpl;

template <element_type EType> using element_type_c = mpl::integral_c<element_type, EType>;

template <typename Iterator, typename Enable = void> struct is_iterator_pointer : std::false_type {};

template <typename Iterator>
struct is_iterator_pointer<Iterator, std::enable_if_t<std::is_constructible<Iterator, char*>::value>> : std::true_type {
};

template <typename Iterator>
struct is_iterator_pointer<
    Iterator, std::enable_if_t<std::is_pointer<typename Iterator::iterator_type>::value>> : std::true_type {};

template <typename Iterator>
struct is_iterator_pointer<
    boost::iterator_range<Iterator>,
    std::enable_if_t<std::is_pointer<std::decay_t<typename Iterator::iterator_type>>::value>> : std::true_type {};

template <typename Container> struct TypeMap {
    using container_type = boost::iterator_range<typename Container::const_iterator>;
    using string_type = std::conditional_t<is_iterator_pointer<typename container_type::iterator>::value,
                                           boost::string_ref, std::string>;
    typedef typename mpl::map<
        mpl::pair<element_type_c<element_type::double_element>, double>,
        mpl::pair<element_type_c<element_type::string_element>, string_type>,
        mpl::pair<element_type_c<element_type::document_element>, basic_document<container_type, container_type>>,
        mpl::pair<element_type_c<element_type::array_element>, basic_array<container_type, container_type>>,
        mpl::pair<element_type_c<element_type::binary_element>, container_type>,
        mpl::pair<element_type_c<element_type::undefined_element>, void>,
        mpl::pair<element_type_c<element_type::oid_element>, std::array<char, 12>>,
        mpl::pair<element_type_c<element_type::boolean_element>, bool>,
        mpl::pair<element_type_c<element_type::date_element>,
                  std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>>,
        mpl::pair<element_type_c<element_type::null_element>, void>,
        mpl::pair<element_type_c<element_type::regex_element>, std::tuple<string_type, string_type>>,
        mpl::pair<element_type_c<element_type::db_pointer_element>, std::tuple<string_type, std::array<char, 12>>>,
        mpl::pair<element_type_c<element_type::javascript_element>, string_type>,
        mpl::pair<element_type_c<element_type::symbol_element>, string_type>,
        mpl::pair<element_type_c<element_type::scoped_javascript_element>,
                  std::tuple<string_type, basic_document<container_type, container_type>>>,
        mpl::pair<element_type_c<element_type::int32_element>, int32_t>,
        mpl::pair<element_type_c<element_type::timestamp_element>, int64_t>,
        mpl::pair<element_type_c<element_type::int64_element>, int64_t>,
        mpl::pair<element_type_c<element_type::min_key>, void>,
        mpl::pair<element_type_c<element_type::max_key>, void>>::type map_type;
};

template <element_type EType, typename Container>
using ElementTypeMap = typename mpl::at<typename TypeMap<Container>::map_type, element_type_c<EType>>::type;

template <typename ReturnT, typename Enable = void> struct get_impl;
template <typename T, typename Enable = void> struct set_impl;

template <template <element_type EType, typename... VArgs> class Visitor, typename... Args>
std::enable_if_t<std::is_void<
    decltype(std::declval<Visitor<element_type::int64_element, Args...>>()(std::declval<Args&&>()...))>::value>
visit(element_type, Args&&... args);

template <template <element_type EType, typename... VArgs> class Visitor, typename... Args>
std::enable_if_t<!std::is_void<decltype(
                      std::declval<Visitor<element_type::int64_element, Args...>>()(std::declval<Args&&>()...))>::value,
                 decltype(std::declval<Visitor<element_type::int64_element, Args...>>()(std::declval<Args&&>()...))>
visit(element_type, Args&&... args);

} // namespace detail

template <class Container> struct basic_element {
    using container_type = typename std::decay<Container>::type;
    static_assert(std::is_same<typename container_type::value_type, char>::value, "");

    basic_element() = delete;

    template <typename OtherContainer>
    basic_element(const basic_element<OtherContainer>&,
                  std::enable_if_t<!std::is_constructible<container_type, OtherContainer>::value>* = nullptr);
    template <typename OtherContainer>
    basic_element(const basic_element<OtherContainer>&,
                  std::enable_if_t<std::is_constructible<container_type, OtherContainer>::value>* = nullptr);
    basic_element(const basic_element&) = default;
    basic_element& operator=(const basic_element&) = default;

    template <typename OtherContainer>
    basic_element(basic_element<OtherContainer>&&,
                  std::enable_if_t<!std::is_constructible<container_type, OtherContainer>::value>* = nullptr);
    template <typename OtherContainer>
    basic_element(basic_element<OtherContainer>&&,
                  std::enable_if_t<std::is_constructible<container_type, OtherContainer>::value>* = nullptr);
    basic_element(basic_element&&) = default;
    basic_element& operator=(basic_element&&) = default;

    explicit basic_element(const container_type& c);

    template <typename ForwardRange> basic_element(const ForwardRange&);
    template <typename ForwardIterator> basic_element(ForwardIterator, ForwardIterator);

    template <typename T>
    basic_element(std::string name, element_type type, T&& val)
        : basic_element(std::move(name), type) {
        value(std::forward<T>(val));
    }
    template <typename ForwardIterator> basic_element(std::string, element_type, ForwardIterator, ForwardIterator);
    basic_element(std::string, element_type);

    // field name
    boost::string_ref name() const { return m_name; }
    void name(std::string n) { m_name = std::move(n); }
    // size in bytes
    size_t size() const noexcept;

    element_type type() const noexcept { return m_type; }
    void type(element_type type) noexcept { m_type = type; }

    template <typename T> T value() const {
        if(!valid_type<T>())
            BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
        return detail::get_impl<T>::call(m_data);
    }

    template <typename T> void value(T&& val) {
        if(!valid_type<T>())
            BOOST_THROW_EXCEPTION(incompatible_type_conversion{});
        decltype(m_data) data;
        detail::set_impl<T>::call(data, std::forward<T>(val));
        using std::swap;
        swap(m_data, data);
    }

    template <typename T> void value(element_type type, T&& val) {
        this->type(type);
        value(std::forward<T>(val));
    }

    template <element_type EType, typename T> void value(T&& val) noexcept {
        using T2 = ElementTypeMap<EType>;
        static_assert(std::is_constructible<T2, typename std::decay<T>::type>::value, "");
        value(EType, std::forward<T>(val));
    }

    template <typename Visitor>
    void
    visit(Visitor&&,
          std::enable_if_t<std::is_void<decltype(std::declval<Visitor>()("", double {}, element_type{}))>::value>* =
              nullptr) const;
    template <typename Visitor>
    auto visit(
        Visitor&&,
        std::enable_if_t<!std::is_void<decltype(std::declval<Visitor>()("", double {}, element_type{}))>::value>* =
            nullptr) const -> decltype(std::declval<Visitor>()("", double {}, element_type{}));

    explicit operator bool() const noexcept { return !m_data.empty(); }

    template <typename OutContainer> void write_to_container(OutContainer&) const;
    template <typename OutContainer> explicit operator OutContainer() const;

    bool operator==(const basic_element& other) const {
        return m_name == other.m_name && m_type == other.m_type && boost::range::equal(m_data, other.m_data);
    }
    bool operator!=(const basic_element& other) const { return !(*this == other); }

  private:
    std::string m_name;
    element_type m_type;
    container_type m_data;

    template <element_type EType> using ElementTypeMap = detail::ElementTypeMap<EType, container_type>;
    template <typename ForwardIterator> static size_t detect_size(element_type, ForwardIterator, ForwardIterator);
    template <typename T> static bool valid_type(element_type);
    template <typename T> bool valid_type() const { return valid_type<T>(type()); }

    template <typename ReturnT, typename> friend struct detail::get_impl;
    template <typename T, typename> friend struct detail::set_impl;

    template <typename> friend struct basic_element;
    template <element_type EType, typename T>
    friend auto get(const basic_element<T>& elem) -> detail::ElementTypeMap<EType, T>;
};

template <element_type EType, typename Container>
auto get(const basic_element<Container>& elem) -> detail::ElementTypeMap<EType, Container>;

using element = basic_element<std::vector<char>>;

template <class Container>
template <typename OutContainer>
void basic_element<Container>::write_to_container(OutContainer& c) const {
    static_assert(std::is_same<typename OutContainer::value_type, char>::value, "");
    if(!m_type)
        BOOST_THROW_EXCEPTION(invalid_element_type{});
    c.push_back(static_cast<uint8_t>(m_type));
    if(m_name.empty())
        BOOST_THROW_EXCEPTION(invalid_element_size{});
    boost::range::push_back(c, m_name);
    c.push_back('\0');
    if(detect_size(m_type, m_data.begin(), m_data.end()) != m_data.size())
        BOOST_THROW_EXCEPTION(invalid_element_size{});
    boost::range::push_back(c, m_data);
}

template <class Container> template <typename OutContainer> basic_element<Container>::operator OutContainer() const {
    OutContainer c;
    write_to_container(c);
    return std::move(c);
}

template <class Container>
template <typename OtherContainer>
basic_element<Container>::basic_element(
    const basic_element<OtherContainer>& elem,
    std::enable_if_t<!std::is_constructible<container_type, OtherContainer>::value>*)
    : m_name(elem.m_name), m_type(elem.m_type) {
    boost::range::push_back(m_data, elem.m_data);
}

template <class Container>
template <typename OtherContainer>
basic_element<Container>::basic_element(const basic_element<OtherContainer>& elem,
                                        std::enable_if_t<std::is_constructible<container_type, OtherContainer>::value>*)
    : m_name(elem.m_name), m_type(elem.m_type), m_data(elem.m_data) {}

template <class Container>
template <typename OtherContainer>
basic_element<Container>::basic_element(
    basic_element<OtherContainer>&& elem,
    std::enable_if_t<!std::is_constructible<container_type, OtherContainer>::value>*)
    : m_name(std::move(elem.m_name)), m_type(std::move(elem.m_type)) {
    boost::range::push_back(m_data, elem.m_data);
    elem.m_data.clear();
}

template <class Container>
template <typename OtherContainer>
basic_element<Container>::basic_element(basic_element<OtherContainer>&& elem,
                                        std::enable_if_t<std::is_constructible<container_type, OtherContainer>::value>*)
    : m_name(std::move(elem.m_name)), m_type(std::move(elem.m_type)), m_data(std::move(elem.m_data)) {}

template <class Container> basic_element<Container>::basic_element(const container_type& c) {
    auto first = c.begin(), last = c.end();
    m_type = static_cast<element_type>(*first++);
    if(!m_type)
        BOOST_THROW_EXCEPTION(invalid_element_type{});
    auto str_end = std::find(first, last, '\0');
    m_name.assign(first, str_end++);
    first = str_end;
    const auto elem_size = detect_size(m_type, first, last);
    if(std::distance(first, last) < elem_size)
        BOOST_THROW_EXCEPTION(invalid_element_size{});
    last = std::next(first, elem_size);
    m_data = container_type{first, last};
}

template <class Container>
template <typename ForwardRange>
basic_element<Container>::basic_element(const ForwardRange& range)
    : basic_element(std::begin(range), std::end(range)) {}

template <class Container>
template <typename ForwardIterator>
basic_element<Container>::basic_element(ForwardIterator first, ForwardIterator last)
    : basic_element(container_type{first, last}) {}

template <class Container>
template <typename ForwardIterator>
basic_element<Container>::basic_element(std::string name, element_type type, ForwardIterator first,
                                        ForwardIterator last)
    : m_name(std::move(name)), m_type(type) {
    if(!m_type)
        BOOST_THROW_EXCEPTION(invalid_element_type{});
    last = std::next(first, detect_size(m_type, first, last));
    m_data = Container{first, last};
}

template <class Container>
basic_element<Container>::basic_element(std::string name, element_type type)
    : m_name(std::move(name)), m_type(type) {
    if(!m_type)
        BOOST_THROW_EXCEPTION(invalid_element_type{});
}

template <class Container> size_t basic_element<Container>::size() const noexcept {
    return sizeof(m_type) + m_data.size() + m_name.size() + sizeof('\0');
}

template <element_type EType, typename Visitor, typename Element> struct element_visitor {
    using element_type = std::decay_t<Element>;
    auto operator()(Visitor&& visitor, const element_type& elem) const {
        return visitor(elem.name(), get<EType>(elem), EType);
    }
};

template <typename Visitor, typename Element>
struct element_visitor<element_type::undefined_element, Visitor, Element> {
    auto operator()(Visitor&& visitor, Element&& elem) const {
        return visitor(elem.name(), element_type::undefined_element);
    }
};

template <typename Visitor, typename Element> struct element_visitor<element_type::null_element, Visitor, Element> {
    auto operator()(Visitor&& visitor, Element&& elem) const {
        return visitor(elem.name(), element_type::null_element);
    }
};

template <typename Visitor, typename Element> struct element_visitor<element_type::min_key, Visitor, Element> {
    auto operator()(Visitor&& visitor, Element&& elem) const { return visitor(elem.name(), element_type::min_key); }
};

template <typename Visitor, typename Element> struct element_visitor<element_type::max_key, Visitor, Element> {
    auto operator()(Visitor&& visitor, Element&& elem) const { return visitor(elem.name(), element_type::max_key); }
};

template <class Container>
template <typename Visitor>
void basic_element<Container>::visit(
    Visitor&& visitor,
    std::enable_if_t<std::is_void<decltype(std::declval<Visitor>()("", double {}, element_type{}))>::value>*) const {
    detail::visit<element_visitor>(m_type, std::forward<Visitor>(visitor), *this);
    return;
}

template <class Container>
template <typename Visitor>
auto basic_element<Container>::visit(
    Visitor&& visitor,
    std::enable_if_t<!std::is_void<decltype(std::declval<Visitor>()("", double {}, element_type{}))>::value>*)
    const -> decltype(std::declval<Visitor>()("", double {}, element_type{})) {
    return detail::visit<element_visitor>(m_type, std::forward<Visitor>(visitor), *this);
}

namespace detail {

// void visit
template <template <element_type EType, typename... VArgs> class Visitor, typename... Args>
std::enable_if_t<std::is_void<
    decltype(std::declval<Visitor<element_type::int64_element, Args...>>()(std::declval<Args&&>()...))>::value>
visit(element_type type, Args&&... args) {
    switch(type) {
        case element_type::double_element:
            Visitor<element_type::double_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::string_element:
            Visitor<element_type::string_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::document_element:
            Visitor<element_type::document_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::array_element:
            Visitor<element_type::array_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::binary_element:
            Visitor<element_type::binary_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::undefined_element:
            Visitor<element_type::undefined_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::oid_element:
            Visitor<element_type::oid_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::boolean_element:
            Visitor<element_type::boolean_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::date_element:
            Visitor<element_type::date_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::null_element:
            Visitor<element_type::null_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::regex_element:
            Visitor<element_type::regex_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::db_pointer_element:
            Visitor<element_type::db_pointer_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::javascript_element:
            Visitor<element_type::javascript_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::symbol_element:
            Visitor<element_type::symbol_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::scoped_javascript_element:
            Visitor<element_type::scoped_javascript_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::int32_element:
            Visitor<element_type::int32_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::timestamp_element:
            Visitor<element_type::timestamp_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::int64_element:
            Visitor<element_type::int64_element, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::min_key:
            Visitor<element_type::min_key, Args...> {}
            (std::forward<Args>(args)...);
            return;
        case element_type::max_key:
            Visitor<element_type::max_key, Args...> {}
            (std::forward<Args>(args)...);
            return;
        default:
            BOOST_THROW_EXCEPTION(invalid_element_type{});
    };
}

// non-void visit
template <template <element_type EType, typename... VArgs> class Visitor, typename... Args>
std::enable_if_t<!std::is_void<decltype(
                      std::declval<Visitor<element_type::int64_element, Args...>>()(std::declval<Args&&>()...))>::value,
                 decltype(std::declval<Visitor<element_type::int64_element, Args...>>()(std::declval<Args&&>()...))>
visit(element_type type, Args&&... args) {
    switch(type) {
        case element_type::double_element:
            return Visitor<element_type::double_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::string_element:
            return Visitor<element_type::string_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::document_element:
            return Visitor<element_type::document_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::array_element:
            return Visitor<element_type::array_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::binary_element:
            return Visitor<element_type::binary_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::undefined_element:
            return Visitor<element_type::undefined_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::oid_element:
            return Visitor<element_type::oid_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::boolean_element:
            return Visitor<element_type::boolean_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::date_element:
            return Visitor<element_type::date_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::null_element:
            return Visitor<element_type::null_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::regex_element:
            return Visitor<element_type::regex_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::db_pointer_element:
            return Visitor<element_type::db_pointer_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::javascript_element:
            return Visitor<element_type::javascript_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::symbol_element:
            return Visitor<element_type::symbol_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::scoped_javascript_element:
            return Visitor<element_type::scoped_javascript_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::int32_element:
            return Visitor<element_type::int32_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::timestamp_element:
            return Visitor<element_type::timestamp_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::int64_element:
            return Visitor<element_type::int64_element, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::min_key:
            return Visitor<element_type::min_key, Args...> {}
            (std::forward<Args>(args)...);
        case element_type::max_key:
            return Visitor<element_type::max_key, Args...> {}
            (std::forward<Args>(args)...);
        default:
            BOOST_THROW_EXCEPTION(invalid_element_type{});
    };
}

template <typename T, typename Container, typename Enable = void> struct is_valid_func;

} // namespace detail

template <element_type EType, typename ForwardIterator, typename = ForwardIterator> struct size_func {
    size_t operator()(ForwardIterator, ForwardIterator) const {
        return sizeof(detail::ElementTypeMap<EType, std::string>);
    }
};

template <typename ForwardIterator> struct size_func<element_type::null_element, ForwardIterator> {
    size_t operator()(ForwardIterator, ForwardIterator) const { return 0; }
};

template <typename ForwardIterator> struct size_func<element_type::min_key, ForwardIterator> {
    size_t operator()(ForwardIterator, ForwardIterator) const { return 0; }
};

template <typename ForwardIterator> struct size_func<element_type::max_key, ForwardIterator> {
    size_t operator()(ForwardIterator, ForwardIterator) const { return 0; }
};

template <typename ForwardIterator> struct size_func<element_type::undefined_element, ForwardIterator> {
    size_t operator()(ForwardIterator, ForwardIterator) const { return 0; }
};

template <typename ForwardIterator> struct size_func<element_type::string_element, ForwardIterator> {
    size_t operator()(ForwardIterator first, ForwardIterator last) const {
        if(sizeof(int32_t) > std::distance(first, last))
            BOOST_THROW_EXCEPTION(invalid_element_size{});
        return sizeof(int32_t) + detail::little_endian_to_native<int32_t>(first, last);
    }
};

template <typename ForwardIterator>
struct size_func<element_type::javascript_element, ForwardIterator> : size_func<element_type::string_element,
                                                                                ForwardIterator> {};

template <typename ForwardIterator>
struct size_func<element_type::symbol_element, ForwardIterator> : size_func<element_type::string_element,
                                                                            ForwardIterator> {};

template <typename ForwardIterator>
struct size_func<element_type::binary_element, ForwardIterator> : private size_func<element_type::string_element,
                                                                                    ForwardIterator> {
    size_t operator()(ForwardIterator first, ForwardIterator last) const {
        return size_func<element_type::string_element, ForwardIterator>::operator()(first, last) + 1;
    }
};

template <typename ForwardIterator> struct size_func<element_type::document_element, ForwardIterator> {
    size_t operator()(ForwardIterator first, ForwardIterator last) const {
        if(sizeof(int32_t) > std::distance(first, last))
            BOOST_THROW_EXCEPTION(invalid_element_size{});
        return detail::little_endian_to_native<int32_t>(first, last);
    }
};

template <typename ForwardIterator>
struct size_func<element_type::array_element, ForwardIterator> : size_func<element_type::document_element,
                                                                           ForwardIterator> {};

template <typename ForwardIterator>
struct size_func<element_type::scoped_javascript_element, ForwardIterator> : size_func<element_type::document_element,
                                                                                       ForwardIterator> {};

template <typename ForwardIterator>
struct size_func<element_type::db_pointer_element,
                 ForwardIterator> : private size_func<element_type::oid_element, ForwardIterator>,
                                    private size_func<element_type::string_element, ForwardIterator> {
    size_t operator()(ForwardIterator first, ForwardIterator last) const {
        return size_func<element_type::string_element, ForwardIterator>::operator()(first, last) +
               size_func<element_type::oid_element, ForwardIterator>::operator()(first, last);
    }
};

template <typename ForwardIterator> struct size_func<element_type::regex_element, ForwardIterator> {
    size_t operator()(ForwardIterator first, ForwardIterator last) const {
        auto it = std::next(std::find(first, last, '\0'));
        it = std::next(std::find(it, last, '\0'));
        return std::distance(first, it);
    }
};

template <class Container>
template <typename ForwardIterator>
size_t basic_element<Container>::detect_size(element_type e, ForwardIterator first, ForwardIterator last) {
    return detail::visit<size_func>(e, first, last);
}

template <class Container> template <typename T> bool basic_element<Container>::valid_type(element_type type) {
    return detail::visit<detail::is_valid_func<T, Container>::template inner>(type);
}

namespace detail {

template <typename T, typename Container, typename Enable> struct is_valid_func {
    template <element_type EType, typename... Args>
    struct inner : std::is_convertible<T, detail::ElementTypeMap<EType, Container>> {
        static_assert(sizeof...(Args) == 0, "");
    };
    template <typename... Args> struct inner<element_type::boolean_element, Args...> : std::is_same<T, bool> {
        static_assert(sizeof...(Args) == 0, "");
    };
};

template <typename StringT> struct make_string {
    template <typename Iterator> static StringT call(const Iterator& first, const Iterator& last) {
        return StringT{first, last};
    }
};

template <> struct make_string<boost::string_ref> {
    template <typename Iterator> static boost::string_ref call(const Iterator& first, const Iterator& last) {
        static_assert(is_iterator_pointer<Iterator>::value, "can only use string_ref for raw memory");
        return boost::string_ref{&*first, static_cast<size_t>(std::distance(first, last))};
    }
};

// getters
// number
template <typename ReturnT> struct get_impl<ReturnT, std::enable_if_t<std::is_arithmetic<ReturnT>::value>> {
    template <typename RangeT> static ReturnT call(const RangeT& data) {
        if(boost::distance(data) != sizeof(ReturnT))
            BOOST_THROW_EXCEPTION(invalid_element_size{});
        return detail::little_endian_to_native<ReturnT>(data.begin(), data.end());
    }
};

// string
template <typename ReturnT>
struct get_impl<ReturnT, std::enable_if_t<std::is_convertible<std::decay_t<ReturnT>, boost::string_ref>::value>> {
    template <typename RangeT> static std::decay_t<ReturnT> call(const RangeT& data) {
        auto first = data.begin(), last = data.end();
        if(std::distance(first, last) <= sizeof(int32_t))
            BOOST_THROW_EXCEPTION(invalid_element_size{});
        std::advance(first, sizeof(int32_t));
        auto length = little_endian_to_native<int32_t>(data.begin(), first) - 1;
        last = std::find(first, last, '\0');
        if(std::distance(first, last) != length)
            BOOST_THROW_EXCEPTION(invalid_element_size{});
        return make_string<std::decay_t<ReturnT>>::call(first, last);
    }
};

template <typename> struct is_document : std::false_type {};

template <typename Container, typename ElementContainer>
struct is_document<basic_document<Container, ElementContainer>> : std::true_type {};

template <typename Container, typename ElementContainer>
struct is_document<basic_array<Container, ElementContainer>> : std::true_type {};

template <typename> struct is_element : std::false_type {};

template <typename Container> struct is_element<basic_element<Container>> : std::true_type {};

// embedded document
template <typename ReturnT>
struct get_impl<ReturnT, typename std::enable_if<is_document<std::decay_t<ReturnT>>::value>::type> {
    template <typename RangeT> static ReturnT call(const RangeT& data) {
        return ReturnT{data};
    }
};

// binary data
template <typename ReturnT>
struct get_impl<ReturnT, std::enable_if_t<std::is_same<std::decay_t<ReturnT>, std::vector<char>>::value>> {
    template <typename RangeT> static ReturnT call(const RangeT& data) {
        ReturnT vec;
        boost::range::push_back(vec, data);
        return vec;
    }
};

// ditto
template <template <typename> class ReturnT, typename Iterator>
struct get_impl<
    ReturnT<Iterator>,
    std::enable_if_t<std::is_same<std::decay_t<ReturnT<Iterator>>, typename boost::iterator_range<Iterator>>::value>> {
    template <typename RangeT> static ReturnT<Iterator> call(const RangeT& data) { return data; }
};

// oid
template <typename ReturnT>
struct get_impl<ReturnT, std::enable_if_t<std::is_same<std::decay_t<ReturnT>, std::array<char, 12>>::value>> {
    template <typename RangeT> static ReturnT call(const RangeT& data) {
        if(boost::distance(data) != 12)
            BOOST_THROW_EXCEPTION(invalid_element_size{});
        ReturnT arr;
        std::copy(data.begin(), data.end(), arr.data());
        return arr;
    }
};

// date
template <typename ReturnT>
struct get_impl<
    ReturnT,
    std::enable_if_t<std::is_same<
        std::decay_t<ReturnT>, std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>>::value>> {
    template <typename RangeT> static ReturnT call(const RangeT& data) {
        if(boost::distance(data) != 8)
            BOOST_THROW_EXCEPTION(invalid_element_size{});
        return ReturnT{std::chrono::milliseconds{get_impl<int64_t>::call(data)}};
    }
};

template <size_t N, typename TupleT> using tuple_element_t = typename std::tuple_element<N, std::decay_t<TupleT>>::type;

// regex
template <typename TupleT>
struct get_impl<TupleT,
                std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                                 std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value&&
                                     std::is_same<tuple_element_t<0, TupleT>, tuple_element_t<1, TupleT>>::value>> {
    template <typename RangeT> static std::decay_t<TupleT> call(const RangeT& data) {
        using string_maker = make_string<std::decay_t<tuple_element_t<1, TupleT>>>;
        auto first = std::find(data.begin(), data.end(), '\0');
        auto str = string_maker::call(data.begin(), first);
        auto last = std::find(++first, data.end(), '\0');
        return std::make_tuple(str, string_maker::call(first, last));
    }
};

// db pointer
template <typename TupleT>
struct get_impl<
    TupleT, std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                             std::is_same<std::array<char, 12>, std::decay_t<tuple_element_t<1, TupleT>>>::value&&
                                 std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value>> {
    template <typename RangeT> static std::decay_t<TupleT> call(const RangeT& data) {
        auto str = get_impl<tuple_element_t<0, TupleT>>::call(data);
        auto oid = get_impl<std::array<char, 12>>::call(boost::make_iterator_range(
            std::next(data.begin(), element::detect_size(element_type::string_element, data.begin(), data.end())),
            data.end()));
        return std::make_tuple(str, oid);
    }
};

// scoped javascript
template <typename TupleT>
struct get_impl<
    TupleT, std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                             is_document<std::decay_t<tuple_element_t<1, TupleT>>>::value&&
                                 std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value>> {
    template <typename RangeT> static TupleT call(const RangeT& data) {
        auto str = get_impl<std::decay_t<tuple_element_t<0, TupleT>>>::call(data);
        auto doc = get_impl<std::decay_t<tuple_element_t<1, TupleT>>>::call(boost::make_iterator_range(
            std::next(data.begin(), element::detect_size(element_type::string_element, data.begin(), data.end())),
            data.end()));
        return std::make_tuple(str, doc);
    }
};

template <typename ReturnT> struct get_impl<ReturnT, std::enable_if_t<std::is_void<ReturnT>::value>> {
    static_assert(std::is_void<ReturnT>::value, "cannot get void");
};

// setters
// integer & bool
template <typename T> struct set_impl<T, std::enable_if_t<std::is_integral<std::decay_t<T>>::value>> {
    template <typename Container> static void call(Container& data, T val) {
        boost::range::push_back(data, detail::native_to_little_endian(val));
    }
};

// floating point
template <typename T> struct set_impl<T, std::enable_if_t<std::is_floating_point<std::decay_t<T>>::value>> {
    template <typename Container> static void call(Container& data, double val) {
        boost::range::push_back(data, detail::native_to_little_endian(val));
    }
};

// string
template <typename T>
struct set_impl<T, std::enable_if_t<std::is_constructible<boost::string_ref, std::decay_t<T>>::value &&
                                    !is_document<typename std::decay<T>::type>::value>> {
    template <typename Container> static void call(Container& data, boost::string_ref val) {
        auto old_size = data.size();
        boost::range::push_back(data, detail::native_to_little_endian(static_cast<int32_t>(val.size() + 1)));
        boost::range::push_back(data, val);
        data.push_back('\0');
        if(data.size() - old_size != val.size() + sizeof(int32_t) + sizeof('\0'))
            BOOST_THROW_EXCEPTION(invalid_element_size{});
    }
};

// date
template <typename DateT>
struct set_impl<DateT, std::enable_if_t<std::is_arithmetic<typename std::decay_t<DateT>::rep>::value&&
                                            std::is_arithmetic<typename std::decay_t<DateT>::clock::rep>::value>> {
    template <typename Container> static void call(Container& data, DateT val) {
        auto old_size = data.size();
        boost::range::push_back(data,
                                detail::native_to_little_endian(static_cast<int64_t>(val.time_since_epoch().count())));
        if(data.size() - old_size != sizeof(int64_t))
            BOOST_THROW_EXCEPTION(invalid_element_size{});
    }
};

// oid
template <typename T> struct set_impl<T, std::enable_if_t<std::is_same<std::decay_t<T>, std::array<char, 12>>::value>> {
    template <typename Container> static void call(Container& data, T val) {
        auto old_size = data.size();
        boost::range::push_back(data, val);
        if(data.size() - old_size != val.size())
            BOOST_THROW_EXCEPTION(invalid_element_size{});
    }
};

// regex
template <class TupleT>
struct set_impl<TupleT,
                std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                                 std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value&&
                                     std::is_same<tuple_element_t<0, TupleT>, tuple_element_t<1, TupleT>>::value>> {
    template <typename Container> static void call(Container& data, TupleT&& val) {
        boost::push_back(data, std::get<0>(val));
        data.push_back('\0');
        boost::push_back(data, std::get<1>(val));
        data.push_back('\0');
    }
};

// db_pointer
template <class TupleT>
struct set_impl<
    TupleT, std::enable_if_t<std::tuple_size<std::decay_t<TupleT>>::value == 2 &&
                             std::is_same<std::array<char, 12>, std::decay_t<tuple_element_t<1, TupleT>>>::value&&
                                 std::is_constructible<std::string, std::decay_t<tuple_element_t<0, TupleT>>>::value>> {
    template <typename Container> static void call(Container& data, TupleT&& val) {
        set_impl<std::decay_t<tuple_element_t<0, TupleT>>>::call(data, std::get<0>(val));
        set_impl<std::decay_t<tuple_element_t<1, TupleT>>>::call(data, std::get<1>(val));
    }
};

template <typename T> struct set_impl<T, std::enable_if_t<std::is_void<T>::value>> {
    static_assert(std::is_void<T>::value, "cannot set void");
};

// endian shit
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
    if(EType != elem.type())
        BOOST_THROW_EXCEPTION(incompatible_element_conversion{});

    return detail::get_impl<ReturnT>::call(elem.m_data);
}

template <typename ReturnT, typename Container> ReturnT get(const basic_element<Container>& elem) {
    return detail::get_impl<ReturnT>::call(elem.m_data);
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
            BOOST_THROW_EXCEPTION(invalid_element_type{});
    };

    return os;
}

} // namespace jbson

#endif // JBSON_ELEMENT_HPP
