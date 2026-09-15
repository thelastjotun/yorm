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
    bool is_rpc_output{false};

    std::string render(int indent_level = 0) const
    {
        // TODO(Future release): Add YANG constraints validation (must, when, mandatory, min-elements, max-elements)
        // Currently YORM relies entirely on the backend driver to perform these validations.

        std::string rendered;
        std::string indent(indent_level, '\t');

        bool has_namespace = !namespace_path.empty();
        if (has_namespace) {
            fmt::format_to(std::back_inserter(rendered), "{}namespace {} {{\n\n", indent, namespace_path);
        }

        fmt::format_to(std::back_inserter(rendered), "{}class {} : public {} {{\n", indent, name, base_class);

        fmt::format_to(std::back_inserter(rendered), "{}public:\n", indent);
        if (is_rpc_output) {
            fmt::format_to(
                std::back_inserter(rendered),
                "{indent}\texplicit {name}(yorm::DataDriver *driver, void *rpc_result) noexcept\n"
                "{indent}\t\t: {base} {{ driver, rpc_result }} {{}}\n\n"
                "{indent}\t{name}(const {name}&) = delete;\n"
                "{indent}\t{name}& operator=(const {name}&) = delete;\n"
                "{indent}\t{name}({name}&& other) noexcept : {base}(other.driver_, other.data_node_) {{ other.data_node_ = nullptr; }}\n"
                "{indent}\t{name}& operator=({name}&& other) noexcept {{\n"
                "{indent}\t\tif (this != &other) {{\n"
                "{indent}\t\t\tif (driver_ && data_node_) driver_->free_node(data_node_);\n"
                "{indent}\t\t\tdriver_ = other.driver_;\n"
                "{indent}\t\t\tdata_node_ = other.data_node_;\n"
                "{indent}\t\t\tother.data_node_ = nullptr;\n"
                "{indent}\t\t}}\n"
                "{indent}\t\treturn *this;\n"
                "{indent}\t}}\n"
                "{indent}\t~{name}() {{ if (driver_ && data_node_) driver_->free_node(data_node_); }}\n\n",
                fmt::arg("indent", indent),
                fmt::arg("name", name),
                fmt::arg("base", base_class));
        } else {
            fmt::format_to(std::back_inserter(rendered), "{}\tusing {}::Node;\n\n", indent, base_class);
        }

        if (!fields.empty()) {
            fmt::format_to(std::back_inserter(rendered), "{}\tstruct fields {{\n", indent);

            for (const auto& [cpp_name, yang_name] : fields) {
                fmt::format_to(std::back_inserter(rendered),
                               "{}\t\tstatic constexpr std::string_view {} = \"{}\";\n",
                               indent,
                               cpp_name,
                               yang_name);
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
