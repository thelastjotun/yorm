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

template <typename T>
concept IsYormNode = requires(T node) {
    { node.get_data_node() } -> std::same_as<void*>;
    { node.get_driver() } -> std::convertible_to<DataDriver*>;
};

template <IsYormNode T>
class tx
{
    TxDriver *tx_driver_;
    void *node_;

public:
    explicit tx(const T &node)
        : node_(node.get_data_node())
    {
        tx_driver_ = node.get_driver()->get_tx_driver();
        if (!tx_driver_) [[unlikely]] {
            throw std::runtime_error("yorm::tx: DataDriver does not support transactions!");
        }
    }

    bool is_added() const { return tx_driver_->is_added(node_); }
    bool is_deleted() const { return tx_driver_->is_deleted(node_); }
    bool is_changed() const { return tx_driver_->is_changed(node_); }
};

} // namespace yorm
