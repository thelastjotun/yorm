#pragma once

#include <charconv>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

namespace yorm {

// ---------------------------------------------------------------------------
//  constants
// ---------------------------------------------------------------------------
static constexpr size_t max_numeric_buffer_size = 32;

// ---------------------------------------------------------------------------
//  KeyViewList
// ---------------------------------------------------------------------------
using KeyViewList = std::span<const std::pair<std::string_view, std::string_view>>;

// ---------------------------------------------------------------------------
//  KeyPacker
// ---------------------------------------------------------------------------
template<typename T>
struct KeyPacker
{
    std::array<char, max_numeric_buffer_size> buffer;
    std::string_view view;

    constexpr KeyPacker(std::string_view val) noexcept
        : view{val}
    {}

    constexpr KeyPacker(const std::string &val) noexcept
        : view{val}
    {}

    explicit KeyPacker(T val) noexcept
        requires(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
    {
        auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), val);
        if (ec == std::errc{}) [[likely]] {
            view = std::string_view(buffer.data(), ptr - buffer.data());
        }
    }

    explicit KeyPacker(bool val) noexcept
        : view{val ? "true" : "false"}
    {}
};

template<typename T>
KeyPacker(T) -> KeyPacker<T>;

// ---------------------------------------------------------------------------
//  from_string_view
// ---------------------------------------------------------------------------
template<typename T>
inline T from_string_view(std::string_view str) noexcept
{
    if constexpr (std::is_same_v<T, std::string_view>) {
        return str;
    } else if constexpr (std::is_same_v<T, std::string>) {
        return std::string(str);
    } else if constexpr (std::is_same_v<T, bool>) {
        return (str == "true" || str == "1");
    } else {
        T result{};
        if (!str.empty()) [[likely]] {
            std::from_chars(str.data(), str.data() + str.size(), result);
        }
        return result;
    }
}

} // namespace yorm
