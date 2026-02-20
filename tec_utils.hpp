// Time-stamp: <Last changed 2026-02-20 16:07:01 by magnolia>
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
 * @file tec_utils.hpp
 * @brief Provides time-related utilities and system information functions for the tec namespace.
 * @author The Emacs Cat
 * @date 2025-09-17
 */

#pragma once

#include <chrono>
#include <cstdio>
#include <string>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/// @name Time Utilities
/// @brief Type aliases and functions for time-related operations.
/// @details Provides utilities for measuring time intervals using a monotonic clock,
/// along with type aliases for common time durations and time points.
/// @{

/**
 * @brief Type alias for a monotonic clock suitable for measuring intervals.
 * @details Uses std::chrono::steady_clock for consistent interval measurements.
 */
using Clock = std::chrono::steady_clock;

/**
 * @brief Type alias for seconds duration.
 */
using Seconds = std::chrono::seconds;

/**
 * @brief Type alias for a time point in seconds.
 */
using TimePointSec = std::chrono::time_point<Clock, Seconds>;

/**
 * @brief Type alias for milliseconds duration.
 */
using MilliSec = std::chrono::milliseconds;

/**
 * @brief Type alias for a time point in milliseconds.
 */
using TimePointMs = std::chrono::time_point<Clock, MilliSec>;

/**
 * @brief Type alias for microseconds duration.
 */
using MicroSec = std::chrono::microseconds;

/**
 * @brief Type alias for a time point in microseconds.
 */
using TimePointMu = std::chrono::time_point<Clock, MicroSec>;

/**
 * @brief Returns the current time as a duration since the epoch.
 * @details Measures the time elapsed since the clock's epoch, cast to the specified duration type.
 * @tparam Duration The duration type (e.g., Seconds, MilliSec, MicroSec).
 * @return Duration The current time as a duration since the epoch.
 */
template <typename Duration>
Duration now() {
    return std::chrono::duration_cast<Duration>(Clock::now() - std::chrono::time_point<Clock, Duration>());
}

/**
 * @brief Returns the duration since a specified start time.
 * @details Calculates the time elapsed since the given start time, using the specified duration type.
 * @tparam Duration The duration type (e.g., Seconds, MilliSec, MicroSec).
 * @param start The start time as a duration since the epoch.
 * @return Duration The time elapsed since the start time.
 */
template <typename Duration>
Duration since(Duration start) {
    return now<Duration>() - start;
}

/**
 * @brief Returns the unit string for a duration type.
 * @details Provides a human-readable string representing the unit of the specified duration type.
 * @param duration The duration type (e.g., Seconds, MilliSec, MicroSec).
 * @return const char* The unit string ("s" for seconds, "ms" for milliseconds, "mu" for microseconds).
 */
constexpr const char* time_unit(Seconds) { return "s"; }
constexpr const char* time_unit(MilliSec) { return "ms"; }
constexpr const char* time_unit(MicroSec) { return "mu"; }

/**
 * @brief Returns a constant duration of one hour.
 * @details Provides a constant representing one hour in seconds.
 * @return Seconds A duration of one hour (3600 seconds).
 */
constexpr Seconds one_hour() { return Seconds(60 * 60); }

/**
 * @brief Returns a constant duration of one day.
 * @details Provides a constant representing one day in seconds.
 * @return Seconds A duration of one day (86400 seconds).
 */
constexpr Seconds one_day() { return Seconds(24 * 60 * 60); }

/// @}

/**
 * @class Timer
 * @brief A simple timer for measuring elapsed time.
 * @details Measures the time elapsed between start and stop calls using a monotonic clock.
 * @tparam Duration The duration type for measurements (defaults to MilliSec).
 */
template <typename Duration = MilliSec>
class Timer {
private:
    Duration src_; ///< The start time of the timer.

public:
    /**
     * @brief Constructs and starts a timer.
     * @details Initializes the timer and immediately starts it by recording the current time.
     */
    Timer() {
        start();
    }

    /**
     * @brief Starts or restarts the timer.
     * @details Records the current time as the timer's start point.
     */
    void start() {
        src_ = now<Duration>();
    }

    /**
     * @brief Stops the timer and returns the elapsed time.
     * @details Calculates the time elapsed since the last start() call.
     * @return Duration The elapsed time since the timer was started.
     */
    Duration stop() {
        return now<Duration>() - src_;
    }
};

/// @name System Utilities
/// @brief Functions for retrieving system information (non-Windows platforms).
/// @details Provides utilities for retrieving the computer name and username on non-Windows platforms.
/// Windows-specific utilities are defined in tec/mswin/tec_win_utils.hpp.
/// @{

#if !(defined(__TEC_WINDOWS__) || defined(__TEC_MINGW__))

#include <unistd.h>
#include <pwd.h>

#if defined(__TEC_APPLE__)
#include <sys/syslimits.h>
#else
#include <linux/limits.h>
#endif

/**
 * @brief Retrieves the computer hostname.
 * @details Returns the hostname of the current system using UTF-8 encoding. Returns an empty string on failure.
 * @return std::string The hostname or an empty string if the operation fails.
 */
inline std::string getcomputername() {
    char host[PATH_MAX];
    if (gethostname(host, sizeof(host)) == 0)
        return host;
    else
        return "";
}

/**
 * @brief Retrieves the logged-in username.
 * @details Returns the username of the current user using UTF-8 encoding. Returns an empty string on failure.
 * @return std::string The username or an empty string if the operation fails.
 */
inline std::string getusername() {
    uid_t uid = geteuid();
    struct passwd* pw = getpwuid(uid);
    if (pw != NULL)
        return pw->pw_name;
    else
        return "";
}

#endif // !(__TEC_WINDOWS__ || __TEC_MINGW__)

/// @}

} // namespace tec

#if defined(__TEC_WINDOWS__) || defined(__TEC_MINGW__)
// MS Windows stuff goes here. NOT YET!
#include "tec/mswin/tec_win_utils.hpp"
#endif // __TEC_WINDOWS__
