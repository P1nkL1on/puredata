#pragma once

#include "node.h"


struct summ_i32 : node
{
    enum { a, b, };
    enum { summ, };

    void init(node_init_ctx &ctx) override;
    void run(node_run_ctx &ctx) override;
};


struct map_f : node
{
    enum { expr, buffer_in, };
    enum { buffer_out, };

    void init(node_init_ctx &ctx) override;
    void run(node_run_ctx &ctx) override;
};


struct canvas_f : node
{
    enum { width, height, buffer_in, };
    enum { buffer_out, };

    void init(node_init_ctx &ctx) override;
    void run(node_run_ctx &ctx) override;
};


struct readimg_f : node
{
    enum { filepath, };
    enum { width, height, channels, buffer, };

    void init(node_init_ctx &ctx) override;
    void run(node_run_ctx &ctx) override;
};


struct writeimg_f : node
{
    enum { filepath, width, height, channels, buffer, };

    void init(node_init_ctx &ctx) override;
    void run(node_run_ctx &ctx) override;
};


struct splitbuffer_f : node
{
    enum { buffer_in, channels, };
    enum { buffer_out_first, };

    void init(node_init_ctx &ctx) override;
    void run(node_run_ctx &ctx) override;
    void update(node_update_ctx &ctx) override;
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
