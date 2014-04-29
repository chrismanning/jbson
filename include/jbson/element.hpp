//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_ELEMENT_HPP
#define JBSON_ELEMENT_HPP

#include <string>
#include <vector>

#include "detail/config.hpp"

JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/mpl/or.hpp>
JBSON_CLANG_POP_WARNINGS

#include "element_fwd.hpp"
#include "document_fwd.hpp"
#include "detail/error.hpp"
#include "detail/get.hpp"
#include "detail/set.hpp"
#include "detail/traits.hpp"
#include "detail/visit.hpp"

JBSON_PUSH_DISABLE_DEPRECATED_WARNING

namespace jbson {
namespace detail {

//! Visitor for obtaining std::type_index of a mapped element_type.
template <element_type EType, typename Element, typename Enable = void> struct typeid_visitor;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
template <element_type EType, typename Element>
struct typeid_visitor<EType, Element, std::enable_if_t<is_element<std::decay_t<Element>>::value>> {
    //! Functor call operator
    template <typename... Args> std::type_index operator()(Args&&...) const {
        return typeid(ElementTypeMap<EType, typename std::decay_t<Element>::container_type>);
    }
};

template <element_type EType, typename Element>
struct typeid_visitor<EType, Element, std::enable_if_t<!is_element<std::decay_t<Element>>::value>> {
    //! Functor call operator
    template <typename... Args> std::type_index operator()(Args&&...) const {
        return typeid(ElementTypeMap<EType, std::decay_t<Element>>);
    }
};
#endif // DOXYGEN_SHOULD_SKIP_THIS

} // namespace detail

/*!
 * \tparam Container Type of the underlying data storage.
 *                   Must be range of `char`. Must be detectably noexcept swappable.
 * \internal
 * Currently stores the name and type separately from the value data.
 * This is to simplify the implementation and the exception safety of it.
 *
 * However, should the storage be unified it would eliminate std::string copies for all non-data-owning elements.
 * It would also change the semantics to be more reasonable. Modifying the name of an element does not change the
 * name in the underlying storage, and should thus be disallowed. The only way to enforce this is by using const
 * basic_elements everywhere, or perhaps some tricky metaprogramming, or unified storage.
 */
template <class Container> struct basic_element {
    //! Underlying storage container.
    using container_type = std::decay_t<Container>;
    static_assert(!std::is_convertible<container_type, std::string>::value,
                  "container_type must not be a string type (or convertible)");
    static_assert(std::is_same<typename container_type::value_type, char>::value,
                  "container_type's value_type must be char");
    static_assert(detail::is_nothrow_swappable<container_type>::value, "container_type must have noexcept swap()");

    /*!
     * \brief Default constructor.
     *
     * Resultant basic_element represents a valid, empty-named, element_type::null_element typed BSON element.
     */
    basic_element() = default;

    //! \brief Copy constructor.
    template <typename OtherContainer>
    basic_element(const basic_element<OtherContainer>&,
                  std::enable_if_t<std::is_constructible<container_type, OtherContainer>::value>* = nullptr);

    //! \brief Copy constructor.
    template <typename OtherContainer>
    basic_element(const basic_element<OtherContainer>&,
                  std::enable_if_t<!std::is_constructible<container_type, OtherContainer>::value>* = nullptr,
                  std::enable_if_t<std::is_constructible<container_type, typename OtherContainer::const_iterator,
                                                         typename OtherContainer::const_iterator>::value>* = nullptr);

    //! \brief Move constructor.
    template <typename OtherContainer>
    basic_element(basic_element<OtherContainer>&&,
                  std::enable_if_t<std::is_constructible<container_type, OtherContainer&&>::value>* = nullptr);

    //! \brief Construct an element from raw BSON byte sequence.
    template <typename ForwardRange>
    explicit basic_element(
        ForwardRange&&, std::enable_if_t<!std::is_constructible<std::string, ForwardRange>::value>* = nullptr,
        std::enable_if_t<detail::is_range_of_same_value<ForwardRange, typename Container::value_type>::value>* =
            nullptr);

    //! \brief Construct an element from raw BSON byte sequence.
    /*!
     * \param first Iterator to beginning of data.
     * \param last Iterator past end of data.
     * \throws invalid_element_size When range is too small.
     *                              When value data is indicated to be larger than range.
     * \throws invalid_element_type When the detected element_type is invalid.
     */
    template <typename ForwardIterator>
    basic_element(ForwardIterator&& first, ForwardIterator&& last,
                  std::enable_if_t<
                      !std::is_constructible<boost::string_ref, ForwardIterator>::value ||
                      std::is_convertible<ForwardIterator, typename container_type::const_iterator>::value>* = nullptr,
                  std::enable_if_t<detail::is_range_of_same_value<decltype(boost::make_iterator_range(first, last)),
                                                                  typename Container::value_type>::value>* = nullptr)
        : basic_element(boost::make_iterator_range(first, last)) {}

    /*!
     * \brief Construct an element with specified name, type and value.
     *
     * \throws invalid_element_type When supplied type is invalid.
     * \throws incompatible_type_conversion When type is value-less, e.g. null_element.
     */
    template <typename T> basic_element(std::string name, element_type type, T&& val) : basic_element(std::move(name)) {
        value(type, std::forward<T>(val));
    }

    //! \brief Construct an element with specified name, type and value data.
    template <typename ForwardIterator> basic_element(std::string, element_type, ForwardIterator, ForwardIterator);

    //! \brief Construct an element with specified name, and optionally, type.
    explicit basic_element(std::string, element_type = element_type::null_element);

    /*!
     * \brief Construct an element with specified name and value.
     *
     * Attempts type deduction to determine suitable element_type.
     *
     * \param name Element name.
     * \param val JSON compatible value.
     * \throws incompatible_type_conversion When type deduction fails.
     */
    template <typename T> basic_element(std::string name, T&& val) : basic_element(std::move(name)) {
        value(std::forward<T>(val));
    }

    /*!
     * \brief Returns name of element.
     *
     * \return boost::string_ref. The basic_element must remain alive at least as long as the returned string.
     */
    boost::string_ref name() const
        noexcept(std::is_nothrow_constructible<boost::string_ref, const std::string&>::value) {
        return m_name;
    }

    /*!
     * \brief Sets name to n.
     * \param[in] n Name to set element to.
     * \warning Strong exception guarantee (parameter \p n construction could throw).
     */
    void name(std::string n) { m_name.swap(n); }

    //! Returns size in bytes.
    size_t size() const noexcept;

    //! Returns BSON type of this element.
    element_type type() const noexcept { return m_type; }

    /*!
     * \brief Sets type of this element.
     *
     * \note No checks are performed determining if the current value data is compatible with the new type.
     *
     * \param new_type Valid BSON element_type.
     * \throws invalid_element_type When type is invalid.
     *
     * \warning Strong exception guarantee.
     */
    void type(element_type new_type) {
        if(!detail::valid_type(new_type))
            BOOST_THROW_EXCEPTION(invalid_element_type{});
        m_type = new_type;
    }

    /*!
     * \brief Returns the value data in the form of a specific type.
     *
     * \note This overload only returns types from detail::ElementTypeMap.
     *
     * \tparam T The value type to be returned. Must be default constructible.
     * \returns Value represented by this basic_element in the form of a \p T.
     * \throws incompatible_type_conversion When \p ReturnT is incompatible with the current element_type.
     *
     * \sa detail::ElementTypeMap
     */
    template <typename T>
    std::enable_if_t<detail::is_valid_element_value_type<container_type, T>::value, T> value() const {
        static_assert(!std::is_void<T>::value, "Cannot assign value to void.");
        static_assert(std::is_default_constructible<T>::value, "Return type must be default constructible.");

        if(!valid_type<T>(type()))
            BOOST_THROW_EXCEPTION(incompatible_type_conversion{}
                                  << detail::actual_type(typeid(T))
                                  << detail::expected_type(detail::visit<detail::typeid_visitor>(type(), *this)));

        T ret{};
        detail::deserialise(m_data, ret);
        return std::move(ret);
    }

    /*!
     * \brief Returns the value data in the form of a specific type.
     *
     * \note This overload requires a compatible value_get function overload. It is found via ADL.
     *
     * \tparam T The value type to be returned. Must be default constructible.
     * \returns Value represented by this basic_element in the form of a \p T.
     * \throws incompatible_type_conversion When \p ReturnT is incompatible with the current element_type.
     *
     * \sa value_get
     */
    template <typename T>
    std::enable_if_t<!detail::is_valid_element_value_type<container_type, T>::value, T> value() const {
        static_assert(!std::is_void<T>::value, "Cannot assign value to void.");
        static_assert(std::is_default_constructible<T>::value, "Return type must be default constructible.");
        T ret{};
        value_get(*this, ret);
        return std::move(ret);
    }

    //! \brief Sets value and type to element_type::string_element.
    //! \warning Strong exception guarantee.
    void value(boost::string_ref val) { value<element_type::string_element>(val); }
    //! \brief Sets value and type to element_type::string_element.
    //! \warning Strong exception guarantee.
    void value(std::string val) { value(boost::string_ref(val)); }
    //! \brief Sets value and type to element_type::string_element.
    //! \warning Strong exception guarantee.
    void value(const char* val) { value(boost::string_ref(val)); }

    //! \brief Sets value and type to element_type::boolean_element.
    //! \warning Strong exception guarantee.
    void value(bool val) { value<element_type::boolean_element>(val); }
    //! \brief Sets value and type to element_type::double_element.
    //! \warning Strong exception guarantee.
    void value(float val) { value<element_type::double_element>(val); }
    //! \brief Sets value and type to element_type::double_element.
    //! \warning Strong exception guarantee.
    void value(double val) { value<element_type::double_element>(val); }
    //! \brief Sets value and type to element_type::int64_element.
    //! \warning Strong exception guarantee.
    void value(int64_t val) { value<element_type::int64_element>(val); }
    //! \brief Sets value and type to element_type::int32_element.
    //! \warning Strong exception guarantee.
    void value(int32_t val) { value<element_type::int32_element>(val); }

    //! \brief Sets value and type to element_type::document_element.
    //! \warning Strong exception guarantee.
    void value(const builder& val) { value<element_type::document_element>(val); }
    //! \brief Sets value and type to element_type::array_element.
    //! \warning Strong exception guarantee.
    void value(const array_builder& val) { value<element_type::array_element>(val); }

    //! \brief Sets value and type to element_type::document_element.
    //! \warning Strong exception guarantee.
    void value(builder&& val) { value<element_type::document_element>(std::move(val)); }
    //! \brief Sets value and type to element_type::array_element.
    //! \warning Strong exception guarantee.
    void value(array_builder&& val) { value<element_type::array_element>(std::move(val)); }

    //! \brief Sets value and type to element_type::document_element.
    //! \warning Strong exception guarantee.
    template <typename C, typename C2> void value(const basic_document<C, C2>& val) {
        value<element_type::document_element>(val);
    }
    //! \brief Sets value and type to element_type::array_element.
    //! \warning Strong exception guarantee.
    template <typename C, typename C2> void value(const basic_array<C, C2>& val) {
        value<element_type::array_element>(val);
    }

    //! \brief Sets value and type to element_type::document_element.
    //! \warning Strong exception guarantee.
    template <typename C, typename C2> void value(basic_document<C, C2>&& val) {
        value<element_type::document_element>(std::move(val));
    }
    //! \brief Sets value and type to element_type::array_element.
    //! \warning Strong exception guarantee.
    template <typename C, typename C2> void value(basic_array<C, C2>&& val) {
        value<element_type::array_element>(std::move(val));
    }

    /*!
     * \brief Sets value to an undetermined type.
     *
     * \param val Value of undeducable type.
     * \note Attempts to convert \p val to the type as determined by detail::ElementTypeMapSet
     * and the currently set element_type.
     * \warning Strong exception guarantee.
     * \throws invalid_element_size When detected size differs from size of data.
     * \sa detail::ElementTypeMapSet detail::serialise()
     */
    template <typename T>
    void value(T&& val, std::enable_if_t<detail::is_valid_element_set_type<container_type, T>::value>* = nullptr) {
        container_type data;
        detail::visit<detail::set_visitor>(m_type, data, data.end(), std::forward<T>(val));

        if(detail::detect_size(m_type, data.begin(), data.end()) != static_cast<ptrdiff_t>(boost::distance(data)))
            BOOST_THROW_EXCEPTION(invalid_element_size{}
                                  << detail::actual_size(static_cast<ptrdiff_t>(boost::distance(m_data)))
                                  << detail::expected_size(detail::detect_size(m_type, m_data.begin(), m_data.end())));
        using std::swap;
        swap(m_data, data);
    }

    template <typename T>
    void value(T&& val, std::enable_if_t<!detail::is_valid_element_set_type<container_type, T>::value>* = nullptr) {
        value_set(*this, std::forward<T>(val));
    }

    /*!
     * \brief Sets element_type and value.
     *
     * \param new_type element_type of supplied value.
     * \param val Value of undetermined type.
     * \note Does not perform type deduction. Assumes \p new_type is correct type for \p val.
     * \note Attempts to convert \p val to the type as determined by detail::ElementTypeMapSet
     * and the currently set element_type.
     * \warning Strong exception guarantee.
     * \sa detail::serialise() value(T&&val)
     */
    template <typename T> void value(element_type new_type, T&& val) {
        const auto old_type = m_type;
        type(new_type);
        try {
            value<T>(std::forward<T>(val));
            assert(m_type == new_type);
        }
        catch(...) {
            m_type = old_type;
            throw;
        }
    }

    /*!
     * \brief Sets value. Statically ensures type compatibility.
     * \tparam EType element_type to set and check against.
     * \param val Value to set.
     * \warning Strong exception guarantee.
     * \sa detail::ElementTypeMapSet detail::serialise()
     */
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
        detail::serialise(data, it, std::forward<T>(val));
        using std::swap;
        swap(m_data, data);
        type(EType);
    }

    /*!
     * \brief Sets value. Statically ensures type compatibility.
     *
     * Performs conversion to type mapped from detail::ElementTypeMapSet.
     * \tparam EType element_type to set and check against.
     * \param val Value to set.
     * \warning Strong exception guarantee.
     * \sa detail::ElementTypeMapSet detail::serialise()
     */
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
        detail::serialise(data, it, static_cast<T2>(std::forward<T>(val)));
        using std::swap;
        swap(m_data, data);
        type(EType);
    }

    //! Apply the visitor pattern with a void-return visitor.
    template <typename Visitor>
    void visit(Visitor&&, std::enable_if_t<std::is_void<decltype(std::declval<Visitor>()(
                              "", std::declval<element_type>(), std::declval<double>()))>::value>* = nullptr) const;
    //! Apply the visitor pattern with a value-returning visitor.
    template <typename Visitor>
    auto visit(Visitor&&, std::enable_if_t<!std::is_void<decltype(std::declval<Visitor>()(
                              "", std::declval<element_type>(), std::declval<double>()))>::value>* = nullptr) const
        -> decltype(std::declval<Visitor>()("", std::declval<element_type>(), std::declval<double>()));

    //! \brief Constructs a BSON element without data in place into a container.
    static void write_to_container(container_type&, typename container_type::const_iterator, boost::string_ref,
                                   element_type);
    //! \brief Constructs a type-deduced BSON element in place into a container.
    template <typename T>
    static void write_to_container(container_type&, typename container_type::const_iterator, boost::string_ref, T&&);
    //! \brief Constructs a BSON element in place into a container.
    template <typename T>
    static void
    write_to_container(container_type&, typename container_type::const_iterator, boost::string_ref, element_type, T&&,
                       std::enable_if_t<detail::is_valid_element_set_type<container_type, T>::value>* = nullptr);

    template <typename T>
    static void
    write_to_container(container_type&, typename container_type::const_iterator, boost::string_ref, element_type, T&&,
                       std::enable_if_t<!detail::is_valid_element_set_type<container_type, T>::value>* = nullptr);

    //! \brief Transforms basic_element to BSON data.
    template <typename OutContainer>
    void write_to_container(OutContainer&, typename OutContainer::const_iterator) const;

    //! Explicit conversion to a write_to_container compatible container.
    //! \sa write_to_container
    template <typename OutContainer> explicit operator OutContainer() const;

    //! \brief Checks if this and \p other are equal.
    bool operator==(const basic_element& other) const {
        return m_name == other.m_name && m_type == other.m_type && boost::range::equal(m_data, other.m_data);
    }
    //! \brief Checks if this and \p other are not equal.
    bool operator!=(const basic_element& other) const { return !(*this == other); }

    /*!
     * \brief Checks if this is less than (<) \p other.
     *
     * When the elements have differing types, simply compares names.
     *
     * Only when both the types and names are equal are the values compared.\n
     * When the values are element_type::string_element, they are fetched and then compared according to the
     * current global locale.\n
     * When the values are element_type::double_element, they are fetched and compared with \p <.\n
     * For all other types, a lexicographical comparison of the data is performed.
     */
    bool operator<(const basic_element& other) const {
        auto res = name().compare(other.name());
        if(res == 0 && type() == other.type()) {
            if(type() == element_type::double_element)
                return value<double>() < other.value<double>();
            if(type() == element_type::string_element) {
                auto a_str = this->value<detail::ElementTypeMap<element_type::string_element, container_type>>();
                auto b_str = other.value<detail::ElementTypeMap<element_type::string_element, container_type>>();
                return std::use_facet<std::collate<char>>({}).compare(a_str.data(), a_str.data() + a_str.size(),
                                                                      b_str.data(), b_str.data() + b_str.size()) < 0;
            }
            return m_data < other.m_data;
        }
        return res < 0;
    }

    //! Swaps contents with \p other.
    void swap(basic_element& other) noexcept {
        static_assert(detail::is_nothrow_swappable<decltype(m_name)>::value, "");
        static_assert(detail::is_nothrow_swappable<container_type>::value, "");
        static_assert(detail::is_nothrow_swappable<element_type>::value, "");
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
    template <typename T> static bool valid_set_type(element_type);

    template <typename> friend struct basic_element;
};

//! Non-member swap. Calls member swap.
template <typename Container>
void swap(basic_element<Container>& a, basic_element<Container>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

/*!
 * \param c Container to write to.
 * \param it Position in \p c to insert data.
 *
 * \throws invalid_element_type When type of this basic_element is invalid.
 * \throws invalid_element_size When the detected size of the data differs from the actual size.
 */
template <class Container>
template <typename OutContainer>
void basic_element<Container>::write_to_container(OutContainer& c, typename OutContainer::const_iterator it) const {
    static_assert(std::is_same<typename OutContainer::value_type, char>::value, "");
    if(!detail::valid_type(m_type))
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

//! Helper to deduce element_type::string_element.
inline element_type deduce_type(const boost::string_ref&) noexcept { return element_type::string_element; }
//! Helper to deduce element_type::string_element.
inline element_type deduce_type(const std::string&) noexcept { return element_type::string_element; }
//! Helper to deduce element_type::string_element.
inline element_type deduce_type(const char*) noexcept { return element_type::string_element; }

//! Helper to deduce element_type::boolean_element.
inline element_type deduce_type(bool) noexcept { return element_type::boolean_element; }
//! Helper to deduce element_type::double_element.
inline element_type deduce_type(float) noexcept { return element_type::double_element; }
//! Helper to deduce element_type::double_element.
inline element_type deduce_type(double) noexcept { return element_type::double_element; }
//! Helper to deduce element_type::int64_element.
inline element_type deduce_type(int64_t) noexcept { return element_type::int64_element; }
//! Helper to deduce element_type::int32_element.
inline element_type deduce_type(int32_t) noexcept { return element_type::int32_element; }

//! Helper to deduce element_type::document_element.
inline element_type deduce_type(const builder&) noexcept { return element_type::document_element; }
//! Helper to deduce element_type::array_element.
inline element_type deduce_type(const array_builder&) noexcept { return element_type::array_element; }

//! Helper to deduce element_type::document_element.
inline element_type deduce_type(builder&&) noexcept { return element_type::document_element; }
//! Helper to deduce element_type::array_element.
inline element_type deduce_type(array_builder&&) noexcept { return element_type::array_element; }

//! Helper to deduce element_type::document_element.
template <typename C, typename C2> inline element_type deduce_type(const basic_document<C, C2>&) noexcept {
    return element_type::document_element;
}
//! Helper to deduce element_type::array_element.
template <typename C, typename C2> inline element_type deduce_type(const basic_array<C, C2>&) noexcept {
    return element_type::array_element;
}

//! Helper to deduce element_type::document_element.
template <typename C, typename C2> inline element_type deduce_type(basic_document<C, C2>&&) noexcept {
    return element_type::document_element;
}
//! Helper to deduce element_type::array_element.
template <typename C, typename C2> inline element_type deduce_type(basic_array<C, C2>&&) noexcept {
    return element_type::array_element;
}

//! Helper to deduce an invalid element_type.
template <typename T> inline element_type deduce_type(T&&) noexcept { return static_cast<element_type>(0); }

} // namespace detail

/*!
 * Performs basic type deduction for JSON types only.
 * Any other type results in an invalid element_type being passed to typed overload.
 *
 * \param c Container to write to.
 * \param it Position in \p c to insert data.
 * \param name Name of element.
 * \param val Value of element. element_type deduction is attempted with this.
 *
 * \warning Basic exception guarantee.
 * \throws invalid_element_type When deduced type is invalid
 */
template <typename Container>
template <typename T>
void basic_element<Container>::write_to_container(container_type& c, typename container_type::const_iterator it,
                                                  boost::string_ref name, T&& val) {
    static_assert(!std::is_same<element_type, T>::value, "");
    static_assert(detail::is_valid_element_set_type<container_type, T>::value, "T must be compatible for deduction");
    write_to_container(c, it, name.to_string(), detail::deduce_type(std::forward<T>(val)), std::forward<T>(val));
}

/*!
 * \param c Container to write to.
 * \param it Position in \p c to insert data.
 * \param name Name of element.
 * \param type Type of element.
 * \param val Value of element.
 *
 * \warning Basic exception guarantee.
 * \throws invalid_element_type When type is invalid
 * \throws incompatible_type_conversion When type is void, i.e. should not contain data.
 */
template <typename Container>
template <typename T>
void basic_element<Container>::write_to_container(
    container_type& c, typename container_type::const_iterator it, boost::string_ref name, element_type type, T&& val,
    std::enable_if_t<detail::is_valid_element_set_type<container_type, T>::value>*) {
    if(!detail::valid_type(type))
        BOOST_THROW_EXCEPTION(invalid_element_type{});

    if(!valid_set_type<T>(type))
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{}
                              << detail::actual_type(typeid(T))
                              << detail::expected_type(detail::visit<detail::typeid_visitor>(type, Container{})));

    it = std::next(c.insert(it, static_cast<uint8_t>(type)));
    it = c.insert(it, name.begin(), name.end());
    std::advance(it, name.size());
    it = std::next(c.insert(it, '\0'));

    detail::visit<detail::set_visitor>(type, c, it, std::forward<T>(val));
}

template <typename Container>
template <typename T>
void basic_element<Container>::write_to_container(
    container_type& c, typename container_type::const_iterator it, boost::string_ref name, element_type type, T&& val,
    std::enable_if_t<!detail::is_valid_element_set_type<container_type, T>::value>*) {
    auto e = basic_element{name.to_string(), type, std::forward<T>(val)};
    e.write_to_container(c, it);
}

/*!
 * \param c Container to write to.
 * \param it Position in \p c to insert data.
 * \param name Name of element.
 * \param type Type of element.
 *
 * \warning Basic exception guarantee.
 * \throws invalid_element_type When type is invalid
 * \throws incompatible_type_conversion When type is non-void, i.e. should contain data.
 */
template <typename Container>
void basic_element<Container>::write_to_container(container_type& c, typename container_type::const_iterator it,
                                                  boost::string_ref name, element_type type) {
    if(!detail::valid_type(type))
        BOOST_THROW_EXCEPTION(invalid_element_type{});

    if(type != element_type::undefined_element && type != element_type::null_element && type != element_type::min_key &&
       type != element_type::max_key)
        BOOST_THROW_EXCEPTION(incompatible_type_conversion{}
                              << detail::actual_type(typeid(void))
                              << detail::expected_type(detail::visit<detail::typeid_visitor>(type, basic_element())));

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

/*!
 * Copies the contents of a basic_element with a compatible container_type.
 * \param elem Element to be copied.
 */
template <class Container>
template <typename OtherContainer>
basic_element<Container>::basic_element(const basic_element<OtherContainer>& elem,
                                        std::enable_if_t<std::is_constructible<container_type, OtherContainer>::value>*)
    : m_name(elem.m_name), m_type(elem.m_type), m_data(elem.m_data) {}

/*!
 * Copies the contents of a basic_element with an incompatible container_type.
 * \param elem Element to be copied.
 * \internal Does this via `container_type(begin(other), end(other))`.
 */
template <class Container>
template <typename OtherContainer>
basic_element<Container>::basic_element(
    const basic_element<OtherContainer>& elem,
    std::enable_if_t<!std::is_constructible<container_type, OtherContainer>::value>*,
    std::enable_if_t<std::is_constructible<container_type, typename OtherContainer::const_iterator,
                                           typename OtherContainer::const_iterator>::value>*)
    : m_name(elem.m_name), m_type(elem.m_type), m_data(elem.m_data.begin(), elem.m_data.end()) {}

/*!
 * Moves the contents of a basic_element with a compatible container_type.
 * \param elem Element to be moved.
 */
template <class Container>
template <typename OtherContainer>
basic_element<Container>::basic_element(
    basic_element<OtherContainer>&& elem,
    std::enable_if_t<std::is_constructible<container_type, OtherContainer&&>::value>*)
    : m_name(std::move(elem.m_name)), m_type(std::move(elem.m_type)), m_data(std::move(elem.m_data)) {}

/*!
 * \param range Range of bytes representing BSON data.
 * \throws invalid_element_size When range is too small.
 *                              When value data is indicated to be larger than range.
 * \throws invalid_element_type When the detected element_type is invalid.
 */
template <class Container>
template <typename ForwardRange>
basic_element<Container>::basic_element(
    ForwardRange&& range, std::enable_if_t<!std::is_constructible<std::string, ForwardRange>::value>*,
    std::enable_if_t<detail::is_range_of_same_value<ForwardRange, typename Container::value_type>::value>*) {
    if(boost::distance(range) < 2)
        BOOST_THROW_EXCEPTION(invalid_element_size{} << detail::expected_size(2)
                                                     << detail::actual_size(boost::distance(range)));

    auto first = std::begin(range), last = std::end(range);

    this->type(static_cast<element_type>(*first++));

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

/*!
 * \param name Name of the element.
 * \param type Type of element the supplied data represents.
 * \param first Iterator to start of data.
 * \param last Iterator past the end of data.
 * \warning This constructor bypasses any type checking of the data.
 * \throws invalid_element_type When the supplied element_type is invalid.
 */
template <class Container>
template <typename ForwardIterator>
basic_element<Container>::basic_element(std::string name, element_type type, ForwardIterator first,
                                        ForwardIterator last)
    : m_name(std::move(name)), m_data(first, last) {
    this->type(type);
}

/*!
 * \param name Element name.
 * \param type Element type. Defaults to element_type::null_element.
 * \throws invalid_element_type When the supplied element_type is invalid.
 */
template <class Container>
basic_element<Container>::basic_element(std::string name, element_type type)
    : m_name(std::move(name)) {
    this->type(type);
}

template <class Container> size_t basic_element<Container>::size() const noexcept {
    return sizeof(m_type) + boost::distance(m_data) + m_name.size() + sizeof('\0');
}

namespace detail {

/*!
 * \brief Helper class to implement basic_element::visit with detail::visit.
 *
 * Also specialised for void `element_type`s.
 * \sa basic_element::visit detail::visit
 */
template <element_type EType, typename Visitor, typename Element> struct element_visitor {
    //! Functor call operator.
    auto operator()(Visitor&& visitor, Element&& elem) const {
        return visitor(
            elem.name(), EType,
            elem.template value<detail::ElementTypeMap<EType, typename std::decay_t<Element>::container_type>>());
    }
};

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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

#endif // DOXYGEN_SHOULD_SKIP_THIS

} // namespace detail

namespace detail {

/*!
 * \brief Functor for basic_element comparison.
 *
 * Drop-in replacement for std::less, allowing heterogeneous comparison with a name-only comparsion in e.g. std::set.
 */
struct elem_compare {
    //! Enables heterogeneous comparison in some standard algorithms and containers.
    using is_transparent = std::true_type;
    //! Functor call operator. Normal comparison.
    template <typename EContainer, typename EContainer2>
    bool operator()(const basic_element<EContainer>& lhs, const basic_element<EContainer2>& rhs) const {
        return lhs < rhs;
    }
    //! Functor call operator. Heterogeneous comparison.
    template <typename EContainer> bool operator()(const basic_element<EContainer>& lhs, boost::string_ref rhs) const {
        return lhs.name() < rhs;
    }
    //! Functor call operator. Heterogeneous comparison.
    template <typename EContainer> bool operator()(boost::string_ref lhs, const basic_element<EContainer>& rhs) const {
        return lhs < rhs.name();
    }
};

//! Wrapper for type validity checking visitors.
template <typename T, typename Container> struct is_valid_func {
    //! Visitor for checking validity of a type for fetching.
    template <element_type EType, typename... Args>
    struct inner : std::integral_constant<
                       bool, mpl::or_<std::is_convertible<T, detail::ElementTypeMap<EType, Container>>,
                                      std::is_constructible<detail::ElementTypeMap<EType, Container>, T>>::value> {
        static_assert(sizeof...(Args) == 0, "");
    };
    //! Visitor for checking validity of a type for setting.
    template <element_type EType, typename... Args>
    struct set_inner
        : std::integral_constant<
              bool, mpl::or_<std::is_convertible<T, detail::ElementTypeMapSet<EType, Container>>,
                             std::is_constructible<detail::ElementTypeMapSet<EType, Container>, T>>::value> {
        static_assert(sizeof...(Args) == 0, "");
    };
};

using expected_element_type = boost::error_info<struct expected_element_type_, element_type>;
using actual_element_type = boost::error_info<struct actual_element_type_, element_type>;

} // namespace detail

template <class Container> template <typename T> bool basic_element<Container>::valid_type(element_type type) {
    return detail::visit<detail::is_valid_func<T, Container>::template inner>(type);
}

template <class Container> template <typename T> bool basic_element<Container>::valid_set_type(element_type type) {
    return detail::visit<detail::is_valid_func<T, Container>::template set_inner>(type);
}

/*!
 * \brief Access element value of specific element_type
 *
 * \param elem basic_element to fetch value from.
 * \tparam EType element_type which maps to the type to fetch.
 * \throws incompatible_element_conversion When EType does not match the element_type of \p elem.
 * \throws invalid_element_size When the size of the element's data is invalid for the return type.
 */
template <element_type EType, typename Container>
auto get(const basic_element<Container>& elem) -> detail::ElementTypeMap<EType, Container> {
    if(EType != elem.type())
        BOOST_THROW_EXCEPTION(incompatible_element_conversion{} << detail::expected_element_type(EType)
                                                                << detail::actual_element_type(elem.type()));

    return elem.template value<detail::ElementTypeMap<EType, Container>>();
}

/*!
 * \brief Get value of specific type from a basic_element.
 *
 * \tparam ReturnT Specified return type.
 * \throws invalid_element_size When the size of the element's data is invalid for the chosen type \p ReturnT.
 * \sa basic_element::value
 */
template <typename ReturnT, typename Container> ReturnT get(const basic_element<Container>& elem) {
    return elem.template value<ReturnT>();
}

//! \brief Stream operator for getting a string representation of an element_type.
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
            os << "unknown element_type";
            break;
    };

    return os;
}

/*!
 * \brief Apply the visitor pattern on the element, based on its element_type.
 *
 * Will pass the element's value, or nothing when the value type is void, to a supplied functor which returns `void`.
 * This value is retrieved as if via get(), so the functor must accept all variations of:
 * \code
 visitor(boost::string_ref, element_type) // for void types, e.g. element_type::null_element
 visitor(boost::string_ref, element_type, detail::ElementTypeMap<type>) // for all other element_type
 \endcode
 *
 * \param visitor Functor that should accept multiple signatures.
 * \throws invalid_element_type When \p type is invalid.
 * \sa detail::ElementTypeMap
 */
template <class Container>
template <typename Visitor>
void basic_element<Container>::visit(Visitor&& visitor,
                                     std::enable_if_t<std::is_void<decltype(std::declval<Visitor>()(
                                         "", std::declval<element_type>(), std::declval<double>()))>::value>*) const {
    detail::visit<detail::element_visitor>(m_type, std::forward<Visitor>(visitor), *this);
    return;
}

/*!
 * \brief Apply the visitor pattern on the element, based on its element_type.
 *
 * Will pass the element's value, or nothing when the value type is void, to a supplied functor which returns some
 * non-void type.
 * This value is retrieved as if via get(), so the functor must accept all variations of:
 * \code
 visitor(boost::string_ref, element_type) // for void types, e.g. element_type::null_element
 visitor(boost::string_ref, element_type, detail::ElementTypeMap<type>) // for all other element_type
 \endcode
 *
 * \param visitor Functor that should accept multiple signatures.
 * \throws invalid_element_type When \p type is invalid.
 * \sa detail::ElementTypeMap
 */
template <class Container>
template <typename Visitor>
auto basic_element<Container>::visit(Visitor&& visitor,
                                     std::enable_if_t<!std::is_void<decltype(std::declval<Visitor>()(
                                         "", std::declval<element_type>(), std::declval<double>()))>::value>*) const
    -> decltype(std::declval<Visitor>()("", std::declval<element_type>(), std::declval<double>())) {
    return detail::visit<detail::element_visitor>(m_type, std::forward<Visitor>(visitor), *this);
}

} // namespace jbson

JBSON_POP_WARNINGS

#endif // JBSON_ELEMENT_HPP
