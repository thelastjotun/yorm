#pragma once

#include <concepts>
#include <stdexcept>
#include <yorm/node.hpp>

namespace yorm {

class TxDriver
{
public:
    virtual ~TxDriver() = default;
    virtual bool is_added(void *node) const = 0;
    virtual bool is_deleted(void *node) const = 0;
    virtual bool is_changed(void *node) const = 0;
};

template<typename T>
concept IsYormNode = requires(T node) {
    { node.get_data_node() } -> std::same_as<void *>;
    { node.get_driver() } -> std::convertible_to<DataDriver *>;
};

template<IsYormNode T>
class tx
{
    DataDriver *driver_;
    TxDriver *tx_driver_;
    void *node_;

public:
    explicit tx(const T &node)
        : driver_(node.get_driver())
        , node_(node.get_data_node())
    {
        tx_driver_ = driver_->get_tx_driver();
        if (!tx_driver_) [[unlikely]] {
            throw std::runtime_error("yorm::tx: DataDriver does not support transactions!");
        }
    }

    bool is_added() const { return tx_driver_->is_added(node_); }
    bool is_deleted() const { return tx_driver_->is_deleted(node_); }
    bool is_changed() const { return tx_driver_->is_changed(node_); }

    bool is_leaf_added(std::string_view leaf_name, std::string_view ns = "") const
    {
        void *child = driver_->get_child_node(node_, leaf_name, ns);
        return child ? tx_driver_->is_added(child) : false;
    }

    bool is_leaf_deleted(std::string_view leaf_name, std::string_view ns = "") const
    {
        void *child = driver_->get_child_node(node_, leaf_name, ns);
        return child ? tx_driver_->is_deleted(child) : false;
    }

    bool is_leaf_changed(std::string_view leaf_name, std::string_view ns = "") const
    {
        void *child = driver_->get_child_node(node_, leaf_name, ns);
        return child ? tx_driver_->is_changed(child) : false;
    }
};

} // namespace yorm
