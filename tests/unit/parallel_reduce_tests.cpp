#include <cmath>
#include <cstdint>
#include <forward_list>
#include <functional>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "execution.hpp"
#include "reduce.hpp"
#include "thread_pool.hpp"

namespace {

template <typename T>
bool almost_equal(T a, T b) {
    if constexpr (std::is_floating_point_v<T>) {
        const T diff = std::fabs(a - b);
        const T scale = std::max<T>({T(1), std::fabs(a), std::fabs(b)});
        return diff <= (std::numeric_limits<T>::epsilon() * T(128) * scale);
    } else {
        return a == b;
    }
}

my_reduce::reduce_config small_parallel_cfg(std::size_t threads = 4) {
    my_reduce::reduce_config cfg{};
    cfg.thread_count       = threads;
    cfg.parallel_threshold = 1;
    cfg.min_block_size     = 1;
    cfg.debug_checks       = true;
    cfg.pool = std::make_shared<my_reduce::thread_pool>(threads);
    return cfg;
}

struct throwing_op final {
    int bad_value = 777;

    int operator()(int a, int b) const {
        if (a == bad_value || b == bad_value) {
            throw std::runtime_error("bad element");
        }
        return a + b;
    }
};

} // namespace

TEST(parallel_reduce_tests, empty_range_returns_init) {
    const std::vector<int> data;
    auto cfg = small_parallel_cfg();
    const auto result = my_reduce::my_reduce(my_reduce::par(cfg), data.begin(), data.end(), 42);
    EXPECT_EQ(result, 42);
}

TEST(parallel_reduce_tests, matches_accumulate_on_int_vector) {
    std::vector<std::int64_t> data(100000);
    std::iota(data.begin(), data.end(), 1LL);

    auto cfg = small_parallel_cfg(4);

    const auto expected = std::accumulate(data.begin(), data.end(), 0LL);
    const auto actual   = my_reduce::my_reduce(my_reduce::par(cfg), data.begin(), data.end(), 0LL);

    EXPECT_EQ(actual, expected);
}

TEST(parallel_reduce_tests, matches_sequential_reduce_on_random_ints) {
    std::mt19937 rng(123);
    std::uniform_int_distribution<int> dist(-1000, 1000);

    std::vector<int> data(50000);
    for (auto& x : data) {
        x = dist(rng);
    }

    auto cfg = small_parallel_cfg(6);

    const auto expected = my_reduce::my_reduce(my_reduce::seq(),    data.begin(), data.end(), 0);
    const auto actual   = my_reduce::my_reduce(my_reduce::par(cfg), data.begin(), data.end(), 0);

    EXPECT_EQ(actual, expected);
}

TEST(parallel_reduce_tests, works_with_forward_iterators) {
    std::forward_list<int> data{1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto cfg = small_parallel_cfg(3);

    const auto actual = my_reduce::my_reduce(my_reduce::par(cfg), data.begin(), data.end(), 0);
    EXPECT_EQ(actual, 45);
}

TEST(parallel_reduce_tests, works_with_custom_associative_op) {
    const std::vector<int> data{1, 2, 3, 4};
    auto cfg = small_parallel_cfg(2);

    const auto actual = my_reduce::my_reduce(
        my_reduce::par(cfg), data.begin(), data.end(), 2, std::multiplies<>{});

    EXPECT_EQ(actual, 48);
}

TEST(parallel_reduce_tests, propagates_exception_from_worker_task) {
    const std::vector<int> data{1, 2, 777, 4, 5};
    auto cfg = small_parallel_cfg(3);

    EXPECT_THROW(
        (void)my_reduce::my_reduce(my_reduce::par(cfg),
                                   data.begin(),
                                   data.end(),
                                   0,
                                   throwing_op{}),
        std::runtime_error);
}

TEST(parallel_reduce_tests, double_result_is_reasonable_against_long_double_reference) {
    std::vector<double> data;
    data.reserve(100000);

    for (int i = 0; i < 100000; ++i)
    {
        data.push_back(std::sin(static_cast<double>(i) * 0.001));
    }

    auto cfg = small_parallel_cfg(8);

    const double seq_res =
        my_reduce::my_reduce(my_reduce::seq(), data.begin(), data.end(), 0.0);

    const double par_res =
        my_reduce::my_reduce(my_reduce::par(cfg), data.begin(), data.end(), 0.0);

    const long double ref =
        std::accumulate(data.begin(), data.end(), 0.0L,
                        [](long double acc, double x)
                        {
                            return acc + static_cast<long double>(x);
                        });

    EXPECT_TRUE(std::isfinite(seq_res));
    EXPECT_TRUE(std::isfinite(par_res));

    const long double seq_err = std::fabs(static_cast<long double>(seq_res) - ref);
    const long double par_err = std::fabs(static_cast<long double>(par_res) - ref);

    EXPECT_LT(seq_err, 1e-6L);
    EXPECT_LT(par_err, 1e-6L);
}