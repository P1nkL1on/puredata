# Roadmap

- [ ] Add a dirty state, and node caching
- [ ] User graphics Api 

# TL;DR
computational (in future image processing) graph inspired by https://github.com/pure-data/pure-data

here is a code in graph nodes to read an image and write it's channels separately:
```cpp
// void split_image_by_channels(graph &g) {

size_t read = g.add_node(new readimg_f);
size_t split = g.add_node(new splitbuffer_f);

g.connect_nodes(read, readimg_f::buffer, split, splitbuffer_f::buffer_in);
g.connect_nodes(read, readimg_f::channels, split, splitbuffer_f::channels);

g.str_in(read, readimg_f::filepath) = "/path/to/input/image.jpg";

g.run_node(read);
g.update_node(split);
g.run_node(split);

for (int i = 0; i < g.i32_out(read, readimg_f::channels); ++i) {
    size_t write = g.add_node(new writeimg_f);
    g.str_in(write, writeimg_f::filepath) = "/path/to/output/image." + std::to_string(i) + ".jpg";
    g.i32_in(write, writeimg_f::channels) = 1;
    g.connect_nodes(split, splitbuffer_f::buffer_out_first + i, write, writeimg_f::buffer);
    g.connect_nodes(read, readimg_f::width, write, writeimg_f::width);
    g.connect_nodes(read, readimg_f::height, write, writeimg_f::height);
    g.run_node(write);
}
```
