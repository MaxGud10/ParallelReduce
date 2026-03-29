#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace my_reduce
{

namespace detail
{

inline constexpr std::size_t cacheline_size = 64;

template <typename T>
struct alignas(cacheline_size) cacheline_padded final
{
    T value;

    cacheline_padded() = default;

    template <typename... Args>
    explicit cacheline_padded(Args&&... args)
        : value(std::forward<Args>(args)...) {}
};

static_assert(alignof(cacheline_padded<int>) >= cacheline_size,
              "cacheline_padded must be aligned to cacheline_size");

} // namespace detail

} // namespace my_reduce