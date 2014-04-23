//          Copyright Christian Manning 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_BUILDER_HPP
#define JBSON_BUILDER_HPP

#include <vector>

#include <jbson/element.hpp>
#include <jbson/document.hpp>

namespace jbson {

/*!
 * \brief builder provides a simple interface for document construction.
 *
 * builder uses basic_element::write_to_container() to construct elements directly to a container,
 * in an easier manner than by doing so manually.
 * builder is implicitly convertible to a basic_document.
 * After conversion, it is still a valid builder object and can be expanded and/or converted again.
 *
 * builder can also be used on rvalues, propagating itself as an rvalue reference (builder&&)
 * rather than an lvalue reference (builder&).
 * This means that rvalue builders can be implicitly move converted to a document
 * (basic_document<std::vector<char>>), without an explicit cast or std::move().
 * This is generally only beneficial in this case.
 *
 * Example of implicit move conversion:
 * To create the JSON object: `{ "abc": "some string", "def": 123, "xyz": {} }`
 * \code
    document doc = builder("abc", "some string")
                          ("def", 123);
                          ("xyz", builder());
    // equivalent to:
    auto build = builder("abc", "some string")
                        ("def", 123);
                        ("xyz", builder());
    document doc = std::move(build);
   \endcode
 * \sa array_builder
 */
struct builder {
    /*!
     * \brief Default constructor.
     *
     * A default constructed builder can be converted to a valid, empty basic_document.
     *
     * \throws something if vector constructor throws
     */
    builder() = default;

    /*!
     * \brief Constructor which forwards to emplace.
     *
     * Equivalent to `builder().emplace(arg, args...)`.
     *
     * \sa emplace()
     *
     * \throws something if vector constructor throws
     * \throws invalid_element_type When supplied/deduced element_type is invalid.
     * \throws incompatible_type_conversion When data is supplied to a void element_type.
     *                                      When data is not supplied to a non-void element_type.
     */
    template <typename Arg1, typename... ArgN> explicit builder(Arg1&& arg, ArgN&&... args) {
        emplace(std::forward<Arg1>(arg), std::forward<ArgN>(args)...);
    }

    // lvalue funcs

    /*!
     * \brief Forwards to emplace.
     * \sa emplace()
     *
     * \throws something if vector constructor throws
     * \throws invalid_element_type When supplied/deduced element_type is invalid.
     * \throws incompatible_type_conversion When data is supplied to a void element_type.
     *                                      When data is not supplied to a non-void element_type.
     */
    template <typename... Args> builder& operator()(Args&&... args) & {
        emplace(std::forward<Args>(args)...);
        return *this;
    }

    /*!
     * \brief Constructs a BSON element in place with the supplied args.
     *
     * Exception safety: Strong
     *
     * \sa basic_element::write_to_container()
     *
     * \throws something if vector constructor throws
     * \throws invalid_element_type When supplied/deduced element_type is invalid.
     * \throws incompatible_type_conversion When data is supplied to a void element_type.
     *                                      When data is not supplied to a non-void element_type.
     */
    template <typename... Args> builder& emplace(Args&&... args) & {
        auto old_size = m_elements.size();
        try {
            basic_element<decltype(m_elements)>::write_to_container(m_elements, m_elements.end(),
                                                                    std::forward<Args>(args)...);
        }
        catch(...) {
            m_elements.resize(old_size);
            throw;
        }

        return *this;
    }

    // rvalue funcs

    /*!
     * \brief Rvalue overload. Forwards `*this` as an rvalue reference.
     * Constructs a BSON element in place with the supplied args.
     *
     * Exception safety: Strong
     *
     * \sa basic_element::write_to_container()
     *
     * \throws something if vector constructor throws
     * \throws invalid_element_type When supplied/deduced element_type is invalid.
     * \throws incompatible_type_conversion When data is supplied to a void element_type.
     *                                      When data is not supplied to a non-void element_type.
     */
    template <typename... Args> builder&& operator()(Args&&... args) && {
        return std::move(emplace(std::forward<Args>(args)...));
    }

    /*!
     * \brief Rvalue overload. Forwards `*this` as an rvalue reference.
     * Constructs a BSON element in place with the supplied args.
     *
     * Exception safety: Strong
     *
     * \sa basic_element::write_to_container()
     *
     * \throws something if vector constructor throws
     * \throws invalid_element_type When supplied/deduced element_type is invalid.
     * \throws incompatible_type_conversion When data is supplied to a void element_type.
     *                                      When data is not supplied to a non-void element_type.
     */
    template <typename... Args> builder&& emplace(Args&&... args) && {
        return std::move(emplace(std::forward<Args>(args)...));
    }

    /*!
     * \brief Implicit conversion to basic_document.
     *
     * Constructs a basic_document with copy of internal storage.
     */
    template <typename Container, typename EContainer> operator basic_document<Container, EContainer>() const& {
        static_assert(!detail::is_iterator_range<Container>::value, "");
        m_elements.push_back('\0');
        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_elements.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_elements.begin());
        auto doc = basic_document<Container, EContainer>(m_elements);
        m_elements.pop_back();
        return std::move(doc);
    }

    /*!
     * \brief Implicit move-conversion to basic_document.
     *
     * Constructs a basic_document with the internal storage.
     */
    template <typename Container, typename EContainer> operator basic_document<Container, EContainer>() && {
        static_assert(!detail::is_iterator_range<Container>::value, "");
        m_elements.push_back('\0');
        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_elements.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_elements.begin());
        return basic_document<Container, EContainer>(std::move(m_elements));
    }

  private:
    mutable std::vector<char> m_elements{{0, 0, 0, 0}};
};
static_assert(std::is_convertible<builder, document>::value, "");
static_assert(std::is_convertible<builder, basic_document<std::vector<char>, std::vector<char>>>::value, "");

/*!
 * \brief array_builder provides a simple interface for array construction
 *
 * \sa builder
 */
struct array_builder {
    array_builder() = default;

    template <typename Arg1, typename... ArgN> explicit array_builder(Arg1&& arg, ArgN&&... args) {
        emplace(std::forward<Arg1>(arg), std::forward<ArgN>(args)...);
    }

    // lvalue funcs

    template <typename... Args> array_builder& operator()(Args&&... args) & {
        static_assert(sizeof...(Args) > 0, "");
        emplace(std::forward<Args>(args)...);
        return *this;
    }

    template <typename... Args> array_builder& emplace(Args&&... args) & {
        static_assert(sizeof...(Args) > 0, "");
        std::array<char, std::numeric_limits<decltype(m_count)>::digits10 + 1> int_str;
        auto n = std::snprintf(int_str.data(), int_str.size(), "%zd", m_count);
        if(n <= 0) {
            if(errno)
                BOOST_THROW_EXCEPTION(std::system_error(errno, std::generic_category()));
            return *this;
        }
        auto old_size = m_elements.size();
        try {
            basic_element<decltype(m_elements)>::write_to_container(
                m_elements, m_elements.end(), boost::string_ref{int_str.data(), static_cast<size_t>(n)},
                std::forward<Args>(args)...);
            m_count++;
        }
        catch(...) {
            m_elements.resize(old_size);
            throw;
        }

        return *this;
    }

    // rvalue funcs

    template <typename... Args> array_builder&& operator()(Args&&... args) && {
        return std::move(emplace(std::forward<Args>(args)...));
    }

    template <typename... Args> array_builder&& emplace(Args&&... args) && {
        return std::move(emplace(std::forward<Args>(args)...));
    }

    template <typename Container, typename EContainer> operator basic_array<Container, EContainer>() const& {
        m_elements.push_back('\0');
        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_elements.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_elements.begin());
        auto doc = basic_array<Container, EContainer>(m_elements);
        m_elements.pop_back();
        return std::move(doc);
    }

    template <typename Container, typename EContainer> operator basic_array<Container, EContainer>() && {
        m_elements.push_back('\0');
        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_elements.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_elements.begin());
        return basic_array<Container, EContainer>(std::move(m_elements));
    }

  private:
    mutable std::vector<char> m_elements{{0, 0, 0, 0}};
    uint32_t m_count{0u};
};
static_assert(std::is_convertible<array_builder, array>::value, "");
static_assert(std::is_convertible<array_builder, basic_array<std::vector<char>, std::vector<char>>>::value, "");

// builder
template <typename Container, typename IteratorT> void serialise(Container& c, IteratorT& it, builder val) {
    serialise(c, it, document(std::move(val)));
}

// array_builder
template <typename Container, typename IteratorT> void serialise(Container& c, IteratorT& it, array_builder val) {
    serialise(c, it, array(std::move(val)));
}

} // namespace jbson

#endif // JBSON_BUILDER_HPP
