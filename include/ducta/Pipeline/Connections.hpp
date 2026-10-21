/**
 * @file Connections.hpp
 * @brief Output -> input connections of a pipeline, as described in its configuration
 *
 *   "connections": [
 *       { "a.out": ["b.in1", "c.in1"] },
 *       { "c.out": ["b.in2"] }
 *   ]
 *
 * Every member of an element connects one output ("<node>.<output>") to one or more inputs
 * ("<node>.<input>", or a single string instead of the array). The port name follows the last
 * '.', so node names may contain dots ("audio.left.out" is output "out" of node "audio.left").
 */

#ifndef DUCTA_PIPELINE_CONNECTIONS_HPP
#define DUCTA_PIPELINE_CONNECTIONS_HPP

#include "ducta/Core/Config.hpp"

#include "ducta/Core/Capacity.hpp"

#include "ducta/Configuration/Decode.hpp"
#include "ducta/Configuration/Diagnostics.hpp"
#include "ducta/Configuration/PropTree.hpp"

#include "ducta/Core/Types/String.hpp"
#include "ducta/Core/Types/StringView.hpp"
#include "ducta/Core/Types/Vector.hpp"

#include "ducta/Pipeline/Nodes.hpp"

namespace ducta {
namespace Pipeline {

using PortName = ::ducta::String<DUCTA_MAX_PORT_NAME_LENGTH>;

/* An input or output of a node, written "<node>.<port>" */
struct Port
{
    NodeName node;
    PortName name;
};

/* One output -> input connection */
struct PortLink
{
    Port output;
    Port input;
};

using PortLinks = Vector<PortLink, DUCTA_MAX_CONNECTIONS>;

namespace Private {

constexpr char kPortSeparator = '.';

/*
 * Parse "<node>.<port>", split at the last '.' (node names may contain dots, port names may
 * not); false if the separator, the node or the port is missing or too long
 */
inline bool parse_port(StringView text, Port& port)
{
    size_t const separator = text.rfind(kPortSeparator);
    if (separator == StringView::npos || separator == 0 || separator + 1 >= text.size())
    {
        return false;
    }
    StringView const node = text.substr(0, separator);
    StringView const name = text.substr(separator + 1);
    if (node.size() > port.node.max_size() || name.size() > port.name.max_size())
    {
        return false;
    }
    port.node.assign(node.data(), node.size());
    port.name.assign(name.data(), name.size());
    return true;
}

} // namespace Private

} // namespace Pipeline

namespace Configuration {

template <>
struct ValueDecoder<Pipeline::PortLinks>
{
    static constexpr bool accepts(NodeKind kind) noexcept
    {
        return kind == NodeKind::Array;
    }

    static bool decode(PropNode node, Pipeline::PortLinks& out, Diagnostics& diagnostics)
    {
        if (!Private::expect_kind(node, NodeKind::Array, diagnostics))
        {
            return false;
        }

        out.clear();
        bool ok = true;
        for (PropNode element : node)
        {
            if (!Private::expect_kind(element, NodeKind::Object, diagnostics))
            {
                ok = false;
                continue;
            }
            if (element.size() == 0)
            {
                diagnostics.error(
                    element, "expected { \"<node>.<output>\": [\"<node>.<input>\", ...] }");
                ok = false;
                continue;
            }
            for (PropNode inputs : element)
            {
                Pipeline::PortLink link;
                if (!Pipeline::Private::parse_port(inputs.key(), link.output))
                {
                    diagnostics.errorf(
                        inputs, "invalid output '{}' (expected \"<node>.<output>\")", inputs.key());
                    ok = false;
                    continue;
                }
                ok = decode_inputs(inputs, link, out, diagnostics) && ok;
            }
        }
        return ok;
    }

private:
    static bool decode_inputs(
        PropNode inputs,
        Pipeline::PortLink& link,
        Pipeline::PortLinks& out,
        Diagnostics& diagnostics)
    {
        if (inputs.kind() == NodeKind::String)
        {
            return add_input(inputs, link, out, diagnostics);
        }
        if (!Private::expect_kind(inputs, NodeKind::Array, diagnostics))
        {
            return false;
        }
        if (inputs.size() == 0)
        {
            diagnostics.errorf(inputs, "output '{}' is connected to no input", inputs.key());
            return false;
        }
        bool ok = true;
        for (PropNode input : inputs)
        {
            ok = add_input(input, link, out, diagnostics) && ok;
        }
        return ok;
    }

    static bool add_input(PropNode input, Pipeline::PortLink& link, Pipeline::PortLinks& out, Diagnostics& diagnostics)
    {
        if (!Private::expect_kind(input, NodeKind::String, diagnostics))
        {
            return false;
        }
        if (!Pipeline::Private::parse_port(input.as_string(), link.input))
        {
            diagnostics.errorf(
                input, "invalid input '{}' (expected \"<node>.<input>\")", input.as_string());
            return false;
        }
        if (out.size() >= out.max_size())
        {
            diagnostics.errorf(
                input, "too many connections (max {})", static_cast<unsigned long long>(out.max_size()));
            return false;
        }
        out.push_back(link);
        return true;
    }
};

} // namespace Configuration
} // namespace ducta

#endif // DUCTA_PIPELINE_CONNECTIONS_HPP
