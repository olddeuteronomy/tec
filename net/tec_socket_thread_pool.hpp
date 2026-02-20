// Time-stamp: <Last changed 2026-02-20 16:27:10 by magnolia>
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
 * @file tec_socket_thread_pool.hpp
 * @brief Specialized thread pool that maintains one pre-allocated fixed-size buffer per worker thread.
 * @author The Emacs Cat
 * @date 2025-12-29
 */

#pragma once

#include <cstddef>
#include <atomic>
#include <vector>

#include "tec/tec_def.hpp"  // IWYU pragma: keep
#include "tec/tec_trace.hpp"
#include "tec/tec_thread_pool.hpp"


namespace tec {

/**
 * @brief Specialized thread pool that maintains one pre-allocated fixed-size buffer
 *        per worker thread — intended mainly for network/socket I/O patterns.
 *
 * Each worker thread gets its own dedicated buffer that lives for the entire
 * lifetime of the pool. This avoids frequent small allocations during high-frequency
 * socket receive/send operations and reduces pressure on the general-purpose allocator.
 *
 * Buffer access is index-based and uses simple round-robin distribution via an
 * atomic counter.
 *
 * @note This class does **not** own sockets or manage connections — it only provides
 *       per-thread scratch buffers and inherits task scheduling from ThreadPool.
 */
class SocketThreadPool : public ThreadPool {
private:
    size_t buffer_size_; ///< Size (in bytes) of each per-thread scratch buffer
    std::vector<char> buffers_; ///< Vector of raw pointers to per-thread buffers (one per worker)
    std::atomic<size_t> next_worker_index_; ///< Atomic counter used for round-robin worker index selection

public:
    /**
     * @brief Constructs a socket-oriented thread pool with per-thread buffers
     * @param buffer_size Size in bytes of each worker's private buffer
     * @param num_threads Number of worker threads (passed to base ThreadPool)
     *
     * Allocates exactly `num_threads` buffers of size `buffer_size`.
     * Buffers are allocated with plain `new[]` and must be POD-compatible.
     */
    explicit SocketThreadPool(size_t buffer_size, size_t num_threads)
        : ThreadPool(num_threads)
        , buffer_size_{buffer_size}
        , next_worker_index_{0}
    {
        TEC_ENTER("SocketThreadPool::SocketThreadPool");
        //
        // Allocate thread buffers.
        //
        buffers_.resize(num_threads_ * buffer_size_);
    }

    /**
     * @brief Returns a pointer to the buffer belonging to the worker at index `idx`
     * @param idx Worker index (any integral value is accepted)
     * @return Pointer to the beginning of the corresponding buffer
     *
     * Uses modulo arithmetic so any large `idx` value maps safely into the valid range.
     * Intended usage: `pool.get_buffer(pool.get_next_worker_index())`
     */
    char* get_buffer(size_t idx) const {
        // safe even if idx is huge
        return (char*)buffers_.data() + ((idx % num_threads_) * buffer_size_);
    }

    /**
     * @brief Returns the size (in bytes) of each per-thread buffer
     * @return Buffer size passed at construction
     */
    constexpr size_t get_buffer_size() { return buffer_size_; }

    /**
     * @brief Atomically selects the next worker index in round-robin fashion
     * @return Index (0 … num_threads-1) of the next worker to be used
     *
     * Uses relaxed memory order — sufficient for simple round-robin distribution
     * when no additional synchronization is required between consecutive calls.
     *
     * Typical pattern:
     * @code
     * size_t idx = pool.get_next_worker_index();
     * char* buf = pool.get_buffer(idx);
     * // use buf for this socket operation...
     * @endcode
     */
    size_t get_next_worker_index() {
        return next_worker_index_.fetch_add(1, std::memory_order_relaxed)
             % workers_.size();
    }

    /**
     * @brief Destructor – releases all per-thread buffers
     *
     * Deletes every buffer previously allocated with `new[]`.
     * Base class (`ThreadPool`) destructor is called afterwards and joins all threads.
     */
    virtual ~SocketThreadPool() {
    }

}; // class SocketThreadPool

} // namespace tec
