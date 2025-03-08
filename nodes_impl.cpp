#include "nodes_impl.h"

#include "OpenImageIO/imageio.h"


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
