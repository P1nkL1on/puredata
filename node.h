#pragma once

#include <string>
#include <cstddef>
#include <vector>
#include <functional>


enum class data_type
{
    i32,
    str,
    buffer_f,

    _last,
    _first = i32,
};


constexpr std::array<const char *, static_cast<size_t>(data_type::_last)> data_type_titles = {
    "i32",
    "str",
    "buffer-f",
};



struct node_init_ctx
{
    virtual ~node_init_ctx() = default;
    virtual void set_name(const std::string &) = 0;

    virtual void add_in_i32(
            size_t id, int &&value = 0, const std::string &title = "") = 0;
    virtual void add_in_str(
            size_t id, std::string &&value = "", const std::string &title = "") = 0;
    virtual void add_in_fbuffer(
            size_t id, std::vector<float> &&value = {}, const std::string &title = "") = 0;

    virtual void add_out_i32(
            size_t id, const std::string &title = "") = 0;
    virtual void add_out_fbuffer(
            size_t id, const std::string &title = "") = 0;
};

using foo_i32 = std::function<int(size_t, const int *)>;
using foo_i64 = std::function<size_t(size_t, const size_t *)>;
using foo_f = std::function<float(size_t, const float *)>;
using foo_iter = std::function<void(size_t start, size_t length)>;


struct node_run_ctx
{
    virtual ~node_run_ctx() = default;

    virtual const int &i32_in(size_t id) const = 0;
    virtual int &i32_out(size_t id) = 0;

    virtual const std::string &str_in(size_t id) const = 0;

    virtual const std::vector<float> &fbuffer_in(size_t id) const = 0;
    virtual std::vector<float> &fbuffer_out(size_t id) = 0;

    virtual foo_f parse_foo_f(const std::string &str, size_t &foo_input_count) = 0;
    virtual void run_foo(const size_t start, const size_t length, const foo_iter &foo) = 0;

    virtual void warning(const std::string &msg) = 0;
    virtual void error(const std::string &msg) = 0;
    virtual void canvas_f(size_t w, size_t h, size_t size, const float *d) = 0;
};


struct node_update_ctx
{
    virtual ~node_update_ctx() = default;

    virtual void remove_unstable_outs() = 0;
    virtual void add_unstable_out_fbuffer(size_t id, const std::string &title = "") = 0;

    virtual const int &stable_in_i32(size_t id) const = 0;
};


struct node
{
    virtual ~node() = default;
    virtual void init(node_init_ctx &ctx) = 0; // aka signature
    virtual void run(node_run_ctx &ctx) = 0;
    virtual void update(node_update_ctx &) {} // aka change input/outputs based on inputs
};


struct nodes_factory
{
    virtual ~nodes_factory() = default;
    virtual node *create(const std::string &name) const = 0;
};
