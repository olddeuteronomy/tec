// Time-stamp: <Last changed 2025-04-08 16:32:36 by magnolia>
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
 *   @file tec_message.hpp
 *   @brief Defines a basic WorkerMessage.
 *
*/

#pragma once

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/**
 * @brief      A basic message to manage the Worker.
 *
 * @details    WorkerMessage implements quit() to indicate that
 * Worker thread message polling should stop.
 *
 * @note All user-defined messages should be derived from WorkerMessage.
 * @sa Worker
 */
struct WorkerMessage {
    typedef unsigned long Command;

    //! The quit command. Stops message processing. See Worker.
    static constexpr const Command QUIT{0};

    Command command; //!< A command.

    //! Indicates that the Worker's message loop should be terminated.
    constexpr bool quit() const { return (command == WorkerMessage::QUIT); }
};

//! Null message. Send it to Worker to quit the message loop and
//! terminate the Worker's thread.
template <typename TMessage>
TMessage nullmsg() { TMessage msg; msg.command = WorkerMessage::QUIT; return msg; }

} // ::tec
