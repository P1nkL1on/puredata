#include <sstream>

#include "exceptions.h"
#include "nodes_impl.h"
#include "graph_impl.h"
#include "expr.h"


void test_graph_run_dump_read()
{
    graph_impl gi;
    graph &g = gi;

    size_t summ_id = g.add_node(new summ_i32);
    size_t summ_id2 = g.add_node(new summ_i32);

    g.i32_in(summ_id, summ_i32::a) = 42;
    g.i32_in(summ_id, summ_i32::b) = 69;
    g.connect_nodes(data_type::i32, summ_id, summ_i32::summ, summ_id2, summ_i32::a);
    g.connect_nodes(data_type::i32, summ_id, summ_i32::summ, summ_id2, summ_i32::b);

    g.run_node(summ_id);
    g.run_node(summ_id2);

    EXPECT(g.i32_out(summ_id, summ_i32::summ) == 111);
    EXPECT(g.i32_out(summ_id2, summ_i32::summ) == 222);

    const std::string expected_dump =
            "version 1\n"
            "nodes 2\n"
            "0 summ-i32 i32 42 69\n"
            "1 summ-i32 i32 out-0-0 out-0-0\n";

    std::stringstream ss;
    g.dump_graph(ss);
    std::cout << ss.str();
    EXPECT(ss.str() == expected_dump);

    graph_impl gi2;

    const nodes_factory_impl nodes;
    gi2.read_dump(ss, nodes);

    std::stringstream ss2;
    gi2.dump_graph(ss2);
    return;
    EXPECT(ss2.str() == expected_dump);
}


void test_graph_run_buffer_map()
{
    graph_impl gi;
    graph &g = gi;

    size_t map_id = g.add_node(new map_f);
    g.fbuffer_in(map_id, map_f::buffer_in) = std::vector<float>{ 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };
    g.run_node(map_id);
    g.dump_graph(std::cout);

    const auto &result = g.fbuffer_out(map_id, map_f::buffer_out);
    std::cout << "size=" << result.size();
    for (const float &v : result) std::cout << ' ' << v;
    std::cout << '\n';
}


void test_expr()
{
    expr::parse("2 + 2");
    expr::parse("2 + (2)");
    expr::parse("2 + (3 - 2)");
    expr::parse("2 + (3 - a)");
    expr::parse("2 * (A - a)");
    expr::parse("2 * A - a");
    expr::parse("2 - A * a");
    expr::parse("foo(2 , A , a)");
}


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

    test_graph_run_dump_read();
    test_graph_run_buffer_map();
    test_expr();
    return 0;
}

