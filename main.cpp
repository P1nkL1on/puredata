#include <ostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>
#include <cstddef>
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


struct graph
{
    virtual ~graph() = default;
    virtual size_t add_node(node *node) = 0;
    virtual void run_node(size_t node_idx) = 0;
    virtual int &i32_in(size_t node_idx, size_t node_input) = 0;
    virtual int i32_out(size_t node_idx, size_t node_output) const = 0;
    virtual void connect_nodes(
        size_t node_provider_idx,
        size_t node_provider_output,
        size_t node_reciever_idx,
        size_t node_reciever_input) = 0;
    virtual void dump(std::ostream &os) const = 0;
};


struct graph_impl : graph
{
    struct node_inout_spec : node_init_ctx, node_run_ctx
    {
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
            _g._bus_i32[_g._bus_i32_next_free_cell] = value_default;
            _default_ins_i32.push_back(_g._bus_i32_next_free_cell);
            _ins_i32.push_back(_g._bus_i32_next_free_cell++);
        }
        void add_out_i32() override
        {
            _outs_i32.push_back(_g._bus_i32_next_free_cell++);
        }
        int i32_in(size_t idx) const override
        {
            return _g._bus_i32[_ins_i32[idx]];
        }
        int &i32_out(size_t idx) override
        {
            return _g._bus_i32[_outs_i32[idx]];
        }
        node_inout_spec(graph_impl &g, node *node) : _g(g) 
        {
            _node.reset(node);
            _node->init(*this);
        }
        size_t i32_out_bus_idx(size_t idx) const 
        {
            return _outs_i32[idx]; 
        }
        size_t i32_in_bus_idx(size_t idx) const 
        {
            return _ins_i32[idx]; 
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
    private:
        graph_impl &_g;
        std::unique_ptr<node> _node;
        std::string _name;
        std::vector<size_t> _default_ins_i32;
        std::vector<size_t> _ins_i32; // may change after init to declare input connections
        std::vector<size_t> _outs_i32;
    };

    std::vector<node_inout_spec> _nodes;
    std::vector<int> _bus_i32;
    size_t _bus_i32_next_free_cell = 0;

    graph_impl()
    {
        _bus_i32.resize(2048);
    }
    size_t add_node(node *n) override
    {
        _nodes.push_back(node_inout_spec(*this, n));
        return _nodes.size() - 1;
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
        for (size_t node_idx = 0; node_idx < _nodes.size(); ++node_idx) {
            if (_nodes[node_idx].was_removed()) continue;

        }
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
        ctx.set_name("");
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

    graph_impl gi;
    graph &g = gi;

    size_t summ_id = g.add_node(new summ_i32);
    size_t summ_id2 = g.add_node(new summ_i32);

    g.i32_in(summ_id, summ_i32::a) = 42;
    g.i32_in(summ_id, summ_i32::b) = 69;
    g.connect_nodes(summ_id, summ_i32::summ, summ_id2, summ_i32::a);
    g.connect_nodes(summ_id, summ_i32::summ, summ_id2, summ_i32::b);

    g.run_node(summ_id);
    g.run_node(summ_id2);

    std::cout << g.i32_out(summ_id, summ_i32::summ) << '\n';
    std::cout << g.i32_out(summ_id2, summ_i32::summ) << '\n';
    return 0;
}

