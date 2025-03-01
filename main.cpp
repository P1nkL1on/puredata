#include <vector>
#include <iostream>
#include <cstddef>
#include <memory>


struct node_init_ctx
{
    virtual ~node_init_ctx() = default;
    virtual void add_in_i32() = 0;
    virtual void add_out_i32() = 0;
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
        ctx.add_in_i32();
        ctx.add_in_i32();
        ctx.add_out_i32();
    }
    void run(node_run_ctx &ctx) override
    {
        ctx.i32_out(summ) = ctx.i32_in(a) + ctx.i32_in(b);
    }
};


struct graph
{
    struct node_inout_spec : node_init_ctx, node_run_ctx
    {
        void run()
        {
            _node->run(*this);
        }
        void add_in_i32() override
        {
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
        node_inout_spec(graph &g, node *node) : _g(g) 
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
    private:
        graph &_g;
        std::unique_ptr<node> _node;
        std::vector<size_t> _ins_i32;
        std::vector<size_t> _outs_i32;
    };

    std::vector<node_inout_spec> _nodes;
    std::vector<int> _bus_i32;
    size_t _bus_i32_next_free_cell = 0;

    size_t add_node(node *n)
    {
        _nodes.push_back(node_inout_spec(*this, n));
        return _nodes.size() - 1;
    }
    void connect_nodes(
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

    graph()
    {
        _bus_i32.resize(256);
        std::fill(_bus_i32.begin(), _bus_i32.end(), 0);

        size_t summ_id = add_node(new summ_i32);
        size_t summ_id2 = add_node(new summ_i32);

        _bus_i32[0] = 42;
        _bus_i32[1] = 69;
        connect_nodes(summ_id, summ_i32::summ, summ_id2, summ_i32::a);
        connect_nodes(summ_id, summ_i32::summ, summ_id2, summ_i32::b);

        _nodes[summ_id].run();
        _nodes[summ_id2].run();

        std::cout << _bus_i32[_nodes[summ_id].i32_out_bus_idx(summ_i32::summ)] << '\n';
        std::cout << _bus_i32[_nodes[summ_id2].i32_out_bus_idx(summ_i32::summ)] << '\n';
    }
};


int main()
{
    graph g;
    return 0;
}