#pragma once

#include <functional>
#include <utility>

#include "../detail/unroll.hpp"

namespace my_reduce::core
{

template <typename It, typename T, typename BinaryOp>
T sequential_reduce(It first, It last, T init, BinaryOp op)
{
    return detail::reduce_unrolled<4>(first, last, std::move(init), op);
}

template <typename It, typename T>
T sequential_reduce(It first, It last, T init)
{
    return sequential_reduce(first, last, std::move(init), std::plus<>{});
}

} // namespace my_reduce::core