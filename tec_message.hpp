// Time-stamp: <Last changed 2026-02-04 16:05:40 by magnolia>
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
 * @file tec_message.hpp
 * @brief Defines a flexible message type and helper functions for the tec namespace.
 * @author The Emacs Cat
 * @date 2025-09-17
 */

#pragma once

#include <any>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_status.hpp"
#include "tec/tec_signal.hpp"


namespace tec {

/**
 * @brief Type alias for a message that can hold any object.
 * @details Uses std::any to store arbitrary data types, intended for use in message
 * passing systems, such as processing by a Worker in the tec library. The null message
 * (empty std::any) is used to signal the Worker to exit its message loop.
 */
using Message = std::any;

/**
 * @brief Type alias for a request object that can hold any object.
 * @see Daemon::make_request()
 */
using Request = std::any;

/**
 * @brief Type alias for a reply object that can hold any object.
 * @see Daemon::make_request()
 */
using Reply = std::any;

/// @name Message Helper Functions
/// @brief Utility functions for working with Message objects.
/// @{

/**
 * @brief Creates a null message.
 * @details Returns an empty Message (std::any with no value) used to signal a Worker
 * to exit its message loop.
 * @return Message A null message.
 */
inline Message nullmsg() noexcept { return Message{}; }

/**
 * @brief Checks if a message is null.
 * @details Determines whether the provided Message is empty (has no value).
 * @param msg The Message to check.
 * @return bool True if the message is null, false otherwise.
 */
inline bool is_null(const Message& msg) noexcept { return !msg.has_value(); }

/**
 * @brief Retrieves the type name of a message's content for registering the corresponding message handler.
 * @details Returns the type name of the object stored in the Message.
 * @param msg The Message whose type name is to be retrieved.
 * @return const char* The name of the type stored in the Message.
 * @sa Worker::register_handler().
 */
inline auto name(const Message& msg) noexcept { return msg.type().name(); }

/**
 * @brief A message used for RPC-style calls.
 */
struct Payload {
    Signal* ready;
    Status* status;
    Request request;
    Reply   reply;
};

/// @}

} // namespace tec
