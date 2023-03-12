/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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
 *   \file tec_queue.hpp
 *   \brief Thread safe message queue.
 *
 *  Borrowed from https://stackoverflow.com/questions/15278343/c11-thread-safe-queue
 *  with some extensions (see poll()).
 *
*/

#pragma once

#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "tec/tec_def.hpp"


namespace tec {


template <class T>
class SafeQueue
{
private:
    std::queue<T> q;
    mutable std::mutex m;
    mutable std::condition_variable c;

public:
    //! Construct the empty queue.
    SafeQueue(void)
        : q()
        , m()
        , c()
    {}

    ~SafeQueue(void)
    {}

    //! Add an element to the queue.
    void enqueue(T t)
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(t);
        c.notify_one();
    }

    //! Get the front element.
    //! If the queue is empty, wait till an element is avaiable.
    T dequeue(void)
    {
        std::unique_lock<std::mutex> lock(m);
        while( q.empty() )
        {
            // Release lock as long as the wait and reaquire it afterwards.
            c.wait(lock);
        }
        T val = q.front();
        q.pop();
        return val;
    }

    //! Wait till a message is avaiable.
    //! Returns true on message arriving, returns false if msg.quit() is set.
    bool poll(T& msg)
    {
        msg = std::move(dequeue());
        return !msg.quit();
    }

};


} // ::tec
