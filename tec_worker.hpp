// Time-stamp: <Last changed 2025-04-01 14:38:52 by magnolia>
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
 *   @file tec_worker.hpp
 *   @brief Declares an abstract Worker class.
 *
*/

#pragma once

#include <type_traits>
#include <unordered_map>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_daemon.hpp"


namespace tec {


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                            Message
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @brief      A basic message to manage the Worker.
 *
 * @details    Message implements quit() to indicate that
 * Worker thread message polling should stop.
 *
 * @note All user-defined messages should be derived from WorkerMessage.
 */
struct WorkerMessage {
    typedef unsigned long Command;

    //! The quit message loop command.
    static constexpr const Command QUIT{0};

    Command command; //!< A command.

    //! Indicates that the Worker's message loop should be terminated.
    inline bool quit() const { return (command == WorkerMessage::QUIT); }
};

//! Null message. Send it to the Worker to quit the message loop.
template <typename TMessage>
TMessage quit() { TMessage msg; msg.command = WorkerMessage::QUIT; return msg; }


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                          Worker traits
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//! Declares the Worker trait - a pair of TParams and TMessage types.
template <
    typename TParams,
    typename TMessage
    >
struct worker_traits {
    typedef TParams Params;
    typedef TMessage Message;
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                         Abstract Worker
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @brief      An abstract Worker class.
 *
 * @details Extends the Daemon class with the `send()` method that is used to
 * manage the Worker's thread.
 * Defines message handlers and default callbacks `on_init()` and `on_exit()`.
 *
 */
template <typename Traits>
class Worker : public Daemon
{
public:
    typedef Traits traits;
    typedef typename traits::Params Params;
    typedef typename traits::Message Message;

    //! Message handler.
    typedef void (*Handler)(Worker<Traits>&, const Message&);

protected:
    // Worker parameters.
    Params params_;

private:
    // Message handlers.
    std::unordered_map<typename Message::Command, Handler> slots_;

public:
    //! Constructs the Worker.
    Worker(const Params& params)
        : Daemon{}
        , params_{params}
    {
    }

    virtual ~Worker() = default;

    //! Returns Worker parameters.
    constexpr Params params() const { return params_; }

    /**
     * @brief Returns the Status of the Worker execution.
     *
     * @details Should be implemented by a derived class as a result
     * of *on_init()* or *on_exit()* callbacks.
     *
     * @return Status
     */
    virtual Status status() = 0;

    /**
     * @brief Sends a message to be dispatched by the Worker.
     *
     * @details Should return `false` if the Worker is not
     * initialized properly (`on_init()` returned an error).
     *
     * @param Message A message to send.
     *
     * @return bool
     */
    virtual bool send(const Message&) = 0;

    /**
     * @brief Registers a message handler.
     * @param cmd
     * @param handler A handler to register.
     */
    void register_handler(typename Message::Command cmd, Handler handler) {
        // Remove existing handler.
        if( auto slot = slots_.find(cmd); slot != slots_.end() ) {
            slots_.erase(cmd);
        }
        slots_[cmd] = {handler};
    }

protected:
    /**
     *  @brief A callback to be called on worker initialization.
     *
     *  @note If `on_init()` returns Status other than `Error::Kind::Ok`,
     *  the Worker should stop message processing and quit the Worker thread
     *  immediately. `on_exit()` callback **will not be called** in this case.
     *
     *  @note Default implementation does nothing returning Error::Kind::Ok.
     *  @return Result `Error::Kind::Ok` by default.
     */
    virtual Status on_init() { return {}; }

    /**
     *  @brief A callback to be called on exiting from the Worker's thread.
     *
     *  @note This callback **will not be called** if `on_init()`
     *  callback returned Result other than `Error::Kind::Ok`.
     *
     *  @note Default implementation does nothing returning `Error::Kind::Ok`.
     *  @return Status `Error::Kind::Ok` by default.
     */
    virtual Status on_exit() { return {}; }


    /**
     * @brief      Dispatches a message by calling the corresponding message handler.
     * @param      msg A message to dispatch.
     * @sa register_handler()
     */
    virtual void dispatch(const Message& msg) {
        if( auto slot = slots_.find(msg.command); slot != slots_.end() ) {
            slot->second(std::ref(*this), msg);
        }
    }

public:
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                        Worker buider
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    //! Builds the Worker from a derived class.
    template <typename Derived>
    struct Builder {
        std::unique_ptr<Worker<typename Derived::traits>>
        operator()(typename Derived::Params& params) {
            static_assert(std::is_base_of<Worker<typename Derived::traits>, Derived>::value,
                "not derived from tec::Worker class");
            return std::make_unique<Derived>(params);
        }
    };
};

} // ::tec
