#include "graph_impl.h"

#include <sstream>
#include "exceptions.h"
#include "expr.h"


node_spec::node_spec(graph_impl &g, node *node, size_t node_idx) :
    _g(&g), _node_idx(node_idx)
{
    _node.reset(node);
    if (_node)
        _node->init(*this);
}

node_spec::node_spec(node_spec &&other)
{
    *this = std::move(other);
}

node_spec &node_spec::operator=(node_spec &&other)
{
    _g = other._g;
    _node.reset(other._node.release());
    _node_idx = other._node_idx;
    _name = std::move(other._name);
    _in_specs = std::move(other._in_specs);
    _out_specs = std::move(other._out_specs);
    return *this;
}

void node_spec::run()
{
    _node->run(*this);
}

void node_spec::update()
{
    _node->update(*this);
}

void node_spec::set_name(const std::string &name)
{
    EXPECT(!name.empty());
    _name = name;
}

void node_spec::remove_unstable_outs()
{
    for (auto it = _out_specs.begin(); it != _out_specs.end(); ) {
        if (it->second._stable) { ++it; continue; }
        const out_spec &spec = it->second;
        _g->free_bus_slot(spec._type, spec._out_bus_idx);
        it = _out_specs.erase(it);
    }
}

foo_f node_spec::parse_foo_f(const std::string &expr_string, size_t &foo_input_count)
{
    std::shared_ptr<expr> foo(new expr(expr_string));
    foo_input_count = 1;
    return foo_f([foo](size_t count, const float *value_ptr) -> float {
        return foo->eval({ count, value_ptr });
    });
}

void node_spec::run_foo(const size_t start, const size_t length, const foo_iter &foo)
{
    const size_t chunk = 4096;
    for (size_t i = 0; i < length; i += chunk)
        foo(start + i, std::min(length - i, chunk));
}

void node_spec::warning(const std::string &msg)
{

}

void node_spec::error(const std::string &msg)
{

}

void node_spec::canvas_f(size_t w, size_t h, size_t size, const float *d)
{

}

size_t graph_impl::add_node(node *n)
{
    size_t node_idx = _nodes.size();
    set_node(node_idx, n);
    return node_idx;
}

void graph_impl::set_node(size_t node_idx, node *n)
{
    if (_nodes.size() < node_idx + 1)
        _nodes.resize(node_idx + 1);
    _nodes[node_idx] = node_spec(*this, n, node_idx);
}

void graph_impl::run_node(size_t node_idx)
{
    _nodes[node_idx].run();
}

void graph_impl::update_node(size_t node_idx)
{
    _nodes[node_idx].update();
}

void graph_impl::move_node(size_t node_idx, int x, int y)
{
    _nodes[node_idx]._x = x;
    _nodes[node_idx]._y = y;
}

std::pair<int, int> graph_impl::node_xy(size_t node_idx) const
{
    const node_spec &node = _nodes[node_idx];
    return { node._x, node._y };
}

std::vector<size_t> graph_impl::node_idxs() const
{
    std::vector<size_t> idxs; idxs.reserve(_nodes.size());
    for (size_t i = 0; i < _nodes.size(); ++i)
        if (!_nodes.at(i).was_removed()) idxs.push_back(i);
    return idxs;
}

void graph_impl::connect_nodes(
        size_t node_provider_idx,
        size_t node_provider_output,
        size_t node_reciever_idx,
        size_t node_reciever_input)
{
    EXPECT(_nodes.at(node_provider_idx).out_bus_type(node_provider_output)
           == _nodes.at(node_reciever_idx).in_bus_type(node_reciever_input));

    _nodes.at(node_reciever_idx).set_in_bus_idx(
                node_reciever_input,
                _nodes.at(node_provider_idx).out_bus_idx(node_provider_output));
}

void graph_impl::dump_node_in_value(
        std::ostream &os, size_t node_idx, size_t node_input) const
{
    const size_t bus_offset = _nodes.at(node_idx).in_bus_idx(node_input);
    switch (_nodes.at(node_idx).in_bus_type(node_input)) {
        case data_type::i32:
            os << _bus_i32.at(bus_offset);
            return;
        case data_type::buffer_f: {
            const std::vector<float> &buffer = _bus_fbuffer.at(bus_offset);
            os << buffer.size();
            for (const float &v : buffer) os << ' ' << v;
            return;
        }
        case data_type::str:
            os << '"' << _bus_str.at(bus_offset) << '"'; // escape \n \t etc
            return;
        default:
            break;
    }
    EXPECT(false && "unreachable");
}

void graph_impl::dump_graph(std::ostream &os, const bool compact) const
{
    os << "version 1\n";
    if (!compact) os << '\n';

    const size_t nodes_count = _nodes.size();
    os << "nodes " << nodes_count << '\n';
    if (!compact) os << '\n';

    for (size_t node_idx = 0; node_idx < nodes_count; ++node_idx) {
        const node_spec &spec = _nodes[node_idx];
        if (spec.was_removed()) continue;

        const size_t ins_count = spec.ins_count();

        std::stringstream comment;
        std::stringstream line;
        const auto fill_spaces = [compact, &comment, &line] {
            if (compact) return;
            while (comment.tellp() < line.tellp()) comment << ' ';
            while (line.tellp() < comment.tellp()) line << ' ';
        };
        if (!compact) {
            comment << "# ";
            fill_spaces();
        }
        comment << "idx(ui64) ";
        line << node_idx << ' ';
        fill_spaces();

        comment << "x(i32) ";
        line << spec._x << ' ';
        fill_spaces();

        comment << "y(i32) ";
        line << spec._y << ' ';
        fill_spaces();

        comment << spec.name();
        line << spec.name();

        for (size_t i = 0; i < ins_count; ++i) {
            const size_t id = spec.in_id_at(i);

            comment << ' ';
            comment << spec.in_title_cref(id);
            comment << '(';
            comment << data_type_titles.at(
                      static_cast<size_t>(spec.in_bus_type(id)));
            comment << ')';

            line << ' ';
            const bool has_own_value =
                    spec.in_bus_idx(id) == spec.default_in_bus_idx(id);
            if (has_own_value) {
                dump_node_in_value(line, node_idx, id);
            } else {
                const bus_slot_spec &connection =
                        _bus.at(spec.in_bus_type(id))._bus_spec.at(spec.in_bus_idx(id));
                line << "out " << connection.node_idx << ' ' << connection.node_output_id;
            }
            fill_spaces();
        }

        if (!compact) {
            os << comment.str() << '\n' << line.str() << "\n\n";
        } else {
            os << line.str() << '\n';
        }
    }
}

void graph_impl::read_dump(std::istream &is, const nodes_factory &nodes)
{
    // FIXME: work on how user will see errors

//    std::ostream &os = std::cout;
    std::vector<std::string> stack_trace;
    const auto skip_lines = [&is] {
        std::string line;
        while (std::getline(is, line)) {
            std::istringstream iss(line);
            std::string first_word;
            iss >> std::ws >> first_word;
            if (first_word.empty()) {
//                os << "skipped an empty line\n";
                continue;
            }
            if (first_word[0] == '#') {
//                os << "skipped a comment line\n";
                continue;
            }
            is.seekg(-static_cast<int>(line.size()) - 1, std::ios_base::cur);
            break;
        }
    };
    const auto bad_token = [&is, &stack_trace] {
        is.clear(); std::string s; is >> s;
        stack_trace.push_back("ERROR: get '" + s + "' instead!");
        std::stringstream ss;
        ss << "reading graph dump failed:\n";
        for (const std::string &s : stack_trace) ss << s << '\n';
        throw bad_io(ss.str());
    };
    const auto expect_keyword = [&](const std::string &keyword) {
        stack_trace.push_back("expected keyword '" + keyword + "'");
        std::string s; is >> std::ws >> s;
        if (s != keyword) bad_token();
        stack_trace.pop_back();
//        os << "parsed keyword " << keyword << '\n';
    };
    const auto peek_keyword = [&](const std::string &keyword) -> bool {
        auto pos = is.tellg();
        std::string s; is >> std::ws >> s;
        if (s != keyword) { is.seekg(pos); return false; }
//        os << "peeked keyword " << keyword << '\n';
        return true;
    };
    const auto expect_str = [&](std::string &f, const std::string &what) {
        stack_trace.push_back("expected string in double quotes (" + what + ")");
        char quote;
        if (!(is >> std::ws >> quote) || quote != '"') bad_token();
        if (!std::getline(is, f, '"')) bad_token();
//        os << "parsed str " << f << " (" << what << ")\n";
        stack_trace.pop_back();
    };
    const auto expect_f = [&](float &f, const std::string &what) {
        stack_trace.push_back("expected 32-bit floating number (" + what + ")");
        if (!(is >> std::ws >> f)) bad_token();
//        os << "parsed f " << f << " (" << what << ")\n";
        stack_trace.pop_back();
    };
    const auto expect_i32 = [&](int &i, const std::string &what) {
        stack_trace.push_back("expected 32-bit signed integer (" + what + ")");
        if (!(is >> std::ws >> i)) bad_token();
//        os << "parsed i32 " << i << " (" << what << ")\n";
        stack_trace.pop_back();
    };
    const auto expect_ui64 = [&](size_t &i, const std::string &what) {
        stack_trace.push_back("expected 64-bit unsigned integer (" + what + ")");
        if (!(is >> std::ws >> i)) bad_token();
//        os << "parsed i32 " << i << " (" << what << ")\n";
        stack_trace.pop_back();
    };

    stack_trace.push_back("parsing graph");
    skip_lines();
    expect_keyword("version");
    size_t version; expect_ui64(version, "graph version number");
    if (version != 1)
        throw bad_io("ERROR: can read projects with version 1 only. "
                     "get " + std::to_string(version) + "!");
    skip_lines();
    expect_keyword("nodes");
    size_t nodes_count; expect_ui64(nodes_count, "graph nodes count");

    graph_impl g;
    size_t node_idx;
    int node_x;
    int node_y;
    std::string node_name;
    std::vector<std::tuple<size_t, size_t, size_t, size_t>> connections;
    for (size_t i = 0; i < nodes_count; ++i) {
        skip_lines();
        stack_trace.push_back(
                    std::string("parsing node ") + std::to_string(i + 1) + " out of " + std::to_string(nodes_count));
        expect_ui64(node_idx, "node index");
        expect_i32(node_x, "node x");
        expect_i32(node_y, "node y");

        is >> std::ws >> node_name;
//        os << "parsed node name '" << node_name << "'\n";
        node *n = nodes.create(node_name);
        if (n == nullptr)
            throw bad_io("ERROR: unknown node name " + node_name);
        g.set_node(node_idx, n);
        stack_trace.push_back(std::string("parsing node args for ") + node_name);
        {
            const node_spec &spec = g._nodes[node_idx];
            for (size_t i = 0; i < spec.ins_count(); ++i) {
                const size_t id = spec.in_id_at(i);
                stack_trace.push_back(
                            std::string("parsing node arg #") + std::to_string(i + 1)
                            + " of " + std::to_string(spec.ins_count())
                            + ", named '"
                            + spec.in_title_cref(id) + "' of type "
                            + data_type_titles.at(static_cast<size_t>(spec.in_bus_type(id)))
                            + " value, or 'out' keyword");
                if (peek_keyword("out")) {
                    stack_trace.push_back("parsing node arg connection 'out'");
                    size_t provider_idx; expect_ui64(provider_idx, "provider node index");
                    size_t provider_output_id; expect_ui64(provider_output_id, "provider node output id");
                    connections.emplace_back(provider_idx, provider_output_id, node_idx, id);
                    stack_trace.pop_back();

                } else switch (spec.in_bus_type(id)) {
                    case data_type::i32: {
                        expect_i32(g.i32_in(node_idx, id), "arg i32 value");
                        break;
                    }
                    case data_type::buffer_f: {
                        size_t size; expect_ui64(size, "arg fbuffer size (i32)");
                        g.fbuffer_in(node_idx, id).resize(size);
                        for (size_t i = 0; i < size; ++i) {
                            expect_f(g.fbuffer_in(node_idx, id).at(i),
                                     "arg fbuffer value " + std::to_string(i + 1) + " out of " + std::to_string(size));
                        }
                        break;
                    }
                    case data_type::str: {
                        expect_str(g.str_in(node_idx, id), "arg str value");
                        break;
                    }
                    default:
                        EXPECT(false && "unreachable");
                }
                stack_trace.pop_back();
            }
        }
        stack_trace.pop_back();
        stack_trace.pop_back();
    }
    stack_trace.pop_back();

    for (const auto &[pidx, poidx, ridx, riidx] : connections)
        g.connect_nodes(pidx, poidx, ridx, riidx);
    *this = std::move(g);
}

size_t graph_impl::next_free_bus_slot(data_type type)
{
    bus &b = _bus.at(type);
    for (auto it = b._bus_spec.begin(); it != b._bus_spec.end(); ++it)
        if (it->second._freed) return it->first;
    return b._bus_next_free_slot++;
}

void graph_impl::set_bus_slot_spec(
        data_type type, size_t slot_idx, size_t node_idx, size_t output_id)
{
    bus &b = _bus.at(type);
    b._bus_spec.insert_or_assign(
                slot_idx, bus_slot_spec{ node_idx, output_id, false });
}

void graph_impl::free_bus_slot(data_type type, size_t slot_idx)
{
    bus &b = _bus.at(type);
    b._bus_spec.at(slot_idx)._freed = true;
}
