// Time-stamp: <Last changed 2025-03-31 15:36:04 by magnolia>
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
 *   @file  tec_queue.hpp
 *   @brief The generalized thread-safe message queue.
 *
 *  Borrowed from https://stackoverflow.com/questions/15278343/c11-thread-safe-queue
*/

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {


//! Implements the generalized thread-safe queue.
template <class T>
class SafeQueue {
private:
    std::queue<T> q_;
    mutable std::mutex m_;
    mutable std::condition_variable c_;

public:
    //! Constructs the empty queue.
    SafeQueue(void)
        : q_{}
        , m_{}
        , c_{}
    {}

    ~SafeQueue(void) {}

    /**
     * @brief      Adds an element to the queue.
     * @details    _Moves_ the element to the queue.
     * @param      t T an element to add.
     */
    void enqueue(T t) {
        std::lock_guard<std::mutex> lock(m_);
        q_.push(std::move(t));
        c_.notify_one();
    }

    /**
     * @brief      Returns the front element and removes it from the queue.
     * @details    If the queue is empty, waits till an element is avaiable.
     * @return     T
     */
    T dequeue(void) {
        std::unique_lock<std::mutex> lock(m_);
        while( q_.empty() )
        {
            // Release lock as long as the wait and reaquire it afterwards.
            c_.wait(lock);
        }
        T val = q_.front();
        q_.pop();
        return val;
    }

}; // ::SafeQueue


} // ::tec
