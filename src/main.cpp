#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "execution.hpp"
#include "reduce.hpp"
#include "thread_pool.hpp"

namespace
{

struct options final
{
    std::string mode           = "seq";
    std::string type           = "int64";
    std::size_t threads        = my_reduce::default_thread_count();
    std::size_t min_block_size = 1u << 12;
};

void print_usage(const char *argv0)
{
    std::cerr
        << "Usage:\n"
        << "  " << argv0 << " --mode seq|par --type int64|double [--threads N] [--min-block-size N]\n\n"
        << "Input format (stdin):\n"
        << "  N\n"
        << "  x1 x2 x3 ... xN\n\n"
        << "Examples:\n"
        << "  " << argv0 << " --mode seq --type int64 < test.dat\n"
        << "  " << argv0 << " --mode par --type int64 --threads 8 < test.dat\n";
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

        if (arg == "--mode")
        {
            opts.mode = require_value("--mode");

            if (opts.mode != "seq" && opts.mode != "par")
                throw std::runtime_error("invalid --mode, expected seq or par");
        }
        else if (arg == "--type")
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
int run_typed(const options &opts)
{
    const std::vector<T> data = read_input<T>(std::cin);

    T result{};

    if (opts.mode == "seq")
        result = my_reduce::my_reduce(my_reduce::seq(), data.begin(), data.end(), T{});
    else
    {
        my_reduce::reduce_config cfg{};
        cfg.thread_count   = opts.threads;
        cfg.min_block_size = opts.min_block_size;
        cfg.debug_checks   = false;
        cfg.pool           = std::make_shared<my_reduce::thread_pool>(cfg.thread_count);

        result = my_reduce::my_reduce(my_reduce::par(std::move(cfg)),
                                      data.begin(),
                                      data.end(),
                                      T{});
    }

    std::cout << std::setprecision(17) << result << '\n';
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
