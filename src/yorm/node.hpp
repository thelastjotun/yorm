#pragma once

#include "common.hpp"

#include <charconv>
#include <functional>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace yorm {

enum class RangeType { List, LeafList };

template<typename T, RangeType Type = RangeType::List>
class Range;

template<typename T>
using ListRange = Range<T>;

template<typename T>
using LeafListRange = Range<T, RangeType::LeafList>;

// ---------------------------------------------------------------------------
//  DataDriver
// ---------------------------------------------------------------------------
class TxDriver;

class DataDriver
{
public:
    virtual ~DataDriver() = default;

    virtual std::optional<std::string_view> get_child_value(void *parent, std::string_view child_name, std::string_view ns) const = 0;
    virtual void *get_child_node(void *parent, std::string_view child_name, std::string_view ns) const = 0;
    virtual void set_child_value(void *parent, std::string_view child_name, std::string_view value, std::string_view ns) = 0;
    virtual void delete_child(void *parent, std::string_view child_name, std::string_view ns) = 0;
    virtual bool has_child(void *parent, std::string_view name, std::string_view ns) const = 0;

    virtual void *get_container(void *parent, std::string_view name, std::string_view ns) const = 0;
    virtual void *create_container(void *parent, std::string_view name, std::string_view ns) = 0;

    virtual void *add_list_item(void *parent, std::string_view name, KeyViewList keys, std::string_view ns) = 0;
    virtual void delete_list_item(void *parent, std::string_view name, KeyViewList keys, std::string_view ns) = 0;

    virtual void add_leaflist_item(void *parent, std::string_view name, std::string_view value, std::string_view ns) = 0;
    virtual void delete_leaflist_item(void *parent, std::string_view name, std::string_view value, std::string_view ns) = 0;

    virtual void *get_range_first(void *parent, std::string_view name, std::string_view ns) const = 0;
    virtual void *get_range_next(void *parent, void *current_node, std::string_view name, std::string_view ns) const = 0;
    virtual std::string_view get_node_value(void *node) const = 0;

    virtual void *execute_rpc(void *parent, std::string_view rpc_name, std::string_view ns) = 0;
    virtual void free_node(void *node) = 0;

    virtual TxDriver *get_tx_driver() { return nullptr; }
};

// ---------------------------------------------------------------------------
//  Node
// ---------------------------------------------------------------------------
class Node
{
protected:
    DataDriver *driver_;
    void *data_node_;

public:
    explicit Node(DataDriver &driver)
        : driver_{&driver}
        , data_node_{nullptr}
    {}

    explicit Node(DataDriver *driver, void *data_node = nullptr)
        : driver_{driver}
        , data_node_{data_node}
    {
        if (!driver_) [[unlikely]] {
            throw std::invalid_argument("yorm::Node cannot be constructed with null DataDriver");
        }
    }

    DataDriver *get_driver() const { return driver_; }
    void *get_data_node() const { return data_node_; }

private:
    template<typename Func, typename T, typename... Args>
    inline void write_value_helper(Func &&driver_method, std::string_view name, std::string_view ns, T &&value, Args &&...extra_args)
    {
        using raw_t = std::decay_t<T>;

        if constexpr (std::is_same_v<raw_t, std::string_view> || std::is_same_v<raw_t, std::string>) {
            std::invoke(driver_method, driver_, data_node_, name, std::forward<T>(value), ns, std::forward<Args>(extra_args)...);
        } else if constexpr (std::is_same_v<raw_t, bool>) {
            std::invoke(driver_method, driver_, data_node_, name, value ? "true" : "false", ns, std::forward<Args>(extra_args)...);
        } else {
            char buf[max_numeric_buffer_size];
            auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), value);
            if (ec == std::errc{}) [[likely]] {
                std::invoke(driver_method, driver_, data_node_, name, std::string_view(buf, ptr - buf), ns, std::forward<Args>(extra_args)...);
            }
        }
    }

protected:
    // --- Leafs ---
    template<typename T>
    std::optional<T> get_optional_leaf_value(std::string_view child_name, std::string_view ns = "") const
    {
        if (!data_node_ || !driver_) [[unlikely]] {
            return std::nullopt;
        }

        auto val = driver_->get_child_value(data_node_, child_name, ns);
        if (!val) {
            return std::nullopt;
        }

        return from_string_view<T>(*val);
    }

    template<typename T>
    T get_leaf_value(std::string_view child_name, std::string_view ns = "") const
    {
        auto val = get_optional_leaf_value<T>(child_name, ns);
        return val ? *val : T{};
    }

    template<typename T>
    void set_leaf_value(std::string_view child_name, const T &value, std::string_view ns = "")
    {
        write_value_helper(&DataDriver::set_child_value, child_name, ns, value);
    }

    void delete_leaf(std::string_view child_name, std::string_view ns = "") { driver_->delete_child(data_node_, child_name, ns); }

    // --- Containers ---
    template<typename T>
    T get_container(std::string_view name, std::string_view ns = "") const
    {
        return T{driver_, driver_->get_container(data_node_, name, ns)};
    }

    void create_container(std::string_view name, std::string_view ns = "") { driver_->create_container(data_node_, name, ns); }

    void delete_container(std::string_view name, std::string_view ns = "") { driver_->delete_child(data_node_, name, ns); }

    // --- Common ---
    bool has_child(std::string_view name, std::string_view ns = "") const { return driver_->has_child(data_node_, name, ns); }

    // --- Lists ---
    template<typename T>
    ListRange<T> get_list_items(std::string_view name, std::string_view ns = "") const
    {
        return ListRange<T>{driver_, data_node_, name, ns};
    }

    template<typename T>
    T add_list_item(std::string_view name, KeyViewList keys, std::string_view ns = "")
    {
        return T{driver_, driver_->add_list_item(data_node_, name, keys, ns)};
    }

    void delete_list_item(std::string_view name, KeyViewList keys, std::string_view ns = "")
    {
        driver_->delete_list_item(data_node_, name, keys, ns);
    }

    // --- Leaf-lists ---
    template<typename T>
    LeafListRange<T> get_leaflist(std::string_view name, std::string_view ns = "") const
    {
        return LeafListRange<T>{driver_, data_node_, name, ns};
    }

    template<typename T>
    void add_leaflist_item(std::string_view name, const T &value, std::string_view ns = "")
    {
        write_value_helper(&DataDriver::add_leaflist_item, name, ns, value);
    }

    template<typename T>
    void delete_leaflist_item(std::string_view name, const T &value, std::string_view ns = "")
    {
        write_value_helper(&DataDriver::delete_leaflist_item, name, ns, value);
    }

    template<typename T>
    void set_leaflist(std::string_view name, std::span<const T> values, std::string_view ns = "")
    {
        driver_->delete_child(data_node_, name, ns);
        for (const auto &value : values) {
            write_value_helper(&DataDriver::add_leaflist_item, name, ns, value);
        }
    }
};

// ---------------------------------------------------------------------------
//  Range
// ---------------------------------------------------------------------------
template<typename T, RangeType Type>
class Range final
{
private:
    DataDriver *driver_;
    void *parent_node_;
    std::string_view name_;
    std::string_view ns_;

public:
    class Iterator
    {
    private:
        DataDriver *driver_;
        void *parent_node_;
        std::string_view name_;
        std::string_view ns_;
        void *current_;

    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = T;

        Iterator() = default;
        Iterator(DataDriver *driver, void *parent_node, std::string_view name, std::string_view ns, void *node)
            : driver_(driver)
            , parent_node_(parent_node)
            , name_(name)
            , ns_(ns)
            , current_(node)
        {}

        inline reference operator*() const
        {
            if constexpr (Type == RangeType::LeafList) {
                return from_string_view<T>(driver_->get_node_value(current_));
            } else {
                return T{driver_, current_};
            }
        }

        Iterator &operator++()
        {
            current_ = driver_->get_range_next(parent_node_, current_, name_, ns_);
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            current_ = driver_->get_range_next(parent_node_, current_, name_, ns_);
            return tmp;
        }

        bool operator==(const Iterator &other) const { return current_ == other.current_; }
        bool operator!=(const Iterator &other) const { return current_ != other.current_; }
    };

    Range(DataDriver *driver, void *parent_node, std::string_view name, std::string_view ns = "")
        : driver_(driver)
        , parent_node_(parent_node)
        , name_(name)
        , ns_(ns)
    {}

    Iterator begin() const { return Iterator(driver_, parent_node_, name_, ns_, driver_->get_range_first(parent_node_, name_, ns_)); }
    Iterator end() const { return Iterator(driver_, parent_node_, name_, ns_, nullptr); }
};

// ---------------------------------------------------------------------------
} // namespace yorm
