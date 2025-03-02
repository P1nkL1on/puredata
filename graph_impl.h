#pragma once

#include <unordered_map>
#include <memory>

#include "exceptions.h"
#include "graph.h"


struct graph_impl : graph
{
    struct node_inout_spec : node_init_ctx, node_run_ctx
    {
        node_inout_spec() = default;
        node_inout_spec(graph_impl &g, node *node, int node_idx);
        node_inout_spec(node_inout_spec &&other);
        node_inout_spec &operator=(node_inout_spec &&other);
        void run();
        // node_init_ctx
        void set_name(const std::string &name) override;
        void add_in_i32(int value_default = 0) override;
        void add_out_i32() override;
        void add_in_str(const std::string &) override;
        void add_in_fbuffer(size_t size = 0, std::vector<float> &&init = {}) override;
        void add_out_fbuffer() override;
        // not_run_ctx
        int i32_in(size_t idx) const override;
        int &i32_out(size_t idx) override;
        const std::string &str_in(size_t idx) const override;
        const std::vector<float> &fbuffer_in(size_t idx) const override;
        std::vector<float> &fbuffer_out(size_t idx) override;
        foo_f parse_foo_f(const std::string &str, const size_t ins_count = 1) override;
        void run_foo(const size_t start, const size_t end, const foo_iter &foo) override;
        // graph_impl
        size_t i32_out_bus_idx(size_t idx) const;
        size_t i32_in_bus_idx(size_t idx) const;
        size_t i32_default_in_bus_idx(size_t idx) const;
        void set_i32_in_bus_idx(size_t idx, size_t bus_idx);
        bool was_removed() const;
        const std::string &name() const;
        size_t ins_i32_count() const;
    private:
        graph_impl *_g = nullptr;
        std::unique_ptr<node> _node;
        size_t _node_idx;
        std::string _name;
        std::vector<size_t> _default_ins_i32;
        std::vector<size_t> _ins_i32; // may change after init to declare input connections
        std::vector<size_t> _outs_i32;
    };

    struct bus_cell_spec
    {
        size_t node_idx;
        size_t node_output_idx;
    };

    std::vector<node_inout_spec> _nodes;

    std::vector<int> _bus_i32;
    std::unordered_map<size_t, bus_cell_spec> _bus_i32_spec;
    size_t _bus_i32_next_free_cell = 0;
    
    std::vector<std::string> _bus_str;
    std::unordered_map<size_t, bus_cell_spec> _bus_str_spec;
    size_t _bus_str_next_free_cell = 0;

    explicit graph_impl()
    {
        _bus_i32.resize(2048);
        _bus_str.resize(2048);
    }
    size_t add_node(node *n) override;
    void set_node(size_t node_idx, node *n) override;
    void run_node(size_t node_idx) override;
    void connect_nodes(
        size_t node_provider_idx,
        size_t node_provider_output,
        size_t node_reciever_idx,
        size_t node_reciever_input) override;
    int &i32_in(size_t node_idx, size_t node_input) override;
    int i32_out(size_t node_idx, size_t node_output) const override;
    void dump(std::ostream &os) const override;
    void read_dump(std::istream &is, const nodes_factory &nodes) override;
};

