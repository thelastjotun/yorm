#pragma once

#include "node.hpp"
#include <memory>
#include <stdexcept>
#include <string>

namespace yorm {

class TxWrapper
{
    std::shared_ptr<TransactionalNode> tx_node_;

public:
    explicit TxWrapper(const Node &node)
    {
        tx_node_ = std::dynamic_pointer_cast<TransactionalNode>(node.get_data_node());

        if (!tx_node_) {
            throw std::runtime_error("This node does not support transactions.");
        }
    }

    bool is_added() const { return tx_node_->is_added(); }
    bool is_deleted() const { return tx_node_->is_deleted(); }
    bool is_changed() const { return tx_node_->is_changed(); }

    bool is_child_added(const std::string &child_name) const { return tx_node_->is_child_added(child_name); }
    bool is_child_deleted(const std::string &child_name) const { return tx_node_->is_child_deleted(child_name); }
    bool is_child_changed(const std::string &child_name) const { return tx_node_->is_child_changed(child_name); }
};

inline TxWrapper tx(const Node &node)
{
    return TxWrapper(node);
}

} // namespace yorm
