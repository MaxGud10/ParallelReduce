#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>

#include "../detail/cacheline.hpp"
#include "../detail/partition.hpp"

namespace my_reduce
{

namespace debug
{

inline void verify_partitions(const std::vector<detail::chunk> &chunks,
                              std::size_t total_size)
{
    std::size_t cursor = 0;

    for (const auto &ch : chunks)
    {
        if (ch.end < ch.begin)
            throw std::logic_error("invalid partition: end < begin");

        if (ch.begin != cursor)
            throw std::logic_error("invalid partition: gap or overlap detected");

        cursor = ch.end;
    }

    if (cursor != total_size)
        throw std::logic_error("invalid partition: total coverage mismatch");
}

template <typename T>
void verify_partials(const std::vector<detail::cacheline_padded<std::optional<T>>> &partials,
                     const std::vector<detail::chunk> &chunks)
{
    if (partials.size() != chunks.size())
        throw std::logic_error("partials/chunks size mismatch");

    for (std::size_t i = 0; i < chunks.size(); ++i)
    {
        if (!chunks[i].empty() && !partials[i].value.has_value())
            throw std::logic_error("missing partial result for non-empty chunk");
    }
}

} // namespace debug

} // namespace my_reduce