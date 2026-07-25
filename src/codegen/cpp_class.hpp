#pragma once

#include "cpp_enum.hpp"
#include "cpp_method.hpp"

#include <fmt/format.h>
#include <iterator>
#include <string>
#include <vector>

struct CppClass final
{
    std::string name;
    std::string base_class;
    std::vector<CppMethod> methods;
    std::string namespace_path;
    std::vector<CppEnum> enums;
    std::vector<std::pair<std::string, std::string>> fields;

    std::string render(int indent_level = 0) const
    {
        std::string rendered;
        std::string indent(indent_level, '\t');

        bool has_namespace = !namespace_path.empty();
        if (has_namespace) {
            fmt::format_to(std::back_inserter(rendered), "{}namespace {} {{\n\n", indent, namespace_path);
        }

        fmt::format_to(std::back_inserter(rendered), "{}class {} : public {} {{\n", indent, name, base_class);
        fmt::format_to(std::back_inserter(rendered), "{}public:\n", indent);
        fmt::format_to(std::back_inserter(rendered), "{}\tusing {}::Node;\n\n", indent, base_class);

        if (!fields.empty()) {
            fmt::format_to(std::back_inserter(rendered), "{}\tstruct fields {{\n", indent);

            for (const auto &[cpp_name, yang_name] : fields) {
                fmt::format_to(std::back_inserter(rendered), "{}\t\tstatic constexpr const char* {} = \"{}\";\n", indent, cpp_name, yang_name);
            }
            fmt::format_to(std::back_inserter(rendered), "{}\t}};\n\n", indent);
        }

        for (const auto &e : enums) {
            fmt::format_to(std::back_inserter(rendered), "{}\n", e.render(1 + indent_level));
        }

        for (const auto &method : methods) {
            fmt::format_to(std::back_inserter(rendered), "{}\n", method.render(1 + indent_level));
        }

        fmt::format_to(std::back_inserter(rendered), "{}}};\n\n", indent);

        if (has_namespace) {
            fmt::format_to(std::back_inserter(rendered), "{}}} // {}\n\n", indent, namespace_path);
        }

        return rendered;
    };
};
