#pragma once

#include <unordered_map>
#include <memory>

#include "exceptions.h"
#include "graph.h"


template <data_type T> struct bus_type { using _type = void; };
template<> struct bus_type<data_type::i32> { using _type = int; };
template<> struct bus_type<data_type::str> { using _type = std::string; };
template<> struct bus_type<data_type::fbuffer> { using _type = std::vector<float>; };
template <data_type T> using bus_underlying_type = typename bus_type<T>::_type;
template <data_type T> using bus_underlying_vector_type = std::vector<bus_underlying_type<T>>;


struct graph_impl : graph
{

    struct node_inout_spec : node_init_ctx, node_run_ctx
    {
        node_inout_spec() = default;
        node_inout_spec(graph_impl &g, node *node, size_t node_idx);
        node_inout_spec(node_inout_spec &&other);
        node_inout_spec &operator=(node_inout_spec &&other);
        void run();
        bool was_removed() const { return _node == nullptr; }

        // node_init_ctx
        void set_name(const std::string &name) override;
        const std::string &name() const { return _name; }

        void add_in_i32(int &&value = 0) override {
            return add_in_X<data_type::i32>(std::move(value)); }
        void add_out_i32() override {
            return add_out_X<data_type::i32>(); }

        void add_in_fbuffer(std::vector<float> &&value = {}) override {
            return add_in_X<data_type::fbuffer>(std::move(value)); }
        void add_out_fbuffer() override {
            return add_out_X<data_type::fbuffer>(); }

        void add_in_str(std::string &&value) override {
            return add_in_X<data_type::str>(std::move(value)); }

        // not_run_ctx
        foo_f parse_foo_f(const std::string &str, const size_t ins_count = 1) override;
        void run_foo(const size_t start, const size_t end, const foo_iter &foo) override;

        const int &i32_in(size_t idx) const override {
            return in_X<data_type::i32>(idx); }
        int &i32_out(size_t idx) override {
            return out_X<data_type::i32>(idx); }

        const std::vector<float> &fbuffer_in(size_t idx) const override {
            return in_X<data_type::fbuffer>(idx); }
        std::vector<float> &fbuffer_out(size_t idx) override {
            return out_X<data_type::fbuffer>(idx); }

        const std::string &str_in(size_t idx) const override {
            return in_X<data_type::str>(idx); }

        // graph_impl
        size_t in_bus_idx(data_type type, size_t idx) const {
            return _bus.at(type)._ins.at(idx); }
        size_t out_bus_idx(data_type type, size_t idx) const {
            return _bus.at(type)._outs.at(idx); }
        size_t default_in_bus_idx(data_type type, size_t idx) const {
            return _bus.at(type)._default_ins.at(idx); }
        size_t ins_count(data_type type) const {
            return _bus.at(type)._ins.size(); }
        void set_in_bus_idx(data_type type, size_t idx, size_t bus_idx) {
            _bus[type]._ins[idx] = bus_idx; }

    private:
        graph_impl *_g = nullptr;
        std::unique_ptr<node> _node;
        size_t _node_idx;
        std::string _name;
        struct bus
        {
            std::vector<size_t> _default_ins;
            std::vector<size_t> _ins; // may change after init to declare input connections
            std::vector<size_t> _outs;
        };
        std::unordered_map<data_type, bus> _bus = {
            { data_type::i32, bus{} },
            { data_type::str, bus{} },
            { data_type::fbuffer, bus{} },
        };
        template <data_type T, typename X> void add_in_X(X &&x);
        template <data_type T> void add_out_X();
        template <data_type T> const bus_underlying_type<T> &in_X(size_t idx) const;
        template <data_type T> bus_underlying_type<T> &out_X(size_t idx);
    };

    struct bus_cell_spec
    {
        size_t node_idx;
        size_t node_output_idx;
    };
    struct bus
    {
        std::unordered_map<size_t, bus_cell_spec> _bus_spec;
        size_t _bus_next_free_cell = 0;
    };

    std::vector<node_inout_spec> _nodes;
    std::vector<int> _bus_i32;
    std::vector<std::vector<float>> _bus_fbuffer;
    std::vector<std::string> _bus_str;
    std::unordered_map<data_type, bus> _bus = {
        { data_type::i32, bus{} },
        { data_type::str, bus{} },
        { data_type::fbuffer, bus{} },
    };
    template <data_type T> bus_underlying_vector_type<T> &bus_X_ref();
    template <data_type T> const bus_underlying_vector_type<T> &bus_X_cref() const;
    template <data_type T> bus_underlying_type<T> &in_X(size_t idx, size_t node_input);
    template <data_type T> const bus_underlying_type<T> &out_X(size_t idx, size_t node_output) const;

    explicit graph_impl()
    {
        constexpr size_t buffer_size = 10;

        _bus_i32.resize(buffer_size);
        _bus_fbuffer.resize(buffer_size);
        _bus_str.resize(buffer_size);
    }
    size_t add_node(node *n) override;
    void set_node(size_t node_idx, node *n) override;
    void run_node(size_t node_idx) override;
    void connect_nodes(
            data_type type,
            size_t node_provider_idx,
            size_t node_provider_output,
            size_t node_reciever_idx,
            size_t node_reciever_input) override;

    int &i32_in(size_t node_idx, size_t node_input) override {
        return in_X<data_type::i32>(node_idx, node_input); }
    const int &i32_out(size_t node_idx, size_t node_output) const override {
        return out_X<data_type::i32>(node_idx, node_output); }

    std::vector<float> &fbuffer_in(size_t node_idx, size_t node_input) override {
        return in_X<data_type::fbuffer>(node_idx, node_input); }
    const std::vector<float> &fbuffer_out(size_t node_idx, size_t node_output) const override {
        return out_X<data_type::fbuffer>(node_idx, node_output); }

    void dump(std::ostream &os) const override;
    void read_dump(std::istream &is, const nodes_factory &nodes) override;
};









// impl


template <data_type T> bus_underlying_vector_type<T> &graph_impl::bus_X_ref()
{
    if constexpr (T == data_type::i32) { return _bus_i32;
    } else if constexpr (T == data_type::str) { return _bus_str;
    } else if constexpr (T == data_type::fbuffer) { return _bus_fbuffer;
    }
}


template <data_type T> const bus_underlying_vector_type<T> &graph_impl::bus_X_cref() const
{
    if constexpr (T == data_type::i32) { return _bus_i32;
    } else if constexpr (T == data_type::str) { return _bus_str;
    } else if constexpr (T == data_type::fbuffer) { return _bus_fbuffer;
    }
}


template <data_type T, typename X>
void graph_impl::node_inout_spec::add_in_X(X &&x)
{
    size_t &cell_idx = _g->_bus[T]._bus_next_free_cell;
    _bus[T]._default_ins.push_back(cell_idx);
    _bus[T]._ins.push_back(cell_idx);
    _g->bus_X_ref<T>()[cell_idx++] = std::move(x);
}


template <data_type T>
void graph_impl::node_inout_spec::add_out_X()
{
    size_t &cell_idx = _g->_bus[T]._bus_next_free_cell;
    _g->_bus[T]._bus_spec.emplace(cell_idx, bus_cell_spec{ _node_idx, _bus[T]._outs.size() });
    _bus[T]._outs.push_back(cell_idx++);
}


template <data_type T>
const bus_underlying_type<T> &graph_impl::node_inout_spec::in_X(size_t idx) const
{
    return _g->bus_X_cref<T>().at(_bus.at(T)._ins.at(idx));
}


template <data_type T>
bus_underlying_type<T> &graph_impl::node_inout_spec::out_X(size_t idx)
{
    return _g->bus_X_ref<T>().at(_bus.at(T)._outs.at(idx));
}


template <data_type T>
bus_underlying_type<T> &graph_impl::in_X(size_t idx, size_t node_input)
{
    return bus_X_ref<T>().at(_nodes.at(idx).in_bus_idx(T, node_input));
}


template <data_type T>
const bus_underlying_type<T> &graph_impl::out_X(size_t idx, size_t node_output) const
{
    return bus_X_cref<T>().at(_nodes.at(idx).out_bus_idx(T, node_output));
}

