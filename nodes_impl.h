#pragma once

#include "node.h"


struct summ_i32 : node
{
    enum { a, b, };
    enum { summ, };

    void init(node_init_ctx &ctx) override
    {
        ctx.set_name("summ-i32");
        ctx.add_in_i32(a, 0, "a");
        ctx.add_in_i32(b, 0, "b");
        ctx.add_out_i32(summ, "a+b");
    }
    void run(node_run_ctx &ctx) override
    {
        ctx.i32_out(summ) = ctx.i32_in(a) + ctx.i32_in(b);
    }
};


struct map_f : node
{
    enum { expr, buffer_in, };
    enum { buffer_out, };

    void init(node_init_ctx &ctx) override
    {
        ctx.set_name("map-f");
        ctx.add_in_str(expr, "a * 1 + 0");
        ctx.add_in_fbuffer(buffer_in);
        ctx.add_out_fbuffer(buffer_out);
    }
    void run(node_run_ctx &ctx) override
    {
        const std::vector<float> &in = ctx.fbuffer_in(buffer_in);
        
        std::vector<float> &out = ctx.fbuffer_out(buffer_out);

        size_t foo_input_count;
        foo_f foo = ctx.parse_foo_f(ctx.str_in(expr), foo_input_count);

        out.resize(in.size());
        ctx.run_foo(0, in.size(), [&in, &out, foo](size_t i) { 
            out[i] = foo(1, &in[i]);
        });
    }
};


struct canvas_f : node
{
    enum { width, height, buffer_in, };
    enum { buffer_out, };

    void init(node_init_ctx &ctx) override
    {
        ctx.set_name("canvas-f");
        ctx.add_in_i32(width, 128);
        ctx.add_in_i32(height, 128);
        ctx.add_in_fbuffer(buffer_in);
        ctx.add_out_fbuffer(buffer_out);
    }
    void run(node_run_ctx &ctx) override
    {
        const std::vector<float> &in = ctx.fbuffer_in(buffer_in);
        const int w = ctx.i32_in(width);
        const int h = ctx.i32_in(height);
        if (w < 0 || h < 0)
            return ctx.error("W & H can't be negative");
        const int wh = static_cast<int>(in.size());
        if (w * h < wh)
            ctx.warning("buffer size can't cover W x H canvas");
        if (w * h > wh)
            ctx.warning("W x H canvas can't cover buffer size");
        ctx.canvas_f(
                    static_cast<size_t>(w),
                    static_cast<size_t>(h),
                    in.size(), in.data());
        ctx.fbuffer_out(buffer_out) = in;
    }
};


struct readimg_f : node
{
    enum { filepath, };
    enum { width, height, channels, buffer, };

    void init(node_init_ctx &ctx) override
    {
        ctx.set_name("readimg-f");
        ctx.add_in_str(filepath);
        ctx.add_out_i32(width);
        ctx.add_out_i32(height);
        ctx.add_out_i32(channels);
        ctx.add_out_fbuffer(buffer);
    }
    void run(node_run_ctx &ctx) override;
};


struct writeimg_f : node
{
    enum { filepath, width, height, channels, buffer, };

    void init(node_init_ctx &ctx) override
    {
        ctx.set_name("readimg-f");
        ctx.add_in_str(filepath);
        ctx.add_in_i32(width);
        ctx.add_in_i32(height);
        ctx.add_in_i32(channels);
        ctx.add_in_fbuffer(buffer);
    }
    void run(node_run_ctx &ctx) override;
};


struct nodes_factory_impl : nodes_factory
{
    node *create(const std::string &name) const override
    {
        if (name == "summ-i32") return new summ_i32;
        if (name == "map-f") return new map_f;
        if (name == "canvas-f") return new canvas_f;
        if (name == "readimg-f") return new readimg_f;

        return nullptr;
    }
};
