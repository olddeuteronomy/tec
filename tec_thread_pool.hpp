// Time-stamp: <Last changed 2026-02-20 16:04:44 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2020-2026 The Emacs Cat (https://github.com/olddeuteronomy/tec).

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
------------------------------------------------------------------------
----------------------------------------------------------------------*/
/**
 * @file tec_thread_pool.hpp
 * @brief Simple, non-stealing thread pool implementation using a single shared task queue.
 * @author The Emacs Cat
 * @date 2025-12-29
 */

#pragma once

#include <cstddef>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
#include "tec/tec_trace.hpp"


namespace tec {

/**
 * @brief Simple, non-stealing thread pool implementation using a single shared task queue.
 *
 * Classic worker thread pattern with condition variable notification.
 * Tasks are std::function<void()> objects (can wrap lambdas, functors, bound functions, etc.).
 *
 * Thread safety: all public methods are thread-safe.
 * Destruction: waits for all currently running tasks to finish (but does **not** wait
 * for tasks that are still in the queue when stop is requested).
 */
class ThreadPool {
public:
    /// Type alias for the task function objects stored and executed by the pool
    using TaskFunc = std::function<void()>;

protected:
    size_t num_threads_; ///< Number of worker threads in the pool (set during construction)
    std::vector<std::thread> workers_; ///< Container holding all worker std::thread objects
    std::queue<TaskFunc> tasks_; ///< Thread-safe queue of pending tasks (protected by queue_mutex_)
    std::mutex queue_mutex_; ///< Mutex protecting access to the tasks_ queue
    std::condition_variable condition_; ///< Condition variable used to wake up sleeping worker threads when tasks arrive
    std::atomic<bool> stop_; ///< Atomic flag used to signal all worker threads to terminate

public:
    /**
     * @brief Constructs a thread pool with the specified number of worker threads
     * @param num_threads Desired number of worker threads (usually std::thread::hardware_concurrency() or similar)
     *
     * Starts exactly `num_threads` worker threads that immediately begin waiting
     * for tasks via condition variable.
     */
    explicit ThreadPool(size_t num_threads)
        : num_threads_(num_threads)
        , stop_(false)
    {
        TEC_ENTER("ThreadPool::ThreadPool");
        //
        // Register thread workers.
        //
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    TaskFunc task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });

                        if (stop_ && tasks_.empty())
                            return;

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    if (task) {
                        task();
                    }
                }
            });
        }
        TEC_TRACE("Thread pool created with {} workers.", num_threads_);
    }

    /**
     * @brief Returns the number of worker threads in this pool
     * @return Number of threads that were requested at construction
     */
    constexpr size_t get_num_threads() {
        return num_threads_;
    }

    /**
     * @brief Destructor – gracefully shuts down the thread pool
     *
     * Sets the stop flag, wakes up all waiting threads,
     * and joins every worker thread.
     *
     * Important:
     *   - Any tasks still remaining in the queue when destruction begins
     *     are **discarded** (not executed).
     *   - Tasks that are already running on worker threads are allowed
     *     to complete normally.
     */
    virtual ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable())
                worker.join();
        }
    }

    /**
     * @brief Enqueues a new task to be executed by one of the worker threads
     * @tparam F Type of the callable (usually deduced)
     * @param task Callable object (lambda, std::bind result, function pointer + arguments, etc.)
     *
     * The task is moved into the internal queue (zero-copy when possible).
     * Notification is sent to exactly one waiting worker (if any).
     *
     * Thread-safe — can be called from any thread, including worker threads.
     *
     * @note If the pool is already stopped (being destroyed), the task is
     *       still added to the queue but will never be executed.
     */
    template<class F>
    void enqueue(F&& task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace(std::forward<F>(task));
        }
        condition_.notify_one();
    }
};

} // namespace tec
