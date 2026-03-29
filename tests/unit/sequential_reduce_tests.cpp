#include <cstdint>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "core/sequential_reduce.hpp"

namespace {

struct throwing_op final {
    int limit = 3;
    int calls = 0;

    int operator()(int a, int b) {
        ++calls;
        if (calls >= limit) {
            throw std::runtime_error("boom");
        }
        return a + b;
    }
};

} // namespace

TEST(sequential_reduce_tests, empty_range_returns_init) {
    const std::vector<std::int64_t> data;
    const auto result = my_reduce::core::sequential_reduce(data.begin(), data.end(), 42LL);
    EXPECT_EQ(result, 42LL);
}

TEST(sequential_reduce_tests, one_element) {
    const std::vector<int> data{7};
    const auto result = my_reduce::core::sequential_reduce(data.begin(), data.end(), 10);
    EXPECT_EQ(result, 17);
}

TEST(sequential_reduce_tests, many_elements_plus) {
    const std::vector<int> data{1, 2, 3, 4, 5};
    const auto result = my_reduce::core::sequential_reduce(data.begin(), data.end(), 0);
    EXPECT_EQ(result, 15);
}

TEST(sequential_reduce_tests, custom_binary_op) {
    const std::vector<int> data{1, 2, 3, 4};
    const auto result = my_reduce::core::sequential_reduce(
        data.begin(), data.end(), 2, std::multiplies<>{});
    EXPECT_EQ(result, 48);
}

TEST(sequential_reduce_tests, matches_std_accumulate) {
    const std::vector<std::int64_t> data{10, -5, 12, 99, -7, 3, 1, 8};
    const auto expected = std::accumulate(data.begin(), data.end(), 100LL);
    const auto actual = my_reduce::core::sequential_reduce(data.begin(), data.end(), 100LL);
    EXPECT_EQ(actual, expected);
}

TEST(sequential_reduce_tests, propagates_exception_from_op) {
    const std::vector<int> data{1, 2, 3, 4, 5};
    throwing_op op{};
    EXPECT_THROW(
        (void)my_reduce::core::sequential_reduce(data.begin(), data.end(), 0, op),
        std::runtime_error);
}