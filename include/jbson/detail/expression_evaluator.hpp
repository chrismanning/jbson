//          Copyright Christian Manning 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef JBSON_EXPRESSION_EVALUATOR_HPP
#define JBSON_EXPRESSION_EVALUATOR_HPP

#include <array>

#include <boost/variant.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/lexical_cast.hpp>

#include <jbson/element.hpp>
#include <jbson/detail/expression_compiler.hpp>

namespace jbson {
namespace detail {
namespace path_expression {

template <typename ContainerT, typename EContainerT>
using variable_type =
    boost::variant<bool, int64_t, std::string, basic_element<EContainerT>, basic_document<ContainerT, EContainerT>,
                   basic_array<ContainerT, EContainerT>, std::vector<basic_element<EContainerT>>>;

template <typename ContainerT, typename EContainerT>
std::optional<variable_type<ContainerT, EContainerT>> load_by_name(variable_type<ContainerT, EContainerT> var,
                                                                   std::string_view name) {
    using variable_type = variable_type<ContainerT, EContainerT>;
    if(is_which<basic_element<EContainerT>>(var)) {
        auto elem = get<basic_element<EContainerT>>(var);
        if(elem.type() == element_type::document_element) {
            return load_by_name(variable_type{get<element_type::document_element>(elem)}, name);
        } else if(elem.type() == element_type::array_element) {
            if(boost::algorithm::all_of(name, ::isdigit)) {
                auto arr = get<element_type::array_element>(elem);
                auto it = arr.find(std::stoul(name.to_string()));
                if(it != arr.end())
                    return variable_type{*it};
            }
        }
    } else if(is_which<basic_document<ContainerT, EContainerT>>(var)) {
        auto doc = get<basic_document<ContainerT, EContainerT>>(var);
        auto it = doc.find(name);
        if(it != doc.end())
            return variable_type{*it};
    } else if(is_which<basic_array<ContainerT, EContainerT>>(var)) {
        auto arr = get<basic_array<ContainerT, EContainerT>>(var);
        auto it = arr.find(std::stoul(name.to_string()));
        if(it != arr.end())
            return variable_type{*it};
    } else if(is_which<std::vector<basic_element<EContainerT>>>(var)) {
        auto elems = get<std::vector<basic_element<EContainerT>>>(var);
        std::vector<basic_element<EContainerT>> vars;
        for(auto&& elem : elems) {
            if(elem.name() == name) {
                vars.push_back(elem);
            } else if(auto loaded_var = load_by_name(variable_type{elem}, name)) {
                vars.push_back(get<basic_element<EContainerT>>(*loaded_var));
            }
        }
        if(!vars.empty())
            return variable_type{vars};
    }

    return std::nullopt;
}

template <typename ContainerT, typename EContainerT>
std::vector<basic_element<EContainerT>> load_wildcard(const variable_type<ContainerT, EContainerT>& var) {
    std::vector<basic_element<EContainerT>> all_elems;

    if(is_which<basic_element<EContainerT>>(var)) {
        auto elem = get<basic_element<EContainerT>>(var);
        if(elem.type() == element_type::document_element) {
            boost::push_back(all_elems, get<element_type::document_element>(elem));
        } else if(elem.type() == element_type::array_element) {
            boost::push_back(all_elems, get<element_type::array_element>(elem));
        }
    } else if(is_which<basic_document<ContainerT, EContainerT>>(var)) {
        boost::push_back(all_elems, get<basic_document<ContainerT, EContainerT>>(var));
    } else if(is_which<basic_array<ContainerT, EContainerT>>(var)) {
        boost::push_back(all_elems, get<basic_array<ContainerT, EContainerT>>(var));
    } else if(is_which<std::vector<basic_element<EContainerT>>>(var)) {
        boost::push_back(all_elems, get<std::vector<basic_element<EContainerT>>>(var));
    }

    return all_elems;
}

template <typename EContainerT> struct union_visitor {
    using result_type = std::vector<basic_element<EContainerT>>;

    result_type operator()(const result_type& a, const basic_element<EContainerT>& b) const {
        auto res = a;
        res.push_back(b);
        return res;
    }

    template <typename ElemRangeT>
    result_type
    operator()(const result_type& a, ElemRangeT&& doc,
               std::enable_if_t<detail::is_range_of_value<ElemRangeT, boost::mpl::quote1<detail::is_element>>::value>* =
                   nullptr) const {
        auto res = a;
        boost::push_back(res, doc);
        return res;
    }

    template <typename ElemRangeT>
    result_type operator()(
        const result_type& a, ElemRangeT&&,
        std::enable_if_t<!detail::is_range_of_value<ElemRangeT, boost::mpl::quote1<detail::is_element>>::value>* =
            nullptr) const {
        return a;
    }

    template <typename ElemRangeT>
    result_type
    operator()(ElemRangeT&& doc,
               std::enable_if_t<detail::is_range_of_value<ElemRangeT, boost::mpl::quote1<detail::is_element>>::value>* =
                   nullptr) const {
        return result_type(std::begin(doc), std::end(doc));
    }

    result_type operator()(const basic_element<EContainerT>& elem) const {
        return {elem};
    }

    template <typename T>
    result_type
    operator()(T&&, std::enable_if_t<!detail::is_range_of_value<T, boost::mpl::quote1<detail::is_element>>::value &&
                                     !detail::is_element<std::decay_t<T>>::value>* = nullptr) const {
        return {};
    }

    template <typename T1, typename T2> result_type operator()(const T1& a, const T2& b) const {
        return (*this)((*this)(a), b);
    }
};

template <typename ContainerT, typename EContainerT> struct equality_operation {
    using variable_t = variable_type<ContainerT, EContainerT>;
    using element_t = basic_element<EContainerT>;
    using document_t = basic_document<ContainerT, EContainerT>;
    using array_t = basic_array<ContainerT, EContainerT>;
    using vector_t = std::vector<element_t>;

    using result_type = vector_t;

    struct element_visitor {
        template <typename T> variable_t operator()(std::string_view, element_type) const {
            return {false};
        }
        template <typename T> variable_t operator()(std::string_view, element_type, T&& t) const {
            return {t};
        }
    };

    template <typename T>
    static constexpr bool is_element_range =
        detail::is_range_of_value<T, boost::mpl::quote1<detail::is_element>>::value;

    vector_t operator()(const vector_t& elems, element_t t) const {
        auto range = elems | boost::adaptors::filtered([&t](auto&& elem) { return elem == t; });
        return {range.begin(), range.end()};
    }

    vector_t operator()(const vector_t& elems, int64_t val) const {
        auto range = elems | boost::adaptors::filtered([&val](auto&& elem) {
            if(elem.type() == element_type::int32_element || elem.type() == element_type::int64_element) {
                return get<int64_t>(elem) == val;
            }
            return false;
        });
        return {range.begin(), range.end()};
    }

    vector_t operator()(const vector_t& elems, std::string val) const {
        auto range = elems | boost::adaptors::filtered([&val](auto&& elem) {
            if(elem.type() == element_type::string_element) {
                return get<element_type::string_element>(elem) == val;
            }
            return false;
        });
        return {range.begin(), range.end()};
    }

    template <typename T> vector_t operator()(const vector_t&, T&&) const {
        return {};
    }
};

template <typename ContainerT, typename EContainerT> struct element_variable_visitor {
    using result_type = variable_type<ContainerT, EContainerT>;

    result_type operator()(std::string_view, element_type type) const {
        BOOST_THROW_EXCEPTION(jbson_path_error{} << detail::actual_element_type{type});
    }

    result_type operator()(std::string_view, element_type type, const std::array<char, 12>&) const {
        BOOST_THROW_EXCEPTION(jbson_path_error{} << detail::actual_element_type{type});
    }

    template <typename StringT>
    result_type operator()(std::string_view, element_type type, const std::tuple<StringT, StringT>&) const {
        BOOST_THROW_EXCEPTION(jbson_path_error{} << detail::actual_element_type{type});
    }

    template <typename StringT>
    result_type operator()(std::string_view, element_type type,
                           const std::tuple<StringT, std::array<char, 12>>&) const {
        BOOST_THROW_EXCEPTION(jbson_path_error{} << detail::actual_element_type{type});
    }

    template <typename StringT, typename C, typename E>
    result_type operator()(std::string_view, element_type type,
                           const std::tuple<StringT, basic_document<C, E>>&) const {
        BOOST_THROW_EXCEPTION(jbson_path_error{} << detail::actual_element_type{type});
    }

    result_type operator()(std::string_view, element_type, double val) const {
        return result_type{static_cast<int64_t>(val)};
    }

    result_type operator()(std::string_view, element_type, std::string_view val) const {
        return result_type{static_cast<std::string>(val)};
    }

    template <typename C, typename E>
    result_type operator()(std::string_view, element_type, basic_array<C, E> val) const {
        return result_type{basic_array<ContainerT, EContainerT>(val)};
    }

    template <typename C, typename E>
    result_type operator()(std::string_view, element_type, basic_document<C, E> val) const {
        return result_type{static_cast<basic_document<ContainerT, EContainerT>>(val)};
    }

    template <typename T>
    std::enable_if_t<detail::is_document<T>::value, result_type> operator()(std::string_view, element_type,
                                                                            T&& val) const {
        return result_type{std::forward<T>(val)};
    }
};

template <typename ContainerT, typename EContainerT> auto ensure_range(variable_type<ContainerT, EContainerT> var) {
    std::vector<basic_element<EContainerT>> range;
    if(is_which<basic_element<EContainerT>>(var)) {
        auto& elem = get<basic_element<EContainerT>>(var);
        var = elem.visit(element_variable_visitor<ContainerT, EContainerT>{});
    }
    if(is_which<std::vector<basic_element<EContainerT>>>(var)) {
        range = get<std::vector<basic_element<EContainerT>>>(var);
    } else if(is_which<basic_document<ContainerT, EContainerT>>(var)) {
        boost::range::push_back(range, get<basic_document<ContainerT, EContainerT>>(var));
    } else if(is_which<basic_array<ContainerT, EContainerT>>(var)) {
        boost::range::push_back(range, get<basic_array<ContainerT, EContainerT>>(var));
    } else {
        BOOST_THROW_EXCEPTION(jbson_path_error{} << detail::actual_type{var.type()});
    }
    return range;
}

template <typename ContainerT, typename EContainerT> struct eval_context {
    using variable_type = variable_type<ContainerT, EContainerT>;

    const basic_document<ContainerT, EContainerT>& root;
    variable_type current;
    std::vector<variable_type> stack;
    boost::iterator_range<std::vector<char>::iterator> code;

    template <typename CodeRangeT>
    explicit eval_context(const basic_document<ContainerT, EContainerT>& root, const variable_type& current,
                          CodeRangeT&& code)
        : root(root), current(current), code(code) {
        stack.push_back(current);
    }

    template <typename StackT> static variable_type pop_and_get(StackT& stack) {
        auto var = std::move(stack.back());
        stack.pop_back();
        return var;
    }

    using unary_op = variable_type (eval_context::*)(const variable_type&);
    using unary_op_const = variable_type (eval_context::*)(const variable_type&) const;
    using binary_op = variable_type (eval_context::*)(const variable_type&, const variable_type&);
    using binary_op_const = variable_type (eval_context::*)(const variable_type&, const variable_type&) const;

    variable_type apply_unary(unary_op fun) {
        return (this->*fun)(ensure_var(pop_and_get(stack)));
    }

    variable_type apply_unary(unary_op_const fun) {
        return (this->*fun)(ensure_var(pop_and_get(stack)));
    }

    variable_type apply_binary(binary_op fun) {
        auto a = pop_and_get(stack);
        auto b = pop_and_get(stack);
        return (this->*fun)(ensure_var(b), ensure_var(a));
    }

    variable_type apply_binary(binary_op_const fun) {
        auto a = pop_and_get(stack);
        auto b = pop_and_get(stack);
        return (this->*fun)(ensure_var(b), ensure_var(a));
    }

    static variable_type ensure_var(const variable_type& var) {
        if(is_which<basic_element<EContainerT>>(var))
            return get<basic_element<EContainerT>>(var).visit(element_variable_visitor<ContainerT, EContainerT>{});
        return var;
    }

    variable_type unary_negate(const variable_type& var) const {
        return -get<int64_t>(var);
    }

    virtual void unary_negate() {
        assert(!stack.empty());
        stack.push_back(apply_unary(&eval_context::unary_negate));
    }

    variable_type unary_plus(const variable_type& var) const {
        return +get<int64_t>(var);
    }

    virtual void unary_plus() {
        assert(!stack.empty());
        stack.push_back(apply_unary(&eval_context::unary_plus));
    }

    variable_type unary_not(const variable_type& var) const {
        return is_which<bool>(var) && !get<bool>(var);
    }

    virtual void unary_not() {
        assert(!stack.empty());
        stack.push_back(apply_unary(&eval_context::unary_not));
    }

    variable_type add(const variable_type& var1, const variable_type& var2) const {
        if(var1.which() == var2.which()) {
            if(is_which<std::string>(var1))
                return get<std::string>(var1) + get<std::string>(var2);
            if(is_which<int64_t>(var1))
                return get<int64_t>(var1) + get<int64_t>(var2);
        }
        BOOST_THROW_EXCEPTION(jbson_path_error{});
    }

    virtual void add() {
        assert(stack.size() >= 2);
        stack.push_back(apply_binary(&eval_context::add));
    }

    variable_type subtract(const variable_type& var1, const variable_type& var2) const {
        if(var1.which() == var2.which() && is_which<int64_t>(var1))
            return get<int64_t>(var1) - get<int64_t>(var2);
        BOOST_THROW_EXCEPTION(jbson_path_error{});
    }

    virtual void subtract() {
        assert(stack.size() >= 2);
        stack.push_back(apply_binary(&eval_context::subtract));
    }

    variable_type multiply(const variable_type& var1, const variable_type& var2) const {
        if(var1.which() == var2.which() && is_which<int64_t>(var1))
            return get<int64_t>(var1) * get<int64_t>(var2);
        BOOST_THROW_EXCEPTION(jbson_path_error{});
    }

    virtual void multiply() {
        assert(stack.size() >= 2);
        stack.push_back(apply_binary(&eval_context::multiply));
    }

    variable_type divide(const variable_type& var1, const variable_type& var2) const {
        if(var1.which() == var2.which() && is_which<int64_t>(var1))
            return get<int64_t>(var1) / get<int64_t>(var2);
        BOOST_THROW_EXCEPTION(jbson_path_error{});
    }

    virtual void divide() {
        assert(stack.size() >= 2);
        stack.push_back(apply_binary(&eval_context::divide));
    }

    variable_type equals(const variable_type& var1, const variable_type& var2) const {
        return var1 == var2;
    }

    virtual void equals() {
        assert(stack.size() >= 2);
        stack.push_back(apply_binary(&eval_context::equals));
    }

    variable_type not_equals(const variable_type& var1, const variable_type& var2) const {
        return var1 != var2;
    }

    virtual void not_equals() {
        assert(stack.size() >= 2);
        stack.push_back(apply_binary(&eval_context::not_equals));
    }

    variable_type less(const variable_type& var1, const variable_type& var2) const {
        return var1.which() == var2.which() && var1 < var2;
    }

    virtual void less() {
        assert(stack.size() >= 2);
        stack.push_back(apply_binary(&eval_context::less));
    }

    variable_type less_equal(const variable_type& var1, const variable_type& var2) const {
        return var1.which() == var2.which() && var1 <= var2;
    }

    virtual void less_equal() {
        assert(stack.size() >= 2);
        stack.push_back(apply_binary(&eval_context::less_equal));
    }

    variable_type greater(const variable_type& var1, const variable_type& var2) const {
        return var1.which() == var2.which() && var1 > var2;
    }

    virtual void greater() {
        assert(stack.size() >= 2);
        stack.push_back(apply_binary(&eval_context::greater));
    }

    variable_type greater_equal(const variable_type& var1, const variable_type& var2) const {
        return var1.which() == var2.which() && var1 >= var2;
    }

    virtual void greater_equal() {
        assert(stack.size() >= 2);
        stack.push_back(apply_binary(&eval_context::greater_equal));
    }

    variable_type logical_and(const variable_type& var1, const variable_type& var2) const {
        return get<bool>(var1) && get<bool>(var2);
    }

    virtual void logical_and() {
        assert(stack.size() >= 2);
        stack.push_back(apply_binary(&eval_context::logical_and));
    }

    variable_type logical_or(const variable_type& var1, const variable_type& var2) const {
        return get<bool>(var1) || get<bool>(var2);
    }

    virtual void logical_or() {
        assert(stack.size() >= 2);
        stack.push_back(apply_binary(&eval_context::logical_or));
    }

    void integer() {
        stack.push_back(
            little_endian_to_native<int64_t>(std::begin(code), std::next(std::begin(code), sizeof(int64_t))));
        code.drop_front(sizeof(int64_t));
    }

    void string() {
        auto str = boost::lexical_cast<std::string>(boost::find<boost::return_begin_found>(code, 0));
        code.drop_front(str.size() + 1);
        stack.emplace_back(std::move(str));
    }

    void bool_true() {
        stack.emplace_back(true);
    }

    void bool_false() {
        stack.emplace_back(false);
    }

    std::string pop_var_name() {
        auto name = pop_and_get(stack);
        if(is_which<std::string>(name))
            return get<std::string>(name);
        else if(is_which<int64_t>(name))
            return std::to_string(get<int64_t>(name));
        BOOST_THROW_EXCEPTION(jbson_path_error{} << expected_type{typeid(std::string)} << actual_type{name.type()});
    }

    virtual void load() {
        auto name = pop_var_name();
        if(name == "*")
            stack.push_back(load_wildcard(current));
        else if(name == "$")
            stack.push_back(root);
        else if(name == "@")
            stack.push_back(current);
        else {
            auto v = load_by_name(current, name);
            if(v)
                stack.push_back(*v);
            else
                BOOST_THROW_EXCEPTION(jbson_path_error{});
        }
    }

    virtual void access() {
        eval_context<ContainerT, EContainerT> access_context{root, pop_and_get(stack), code};

        stack.push_back(access_context.exec());
        code = access_context.code;
    }

    virtual void access_union() {
        constexpr auto visitor = union_visitor<EContainerT>{};
        auto a = pop_and_get(stack);
        stack.push_back(boost::apply_visitor(visitor, a));
        if(stack.size() > 2) {
            auto a = pop_and_get(stack);
            auto b = pop_and_get(stack);
            stack.push_back(boost::apply_visitor(visitor, b, a));
        }
    }

    virtual void expr() {
        eval_context<ContainerT, EContainerT> expr_context{root, current, code};
        stack.push_back(expr_context.exec());
        code = expr_context.code;

        load();
    }

    virtual void filter_expr();

    bool exec_one() {
        auto f = static_cast<byte_code>(code.front());
        code.drop_front();
        switch(f) {
            case byte_code::op_neg:
                unary_negate();
                break;
            case byte_code::op_pos:
                unary_plus();
                break;
            case byte_code::op_not:
                unary_not();
                break;
            case byte_code::op_add:
                add();
                break;
            case byte_code::op_sub:
                subtract();
                break;
            case byte_code::op_mul:
                multiply();
                break;
            case byte_code::op_div:
                divide();
                break;
            case byte_code::op_eq:
                equals();
                break;
            case byte_code::op_neq:
                not_equals();
                break;
            case byte_code::op_lt:
                less();
                break;
            case byte_code::op_lte:
                less_equal();
                break;
            case byte_code::op_gt:
                greater();
                break;
            case byte_code::op_gte:
                greater_equal();
                break;
            case byte_code::op_and:
                logical_and();
                break;
            case byte_code::op_or:
                logical_or();
                break;
            case byte_code::op_load:
                string();
                load();
                break;
            case byte_code::op_int:
                integer();
                break;
            case byte_code::op_string:
                string();
                break;
            case byte_code::op_true:
                bool_true();
                break;
            case byte_code::op_false:
                bool_false();
                break;
            case byte_code::op_object:
                throw 0;
            case byte_code::op_array:
                throw 0;
            case byte_code::op_recurse:
                throw 0;
            case byte_code::op_access:
                access();
                break;
            case byte_code::op_union:
                access_union();
                break;
            case byte_code::op_expr:
                expr();
                break;
            case byte_code::op_filter_expr:
                filter_expr();
                break;
            case byte_code::op_return:
                return false;
            case byte_code::op_slice:
                throw 0;
        }
        return true;
    }

    variable_type exec() {
        while(!code.empty() && exec_one())
            ;
        return !stack.empty() ? stack.back() : false;
    }
};

template <typename ContainerT, typename EContainerT> struct dummy_context : eval_context<ContainerT, EContainerT> {
    using eval_context<ContainerT, EContainerT>::eval_context;
    using eval_context<ContainerT, EContainerT>::stack;
    using eval_context<ContainerT, EContainerT>::root;
    using eval_context<ContainerT, EContainerT>::code;
    using typename eval_context<ContainerT, EContainerT>::variable_type;

    virtual void load() override {
    }

    virtual void access() override {
        dummy_context<ContainerT, EContainerT> access_context{root, variable_type{}, code};

        stack.push_back(access_context.exec());
        code = access_context.code;
    }

    virtual void unary_negate() override {
        stack.clear();
        stack.push_back(static_cast<int64_t>(1));
    }

    virtual void unary_plus() override {
        stack.clear();
        stack.push_back(static_cast<int64_t>(1));
    }

    virtual void unary_not() override {
        stack.clear();
        stack.push_back(false);
    }

    virtual void add() override {
        stack.clear();
        stack.push_back(static_cast<int64_t>(1));
    }

    virtual void subtract() override {
        stack.clear();
        stack.push_back(static_cast<int64_t>(1));
    }

    virtual void multiply() override {
        stack.clear();
        stack.push_back(static_cast<int64_t>(1));
    }

    virtual void divide() override {
        stack.clear();
        stack.push_back(static_cast<int64_t>(1));
    }

    virtual void equals() override {
        stack.clear();
        stack.push_back(false);
    }

    virtual void not_equals() override {
        stack.clear();
        stack.push_back(false);
    }

    virtual void less() override {
        stack.clear();
        stack.push_back(false);
    }

    virtual void less_equal() override {
        stack.clear();
        stack.push_back(false);
    }

    virtual void greater() override {
        stack.clear();
        stack.push_back(false);
    }

    virtual void greater_equal() override {
        stack.clear();
        stack.push_back(false);
    }

    virtual void logical_and() override {
        stack.clear();
        stack.push_back(false);
    }

    virtual void logical_or() override {
        stack.clear();
        stack.push_back(false);
    }
};

template <typename ContainerT, typename EContainerT> void eval_context<ContainerT, EContainerT>::filter_expr() {
    std::vector<basic_element<EContainerT>> filtered_elems;
    for(auto operand : ensure_range(pop_and_get(stack))) {
        eval_context<ContainerT, EContainerT> filter_context{root, variable_type{operand}, code};

        try {
            auto res = filter_context.exec();

            if(is_which<bool>(res) && get<bool>(res))
                filtered_elems.push_back(operand);
            else if(!is_which<bool>(res))
                filtered_elems.push_back(operand);
        } catch(...) {
        }
    }
    stack.emplace_back(std::move(filtered_elems));
    dummy_context<ContainerT, EContainerT> dummy{root, variable_type{}, code};
    dummy.exec();
    code = dummy.code;
}

template <typename ContainerT, typename EContainerT, typename CodeIterator>
variable_type<ContainerT, EContainerT> eval_expr(const basic_document<ContainerT, EContainerT>& root,
                                                 const variable_type<ContainerT, EContainerT>& current,
                                                 boost::iterator_range<CodeIterator>& code) {
//    std::clog << "[";
//    for(auto&& c : code) {
//        std::clog << static_cast<byte_code>(c) << ", ";
//    }
//    std::clog << "]" << std::endl;

    try {
        eval_context<ContainerT, EContainerT> context{root, current, code};

        return context.exec();
    }
    catch(...) {
        return std::vector<basic_element<EContainerT>>{};
    }
}

} // namespace filter_expression
} // namespace detail
} // namespace jbson

#endif // JBSON_EXPRESSION_EVALUATOR_HPP
