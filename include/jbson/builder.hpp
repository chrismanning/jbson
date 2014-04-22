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

struct builder {
    builder() = default;

    template <typename Arg1, typename... ArgN>
    explicit builder(Arg1&& arg, ArgN&&... args) {
        emplace(std::forward<Arg1>(arg), std::forward<ArgN>(args)...);
    }

    // lvalue funcs

    template <typename... Args> builder& operator()(Args&&... args) & {
        emplace(std::forward<Args>(args)...);
        return *this;
    }

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

    template <typename... Args> builder&& operator()(Args&&... args) && {
        return std::move(emplace(std::forward<Args>(args)...));
    }

    template <typename... Args> builder&& emplace(Args&&... args) && {
        return std::move(emplace(std::forward<Args>(args)...));
    }

    template <typename Container, typename EContainer> operator basic_document<Container, EContainer>() const& {
        m_elements.push_back('\0');
        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_elements.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_elements.begin());
        auto doc = basic_document<Container, EContainer>(m_elements);
        m_elements.pop_back();
        return std::move(doc);
    }

    template <typename Container, typename EContainer> operator basic_document<Container, EContainer>() && {
        m_elements.push_back('\0');
        auto size = jbson::detail::native_to_little_endian(static_cast<int32_t>(m_elements.size()));
        static_assert(4 == size.size(), "");

        boost::range::copy(size, m_elements.begin());
        return basic_document<Container, EContainer>(std::move(m_elements));
    }

  private:
    mutable std::vector<char> m_elements{{0,0,0,0}};
};
static_assert(std::is_convertible<builder, document>::value, "");
static_assert(std::is_convertible<builder, basic_document<std::vector<char>, std::vector<char>>>::value, "");

struct array_builder {
    array_builder() = default;

    template <typename Arg1, typename... ArgN>
    explicit array_builder(Arg1&& arg, ArgN&&... args) {
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
    mutable std::vector<char> m_elements{{0,0,0,0}};
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
