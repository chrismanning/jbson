//          Copyright Christian Manning 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_CONFIG_HPP
#define JBSON_CONFIG_HPP

#include <boost/config.hpp>
#include <boost/predef.h>

#ifdef JBSON_DEPRECATED
    #undef JBSON_DEPRECATED
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
    #define JBSON_DEPRECATED
#else
    #if defined(BOOST_CLANG)
        #define JBSON_DEPRECATED __attribute__((deprecated))
    #elif defined(BOOST_MSVC)
        #define JBSON_DEPRECATED __declspec(deprecated)
    #else
        #define JBSON_DEPRECATED
    #endif
#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifdef JBSON_PUSH_DISABLE_DEPRECATED_WARNING
    #undef JBSON_PUSH_DISABLE_DEPRECATED_WARNING
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
    #define JBSON_PUSH_DISABLE_DEPRECATED_WARNING
#else
    #if defined(__GNUC__)
        #define JBSON_PUSH_DISABLE_DEPRECATED_WARNING _Pragma("GCC diagnostic push")\
                _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
    #elif defined(BOOST_INTEL)
        #define JBSON_PUSH_DISABLE_DEPRECATED_WARNING _Pragma("warning(push)")\
                _Pragma("warning(disable:1786)")
    #elif defined(BOOST_MSVC)
        #define JBSON_PUSH_DISABLE_DEPRECATED_WARNING _Pragma("warning(push)")\
                _Pragma("warning(disable:4996)")
    #else
        #define JBSON_PUSH_DISABLE_DEPRECATED_WARNING
    #endif
#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifdef JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
    #undef JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
    #define JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
#else
    #if defined(BOOST_CLANG)
        #define JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING _Pragma("GCC diagnostic push")\
                _Pragma("GCC diagnostic ignored \"-Wdocumentation\"")
    #else
        #define JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
    #endif
#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifdef JBSON_POP_WARNINGS
    #undef JBSON_POP_WARNINGS
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
    #define JBSON_POP_WARNINGS
#else
    #if defined(__GNUC__)
        #define JBSON_POP_WARNINGS _Pragma("GCC diagnostic pop")
    #elif defined(BOOST_INTEL) || defined(BOOST_MSVC)
        #define JBSON_POP_WARNINGS _Pragma("warning(pop)")
    #else
        #define JBSON_POP_WARNINGS
    #endif
#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifdef JBSON_CLANG_POP_WARNINGS
    #undef JBSON_CLANG_POP_WARNINGS
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
    #define JBSON_CLANG_POP_WARNINGS
#else
    #if defined(BOOST_CLANG)
        #define JBSON_CLANG_POP_WARNINGS JBSON_POP_WARNINGS
    #else
        #define JBSON_CLANG_POP_WARNINGS
    #endif
#endif // DOXYGEN_SHOULD_SKIP_THIS

#if BOOST_COMP_GNUC && BOOST_COMP_GNUC <= BOOST_VERSION_NUMBER(4,9,0)
#define JBSON_NO_CONST_RVALUE_THIS
#endif

#if BOOST_VERSION < BOOST_VERSION_NUMBER(1,56,0)
#define JBSON_NO_ITERATOR_RANGE_DROP_FUNS
#endif

#endif // JBSON_CONFIG_HPP
