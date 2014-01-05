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

#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/mpl/or.hpp>

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

template <class Container> struct basic_element {
    using container_type = typename std::decay<Container>::type;
    static_assert(!std::is_convertible<container_type, std::string>::value, "");
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

    template <typename ForwardRange>
    explicit basic_element(const ForwardRange&,
                           std::enable_if_t<!std::is_constructible<std::string, ForwardRange>::value>* = nullptr);
    template <typename ForwardIterator> basic_element(ForwardIterator, ForwardIterator);

    template <typename T>
    basic_element(std::string name, element_type type, T&& val)
        : basic_element(std::move(name), type) {
        value(std::forward<T>(val));
    }
    template <typename ForwardIterator> basic_element(std::string, element_type, ForwardIterator, ForwardIterator);
    basic_element(std::string, element_type = element_type::null_element);

    // field name
    boost::string_ref name() const { return m_name; }
    void name(std::string n) { m_name = std::move(n); }
    // size in bytes
    size_t size() const noexcept;

    element_type type() const noexcept { return m_type; }
    void type(element_type type) noexcept { m_type = type; }

    template <typename T> T value() const {
        if(!valid_type<T>())
            BOOST_THROW_EXCEPTION(incompatible_type_conversion{}
                                  << actual_type(typeid(T))
                                  << expected_type(detail::visit<detail::typeid_visitor>(m_type, *this)));
        return detail::get_impl<T>::call(m_data);
    }

    template <typename T> void value(T&& val) {
        if(!valid_type<T>())
            BOOST_THROW_EXCEPTION(incompatible_type_conversion{}
                                  << actual_type(typeid(T))
                                  << expected_type(detail::visit<detail::typeid_visitor>(m_type, *this)));
        decltype(m_data) data;
        detail::visit<detail::set_visitor>(m_type, data, std::forward<T>(val));
        using std::swap;
        swap(m_data, data);
    }

    template <typename T> void value(element_type type, T&& val) {
        this->type(type);
        value(std::forward<T>(val));
    }

    template <element_type EType, typename T>
    void value(T&& val,
               std::enable_if_t<std::is_same<std::decay_t<T>, detail::ElementTypeMap<EType, container_type>>::value>* =
                   nullptr) {
        using T2 = ElementTypeMap<EType>;
        static_assert(std::is_same<std::decay_t<T>, T2>::value, "");
        type(EType);
        decltype(m_data) data;
        detail::set_visitor<EType, container_type, T2> {}
        (data, std::forward<T>(val));
        using std::swap;
        swap(m_data, data);
    }

    template <element_type EType, typename T>
    void value(T&& val,
               std::enable_if_t<!std::is_same<std::decay_t<T>, detail::ElementTypeMap<EType, container_type>>::value>* =
                   nullptr) {
        using T2 = ElementTypeMap<EType>;
        static_assert(std::is_constructible<T2, T>::value || std::is_convertible<std::decay_t<T>, T2>::value, "");
        type(EType);
        decltype(m_data) data;
        detail::set_visitor<EType, container_type, T2> {}
        (data, T2(std::forward<T>(val)));
        using std::swap;
        swap(m_data, data);
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
    template <typename T> static bool valid_type(element_type);
    template <typename T> bool valid_type() const { return valid_type<T>(type()); }

    template <typename ReturnT, typename> friend struct detail::get_impl;
    template <element_type EType, typename C, typename T, typename> friend struct detail::set_impl;

    template <typename> friend struct basic_element;
    template <element_type EType, typename T>
    friend auto get(const basic_element<T>& elem) -> detail::ElementTypeMap<EType, T>;
};

template <element_type EType, typename Container>
auto get(const basic_element<Container>& elem) -> detail::ElementTypeMap<EType, Container>;

template <class Container>
template <typename OutContainer>
void basic_element<Container>::write_to_container(OutContainer& c) const {
    static_assert(std::is_same<typename OutContainer::value_type, char>::value, "");
    if(!m_type)
        BOOST_THROW_EXCEPTION(invalid_element_type{});
    c.push_back(static_cast<uint8_t>(m_type));
    boost::range::push_back(c, m_name);
    c.push_back('\0');
    if(detail::detect_size(m_type, m_data.begin(), m_data.end()) != static_cast<ptrdiff_t>(boost::distance(m_data)))
        BOOST_THROW_EXCEPTION(
            invalid_element_size{} << actual_size(static_cast<ptrdiff_t>(boost::distance(m_data)))
                                   << expected_size(detail::detect_size(m_type, m_data.begin(), m_data.end())));
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
    const auto elem_size = detail::detect_size(m_type, first, last);
    if(std::distance(first, last) < elem_size)
        BOOST_THROW_EXCEPTION(invalid_element_size{} << expected_size(elem_size)
                                                     << actual_size(std::distance(first, last)));
    last = std::next(first, elem_size);
    m_data = container_type{first, last};
}

template <class Container>
template <typename ForwardRange>
basic_element<Container>::basic_element(const ForwardRange& range,
                                        std::enable_if_t<!std::is_constructible<std::string, ForwardRange>::value>*)
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
    last = std::next(first, detail::detect_size(m_type, first, last));
    m_data = Container{first, last};
}

template <class Container>
basic_element<Container>::basic_element(std::string name, element_type type)
    : m_name(std::move(name)), m_type(type) {
    if(!m_type)
        BOOST_THROW_EXCEPTION(invalid_element_type{});
}

template <class Container> size_t basic_element<Container>::size() const noexcept {
    return sizeof(m_type) + boost::distance(m_data) + m_name.size() + sizeof('\0');
}

namespace detail {

template <element_type EType, typename Visitor, typename Element> struct element_visitor {
    auto operator()(Visitor&& visitor, Element&& elem) const {
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
    std::enable_if_t<!std::is_void<decltype(std::declval<Visitor>()("", double {}, element_type{}))>::value>*)
    const -> decltype(std::declval<Visitor>()("", double {}, element_type{})) {
    return detail::visit<detail::element_visitor>(m_type, std::forward<Visitor>(visitor), *this);
}

namespace detail {

template <typename T, typename Container, typename Enable = void> struct is_valid_func;

} // namespace detail

template <class Container> template <typename T> bool basic_element<Container>::valid_type(element_type type) {
    return detail::visit<detail::is_valid_func<T, Container>::template inner>(type);
}

namespace detail {

struct elem_string_compare {
    using is_transparent = std::true_type;
    template <typename EContainer, typename EContainer2>
    bool operator()(const basic_element<EContainer>& lhs, const basic_element<EContainer2>& rhs) const {
        return lhs.name() < rhs.name();
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
        constexpr bool operator()() const {
            return inner::value;
        }
    };
};

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
