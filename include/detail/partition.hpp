#pragma once

#include <cstddef>
#include <vector>
#include <algorithm>

namespace my_reduce::detail
{

struct chunk final
{
    std::size_t begin = 0;
    std::size_t end   = 0;

    [[nodiscard]] std::size_t size()  const noexcept {return end - begin;}
    [[nodiscard]] bool        empty() const noexcept {return begin == end;}
};

inline std::vector<chunk> make_partitions(std::size_t total_size,
                                          std::size_t desired_chunks,
                                          std::size_t min_block_size = 1)
{
    std::vector<chunk> result;

    if (total_size == 0 || desired_chunks == 0)
        return result;

    if (min_block_size == 0)
        min_block_size = 1;

    std::size_t chunk_count = std::min(desired_chunks, total_size);

    const std::size_t max_chunks_by_block =
        std::max<std::size_t>(1, total_size / min_block_size);

    chunk_count = std::min(chunk_count, max_chunks_by_block);
    chunk_count = std::max<std::size_t>(1, chunk_count);

    result.reserve(chunk_count);

    const std::size_t base  = total_size / chunk_count;
    const std::size_t extra = total_size % chunk_count;

    std::size_t cursor = 0;
    for (std::size_t i = 0; i < chunk_count; ++i)
    {
        const std::size_t current_size = base + (i < extra ? 1u : 0u);
        result.push_back(chunk{cursor, cursor + current_size});
        cursor += current_size;
    }

    return result;
}

} // namespace my_reduce::detail