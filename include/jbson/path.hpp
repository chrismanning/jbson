//          Copyright Christian Manning 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_PATH_HPP
#define JBSON_PATH_HPP

#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdocumentation"
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/type_traits/has_operator.hpp>
#pragma GCC diagnostic pop

#include "document.hpp"
#include "expression_parser.hpp"

namespace jbson {

namespace detail {

template <typename ElemRangeT, typename StrRngT, typename OutIterator>
void select(ElemRangeT&& doc, StrRngT path, OutIterator out);

template <typename ElemRangeT, typename StrRngT, typename OutIterator>
void select_name(ElemRangeT&& doc, StrRngT path, StrRngT name, OutIterator out) {
    if(name.empty())
        return;

    using namespace boost;

    if(range::equal(name, as_literal("*")) || range::equal(name, as_literal(".."))) {
        if(!path.empty()) { // descend
            for(auto&& e : doc) {
                if(e.type() == element_type::document_element)
                    select(get<element_type::document_element>(e), path, out);
                else if(e.type() == element_type::array_element)
                    select(get<element_type::array_element>(e), path, out);
            }
        } else
            out = range::copy(doc, out);
    }

    auto doc_it = range::find_if(doc, [&](auto&& e) { return range::equal(name, e.name()); });

    if(doc_it != doc.end()) {
        if(!path.empty()) { // descend
            if(doc_it->type() == element_type::document_element)
                select(get<element_type::document_element>(*doc_it), path, out);
            else if(doc_it->type() == element_type::array_element)
                select(get<element_type::array_element>(*doc_it), path, out);
        } else
            *out++ = *doc_it;
    } else
        return;
}

template <typename ElemRangeT, typename StrRngT, typename OutIterator>
void select_expr(ElemRangeT&& doc, StrRngT path, StrRngT expr, OutIterator out);

template <typename ElemRangeT, typename StrRngT, typename OutIterator>
void select_sub(ElemRangeT&& doc, StrRngT path, StrRngT subscript, OutIterator out) {
    using namespace boost;

    path.drop_front(subscript.size() + 2);
    if(!subscript.empty() && subscript.back() == ']')
        BOOST_THROW_EXCEPTION(jbson_error());

    std::vector<typename std::decay_t<ElemRangeT>::value_type> vec;
    while(!subscript.empty()) {
        StrRngT elem_name;
        if(subscript.front() == '"') {
            subscript.drop_front();
            elem_name = range::find<return_begin_found>(subscript, '"');
            subscript.drop_front(elem_name.size() + 1);
        } else if(subscript.front() == '\'') {
            subscript.drop_front();
            elem_name = range::find<return_begin_found>(subscript, '\'');
            subscript.drop_front(elem_name.size() + 1);
        } else if(::isdigit(subscript.front())) {
            elem_name = range::find<return_begin_found>(subscript, ',');
            subscript.drop_front(elem_name.size());
        } else if(subscript.front() == '*') {
            elem_name = {subscript.begin(), std::next(subscript.begin())};
            subscript.drop_front();
        } else if(!range::binary_search("(?", subscript.front()))
            BOOST_THROW_EXCEPTION(jbson_error());

        if(!subscript.empty() && range::binary_search("(?", subscript.front())) {
            elem_name = range::find_if<return_begin_next>(subscript, boost::is_any_of(",]"));
            subscript.drop_front(elem_name.size());
            select_expr(std::forward<ElemRangeT>(doc), path, elem_name, out);
        } else
            select_name(std::forward<ElemRangeT>(doc), path, elem_name, std::back_inserter(vec));

        if(!subscript.empty() && range::binary_search(",]", subscript.front()))
            subscript.drop_front();
    }
    out = range::unique_copy(vec, out);
}

namespace expression {
enum byte_code {
    op_neg, //  negate the top stack entry
    op_pos,
    op_add,   //  add top two stack entries
    op_sub,   //  subtract top two stack entries
    op_mul,   //  multiply top two stack entries
    op_div,   //  divide top two stack entries
    op_not,   //  boolean negate the top stack entry
    op_eq,    //  compare the top two stack entries for ==
    op_neq,   //  compare the top two stack entries for !=
    op_lt,    //  compare the top two stack entries for <
    op_lte,   //  compare the top two stack entries for <=
    op_gt,    //  compare the top two stack entries for >
    op_gte,   //  compare the top two stack entries for >=
    op_and,   //  logical and top two stack entries
    op_or,    //  logical or top two stack entries
    op_load,  //  load a variable
    op_store, //  store a variable
    op_int,   //  push constant integer into the stack
    op_string,
    op_true,  //  push constant 0 into the stack
    op_false, //  push constant 1 into the stack
};

template <typename OutIterator> bool compile_expr(ast::nil, OutIterator) {
    BOOST_ASSERT(0);
    return false;
}

template <typename OutIterator> bool compile_expr(int64_t x, OutIterator out) {
    *out++ = op_int;
    *out++ = x;
    return true;
}

template <typename OutIterator> bool compile_expr(bool x, OutIterator out) {
    *out++ = x ? op_true : op_false;
    return true;
}

template <typename OutIterator> bool compile_expr(ast::variable const& x, OutIterator out) {
    *out++ = op_load;
    out = boost::range::copy(x.name, out);
    *out++ = 0;
    return true;
}

template <typename OutIterator> bool compile_expr(std::string const& x, OutIterator out) {
    *out++ = op_string;
    out = boost::range::copy(x, out);
    *out++ = 0;
    return true;
}

struct Visitor {
    using result_type = bool;
    template <typename... Args> auto operator()(Args&&... args) const;
};

template <typename OutIterator> bool compile_expr(ast::operation const& x, OutIterator out) {
    using namespace std::placeholders;
    auto f = std::bind(Visitor(), _1, out);
    if(!x.operand_.apply_visitor(f))
        return false;
    switch(x.operator_) {
        case ast::optoken::plus:
            *out++ = (op_add);
            break;
        case ast::optoken::minus:
            *out++ = (op_sub);
            break;
        case ast::optoken::times:
            *out++ = (op_mul);
            break;
        case ast::optoken::divide:
            *out++ = (op_div);
            break;
        case ast::optoken::equal:
            *out++ = (op_eq);
            break;
        case ast::optoken::not_equal:
            *out++ = (op_neq);
            break;
        case ast::optoken::less:
            *out++ = (op_lt);
            break;
        case ast::optoken::less_equal:
            *out++ = (op_lte);
            break;
        case ast::optoken::greater:
            *out++ = (op_gt);
            break;
        case ast::optoken::greater_equal:
            *out++ = (op_gte);
            break;
        case ast::optoken::op_and:
            *out++ = (op_and);
            break;
        case ast::optoken::op_or:
            *out++ = (op_or);
            break;
        default:
            BOOST_ASSERT(0);
            return false;
    }
    return true;
}

template <typename OutIterator> bool compile_expr(ast::unary const& x, OutIterator out) {
    using namespace std::placeholders;
    auto f = std::bind(Visitor(), _1, out);
    if(!boost::apply_visitor(f, x.operand_))
        return false;
    switch(x.operator_) {
        case ast::optoken::negative:
            *out++ = (op_neg);
            break;
        case ast::optoken::op_not:
            *out++ = (op_not);
            break;
        case ast::optoken::positive:
            *out++ = (op_pos);
            break;
        default:
            BOOST_ASSERT(0);
            return false;
    }
    return true;
}

template <typename OutIterator> bool compile_expr(ast::expression const& x, OutIterator out) {
    using namespace std::placeholders;
    auto f = std::bind(Visitor(), _1, out);
    if(!boost::apply_visitor(f, x.first))
        return false;
    for(const auto& oper : x.rest) {
        if(!compile_expr(oper, out))
            return false;
    }
    return true;
}

template <typename... Args> auto Visitor::operator()(Args&&... args) const {
    return compile_expr(std::forward<Args>(args)...);
}

template <typename Derived> struct CompareVariable_impl : boost::static_visitor<bool> {
    template <typename C, typename U>
    bool operator()(const basic_element<C>& e, const U& v, std::enable_if_t<!is_element<U>::value>* = nullptr) const {
        switch(e.type()) {
            case element_type::boolean_element:
                return (*static_cast<const Derived*>(this))(e.template value<bool>(), v);
            case element_type::int32_element:
                return (*static_cast<const Derived*>(this))(e.template value<int32_t>(), v);
            case element_type::int64_element:
                return (*static_cast<const Derived*>(this))(e.template value<int64_t>(), v);
            case element_type::string_element:
                return (*static_cast<const Derived*>(this))(get<element_type::string_element>(e), v);
            default:
                break;
        }

        return false;
    }
    template <typename U, typename C>
    bool operator()(const U& v, const basic_element<C>& e, std::enable_if_t<!is_element<U>::value>* = nullptr) const {
        return (*static_cast<const Derived*>(this))(e, v);
    }
};

template <byte_code op> struct CompareVariable;

template <> struct CompareVariable<op_eq> : CompareVariable_impl<CompareVariable<op_eq>> {
    using CompareVariable_impl<CompareVariable<op_eq>>::operator();
    template <typename T, typename U>
    bool operator()(const T&, const U&, std::enable_if_t<!boost::has_equal_to<T, U>::value>* = nullptr) const {
        return false;
    }
    template <typename T, typename U>
    bool operator()(const T& lhs, const U& rhs, std::enable_if_t<boost::has_equal_to<T, U>::value>* = nullptr) const {
        return lhs == rhs;
    }
};

template <> struct CompareVariable<op_neq> : CompareVariable_impl<CompareVariable<op_neq>> {
    using CompareVariable_impl<CompareVariable<op_neq>>::operator();
    template <typename T, typename U>
    bool operator()(const T&, const U&, std::enable_if_t<!boost::has_equal_to<T, U>::value>* = nullptr) const {
        return false;
    }
    template <typename T, typename U>
    bool operator()(const T& lhs, const U& rhs, std::enable_if_t<boost::has_equal_to<T, U>::value>* = nullptr) const {
        return !(lhs == rhs);
    }
};

template <> struct CompareVariable<op_lt> : CompareVariable_impl<CompareVariable<op_lt>> {
    using CompareVariable_impl<CompareVariable<op_lt>>::operator();
    template <typename T, typename U>
    bool operator()(const T&, const U&, std::enable_if_t<!boost::has_less<T, U>::value>* = nullptr) const {
        return false;
    }
    template <typename T, typename U>
    bool operator()(const T& lhs, const U& rhs, std::enable_if_t<boost::has_less<T, U>::value>* = nullptr) const {
        return lhs < rhs;
    }
};

template <> struct CompareVariable<op_lte> : CompareVariable_impl<CompareVariable<op_lte>> {
    using CompareVariable_impl<CompareVariable<op_lte>>::operator();
    template <typename T, typename U>
    bool operator()(const T&, const U&, std::enable_if_t<!boost::has_less_equal<T, U>::value>* = nullptr) const {
        return false;
    }
    template <typename T, typename U>
    bool operator()(const T& lhs, const U& rhs, std::enable_if_t<boost::has_less_equal<T, U>::value>* = nullptr) const {
        return lhs <= rhs;
    }
};

template <> struct CompareVariable<op_gt> : CompareVariable_impl<CompareVariable<op_gt>> {
    using CompareVariable_impl<CompareVariable<op_gt>>::operator();
    template <typename T, typename U>
    bool operator()(const T&, const U&, std::enable_if_t<!boost::has_greater<T, U>::value>* = nullptr) const {
        return false;
    }
    template <typename T, typename U>
    bool operator()(const T& lhs, const U& rhs, std::enable_if_t<boost::has_greater<T, U>::value>* = nullptr) const {
        return lhs > rhs;
    }
};

template <> struct CompareVariable<op_gte> : CompareVariable_impl<CompareVariable<op_gte>> {
    using CompareVariable_impl<CompareVariable<op_gte>>::operator();
    template <typename T, typename U>
    bool operator()(const T&, const U&, std::enable_if_t<!boost::has_greater_equal<T, U>::value>* = nullptr) const {
        return false;
    }
    template <typename T, typename U>
    bool operator()(const T& lhs, const U& rhs,
                    std::enable_if_t<boost::has_greater_equal<T, U>::value>* = nullptr) const {
        return lhs >= rhs;
    }
};

template <typename ElemRangeT> auto eval_expr(ElemRangeT&& doc, const std::vector<int>& code) {
    using variable_type = boost::variant<bool, int64_t, std::string, typename std::decay_t<ElemRangeT>::value_type>;
    std::array<variable_type, 32> stack;
    auto stack_ptr = stack.begin();
    auto pc = code.begin();

    while(pc != code.end()) {
        BOOST_ASSERT(pc != code.end());
        switch(*pc++) {
            using namespace expression;
            case op_neg:
                assert(stack_ptr != stack.begin());
                assert(stack_ptr[-1].which() == 1);
                stack_ptr[-1] = -boost::get<int64_t>(stack_ptr[-1]);
                break;
            case op_pos:
                assert(stack_ptr != stack.begin());
                assert(stack_ptr[-1].which() == 1);
                stack_ptr[-1] = +boost::get<int64_t>(stack_ptr[-1]);
                break;
            case op_not:
                assert(stack_ptr != stack.begin());
                assert(stack_ptr[-1].which() == 0);
                stack_ptr[-1] = !bool(boost::get<bool>(stack_ptr[-1]));
                break;
            case op_add:
                --stack_ptr;
                assert(stack_ptr != stack.begin());
                assert(stack_ptr != stack.end());
                assert(stack_ptr[-1].which() == stack_ptr[0].which());
                assert(stack_ptr[-1].which() == 1);
                stack_ptr[-1] = boost::get<int64_t>(stack_ptr[-1]) + boost::get<int64_t>(stack_ptr[0]);
                break;
            case op_sub:
                --stack_ptr;
                assert(stack_ptr != stack.begin());
                assert(stack_ptr != stack.end());
                assert(stack_ptr[-1].which() == stack_ptr[0].which());
                assert(stack_ptr[-1].which() == 1);
                stack_ptr[-1] = boost::get<int64_t>(stack_ptr[-1]) - boost::get<int64_t>(stack_ptr[0]);
                break;
            case op_mul:
                --stack_ptr;
                assert(stack_ptr != stack.begin());
                assert(stack_ptr != stack.end());
                assert(stack_ptr[-1].which() == stack_ptr[0].which());
                assert(stack_ptr[-1].which() == 1);
                stack_ptr[-1] = boost::get<int64_t>(stack_ptr[-1]) * boost::get<int64_t>(stack_ptr[0]);
                break;
            case op_div:
                --stack_ptr;
                assert(stack_ptr != stack.begin());
                assert(stack_ptr != stack.end());
                assert(stack_ptr[-1].which() == stack_ptr[0].which());
                assert(stack_ptr[-1].which() == 1);
                stack_ptr[-1] = boost::get<int64_t>(stack_ptr[-1]) / boost::get<int64_t>(stack_ptr[0]);
                break;
            case op_eq:
                --stack_ptr;
                assert(stack_ptr != stack.begin());
                assert(stack_ptr != stack.end());
                stack_ptr[-1] = boost::apply_visitor(CompareVariable<op_eq>{}, stack_ptr[-1], stack_ptr[0]);
                break;
            case op_neq:
                --stack_ptr;
                assert(stack_ptr != stack.begin());
                assert(stack_ptr != stack.end());
                stack_ptr[-1] = boost::apply_visitor(CompareVariable<op_neq>{}, stack_ptr[-1], stack_ptr[0]);
                break;
            case op_lt:
                --stack_ptr;
                assert(stack_ptr != stack.begin());
                assert(stack_ptr != stack.end());
                stack_ptr[-1] = boost::apply_visitor(CompareVariable<op_lt>{}, stack_ptr[-1], stack_ptr[0]);
                break;
            case op_lte:
                --stack_ptr;
                assert(stack_ptr != stack.begin());
                assert(stack_ptr != stack.end());
                stack_ptr[-1] = boost::apply_visitor(CompareVariable<op_lte>{}, stack_ptr[-1], stack_ptr[0]);
                break;
            case op_gt:
                --stack_ptr;
                assert(stack_ptr != stack.begin());
                assert(stack_ptr != stack.end());
                stack_ptr[-1] = boost::apply_visitor(CompareVariable<op_gt>{}, stack_ptr[-1], stack_ptr[0]);
                break;
            case op_gte:
                --stack_ptr;
                assert(stack_ptr != stack.begin());
                assert(stack_ptr != stack.end());
                stack_ptr[-1] = boost::apply_visitor(CompareVariable<op_gte>{}, stack_ptr[-1], stack_ptr[0]);
                break;
            case op_and:
                --stack_ptr;
                assert(stack_ptr != stack.begin());
                assert(stack_ptr != stack.end());
                assert(stack_ptr[-1].which() == stack_ptr[0].which());
                assert(stack_ptr[-1].which() == 0);
                stack_ptr[-1] = boost::get<bool>(stack_ptr[-1]) && boost::get<bool>(stack_ptr[0]);
                break;
            case op_or:
                --stack_ptr;
                assert(stack_ptr != stack.begin());
                assert(stack_ptr != stack.end());
                assert(stack_ptr[-1].which() == stack_ptr[0].which());
                assert(stack_ptr[-1].which() == 0);
                stack_ptr[-1] = boost::get<bool>(stack_ptr[-1]) || boost::get<bool>(stack_ptr[0]);
                break;
            case op_load: {
                std::string str;
                for(; pc != code.end() && *pc != 0; ++pc)
                    str += char(*pc);
                if(pc != code.end())
                    ++pc;
                std::vector<typename std::decay_t<ElemRangeT>::value_type> vec;
                select(doc, boost::make_iterator_range(str), std::back_inserter(vec));
                if(!vec.empty())
                    stack_ptr = boost::range::copy(vec, stack_ptr);
                else
                    return variable_type{false};
            } break;
            case op_store:
                --stack_ptr;
                stack[*pc++] = stack_ptr[0];
                break;
            case op_int:
                *stack_ptr++ = int64_t(*pc++);
                break;
            case op_string: {
                std::string str;
                for(; pc != code.end() && *pc != 0; ++pc)
                    str += char(*pc);
                if(pc != code.end())
                    ++pc;
                *stack_ptr++ = str;
                break;
            }
            case op_true:
                *stack_ptr++ = true;
                break;
            case op_false:
                *stack_ptr++ = false;
                break;
        }
    }
    return stack_ptr[-1];
}

} // namespace expresion

template <typename ElemRangeT, typename StrRngT, typename OutIterator>
void select_expr(ElemRangeT&& doc, StrRngT path, StrRngT expr, OutIterator out) {
    using namespace boost;

    if(expr.empty())
        return;

    if(expr.back() != ')')
        return;
    expr.drop_back();

    std::vector<int> code;
    bool filter = false;
    if(starts_with(expr, as_literal("?("))) {
        expr.drop_front(2);
        filter = true;
    } else
        expr.drop_front();

    expression::error_handler<typename std::decay_t<StrRngT>::const_iterator> err_h{expr.begin(), expr.end()};
    expression::parser<typename std::decay_t<StrRngT>::const_iterator> expr_parser{err_h};
    expression::ast::expression ast;

    boost::spirit::ascii::space_type space;
    auto r = boost::spirit::qi::phrase_parse(expr.begin(), expr.end(), expr_parser, space, ast);
    assert(r);
    r = expression::compile_expr(ast, std::back_inserter(code));
    assert(r);

    using variable_type = decltype(expression::eval_expr(std::forward<ElemRangeT>(doc), code));
    variable_type v;
    std::vector<typename std::decay_t<ElemRangeT>::value_type> vec;
    if(!filter) {
        v = expression::eval_expr(std::forward<ElemRangeT>(doc), code);
    } else {
        for(auto&& e : doc) {
            v = false;
            if(e.type() == element_type::document_element)
                v = expression::eval_expr(get<element_type::document_element>(e), code);
            else if(e.type() == element_type::array_element)
                v = expression::eval_expr(get<element_type::array_element>(e), code);
            switch(v.which()) {
                case 0: // bool
                    if(get<bool>(v))
                        vec.push_back(e);
                    break;
                case 1: // int64
                    if(e.name() == std::to_string(get<int64_t>(v)))
                        vec.push_back(e);
                    break;
                case 2: // string
                    if(e.name() == get<std::string>(v))
                        vec.push_back(e);
                    break;
                case 3: // element
                    vec.push_back(e);
                    break;
            }
        }
    }
    if(path.empty()) {
        out = boost::copy(vec, out);
        return;
    }

    for(auto&& e : vec) {
        if(e.type() == element_type::document_element)
            select(get<element_type::document_element>(e), path, out);
        else if(e.type() == element_type::array_element)
            select(get<element_type::array_element>(e), path, out);
    }
}

template <typename ElemRangeT, typename StrRngT, typename OutIterator>
void select(ElemRangeT&& doc, StrRngT path, OutIterator out) {
    static_assert(detail::is_range_of_value<std::decay_t<ElemRangeT>, boost::mpl::quote1<detail::is_element>>::value,
                  "");
    static_assert(is_iterator_range<std::decay_t<StrRngT>>::value, "");
    using namespace boost;

    if(path.empty()) {
        out = range::copy(doc, out);
        return;
    }

    if(path.front() == '@')
        path.drop_front();

    StrRngT elem_name;
    if(!starts_with(path, as_literal("..")))
        path = range::find_if<boost::return_found_end>(path, [](auto c) { return c != '.'; });

    if(path.front() == '[') {
        elem_name = range::find<return_begin_found>(path, ']');
        elem_name.drop_front();
        select_sub(std::forward<ElemRangeT>(doc), path, elem_name, out);
    } else {
        if(starts_with(path, as_literal(".."))) {
            elem_name = {path.begin(), std::next(path.begin(), 2)};
            select_name(std::forward<ElemRangeT>(doc), path, elem_name, out);
            path.drop_front(2);
        }
        elem_name = range::find_if<return_begin_found>(path, is_any_of(".["));
        path.drop_front(elem_name.size());
        if(!path.empty() && path.front() == '.' && !starts_with(path, as_literal("..")))
            path.drop_front();
        select_name(std::forward<ElemRangeT>(doc), path, elem_name, out);
    }
}

} // namespace detail

template <typename Container, typename EContainer, typename StrRngT>
auto path_select(basic_document<Container, EContainer>&& doc, StrRngT&& path_rng) {
    auto path = boost::as_literal(path_rng);
    std::vector<basic_element<Container>> vec;

    if(path.empty() || (path.size() == 1 && path.front() == '$')) {
        boost::range::push_back(vec, doc);

        return std::move(vec);
    }

    path = boost::range::find_if<boost::return_found_end>(path, [](auto c) { return c != '$'; });

    detail::select(doc, path, std::back_inserter(vec));

    return std::move(vec);
}

template <typename ElemRangeT, typename StrRngT>
auto path_select(
    ElemRangeT&& doc, StrRngT&& path_rng,
    std::enable_if_t<detail::is_range_of_value<ElemRangeT, boost::mpl::quote1<detail::is_element>>::value>* = nullptr) {
    auto path = boost::as_literal(path_rng);
    std::vector<typename std::decay_t<ElemRangeT>::value_type> vec;

    if(path.empty() || (path.size() == 1 && path.front() == '$')) {
        boost::range::push_back(vec, doc);

        return std::move(vec);
    }

    path = boost::range::find_if<boost::return_found_end>(path, [](auto c) { return c != '$'; });

    detail::select(doc, path, std::back_inserter(vec));

    return std::move(vec);
}

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
            return std::move(vec);
        }

        path = boost::range::find_if<boost::return_found_end>(path, [](auto c) { return c != '$'; });
        detail::select(doc, path, std::back_inserter(vec));
    }

    return std::move(vec);
}

} // namespace jbson

#endif // JBSON_PATH_HPP
