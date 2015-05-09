//          Copyright Christian Manning 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_EXPRESSION_PARSER_HPP
#define JBSON_EXPRESSION_PARSER_HPP

#include <jbson/detail/config.hpp>

JBSON_PUSH_DISABLE_DOCUMENTATION_WARNING
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/range.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <experimental/optional>
#include <experimental/string_view>
#include <vector>
#include <map>
#include <boost/mpl/index_of.hpp>
JBSON_CLANG_POP_WARNINGS

#include <jbson/document.hpp>
#include <jbson/element.hpp>

namespace std {
using namespace experimental;
}

namespace jbson {
namespace detail {
namespace path_expression {

namespace ast {

struct nil {};
struct unary;
struct expression;

enum class optoken {
    plus,
    minus,
    times,
    divide,
    positive,
    negative,
    op_not,
    equal,
    not_equal,
    less,
    less_equal,
    greater,
    greater_equal,
    op_and,
    op_or,
    descend,
    recurse,
    all,
    root,
    current
};

struct field_access {
    std::string name;
};

struct slice {
    std::optional<int> start;
    std::optional<int> end;
};

struct filter_expression;
struct eval_expression;

typedef boost::variant<field_access, slice, boost::recursive_wrapper<filter_expression>,
                       boost::recursive_wrapper<eval_expression>> child_operand;

struct path_union {
    child_operand first;
    std::vector<child_operand> rest;
};

struct path_descend {
    optoken operator_;
    path_union operand_;
};

struct path_expression {
    field_access first;
    std::vector<path_descend> rest;
};

typedef boost::variant<nil, bool, int64_t, std::string, jbson::document, jbson::array, path_expression,
                       boost::recursive_wrapper<unary>, boost::recursive_wrapper<expression>> operand;

struct unary {
    optoken operator_;
    operand operand_;
};

struct operation {
    optoken operator_;
    operand operand_;
};

struct expression {
    operand first;
    std::vector<operation> rest;
};

struct filter_expression {
    expression expr;
};

struct eval_expression {
    expression expr;
};

inline std::ostream& operator<<(std::ostream& out, optoken tk) {
    out << (int)tk;
    return out;
}
inline std::ostream& operator<<(std::ostream& out, nil) {
    out << "nil";
    return out;
}
inline std::ostream& operator<<(std::ostream& out, path_expression const& var) {
    out << var.first.name;
    return out;
}

} // namespace ast

template <typename T, typename... Ts> bool is_which(const boost::variant<Ts...>& var) {
    using types = typename boost::mpl::vector<Ts...>;
    return var.which() == boost::mpl::index_of<types, T>::type::value;
}

constexpr std::string_view operator""_sv(const char* str, size_t len) {
    return {str, len};
}

inline bool parse_path_expression(std::string_view& expr, ast::path_expression& ast);
inline bool parse_path_expression(std::string_view& expr, ast::operand& ast);
inline bool parse_path_descent(std::string_view& expr, std::vector<ast::path_descend>& ast);
inline bool parse_path_descent(std::string_view& expr, ast::path_descend& ast);
inline bool parse_union(std::string_view& expr, ast::path_union& ast);
inline bool parse_eval_operand(std::string_view& expr, ast::child_operand& ast);
inline bool parse_wildcard(std::string_view& expr, ast::field_access& ast);
inline bool parse_wildcard(std::string_view& expr, ast::child_operand& ast);
inline bool parse_field(std::string_view& expr, ast::field_access& ast);
inline bool parse_field(std::string_view& expr, ast::child_operand& ast);
inline bool parse_slice(std::string_view& expr, ast::slice& ast);
inline bool parse_slice(std::string_view& expr, ast::child_operand& ast);
inline bool parse_eval_expression(std::string_view& expr, ast::expression& ast);
inline bool parse_logical_expression(std::string_view& expr, ast::expression& ast);
inline bool parse_logical_expression(std::string_view& expr, ast::operand& ast);
inline bool parse_equality_expression(std::string_view& expr, ast::expression& ast);
inline bool parse_equality_expression(std::string_view& expr, ast::operand& ast);
inline bool parse_relational_expression(std::string_view& expr, ast::expression& ast);
inline bool parse_relational_expression(std::string_view& expr, ast::operand& ast);
inline bool parse_additive_expression(std::string_view& expr, ast::expression& ast);
inline bool parse_additive_expression(std::string_view& expr, ast::operand& ast);
inline bool parse_multiplicative_expression(std::string_view& expr, ast::expression& ast);
inline bool parse_multiplicative_expression(std::string_view& expr, ast::operand& ast);
inline bool parse_unary_expression(std::string_view& expr, ast::operand& ast);
inline bool parse_unary_expression(std::string_view& expr, ast::unary& ast);
inline bool parse_primary_expression(std::string_view& expr, ast::operand& ast);
inline bool parse_int(std::string_view& expr, ast::operand& ast);
inline bool parse_quoted_string(std::string_view& expr, ast::operand& ast);
inline bool parse_path(std::string_view& expr, ast::operand& ast);
inline bool parse_parens_expression(std::string_view& expr, ast::expression& ast);
inline bool parse_parens_expression(std::string_view& expr, ast::operand& ast);
inline bool parse_bool(std::string_view& expr, ast::operand& ast);
inline void skip_space(std::string_view& expr);

inline bool parse_path_expression(std::string_view& expr, ast::operand& ast) {
    ast::path_expression path_expression;
    return parse_path_expression(expr, path_expression) && (ast = path_expression, true);
}

inline bool parse_path_expression(std::string_view& expr, ast::path_expression& ast) {
    skip_space(expr);

    if(parse_wildcard(expr, ast.first) || parse_field(expr, ast.first)) {
        skip_space(expr);
        auto loc_expr = expr;
        while(!loc_expr.empty() && parse_path_descent(loc_expr, ast.rest))
            ;
        expr = loc_expr;
        return true;
    }

    return false;
}

inline bool parse_path_descent(std::string_view& expr, std::vector<ast::path_descend>& ast) {
    ast::path_descend path;
    return parse_path_descent(expr, path) && (ast.push_back(std::move(path)), true);
}

inline bool parse_path_descent(std::string_view& expr, ast::path_descend& ast) {
    auto loc_expr = expr;
    skip_space(loc_expr);
    if(boost::algorithm::any_of_equal(".["_sv, loc_expr.front())) {
        auto access_operator = loc_expr.front();
        loc_expr.remove_prefix(1);
        if(access_operator == '.' && loc_expr.front() == '.') {
            loc_expr.remove_prefix(1);
            ast.operator_ = ast::optoken::recurse;
        } else
            ast.operator_ = ast::optoken::descend;
        if(parse_union(loc_expr, ast.operand_)) {
            skip_space(loc_expr);
            if(!loc_expr.empty() && loc_expr.front() == ']')
                loc_expr.remove_prefix(1);
            skip_space(loc_expr);
            expr = loc_expr;
            return true;
        }
    }
    return false;
}

inline bool parse_eval_operand(std::string_view& expr, ast::child_operand& ast) {
    if(boost::algorithm::any_of_equal("?("_sv, expr.front())) {
        auto loc_expr = expr;
        const bool filter = expr.front() == '?';
        loc_expr.remove_prefix(1);
        if(loc_expr.front() == '(')
            loc_expr.remove_prefix(1);
        ast::expression eval_expr;
        if(parse_logical_expression(loc_expr, eval_expr) && loc_expr.front() == ')') {
            loc_expr.remove_prefix(1);
            if(filter)
                ast = ast::filter_expression{std::move(eval_expr)};
            else
                ast = ast::eval_expression{std::move(eval_expr)};
            expr = loc_expr;
            return true;
        }
    }
    return false;
}

inline bool parse_union(std::string_view& expr, ast::path_union& ast) {
    auto loc_expr = expr;
    std::vector<ast::child_operand> operands;
    for(ast::child_operand operand;
        !loc_expr.empty() && (parse_eval_operand(loc_expr, operand) || parse_wildcard(loc_expr, operand) ||
                              parse_field(loc_expr, operand) || parse_slice(loc_expr, operand));
        skip_space(loc_expr)) {
        operands.push_back(std::move(operand));
        skip_space(loc_expr);
        if(loc_expr.front() == ',') {
            loc_expr.remove_prefix(1);
            skip_space(loc_expr);
        }
        expr = loc_expr;
    }
    if(!operands.empty()) {
        ast.first = operands.front();
        ast.rest = std::vector<ast::child_operand>(std::next(operands.begin()), operands.end());
        return true;
    }
    return false;
}

inline bool parse_wildcard(std::string_view& expr, ast::child_operand& ast) {
    ast::field_access field;
    return parse_wildcard(expr, field) && (ast = field, true);
}

inline bool parse_wildcard(std::string_view& expr, ast::field_access& ast) {
    skip_space(expr);
    if(expr.front() == '*') {
        ast.name = {expr.front()};
        expr.remove_prefix(1);
        return true;
    }
    return false;
}

inline bool parse_field(std::string_view& expr, ast::child_operand& ast) {
    ast::field_access field;
    return parse_field(expr, field) && (ast = field, true);
}

inline bool parse_field(std::string_view& expr, ast::field_access& ast) {
    skip_space(expr);
    auto loc_expr = expr;
    std::optional<char> quote;
    if(boost::algorithm::any_of_equal("\"'"_sv, loc_expr.front())) {
        quote = loc_expr.front();
        loc_expr.remove_prefix(1);
    }
    if(::isalnum(loc_expr.front()) || boost::algorithm::any_of_equal("_@$"_sv, loc_expr.front())) {
        std::string field;
        do {
            field += loc_expr.front();
            loc_expr.remove_prefix(1);
        } while(!loc_expr.empty() &&
                (::isalnum(loc_expr.front()) || boost::algorithm::any_of_equal("_@$"_sv, loc_expr.front()) ||
                 (quote && (::isspace(loc_expr.front()) || loc_expr.front() != quote))));
        if(quote) {
            loc_expr.remove_prefix(1);
        }
        ast.name = field;
        expr = loc_expr;
        return true;
    }

    return false;
}

inline bool parse_slice(std::string_view& expr, ast::child_operand& ast) {
    ast::slice slice;
    return parse_slice(expr, slice) && (ast = slice, true);
}

inline bool parse_slice(std::string_view& expr, ast::slice& ast) {
    skip_space(expr);

    if(!expr.empty() && (::isdigit(expr.front()) || expr.front() == ':')) {
        auto loc_expr = expr;
        std::string int_str;
        while(::isdigit(loc_expr.front())) {
            int_str += loc_expr.front();
            loc_expr.remove_prefix(1);
        }
        if(!int_str.empty()) {
            ast.start = std::stoi(int_str);
            int_str.clear();
        }
        skip_space(expr);
        if(loc_expr.front() != ':')
            return false;
        loc_expr.remove_prefix(1);
        while(::isdigit(loc_expr.front())) {
            int_str += loc_expr.front();
            loc_expr.remove_prefix(1);
        }
        if(!int_str.empty()) {
            ast.end = std::stoi(int_str);
            int_str.clear();
        }
        return true;
    }

    return false;
}

inline bool parse_eval_expression(std::string_view& expr, ast::expression& ast) {
    return parse_logical_expression(expr, ast) && expr.empty();
}

inline bool parse_logical_expression(std::string_view& expr, ast::operand& ast) {
    ast::expression logical_expression;
    return parse_logical_expression(expr, logical_expression) && (ast = logical_expression, true);
}

inline bool parse_logical_expression(std::string_view& expr, ast::expression& ast) {
    static const std::map<std::string_view, ast::optoken> logical_op{{"||", ast::optoken::op_or},
                                                                     {"&&", ast::optoken::op_and}};
    skip_space(expr);

    if(!expr.empty() && parse_equality_expression(expr, ast.first)) {
        skip_space(expr);
        while(expr.size() >= 2 && logical_op.count(expr.substr(0, 2)) == 1) {
            skip_space(expr);
            auto loc_expr = expr;
            ast::operation op;
            op.operator_ = logical_op.at(expr.substr(0, 2));
            loc_expr.remove_prefix(2);
            if(!parse_equality_expression(loc_expr, op.operand_))
                break;
            ast.rest.push_back(std::move(op));
            expr = loc_expr;
            skip_space(expr);
        }
        return true;
    }

    return false;
}

inline bool parse_equality_expression(std::string_view& expr, ast::operand& ast) {
    ast::expression equality_expression;
    return parse_equality_expression(expr, equality_expression) && (ast = equality_expression, true);
}

inline bool parse_equality_expression(std::string_view& expr, ast::expression& ast) {
    static const std::map<std::string_view, ast::optoken> equality_op{{"==", ast::optoken::equal},
                                                                      {"!=", ast::optoken::not_equal}};
    skip_space(expr);

    if(!expr.empty() && parse_relational_expression(expr, ast.first)) {
        skip_space(expr);
        while(expr.size() >= 2 && equality_op.count(expr.substr(0, 2)) == 1) {
            skip_space(expr);
            auto loc_expr = expr;
            ast::operation op;
            op.operator_ = equality_op.at(expr.substr(0, 2));
            loc_expr.remove_prefix(2);
            if(!parse_relational_expression(loc_expr, op.operand_))
                break;
            ast.rest.push_back(std::move(op));
            expr = loc_expr;
            skip_space(expr);
        }
        return true;
    }

    return false;
}

inline bool parse_relational_expression(std::string_view& expr, ast::operand& ast) {
    ast::expression relational_expression;
    return parse_relational_expression(expr, relational_expression) && (ast = relational_expression, true);
}

inline bool parse_relational_expression(std::string_view& expr, ast::expression& ast) {
    static const std::map<std::string_view, ast::optoken> relational_op{{"<", ast::optoken::less},
                                                                        {">", ast::optoken::greater},
                                                                        {"<=", ast::optoken::less_equal},
                                                                        {">=", ast::optoken::greater_equal}};
    skip_space(expr);

    if(!expr.empty() && parse_additive_expression(expr, ast.first)) {
        skip_space(expr);
        while(!expr.empty() && relational_op.count(expr.substr(0, 1)) == 1) {
            skip_space(expr);
            auto loc_expr = expr;
            ast::operation op;
            op.operator_ = relational_op.at(expr.substr(0, 1));
            if(expr.size() >= 2 && relational_op.count(expr.substr(0, 2)) == 1) {
                op.operator_ = relational_op.at(expr.substr(0, 2));
                loc_expr.remove_prefix(1);
            }
            loc_expr.remove_prefix(1);
            if(!parse_additive_expression(loc_expr, op.operand_))
                break;
            ast.rest.push_back(std::move(op));
            expr = loc_expr;
            skip_space(expr);
        }
        return true;
    }

    return false;
}

inline bool parse_additive_expression(std::string_view& expr, ast::operand& ast) {
    ast::expression additive_expression;
    return parse_additive_expression(expr, additive_expression) && (ast = additive_expression, true);
}

inline bool parse_additive_expression(std::string_view& expr, ast::expression& ast) {
    static const std::map<char, ast::optoken> additive_op{{'+', ast::optoken::plus}, {'-', ast::optoken::minus}};
    skip_space(expr);

    if(!expr.empty() && parse_multiplicative_expression(expr, ast.first)) {
        skip_space(expr);
        while(!expr.empty() && additive_op.count(expr.front()) == 1) {
            skip_space(expr);
            auto loc_expr = expr;
            ast::operation op;
            op.operator_ = additive_op.at(expr.front());
            loc_expr.remove_prefix(1);
            if(!parse_multiplicative_expression(loc_expr, op.operand_))
                break;
            ast.rest.push_back(std::move(op));
            expr = loc_expr;
            skip_space(expr);
        }
        return true;
    }

    return false;
}

inline bool parse_multiplicative_expression(std::string_view& expr, ast::operand& ast) {
    ast::expression multiplicative_expression;
    return parse_multiplicative_expression(expr, multiplicative_expression) && (ast = multiplicative_expression, true);
}

inline bool parse_multiplicative_expression(std::string_view& expr, ast::expression& ast) {
    static const std::map<char, ast::optoken> multiplicative_op{{'*', ast::optoken::times},
                                                                {'/', ast::optoken::divide}};
    skip_space(expr);

    if(!expr.empty() && parse_unary_expression(expr, ast.first)) {
        skip_space(expr);
        while(!expr.empty() && multiplicative_op.count(expr.front()) == 1) {
            skip_space(expr);
            auto loc_expr = expr;
            ast::operation op;
            op.operator_ = multiplicative_op.at(expr.front());
            loc_expr.remove_prefix(1);
            if(!parse_unary_expression(loc_expr, op.operand_))
                break;
            ast.rest.push_back(std::move(op));
            expr = loc_expr;
            skip_space(expr);
        }
        return true;
    }

    return false;
}

inline bool parse_unary_expression(std::string_view& expr, ast::operand& ast) {
    skip_space(expr);
    ast::unary unary;
    return !expr.empty() &&
           (parse_primary_expression(expr, ast) || (parse_unary_expression(expr, unary) && (ast = unary, true)));
}

inline bool parse_unary_expression(std::string_view& expr, ast::unary& ast) {
    static const std::map<char, ast::optoken> unary_op{
        {'+', ast::optoken::positive}, {'-', ast::optoken::negative}, {'!', ast::optoken::op_not}};
    skip_space(expr);
    if(unary_op.count(expr.front()) == 1) {
        ast.operator_ = unary_op.at(expr.front());
        expr.remove_prefix(1);
        return parse_primary_expression(expr, ast.operand_);
    }
    return false;
}

inline bool parse_primary_expression(std::string_view& expr, ast::operand& operand) {
    skip_space(expr);
    return !expr.empty() &&
           (parse_int(expr, operand) || parse_quoted_string(expr, operand) || parse_bool(expr, operand) ||
            parse_path_expression(expr, operand) || parse_parens_expression(expr, operand));
}

inline bool parse_int(std::string_view& expr, ast::operand& ast) {
    if(::isdigit(expr.front())) {
        auto loc_expr = expr;
        std::string int_token{};
        while(!loc_expr.empty() && ::isdigit(loc_expr.front())) {
            int_token += loc_expr.front();
            loc_expr.remove_prefix(1);
        }
        expr = loc_expr;
        ast = static_cast<int64_t>(std::stoll(int_token));
        return true;
    }
    return false;
}

inline bool parse_quoted_string(std::string_view& expr, ast::operand& ast) {
    if(boost::algorithm::any_of_equal("\"'"_sv, expr.front())) {
        auto loc_expr = expr;
        std::string quoted_string{};
        const auto quote = loc_expr.front();
        loc_expr.remove_prefix(1);

        while(!loc_expr.empty() && quote != loc_expr.front()) {
            if('\\' == loc_expr.front()) {
                loc_expr.remove_prefix(1);
                if(quote != loc_expr.front())
                    quoted_string += '\\';
            }
            quoted_string += loc_expr.front();
            loc_expr.remove_prefix(1);
        }
        loc_expr.remove_prefix(1);
        expr = loc_expr;
        ast = quoted_string;
        return true;
    }
    return false;
}

inline bool parse_subpath(std::string_view& loc_expr, std::string& path) {
    skip_space(loc_expr);
    if(loc_expr.front() == '[')
        loc_expr.remove_prefix(1);
    while(!loc_expr.empty()) {
        skip_space(loc_expr);
        path += loc_expr.front();
        loc_expr.remove_prefix(1);
    }

    return false;
}

inline bool parse_path(std::string_view& expr, ast::operand& ast) {
    if(::isalpha(expr.front()) || boost::algorithm::any_of_equal(".@_"_sv, expr.front())) {
        auto loc_expr = expr;
        std::string path{};
        do {
            if(loc_expr.front() == '[' && parse_subpath(expr, path))
                continue;

            path += loc_expr.front();
            loc_expr.remove_prefix(1);
        } while(!loc_expr.empty() &&
                (::isalnum(loc_expr.front()) || boost::algorithm::any_of_equal(".@_["_sv, loc_expr.front())));
        expr = loc_expr;
        ast = ast::path_expression{{std::move(path)}, {}};
        return true;
    }
    return false;
}

inline bool parse_bool(std::string_view& expr, ast::operand& ast) {
    if(expr.size() >= 5 && expr.substr(0, 5) == "false") {
        expr.remove_prefix(5);
        ast = false;
    } else if(expr.size() >= 4 && expr.substr(0, 4) == "true") {
        expr.remove_prefix(4);
        ast = true;
    } else {
        return false;
    }
    return true;
}

inline bool parse_parens_expression(std::string_view& expr, ast::operand& ast) {
    ast::expression parens_expression;
    return parse_parens_expression(expr, parens_expression) && (ast = parens_expression, true);
}

inline bool parse_parens_expression(std::string_view& expr, ast::expression& ast) {
    if(expr.front() == '(') {
        auto loc_expr = expr;
        loc_expr.remove_prefix(1);

        if(parse_logical_expression(loc_expr, ast) && (skip_space(loc_expr), loc_expr.front() == ')')) {
            loc_expr.remove_prefix(1);
            expr = loc_expr;
            return true;
        }
    }
    return false;
}

inline void skip_space(std::string_view& expr) {
    while(!expr.empty() && ::isspace(expr.front())) {
        expr.remove_prefix(1);
    }
}

} // namespace path_expression
} // namespace detail
} // namespace jbson

#endif // JBSON_EXPRESSION_PARSER_HPP
