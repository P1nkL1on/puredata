#pragma once

#include <iostream>

#include "node.h"


namespace {

struct graph
{
    virtual ~graph() = default;
    virtual size_t add_node(node *node) = 0;
    virtual void set_node(size_t node_idx, node *node) = 0;
    virtual void run_node(size_t node_idx) = 0;
    virtual int &i32_in(size_t node_idx, size_t node_input) = 0;
    virtual int i32_out(size_t node_idx, size_t node_output) const = 0;
    virtual void connect_nodes(
        size_t node_provider_idx,
        size_t node_provider_output,
        size_t node_reciever_idx,
        size_t node_reciever_input) = 0;
    virtual void dump(std::ostream &os) const = 0;
    virtual void read_dump(std::istream &is, const nodes_factory &nodes) = 0;
};    

}