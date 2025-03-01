#include <sstream>

#include "nodes_impl.h"
#include "graph_impl.h"




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

