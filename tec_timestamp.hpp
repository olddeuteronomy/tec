// Time-stamp: <Last changed 2026-01-19 16:35:22 by magnolia>
/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022-2026 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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
 * @details A Timestamp represents a point in time independent of any
 * time zone or local calendar, encoded as a count of nanoseconds.
 * The count is relative to an epoch at UTC midnight on January 1, 1970.
 *
 * @sa https://github.com/protocolbuffers/protobuf/blob/main/src/google/protobuf/timestamp.proto
 */
struct Timestamp {
    using count_t = int64_t;
    using duration_t = std::chrono::nanoseconds;
    using system_clock_t = std::chrono::system_clock;
    using time_point_t = std::chrono::time_point<system_clock_t, duration_t>;

    /// Represents nanoseconds of UTC time since Unix epoch
    /// 1970-01-01T00:00:00Z to 2262-04-11T23:47:16Z inclusive.
    count_t count;


    /// Get `std::chrono::duration` in nanoseconds.
    duration_t dur() const {
        duration_t d{count};
        return d;
    }

    /// Get UTC as `std::tm` struct.
    std::tm utc_time() const {
        std::lock_guard<std::mutex> lk(time_mutex::get());
        // duration_t d{count};
        auto tp = time_point_t{dur()};
        auto tt = system_clock_t::to_time_t(tp);
        auto tm_utc = *std::gmtime(&tt);
        return tm_utc;
    }

    /// Get local time as `std::tm` struct.
    std::tm local_time() const {
        std::lock_guard<std::mutex> lk(time_mutex::get());
        // duration_t dur{count};
        auto tp = time_point_t{dur()};
        auto tt = system_clock_t::to_time_t(tp);
        auto tm_local = *std::localtime(&tt);
        return tm_local;
    }

    std::string utc_time_str() const {
        static constexpr char fmt[] = "%FT%TZ";
        auto tm = utc_time();
        return iso_8601(fmt, &tm);
    }

    std::string local_time_str() const {
        static constexpr char fmt[] = "%FT%T%z";
        auto tm = local_time();
        return iso_8601(fmt, &tm);
    }

    /// It's UTC-based, no leap seconds, and what virtually every system, language, and network protocol expects.
    static Timestamp now() {
        auto now = system_clock_t::now();
        auto d = now.time_since_epoch();
        // return {d.count()};
        return {std::chrono::duration_cast<duration_t>(d).count()};
    }

    static std::string iso_8601(const char* fmt, std::tm* tm) {
        char buf[80];
        std::strftime(buf, sizeof(buf)-1, fmt, tm);
        return buf;
    }

private:

   /**
    * @struct time_mutex
    * @brief Provides a global mutex for synchronizing time conversion.
    * @details Manages a static mutex to ensure thread-safe time conversion operations.
    */
    struct time_mutex {
        /**
         * @brief Retrieves the global time mutex.
         * @details Returns a reference to a static mutex used for synchronizing time conversion operations.
         * @return std::mutex& The global time mutex.
         */
        static std::mutex& get() {
            static std::mutex mtx_time__;
            return mtx_time__;
        }
    };

}; // class Timestamp

} // namespace tec
