#pragma once

#include "yorm/node.hpp"
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace yorm::test {

class MockDataNode : public yorm::DataNode, public std::enable_shared_from_this<MockDataNode>
{
private:
    std::string node_name_;
    std::map<std::string, std::string> leafs_;
    std::map<std::string, std::vector<std::string>> leaf_lists_;
    std::vector<std::shared_ptr<MockDataNode>> containers_;

public:
    explicit MockDataNode(std::string name = "root")
        : node_name_(std::move(name))
    {}

    std::string get_name() const { return node_name_; }

    // --- Leafs ---
    std::string get_child_value(const std::string &child_name) const override
    {
        auto it = leafs_.find(child_name);
        if (it != leafs_.end()) {
            return it->second;
        }
        return "";
    }

    void set_child_value(const std::string &child_name, const std::string &value) override { leafs_[child_name] = value; }

    // --- Containers ---
    std::shared_ptr<yorm::DataNode> get_container(const std::string &name) const override
    {
        for (const auto &container : containers_) {
            if (container->get_name() == name) {
                return container;
            }
        }
        return nullptr;
    }

    void create_container(const std::string &name) override
    {
        if (!get_container(name)) {
            containers_.push_back(std::make_shared<MockDataNode>(name));
        }
    }

    // --- Common ---
    bool has_child(const std::string &name) const override
    {
        if (leafs_.find(name) != leafs_.end()) {
            return true;
        }
        if (leaf_lists_.find(name) != leaf_lists_.end() && !leaf_lists_.at(name).empty()) {
            return true;
        }
        for (const auto &container : containers_) {
            if (container->get_name() == name) {
                return true;
            }
        }
        return false;
    }

    void delete_child(const std::string &child_name) override
    {
        leafs_.erase(child_name);
        leaf_lists_.erase(child_name);
        containers_.erase(
            std::remove_if(containers_.begin(), containers_.end(), [&](const auto &container) { return container->get_name() == child_name; }),
            containers_.end());
    }

    // --- Lists ---
    std::vector<std::shared_ptr<yorm::DataNode>> get_list_items(const std::string &name) const override
    {
        std::vector<std::shared_ptr<yorm::DataNode>> result;
        for (const auto &container : containers_) {
            if (container->get_name() == name) {
                result.push_back(container);
            }
        }
        return result;
    }

    std::shared_ptr<yorm::DataNode> add_list_item(
        const std::string &name, const std::vector<std::pair<std::string, std::string>> &keys) override
    {
        auto item = std::make_shared<MockDataNode>(name);
        for (const auto &key : keys) {
            item->set_child_value(key.first, key.second);
        }
        containers_.push_back(item);
        return item;
    }

    void delete_list_item(const std::string &name, const std::vector<std::pair<std::string, std::string>> &keys) override
    {
        containers_.erase(
            std::remove_if(
                containers_.begin(),
                containers_.end(),
                [&](const auto &container) {
                    if (container->get_name() != name) {
                        return false;
                    }
                    for (const auto &key : keys) {
                        if (container->get_child_value(key.first) != key.second) {
                            return false;
                        }
                    }
                    return true;
                }),
            containers_.end());
    }

    // --- Leaf-lists ---
    std::vector<std::string> get_leaflist_values(const std::string &name) const override
    {
        auto it = leaf_lists_.find(name);
        if (it != leaf_lists_.end()) {
            return it->second;
        }
        return {};
    }

    void add_leaflist_item(const std::string &name, const std::string &value) override { leaf_lists_[name].push_back(value); }

    void delete_leaflist_item(const std::string &name, const std::string &value) override
    {
        auto it = leaf_lists_.find(name);
        if (it != leaf_lists_.end()) {
            auto &vec = it->second;
            vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
        }
    }

    // --- RPC ---
    std::shared_ptr<yorm::DataNode> execute_rpc(const std::string &rpc_name) override { return std::make_shared<MockDataNode>("output"); }
};

} // namespace yorm::test