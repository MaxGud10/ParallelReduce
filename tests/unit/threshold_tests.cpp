#include <cstddef>

#include <gtest/gtest.h>

#include "core/threshold.hpp"
#include "execution.hpp"

TEST(threshold_tests, small_input_runs_sequentially) {
    my_reduce::reduce_config cfg{};
    cfg.thread_count = 8;
    cfg.parallel_threshold = 100;
    cfg.min_block_size = 10;

    EXPECT_FALSE(my_reduce::core::should_run_parallel(50, cfg));
}

TEST(threshold_tests, one_thread_runs_sequentially) {
    my_reduce::reduce_config cfg{};
    cfg.thread_count = 1;
    cfg.parallel_threshold = 1;
    cfg.min_block_size = 1;

    EXPECT_FALSE(my_reduce::core::should_run_parallel(1000, cfg));
}

TEST(threshold_tests, large_input_runs_in_parallel) {
    my_reduce::reduce_config cfg{};
    cfg.thread_count = 8;
    cfg.parallel_threshold = 100;
    cfg.min_block_size = 10;

    EXPECT_TRUE(my_reduce::core::should_run_parallel(10'000, cfg));
}

TEST(threshold_tests, choose_worker_count_is_bounded_by_input_size) {
    my_reduce::reduce_config cfg{};
    cfg.thread_count = 100;
    cfg.parallel_threshold = 1;
    cfg.min_block_size = 1;

    EXPECT_EQ(my_reduce::core::choose_worker_count(7, cfg), 7u);
}

TEST(threshold_tests, choose_worker_count_respects_min_block_size) {
    my_reduce::reduce_config cfg{};
    cfg.thread_count = 16;
    cfg.parallel_threshold = 1;
    cfg.min_block_size = 100;

    EXPECT_EQ(my_reduce::core::choose_worker_count(250, cfg), 2u);
}

TEST(threshold_tests, choose_block_size_rounds_up) {
    EXPECT_EQ(my_reduce::core::choose_block_size(100, 6), 17u);
    EXPECT_EQ(my_reduce::core::choose_block_size(12, 3), 4u);
    EXPECT_EQ(my_reduce::core::choose_block_size(0, 3), 0u);
}