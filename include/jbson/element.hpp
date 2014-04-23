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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdocumentation"
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/mpl/or.hpp>
#pragma GCC diagnostic pop

#include "element_fwd.hpp"
#include "document_fwd.hpp"
#include "detail/error.hpp"
#include "detail/get.hpp"
#include "detail/set.hpp"
#include "detail/traits.hpp"
#include "detail/visit.hpp"

namespace jbson {

namespace detail {

template <element_type EType, typename Element> struct typeid_visitor {
    template <typename... Args> std::type_index operator()(Args&&...) const {
        return typeid(ElementTypeMap<EType, typename std::decay_t<Element>::container_type>);
    }
};

} // namespace detail

/*!
 * \brief Access element value of specific element_type
 */
template <element_type EType, typename Container>
auto get(const basic_element<Container>& elem) -> detail::ElementTypeMap<EType, Container>;

/*!
 * blah blah
 */
template <class Container> struct basic_element {
    using container_type = std::decay_t<Container>;
    static_assert(!std::is_convertible<container_type, std::string>::value,
                  "container_type must not be a string type (or convertible)");
    static_assert(std::is_same<typename container_type::value_type, char>::value,
                  "container_type's value_type must be char");
    static_assert(detail::is_nothrow_swappable<container_type>::value, "container_type must have noexcept swap()");

    basic_element() noexcept(std::is_nothrow_default_constructible<container_type>::value) = default;

    template <typename OtherContainer>
    basic_element(const basic_element<OtherContainer>&,
                  std::enable_if_t<!std::is_constructible<container_type, OtherContainer>::value>* = nullptr);
    template <typename OtherContainer>
    basic_element(const basic_element<OtherContainer>&,
                  std::enable_if_t<std::is_constructible<container_type, OtherContainer>::value>* = nullptr);

    template <typename OtherContainer>
    basic_element(basic_element<OtherContainer>&&,
                  std::enable_if_t<std::is_constructible<container_type, OtherContainer&&>::value>* = nullptr);

    template <typename ForwardRange>
    explicit basic_element(
        ForwardRange&&, std::enable_if_t<!std::is_constructible<std::string, ForwardRange>::value>* = nullptr,
        std::enable_if_t<detail::is_range_of_same_value<ForwardRange, typename Container::value_type>::value>* =
            nullptr);

    template <typename ForwardIterator>
    basic_element(ForwardIterator&& first, ForwardIterator&& last,
                  std::enable_if_t<
                      !std::is_constructible<boost::string_ref, ForwardIterator>::value ||
                      std::is_convertible<ForwardIterator, typename container_type::const_iterator>::value>* = nullptr,
                  std::enable_if_t<detail::is_range_of_same_value<decltype(boost::make_iterator_range(first, last)),
                                                                  typename Container::value_type>::value>* = nullptr)
        : basic_element(boost::make_iterator_range(first, last)) {}

    template <typename T> basic_element(std::string name, element_type type, T&& val) : basic_element(std::move(name)) {
        value(type, std::forward<T>(val));
    }
    template <typename ForwardIterator> basic_element(std::string, element_type, ForwardIterator, ForwardIterator);
    explicit basic_element(std::string, element_type = element_type::null_element);

    template <size_t N>
    explicit basic_element(const char (&name)[N], element_type type = element_type::null_element)
        : basic_element(std::string(name, N - 1), type) {}

    template <typename T> basic_element(std::string name, T&& val) : basic_element(std::move(name)) {
        value(std::forward<T>(val));
    }

    template <size_t N, typename T>
    basic_element(const char (&name)[N], T&& val)
        : basic_element(std::string(name, N - 1)) {
        value(std::forward<T>(val));
    }

    template <size_t N1, size_t N2>
    basic_element(const char (&name)[N1], const char (&val)[N2])
        : basic_element(std::string(name, N1 - 1)) {
        value(boost::string_ref(val, N2 - 1));
    }

    // field name
    boost::string_ref name() const { return m_name; }
    void name(std::string n) { m_name = std::move(n); }
    // size in bytes
    size_t size() const noexcept;

    element_type type() const noexcept { return m_type; }
    void type(element_type type) noexcept { m_type = type; }

    /*!
     * \brief access value
     */
    template <typename T> T value() const {
        T ret{};
        deserialise(m_data, ret);
        return std::move(ret);
    }

    void value(boost::string_ref val) { value<element_type::string_element>(val); }
    void value(std::string val) {
        value<element_type::string_element>(boost::string_ref{val});
    }
    void value(const char* val) { value(boost::string_ref(val)); }

    void value(bool val) { value<element_type::boolean_element>(val); }
    void value(float val) { value<element_type::double_element>(val); }
    void value(double val) { value<element_type::double_element>(val); }
    void value(int64_t val) { value<element_type::int64_element>(val); }
    void value(int32_t val) { value<element_type::int32_element>(val); }

    void value(const builder& val) { value<element_type::document_element>(val); }
    void value(const array_builder& val) { value<element_type::array_element>(val); }

    void value(builder&& val) { value<element_type::document_element>(std::move(val)); }
    void value(array_builder&& val) { value<element_type::array_element>(std::move(val)); }

    template <typename T> void value(T&& val) {
        container_type data;
        detail::visit<detail::set_visitor>(m_type, data, data.end(), std::forward<T>(val));
        using std::swap;
        swap(m_data, data);
    }

    template <typename T> void value(element_type type, T&& val) {
        const auto old_type = m_type;
        m_type = type;
        try {
            value<T>(std::forward<T>(val));
            assert(m_type == type);
        }
        catch(...) {
            m_type = old_type;
            throw;
        }
    }

    template <element_type EType, typename T>
    void
    value(T&& val,
          std::enable_if_t<std::is_same<std::decay_t<T>, detail::ElementTypeMapSet<EType, container_type>>::value>* =
              nullptr) {
        static_assert(detail::container_has_push_back<container_type>::value, "");
        using T2 = ElementTypeMapSet<EType>;
        static_assert(std::is_same<std::decay_t<T>, T2>::value, "");

        container_type data;
        auto it = data.end();
        serialise(data, it, std::forward<T>(val));
        using std::swap;
        swap(m_data, data);
        type(EType);
    }

    template <element_type EType, typename T>
    void
    value(T&& val,
          std::enable_if_t<!std::is_same<std::decay_t<T>, detail::ElementTypeMapSet<EType, container_type>>::value>* =
              nullptr) {
        static_assert(detail::container_has_push_back<container_type>::value, "");
        using T2 = ElementTypeMapSet<EType>;
        static_assert(std::is_constructible<T2, T>::value || std::is_convertible<std::decay_t<T>, T2>::value, "");

        container_type data;
        auto it = data.end();
        serialise(data, it, static_cast<T2>(std::forward<T>(val)));
        using std::swap;
        swap(m_data, data);
        type(EType);
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

    //! \brief Constructs a BSON element without data in place into a container.
    static void write_to_container(container_type&, typename container_type::const_iterator, boost::string_ref,
                                   element_type);
    //! \brief Constructs a type-deduced BSON element in place into a container.
    template <typename T>
    static void write_to_container(container_type&, typename container_type::const_iterator, boost::string_ref, T&&);
    //! \brief Constructs a BSON element in place into a container.
    template <typename T>
    static void write_to_container(container_type&, typename container_type::const_iterator, boost::string_ref,
                                   element_type, T&&);

    //! \brief Writes data to BSON.
    template <typename OutContainer>
    void write_to_container(OutContainer&, typename OutContainer::const_iterator) const;
    template <typename OutContainer> explicit operator OutContainer() const;

    bool operator==(const basic_element& other) const {
        return m_name == other.m_name && m_type == other.m_type && boost::range::equal(m_data, other.m_data);
    }
    bool operator!=(const basic_element& other) const { return !(*this == other); }

    bool operator<(const basic_element& other) const {
        auto res = name().compare(other.name());
        if(res == 0 && type() == other.type()) {
            if(type() == element_type::double_element)
                return value<double>() < other.value<double>();
            if(type() == element_type::string_element)
                return ::strcoll(get<element_type::string_element>(*this).data(),
                                 get<element_type::string_element>(other).data()) < 0;
            return m_data < other.m_data;
        }
        return res < 0;
    }

    void swap(basic_element& other) noexcept {
        using std::swap;
        swap(m_name, other.m_name);
        swap(m_type, other.m_type);
        swap(m_data, other.m_data);
    }

  private:
    std::string m_name;
    element_type m_type{element_type::null_element};
    container_type m_data;

    template <element_type EType> using ElementTypeMap = detail::ElementTypeMap<EType, container_type>;
    template <element_type EType> using ElementTypeMapSet = detail::ElementTypeMapSet<EType, container_type>;
    template <typename T> static bool valid_type(element_type);
    template <typename T> bool valid_type() const { return valid_type<T>(type()); }
    template <typename T> static bool valid_set_type(element_type);
    template <typename T> bool valid_set_type() const { return valid_set_type<T>(type()); }

    template <typename> friend struct basic_element;
    template <element_type EType, typename T>
    friend auto get(const basic_element<T>& elem) -> detail::ElementTypeMap<EType, T>;
};

template <typename Container>
void swap(basic_element<Container>& a, basic_element<Container>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

template <class Container>
template <typename OutContainer>
void basic_element<Container>::write_to_container(OutContainer& c, typename OutContainer::const_iterator it) const {
    static_assert(std::is_same<typename OutContainer::value_type, char>::value, "");
    if(!(bool)m_type)
        BOOST_THROW_EXCEPTION(invalid_element_type{});

    it = std::next(c.insert(it, static_cast<uint8_t>(m_type)));
    it = c.insert(it, m_name.begin(), m_name.end());
    std::advance(it, m_name.size());
    it = std::next(c.insert(it, '\0'));

    if(detail::detect_size(m_type, m_data.begin(), m_data.end()) != static_cast<ptrdiff_t>(boost::distance(m_data)))
        BOOST_THROW_EXCEPTION(invalid_element_size{}
                              << detail::actual_size(static_cast<ptrdiff_t>(boost::distance(m_data)))
                              << detail::expected_size(detail::detect_size(m_type, m_data.begin(), m_data.end())));
    c.insert(it, m_data.begin(), m_data.end());
}

namespace detail {

inline element_type deduce_type(const boost::string_ref&) noexcept { return element_type::string_element; }
inline element_type deduce_type(const std::string&) noexcept { return element_type::string_element; }
inline element_type deduce_type(const char*) noexcept { return element_type::string_element; }

inline element_type deduce_type(bool) noexcept { return element_type::boolean_element; }
inline element_type deduce_type(float) noexcept { return element_type::double_element; }
inline element_type deduce_type(double) noexcept { return element_type::double_element; }
inline element_type deduce_type(int64_t) noexcept { return element_type::int64_element; }
inline element_type deduce_type(int32_t) noexcept { return element_type::int32_element; }

inline element_type deduce_type(const builder&) noexcept { return element_type::document_element; }
inline element_type deduce_type(const array_builder&) noexcept { return element_type::array_element; }

inline element_type deduce_type(builder&&) noexcept { return element_type::document_element; }
inline element_type deduce_type(array_builder&&) noexcept { return element_type::array_element; }

template <typename T> inline element_type deduce_type(T&&) noexcept { return static_cast<element_type>(0); }

} // namespace detail

/*!
 * Performs basic type deduction for JSON types only.
 * Any other type results in an invalid element_type being passed to typed overload.
 *
 * \throws invalid_element_type When deduced type is invalid
 */
template <typename Container>
template <typename T>
void basic_element<Container>::write_to_container(container_type& c, typename container_type::const_iterator it,
                                                  boost::string_ref name, T&& val) {
    static_assert(!std::is_same<element_type, T>::value, "");
    write_to_container(c, it, name.to_string(), detail::deduce_type(std::forward<T>(val)), std::forward<T>(val));
}

/*!
 * \throws invalid_element_type When type is invalid
 * \throws incompatible_type_conversion When type is void, i.e. should not contain data.
 */
template <typename Container>
template <typename T>
void basic_element<Container>::write_to_container(container_type& c, typename container_type::const_iterator it,
                                                  boost::string_ref name, element_type type, T&& val) {
    if(!(bool)type)
        BOOST_THROW_EXCEPTION(invalid_element_type{});

    it = std::next(c.insert(it, static_cast<uint8_t>(type)));
    it = c.insert(it, name.begin(), name.end());
    std::advance(it, name.size());
    it = std::next(c.insert(it, '\0'));

    detail::visit<detail::set_visitor>(type, c, it, std::forward<T>(val));
}

/*!
 * \throws invalid_element_type When type is invalid
 * \throws incompatible_type_conversion When type is non-void, i.e. should contain data.
 */
template <typename Container>
void basic_element<Container>::write_to_container(container_type& c, typename container_type::const_iterator it,
                                                  boost::string_ref name, element_type type) {
    if(!(bool)type)
        BOOST_THROW_EXCEPTION(invalid_element_type{});

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    if(type != element_type::undefined_element && type != element_type::null_element && type != element_type::min_key &&
       type != element_type::max_key)
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{}
                              << detail::actual_type(typeid(void))
                              << detail::expected_type(detail::visit<detail::typeid_visitor>(type, basic_element())));
#pragma GCC diagnostic pop

    it = std::next(c.insert(it, static_cast<uint8_t>(type)));
    it = c.insert(it, name.begin(), name.end());
    std::advance(it, name.size());
    it = std::next(c.insert(it, '\0'));
}

template <class Container> template <typename OutContainer> basic_element<Container>::operator OutContainer() const {
    OutContainer c;
    write_to_container(c, c.end());
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
    std::enable_if_t<std::is_constructible<container_type, OtherContainer&&>::value>*)
    : m_name(std::move(elem.m_name)), m_type(std::move(elem.m_type)), m_data(std::move(elem.m_data)) {}

template <class Container>
template <typename ForwardRange>
basic_element<Container>::basic_element(
    ForwardRange&& range, std::enable_if_t<!std::is_constructible<std::string, ForwardRange>::value>*,
    std::enable_if_t<detail::is_range_of_same_value<ForwardRange, typename Container::value_type>::value>*) {
    if(boost::distance(range) < 2)
        BOOST_THROW_EXCEPTION(invalid_element_size{} << detail::expected_size(2)
                                                     << detail::actual_size(boost::distance(range)));

    auto first = std::begin(range), last = std::end(range);
    m_type = static_cast<element_type>(*first++);
    if(!(bool)m_type)
        BOOST_THROW_EXCEPTION(invalid_element_type{});
    auto str_end = std::find(first, last, '\0');
    m_name.assign(first, str_end++);
    first = str_end;
    const auto elem_size = detail::detect_size(m_type, first, last);
    if(std::distance(first, last) < elem_size)
        BOOST_THROW_EXCEPTION(invalid_element_size{} << detail::expected_size(elem_size)
                                                     << detail::actual_size(std::distance(first, last)));
    last = std::next(first, elem_size);
    m_data = container_type{first, last};
}

template <class Container>
template <typename ForwardIterator>
basic_element<Container>::basic_element(std::string name, element_type type, ForwardIterator first,
                                        ForwardIterator last)
    : m_name(std::move(name)), m_type(type) {
    if(!(bool)m_type)
        BOOST_THROW_EXCEPTION(invalid_element_type{});
    last = std::next(first, detail::detect_size(m_type, first, last));
    m_data = Container{first, last};
}

template <class Container>
basic_element<Container>::basic_element(std::string name, element_type type)
    : m_name(std::move(name)), m_type(type) {
    if(!(bool)m_type)
        BOOST_THROW_EXCEPTION(invalid_element_type{});
}

template <class Container> size_t basic_element<Container>::size() const noexcept {
    return sizeof(m_type) + boost::distance(m_data) + m_name.size() + sizeof('\0');
}

namespace detail {

template <element_type EType, typename Visitor, typename Element> struct element_visitor {
    auto operator()(Visitor&& visitor, Element&& elem) const { return visitor(elem.name(), get<EType>(elem), EType); }
};

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
template <typename Visitor, typename Element>
struct element_visitor<element_type::undefined_element, Visitor, Element> {
    auto operator()(Visitor&& visitor, Element&& elem) const {
        return visitor(elem.name(), element_type::undefined_element);
    }
};
#pragma GCC diagnostic pop

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

#endif // DOXYGEN_SHOULD_SKIP_THIS

} // namespace detail

template <class Container>
template <typename Visitor>
void basic_element<Container>::visit(
    Visitor&& visitor,
    std::enable_if_t<std::is_void<decltype(std::declval<Visitor>()("", double {}, element_type{}))>::value>*) const {
    detail::visit<detail::element_visitor>(m_type, std::forward<Visitor>(visitor), *this);
    return;
}

template <class Container>
template <typename Visitor>
auto basic_element<Container>::visit(
    Visitor&& visitor,
    std::enable_if_t<!std::is_void<decltype(std::declval<Visitor>()("", double {}, element_type{}))>::value>*) const
    -> decltype(std::declval<Visitor>()("", double {}, element_type{})) {
    return detail::visit<detail::element_visitor>(m_type, std::forward<Visitor>(visitor), *this);
}

namespace detail {

template <typename T, typename Container, typename Enable = void> struct is_valid_func;

} // namespace detail

template <class Container> template <typename T> bool basic_element<Container>::valid_type(element_type type) {
    return detail::visit<detail::is_valid_func<T, Container>::template inner>(type);
}

template <class Container> template <typename T> bool basic_element<Container>::valid_set_type(element_type type) {
    return detail::visit<detail::is_valid_func<T, Container>::template set_inner>(type);
}

namespace detail {

struct elem_compare {
    using is_transparent = std::true_type;
    template <typename EContainer, typename EContainer2>
    bool operator()(const basic_element<EContainer>& lhs, const basic_element<EContainer2>& rhs) const {
        return lhs < rhs;
    }
    template <typename EContainer> bool operator()(const basic_element<EContainer>& lhs, boost::string_ref rhs) const {
        return lhs.name() < rhs;
    }
    template <typename EContainer> bool operator()(boost::string_ref lhs, const basic_element<EContainer>& rhs) const {
        return lhs < rhs.name();
    }
};

template <typename T, typename Container, typename Enable> struct is_valid_func {
    template <element_type EType, typename... Args>
    struct inner : mpl::or_<std::is_convertible<T, detail::ElementTypeMap<EType, Container>>,
                            std::is_constructible<detail::ElementTypeMap<EType, Container>, T>> {
        static_assert(sizeof...(Args) == 0, "");
        constexpr bool operator()() const { return inner::value; }
    };
    template <element_type EType, typename... Args>
    struct set_inner : mpl::or_<std::is_convertible<T, detail::ElementTypeMapSet<EType, Container>>,
                                std::is_constructible<detail::ElementTypeMapSet<EType, Container>, T>> {
        static_assert(sizeof...(Args) == 0, "");
        constexpr bool operator()() const { return set_inner::value; }
    };
};

using expected_element_type = boost::error_info<struct expected_element_type_, element_type>;
using actual_element_type = boost::error_info<struct actual_element_type_, element_type>;

} // namespace detail

template <element_type EType, typename Container>
auto get(const basic_element<Container>& elem) -> detail::ElementTypeMap<EType, Container> {
    if(EType != elem.type())
        BOOST_THROW_EXCEPTION(incompatible_element_conversion{} << detail::expected_element_type(EType)
                                                                << detail::actual_element_type(elem.type()));

    return elem.template value<detail::ElementTypeMap<EType, Container>>();
}

template <typename ReturnT, typename Container> ReturnT get(const basic_element<Container>& elem) {
    return elem.template value<ReturnT>();
}

template <typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<<(std::basic_ostream<CharT, TraitsT>& os, element_type e) {
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        case element_type::undefined_element:
            os << "undefined_element";
            break;
#pragma GCC diagnostic pop
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        case element_type::db_pointer_element:
            os << "db_pointer_element";
            break;
#pragma GCC diagnostic pop
        case element_type::javascript_element:
            os << "javascript_element";
            break;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        case element_type::symbol_element:
            os << "symbol_element";
            break;
#pragma GCC diagnostic pop
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
