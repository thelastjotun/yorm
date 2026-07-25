#pragma once

#include <fmt/format.h>
#include <string>
#include <vector>

struct CppEnum final
{
    std::string name;

    struct EnumValue
    {
        std::string yang_name;
        std::string cpp_name;
    };
    std::vector<EnumValue> values;

    std::string render(int indent_level = 0) const
    {
        std::string rendered;
        std::string indent(indent_level, '\t');

        fmt::format_to(std::back_inserter(rendered), "{}enum class {} {{\n", indent, name);
        for (const auto &val : values) {
            fmt::format_to(std::back_inserter(rendered), "{}\t{},\n", indent, val.cpp_name);
        }
        fmt::format_to(std::back_inserter(rendered), "{}}};\n\n", indent);

        fmt::format_to(std::back_inserter(rendered), "{}static std::string to_string({} val) {{\n", indent, name);
        fmt::format_to(std::back_inserter(rendered), "{}\tswitch(val) {{\n", indent);
        for (const auto &val : values) {
            fmt::format_to(std::back_inserter(rendered), "{}\t\tcase {}::{}: return \"{}\";\n", indent, name, val.cpp_name, val.yang_name);
        }
        fmt::format_to(std::back_inserter(rendered), "{}\t\tdefault: return \"\";\n", indent);
        fmt::format_to(std::back_inserter(rendered), "{}\t}}\n", indent);
        fmt::format_to(std::back_inserter(rendered), "{}}}\n\n", indent);

        fmt::format_to(std::back_inserter(rendered), "{}static {} string_to_{}(const std::string& str) {{\n", indent, name, name);
        for (const auto &val : values) {
            fmt::format_to(std::back_inserter(rendered), "{}\tif (str == \"{}\") return {}::{};\n", indent, val.yang_name, name, val.cpp_name);
        }
        fmt::format_to(std::back_inserter(rendered), "{}\tthrow std::invalid_argument(\"Invalid enum value for {}\");\n", indent, name);
        fmt::format_to(std::back_inserter(rendered), "{}}}\n", indent);

        return rendered;
    }
};
