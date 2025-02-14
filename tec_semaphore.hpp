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
 *   @file tec_semaphore.hpp
 *   @brief Declares an abstract semaphore class.
*/

#pragma once

#include <functional>
#include <mutex>
#include <condition_variable>


namespace tec {


/**
 * @class      Semaphore
 * @brief      Declares an abstract semaphore.
 *
 * @details    Signalled when a predicate returns `true.`
 *
 */
template <typename Value>
class Semaphore {
public:
    //! A predicate to check out as a functional object.
    using Predicate = std::function<bool(const Value&)>;

private:
    //@{ Synchronization stuff.
    mutable std::mutex m_;
    mutable std::condition_variable cv_;
    //@}

    Value value_;    //!< A value to check out.
    Predicate pred_; //!< A predicate that checks the value.

public:
    using Lock = std::lock_guard<std::mutex>;
    using ULock = std::unique_lock<std::mutex>;

    //! Constructs a semaphore.
    Semaphore(Predicate&& pred)
        : value_{}
        , pred_(std::function(std::move(pred)))
    {}

    //! Assign a new value to the semaphore and notify all other threads.
    void set_value(const Value& new_value) {
        {
            Lock lock(m_);
            value_ = new_value;
        }
        cv_.notify_all();
    }

    //! Reset the semaphore to the inital state and notify all other threads.
    void reset() {
        {
            Lock lock(m_);
            value_ = Value{};
        }
        cv_.notify_all();
    }

    //! Wait for the predicate returns `true`, unconditionally.
    void wait() const {
        ULock ulock(m_);
        cv_.wait(ulock, [this]{ return pred_(value_);});
    }

    /**
     * @brief      Wait for the predicate returns `true` for the specified amount of time.
     * @param      dur Duration Amount of time to wait for the signal is set.
     * @return     bool `false` if timeout otherwise `true`.
     * @sa wait()
     * @sa Milliseconds
     */
    template <typename Duration>
    bool wait_for(Duration dur) const {
        ULock ulock(m_);
        return cv_.wait_for(ulock, dur, [this]{ return pred_(value_);});
    }

}; // Semaphore

//! The boolean semaphore signals when `value` is set to `true`.
using SemaphoreBool = Semaphore<bool>;
//! The integer semaphore signals when a predicate returns `true`.
using SemaphoreInt = Semaphore<int>;


//! Extends `SemaphoreBool` with `set()` method.
class Signal: public SemaphoreBool {
public:
    //! Constructs a boolean semaphore.
    Signal(): SemaphoreBool([](auto f){return f;}) {}
    //! Set signalled state.
    void set() { set_value(true); }
};


} // ::tec
