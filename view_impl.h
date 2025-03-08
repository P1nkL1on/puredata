#pragma once

#include <tuple>
#include <string>
#include <vector>
#include <unordered_map>

#include "view.h"


struct graph_view_raylib : graph_view
{
    void draw(int x, int y, int w, int h) override;
    void update(const graph &g) override;
private:
    struct plug_view
    {
        std::string _type;
        std::string _title;
        std::string _text;
    };
    struct node_view
    {
        int _x, _y;
        std::string _title;
        std::string _text;
        std::vector<plug_view> _ins;
        std::vector<plug_view> _outs;
    };
    std::unordered_map<size_t, node_view> _nodes;
};


