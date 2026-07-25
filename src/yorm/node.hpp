#pragma once

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace yorm {

template<typename T>
std::string to_string(const T &val)
{
    if constexpr (std::is_same_v<T, std::string>) {
        return val;
    } else {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }
}

class TransactionalNode
{
public:
    virtual ~TransactionalNode() = default;

    virtual bool is_added() const = 0;
    virtual bool is_deleted() const = 0;
    virtual bool is_changed() const = 0;

    virtual bool is_child_added(const std::string &child_name) const = 0;
    virtual bool is_child_deleted(const std::string &child_name) const = 0;
    virtual bool is_child_changed(const std::string &child_name) const = 0;
};

class DataNode
{
public:
    virtual ~DataNode() = default;

    virtual std::string get_child_value(const std::string &child_name) const = 0;
    virtual std::shared_ptr<DataNode> get_container(const std::string &name) const = 0;
    virtual std::vector<std::shared_ptr<DataNode>> get_list_items(const std::string &name) const = 0;

    virtual void set_child_value(const std::string &child_name, const std::string &value) = 0;
    virtual void delete_child(const std::string &child_name) = 0;
    virtual bool has_child(const std::string &name) const = 0;
    virtual void create_container(const std::string &name) = 0;

    virtual std::shared_ptr<DataNode> add_list_item(const std::string &name, const std::vector<std::pair<std::string, std::string>> &keys)
        = 0;
    virtual void delete_list_item(const std::string &name, const std::vector<std::pair<std::string, std::string>> &keys) = 0;

    virtual std::vector<std::string> get_leaflist_values(const std::string &name) const = 0;
    virtual void add_leaflist_item(const std::string &name, const std::string &value) = 0;
    virtual void delete_leaflist_item(const std::string &name, const std::string &value) = 0;

    virtual std::shared_ptr<DataNode> execute_rpc(const std::string &rpc_name) = 0;
};

class Node
{
protected:
    std::shared_ptr<DataNode> data_node_;

public:
    explicit Node(std::shared_ptr<DataNode> data_node)
        : data_node_{std::move(data_node)}
    {
        if (!data_node_) {
            throw std::invalid_argument("yorm::Node cannot be constructed with null DataNode");
        }
    }

    std::shared_ptr<DataNode> get_data_node() const { return data_node_; }

protected:
    // --- Leafs ---
    template<typename T>
    T get_leaf_value(const std::string &child_name) const
    {
        std::string str_val = data_node_->get_child_value(child_name);

        if constexpr (std::is_same_v<T, std::string>) {
            return str_val;
        } else {
            T result;
            std::stringstream ss(str_val);
            ss >> result;
            return result;
        }
    }

    template<typename T>
    void set_leaf_value(const std::string &child_name, const T &value)
    {
        if constexpr (std::is_same_v<T, std::string>) {
            data_node_->set_child_value(child_name, value);
        } else {
            std::stringstream ss;
            ss << value;
            data_node_->set_child_value(child_name, ss.str());
        }
    }

    void delete_leaf(const std::string &child_name) { data_node_->delete_child(child_name); }

    // --- Containers ---
    template<typename T>
    T get_container(const std::string &name) const
    {
        return T{data_node_->get_container(name)};
    }

    void create_container(const std::string &name) { data_node_->create_container(name); }
    
    void delete_container(const std::string &name) { data_node_->delete_child(name); }
    
    // --- Common ---
    bool has_child(const std::string &name) const { return data_node_->has_child(name); }

    // --- Lists ---
    template<typename T>
    std::vector<T> get_list_items(const std::string &name) const
    {
        auto data_node_list = data_node_->get_list_items(name);

        std::vector<T> result;
        result.reserve(data_node_list.size());

        for (const auto &item : data_node_list) {
            result.emplace_back(item);
        }

        return result;
    }

    template<typename T>
    T add_list_item(const std::string &name, const std::vector<std::pair<std::string, std::string>> &keys)
    {
        return T{data_node_->add_list_item(name, keys)};
    }

    void delete_list_item(const std::string &name, const std::vector<std::pair<std::string, std::string>> &keys)
    {
        data_node_->delete_list_item(name, keys);
    }

    // --- Leaf-lists ---
    template<typename T>
    std::vector<T> get_leaflist(const std::string &name) const
    {
        auto raw_values = data_node_->get_leaflist_values(name);
        std::vector<T> result;
        result.reserve(raw_values.size());
        for (const auto &str_val : raw_values) {
            if constexpr (std::is_same_v<T, std::string>) {
                result.push_back(str_val);
            } else {
                T val;
                std::stringstream ss(str_val);
                ss >> val;
                result.push_back(val);
            }
        }
        return result;
    }

    template<typename T>
    void add_leaflist_item(const std::string &name, const T &value)
    {
        data_node_->add_leaflist_item(name, yorm::to_string(value));
    }

    template<typename T>
    void delete_leaflist_item(const std::string &name, const T &value)
    {
        data_node_->delete_leaflist_item(name, yorm::to_string(value));
    }

    template<typename T>
    void set_leaflist(const std::string &name, const std::vector<T> &values)
    {
        data_node_->delete_child(name);
        for (const auto &v : values) {
            data_node_->add_leaflist_item(name, yorm::to_string(v));
        }
    }
};

} // namespace yorm
