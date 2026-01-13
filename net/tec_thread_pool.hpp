// Time-stamp: <Last changed 2026-01-13 15:45:52 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022-2025 The Emacs Cat (https://github.com/olddeuteronomy/tec).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------
----------------------------------------------------------------------*/
/**
 * @file tec_thread_pool.hpp
 * @brief Server thread pool implementation.
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


// Thread pool implementation
class ThreadPool {

private:
    size_t num_threads_;
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;

    size_t buffer_size_;
    std::vector<char*> buffers_;
    std::atomic<size_t> next_worker_index_;

public:

    explicit ThreadPool(size_t buffer_size, size_t num_threads)
        : num_threads_(num_threads)
        , stop_(false)
        , buffer_size_(buffer_size)
        , next_worker_index_{0}
    {
        TEC_ENTER("ThreadPool::ThreadPool");
        //
        // Allocate thread buffers.
        //
        buffers_.resize(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            buffers_[i] = new char[BUFSIZ];
        }
        //
        // Register thread workers.
        //
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
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

    char* get_buffer(size_t idx) {
        return buffers_[idx % buffers_.size()];  // safe even if idx is huge
    }

    // Helper to get next index (atomic increment + modulo)
    size_t get_next_worker_index() {
        return next_worker_index_.fetch_add(1, std::memory_order_relaxed)
             % workers_.size();
    }

    size_t get_num_threads() const {
        return num_threads_;
    }

    size_t get_buffer_size() const {
        return buffer_size_;
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable())
                worker.join();
        }
        for (char* buf : buffers_) {
            delete[] buf;
        }
    }

    // Add a new task (client handling function)
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
