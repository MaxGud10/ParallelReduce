#pragma once

#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "execution.hpp"

namespace my_reduce
{

class thread_pool final
{
public:
    using task_type = std::function<void()>;

    explicit thread_pool(std::size_t thread_count = default_thread_count())
    {
        if (thread_count == 0)
            thread_count = 1;

        try
        {
            workers_.reserve(thread_count);

            for (std::size_t i = 0; i < thread_count; ++i)
                workers_.emplace_back([this] { worker_loop(); });
        }
        catch (...)
        {
            shutdown_noexcept();
            throw;
        }
    }

    ~thread_pool() noexcept
    {
        shutdown_noexcept();
    }

    thread_pool(const thread_pool &)            = delete;
    thread_pool &operator=(const thread_pool &) = delete;
    thread_pool(thread_pool &&)                 = delete;
    thread_pool &operator=(thread_pool &&)      = delete;

    [[nodiscard]] std::size_t size() const noexcept
    {
        return workers_.size();
    }

    template <typename F>
    void submit(F &&f)
    {
        static_assert(std::is_invocable_v<std::decay_t<F> &>,
                      "submitted task must be callable with no arguments");

        task_type task(std::forward<F>(f));

        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (stop_)
                throw std::runtime_error("submit() called on stopped thread_pool");

            tasks_.push(std::move(task));
            ++unfinished_tasks_;
        }

        cv_.notify_one();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        done_cv_.wait(lock, [this] { return unfinished_tasks_ == 0; });

        if (first_exception_)
        {
            auto ex = first_exception_;
            first_exception_ = nullptr;
            lock.unlock();
            std::rethrow_exception(ex);
        }
    }

    void clear_exceptions() noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        first_exception_ = nullptr;
    }

private:
    void worker_loop() noexcept
    {
        for (;;)
        {
            task_type task;

            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

                if (stop_ && tasks_.empty())
                    return;

                task = std::move(tasks_.front());
                tasks_.pop();
            }

            try
            {
                task();
            }
            catch (...)
            {
                store_exception_once(std::current_exception());
            }

            {
                std::lock_guard<std::mutex> lock(mutex_);

                if (unfinished_tasks_ > 0)
                    --unfinished_tasks_;

                if (unfinished_tasks_ == 0)
                    done_cv_.notify_all();
            }
        }
    }

    void store_exception_once(std::exception_ptr ex) noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!first_exception_)
            first_exception_ = ex;
    }

    void shutdown_noexcept() noexcept
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }

        cv_.notify_all();

        for (auto &worker : workers_)
        {
            if (worker.joinable())
                worker.join();
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<task_type>    tasks_;

    mutable std::mutex       mutex_;
    std::condition_variable  cv_;
    std::condition_variable  done_cv_;

    bool               stop_             = false;
    std::size_t        unfinished_tasks_ = 0;
    std::exception_ptr first_exception_{};
};

} // namespace my_reduce