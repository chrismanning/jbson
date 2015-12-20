//          Copyright Christian Manning 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_PATH_HPP
#define JBSON_PATH_HPP

#include <vector>

#include <jbson/detail/config.hpp>

JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/type_traits/has_operator.hpp>
JBSON_CLANG_POP_WARNINGS

#include <jbson/document.hpp>
#include <jbson/detail/expression_parser.hpp>
#include <jbson/detail/expression_compiler.hpp>
#include <jbson/detail/expression_evaluator.hpp>

namespace jbson {

template <typename Container, typename EContainer>
auto path_select(basic_document<Container, EContainer>&& doc, std::experimental::string_view path) {
    std::vector<basic_element<Container>> results;

    detail::path_expression::ast::path_expression ast;
    auto r = detail::path_expression::parse_path_expression(path, ast);
    assert(r);

    std::vector<char> code;
    r = detail::path_expression::compile_expr(ast, std::back_inserter(code));
    assert(r);
    (void)r;

    auto loc_doc = basic_document<Container, Container>{std::move(doc)};
    auto code_range = boost::make_iterator_range(code);
    auto v = detail::path_expression::eval_expr(loc_doc, {loc_doc}, code_range);

    if(detail::path_expression::is_which<bool>(v))
        results.emplace_back("", get<bool>(v));
    else if(detail::path_expression::is_which<int64_t>(v))
        results.emplace_back("", get<int64_t>(v));
    else if(detail::path_expression::is_which<std::string>(v))
        results.emplace_back("", get<std::string>(v));
    else if(detail::path_expression::is_which<basic_element<Container>>(v))
        results.emplace_back(get<basic_element<Container>>(v));
    else if(detail::path_expression::is_which<basic_document<Container, Container>>(v)) {
        if(auto doc = get<basic_document<Container, Container>>(&v))
            results.emplace_back("", *doc);
    } else if(detail::path_expression::is_which<basic_array<Container, Container>>(v)) {
        if(auto doc = get<basic_array<Container, Container>>(&v))
            results.emplace_back("", *doc);
    } else if(detail::path_expression::is_which<std::vector<basic_element<Container>>>(v))
        boost::push_back(results, get<std::vector<basic_element<Container>>>(v));

    return results;
}

template <typename Container, typename EContainer>
auto path_select(const basic_document<Container, EContainer>& doc, std::experimental::string_view path) {
    std::vector<basic_element<EContainer>> results;

    detail::path_expression::ast::path_expression ast;
    auto r = detail::path_expression::parse_path_expression(path, ast);
    assert(r);

    std::vector<char> code;
    r = detail::path_expression::compile_expr(ast, std::back_inserter(code));
    assert(r);
    (void)r;

    auto loc_doc = basic_document<EContainer, EContainer>{std::move(doc)};
    auto code_range = boost::make_iterator_range(code);
    auto v = detail::path_expression::eval_expr(loc_doc, {loc_doc}, code_range);

    if(detail::path_expression::is_which<basic_element<EContainer>>(v))
        results.emplace_back(get<basic_element<EContainer>>(v));
    else if(detail::path_expression::is_which<basic_document<EContainer, EContainer>>(v)) {
        if(auto doc = get<basic_document<EContainer, EContainer>>(&v))
            results.push_back(basic_element<EContainer>::raw("", element_type::document_element, doc->data()));
    } else if(detail::path_expression::is_which<basic_array<EContainer, EContainer>>(v)) {
        if(auto arr = get<basic_array<EContainer, EContainer>>(&v))
            results.push_back(basic_element<EContainer>::raw("", element_type::array_element, arr->data()));
    } else if(detail::path_expression::is_which<std::vector<basic_element<EContainer>>>(v))
        boost::push_back(results, get<std::vector<basic_element<EContainer>>>(v));

    return results;
}

// template <typename ElemRangeT>
// auto path_select(
//    ElemRangeT& doc, std::experimental::string_view path,
//    std::enable_if_t<detail::is_range_of_value<ElemRangeT, boost::mpl::quote1<detail::is_element>>::value>* = nullptr)
//    {
//    using Container = typename boost::range_value<ElemRangeT>::type::container_type;

//    return path_select(basic_document<Container>{doc}, path);
//}

template <typename DocRangeT, typename StrRngT>
auto path_select(DocRangeT&& docs, StrRngT&& path_rng,
                 std::enable_if_t<detail::is_range_of_value<
                     DocRangeT, boost::mpl::bind<boost::mpl::quote2<detail::is_range_of_value>, boost::mpl::arg<1>,
                                                 boost::mpl::quote1<detail::is_element>>>::value>* = nullptr) {
    auto path = boost::as_literal(path_rng);
    std::vector<typename std::decay_t<DocRangeT>::value_type> vec;

    for(auto&& doc : docs) {
        if(path.empty() || (path.size() == 1 && path.front() == '$')) {
            boost::range::push_back(vec, doc);
            continue;
        }

        path = boost::range::find_if<boost::return_found_end>(path, [](auto c) { return c != '$'; });
        boost::push_back(vec, path_select(doc, path));
    }

    return std::move(vec);
}

} // namespace jbson

#endif // JBSON_PATH_HPP
