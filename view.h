#pragma once


struct view
{
    virtual ~view() = default;
    virtual void draw(int x, int y, int w, int h) = 0;
};


struct graph;


struct graph_view : view
{
    virtual void update(const graph &) = 0;
};
