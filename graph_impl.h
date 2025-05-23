#pragma once

#include <unordered_map>
#include <map>
#include <memory>

#include "exceptions.h"
#include "graph.h"


template <data_type T> struct bus_type { using _type = void; };
template<> struct bus_type<data_type::i32> { using _type = int; };
template<> struct bus_type<data_type::str> { using _type = std::string; };
template<> struct bus_type<data_type::buffer_f> { using _type = std::vector<float>; };
template <data_type T> using bus_underlying_type = typename bus_type<T>::_type;
template <data_type T> using bus_underlying_vector_type = std::vector<bus_underlying_type<T>>;


struct graph_impl;


struct node_spec : node_init_ctx, node_run_ctx, node_update_ctx
{
    node_spec() = default;
    node_spec(graph_impl &g, node *node, size_t node_idx);
    node_spec(node_spec &&other);
    node_spec &operator=(node_spec &&other);
    void run();
    void update();
    bool was_removed() const { return _node == nullptr; }

    // node_init_ctx
    void set_name(const std::string &name) override;
    const std::string &name() const { return _name; }

    void add_in_i32(size_t id, int &&value = 0, const std::string &title = "") override {
        return add_in_X<data_type::i32>(id, std::move(value), title); }
    void add_out_i32(size_t id, const std::string &title = "") override {
        return add_out_X<data_type::i32>(id, title); }

    void add_in_fbuffer(size_t id, std::vector<float> &&value = {}, const std::string &title = "") override {
        return add_in_X<data_type::buffer_f>(id, std::move(value), title); }
    void add_out_fbuffer(size_t id, const std::string &title = "") override {
        return add_out_X<data_type::buffer_f>(id, title); }

    void add_in_str(size_t id, std::string &&value = "", const std::string &title = "") override {
        return add_in_X<data_type::str>(id, std::move(value), title); }

    // node_update_ctx
    enum { unstable, stable };
    void remove_unstable_outs() override;
    void add_unstable_out_fbuffer(size_t id, const std::string &title) override {
        return add_out_X<data_type::buffer_f>(id, title, unstable); }
    // TODO: make interface split and virtual inheretance to remove methods duplication
    const int &stable_in_i32(size_t id) const override { return i32_in(id); }

    // node_run_ctx
    foo_f parse_foo_f(const std::string &str, size_t &foo_input_count) override;
    void run_foo(const size_t start, const size_t length, const foo_iter &foo) override;

    const int &i32_in(size_t idx) const override {
        return in_X<data_type::i32>(idx); }
    int &i32_out(size_t idx) override {
        return out_X<data_type::i32>(idx); }

    const std::vector<float> &fbuffer_in(size_t idx) const override {
        return in_X<data_type::buffer_f>(idx); }
    std::vector<float> &fbuffer_out(size_t idx) override {
        return out_X<data_type::buffer_f>(idx); }

    const std::string &str_in(size_t idx) const override {
        return in_X<data_type::str>(idx); }

    // FIXME: TODO: redo warning/error as outputs!
    void warning(const std::string &msg) override;
    void error(const std::string &msg) override;
    void canvas_f(size_t w, size_t h, size_t size, const float *d) override;

    // graph_impl
    size_t in_bus_idx(size_t id) const {
        return _in_specs.at(id)._in_bus_idx; }
    data_type in_bus_type(size_t id) const {
        return _in_specs.at(id)._type; }
    size_t out_bus_idx(size_t id) const {
        return _out_specs.at(id)._out_bus_idx; }
    data_type out_bus_type(size_t id) const {
        return _out_specs.at(id)._type; }
    size_t default_in_bus_idx(size_t id) const {
        return _in_specs.at(id)._default_in_bus_idx; }
    size_t ins_count() const {
        return _in_specs.size(); }
    size_t outs_count() const {
        return _out_specs.size(); }
    void set_in_bus_idx(size_t id, size_t bus_idx) {
        _in_specs.at(id)._in_bus_idx = bus_idx; }
    const std::string &in_title_cref(size_t id) const {
        return _in_specs.at(id)._title; }
    size_t in_id_at(size_t idx) const { // input index to input id
        auto it = _in_specs.begin();
        while (idx) { ++it; --idx; }
        return it->first; }

    int _x = -1;
    int _y = -1;
private:
    struct in_spec
    {
        size_t _in_bus_idx = -1ul;
        size_t _default_in_bus_idx = -1ul;
        data_type _type = data_type::_first;
        std::string _title;
        bool _stable = true;
    };
    struct out_spec
    {
        size_t _out_bus_idx = -1ul;
        data_type _type = data_type::_first;
        std::string _title;
        bool _stable = true;
    };

    graph_impl *_g = nullptr;
    std::unique_ptr<node> _node;
    size_t _node_idx;

    std::string _name;
    std::map<size_t, in_spec> _in_specs;
    std::map<size_t, out_spec> _out_specs;

    template <data_type T, typename X> void add_in_X(size_t id, X &&x, const std::string &title, bool stable = true);
    template <data_type T> void add_out_X(size_t id, const std::string &title, bool stable = true);
    template <data_type T> const bus_underlying_type<T> &in_X(size_t idx) const;
    template <data_type T> bus_underlying_type<T> &out_X(size_t idx);
};


struct graph_impl : graph
{
    explicit graph_impl();

    // for graph
    size_t add_node(node *n) override;
    void set_node(size_t node_idx, node *n) override;
    void run_node(size_t node_idx) override;
    void update_node(size_t node_idx) override;
    void move_node(size_t node_idx, int x, int y) override;
    std::pair<int, int> node_xy(size_t node_idx) const override;
    std::vector<size_t> node_idxs() const override;
    void connect_nodes(
            size_t node_provider_idx,
            size_t node_provider_output,
            size_t node_reciever_idx,
            size_t node_reciever_input) override;

    int &i32_in(size_t node_idx, size_t node_input) override {
        return in_X<data_type::i32>(node_idx, node_input); }
    const int &i32_out(size_t node_idx, size_t node_output) const override {
        return out_X<data_type::i32>(node_idx, node_output); }

    std::vector<float> &fbuffer_in(size_t node_idx, size_t node_input) override {
        return in_X<data_type::buffer_f>(node_idx, node_input); }
    const std::vector<float> &fbuffer_out(size_t node_idx, size_t node_output) const override {
        return out_X<data_type::buffer_f>(node_idx, node_output); }

    std::string &str_in(size_t node_idx, size_t node_input) override {
        return in_X<data_type::str>(node_idx, node_input); }

    void dump_node_in_value(std::ostream &os, size_t node_idx, size_t input) const override;
    void dump_graph(std::ostream &os, const bool compact = true) const override;
    void read_dump(std::istream &is, const nodes_factory &node_idxs) override;

    // for node_spec
    template <data_type T> bus_underlying_vector_type<T> &bus_X_ref();
    template <data_type T> const bus_underlying_vector_type<T> &bus_X_cref() const;
    size_t next_free_bus_slot(data_type);
    void set_bus_slot_spec(data_type, size_t slot_idx, size_t node_idx, size_t output_id);
    void free_bus_slot(data_type, size_t slot_idx);
private:
    struct bus_slot_spec
    {
        size_t node_idx;
        size_t node_output_id;
        bool _freed = false;
    };
    struct bus
    {
        std::map<size_t, bus_slot_spec> _bus_spec;
        size_t _bus_next_free_slot = 0;
    };
    static std::unordered_map<data_type, bus> init_bus();
    std::vector<node_spec> _nodes;
    std::vector<int> _bus_i32;
    std::vector<std::vector<float>> _bus_fbuffer;
    std::vector<std::string> _bus_str;
    std::unordered_map<data_type, bus> _bus = init_bus();
    template <data_type T> bus_underlying_type<T> &in_X(size_t idx, size_t node_input);
    template <data_type T> const bus_underlying_type<T> &out_X(size_t idx, size_t node_output) const;
};









// impl


template <data_type T, typename X>
void node_spec::add_in_X(size_t id, X &&x, const std::string &title, bool stable)
{
    const size_t slot_idx = _g->next_free_bus_slot(T);
    _in_specs.emplace(id, in_spec { slot_idx, slot_idx, T, title, stable });
    _g->bus_X_ref<T>()[slot_idx] = std::move(x);
}


template <data_type T>
void node_spec::add_out_X(size_t id, const std::string &title, bool stable)
{
    const size_t slot_idx = _g->next_free_bus_slot(T);
    _out_specs.emplace(id, out_spec { slot_idx, T, title, stable });
    _g->set_bus_slot_spec(T, slot_idx, _node_idx, id);
}


template <data_type T>
const bus_underlying_type<T> &node_spec::in_X(size_t idx) const
{
    EXPECT(in_bus_type(idx) == T);
    return _g->bus_X_cref<T>().at(in_bus_idx(idx));
}


template <data_type T>
bus_underlying_type<T> &node_spec::out_X(size_t idx)
{
    EXPECT(out_bus_type(idx) == T);
    return _g->bus_X_ref<T>().at(out_bus_idx(idx));
}


template <data_type T>
bus_underlying_type<T> &graph_impl::in_X(size_t idx, size_t node_input)
{
    EXPECT(_nodes.at(idx).in_bus_type(node_input) == T);
    return bus_X_ref<T>().at(_nodes.at(idx).in_bus_idx(node_input));
}


template <data_type T>
const bus_underlying_type<T> &graph_impl::out_X(size_t idx, size_t node_output) const
{
    EXPECT(_nodes.at(idx).out_bus_type(node_output) == T);
    return bus_X_cref<T>().at(_nodes.at(idx).out_bus_idx(node_output));
}


// listing all types


template <data_type T> bus_underlying_vector_type<T> &graph_impl::bus_X_ref()
{
    if constexpr (T == data_type::i32) { return _bus_i32;
    } else if constexpr (T == data_type::str) { return _bus_str;
    } else if constexpr (T == data_type::buffer_f) { return _bus_fbuffer;
    }
}


template <data_type T> const bus_underlying_vector_type<T> &graph_impl::bus_X_cref() const
{
    if constexpr (T == data_type::i32) { return _bus_i32;
    } else if constexpr (T == data_type::str) { return _bus_str;
    } else if constexpr (T == data_type::buffer_f) { return _bus_fbuffer;
    }
}


inline std::unordered_map<data_type, graph_impl::bus>
graph_impl::init_bus()
{
    return {
        { data_type::i32, {}},
        { data_type::str, {}},
        { data_type::buffer_f, {}},
    };
}

inline graph_impl::graph_impl()
{
    constexpr size_t buffer_size = 1024;

    _bus_i32.resize(buffer_size);
    _bus_fbuffer.resize(buffer_size);
    _bus_str.resize(buffer_size);
}
