#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "thread_pool.hpp"

TEST(thread_pool_tests, executes_all_tasks) {
    my_reduce::thread_pool pool(4);

    std::atomic<int> counter{0};

    for (int i = 0; i < 1000; ++i) {
        pool.submit([&counter] {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    pool.wait();
    EXPECT_EQ(counter.load(std::memory_order_relaxed), 1000);
}

TEST(thread_pool_tests, wait_rethrows_task_exception) {
    my_reduce::thread_pool pool(4);

    pool.submit([] {});
    pool.submit([] { throw std::runtime_error("task failed"); });
    pool.submit([] {});

    EXPECT_THROW(pool.wait(), std::runtime_error);
}

TEST(thread_pool_tests, pool_can_be_reused_after_exception_was_observed) {
    my_reduce::thread_pool pool(2);

    pool.submit([] { throw std::runtime_error("boom"); });
    EXPECT_THROW(pool.wait(), std::runtime_error);

    std::atomic<int> counter{0};
    for (int i = 0; i < 10; ++i) {
        pool.submit([&counter] {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    EXPECT_NO_THROW(pool.wait());
    EXPECT_EQ(counter.load(std::memory_order_relaxed), 10);
}

TEST(thread_pool_tests, many_small_tasks) {
    my_reduce::thread_pool pool(8);

    std::atomic<int> sum{0};

    for (int i = 0; i < 5000; ++i) {
        pool.submit([&sum, i] {
            sum.fetch_add(i % 3, std::memory_order_relaxed);
        });
    }

    EXPECT_NO_THROW(pool.wait());
    EXPECT_GT(sum.load(std::memory_order_relaxed), 0);
}