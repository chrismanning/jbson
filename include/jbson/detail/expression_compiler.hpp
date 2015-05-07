//          Copyright Christian Manning 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_EXPRESSION_COMPILER_HPP
#define JBSON_EXPRESSION_COMPILER_HPP

#include <jbson/detail/config.hpp>

JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
#include <boost/variant.hpp>
#include <boost/range.hpp>
#include <boost/type_traits/has_operator.hpp>
#include <type_traits>
#include <experimental/string_view>
JBSON_CLANG_POP_WARNINGS

#include <jbson/detail/expression_parser.hpp>
#include <jbson/element.hpp>

namespace std {
using namespace experimental;
}

namespace jbson {
namespace detail {
namespace path_expression {

enum class byte_code {
    op_neg, //  negate the top stack entry
    op_pos,
    op_add,  //  add top two stack entries
    op_sub,  //  subtract top two stack entries
    op_mul,  //  multiply top two stack entries
    op_div,  //  divide top two stack entries
    op_not,  //  boolean negate the top stack entry
    op_eq,   //  compare the top two stack entries for ==
    op_neq,  //  compare the top two stack entries for !=
    op_lt,   //  compare the top two stack entries for <
    op_lte,  //  compare the top two stack entries for <=
    op_gt,   //  compare the top two stack entries for >
    op_gte,  //  compare the top two stack entries for >=
    op_and,  //  logical and top two stack entries
    op_or,   //  logical or top two stack entries
    op_load, //  load a variable
    op_int,  //  push constant integer into the stack
    op_string,
    op_object,
    op_array,
    op_true,  //  push constant 0 into the stack
    op_false, //  push constant 1 into the stack
    op_access,
    op_union,
    op_expr,
    op_filter_expr,
    op_return,
    op_recurse,
    op_slice,
};

std::ostream& operator<<(std::ostream& os, byte_code val) {
    switch(val) {
        case byte_code::op_neg:
            os << "op_neg";
            break;
        case byte_code::op_pos:
            os << "op_pos";
            break;
        case byte_code::op_not:
            os << "op_not";
            break;
        case byte_code::op_add:
            os << "op_add";
            break;
        case byte_code::op_sub:
            os << "op_sub";
            break;
        case byte_code::op_mul:
            os << "op_mul";
            break;
        case byte_code::op_div:
            os << "op_div";
            break;
        case byte_code::op_eq:
            os << "op_eq";
            break;
        case byte_code::op_neq:
            os << "op_neq";
            break;
        case byte_code::op_lt:
            os << "op_lt";
            break;
        case byte_code::op_lte:
            os << "op_lte";
            break;
        case byte_code::op_gt:
            os << "op_gt";
            break;
        case byte_code::op_gte:
            os << "op_gte";
            break;
        case byte_code::op_and:
            os << "op_and";
            break;
        case byte_code::op_or:
            os << "op_or";
            break;
        case byte_code::op_load:
            os << "op_load";
            break;
        case byte_code::op_int:
            os << "op_int";
            break;
        case byte_code::op_string:
            os << "op_string";
            break;
        case byte_code::op_true:
            os << "op_true";
            break;
        case byte_code::op_false:
            os << "op_false";
            break;
        case byte_code::op_object:
            os << "op_object";
            break;
        case byte_code::op_array:
            os << "op_array";
            break;
        case byte_code::op_recurse:
            os << "op_recurse";
            break;
        case byte_code::op_access:
            os << "op_access";
            break;
        case byte_code::op_union:
            os << "op_union";
            break;
        case byte_code::op_expr:
            os << "op_expr";
            break;
        case byte_code::op_filter_expr:
            os << "op_filter_expr";
            break;
        case byte_code::op_return:
            os << "op_return";
            break;
        case byte_code::op_slice:
            os << "op_slice";
            break;
        default:
            os << static_cast<int>(val);
            break;
    }

    return os;
}

template <typename OutIterator> bool compile_expr(ast::nil, OutIterator);
template <typename OutIterator> bool compile_expr(bool x, OutIterator out);
template <typename OutIterator> bool compile_expr(int64_t x, OutIterator out);
template <typename OutIterator> bool compile_expr(const ast::path_descend& x, OutIterator out);
template <typename OutIterator> bool compile_expr(const ast::path_union& x, OutIterator out);
template <typename OutIterator> bool compile_expr(const ast::slice& x, OutIterator out);
template <typename OutIterator> bool compile_expr(const ast::field_access& x, OutIterator out);
template <typename OutIterator> bool compile_expr(const ast::path_expression& x, OutIterator out);
template <typename OutIterator> bool compile_expr(const std::string& x, OutIterator out);
template <typename OutIterator> bool compile_expr(const jbson::document& x, OutIterator out);
template <typename OutIterator> bool compile_expr(const jbson::array& x, OutIterator out);
template <typename OutIterator> bool compile_expr(const ast::operation& x, OutIterator out);
template <typename OutIterator> bool compile_expr(const ast::unary& x, OutIterator out);
template <typename OutIterator> bool compile_expr(const ast::expression& x, OutIterator out);
template <typename OutIterator> bool compile_expr(const ast::filter_expression& x, OutIterator out);
template <typename OutIterator> bool compile_expr(const ast::eval_expression& x, OutIterator out);

struct Visitor {
    using result_type = bool;
    template <typename... Args> auto operator()(Args&&... args) const {
        return compile_expr(std::forward<Args>(args)...);
    }
};

template <typename OutIterator> bool compile_expr(ast::nil, OutIterator) {
    throw 0;
    return false;
}

template <typename OutIterator> bool compile_expr(bool x, OutIterator out) {
    *out++ = static_cast<char>(x ? byte_code::op_true : byte_code::op_false);
    return true;
}

template <typename OutIterator> bool compile_expr(int64_t x, OutIterator out) {
    *out++ = static_cast<char>(byte_code::op_int);
    for(auto&& i : native_to_little_endian(x))
        *out++ = i;
    return true;
}

template <typename OutIterator> bool compile_expr(const ast::path_descend& x, OutIterator out) {
    using namespace std::placeholders;
    switch(x.operator_) {
        case ast::optoken::recurse:
            *out++ = static_cast<char>(byte_code::op_recurse);
            throw 0;
            break;
        case ast::optoken::descend:
            *out++ = static_cast<char>(byte_code::op_access);
            break;
        default:
            throw 0;
            return false;
    }
    if(!compile_expr(x.operand_, out))
        return false;
    *out++ = static_cast<char>(byte_code::op_return);
    return true;
}

template <typename OutIterator> bool compile_expr(const ast::path_union& x, OutIterator out) {
    using namespace std::placeholders;
    auto f = std::bind(Visitor(), _1, out);
    //    if(!x.rest.empty())
    //        *out++ = static_cast<char>(byte_code::op_union);
    if(!boost::apply_visitor(f, x.first))
        return false;
    for(auto&& operand : x.rest) {
        *out++ = static_cast<char>(byte_code::op_union);
        if(!boost::apply_visitor(f, operand))
            return false;
    }
    if(!x.rest.empty())
        *out++ = static_cast<char>(byte_code::op_union);
    return true;
}

template <typename OutIterator> bool compile_expr(const ast::slice& x, OutIterator out) {
    *out++ = static_cast<char>(byte_code::op_slice);
    *out++ = x.start.value_or(0);
    *out++ = x.end.value_or(-1);
    return true;
}

template <typename OutIterator> bool compile_expr(const ast::field_access& x, OutIterator out) {
    *out++ = static_cast<char>(byte_code::op_load);
    out = boost::range::copy(x.name, out);
    *out++ = 0;
    return true;
}

template <typename OutIterator> bool compile_expr(const ast::path_expression& x, OutIterator out) {
    if(!compile_expr(x.first, out))
        return false;
    for(const auto& oper : x.rest) {
        if(!compile_expr(oper, out))
            return false;
    }
    return true;
}

template <typename OutIterator> bool compile_expr(const std::string& x, OutIterator out) {
    *out++ = static_cast<char>(byte_code::op_string);
    out = boost::range::copy(x, out);
    *out++ = 0;
    return true;
}

template <typename OutIterator> bool compile_expr(const jbson::document& x, OutIterator out) {
    *out++ = static_cast<char>(byte_code::op_object);
    out = boost::range::copy(x.data(), out);
    *out++ = 0;
    return true;
}

template <typename OutIterator> bool compile_expr(const jbson::array& x, OutIterator out) {
    *out++ = static_cast<char>(byte_code::op_array);
    out = boost::range::copy(x.data(), out);
    *out++ = 0;
    return true;
}

template <typename OutIterator> bool compile_expr(const ast::operation& x, OutIterator out) {
    using namespace std::placeholders;
    auto f = std::bind(Visitor(), _1, out);
    if(!x.operand_.apply_visitor(f))
        return false;
    switch(x.operator_) {
        case ast::optoken::plus:
            *out++ = static_cast<char>(byte_code::op_add);
            break;
        case ast::optoken::minus:
            *out++ = static_cast<char>(byte_code::op_sub);
            break;
        case ast::optoken::times:
            *out++ = static_cast<char>(byte_code::op_mul);
            break;
        case ast::optoken::divide:
            *out++ = static_cast<char>(byte_code::op_div);
            break;
        case ast::optoken::equal:
            *out++ = static_cast<char>(byte_code::op_eq);
            break;
        case ast::optoken::not_equal:
            *out++ = static_cast<char>(byte_code::op_neq);
            break;
        case ast::optoken::less:
            *out++ = static_cast<char>(byte_code::op_lt);
            break;
        case ast::optoken::less_equal:
            *out++ = static_cast<char>(byte_code::op_lte);
            break;
        case ast::optoken::greater:
            *out++ = static_cast<char>(byte_code::op_gt);
            break;
        case ast::optoken::greater_equal:
            *out++ = static_cast<char>(byte_code::op_gte);
            break;
        case ast::optoken::op_and:
            *out++ = static_cast<char>(byte_code::op_and);
            break;
        case ast::optoken::op_or:
            *out++ = static_cast<char>(byte_code::op_or);
            break;
        default:
            throw 0;
            return false;
    }
    return true;
}

template <typename OutIterator> bool compile_expr(const ast::unary& x, OutIterator out) {
    using namespace std::placeholders;
    auto f = std::bind(Visitor(), _1, out);
    if(!boost::apply_visitor(f, x.operand_))
        return false;
    switch(x.operator_) {
        case ast::optoken::negative:
            *out++ = static_cast<char>(byte_code::op_neg);
            break;
        case ast::optoken::op_not:
            *out++ = static_cast<char>(byte_code::op_not);
            break;
        case ast::optoken::positive:
            *out++ = static_cast<char>(byte_code::op_pos);
            break;
        default:
            throw 0;
            return false;
    }
    return true;
}

template <typename OutIterator> bool compile_expr(const ast::expression& x, OutIterator out) {
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

template <typename OutIterator> bool compile_expr(const ast::filter_expression& x, OutIterator out) {
    *out++ = static_cast<char>(byte_code::op_filter_expr);
    if(!compile_expr(x.expr, out))
        return false;
    *out++ = static_cast<char>(byte_code::op_return);
    return true;
}

template <typename OutIterator> bool compile_expr(const ast::eval_expression& x, OutIterator out) {
    *out++ = static_cast<char>(byte_code::op_expr);
    if(!compile_expr(x.expr, out))
        return false;
    *out++ = static_cast<char>(byte_code::op_return);
    return true;
}

} // namespace filter_expression
} // namespace detail
} // namespace jbson

#endif // JBSON_EXPRESSION_COMPILER_HPP
