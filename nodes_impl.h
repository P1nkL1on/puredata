#pragma once

#include "node.h"

namespace {

struct summ_i32 : node
{
    enum {
        a, b,
    };
    enum {
        summ
    };
    void init(node_init_ctx &ctx) override
    {
        ctx.set_name("summ_i32");
        ctx.add_in_i32();
        ctx.add_in_i32();
        ctx.add_out_i32();
    }
    void run(node_run_ctx &ctx) override
    {
        ctx.i32_out(summ) = ctx.i32_in(a) + ctx.i32_in(b);
    }
};


struct nodes_factory_impl : nodes_factory
{
    node *create(const std::string &name) const override
    {
        if (name == "summ_i32") return new summ_i32;

        return nullptr;
    }
};

}