#include <sstream>

#include "exceptions.h"
#include "nodes_impl.h"
#include "graph_impl.h"


void test_graph_run_dump_read()
{
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

    EXPECT(g.i32_out(summ_id, summ_i32::summ) == 111);
    EXPECT(g.i32_out(summ_id2, summ_i32::summ) == 222);

    const std::string expected_dump =
            "version 1\n"
            "nodes 2\n"
            "0 summ_i32 i32 42 69\n"
            "1 summ_i32 i32 out 0 0 out 0 0\n";

    std::stringstream ss;
    g.dump(ss);
    EXPECT(ss.str() == expected_dump);

    graph_impl gi2;

    const nodes_factory_impl nodes;
    gi2.read_dump(ss, nodes);

    std::stringstream ss2;
    gi2.dump(ss2);
    EXPECT(ss2.str() == expected_dump);
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
    return 0;
}

