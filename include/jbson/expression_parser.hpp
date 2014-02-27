//          Copyright Christian Manning 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_EXPRESSION_PARSER_HPP
#define JBSON_EXPRESSION_PARSER_HPP

#include <boost/variant/recursive_variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/qi.hpp>
#include <boost/optional.hpp>

#ifdef BOOST_SPIRIT_DEBUG_OUT
#undef BOOST_SPIRIT_DEBUG_OUT
#endif
#define BOOST_SPIRIT_DEBUG_OUT ::std::clog

namespace jbson {
namespace detail {
namespace expression {

namespace ast {

struct nil {};
struct unary;
struct expression;

struct variable {
    //    variable() = default;
    //    variable(std::string const& name) : name(name) {}
    std::string name;
};

typedef boost::variant<nil, bool, int64_t, std::string, variable, boost::recursive_wrapper<unary>,
                       boost::recursive_wrapper<expression>> operand;

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
    op_or
};

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
    std::list<operation> rest;
};

inline std::ostream& operator<<(std::ostream& out, optoken tk) {
    out << (int)tk;
    return out;
}
inline std::ostream& operator<<(std::ostream& out, nil) {
    out << "nil";
    return out;
}
inline std::ostream& operator<<(std::ostream& out, variable const& var) {
    out << var.name;
    return out;
}

} // namespace ast

template <typename Iterator> struct error_handler {
    template <typename, typename, typename> struct result {
        typedef void type;
    };

    error_handler(Iterator first, Iterator last) : first(first), last(last) {}

    template <typename Message, typename What>
    void operator()(Message const& message, What const& what, Iterator err_pos) const {
        int line;
        Iterator line_start = get_pos(err_pos, line);
        if(err_pos != last) {
            std::clog << message << what << " line " << line << ':' << std::endl;
            std::clog << get_line(line_start) << std::endl;
            for(; line_start != err_pos; ++line_start)
                std::clog << ' ';
            std::clog << '^' << std::endl;
        } else {
            std::clog << "Unexpected end of file. ";
            std::clog << message << what << " line " << line << std::endl;
        }
    }

    Iterator get_pos(Iterator err_pos, int& line) const {
        line = 1;
        Iterator i = first;
        Iterator line_start = first;
        while(i != err_pos) {
            bool eol = false;
            if(i != err_pos && *i == '\r') // CR
            {
                eol = true;
                line_start = ++i;
            }
            if(i != err_pos && *i == '\n') // LF
            {
                eol = true;
                line_start = ++i;
            }
            if(eol)
                ++line;
            else
                ++i;
        }
        return line_start;
    }

    std::string get_line(Iterator err_pos) const {
        Iterator i = err_pos;
        // position i to the next EOL
        while(i != last && (*i != '\r' && *i != '\n'))
            ++i;
        return std::string(err_pos, i);
    }

    Iterator first;
    Iterator last;
    std::vector<Iterator> iters;
};

template <typename Iterator>
struct parser : boost::spirit::qi::grammar<Iterator, ast::expression(), boost::spirit::ascii::space_type> {
    boost::spirit::qi::rule<Iterator, ast::expression(), boost::spirit::ascii::space_type> expr, equality_expr,
        relational_expr, logical_expr, additive_expr, multiplicative_expr;
    boost::spirit::qi::rule<Iterator, ast::operand(), boost::spirit::ascii::space_type> unary_expr, primary_expr;
    boost::spirit::qi::rule<Iterator, ast::variable(), boost::spirit::ascii::space_type> identifier;
    boost::spirit::qi::rule<Iterator, std::string(), boost::spirit::ascii::space_type> quoted_string;
    boost::spirit::qi::symbols<char, ast::optoken> equality_op, relational_op, logical_op, additive_op,
        multiplicative_op, unary_op;
    boost::spirit::qi::symbols<char> keywords;

    explicit parser(error_handler<Iterator>& error_handle) : parser::base_type(expr) {
        namespace qi = boost::spirit::qi;
        qi::_3_type _3;
        qi::_4_type _4;

        qi::char_type char_;
        qi::any_int_parser<int64_t> int_;
        qi::raw_type raw;
        qi::lexeme_type lexeme;
        qi::alpha_type alpha;
        qi::alnum_type alnum;
        qi::bool_type bool_;

        using qi::on_error;
        using qi::on_success;
        using qi::fail;
        using boost::phoenix::function;

        typedef function<error_handler<Iterator>> error_handler_function;

        // Tokens
        equality_op.add("==", ast::optoken::equal)("!=", ast::optoken::not_equal);

        relational_op.add("<", ast::optoken::less)("<=", ast::optoken::less_equal)(">", ast::optoken::greater)(
            ">=", ast::optoken::greater_equal);

        logical_op.add("&&", ast::optoken::op_and)("||", ast::optoken::op_or);

        additive_op.add("+", ast::optoken::plus)("-", ast::optoken::minus);

        multiplicative_op.add("*", ast::optoken::times)("/", ast::optoken::divide);

        unary_op.add("+", ast::optoken::positive)("-", ast::optoken::negative)("!", ast::optoken::op_not);

        keywords.add("true")("false");

        // Main expression grammar
        expr = equality_expr.alias();

        equality_expr = relational_expr >> *(equality_op > relational_expr);

        relational_expr = logical_expr >> *(relational_op > logical_expr);

        logical_expr = additive_expr >> *(logical_op > additive_expr);

        additive_expr = multiplicative_expr >> *(additive_op > multiplicative_expr);

        multiplicative_expr = unary_expr >> *(multiplicative_op > unary_expr);

        unary_expr = primary_expr | (unary_op > primary_expr);

        primary_expr = int_ | quoted_string | identifier | bool_ | '(' > expr > ')';

        quoted_string = lexeme['"' >> +(char_ - '"') >> '"'] | lexeme['\'' >> +(char_ - '\'') >> '\''];

        identifier = !keywords >> raw[lexeme[(alpha | '_' | '@' | '.') >> *(alnum | '_' | '@' | '.')]];

        ///////////////////////////////////////////////////////////////////////
        // Debugging and error handling and reporting support.
        BOOST_SPIRIT_DEBUG_NODES((expr)(equality_expr)(relational_expr)(logical_expr)(additive_expr)(
            multiplicative_expr)(unary_expr)(primary_expr)(identifier)(quoted_string));

        ///////////////////////////////////////////////////////////////////////
        // Error handling: on error in expr, call error_handler.
        on_error<fail>(expr, error_handler_function(error_handle)("Error! Expecting ", _4, _3));
    }
};

} // namespace expression
} // namespace detail
} // namespace jbson

BOOST_FUSION_ADAPT_STRUCT(jbson::detail::expression::ast::unary,
                          (jbson::detail::expression::ast::optoken, operator_)(jbson::detail::expression::ast::operand,
                                                                               operand_))

BOOST_FUSION_ADAPT_STRUCT(jbson::detail::expression::ast::variable, (std::string, name))

BOOST_FUSION_ADAPT_STRUCT(jbson::detail::expression::ast::operation,
                          (jbson::detail::expression::ast::optoken, operator_)(jbson::detail::expression::ast::operand,
                                                                               operand_))

BOOST_FUSION_ADAPT_STRUCT(jbson::detail::expression::ast::expression,
                          (jbson::detail::expression::ast::operand,
                           first)(std::list<jbson::detail::expression::ast::operation>, rest))

#endif // JBSON_EXPRESSION_PARSER_HPP
