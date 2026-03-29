#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <execution>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "execution.hpp"
#include "reduce.hpp"
#include "thread_pool.hpp"

namespace
{

using clock_type = std::chrono::steady_clock;

struct options final
{
    std::string type           = "int64";
    std::size_t threads        = my_reduce::default_thread_count();
    std::size_t min_block_size = 1u << 12;
};

void print_usage(const char *argv0)
{
    std::cerr
        << "Usage:\n"
        << "  " << argv0 << " --type int64|double [--threads N] [--min-block-size N]\n\n"
        << "Input format (stdin):\n"
        << "  N\n"
        << "  x1 x2 x3 ... xN\n\n"
        << "Examples:\n"
        << "  " << argv0 << " --type int64 --threads 8 < test.dat\n"
        << "  " << argv0 << " --type double --threads 8 --min-block-size 4096 < test.dat\n";
}

options parse_args(int argc, char **argv)
{
    options opts{};

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];

        auto require_value = [&](const char *name) -> std::string
        {
            if (i + 1 >= argc)
                throw std::runtime_error(std::string("missing value for ") + name);

            return argv[++i];
        };

        if (arg == "--type")
        {
            opts.type = require_value("--type");

            if (opts.type != "int64" && opts.type != "double")
                throw std::runtime_error("invalid --type, expected int64 or double");
        }
        else if (arg == "--threads")
        {
            opts.threads = static_cast<std::size_t>(std::stoull(require_value("--threads")));

            if (opts.threads == 0)
                opts.threads = 1;
        }
        else if (arg == "--min-block-size")
        {
            opts.min_block_size =
                static_cast<std::size_t>(std::stoull(require_value("--min-block-size")));

            if (opts.min_block_size == 0)
                opts.min_block_size = 1;
        }
        else if (arg == "--help" || arg == "-h")
        {
            print_usage(argv[0]);
            std::exit(0);
        }
        else
            throw std::runtime_error("unknown argument: " + arg);
    }

    return opts;
}

template <typename T>
std::vector<T> read_input(std::istream &in)
{
    std::size_t n = 0;
    if (!(in >> n))
        throw std::runtime_error("failed to read N");

    std::vector<T> data;
    data.reserve(n);

    for (std::size_t i = 0; i < n; ++i)
    {
        T value{};
        if (!(in >> value))
            throw std::runtime_error("failed to read input value #" + std::to_string(i));

        data.push_back(value);
    }

    return data;
}

template <typename T>
bool almost_equal(T a, T b)
{
    if constexpr (std::is_floating_point_v<T>)
    {
        const T diff  = std::fabs(a - b);
        const T scale = std::max<T>({T(1), std::fabs(a), std::fabs(b)});

        return diff <= (std::numeric_limits<T>::epsilon() * T(128) * scale);
    }
    else
        return a == b;
}

struct bench_result final
{
    std::string algorithm;
    long long   time_ns = 0;
    std::string value_str;
};

template <typename F>
bench_result run_benchmark(const std::string &name, F &&fn)
{
    using result_type = std::decay_t<decltype(fn())>;

    const auto  start  = clock_type::now();
    result_type value  = fn();
    const auto  finish = clock_type::now();

    const auto ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();

    std::ostringstream oss;
    oss << std::setprecision(17) << value;

    return bench_result{name, ns, oss.str()};
}

void print_results_human(const options &opts,
                         std::size_t size,
                         const std::vector<bench_result> &results)
{
    std::cout << "type="            << opts.type
              << " size="           << size
              << " threads="        << opts.threads
              << " min_block_size=" << opts.min_block_size
              << "\n\n";

    std::cout << std::left
              << std::setw(18) << "algorithm"
              << std::setw(16) << "time_ns"
              << "result\n";

    for (const auto &r : results)
    {
        std::cout << std::left
                  << std::setw(18) << r.algorithm
                  << std::setw(16) << r.time_ns
                  << r.value_str   << '\n';
    }
}

template <typename T>
int run_typed(const options &opts)
{
    const std::vector<T> data     = read_input<T>(std::cin);
    const T              expected = std::accumulate(data.begin(), data.end(), T{});

    my_reduce::reduce_config cfg{};
    cfg.thread_count   = opts.threads;
    cfg.min_block_size = opts.min_block_size;
    cfg.debug_checks   = false;
    cfg.pool           = std::make_shared<my_reduce::thread_pool>(cfg.thread_count);

    std::vector<bench_result> results;
    results.reserve(5);

    results.push_back(run_benchmark("std_accumulate", [&]() -> T
    {
        return std::accumulate(data.begin(), data.end(), T{});
    }));

    results.push_back(run_benchmark("std_reduce_seq", [&]() -> T
    {
        return std::reduce(data.begin(), data.end(), T{});
    }));

    results.push_back(run_benchmark("my_reduce_seq", [&]() -> T
    {
        return my_reduce::my_reduce(my_reduce::seq(), data.begin(), data.end(), T{});
    }));

    results.push_back(run_benchmark("my_reduce_par", [&]() -> T
    {
        return my_reduce::my_reduce(my_reduce::par(cfg), data.begin(), data.end(), T{});
    }));

    results.push_back(run_benchmark("std_reduce_par", [&]() -> T
    {
        return std::reduce(std::execution::par, data.begin(), data.end(), T{});
    }));

    for (const auto &r : results)
    {
        std::istringstream iss(r.value_str);
        T                  value{};
        iss >> value;

        if (!almost_equal(value, expected))
            throw std::runtime_error("benchmark correctness check failed for " + r.algorithm);
    }

    print_results_human(opts, data.size(), results);
    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    try
    {
        const options opts = parse_args(argc, argv);

        if (opts.type == "int64")
            return run_typed<std::int64_t>(opts);

        if (opts.type == "double")
            return run_typed<double>(opts);

        std::cerr << "ERROR: unsupported type\n";
        return 1;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "ERROR: " << ex.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "ERROR: unknown exception\n";
        return 1;
    }
}

// ./build/reduce_bench --type int64 --threads 8 < tests/e2e/generated/test_100.dat
// ./build/reduce_bench --type double --threads 8 --min-block-size 4096 < tests/e2e/generated/test_double.dat

// ./build/reduce_bench --type int64 --threads 8 < tests/e2e/generated/test_100.dat
// ./build/reduce_bench --type double --threads 8 --min-block-size 4096 < tests/e2e/generated/test_double.dat