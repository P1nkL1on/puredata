#pragma once

#include <iostream>

#include "node.h"


struct graph
{
    virtual ~graph() = default;
    virtual size_t add_node(node *node) = 0;
    virtual void set_node(size_t node_idx, node *node) = 0;
    virtual void run_node(size_t node_idx) = 0;
    virtual void update_node(size_t node_idx) = 0;
    virtual int &i32_in(size_t node_idx, size_t node_input) = 0;
    virtual const int &i32_out(size_t node_idx, size_t node_output) const = 0;
    virtual std::vector<float> &fbuffer_in(size_t node_idx, size_t node_input) = 0;
    virtual const std::vector<float> &fbuffer_out(size_t node_idx, size_t node_output) const = 0;
    virtual std::string &str_in(size_t node_idx, size_t node_input) = 0;
    virtual void connect_nodes(
            size_t node_provider_idx,
            size_t node_provider_output,
            size_t node_reciever_idx,
            size_t node_reciever_input) = 0;
    virtual void dump_node_in_value(std::ostream &os, size_t node_idx, size_t input) const = 0;
    virtual void dump_graph(std::ostream &os, const bool compact = true) const = 0;
    virtual void read_dump(std::istream &is, const nodes_factory &node_idxs) = 0;
    virtual void move_node(size_t node_idx, int x, int y) = 0;
    virtual std::pair<int, int> node_xy(size_t node_idx) const = 0;
    virtual std::vector<size_t> node_idxs() const = 0;
};    
