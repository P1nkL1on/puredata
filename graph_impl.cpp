#include "graph_impl.h"
#include "exceptions.h"

graph_impl::node_inout_spec::node_inout_spec(graph_impl &g, node *node, int node_idx) : _g(&g), _node_idx(node_idx)
{
    _node.reset(node);
    if (_node)
        _node->init(*this);
}

graph_impl::node_inout_spec::node_inout_spec(node_inout_spec &&other)
{
    *this = std::move(other);
}

graph_impl::node_inout_spec &graph_impl::node_inout_spec::operator=(node_inout_spec &&other)
{
    _g = other._g;
    _node.reset(other._node.release());
    _node_idx = other._node_idx;
    _name = std::move(other._name);
    _default_ins_i32 = std::move(other._default_ins_i32);
    _ins_i32 = std::move(other._ins_i32);
    _outs_i32 = std::move(other._outs_i32);
    return *this;
}

void graph_impl::node_inout_spec::run()
{
    _node->run(*this);
}

void graph_impl::node_inout_spec::set_name(const std::string &name)
{
    EXPECT(!name.empty());
    _name = name;
}

void graph_impl::node_inout_spec::add_in_i32(int value_default)
{
    _g->_bus_i32[_g->_bus_i32_next_free_cell] = value_default;
    _default_ins_i32.push_back(_g->_bus_i32_next_free_cell);
    _ins_i32.push_back(_g->_bus_i32_next_free_cell++);
}

void graph_impl::node_inout_spec::add_out_i32()
{
    _g->_bus_i32_spec.emplace(_g->_bus_i32_next_free_cell, bus_cell_spec{ _node_idx, _outs_i32.size() });
    _outs_i32.push_back(_g->_bus_i32_next_free_cell++);
}

void graph_impl::node_inout_spec::add_in_str(const std::string &str) 
{

}

void graph_impl::node_inout_spec::add_in_fbuffer(size_t size, std::vector<float> &&init)
{

}

void graph_impl::node_inout_spec::add_out_fbuffer() 
{

}

std::vector<float> &graph_impl::node_inout_spec::fbuffer_out(size_t idx) 
{

}

const std::string &graph_impl::node_inout_spec::str_in(size_t idx) const 
{

}

const std::vector<float> &graph_impl::node_inout_spec::fbuffer_in(size_t idx) const 
{

}

foo_f graph_impl::node_inout_spec::parse_foo_f(const std::string &str, const size_t ins_count) 
{
    throw bad_io("can't parse a foo");
}

void graph_impl::node_inout_spec::run_foo(const size_t start, const size_t end, const foo_iter &foo) 
{
    // TODO: add parallel stuff
    for (size_t idx = start; idx != end; ++idx)
        foo(idx);
}

int graph_impl::node_inout_spec::i32_in(size_t idx) const
{
    return _g->_bus_i32[_ins_i32[idx]];
}

int &graph_impl::node_inout_spec::i32_out(size_t idx)
{
    return _g->_bus_i32[_outs_i32[idx]];
}

size_t graph_impl::node_inout_spec::i32_out_bus_idx(size_t idx) const 
{
    return _outs_i32[idx]; 
}

size_t graph_impl::node_inout_spec::i32_in_bus_idx(size_t idx) const 
{
    return _ins_i32[idx]; 
}

size_t graph_impl::node_inout_spec::i32_default_in_bus_idx(size_t idx) const 
{
    return _default_ins_i32[idx]; 
}

void graph_impl::node_inout_spec::set_i32_in_bus_idx(size_t idx, size_t bus_idx) 
{
    _ins_i32[idx] = bus_idx; 
}

bool graph_impl::node_inout_spec::was_removed() const
{
    return _node == nullptr;
}

const std::string &graph_impl::node_inout_spec::name() const
{
    return _name;
}

size_t graph_impl::node_inout_spec::ins_i32_count() const
{
    return _ins_i32.size();
}

size_t graph_impl::add_node(node *n)
{
    size_t node_idx = _nodes.size();
    set_node(node_idx, n);
    return node_idx;
}

void graph_impl::set_node(size_t node_idx, node *n)
{
    if (_nodes.size() < node_idx + 1)
        _nodes.resize(node_idx + 1);
    _nodes[node_idx] = node_inout_spec(*this, n, node_idx);
}

void graph_impl::run_node(size_t node_idx)
{
    _nodes[node_idx].run();
}

void graph_impl::connect_nodes(
    size_t node_provider_idx,
    size_t node_provider_output,
    size_t node_reciever_idx,
    size_t node_reciever_input)
{
    // TODO: check index valid and type OK
    // TODO: add connection type dispatch
    _nodes[node_reciever_idx].set_i32_in_bus_idx(
        node_reciever_input, _nodes[node_provider_idx].i32_out_bus_idx(node_provider_output));
}

int &graph_impl::i32_in(size_t node_idx, size_t node_input)
{
    return _bus_i32[_nodes.at(node_idx).i32_in_bus_idx(node_input)];
}

int graph_impl::i32_out(size_t node_idx, size_t node_output) const
{
    return _bus_i32[_nodes.at(node_idx).i32_out_bus_idx(node_output)];
}

void graph_impl::dump(std::ostream &os) const
{
    os << "version 1\n";

    const size_t nodes_count = _nodes.size();
    os << "nodes " << nodes_count << '\n';

    const auto dump_out_i32 = [&os, &specs = _bus_i32_spec](size_t idx) {
        const bus_cell_spec &spec = specs.at(idx);
        os << "out ";
        os << spec.node_idx << ' ' << spec.node_output_idx;
    };
    for (size_t node_idx = 0; node_idx < nodes_count; ++node_idx) {
        const auto &node = _nodes[node_idx];
        if (node.was_removed()) continue;
        os << node_idx << ' ' << node.name();

        const size_t ins_i32_count = node.ins_i32_count();
        if (ins_i32_count)
            os << " i32";
        for (size_t in_idx = 0; in_idx < ins_i32_count; ++in_idx) {
            os << ' ';
            const size_t bus_idx = node.i32_in_bus_idx(in_idx);
            const size_t default_bus_idx = node.i32_default_in_bus_idx(in_idx);
            if (bus_idx == default_bus_idx)
                os << _bus_i32.at(bus_idx);
            else
                dump_out_i32(bus_idx);
        }

        os << '\n';
    }
}

void graph_impl::read_dump(std::istream &is, const nodes_factory &nodes)
{
    const auto peek_str = [&is] {
        const auto pos = is.tellg();
        is >> std::ws;
        std::string str;
        is >> str;
        is.seekg(pos);
        return str;
    };
    const auto read_str = [&is](const std::string &what = "value") {
        is >> std::ws;
        std::string str;
        if (is >> str) return str;
        throw bad_io("unexpectedly can't read " + what + " (str)");
    };
    const auto read_i32 = [&is](const std::string &what = "value") {
        is >> std::ws;
        int i32;
        if (is >> i32) return i32;
        is.clear();
        std::string s; is >> s;
        throw bad_io("expected a " + what + " (i32), but get '" + s + "'");
    };
    const auto read_ui64 = [&is](const std::string &what = "value") {
        is >> std::ws;
        int ui64;
        if (is >> ui64) return ui64; 
        is.clear();
        std::string s; is >> s;
        throw bad_io("expected a " + what + " (ui64), but get '" + s + "'");
    };
    std::vector<std::tuple<size_t, size_t, size_t, size_t>> connections;
    const auto try_read_connected = [&](size_t node_idx, size_t in_idx) {
        if (read_str() != "out") throw bad_io("expected keyword 'out'");
        const size_t provider_idx = read_ui64("provider node idx");
        const size_t provider_output_idx = read_ui64("provider node output idx");
        connections.emplace_back(provider_idx, provider_output_idx, node_idx, in_idx);
    };
    graph_impl g;
    if (read_str("version") != "version")
        throw bad_io("expected a 'version' tag");
    const size_t version = read_ui64("version number");
    if (version != 1)
        throw bad_io("can't read projects with version other than 1. get " + std::to_string(version));
    if (read_str("tag") != "nodes")
        throw bad_io("expected a 'nodes' tag");
    const size_t nodes_count = read_ui64("nodes count");
    for (size_t i = 0; i < nodes_count; ++i) {
        const size_t node_idx = read_ui64("node idx");
        const std::string node_name = read_str("node name");
        node *n = nodes.create(node_name);
        if (n == nullptr)
            throw bad_io("unknown node name " + node_name);
        g.set_node(node_idx, n);

        const size_t ins_i32_count = g._nodes[node_idx].ins_i32_count();
        if (ins_i32_count) {
            const std::string token = read_str();
            if (token != "i32") throw bad_io("expected a 'i32' (str)");
            for (size_t i = 0; i < ins_i32_count; ++i) {
                if (peek_str() == "out") {
                    try_read_connected(node_idx, i);
                    continue;
                }
                g.i32_in(node_idx, i) = read_i32();
            }
        }
    }
    for (const auto &[pidx, poidx, ridx, riidx] : connections)
        g.connect_nodes(pidx, poidx, ridx, riidx);
    *this = std::move(g);
}