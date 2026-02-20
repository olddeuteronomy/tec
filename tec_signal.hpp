// Time-stamp: <Last changed 2026-02-20 16:02:26 by magnolia>
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

    /**
     * @struct OnExit
     * @brief Helper struct to signal termination on exit.
     * @details Automatically sets the termination signal when destroyed.
     */
    struct OnExit {
    private:
        Signal* sig_; ///< The termination signal.

    public:
        /**
         * @brief Constructs an OnExit helper with a termination signal.
         * @param sig The termination signal to set on destruction.
         */
        explicit OnExit(Signal* sig) : sig_{sig} {}

        /**
         * @brief Destructor that sets the termination signal.
         */
        ~OnExit() { sig_->set(); }
    };
}; // class Signal

} // namespace tec
