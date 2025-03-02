#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>


struct err_parse : std::runtime_error
{
    err_parse(const std::string &msg) :
        std::runtime_error("expression parsing error: " + msg) {}
};


struct err_eval : std::runtime_error
{
    err_eval(const std::string &msg) :
        std::runtime_error("expression evaluation error: " + msg) {}
};


struct params
{
    size_t count;
    const float *data;
};


struct expr
{
    explicit expr(const std::string &expr_string);
    float eval(const params &) const;
    void dump(std::ostream &os) const;
private:
    enum type {
        unknown,
        op,
        f,
        foo,
        var,
    } _type = unknown;
    std::string _text;
    std::vector<std::unique_ptr<expr>> _children;
    expr(type, const std::string &, std::vector<std::unique_ptr<expr>>);
    static std::unique_ptr<expr> make_f(
            const std::string &_text);
    static std::unique_ptr<expr> make_var(
            const std::string &_text);
    static std::unique_ptr<expr> make_foo(
            const std::string &_text, std::vector<std::unique_ptr<expr>> &&args);
    static std::unique_ptr<expr> make_op(
            const std::string &_text, std::unique_ptr<expr> &&left, std::unique_ptr<expr> &&right);
    static std::unique_ptr<expr> parse_expression(std::istream &is);
    static std::unique_ptr<expr> parse_term(std::istream &is);
    static std::unique_ptr<expr> parse_factor(std::istream &is);
};
