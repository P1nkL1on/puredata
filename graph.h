#pragma once

#include <iostream>

#include "node.h"


struct graph
{
    virtual ~graph() = default;
    virtual size_t add_node(node *node) = 0;
    virtual void set_node(size_t node_idx, node *node) = 0;
    virtual void run_node(size_t node_idx) = 0;
    virtual int &i32_in(size_t node_idx, size_t node_input) = 0;
    virtual const int &i32_out(size_t node_idx, size_t node_output) const = 0;
    virtual std::vector<float> &fbuffer_in(size_t node_idx, size_t node_input) = 0;
    virtual const std::vector<float> &fbuffer_out(size_t node_idx, size_t node_output) const = 0;
    virtual void connect_nodes(
            data_type type,
            size_t node_provider_idx,
            size_t node_provider_output,
            size_t node_reciever_idx,
            size_t node_reciever_input) = 0;
    virtual void dump_node_in(std::ostream &os, size_t node_idx, data_type, size_t input) const = 0;
    virtual void dump_graph(std::ostream &os) const = 0;
    virtual void read_dump(std::istream &is, const nodes_factory &nodes) = 0;
};    
