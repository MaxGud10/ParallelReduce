#pragma once

#include <functional>
#include <utility>

#include "core/parallel_reduce.hpp"
#include "core/sequential_reduce.hpp"
#include "core/threshold.hpp"
#include "execution.hpp"

namespace my_reduce
{

template <typename It, typename T, typename BinaryOp>
T my_reduce(seq_policy, It first, It last, T init, BinaryOp op)
{
    return core::sequential_reduce(first, last, std::move(init), std::move(op));
}

template <typename It, typename T>
T my_reduce(seq_policy, It first, It last, T init)
{
    return core::sequential_reduce(first, last, std::move(init), std::plus<>{});
}

template <typename It, typename T, typename BinaryOp>
T my_reduce(par_policy policy, It first, It last, T init, BinaryOp op)
{
    return core::parallel_reduce(first,
                                 last,
                                 std::move(init),
                                 std::move(op),
                                 policy.config);
}

template <typename It, typename T>
T my_reduce(par_policy policy, It first, It last, T init)
{
    return core::parallel_reduce(first,
                                 last,
                                 std::move(init),
                                 std::plus<>{},
                                 policy.config);
}

template <typename It, typename T, typename BinaryOp>
T my_reduce(It first, It last, T init, BinaryOp op)
{
    return core::sequential_reduce(first, last, std::move(init), std::move(op));
}

template <typename It, typename T>
T my_reduce(It first, It last, T init)
{
    return core::sequential_reduce(first, last, std::move(init), std::plus<>{});
}

} // namespace my_reduce