#pragma once

#include <cstddef>
#include <memory>
#include <thread>
#include <utility>

namespace my_reduce {

class thread_pool;

inline std::size_t default_thread_count() noexcept
{
    const auto hc = std::thread::hardware_concurrency();
    return hc == 0 ? 1u : static_cast<std::size_t>(hc);
}

struct reduce_config final
{
    std::size_t thread_count = default_thread_count();
    std::size_t parallel_threshold = 1u << 15; // 32768
    std::size_t min_block_size     = 1u << 12; // 4096
    bool debug_checks = false;
    std::shared_ptr<thread_pool> pool{};
};

struct seq_policy final {};

struct par_policy final
{
    reduce_config config{};

    explicit par_policy(reduce_config cfg = {}) noexcept
        : config(std::move(cfg))
    {
        if (config.thread_count == 0)
            config.thread_count = 1;
        if (config.min_block_size == 0)
            config.min_block_size = 1;
    }
};

constexpr seq_policy seq() noexcept {return {};}
inline    par_policy par()          {return par_policy{};}

inline par_policy par(std::size_t thread_count)
{
    reduce_config cfg{};
    cfg.thread_count = (thread_count == 0 ? 1u : thread_count);
    return par_policy{std::move(cfg)};
}

inline par_policy par(reduce_config cfg)
{
    return par_policy{std::move(cfg)};
}

} // namespace my_reduce