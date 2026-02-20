// Time-stamp: <Last changed 2026-02-20 16:05:28 by magnolia>
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
 *   @file tec_timestamp.hpp
 *   @brief A point in time since UNIX epoch.
 *   @date 2026-01-18
 */

#pragma once

#include <cstdlib>
#include <string>
#include <ctime>
#include <chrono>
#include <mutex>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/**
 * @brief A point in time.
 *
 * @details Timestamp represents a point in time independent of any
 * time zone or local calendar, encoded as a count of nanoseconds.
 * The count is relative to an epoch at UTC midnight on January 1, 1970.
 *
 * @par Example
 * @snippet snp_timestamp.cpp main
 *
 * @par Output
 * @snippet snp_timestamp.cpp output
 *
 * @sa https://github.com/protocolbuffers/protobuf/blob/main/src/google/protobuf/timestamp.proto
 */
struct Timestamp {
    /// @name Type aliases
    /// @{

    /// @brief Underlying integer type used to store nanosecond count
    using count_t = int64_t;

    /// @brief Duration unit used by this timestamp (nanoseconds)
    using duration_t = std::chrono::nanoseconds;

    /// @brief Clock type used as the basis for time_point conversion
    using system_clock_t = std::chrono::system_clock;

    /// @brief std::chrono time_point type with nanosecond precision
    using time_point_t = std::chrono::time_point<system_clock_t, duration_t>;

    /// @}

    /**
     * @brief Nanoseconds since Unix epoch (1970-01-01T00:00:00Z).
     *
     * @details Represents the number of nanoseconds elapsed since
     * 1970-01-01 00:00:00 UTC. The valid range is approximately
     * 1970-01-01 to 2262-04-11 (due to 64-bit signed integer limits).
     *
     * Negative values represent times before the Unix epoch.
     */
    count_t count;

    // ──────────────────────────────────────────────────────────────────────────
    // Constructors
    // ──────────────────────────────────────────────────────────────────────────

    /// @brief Default constructor — creates timestamp at epoch (count = 0)
    Timestamp()
        : count{0}
    {}

    /// @brief Construct from raw nanosecond count since epoch
    Timestamp(count_t _count)
        : count{_count}
    {}

    /// @brief Construct from std::chrono::nanoseconds duration
    explicit Timestamp(duration_t d)
        : count{d.count()}
    {}

    Timestamp(const Timestamp&) = default;
    Timestamp(Timestamp&&) = default;

    // ──────────────────────────────────────────────────────────────────────────
    // Accessors / Conversions
    // ──────────────────────────────────────────────────────────────────────────

    /**
     * @brief Returns the duration since epoch as std::chrono::nanoseconds
     * @return duration_t nanoseconds since 1970-01-01T00:00:00Z
     */
    duration_t dur() const {
        duration_t d{count};
        return d;
    }

    /**
     * @brief Returns broken-down UTC time as std::tm
     * @details Uses thread-safe wrapper around std::gmtime
     * @return std::tm structure with UTC time components
     */
    std::tm utc_time() const {
        // To prevent possible problems with std::gmtime that may not be thread-safe.
        std::lock_guard<std::mutex> lk(time_mutex::get());
        auto tp = time_point_t{dur()};
        auto tt = system_clock_t::to_time_t(tp);
        auto tm_utc = *std::gmtime(&tt);
        return tm_utc;
    }

    /**
     * @brief Returns broken-down local time as std::tm
     * @details Uses thread-safe wrapper around std::localtime
     * @return std::tm structure with local time components
     */
    std::tm local_time() const {
        // To prevent possible problems with std::localtime that may not be thread-safe.
        std::lock_guard<std::mutex> lk(time_mutex::get());
        auto tp = time_point_t{dur()};
        auto tt = system_clock_t::to_time_t(tp);
        auto tm_local = *std::localtime(&tt);
        return tm_local;
    }

    /**
     * @brief Returns ISO 8601 string in UTC (with Z suffix)
     * @return std::string formatted as "YYYY-MM-DDThh:mm:ssZ"
     */
    std::string utc_time_str() const {
        static constexpr char fmt[] = "%FT%TZ";
        auto tm = utc_time();
        return iso_8601(fmt, &tm);
    }

    /**
     * @brief Returns ISO 8601 string in local time (with timezone offset)
     * @return std::string formatted as "YYYY-MM-DDThh:mm:ss±hhmm"
     */
    std::string local_time_str() const {
        static constexpr char fmt[] = "%FT%T%z";
        auto tm = local_time();
        return iso_8601(fmt, &tm);
    }

    /**
     * @brief Returns current time as Timestamp (UTC-based, nanosecond precision)
     * @details Uses std::chrono::system_clock::now() and converts to nanoseconds.
     * This is the recommended way to obtain "current time" in this system.
     * @note It is **not** monotonic — subject to system clock adjustments.
     * @return Timestamp representing current UTC time
     */
    static Timestamp now() {
        auto now = system_clock_t::now();
        auto d = now.time_since_epoch();
        return {std::chrono::duration_cast<duration_t>(d).count()};
    }

private:
    /**
     * @brief Internal helper to format std::tm according to strftime pattern
     * @param fmt   strftime-compatible format string
     * @param tm    pointer to std::tm structure
     * @return Formatted string (max ~80 characters)
     */
    static std::string iso_8601(const char* fmt, std::tm* tm) {
        char buf[80];
        std::strftime(buf, sizeof(buf)-1, fmt, tm);
        return buf;
    }

    /**
     * @brief Helper class providing a static mutex for thread-safe time functions
     * @details Wraps a mutex used to protect calls to
     * std::gmtime / std::localtime which are typically not thread-safe.
     */
    struct time_mutex {
        /**
         * @brief Returns reference to the global static mutex
         * @return std::mutex& thread-safe mutex for time conversion
         */
        static std::mutex& get() {
            static std::mutex mtx_time__;
            return mtx_time__;
        }
    };

}; // struct Timestamp

} // namespace tec
