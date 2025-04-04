#include "expr.h"

#include <sstream>


expr::expr(const std::string &expr_str)
{
    std::stringstream ss(expr_str);
    std::istream &is = ss;
    *this = std::move(*parse_expression(is).release());
}

std::unique_ptr<expr> expr::parse_expression(std::istream &is)
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
        node = expr::make_op(std::string(1, op), std::move(node), std::move(right));
    }
    return node;
}

std::unique_ptr<expr> expr::parse_term(std::istream &is)
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
        node = expr::make_op(std::string(1, op), std::move(node), std::move(right));
    }
    return node;
}

std::unique_ptr<expr> expr::parse_factor(std::istream &is)
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
            std::vector<std::unique_ptr<expr>> args;
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
            return expr::make_foo(name, std::move(args));
        }
        is.putback(ch);
        return expr::make_var(name);
    }

    if (std::isdigit(ch) || ch == '.') {
        std::string number(1, ch);
        while (is.get(ch) && (std::isdigit(ch) || ch == '.'))
            number += ch;
        is.putback(ch);
        return expr::make_f(number);
    }
    throw err_parse(std::string("unexpected char '") + ch + "'");
}

float expr::eval(const params &in) const
{
    switch (_type) {
        case op:
            if (_text == "+")
                return _children.at(0)->eval(in) + _children.at(1)->eval(in);
            if (_text == "-")
                return _children.at(0)->eval(in) - _children.at(1)->eval(in);
            if (_text == "/")
                return _children.at(0)->eval(in) / _children.at(1)->eval(in);
            if (_text == "*")
                return _children.at(0)->eval(in) * _children.at(1)->eval(in);
            throw err_eval("unknown op type");
        case f:
            return std::stof(_text);
        case foo:
            throw err_eval("yet can't eval functions");
        case var: {
            char idx;
            if (std::islower(_text[0]))
                idx = _text[0] - 'a';
            else if (std::isupper(_text[0]))
                idx = _text[0] - 'A' + 27;
            else
                throw err_eval("unexpected variable name");
            const auto var_idx = static_cast<size_t>(idx);
            if (var_idx >= in.count)
                throw err_eval("unexpected variable index");
            return in.data[var_idx];
        }
        default:
            throw err_eval("unknown ast node type");
    }
}

void expr::dump(std::ostream &os) const
{
    switch (_type) {
        case op:
            os << '(';
            _children.at(0)->dump(os);
            os << ' ';
            os << _text;
            os << ' ';
            _children.at(1)->dump(os);
            os << ')';
            return;
        case f:
            os << _text;
            return;
        case foo: {
            os << _text;
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
            os << '$' << _text;
            return;
        default:
            throw err_eval("unknown ast node type");
    }
}

expr::expr(type t, const std::string &s, std::vector<std::unique_ptr<expr>> args) :
    _type(t), _text(s), _children(std::move(args)) {}

std::unique_ptr<expr> expr::make_f(const std::string &text)
{
    return std::unique_ptr<expr>(
                new expr{ f, text, {} });
}

std::unique_ptr<expr> expr::make_var(const std::string &text)
{
    return std::unique_ptr<expr>(
                new expr{ var, text, {} });
}

std::unique_ptr<expr> expr::make_foo(
        const std::string &text, std::vector<std::unique_ptr<expr> > &&args)
{
    return std::unique_ptr<expr>(
                new expr{ foo, text, std::move(args) });
}

std::unique_ptr<expr> expr::make_op(
        const std::string &text, std::unique_ptr<expr> &&left, std::unique_ptr<expr> &&right)
{
    std::vector<std::unique_ptr<expr>> children;
    children.emplace_back(std::move(left));
    children.emplace_back(std::move(right));
    return std::unique_ptr<expr>(
                new expr{ op, text, std::move(children) });
}
