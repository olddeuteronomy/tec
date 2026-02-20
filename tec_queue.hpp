// Time-stamp: <Last changed 2026-02-20 16:01:22 by magnolia>
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
 * @file tec_queue.hpp
 * @brief Thread-safe queue implementation.
 *
 * This file defines a thread-safe queue class using standard library
 * synchronization primitives for use in multi-threaded environments.
 *
 * @note Inspired by https://stackoverflow.com/questions/15278343/c11-thread-safe-queue.
 * @author The Emacs Cat
 * @date 2025-09-17
 */

#pragma once

#include <cstddef>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/**
 * @class SafeQueue
 * @brief A thread-safe queue implementation for storing and retrieving elements of type T.
 * @details This class provides a thread-safe queue using a mutex and condition variable
 * to ensure safe access and modification in a multi-threaded environment. It supports
 * enqueueing, dequeueing, and size querying operations.
 * @tparam T The type of elements stored in the queue.
 */
template <class T>
class SafeQueue {
private:
    std::queue<T> q_; ///< The underlying queue storing the elements.
    mutable std::mutex m_; ///< Mutex for thread-safe access to the queue.
    mutable std::condition_variable c_; ///< Condition variable for signaling queue changes.

public:
    /**
     * @brief Constructs an empty thread-safe queue.
     * @details Initializes an empty queue with a mutex and condition variable
     * for thread-safe operations.
     */
    SafeQueue(void)
        : q_{}
        , m_{}
        , c_{}
    {}

    /**
     * @brief Destructor for the SafeQueue.
     * @details Cleans up the queue, mutex, and condition variable.
     * No additional cleanup is required as the standard library handles resource management.
     */
    ~SafeQueue(void) = default;

    /**
     * @brief Adds an element to the back of the queue.
     * @details Moves the provided element into the queue in a thread-safe manner
     * and notifies one waiting thread that a new element is available.
     * @param t The element to add to the queue (moved into the queue).
     */
    void enqueue(T&& t) {
        std::lock_guard<std::mutex> lock(m_);
        q_.push(std::move(t));
        c_.notify_one();
    }

    /**
     * @brief Retrieves and removes the front element from the queue.
     * @details If the queue is empty, the calling thread waits until an element
     * becomes available. The operation is thread-safe.
     * @return The front element of the queue.
     */
    T dequeue(void) {
        std::unique_lock<std::mutex> lock(m_);
        while (q_.empty()) {
            // Release lock during wait and reacquire it afterwards.
            c_.wait(lock);
        }
        T val = q_.front();
        q_.pop();
        return val;
    }

    /**
     * @brief Returns the current number of elements in the queue.
     * @details Provides a thread-safe way to query the queue's size.
     * @return The number of elements in the queue.
     */
    std::size_t size() const {
        std::unique_lock<std::mutex> lock(m_);
        return q_.size();
    }
}; // class SafeQueue

} // namespace tec
