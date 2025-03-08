#include <sstream>
#include <chrono>

#include "exceptions.h"
#include "nodes_impl.h"
#include "graph_impl.h"
#include "view_impl.h"
#include "expr.h"


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
            "\n"
            "nodes 2\n"
            "\n"
            "# idx(ui64) x(i32) y(i32) summ-i32 a(i32) b(i32)\n"
            "  0         -1     -1     summ-i32 42     69    \n"
            "\n"
            "# idx(ui64) x(i32) y(i32) summ-i32 a(i32)  b(i32) \n"
            "  1         -1     -1     summ-i32 out 0 0 out 0 0\n"
            "\n";
    std::stringstream ss;
    g.dump_graph(ss, false);
    EXPECT(ss.str() == expected_dump);
    ss.str("");

    const std::string expected_dump_compact =
            "version 1\n"
            "nodes 2\n"
            "0 -1 -1 summ-i32 42 69\n"
            "1 -1 -1 summ-i32 out 0 0 out 0 0\n";
    g.dump_graph(ss, true);
    EXPECT(ss.str() == expected_dump_compact);
    ss.str("");

    graph_impl gi2;

    const nodes_factory_impl nodes;
    ss.str(expected_dump);
    gi2.read_dump(ss, nodes);
    ss.str("");

    gi2.dump_graph(ss);
    EXPECT(ss.str() == expected_dump_compact);
    ss.str("");
}


void test_graph_run_buffer_map()
{
    graph_impl gi;
    graph &g = gi;

    size_t map_id = g.add_node(new map_f);
    g.str_in(map_id, map_f::expr) =
            "(a * 10 + 100) / 2 + a";
    g.fbuffer_in(map_id, map_f::buffer_in) =
            std::vector<float>{ 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };
    g.run_node(map_id);
    g.dump_graph(std::cout);

    const auto &result = g.fbuffer_out(map_id, map_f::buffer_out);
    std::cout << "size=" << result.size();
    for (const float &v : result) std::cout << ' ' << v;
    std::cout << '\n';
}


void test_parse_expr()
{
    expr("2 + 2");
    expr("2 + (2)");
    expr("2 + (3 - 2)");
    expr("2 + (3 - a)");
    expr("2 * (A - a)");
    expr("2 * A - a");
    expr("2 - A * a");
    expr("foo(2 , A , a)");
}


void test_graph_buffer_canvas()
{
    graph_impl gi;
    graph &g = gi;

    size_t canvas_id = g.add_node(new canvas_f);
    g.i32_in(canvas_id, canvas_f::width) = 6;
    g.i32_in(canvas_id, canvas_f::height) = 5;
    g.fbuffer_in(canvas_id, canvas_f::buffer_in) = std::vector<float>{
            0, 0, 0, 1, 1, 0,
            0, 0, 1, 0, 1, 0,
            0, 1, 1, 1, 1, 0,
            0, 0, 0, 0, 1, 0,
            0, 0, 0, 0, 1, 0, };
    g.run_node(canvas_id);
}


void test_copy_image_channels()
{
    graph_impl gi;
    graph &g = gi;

    size_t read = g.add_node(new readimg_f);
    size_t write = g.add_node(new writeimg_f);

    g.str_in(read, readimg_f::filepath) =
            "/home/pl/Pictures/rofls/психическое_здоровье.jpg";
    g.str_in(write, writeimg_f::filepath) =
            "tmp.jpeg";

    g.run_node(read);

    g.connect_nodes(read, readimg_f::buffer, write, writeimg_f::buffer);
    g.connect_nodes(read, readimg_f::width, write, writeimg_f::width);
    g.connect_nodes(read, readimg_f::height, write, writeimg_f::height);
    g.connect_nodes(read, readimg_f::channels, write, writeimg_f::channels);
    g.run_node(write);

//    size_t map = g.add_node(new map_f);
//    g.connect_nodes(read, readimg_f::buffer, map, map_f::buffer_in);
//    g.str_in(map, map_f::expr) = "(a * 0.1) + 0.9";
//    g.run_node(map);

    size_t split = g.add_node(new splitbuffer_f);
    g.connect_nodes(read, readimg_f::channels, split, splitbuffer_f::channels);
    g.update_node(split);
    g.update_node(split);

    g.connect_nodes(read, readimg_f::buffer, split, splitbuffer_f::buffer_in);
//    g.connect_nodes(map, readimg_f::buffer, split, splitbuffer_f::buffer_in);
    g.run_node(split);

    for (int i = 0; i < g.i32_out(read, readimg_f::channels); ++i) {
        size_t write = g.add_node(new writeimg_f);
        g.str_in(write, writeimg_f::filepath) = "tmp." + std::to_string(i) + ".jpeg";
        g.connect_nodes(split, splitbuffer_f::buffer_out_first + i, write, writeimg_f::buffer);
        g.connect_nodes(read, readimg_f::width, write, writeimg_f::width);
        g.connect_nodes(read, readimg_f::height, write, writeimg_f::height);
        g.i32_in(write, writeimg_f::channels) = 1;
        g.run_node(write);
    }
}


#include <raylib.h>

void test_start_view()
{
    graph_impl gi;
    graph &g = gi;

    size_t summ_id = g.add_node(new summ_i32);

    g.i32_in(summ_id, summ_i32::a) = 42;
    g.i32_in(summ_id, summ_i32::b) = 69;
    g.move_node(summ_id, 250, 200);

    graph_view_raylib vi;
    graph_view &v = vi;
    v.update(g);

    const int w = 600;
    const int h = 400;
    InitWindow(w, h, "puredata graph view");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(WHITE);
        v.draw(0, 0, w, h);
        // DrawText("Hello, World!", screenWidth / 2 - MeasureText("Hello, World!", 20) / 2, screenHeight / 2 - 10, 20, BLACK);
        EndDrawing();
    }
    CloseWindow();
}



int main()
{

    test_graph_run_dump_read();
    test_graph_run_buffer_map();
    test_parse_expr();
    test_graph_buffer_canvas();

    auto start = std::chrono::high_resolution_clock::now();
    test_copy_image_channels();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "took " << elapsed.count() << " ms\n";

    test_start_view();
    return 0;
}

