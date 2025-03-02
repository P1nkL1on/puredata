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


struct ast
{
    enum {
        unknown,
        op,
        f,
        foo,
        var,
    } type = unknown;
    std::string text;
    std::vector<std::unique_ptr<ast>> _children;

    float eval(const params &) const;
    void dump(std::ostream &os) const;

    static std::unique_ptr<ast> make_f(
            const std::string &text);
    static std::unique_ptr<ast> make_var(
            const std::string &text);
    static std::unique_ptr<ast> make_foo(
            const std::string &text, std::vector<std::unique_ptr<ast>> &&args);
    static std::unique_ptr<ast> make_op(
            const std::string &text, std::unique_ptr<ast> &&left, std::unique_ptr<ast> &&right);
};


struct expr
{
    static expr parse(const std::string &);
    float eval(const params &) const;
private:
    static std::unique_ptr<ast> parse_expression(std::istream &is);
    static std::unique_ptr<ast> parse_term(std::istream &is);
    static std::unique_ptr<ast> parse_factor(std::istream &is);
    std::unique_ptr<ast> _ast;
};
