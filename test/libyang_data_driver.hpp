#pragma once

#include "yorm/node.hpp"
#include "yorm/tx.hpp"
#include <iostream>
#include <string>
#include <string_view>

extern "C" {
#include <libyang.h>
}

namespace yorm::test {

class LibyangDataDriver : public yorm::DataDriver, public yorm::TxDriver
{
private:
    ly_ctx *ctx_;
    lyd_node **root_;

    lyd_node *find_child(lyd_node *parent, std::string_view name, std::string_view ns) const
    {
        lyd_node *siblings = parent ? lyd_child(parent) : (root_ ? *root_ : nullptr);
        for (lyd_node *iter = siblings; iter; iter = iter->next) {
            if (iter->schema && name == iter->schema->name) {
                if (!ns.empty()) {
                    if (ns == iter->schema->module->name) {
                        return iter;
                    }
                } else {
                    return iter;
                }
            }
        }
        return nullptr;
    }

public:
    explicit LibyangDataDriver(ly_ctx *ctx, lyd_node **root)
        : ctx_(ctx)
        , root_(root)
    {}

    std::optional<std::string_view> get_child_value(void *parent, std::string_view child_name, std::string_view ns) const override
    {
        lyd_node *child = find_child(static_cast<lyd_node *>(parent), child_name, ns);
        if (child && child->schema->nodetype & (LYS_LEAF | LYS_LEAFLIST)) {
            const char *val = lyd_get_value(child);
            return val ? std::optional<std::string_view>(val) : std::nullopt;
        }
        return "";
    }

    void set_child_value(void *parent, std::string_view child_name, std::string_view value, std::string_view ns) override
    {
        lyd_node *parent_node = static_cast<lyd_node *>(parent);

        std::string path;
        if (parent == nullptr) {
            path += "/";
        }

        if (!ns.empty()) {
            path += ns;
            path += ":";
        }

        path += child_name;
        lyd_node *new_node = nullptr;
        lyd_node *tree_ctx = parent_node ? parent_node : (root_ ? *root_ : nullptr);
        lyd_new_path(tree_ctx, ctx_, path.c_str(), std::string(value).c_str(), LYD_NEW_PATH_UPDATE, &new_node);
        if (root_ && !*root_) {
            *root_ = new_node;
        } else if (root_ && *root_) {
            *root_ = lyd_first_sibling(*root_);
        }
    }

    bool has_child(void *parent, std::string_view name, std::string_view ns) const override
    {
        return find_child(static_cast<lyd_node *>(parent), name, ns) != nullptr;
    }

    void delete_child(void *parent, std::string_view child_name, std::string_view ns) override
    {
        lyd_node *parent_node = static_cast<lyd_node *>(parent);

        while (true) {
            lyd_node *child = find_child(parent_node, child_name, ns);
            if (!child) {
                break;
            }
            if (root_ && *root_ == child) {
                *root_ = child->next;
            }
            lyd_free_tree(child);
        }
    }

    void *get_container(void *parent, std::string_view name, std::string_view ns) const override
    {
        return find_child(static_cast<lyd_node *>(parent), name, ns);
    }

    void *create_container(void *parent, std::string_view name, std::string_view ns) override
    {
        lyd_node *parent_node = static_cast<lyd_node *>(parent);

        std::string path;
        if (parent == nullptr) {
            path += "/";
        }

        if (!ns.empty()) {
            path += ns;
            path += ":";
        }

        path += name;
        lyd_node *new_node = nullptr;
        lyd_node *tree_ctx = parent_node ? parent_node : (root_ ? *root_ : nullptr);
        lyd_new_path(tree_ctx, ctx_, path.c_str(), nullptr, 0, &new_node);
        if (root_ && !*root_) {
            *root_ = new_node;
        } else if (root_ && *root_) {
            *root_ = lyd_first_sibling(*root_);
        }

        return find_child(parent_node, name, ns);
    }

    void *add_list_item(void *parent, std::string_view name, yorm::KeyViewList keys, std::string_view ns) override
    {
        lyd_node *parent_node = static_cast<lyd_node *>(parent);

        std::string path;
        if (parent == nullptr) {
            path += "/";
        }
        if (!ns.empty()) {
            path += ns;
            path += ":";
        }
        path += name;

        for (const auto &key_val : keys) {
            path += "[";
            path += key_val.first;
            path += "='";
            path += key_val.second;
            path += "']";
        }

        lyd_node *new_node = nullptr;
        lyd_node *tree_ctx = parent_node ? parent_node : (root_ ? *root_ : nullptr);
        lyd_new_path(tree_ctx, ctx_, path.c_str(), nullptr, 0, &new_node);
        if (root_ && !*root_) {
            *root_ = new_node;
        } else if (root_ && *root_) {
            *root_ = lyd_first_sibling(*root_);
        }

        lyd_node *match = nullptr;
        lyd_node *search_start = parent_node ? parent_node : (root_ ? *root_ : nullptr);
        lyd_find_path(search_start, path.c_str(), 0, &match);
        return match;
    }

    void delete_list_item(void *parent, std::string_view name, yorm::KeyViewList keys, std::string_view ns) override
    {
        lyd_node *parent_node = static_cast<lyd_node *>(parent);
        std::string path;

        if (!parent_node && !*root_) {
            return;
        }

        if (!parent_node) {
            path += "/";
        }
        if (!ns.empty()) {
            path += ns;
            path += ":";
        }
        path += name;

        for (const auto &key_val : keys) {
            path += "[";
            path += key_val.first;
            path += "='";
            path += key_val.second;
            path += "']";
        }

        lyd_node *match = nullptr;
        lyd_node *search_start = parent_node ? parent_node : (root_ ? *root_ : nullptr);
        if (lyd_find_path(search_start, path.c_str(), 0, &match) == LY_SUCCESS && match) {
            if (root_ && *root_ == match) {
                *root_ = match->next;
            }
            lyd_free_tree(match);
        }
    }

    void add_leaflist_item(void *parent, std::string_view name, std::string_view value, std::string_view ns) override
    {
        lyd_node *parent_node = static_cast<lyd_node *>(parent);

        std::string path;
        if (parent == nullptr) {
            path += "/";
        }

        if (!ns.empty()) {
            path += ns;
            path += ":";
        }

        path += name;

        lyd_node *new_node = nullptr;

        // If parent is null (top-level), but we have a root, pass root to lyd_new_path so it knows which tree to update.
        // It's safe because path is absolute (starts with /).
        lyd_node *tree_ctx = parent_node ? parent_node : (root_ ? *root_ : nullptr);

        LY_ERR err = lyd_new_path(tree_ctx, ctx_, path.c_str(), std::string(value).c_str(), 0, &new_node);
        if (err != LY_SUCCESS) {
            std::cout << "lyd_new_path failed with error: " << err << " for path: " << path << std::endl;
        }

        if (root_ && !*root_) {
            *root_ = new_node;
        } else if (root_ && *root_) {
            // lyd_new_path might insert before root if ordered, adjust root to point to first sibling
            *root_ = lyd_first_sibling(*root_);
        }
    }

    void delete_leaflist_item(void *parent, std::string_view name, std::string_view value, std::string_view ns) override
    {
        lyd_node *parent_node = static_cast<lyd_node *>(parent);
        if (parent_node == nullptr) {
            parent_node = root_ ? *root_ : nullptr;
        }

        if (parent_node == nullptr) {
            return;
        }

        std::string path;
        if (parent == nullptr) {
            path += "/";
        }

        if (!ns.empty()) {
            path += ns;
            path += ":";
        }

        path += name;
        path += "[.='";
        path += value;
        path += "']";

        lyd_node *match = nullptr;
        if (lyd_find_path(parent_node, path.c_str(), 0, &match) == LY_SUCCESS && match) {
            if (root_ && *root_ == match) {
                *root_ = match->next;
            }
            lyd_free_tree(match);
        }
    }

    void *get_range_first(void *parent, std::string_view name, std::string_view ns) const override
    {
        return find_child(static_cast<lyd_node *>(parent), name, ns);
    }

    void *get_range_next(void *parent, void *current_node, std::string_view name, std::string_view ns) const override
    {
        if (current_node == nullptr) {
            return nullptr;
        }

        lyd_node *current = static_cast<lyd_node *>(current_node);

        for (lyd_node *iter = current->next; iter; iter = iter->next) {
            if (iter->schema && name == iter->schema->name) {
                if (!ns.empty()) {
                    if (ns == iter->schema->module->name) {
                        return iter;
                    }
                } else {
                    return iter;
                }
            }
        }

        return nullptr;
    }

    std::string_view get_node_value(void *node) const override
    {
        if (node == nullptr) {
            return "";
        }

        lyd_node *current = static_cast<lyd_node *>(node);
        const char *string_value = lyd_get_value(current);

        if (string_value) {
            return std::string_view(string_value);
        }

        return "";
    }

    void *execute_rpc(void *parent, std::string_view rpc_name, std::string_view ns) override
    {
        lyd_node *output_tree = nullptr;
        std::string path = "/";
        if (!ns.empty()) {
            path += ns;
            path += ":";
        }
        path += rpc_name;

        // Mock Data because of no real backend
        lyd_new_path(nullptr, ctx_, path.c_str(), nullptr, 0, &output_tree);
        lyd_new_path(output_tree, ctx_, (path + "/success").c_str(), "true", 0x01, nullptr);

        return output_tree;
    }

    void free_node(void *node) override
    {
        if (node) {
            lyd_free_tree(static_cast<lyd_node *>(node));
        }
    }

    // Mock Data because of no real backend
    yorm::TxDriver *get_tx_driver() override { return this; }
    bool is_added(void *node) const override { return true; }
    bool is_deleted(void *node) const override { return false; }
    bool is_changed(void *node) const override { return true; }
};

} // namespace yorm::test
