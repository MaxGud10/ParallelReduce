#pragma once

#include <algorithm>
#include <cstddef>

#include "../execution.hpp"

namespace my_reduce
{

namespace core
{

inline bool should_run_parallel(std::size_t total_size,
                                const reduce_config &cfg) noexcept
{
    if (total_size == 0)
        return false;

    if (cfg.thread_count <= 1)
        return false;

    if (cfg.min_block_size == 0)
        return false;

    if (total_size < cfg.parallel_threshold)
        return false;

    if (total_size < (cfg.min_block_size * 2))
        return false;

    return true;
}

inline std::size_t choose_worker_count(std::size_t total_size,
                                       const reduce_config &cfg) noexcept
{
    if (!should_run_parallel(total_size, cfg))
        return 1;

    std::size_t workers = std::max<std::size_t>(1, cfg.thread_count);
    workers             = std::min(workers, total_size);

    const std::size_t max_by_block =
        std::max<std::size_t>(1, total_size / cfg.min_block_size);

    workers = std::min(workers, max_by_block);
    workers = std::max<std::size_t>(1, workers);

    return workers;
}

inline std::size_t choose_block_size(std::size_t total_size,
                                     std::size_t worker_count) noexcept
{
    if (worker_count == 0)
        return total_size;

    return (total_size + worker_count - 1) / worker_count;
}

} // namespace core

} // namespace my_reduce