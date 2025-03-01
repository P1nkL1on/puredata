#include <ostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>
#include <cstddef>
#include <unordered_map>
#include <memory>


struct constraint_violated : std::runtime_error
{
    constraint_violated(const std::string &msg = "") : std::runtime_error(msg) {}
};


struct bad_io : std::runtime_error
{
    bad_io(const std::string &msg = "") : std::runtime_error(msg) {}
};


#define EXPECT(condition) \
if (!(condition)) throw constraint_violated(__FILE__ ":" + std::to_string(__LINE__) + ": constraint violated: " #condition)


struct node_init_ctx
{
    virtual ~node_init_ctx() = default;
    virtual void add_in_i32(int value_default = 0) = 0;
    virtual void add_out_i32() = 0;
    virtual void set_name(const std::string &) = 0;
};


struct node_run_ctx
{
    virtual ~node_run_ctx() = default;
    virtual int i32_in(size_t idx) const = 0;
    virtual int &i32_out(size_t idx) = 0;
};


struct node
{
    virtual ~node() = default;
    virtual void init(node_init_ctx &ctx) = 0; // aka signature
    virtual void run(node_run_ctx &ctx) = 0;
};


struct nodes_factory
{
    virtual ~nodes_factory() = default;
    virtual node *create(const std::string &name) const = 0;
};


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


struct graph_impl : graph
{
    struct node_inout_spec : node_init_ctx, node_run_ctx
    {
        node_inout_spec() = default;
        node_inout_spec(graph_impl &g, node *node, int node_idx) : _g(&g), _node_idx(node_idx)
        {
            _node.reset(node);
            if (_node)
                _node->init(*this);
        }
        node_inout_spec(node_inout_spec &&other)
        {
            *this = std::move(other);
        }
        node_inout_spec &operator=(node_inout_spec &&other)
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
        void run()
        {
            _node->run(*this);
        }
        void set_name(const std::string &name) override
        {
            EXPECT(!name.empty());
            _name = name;
        }
        void add_in_i32(int value_default = 0) override
        {
            _g->_bus_i32[_g->_bus_i32_next_free_cell] = value_default;
            _default_ins_i32.push_back(_g->_bus_i32_next_free_cell);
            _ins_i32.push_back(_g->_bus_i32_next_free_cell++);
        }
        void add_out_i32() override
        {
            _g->_bus_i32_spec.emplace(_g->_bus_i32_next_free_cell, bus_cell_spec{ _node_idx, _outs_i32.size() });
            _outs_i32.push_back(_g->_bus_i32_next_free_cell++);
        }
        int i32_in(size_t idx) const override
        {
            return _g->_bus_i32[_ins_i32[idx]];
        }
        int &i32_out(size_t idx) override
        {
            return _g->_bus_i32[_outs_i32[idx]];
        }
        size_t i32_out_bus_idx(size_t idx) const 
        {
            return _outs_i32[idx]; 
        }
        size_t i32_in_bus_idx(size_t idx) const 
        {
            return _ins_i32[idx]; 
        }
        size_t i32_default_in_bus_idx(size_t idx) const 
        {
            return _default_ins_i32[idx]; 
        }
        void set_i32_in_bus_idx(size_t idx, size_t bus_idx) 
        {
            _ins_i32[idx] = bus_idx; 
        }
        bool was_removed() const
        {
            return _node == nullptr;
        }
        const std::string &name() const
        {
            return _name;
        }
        size_t ins_i32_count() const
        {
            return _ins_i32.size();
        }
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

    graph_impl()
    {
        _bus_i32.resize(2048);
    }
    size_t add_node(node *n) override
    {
        size_t node_idx = _nodes.size();
        set_node(node_idx, n);
        return node_idx;
    }
    void set_node(size_t node_idx, node *n) override
    {
        if (_nodes.size() < node_idx + 1)
            _nodes.resize(node_idx + 1);
        _nodes[node_idx] = node_inout_spec(*this, n, node_idx);
    }
    void run_node(size_t node_idx) override
    {
        _nodes[node_idx].run();
    }
    void connect_nodes(
        size_t node_provider_idx,
        size_t node_provider_output,
        size_t node_reciever_idx,
        size_t node_reciever_input) override
    {
        // TODO: check index valid and type OK
        // TODO: add connection type dispatch
        _nodes[node_reciever_idx].set_i32_in_bus_idx(
            node_reciever_input, _nodes[node_provider_idx].i32_out_bus_idx(node_provider_output));
    }
    int &i32_in(size_t node_idx, size_t node_input) override
    {
        return _bus_i32[_nodes.at(node_idx).i32_in_bus_idx(node_input)];
    }
    int i32_out(size_t node_idx, size_t node_output) const override
    {
        return _bus_i32[_nodes.at(node_idx).i32_out_bus_idx(node_output)];
    }
    void dump(std::ostream &os) const override
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
    void read_dump(std::istream &is, const nodes_factory &nodes) override
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
};


// impl


struct summ_i32 : node
{
    enum {
        a, b,
    };
    enum {
        summ
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


struct nodes_factory_impl : nodes_factory
{
    node *create(const std::string &name) const override
    {
        if (name == "summ_i32") return new summ_i32;

        return nullptr;
    }
};


// #include <raylib.h>

int main()
{
    // const int screenWidth = 600;
    // const int screenHeight = 400;
    // InitWindow(screenWidth, screenHeight, "Hello World Window");
    // SetTargetFPS(60);
    // while (!WindowShouldClose()) {
    //     BeginDrawing();
    //     ClearBackground(RED);
    //     DrawText("Hello, World!", screenWidth / 2 - MeasureText("Hello, World!", 20) / 2, screenHeight / 2 - 10, 20, BLACK);
    //     EndDrawing();
    // }
    // CloseWindow();

    graph_impl gi;
    graph &g = gi;

    size_t summ_id = g.add_node(new summ_i32);
    size_t summ_id2 = g.add_node(new summ_i32);

    g.i32_in(summ_id, summ_i32::a) = 42;
    g.i32_in(summ_id, summ_i32::b) = 69;
    g.connect_nodes(summ_id, summ_i32::summ, summ_id2, summ_i32::a);

    g.run_node(summ_id);
    g.run_node(summ_id2);

    std::cout << g.i32_out(summ_id, summ_i32::summ) << '\n';
    std::cout << g.i32_out(summ_id2, summ_i32::summ) << '\n';

    std::stringstream ss;
    g.dump(ss);
    std::cout << ss.str();

    graph_impl gi2;

    const nodes_factory_impl nodes;
    gi2.read_dump(ss, nodes);

    gi2.dump(std::cout);
    return 0;
}

