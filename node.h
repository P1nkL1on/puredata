#pragma once

#include <string>
#include <cstddef>
#include <vector>
#include <functional>


struct node_init_ctx
{
    virtual ~node_init_ctx() = default;
    virtual void add_in_i32(int &&value_default = 0) = 0;
    virtual void add_out_i32() = 0;
    virtual void add_in_str(std::string &&) = 0;
    virtual void add_in_fbuffer(std::vector<float> &&init = {}) = 0;
    virtual void add_out_fbuffer() = 0;
    virtual void set_name(const std::string &) = 0;
};


enum class data_type
{
    i32,
    str,
    fbuffer,
};


using foo_i32 = std::function<int(size_t, const int *)>;
using foo_i64 = std::function<size_t(size_t, const size_t *)>;
using foo_f = std::function<float(size_t, const float *)>;
using foo_iter = std::function<void(size_t)>;


struct node_run_ctx
{
    virtual ~node_run_ctx() = default;
    virtual const int &i32_in(size_t idx) const = 0;
    virtual int &i32_out(size_t idx) = 0;
    virtual const std::string &str_in(size_t idx) const = 0;
    virtual const std::vector<float> &fbuffer_in(size_t idx) const = 0;
    virtual std::vector<float> &fbuffer_out(size_t idx) = 0;
    virtual foo_f parse_foo_f(const std::string &str, const size_t ins_count = 1) = 0;
    virtual void run_foo(const size_t start, const size_t end, const foo_iter &foo) = 0;
};


struct node
{
    virtual ~node() = default;
    virtual void init(node_init_ctx &ctx) = 0; // aka signature
    virtual void run(node_run_ctx &ctx) = 0;
};


struct nodes_factory
{
    virtual ~nodes_factory() = default;
    virtual node *create(const std::string &name) const = 0;
};
