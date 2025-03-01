#include <ostream>
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
        const auto dump_out_i32 = [&os, &specs = _bus_i32_spec](size_t idx) {
            const bus_cell_spec &spec = specs.at(idx);
            os << "(out ";
            os << spec.node_idx << ' ' << spec.node_output_idx;
            os << ')';
        };
        for (size_t node_idx = 0; node_idx < _nodes.size(); ++node_idx) {
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
        std::vector<std::tuple<size_t, size_t, size_t, size_t>> connections;
        const auto try_read_parens = [&is, &connections](size_t node_idx, size_t in_idx) {
            if (is.peek() != '(') 
                return false;
            is.ignore(); // (
            std::string type;
            size_t provider_idx;
            size_t provider_output_idx;
            EXPECT(is >> type && type == "out");
            EXPECT(is >> provider_idx && "expected a provider node idx (ui64)");
            EXPECT(is >> provider_output_idx && "expected a provider node output idx (ui64)");
            is.ignore(); // )
            connections.emplace_back(provider_idx, provider_output_idx, node_idx, in_idx);
            return true;
        };

        graph_impl g;
        while (is) {
            size_t node_idx;
            std::string node_name;
            EXPECT(is >> node_idx && "expected a node idx (ui64)");
            EXPECT(is >> node_name && "expected a node name (str)");
            node *n = nodes.create(node_name);
            EXPECT(n != nullptr && "unknown node name");
            g.set_node(node_idx, n);

            const size_t ins_i32_count = _nodes[node_idx].ins_i32_count();
            if (ins_i32_count) {
                std::string token;
                is >> token;    
                EXPECT(token == "i32");
                for (size_t i = 0; i < ins_i32_count; ++i) {
                    if (try_read_parens(node_idx, i))
                        continue;int i32;
                    EXPECT(is >> i32 && "expected a value (i32)");
                    g.i32_in(node_idx, i) = i32;
                }
            }

            // read other type params
        }
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

    std::cout << "a\n";
    graph_impl gi;
    graph &g = gi;

    std::cout << "b\n";
    size_t summ_id = g.add_node(new summ_i32);
    std::cout << summ_id << "b1\n";
    size_t summ_id2 = g.add_node(new summ_i32);
    std::cout << summ_id2 << "b2\n";

    std::cout << "c\n";
    g.i32_in(summ_id, summ_i32::a) = 42;
    g.i32_in(summ_id, summ_i32::b) = 69;
    g.connect_nodes(summ_id, summ_i32::summ, summ_id2, summ_i32::a);

    std::cout << "d\n";
    g.run_node(summ_id);
    g.run_node(summ_id2);

    std::cout << g.i32_out(summ_id, summ_i32::summ) << '\n';
    std::cout << g.i32_out(summ_id2, summ_i32::summ) << '\n';

    g.dump(std::cout);

    return 0;
}

