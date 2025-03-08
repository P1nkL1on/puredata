#include "view_impl.h"

#include <raylib.h>
#include "graph.h"


void graph_view_raylib::draw(int x, int y, int w, int h)
{
    DrawRectangleLines(
                x, y, w, h, RED);
    for (auto it = _nodes.begin(); it != _nodes.end(); ++it) {
        const node_view &nv = it->second;
        DrawRectangleLines(x + nv._x, y + nv._y, 50, 50, RED);
        DrawText(nv._title.c_str(), x + nv._x, y + nv._y, 20, RED);
    }
}

void graph_view_raylib::update(const graph &g)
{
    const auto idxs = g.node_idxs();

    _nodes.clear(); // TODO: yet no caching
    for (size_t node_idx : idxs) {
        node_view nv;
        std::tie(nv._x, nv._y) = g.node_xy(node_idx);
         nv._title = "summ-i32";
        _nodes.insert_or_assign(node_idx, std::move(nv));
    }
}
