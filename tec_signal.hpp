// Time-stamp: <Last changed 2025-09-30 16:33:34 by magnolia>
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
 * @file tec_signal.hpp
 * @brief Defines a thread-safe signal implementation using mutex and condition variable.
 * @author The Emacs Cat
 * @date 2025-09-17
 * @note Inspired by https://stackoverflow.com/questions/14920725/waiting-for-an-atomic-bool
 */

#pragma once

#include <mutex>
#include <condition_variable>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/**
 * @class Signal
 * @brief A thread-safe signal mechanism for inter-thread synchronization.
 * @details This class provides a simple signal implementation using a mutex and condition variable.
 * It allows threads to wait for a signal to be set, either indefinitely or with a timeout.
 * The signal can be set to notify all waiting threads.
 */
class Signal {
private:
    using Lock = std::lock_guard<std::mutex>; ///< Alias for lock guard used for mutex locking.
    using ULock = std::unique_lock<std::mutex>; ///< Alias for unique lock used for condition variable.

    mutable std::mutex m_; ///< Mutex for thread-safe access to the signal state.
    mutable std::condition_variable cv_; ///< Condition variable for signaling waiting threads.
    mutable bool flag_; ///< Flag indicating the signal's state (true = signaled, false = unsignaled).

public:
    /**
     * @brief Constructs a signal in the unsignaled state.
     * @details Initializes the signal with a false flag, indicating it is not signaled.
     */
    Signal()
        : flag_(false)
    {}

    Signal(const Signal&) = delete;
    Signal(Signal&&) = delete;

    virtual ~Signal() = default;

    /**
     * @brief Sets the signal to the signaled state and notifies all waiting threads.
     * @details Updates the signal flag to true in a thread-safe manner and notifies
     * all threads waiting on the condition variable.
     */
    void set() {
        {
            Lock lock(m_);
            flag_ = true;
        }
        cv_.notify_all();
    }

    /**
     * @brief Waits indefinitely until the signal is set.
     * @details Blocks the calling thread until the signal's flag is set to true.
     * The operation is thread-safe.
     */
    void wait() const {
        ULock ulock(m_);
        cv_.wait(ulock, [this] { return flag_; });
    }

    /**
     * @brief Waits for the signal to be set for a specified duration.
     * @details Blocks the calling thread until the signal is set or the specified
     * duration elapses. Returns true if the signal was set, false if the timeout occurred.
     * @tparam Duration The type of the duration parameter (e.g., std::chrono::milliseconds).
     * @param dur The maximum duration to wait for the signal.
     * @return bool True if the signal was set, false if the wait timed out.
     * @see wait()
     */
    template <typename Duration>
    bool wait_for(Duration dur) const {
        ULock ulock(m_);
        return cv_.wait_for(ulock, dur, [this] { return flag_; });
    }
}; // class Signal

} // namespace tec
