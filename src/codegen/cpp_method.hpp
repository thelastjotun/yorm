#pragma once

#include <fmt/format.h>
#include <iterator>
#include <string>

struct CppMethod final
{
    std::string return_type;
    std::string signature;
    std::string body;

    std::string render(int indent_level = 0) const
    {
        std::string render;
        std::string indent(indent_level, '\t');

        fmt::format_to(std::back_inserter(render), "{}{} {} {{\n", indent, return_type, signature);
        fmt::format_to(std::back_inserter(render), "{}\t{}\n", indent, body);
        fmt::format_to(std::back_inserter(render), "{}}}\n", indent);

        return render;
    };
};
