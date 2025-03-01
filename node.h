#pragma once

#include <string>
#include <cstddef>


namespace {

struct node_init_ctx
{
    virtual ~node_init_ctx() = default;
    virtual void add_in_i32(int value_default = 0) = 0;
    virtual void add_out_i32() = 0;
    virtual void set_name(const std::string &) = 0;
};


struct node_run_ctx
{
    virtual ~node_run_ctx() = default;
    virtual int i32_in(size_t idx) const = 0;
    virtual int &i32_out(size_t idx) = 0;
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

}