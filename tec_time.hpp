// Time-stamp: <Last changed 2025-04-12 01:17:11 by magnolia>
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
 *   @file tec_time.hpp
 *   @brief Time-related utilities.
 *
*/

#pragma once

#include <cstdint>
#include <ctime>
#include <sys/time.h>

#include "tec/tec_def.hpp" // IWYU pragma: keep

#if defined (__TEC_WINDOWS)
#error Windows version is not implemented yet.
#endif


namespace tec {

/**
 * @brief      A point in time.
 *
 * @details A Timestamp represents a point in time independent of any
 * time zone or local calendar, encoded as a count of seconds and
 * fractions of seconds at nanosecond resolution. The count is
 * relative to an epoch at UTC midnight on January 1, 1970, in the
 * proleptic Gregorian calendar which extends the Gregorian calendar
 * backwards to year one.
 *
 * @note https://github.com/protocolbuffers/protobuf/blob/main/src/google/protobuf/timestamp.proto
 */
struct Timestamp {
    // Represents seconds of UTC (or local) time since Unix epoch
    // 1970-01-01T00:00:00Z. Must be from 0001-01-01T00:00:00Z to
    // 9999-12-31T23:59:59Z inclusive.
    int64_t seconds;
    //! Must be from 0 to 999,999,999 inclusive.
    int32_t nanos;
    //! Unix struct `tm`.
    ::tm date_time;

    enum class TMZONE {
        UTC,
        LOCAL
    };

    constexpr bool ok() const { return seconds > 0; }
    constexpr operator bool () const { return ok(); }

    static Timestamp now(TMZONE zone = TMZONE::UTC) {
        ::time_t t{};
        if( ::time(&t) != (time_t)-1 ) {
            ::tm date_time{};
            ::tm* buf = ((zone == TMZONE::UTC) ?
                       gmtime_r(&t, &date_time) :
                       localtime_r(&t, &date_time));
            if( buf != NULL ) {
                ::timeval tv{};
                ::gettimeofday(&tv, NULL);
                return {tv.tv_sec, (int32_t)tv.tv_usec * 1000, date_time};
            }
        }
        return {0, 0, {}};
    }


    friend constexpr bool operator == (const Timestamp& lhs, const Timestamp& rhs) {
        return (lhs.seconds == rhs.seconds && lhs.nanos == rhs.nanos);
    }


    friend constexpr bool operator < (const Timestamp& lhs, const Timestamp& rhs) {
        if( lhs.seconds < rhs.seconds ) {
            return true;
        }
        else if( lhs.seconds == rhs.seconds ) {
            return (lhs.nanos < rhs.nanos);
        }
        return false;
    }

}; // ::Timestamp

} // ::tec
