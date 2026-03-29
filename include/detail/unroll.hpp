#pragma once

#include <cstddef>
#include <functional>
#include <utility>

namespace my_reduce::detail
{

// сохраняем левый порядок применения op
template <std::size_t Unroll = 4, typename It, typename T, typename BinaryOp>
T reduce_unrolled(It first, It last, T init, BinaryOp &op)
{
    static_assert(Unroll > 0, "Unroll factor must be > 0");

    while (first != last)
    {
        for (std::size_t i = 0; i < Unroll && first != last; ++i, ++first)
            init = std::invoke(op, std::move(init), *first);
    }

    return init;
}

} // namespace my_reduce::detail