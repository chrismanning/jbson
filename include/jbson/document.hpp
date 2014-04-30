//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_DOCUMENT_HPP
#define JBSON_DOCUMENT_HPP

#include <memory>
#include <vector>
#include <set>
#include <codecvt>

#include "detail/config.hpp"

JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/optional.hpp>
JBSON_CLANG_POP_WARNINGS

#include "document_fwd.hpp"
#include "element_fwd.hpp"
#include "detail/traits.hpp"
#include "element.hpp"

namespace jbson {

/*!
 * \brief Exception thrown when an document's data size differs from that reported.
 */
struct invalid_document_size : jbson_error {
    const char* what() const noexcept override { return "invalid_document_size"; }
};

namespace detail {

/*!
 * \brief Forward traversal iterator type for traversing basic_document and basic_array.
 *
 * The current basic_element is constructed and cached on increment() or document_iter construction.
 * This is required for iteration as the basic_element's size is not known in advance.
 */
template <typename Value, typename BaseIterator>
struct document_iter
    : boost::iterator_facade<document_iter<Value, BaseIterator>, Value, boost::forward_traversal_tag, Value&> {
    static_assert(detail::is_element<std::remove_const_t<Value>>::value, "");

    //! Alias to Value
    using element_type = Value;

    //! Default constructor. Results in an invalid iterator.
    document_iter() = default;

    /*!
     * \brief Construct document_iter from pair of `BaseIterator`s.
     * \param it1 iterator to first item of data.
     * \param it2 iterator past the end of data.
     */
    document_iter(BaseIterator it1, BaseIterator it2) : m_start(it1), m_end(it2) {
        if(m_start == m_end)
            return;
        m_cur = element_type{m_start, m_end};
    }

    /*!
     * \brief Copy constructor.
     *
     * Enables construction from document_iter with different, but compatible type signature.
     */
    template <class OtherValue, typename OtherIt>
    document_iter(const document_iter<OtherValue, OtherIt>& other,
                  std::enable_if_t<std::is_convertible<OtherValue, element_type>::value&&
                                       std::is_convertible<OtherIt, BaseIterator>::value>* = nullptr)
        : m_start(other.m_start), m_end(other.m_end) {
        if(m_start == m_end)
            return;
        m_cur = element_type{m_start, m_end};
    }

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  private:
    friend class boost::iterator_core_access;
    template <typename, typename> friend struct document_iter;
    template <typename, typename> friend class jbson::basic_document;
#endif

    template <typename OtherValue, typename OtherIterator>
    bool equal(const document_iter<OtherValue, OtherIterator>& other) const {
        return other.m_start == m_start;
    }

    void increment() {
        if(m_start == m_end) {
            m_cur = boost::none;
            return;
        }

        std::advance(m_start, m_cur->size());
        if(m_start == m_end)
            m_cur = boost::none;
        else
            m_cur = element_type{m_start, m_end};
    }

    /*!
     * \brief Dereferences currently held basic_element.
     *
     * \return Reference to cached basic_element.
     * \internal Asserts that currently cached element is valid in debug mode.
     */
    element_type& dereference() const {
        assert(m_cur);
        return const_cast<element_type&>(*m_cur);
    }

  private:
    BaseIterator m_start;
    BaseIterator m_end;
    boost::optional<std::remove_const_t<element_type>> m_cur;
};

#ifdef DOXYGEN_SHOULD_SKIP_THIS
/*!
 * \brief Initialises a container or range for a valid empty basic_document or basic_array
 */
template <typename Container> void init_empty(Container& c);
#else
template <typename Container>
void init_empty(Container& c, std::enable_if_t<container_has_push_back<Container>::value>* = nullptr) {
    static constexpr std::array<char, 5> arr{{5, 0, 0, 0, '\0'}};
    boost::range::push_back(c, arr);
}

template <typename Container>
void init_empty(Container& c,
                std::enable_if_t<std::is_constructible<Container, std::array<char, 5>>::value>* = nullptr) {
    static constexpr std::array<char, 5> arr{{5, 0, 0, 0, '\0'}};
    c = arr;
}

template <typename Container>
void init_empty(Container& c, std::enable_if_t<!container_has_push_back<Container>::value>* = nullptr,
                std::enable_if_t<!std::is_constructible<Container, std::array<char, 5>>::value>* = nullptr) {}

#endif // DOXYGEN_SHOULD_SKIP_THIS

} // namespace detail

struct builder;

//! Level of validity to test for with basic_document::valid.
enum class validity_level : uint8_t {
    //! Basic test for the size of the data.
    data_size = 0,
    //! Basic test for the equality of the data size with that reported by the data. Includes above.
    bson_size = 0b0001,
    //! Tests that each element can be constructed without error. Includes above.
    element_construct = 0b0010,
    //! Tests for valid unicode in elements with type element_type::string_element. Includes above.
    unicode_valid = 0b0100,
    //! Tests that array elements are integers named consecutively from 0 (zero). Includes above except unicode_valid.
    array_indices = 0b1000
};

//! Allow less than (<) comparison for validity_level.
inline bool operator<(validity_level a, validity_level b) {
    return (std::underlying_type_t<validity_level>)a < (std::underlying_type_t<validity_level>)b;
}

//! Allow more than (>) comparison for validity_level.
inline bool operator>(validity_level a, validity_level b) { return b < a; }

//! Allow bitwise-and (&) for validity_level.
inline validity_level operator&(validity_level a, validity_level b) {
    return static_cast<validity_level>((std::underlying_type_t<validity_level>)a &
                                       (std::underlying_type_t<validity_level>)b);
}

//! Allow bitwise-or (|) for validity_level.
inline validity_level operator|(validity_level a, validity_level b) {
    return static_cast<validity_level>((std::underlying_type_t<validity_level>)a |
                                       (std::underlying_type_t<validity_level>)b);
}

/*!
 * basic_document represents a range of BSON elements
 * \tparam Container Type of underlying storage container/range. Must be range of `char`.
 * \tparam ElementContainer Type of underlying storage of constituent elements.
 */
template <class Container, class ElementContainer> class basic_document {
  public:
    //! Type of underlying storage container/range.
    using container_type = std::decay_t<Container>;
    static_assert(detail::is_nothrow_swappable<container_type>::value, "container_type must have noexcept swap()");

    //! Type of constituent elements.
    using element_type = basic_element<ElementContainer>;
    //! Forward iterator for traversal of elements. \sa detail::document_iter
    using const_iterator = typename detail::document_iter<const element_type, typename container_type::const_iterator>;
    //! \copydoc const_iterator
    using iterator = const_iterator;
    //! \copybrief element_type Enables usage in STL/Boost algorithms.
    using value_type = element_type;

    /*!
     * \brief Default constructor.
     * \note When container_type is able to own data, this results in a valid, empty document.
     * \note Otherwise (e.g. is boost::iterator_range<>), results in an invalid document.
     */
    basic_document() { detail::init_empty(m_data); }

    /*!
     * \brief Constructs a document with an existing container of data.
     *
     * \note Only available when `SomeType` decays to `container_type`.
     * \tparam SomeType Type which decays to `container_type`.
     * \param c Accepts all forms of `container_type`.
     *
     * Forwards \p c to `container_type` initialiser.
     *
     * \throws invalid_document_size When the size of c is insufficient.
     *                               When the size of c differs from the size specified by c's data.
     */
    template <typename SomeType>
    explicit basic_document(SomeType&& c,
                            std::enable_if_t<std::is_same<container_type, std::decay_t<SomeType>>::value>* = nullptr)
        : m_data(std::forward<SomeType>(c)) {
        if(!valid(validity_level::data_size))
            BOOST_THROW_EXCEPTION(invalid_document_size{} << detail::actual_size(boost::distance(m_data))
                                                          << detail::expected_size(sizeof(int32_t)));
        if(!valid(validity_level::bson_size))
            BOOST_THROW_EXCEPTION(invalid_document_size{}
                                  << detail::expected_size(
                                         detail::little_endian_to_native<int32_t>(m_data.begin(), m_data.end()))
                                  << detail::actual_size(boost::distance(m_data)));
    }

    /*!
     * \brief Copy constructor from a basic_document with a different `container_type`.
     *
     * \tparam OtherContainer basic_document container from which `container_type` can be constructed.
     */
    template <typename OtherContainer>
    basic_document(const basic_document<OtherContainer>& other,
                   std::enable_if_t<std::is_constructible<container_type, OtherContainer>::value>* =
                       nullptr) noexcept(std::is_nothrow_constructible<container_type, OtherContainer>::value)
        : m_data(other.m_data) {}

    /*!
     * \brief Copy constructor from a basic_document with a different `container_type`.
     *
     * \tparam OtherContainer basic_document container from whose iterator_type `container_type` can be constructed.
     */
    template <typename OtherContainer>
    basic_document(const basic_document<OtherContainer>& other,
                   std::enable_if_t<!std::is_constructible<container_type, OtherContainer>::value &&
                                    detail::container_has_push_back<container_type>::value &&
                                    std::is_constructible<container_type, typename OtherContainer::const_iterator,
                                                          typename OtherContainer::const_iterator>::value>* = nullptr)
        : m_data(other.m_data.begin(), other.m_data.end()) {}

    //! \brief Disallows construction from arrays of invalid document size.
    template <size_t N> explicit basic_document(std::array<char, N>, std::enable_if_t<(N < 5)>* = nullptr) = delete;

    /*!
     * \brief Constructs a document from a range of `char`.
     *
     * \tparam ForwardRange Range of `char` (not `container_type`)
     */
    template <typename ForwardRange>
    explicit basic_document(
        ForwardRange&& rng, std::enable_if_t<detail::is_range_of_same_value<ForwardRange, char>::value>* = nullptr,
        std::enable_if_t<
            std::conditional_t<detail::is_iterator_range<container_type>::value,
                               detail::is_range_of_iterator<
                                   ForwardRange, boost::mpl::bind<detail::quote<std::is_constructible>,
                                                                  typename container_type::iterator, boost::mpl::_1>>,
                               std::true_type>::value>* = nullptr,
        std::enable_if_t<!std::is_same<container_type, std::decay_t<ForwardRange>>::value &&
                         detail::is_range_of_iterator<
                             ForwardRange, boost::mpl::bind<detail::quote<std::is_constructible>, container_type,
                                                            boost::mpl::_1, boost::mpl::_1>>::value>* = nullptr)
        : basic_document(container_type(std::begin(rng), std::end(rng))) {}

    /*!
     * \brief Constructs a document from a range of `basic_element`.
     *
     * Other `basic_document`s are excluded.
     * Only enabled when `container_type` is an actual container as it needs to be able to own data.
     *
     * \tparam ForwardRange Range whose `value_type` is `basic_element<...>`.
     */
    template <typename ForwardRange>
    explicit basic_document(
        ForwardRange&& rng,
        std::enable_if_t<
            boost::mpl::and_<detail::is_range_of_value<ForwardRange, boost::mpl::quote1<detail::is_element>>,
                             detail::container_has_push_back<container_type>,
                             boost::mpl::not_<detail::is_document<std::decay_t<ForwardRange>>>>::type::value>* =
            nullptr) {
        static_assert(!detail::is_document<std::decay_t<ForwardRange>>::value, "");
        std::array<char, 4> arr{{0, 0, 0, 0}};
        boost::range::push_back(m_data, arr);
        for(auto&& e : rng) {
            e.write_to_container(m_data, m_data.end());
        }
        m_data.push_back('\0');
        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_data.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_data.begin());
    }

    /*!
     * \brief Constructs a document from two iterators. Forwards to the range-based constructors.
     */
    template <typename ForwardIterator>
    basic_document(ForwardIterator first, ForwardIterator last,
                   std::enable_if_t<
                       std::is_constructible<basic_document, boost::iterator_range<ForwardIterator>>::value>* = nullptr)
        : basic_document(boost::make_iterator_range(first, last)) {}

    /*!
     * \brief Returns an iterator to the first element of the container.
     *
     * \return const_iterator representing the first element in the document,
     *         or end() of sequence of document is empty.
     * \throws invalid_document_size When container size is smaller than necessary for iteration.
     */
    const_iterator begin() const {
        if(boost::distance(m_data) <= static_cast<ptrdiff_t>(sizeof(int32_t)))
            BOOST_THROW_EXCEPTION(invalid_document_size{});
        return {std::next(m_data.begin(), sizeof(int32_t)), std::prev(m_data.end())};
    }

    /*!
     * \return const_iterator representing the end of the document.
     * \note Incrementing is a no-op.
     * \note Attempting to dereference results in undefined behaviour.
     */
    const_iterator end() const noexcept {
        auto last = std::prev(m_data.end());
        return {last, last};
    }

    /*!
     * \brief Find an element with the specified name.
     * \param elem_name Name of required element.
     * \return const_iterator to item with specified name, or end().
     */
    const_iterator find(boost::string_ref elem_name) const {
        return std::find_if(begin(), end(), [&elem_name](auto&& elem) { return elem.name() == elem_name; });
    }

    /*!
     * \brief Removes the element at it.
     *
     * General advice to follow: All iterators and referenced basic_elements are invalidated.
     * Actaul behaviour depends on container_type::erase(elem_data.begin(), elem_data.end()).
     *
     * \param it Iterator to the element to remove. Cannot be end().
     * \return const_iterator following the last removed element.
     * \internal Asserts in debug mode that it != end()
     */
    const_iterator erase(const const_iterator& it) {
        assert(it != end());
        auto pos = m_data.erase(it.m_start, std::next(it.m_start, it.m_cur->size()));

        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_data.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_data.begin());

        return {pos, std::prev(m_data.end())};
    }

    /*!
     * \brief Inserts an element directly before another.
     *
     * General advice to follow: All iterators and referenced basic_elements are invalidated.
     * Actaul behaviour depends on container_type::insert(pos, elem_data.begin(), elem_data.end()).
     *
     * \param it Iterator to the location before which the element will be inserted. Can be end().
     * \param el Element to be inserted.
     *
     * \warning Strong exception guarantee.
     *
     * \return const_iterator to the inserted element.
     */
    template <typename EContainer>
    const_iterator insert(const const_iterator& it, const basic_element<EContainer>& el) {
        container_type data;
        el.write_to_container(data, data.end());

        auto pos = m_data.insert(it.m_start, data.begin(), data.end());

        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_data.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_data.begin());

        return {pos, std::prev(m_data.end())};
    }

    /*!
     * \brief Inserts an element in-place directly before another.
     *
     * General advice to follow: All iterators and referenced basic_elements are invalidated.
     * Actaul behaviour depends on container_type::insert(pos, elem_data.begin(), elem_data.end()).
     *
     * \param it Iterator to the location before which the element will be inserted. Can be end().
     * \param args Arguments to construct an element in-place.
     *
     * \warning Strong exception guarantee.
     *
     * \return const_iterator to the inserted element.
     */
    template <typename... Args> const_iterator emplace(const const_iterator& it, Args&&... args) {
        container_type data;
        basic_element<container_type>::write_to_container(data, data.end(), std::forward<Args>(args)...);

        auto pos = m_data.insert(it.m_start, data.begin(), data.end());

        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_data.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_data.begin());

        return {pos, std::prev(m_data.end())};
    }

    /*!
     * \brief Returns data's size in bytes.
     *
     * \note To determine the number of elements contained, use either of:
     * \code
        boost::distance(doc);
        std::distance(doc.begin(), doc.end());
       \endcode
     */
    int32_t size() const noexcept { return boost::distance(m_data); }

    //! Returns reference to BSON data.
    //! \note const lvalue overload
    const container_type& data() const& noexcept { return m_data; }

    //! Returns copy of BSON data.
    //! \note const rvalue overload
    container_type data() const&& noexcept(std::is_nothrow_copy_constructible<container_type>::value) { return m_data; }

    //! Returns rvalue reference to BSON data.
    //! \note rvalue overload
    container_type&& data() && noexcept(std::is_nothrow_move_constructible<container_type>::value) {
        return std::move(m_data);
    }

    /*!
     * \brief Validates document/array according to a validity_level.
     *
     * \param lvl Level of validity to check for. Defaults to validity_level::bson_size.
     * \param recurse boolean controlling whether or not to recurse to child documents/arrays. Defaults to `true`.
     * \return boolean indicating validity
     *
     * \sa validity_level
     */
    bool valid(const validity_level lvl = validity_level::bson_size, const bool recurse = true) const {
        bool ret{true};

        if(ret && lvl >= validity_level::data_size)
            ret = boost::distance(m_data) > sizeof(int32_t);
        if(ret && lvl >= validity_level::bson_size) {
            ret = static_cast<ptrdiff_t>(boost::distance(m_data)) ==
                  detail::little_endian_to_native<int32_t>(m_data.begin(), m_data.end());
        }
        if(ret && lvl >= validity_level::element_construct) {
            try {
                for(auto&& e : *this) {
                    if(recurse && e.type() == jbson::element_type::document_element)
                        ret = get<jbson::element_type::document_element>(e).valid(lvl, recurse);
                    else if(recurse && e.type() == jbson::element_type::array_element)
                        ret = get<jbson::element_type::array_element>(e).valid(lvl, recurse);
                    else if(recurse && e.type() == jbson::element_type::scoped_javascript_element) {
                        decltype(get<jbson::element_type::document_element>(e)) scope;
                        std::tie(std::ignore, scope) = get<jbson::element_type::scoped_javascript_element>(e);
                        ret = scope.valid(lvl, recurse);
                    }
                    if(!ret)
                        break;

                    if((lvl & validity_level::unicode_valid) == validity_level::unicode_valid &&
                       e.type() == jbson::element_type::string_element) {
                        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> u8to32{};
                        auto str = get<jbson::element_type::string_element>(e);
                        u8to32.from_bytes(str.data(), str.data() + str.size());
                    }
                    if(!ret)
                        break;
                }
            }
            catch(...) {
                ret = false;
            }
        }
        return ret;
    }

    //! basic_document_set conversion. Allows explicit conversion to document_set and other instantiations.
    template <typename EContainer> explicit operator basic_document_set<EContainer>() const {
        auto set = basic_document_set<EContainer>{};
        for(auto&& e : *this) {
            set.emplace(e);
        }
        return std::move(set);
    }

    //! Determines equality with another basic_document.
    template <typename C, typename EC> bool operator==(const basic_document<C, EC>& other) const {
        return boost::equal(m_data, other.m_data);
    }

    //! Swaps contents with another basic_document.
    void swap(basic_document& other) noexcept {
        using std::swap;
        swap(m_data, other.m_data);
    }

  private:
    container_type m_data;
    template <typename, typename> friend class basic_document;
    template <typename, typename> friend class basic_array;
};

template <class Container, class ElementContainer> class basic_array : basic_document<Container, ElementContainer> {
    using base = basic_document<Container, ElementContainer>;
    template <typename, typename> friend class basic_array;

  public:
    using typename base::container_type;
    using typename base::element_type;
    using typename base::iterator;
    using typename base::const_iterator;
    using typename base::value_type;

    using base::m_data;
    using base::begin;
    using base::end;
    using base::data;
    using base::size;
    using base::swap;

    basic_array() : base() {}

    template <typename OtherContainer>
    basic_array(const basic_array<OtherContainer>& other,
                std::enable_if_t<std::is_constructible<container_type, OtherContainer>::value>* =
                    nullptr) noexcept(std::is_nothrow_constructible<container_type, OtherContainer>::value)
        : base(other.m_data) {}

    template <typename Arg>
    explicit basic_array(
        Arg&& arg,
        std::enable_if_t<std::is_constructible<base, Arg&&>::value && !std::is_convertible<Arg&&, base>::value>* =
            nullptr) noexcept(std::is_nothrow_constructible<base, Arg&&>::value)
        : base(std::forward<Arg>(arg)) {}

    template <typename Arg1, typename Arg2>
    basic_array(Arg1&& arg1, Arg2&& arg2,
                std::enable_if_t<std::is_constructible<base, Arg1&&, Arg2&&>::value>* =
                    nullptr) noexcept(std::is_nothrow_constructible<base, Arg1&&, Arg2&&>::value)
        : base(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2)) {}

    const_iterator find(int32_t idx) const { return base::find(std::to_string(idx)); }

    bool valid(const validity_level lvl = validity_level::bson_size, const bool recurse = true) const {
        bool ret = base::valid(lvl, recurse);
        if(ret && (lvl & validity_level::array_indices) == validity_level::array_indices) {
            try {
                int32_t count{0};
                for(auto&& e : *this) {
                    if(!(ret = (e.name() == std::to_string(count))))
                        break;

                    ++count;
                }
            }
            catch(...) {
                ret = false;
            }
        }
        return ret;
    }

    template <typename SequenceContainer,
              typename = std::enable_if_t<detail::container_has_push_back<SequenceContainer>::value>,
              typename = std::enable_if_t<
                  detail::is_range_of_value<SequenceContainer, boost::mpl::quote1<detail::is_element>>::value>>
    explicit operator SequenceContainer() const {
        auto fun = [](auto&& a, auto&& b) {
#ifdef _GNU_SOURCE
            // natural sort
            return strverscmp(a.name().data(), b.name().data());
#else
            return a.name().compare(b.name());
#endif
        };
        auto vec = SequenceContainer{};
        for(auto&& e : *this)
            vec.push_back(e);
        boost::range::sort(vec, [fun](auto&& e1, auto&& e2) { return fun(e1, e2) < 0; });
        assert(std::unique(vec.begin(), vec.end(), [fun](auto&& e1, auto&& e2) { return fun(e1, e2) == 0; }) ==
               vec.end());
        return std::move(vec);
    }
};

//! Non-member swap for basic_document. Calls basic_document::swap.
template <typename Container, typename EContainer>
void swap(basic_document<Container, EContainer>& a,
          basic_document<Container, EContainer>& b) noexcept(noexcept(a.swap(b))) {
    std::abort();
    a.swap(b);
}

//! Non-member swap for basic_document. Calls basic_document::swap.
template <typename Container, typename EContainer>
void swap(basic_array<Container, EContainer>& a, basic_array<Container, EContainer>& b) noexcept(noexcept(a.swap(b))) {
    std::abort();
    a.swap(b);
}

// document_set
template <typename Container, typename SetContainer>
void value_set(basic_element<Container>& c, const basic_document_set<SetContainer>& val) {
    c.value(document(val));
}

} // namespace jbson

#endif // JBSON_DOCUMENT_HPP
