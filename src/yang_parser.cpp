#include "yang_parser.hpp"
#include "codegen/cpp_class.hpp"
#include "codegen/cpp_method.hpp"
#include "codegen/sanitizer.hpp"
#include "tree_schema.h"

#include <iterator>
#include <stdexcept>

extern "C" {
#include <context.h>
}

YangParser::YangParser(const std::string &modules_dir)
    : ctx_{nullptr}
{
    if (LY_SUCCESS != ly_ctx_new(modules_dir.c_str(), 0, &ctx_)) {
        throw std::runtime_error("Failed to create libyang context!");
    }
}

YangParser::~YangParser()
{
    if (ctx_) {
        ly_ctx_destroy(ctx_);
    }
}

CppFile YangParser::parse_module(const std::string &filepath)
{
    lys_module *module = nullptr;
    if (LY_SUCCESS != lys_parse_path(ctx_, filepath.c_str(), LYS_IN_YANG, &module)) {
        throw std::runtime_error(fmt::format("Failed to parse yang module '{}'!", filepath));
    }

    CppClass parent_class{.name = sanitize_name(module->name), .base_class = "yorm::Node", .namespace_path = "yorm_gen"};

    CppFile file;
    file.includes.push_back("yorm/node.hpp");
    file.includes.push_back("cstdint");
    file.includes.push_back("span");

    // Data tree
    walk_schema_tree(parent_class, file, "yorm_gen", module->compiled->data);

    // RPC tree
    walk_schema_tree(parent_class, file, "yorm_gen", reinterpret_cast<const lysc_node *>(module->compiled->rpcs));

    file.classes.push_back(parent_class);

    return file;
}

void YangParser::walk_schema_tree(CppClass &parent_class, CppFile &file, const std::string &current_ns, const lysc_node *node)
{
    const lysc_node *current = node;
    while (current != nullptr) {
        if (current->nodetype == LYS_CONTAINER) {
            generate_container(parent_class, file, current_ns, current);
        } else if (current->nodetype == LYS_LIST) {
            generate_list(parent_class, file, current_ns, reinterpret_cast<const lysc_node_list *>(current));
        } else if (current->nodetype == LYS_LEAF) {
            generate_leaf(parent_class, reinterpret_cast<const lysc_node_leaf *>(current));
        } else if (current->nodetype == LYS_ANYDATA || current->nodetype == LYS_ANYXML) {
            generate_anydata(parent_class, reinterpret_cast<const lysc_node_anydata *>(current));
        } else if (current->nodetype == LYS_LEAFLIST) {
            generate_leaflist(parent_class, reinterpret_cast<const lysc_node_leaflist *>(current));
        } else if (current->nodetype == LYS_RPC || current->nodetype == LYS_ACTION) {
            generate_rpc(parent_class, file, current_ns, reinterpret_cast<const lysc_node_action *>(current));
        } else if (current->nodetype == LYS_CHOICE) {
            const lysc_node_choice *choice = reinterpret_cast<const lysc_node_choice *>(current);
            const lysc_node_case *case_node = choice->cases;

            while (case_node != nullptr) {
                const lysc_node *child = case_node->child;

                while (child != nullptr && child->parent == reinterpret_cast<const lysc_node *>(case_node)) {
                    if (child->nodetype == LYS_CONTAINER) {
                        generate_container(parent_class, file, current_ns, child);
                    } else if (child->nodetype == LYS_LIST) {
                        generate_list(parent_class, file, current_ns, reinterpret_cast<const lysc_node_list *>(child));
                    } else if (child->nodetype == LYS_LEAF) {
                        generate_leaf(parent_class, reinterpret_cast<const lysc_node_leaf *>(child));
                    } else if (child->nodetype == LYS_LEAFLIST) {
                        generate_leaflist(parent_class, reinterpret_cast<const lysc_node_leaflist *>(child));
                    }

                    child = child->next;
                }

                case_node = reinterpret_cast<const lysc_node_case *>(case_node->next);
            }
        }

        current = current->next;
    }
}

void YangParser::generate_container(CppClass &parent_class, CppFile &file, const std::string &current_ns, const lysc_node *container)
{
    std::string sanitized_name = sanitize_name(container->name);
    std::string ns_arg = container->parent == nullptr ? fmt::format(", \"{}\"", container->module->name) : "";

    CppClass container_class{.name = sanitized_name, .base_class = "yorm::Node", .namespace_path = current_ns};

    parent_class.fields.push_back({sanitized_name, container->name});

    std::string full_return_type = current_ns.empty() ? sanitized_name : fmt::format("{}::{}", current_ns, sanitized_name);
    parent_class.methods.emplace_back(
        CppMethod{
            .return_type = full_return_type,
            .signature = fmt::format("get_{}() const", sanitized_name),
            .body = fmt::format("return get_container<{}>(\"{}\"{});", full_return_type, container->name, ns_arg)});

    parent_class.methods.emplace_back(
        CppMethod{
            .return_type = "bool",
            .signature = fmt::format("has_{}() const", sanitized_name),
            .body = fmt::format("return has_child(\"{}\"{});", container->name, ns_arg)});

    parent_class.methods.emplace_back(
        CppMethod{
            .return_type = "void",
            .signature = fmt::format("create_{}()", sanitized_name),
            .body = fmt::format("create_container(\"{}\"{});", container->name, ns_arg)});

    parent_class.methods.emplace_back(
        CppMethod{
            .return_type = "void",
            .signature = fmt::format("delete_{}()", sanitized_name),
            .body = fmt::format("delete_container(\"{}\"{});", container->name, ns_arg)});

    std::string next_ns = current_ns.empty() ? fmt::format("{}_ns", sanitized_name) : fmt::format("{}::{}_ns", current_ns, sanitized_name);
    if (const lysc_node *child = lysc_node_child(container)) {
        walk_schema_tree(container_class, file, next_ns, child);
    }

    file.classes.push_back(container_class);
}

void YangParser::generate_list(CppClass &parent_class, CppFile &file, const std::string &current_ns, const lysc_node_list *list)
{
    std::string sanitized_name = sanitize_name(list->name);
    std::string ns_arg = list->parent == nullptr ? fmt::format(", \"{}\"", list->module->name) : "";

    CppClass list_class{.name = sanitized_name, .base_class = "yorm::Node", .namespace_path = current_ns};

    parent_class.fields.push_back({sanitized_name, list->name});

    std::string full_return_type = current_ns.empty() ? sanitized_name : fmt::format("{}::{}", current_ns, sanitized_name);
    parent_class.methods.emplace_back(
        CppMethod{
            .return_type = fmt::format("yorm::ListRange<{}>", full_return_type),
            .signature = fmt::format("get_{}_list() const", sanitized_name),
            .body = fmt::format("return get_list_items<{}>(\"{}\"{});", full_return_type, list->name, ns_arg)});

    std::vector<const lysc_node_leaf *> keys;
    for (lysc_node *child_node = list->child; child_node != nullptr; child_node = child_node->next) {
        if (lysc_is_key(child_node)) {
            keys.push_back(reinterpret_cast<const lysc_node_leaf *>(child_node));
        }
    }

    if (!keys.empty()) {
        std::string args_signature;
        std::string packers_body;
        std::string keys_vector_body
            = fmt::format("\t\tstd::array<std::pair<std::string_view, std::string_view>, {}> keys = {{{{\n", keys.size());

        for (size_t i = 0; i < keys.size(); ++i) {
            const lysc_node_leaf *key_leaf = keys[i];
            std::string key_sinitized_name = sanitize_name(key_leaf->name);

            fmt::format_to(std::back_inserter(args_signature), "{} {}", get_cpp_type(key_leaf->type), key_sinitized_name);
            if (i < keys.size() - 1) {
                fmt::format_to(std::back_inserter(args_signature), ", ");
            }

            fmt::format_to(
                std::back_inserter(packers_body), "auto _p_{} = yorm::KeyPacker({});\n\t\t", key_sinitized_name, key_sinitized_name);

            fmt::format_to(std::back_inserter(keys_vector_body), "\t\t\t{{\"{}\", _p_{}.view}}", key_leaf->name, key_sinitized_name);
            if (i < keys.size() - 1) {
                fmt::format_to(std::back_inserter(keys_vector_body), ",\n");
            }
        }
        fmt::format_to(std::back_inserter(keys_vector_body), "\n\t\t}}}};\n");

        std::string full_setup_body = fmt::format("{}\n{}", packers_body, keys_vector_body);

        parent_class.methods.emplace_back(
            CppMethod{
                .return_type = full_return_type,
                .signature = fmt::format("add_{}({})", sanitized_name, args_signature),
                .body
                = fmt::format("{}\t\treturn add_list_item<{}>(\"{}\", keys{});", full_setup_body, full_return_type, list->name, ns_arg)});

        parent_class.methods.emplace_back(
            CppMethod{
                .return_type = "void",
                .signature = fmt::format("delete_{}({})", sanitized_name, args_signature),
                .body = fmt::format("{}\t\tdelete_list_item(\"{}\", keys{});", full_setup_body, list->name, ns_arg)});
    }

    std::string next_ns = current_ns.empty() ? fmt::format("{}_ns", sanitized_name) : fmt::format("{}::{}_ns", current_ns, sanitized_name);
    if (const lysc_node *child = lysc_node_child(reinterpret_cast<const lysc_node *>(list))) {
        walk_schema_tree(list_class, file, next_ns, child);
    }

    file.classes.push_back(list_class);
}

void YangParser::generate_leaflist(CppClass &parent_class, const lysc_node_leaflist *leaflist)
{
    std::string ns_arg = leaflist->parent == nullptr ? fmt::format(", \"{}\"", leaflist->module->name) : "";

    parent_class.fields.push_back({sanitize_name(leaflist->name), leaflist->name});

    parent_class.methods.emplace_back(
        CppMethod{
            .return_type = fmt::format("yorm::LeafListRange<{}>", get_cpp_type(leaflist->type)),
            .signature = fmt::format("get_{}() const", sanitize_name(leaflist->name)),
            .body = fmt::format("return get_leaflist<{}>(\"{}\"{});", get_cpp_type(leaflist->type), leaflist->name, ns_arg)});

    parent_class.methods.emplace_back(
        CppMethod{
            .return_type = "void",
            .signature = fmt::format("add_{}({} value)", sanitize_name(leaflist->name), get_cpp_type(leaflist->type)),
            .body = fmt::format("add_leaflist_item(\"{}\", value{});", leaflist->name, ns_arg)});

    parent_class.methods.emplace_back(
        CppMethod{
            .return_type = "void",
            .signature = fmt::format("delete_{}({} value)", sanitize_name(leaflist->name), get_cpp_type(leaflist->type)),
            .body = fmt::format("delete_leaflist_item(\"{}\", value{});", leaflist->name, ns_arg)});

    parent_class.methods.emplace_back(
        CppMethod{
            .return_type = "void",
            .signature = fmt::format("set_{}(std::span<const {}> values)", sanitize_name(leaflist->name), get_cpp_type(leaflist->type)),
            .body = fmt::format("set_leaflist(\"{}\", values{});", leaflist->name, ns_arg)});
}

void YangParser::generate_leaf(CppClass &parent_class, const lysc_node_leaf *leaf)
{
    std::string ns_arg = leaf->parent == nullptr ? fmt::format(", \"{}\"", leaf->module->name) : "";

    parent_class.fields.push_back({sanitize_name(leaf->name), leaf->name});

    if (leaf->type->basetype == LY_TYPE_ENUM) {
        const lysc_type_enum *enum_type = reinterpret_cast<const lysc_type_enum *>(leaf->type);
        std::string enum_name = sanitize_name(leaf->name) + "_enum";

        CppEnum cpp_enum{.name = enum_name};

        uint64_t count = LY_ARRAY_COUNT(enum_type->enums);
        for (uint64_t i = 0; i < count; ++i) {
            std::string val_name = enum_type->enums[i].name;
            std::string sanitized = sanitize_name(val_name);
            std::transform(sanitized.begin(), sanitized.end(), sanitized.begin(), ::toupper);
            cpp_enum.values.push_back({.yang_name = val_name, .cpp_name = sanitized});
        }

        parent_class.enums.push_back(std::move(cpp_enum));

        parent_class.methods.emplace_back(
            CppMethod{
                .return_type = enum_name,
                .signature = fmt::format("get_{}() const", sanitize_name(leaf->name)),
                .body = fmt::format("return string_view_to_{}(get_leaf_value<std::string_view>(\"{}\"{}));", enum_name, leaf->name, ns_arg)});

        if (!(leaf->flags & LYS_CONFIG_R)) {
            parent_class.methods.emplace_back(
                CppMethod{
                    .return_type = "void",
                    .signature = fmt::format("set_{}({} value)", sanitize_name(leaf->name), enum_name),
                    .body = fmt::format("set_leaf_value(\"{}\", to_string_view(value){});", leaf->name, ns_arg)});

            parent_class.methods.emplace_back(
                CppMethod{
                    .return_type = "void",
                    .signature = fmt::format("delete_{}()", sanitize_name(leaf->name)),
                    .body = fmt::format("delete_leaf(\"{}\"{});", leaf->name, ns_arg)});
        }
    } else {
        parent_class.methods.emplace_back(
            CppMethod{
                .return_type = get_cpp_type(leaf->type),
                .signature = fmt::format("get_{}() const", sanitize_name(leaf->name)),
                .body = fmt::format("return get_leaf_value<{}>(\"{}\"{});", get_cpp_type(leaf->type), leaf->name, ns_arg)});

        if (!(leaf->flags & LYS_CONFIG_R)) {
            parent_class.methods.emplace_back(
                CppMethod{
                    .return_type = "void",
                    .signature = fmt::format("set_{}({} value)", sanitize_name(leaf->name), get_cpp_type(leaf->type)),
                    .body = fmt::format("set_leaf_value(\"{}\", value{});", leaf->name, ns_arg)});

            parent_class.methods.emplace_back(
                CppMethod{
                    .return_type = "void",
                    .signature = fmt::format("delete_{}()", sanitize_name(leaf->name)),
                    .body = fmt::format("delete_leaf(\"{}\"{});", leaf->name, ns_arg)});
        }
    }
}

void YangParser::generate_anydata(CppClass &parent_class, const lysc_node_anydata *anydata)
{
    std::string ns_arg = anydata->parent == nullptr ? fmt::format(", \"{}\"", anydata->module->name) : "";

    parent_class.fields.push_back({sanitize_name(anydata->name), anydata->name});

    parent_class.methods.emplace_back(
        CppMethod{
            .return_type = "std::string_view",
            .signature = fmt::format("get_{}() const", sanitize_name(anydata->name)),
            .body = fmt::format("return get_leaf_value<std::string_view>(\"{}\"{});", anydata->name, ns_arg)});

    if (!(anydata->flags & LYS_CONFIG_R)) {
        parent_class.methods.emplace_back(
            CppMethod{
                .return_type = "void",
                .signature = fmt::format("set_{}(std::string_view value)", sanitize_name(anydata->name)),
                .body = fmt::format("set_leaf_value(\"{}\", value{});", anydata->name, ns_arg)});

        parent_class.methods.emplace_back(
            CppMethod{
                .return_type = "void",
                .signature = fmt::format("delete_{}()", sanitize_name(anydata->name)),
                .body = fmt::format("delete_leaf(\"{}\"{});", anydata->name, ns_arg)});
    }
}

void YangParser::generate_rpc(CppClass &parent_class, CppFile &file, const std::string &current_ns, const lysc_node_action *rpc)
{
    std::string rpc_name = sanitize_name(rpc->name);
    std::string ns_arg = rpc->parent == nullptr ? fmt::format(", \"{}\"", rpc->module->name) : "";

    std::string next_ns = current_ns.empty() ? fmt::format("{}_ns", rpc_name) : fmt::format("{}::{}_ns", current_ns, rpc_name);

    CppClass output_class{.name = "output", .base_class = "yorm::Node", .namespace_path = next_ns, .is_rpc_output = true};

    std::string full_input_type = current_ns.empty() ? fmt::format("{}_ns::input", rpc_name)
                                                     : fmt::format("{}::{}_ns::input", current_ns, rpc_name);
    parent_class.methods.emplace_back(
        CppMethod{
            .return_type = full_input_type,
            .signature = fmt::format("create_{}_rpc()", rpc_name),
            .body
            = fmt::format("return {}{{driver_, driver_->create_container(data_node_, \"{}\"{})}};", full_input_type, rpc->name, ns_arg)});

    if (const lysc_node *child = rpc->output.child) {
        walk_schema_tree(output_class, file, next_ns, child);
    }
    file.classes.push_back(output_class);

    CppClass input_class{.name = "input", .base_class = "yorm::Node", .namespace_path = next_ns};

    if (const lysc_node *child = rpc->input.child) {
        walk_schema_tree(input_class, file, next_ns, child);
    }

    input_class.methods.emplace_back(
        CppMethod{
            .return_type = "output",
            .signature = "execute()",
            .body = fmt::format("return output(driver_, driver_->execute_rpc(data_node_, \"{}\"{}));", rpc->name, ns_arg)});

    file.classes.push_back(input_class);
}
