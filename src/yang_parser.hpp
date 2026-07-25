#pragma once

#include <string>
#include <tree_schema.h>

#include "codegen/cpp_class.hpp"
#include "codegen/cpp_file.hpp"

struct ly_ctx;
struct lysc_node;

class YangParser final
{
    ly_ctx *ctx_;

public:
    explicit YangParser(const std::string &modules_dir);
    ~YangParser();

    YangParser(const YangParser &other) = delete;
    YangParser operator=(const YangParser &other) = delete;
    YangParser(YangParser &&other) = delete;
    YangParser operator=(YangParser &&other) = delete;

    CppFile parse_module(const std::string &filepath);

private:
    void walk_schema_tree(CppClass &parent_class, CppFile &file, const std::string &current_ns, const lysc_node *node);

    void generate_container(CppClass &parent_class, CppFile &file, const std::string &current_ns, const lysc_node *container);
    void generate_list(CppClass &parent_class, CppFile &file, const std::string &current_ns, const lysc_node_list *list);

    void generate_leaflist(CppClass &parent_class, const lysc_node_leaflist *list);
    void generate_leaf(CppClass &parent_class, const lysc_node_leaf *leaf);

    void generate_rpc(CppClass &parent_class, CppFile &file, const std::string &current_ns, const lysc_node_action *rpc);
};
