#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

#include "detail/partition.hpp"

TEST(partition_tests, empty_input_gives_no_chunks) {
    const auto parts = my_reduce::detail::make_partitions(0, 4, 1);
    EXPECT_TRUE(parts.empty());
}

TEST(partition_tests, one_element_one_chunk) {
    const auto parts = my_reduce::detail::make_partitions(1, 4, 1);
    ASSERT_EQ(parts.size(), 1u);
    EXPECT_EQ(parts[0].begin, 0u);
    EXPECT_EQ(parts[0].end, 1u);
}

TEST(partition_tests, covers_full_range_without_gaps) {
    const std::size_t n = 1000;
    const auto parts = my_reduce::detail::make_partitions(n, 7, 1);

    std::size_t cursor = 0;
    std::size_t total = 0;

    for (const auto& p : parts) {
        EXPECT_EQ(p.begin, cursor);
        EXPECT_GE(p.end, p.begin);
        total += p.size();
        cursor = p.end;
    }

    EXPECT_EQ(cursor, n);
    EXPECT_EQ(total, n);
}

TEST(partition_tests, chunk_sizes_are_balanced) {
    const auto parts = my_reduce::detail::make_partitions(100, 6, 1);
    ASSERT_FALSE(parts.empty());

    std::size_t min_size = parts.front().size();
    std::size_t max_size = parts.front().size();

    for (const auto& p : parts) {
        min_size = std::min(min_size, p.size());
        max_size = std::max(max_size, p.size());
    }

    EXPECT_LE(max_size - min_size, 1u);
}

TEST(partition_tests, respects_min_block_size) {
    const auto parts = my_reduce::detail::make_partitions(100, 100, 16);
    EXPECT_LE(parts.size(), 100u / 16u + 1u);
}

TEST(partition_tests, no_empty_chunks_for_non_empty_input) {
    const auto parts = my_reduce::detail::make_partitions(17, 8, 1);
    for (const auto& p : parts) {
        EXPECT_FALSE(p.empty());
    }
}