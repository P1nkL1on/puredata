#include "nodes_impl.h"

#include "OpenImageIO/imageio.h"


void readimg_f::init(node_init_ctx &ctx)
{
    ctx.set_name("readimg-f");
    ctx.add_in_str(filepath);
    ctx.add_out_i32(width);
    ctx.add_out_i32(height);
    ctx.add_out_i32(channels);
    ctx.add_out_fbuffer(buffer);
}

void readimg_f::run(node_run_ctx &ctx)
{
    const std::string &_filepath = ctx.str_in(filepath);
    auto in = OIIO::ImageInput::open(_filepath);
    if (!in) {
        ctx.error("can't open image file: " + _filepath);
        return;
    }
    const OIIO::ImageSpec &spec = in->spec();
    const size_t values_count = static_cast<size_t>(
                spec.width * spec.height * spec.nchannels);
    ctx.fbuffer_out(buffer).resize(values_count);
    in->read_image(OIIO::TypeDesc::FLOAT, ctx.fbuffer_out(buffer).data());
    in->close();

    ctx.i32_out(width) = spec.width;
    ctx.i32_out(height) = spec.height;
    ctx.i32_out(channels) = spec.nchannels;
}

void writeimg_f::init(node_init_ctx &ctx)
{
    ctx.set_name("readimg-f");
    ctx.add_in_str(filepath);
    ctx.add_in_i32(width);
    ctx.add_in_i32(height);
    ctx.add_in_i32(channels);
    ctx.add_in_fbuffer(buffer);
}

void writeimg_f::run(node_run_ctx &ctx)
{
    const std::string &_filepath = ctx.str_in(filepath);
    const int w = ctx.i32_in(width);
    const int h = ctx.i32_in(height);
    const int c = ctx.i32_in(channels);
    const std::vector<float> &data = ctx.fbuffer_in(buffer);
    auto out = OIIO::ImageOutput::create(_filepath);
    if (!out) {
        ctx.error("can't not create image file: " + _filepath);
        return;
    }
    OIIO::ImageSpec spec(w, h, c, OIIO::TypeDesc::FLOAT);
    out->open(_filepath, spec);
    out->write_image(OIIO::TypeDesc::FLOAT, data.data());
    out->close();
}

void splitbuffer_f::init(node_init_ctx &ctx)
{
    ctx.set_name("splitbuffer-f");
    ctx.add_in_fbuffer(buffer_in);
    ctx.add_in_i32(channels, 1);
}

void splitbuffer_f::update(node_update_ctx &ctx)
{
    const int c = ctx.stable_in_i32(channels);
    ctx.remove_unstable_outs();
    for (int i = 0; i < c; ++i)
        ctx.add_unstable_out_fbuffer(static_cast<size_t>(buffer_out_first + i));
}

void splitbuffer_f::run(node_run_ctx &ctx)
{
    const std::vector<float> &data = ctx.fbuffer_in(buffer_in);
    const int _c = ctx.i32_in(channels);
    if (_c <= 0) {
        ctx.error("insufficient channel number");
        return;
    }
    const auto c = static_cast<size_t>(_c);
    if (data.size() % c != 0) {
        ctx.warning("some values will be lost");
    }
    std::vector<std::vector<float> *> outs;
    for (size_t i = 0; i < c; ++i)
        outs.push_back(&ctx.fbuffer_out(buffer_out_first + i));

    const size_t n = data.size() / c;
    for (auto &out : outs)
        out->resize(n);
    ctx.run_foo(0, data.size(), [outs, c, &data](size_t start, size_t length) {
        for (size_t i = 0; i < length; ++i)
            outs.at((start + i) % c)->at((start + i) / c) = data.at(start + i);
    });
}

void canvas_f::init(node_init_ctx &ctx)
{
    ctx.set_name("canvas-f");
    ctx.add_in_i32(width, 128);
    ctx.add_in_i32(height, 128);
    ctx.add_in_fbuffer(buffer_in);
    ctx.add_out_fbuffer(buffer_out);
}

void canvas_f::run(node_run_ctx &ctx)
{
    const std::vector<float> &in = ctx.fbuffer_in(buffer_in);
    const int w = ctx.i32_in(width);
    const int h = ctx.i32_in(height);
    if (w < 0 || h < 0)
        return ctx.error("W & H can't be negative");
    const int wh = static_cast<int>(in.size());
    if (w * h < wh)
        ctx.warning("buffer size can't cover W x H canvas");
    if (w * h > wh)
        ctx.warning("W x H canvas can't cover buffer size");
    ctx.canvas_f(
                static_cast<size_t>(w),
                static_cast<size_t>(h),
                in.size(), in.data());
    ctx.fbuffer_out(buffer_out) = in;
}

void map_f::init(node_init_ctx &ctx)
{
    ctx.set_name("map-f");
    ctx.add_in_str(expr, "a * 1 + 0");
    ctx.add_in_fbuffer(buffer_in);
    ctx.add_out_fbuffer(buffer_out);
}

void map_f::run(node_run_ctx &ctx)
{
    const std::vector<float> &in = ctx.fbuffer_in(buffer_in);

    std::vector<float> &out = ctx.fbuffer_out(buffer_out);

    size_t foo_input_count;
    foo_f foo = ctx.parse_foo_f(ctx.str_in(expr), foo_input_count);

    out.resize(in.size());
    ctx.run_foo(0, in.size(), [&in, &out, foo](size_t start, size_t length) {
        for (size_t i = 0; i < length; ++i)
            out[start + i] = foo(1, &in[start + i]);
    });
}

void summ_i32::init(node_init_ctx &ctx)
{
    ctx.set_name("summ-i32");
    ctx.add_in_i32(a, 0, "a");
    ctx.add_in_i32(b, 0, "b");
    ctx.add_out_i32(summ, "a+b");
}

void summ_i32::run(node_run_ctx &ctx)
{
    ctx.i32_out(summ) = ctx.i32_in(a) + ctx.i32_in(b);
}
