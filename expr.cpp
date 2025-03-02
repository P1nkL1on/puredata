#include "expr.h"

#include <iostream>
#include <sstream>


expr expr::parse(const std::string &str)
{
    std::stringstream ss(str);
    std::istream &is = ss;
    std::unique_ptr<ast> ast = parse_expression(is);
    std::cout << "expr: ";
    ast->dump(std::cout);
    std::cout << '\n';
    //    return parse_expression(is);
    return expr{};
}

std::unique_ptr<ast> expr::parse_expression(std::istream &is)
{
    auto node = parse_term(is);
    while (true) {
        char op;
        if (!(is >> op))
            break;
        if (op != '+' && op != '-') {
            is.putback(op);
            break;
        }
        auto right = parse_term(is);
        node = ast::make_op(std::string(1, op), std::move(node), std::move(right));
    }
    return node;
}

std::unique_ptr<ast> expr::parse_term(std::istream &is)
{
    auto node = parse_factor(is);
    while (true) {
        char op;
        if (!(is >> op))
            break;
        if (op != '*' && op != '/') {
            is.putback(op);
            break;
        }
        auto right = parse_term(is);
        node = ast::make_op(std::string(1, op), std::move(node), std::move(right));
    }
    return node;
}

std::unique_ptr<ast> expr::parse_factor(std::istream &is)
{
    char ch;
    if (!(is >> ch))
        throw err_parse("unexpected eof");

    if (ch == '(') {
        auto node = parse_expression(is);
        if (!(is >> ch) || ch != ')')
            throw err_parse("expected ')'");
        return node;
    }

    if (std::isalpha(ch)) {
        std::string name(1, ch);
        while (is.get(ch) && std::isalnum(ch))
            name += ch;
        if (ch == '(') {
            std::vector<std::unique_ptr<ast>> args;
            if (is.peek() != ')') {
                while (true) {
                    args.emplace_back(parse_expression(is));
                    if (!(is >> ch) || ch == ')')
                        break;
                    if (ch != ',')
                        throw err_parse("Expected ',' or ')'");
                }
            } else {
                is.get(ch);
            }
            return ast::make_foo(name, std::move(args));
        }
        is.putback(ch);
        return ast::make_var(name);
    }

    if (std::isdigit(ch) || ch == '.') {
        std::string number(1, ch);
        while (is.get(ch) && (std::isdigit(ch) || ch == '.'))
            number += ch;
        is.putback(ch);
        return ast::make_f(number);
    }
    throw err_parse(std::string("unexpected char '") + ch + "'");
}

float ast::eval(const params &in) const
{
    if (text == "+")
        return _children.at(0)->eval(in) + _children.at(1)->eval(in);
    if (text == "-")
        return _children.at(0)->eval(in) - _children.at(1)->eval(in);
    if (text == "/")
        return _children.at(0)->eval(in) / _children.at(1)->eval(in);
    if (text == "*")
        return _children.at(0)->eval(in) * _children.at(1)->eval(in);

//    if (text.size() == 1 && std::isalpha(text[0])) {
//        char idx;
//        if (std::islower(text[0]))
//            idx = text[0] - 'a';
//        else if (std::isupper(text[0]))
//            idx = text[0] - 'A' + 27;
//        else
//            throw err_eval("unexpected variable name");
//        const auto var_idx = static_cast<size_t>(idx);
//        if (var_idx >= in.count)
//            throw -1;
//        return in.data[var_idx];
//    }
//    return std::stof(text);
    return 0;
}

void ast::dump(std::ostream &os) const
{
    switch (type) {
        case op:
            os << '(';
            _children.at(0)->dump(os);
            os << ' ';
            os << text;
            os << ' ';
            _children.at(1)->dump(os);
            os << ')';
            return;
        case f:
            os << text;
            return;
        case foo: {
            os << text;
            os << '(';
            bool need_comma = false;
            for (const auto &child : _children) {
                if (need_comma) os << ", ";
                child->dump(os);
                need_comma = true;
            }
            os << ')';
            return;
        }
        case var:
            os << '$' << text;
            return;
        default:
            throw err_eval("unknown ast node type");
    }
}

std::unique_ptr<ast> ast::make_f(const std::string &text)
{
    return std::unique_ptr<ast>(
                new ast{ f, text, {} });
}

std::unique_ptr<ast> ast::make_var(const std::string &text)
{
    return std::unique_ptr<ast>(
                new ast{ var, text, {} });
}

std::unique_ptr<ast> ast::make_foo(
        const std::string &text, std::vector<std::unique_ptr<ast> > &&args)
{
    return std::unique_ptr<ast>(
                new ast{ foo, text, std::move(args) });
}

std::unique_ptr<ast> ast::make_op(
        const std::string &text, std::unique_ptr<ast> &&left, std::unique_ptr<ast> &&right)
{
    std::vector<std::unique_ptr<ast>> children;
    children.emplace_back(std::move(left));
    children.emplace_back(std::move(right));
    return std::unique_ptr<ast>(
                new ast{ op, text, std::move(children) });
}
