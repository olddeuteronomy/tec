// Time-stamp: <Last changed 2025-09-30 18:02:12 by magnolia>
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
 * @file tec_server_worker.hpp
 * @brief Defines a worker class that manages a server instance in the tec namespace.
 * @author The Emacs Cat
 * @date 2025-09-17
 */

#pragma once

#include <cassert>
#include <memory>

#include "tec/tec_def.hpp" // IWYU pragma: keep
#include "tec/tec_status.hpp"
#include "tec/tec_worker.hpp"
#include "tec/tec_server.hpp"


namespace tec {

/**
 * @class ServerWorker
 * @brief A worker that owns and manages a server instance.
 * @details Extends the Worker class to start and shut down a server instance,
 * handling server lifecycle within a dedicated thread. It uses signals and status
 * objects to manage server startup and shutdown with timeout support.
 * @tparam TParams The type of parameters, typically derived from ServerParams.
 * @tparam TServer The server type, derived from Server.
 * @see Worker
 * @see Server
 */
template <typename TParams, typename TServer>
class ServerWorker : public Worker<TParams> {
public:
    using Params = TParams; ///< Type alias for worker parameters.

protected:
    std::unique_ptr<TServer> server_; ///< The server instance owned by the worker.

private:
    std::unique_ptr<std::thread> server_thread_; ///< Thread for running the server.
    Signal sig_started_; ///< Signal indicating the server has started.
    Signal sig_stopped_; ///< Signal indicating the server has stopped.
    Status status_started_; ///< Status of the server startup operation.
    Status status_stopped_; ///< Status of the server shutdown operation.

public:
    /**
     * @brief Constructs a ServerWorker with parameters and a server instance.
     * @details Initializes the Worker base class with parameters, takes ownership of
     * the server instance, and creates a server thread that waits for a signal to start.
     * Ensures TServer is derived from Server via static assertion.
     * @param params The configuration parameters for the worker.
     * @param server A unique pointer to the server instance.
     */
    ServerWorker(const Params& params, std::unique_ptr<TServer> server)
        : Worker<Params>(params)
        , server_{std::move(server)}
        , server_thread_{nullptr}
    {
        static_assert(
            std::is_base_of<Server, TServer>::value,
            "ServerWorker::TServer must derive from tec::Server");
    }

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    ServerWorker(const ServerWorker&) = delete;

    /**
     * @brief Deleted move constructor to prevent moving.
     */
    ServerWorker(ServerWorker&&) = delete;

    /**
     * @brief Destructor that ensures proper server thread cleanup.
     * @details Joins the server thread if it is joinable to ensure clean shutdown.
     */
    virtual ~ServerWorker() {
        if (server_thread_ &&  server_thread_->joinable()) {
            server_thread_->join();
        }
    }

protected:
    /**
     * @brief Initializes the worker by starting the server thread.
     * @details Signals the server thread to start and waits for the server to signal
     * completion or timeout. Logs events using tracing if enabled.
     * @return Status Error::Kind::Ok if successful, otherwise an error status (e.g., TimeoutErr).
     * @see Worker::on_init()
     */
    Status on_init() override {
        TEC_ENTER("ServerWorker::on_init");

        if( server_thread_ ) {
            return {"Server is already running", Error::Kind::RuntimeErr};
        }

        // Start the server thread.
        server_thread_ = std::move(std::make_unique<std::thread>([&] {
            server_->start(&sig_started_, &status_started_);
        }));

        // Wait for the server started.
        if (!sig_started_.wait_for(this->params_.start_timeout)) {
            // Timeout!
            TEC_TRACE("!!! Error: server start timeout -- server detached");
            server_thread_->detach();
            return {"Server start timeout", Error::Kind::TimeoutErr};
        }
        // Check the status.
        if (!status_started_) {
            // Something went wrong; join the server thread.
            server_thread_->join();
            return status_started_;
        }

        // Everything is OK.
        return {};
    }

    /**
     * @brief Shuts down the server during worker exit.
     * @details Initiates server shutdown in a separate thread and waits for completion
     * or timeout. Logs events using tracing if enabled.
     * @return Status Error::Kind::Ok if successful, otherwise an error status (e.g., TimeoutErr).
     * @see Worker::on_exit()
     */
    Status on_exit() override {
        TEC_ENTER("ServerWorker::on_exit");

        if (!server_thread_ || !server_thread_->joinable()) {
            // Already finished or not started.
            return {};
        }

        // Start a thread to shut down the server.
        std::thread shutdown_thread([&] {
            server_->shutdown(&sig_stopped_);
        });

        // Wait for the server to shut down.
        if (!sig_stopped_.wait_for(this->params_.shutdown_timeout)) {
            // Timeout! Detach the server thread.
            server_thread_->detach();
            TEC_TRACE("!!! Error: Server shutdown timeout -- server thread detached.");
            status_stopped_ = {"Server shutdown timeout", Error::Kind::TimeoutErr};
        } else {
            server_thread_->join();
        }

        shutdown_thread.join();
        return status_stopped_;
    }

public:
    /// @name ServerWorker Builders
    /// @brief Factory structs for creating ServerWorker instances.
    /// @details Provide builders to create ServerWorker instances as either Daemon or Worker pointers,
    /// ensuring type safety through static assertions.
    /// @{

    /**
     * @struct DaemonBuilder
     * @brief Factory for creating ServerWorker as a Daemon.
     * @details Creates a ServerWorker instance with a derived worker and server,
     * returning it as a Daemon pointer.
     * @tparam WorkerDerived The derived ServerWorker class type.
     * @tparam ServerDerived The derived Server class type.
     */
    template <typename WorkerDerived, typename ServerDerived>
    struct DaemonBuilder {
        /**
         * @brief Creates a unique pointer to a ServerWorker as a Daemon.
         * @details Constructs a WorkerDerived instance with a ServerDerived instance,
         * ensuring type constraints via static assertions.
         * @param params The parameters for constructing the worker and server.
         * @return std::unique_ptr<Daemon> A unique pointer to the created worker.
         */
        std::unique_ptr<Daemon>
        operator()(typename WorkerDerived::Params const& params) {
            static_assert(
                std::is_base_of<Daemon, WorkerDerived>::value,
                "WorkerDerived must derive from tec::Daemon");
            static_assert(
                std::is_base_of<Server, ServerDerived>::value,
                "ServerDerived must derive from tec::Server");
            static_assert(
                std::is_base_of<ServerParams, typename WorkerDerived::Params>::value,
                "Params must derive from tec::ServerParams");

            return std::make_unique<WorkerDerived>(params, std::move(std::make_unique<ServerDerived>(params)));
        }
    };

    /**
     * @struct WorkerBuilder
     * @brief Factory for creating ServerWorker as a Worker.
     * @details Creates a ServerWorker instance with a derived worker and server,
     * returning it as a Worker pointer.
     * @tparam WorkerDerived The derived ServerWorker class type.
     * @tparam ServerDerived The derived Server class type.
     */
    template <typename WorkerDerived, typename ServerDerived>
    struct WorkerBuilder {
        /**
         * @brief Creates a unique pointer to a ServerWorker as a Worker.
         * @details Constructs a WorkerDerived instance with a ServerDerived instance,
         * ensuring type constraints via static assertions.
         * @param params The parameters for constructing the worker and server.
         * @return std::unique_ptr<Worker<typename WorkerDerived::Params>> A unique pointer to the created worker.
         */
        std::unique_ptr<Worker<typename WorkerDerived::Params>>
        operator()(typename WorkerDerived::Params const& params) {
            static_assert(
                std::is_base_of<Worker<typename WorkerDerived::Params>, WorkerDerived>::value,
                "WorkerDerived must derive from tec::Worker");
            static_assert(
                std::is_base_of<Server, ServerDerived>::value,
                "ServerDerived must derive from tec::Server");
            static_assert(
                std::is_base_of<ServerParams, typename WorkerDerived::Params>::value,
                "Params must derive from tec::ServerParams");

            return std::make_unique<WorkerDerived>(params, std::move(std::make_unique<ServerDerived>(params)));
        }
    };

    /// @}
}; // class ServerWorker

} // namespace tec
