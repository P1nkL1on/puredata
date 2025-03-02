#pragma once

#include "node.h"

struct summ_i32 : node
{
    enum {
        a, b,
    };
    enum {
        summ,
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


struct map_f : node
{
    enum {
        expr, buffer_in,
    };
    enum {
        buffer_out,
    };
    void init(node_init_ctx &ctx) override
    {
        ctx.set_name("map_f");
        ctx.add_in_str("x * 2");
        ctx.add_in_fbuffer();
        ctx.add_out_fbuffer();
    }
    void run(node_run_ctx &ctx) override
    {
        foo_f foo = ctx.parse_foo_f(ctx.str_in(expr), 1);
        const std::vector<float> &in = ctx.fbuffer_in(buffer_in);
        
        std::vector<float> &out = ctx.fbuffer_out(buffer_out);
        out.resize(in.size());

        ctx.run_foo(0, in.size(), [&in, &out, foo](size_t i) { 
            out[i] = foo(1, &in[i]); 
        });
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