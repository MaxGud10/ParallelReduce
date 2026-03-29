#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "../debug/verify.hpp"
#include "../detail/cacheline.hpp"
#include "../detail/partition.hpp"
#include "../execution.hpp"
#include "../thread_pool.hpp"
#include "sequential_reduce.hpp"
#include "threshold.hpp"

namespace my_reduce
{

namespace detail
{

template <typename It>
std::vector<std::pair<It, It>>
make_iterator_ranges(It first, const std::vector<chunk> &chunks)
{
    using difference_type = typename std::iterator_traits<It>::difference_type;

    std::vector<std::pair<It, It>> ranges;
    ranges.reserve(chunks.size());

    It current = first;
    for (const auto &ch : chunks)
    {
        It begin_it = current;
        std::advance(current, static_cast<difference_type>(ch.size()));
        It end_it   = current;
        ranges.emplace_back(begin_it, end_it);
    }

    return ranges;
}

} // namespace detail

namespace core
{

template <typename It, typename T, typename BinaryOp>
T parallel_reduce(It first, It last, T init, BinaryOp op, const reduce_config &cfg)
{
    using iterator_category = typename std::iterator_traits<It>::iterator_category;

    static_assert(std::is_base_of_v<std::forward_iterator_tag, iterator_category>,
                  "parallel_reduce requires at least forward iterators");
    static_assert(std::is_copy_constructible_v<BinaryOp>,
                  "parallel_reduce requires a copy-constructible binary op");

    const std::size_t total_size =
        static_cast<std::size_t>(std::distance(first, last));

    if (total_size == 0)
        return init;

    if (!should_run_parallel(total_size, cfg))
        return sequential_reduce(first, last, std::move(init), op);

    std::size_t worker_count = choose_worker_count(total_size, cfg);

    std::unique_ptr<thread_pool> owned_pool;
    thread_pool                 *pool_ptr = nullptr;

    if (cfg.pool)
    {
        pool_ptr = cfg.pool.get();

        if (pool_ptr->size() > 0)
            worker_count = std::min(worker_count, pool_ptr->size());
    }
    else
    {
        owned_pool = std::make_unique<thread_pool>(worker_count);
        pool_ptr   = owned_pool.get();
    }

    if (worker_count <= 1)
        return sequential_reduce(first, last, std::move(init), op);

    const auto chunks = detail::make_partitions(total_size,
                                                worker_count,
                                                cfg.min_block_size);

    if (chunks.size() <= 1)
        return sequential_reduce(first, last, std::move(init), op);
    if (cfg.debug_checks)
        debug::verify_partitions(chunks, total_size);

    const auto ranges = detail::make_iterator_ranges(first, chunks);

    std::vector<detail::cacheline_padded<std::optional<T>>> partials(chunks.size());

    for (std::size_t i = 0; i < ranges.size(); ++i)
    {
        const auto range   = ranges[i];
        auto       op_copy = op;

        pool_ptr->submit([&, i, range, op_copy]() mutable
        {
            auto       chunk_first = range.first;
            const auto chunk_last  = range.second;

            if (chunk_first == chunk_last)
                return;

            T local = static_cast<T>(*chunk_first);
            ++chunk_first;

            local = sequential_reduce(chunk_first, chunk_last, std::move(local), op_copy);
            partials[i].value.emplace(std::move(local));
        });
    }

    pool_ptr->wait();

    if (cfg.debug_checks)
        debug::verify_partials(partials, chunks);

    T result = std::move(init);
    for (auto &slot : partials)
    {
        if (slot.value.has_value())
            result = std::invoke(op, std::move(result), *slot.value);
    }

    return result;
}

template <typename It, typename T>
T parallel_reduce(It first, It last, T init, const reduce_config &cfg)
{
    return parallel_reduce(first, last, std::move(init), std::plus<>{}, cfg);
}

} // namespace core

} // namespace my_reduce