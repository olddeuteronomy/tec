/*----------------------------------------------------------------------
------------------------------------------------------------------------
Copyright (c) 2022 The Emacs Cat (https://github.com/olddeuteronomy/tec).

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
*   \file tec_win_service.hpp
*   \brief MS Windows service template.
*
*  Creates and run MS Windows service.
*
*/

#pragma once

#if !defined (__TEC_WINDOWS__)
#error This file can be used on MS Windows only!

#else
// Windows stuff goes here

#include <winnt.h>
#include <winsvc.h>

#include "tec/tec_rpc.hpp"


//! MUST BE USED IN YOUR main.cpp (or somewhere else in a cpp file).
#define INIT_SERVICE(service) template<> service* service::impl::this_ = nullptr


namespace tec {

namespace winsvc {


static const DWORD WINSVC_SERVICE_TYPE = SERVICE_WIN32;
static const DWORD WINSVC_CONTROLS_ACCEPTED = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN; // | SERVICE_ACCEPT_PAUSE_CONTINUE

//! The estimated time required for a pending start, stop, pause, or continue operation, in milliseconds.
static const DWORD WINSVC_WAIT_HINT = 5000;


//! Service parameters. Must be a descendant of WorkerParams, see Worker.
template <typename TWorkerParams>
struct ServiceParams: public TWorkerParams {
    String name;
    DWORD dwServiceType;
    DWORD dwControlsAccepted;
    DWORD dwWaitHint;

    ServiceParams(const String& _name)
        : name(_name)
        , dwServiceType(WINSVC_SERVICE_TYPE)
        , dwControlsAccepted(WINSVC_CONTROLS_ACCEPTED)
        , dwWaitHint(WINSVC_WAIT_HINT)
    {}
};


//! A functor that creates a service daemon.
template <typename TServerParams, typename TWorkerParams>
struct DaemonBuilder {
    rpc::Daemon* (*fptr)(const TServerParams&, const TWorkerParams&);

    rpc::Daemon* operator()(const TServerParams& server_params, const TWorkerParams& worker_params) {
        if( nullptr == fptr ) {
            return nullptr;
        }
        else {
            return fptr(server_params, worker_params);
        }
    }
};


template <typename TServerParams, typename TWorkerParams, typename TServiceParams>
class Service {
protected:
    // Service custom attributes.
    TServerParams server_params_;
    TServiceParams service_params_;

    // MSWindows service specific attributes.
    SERVICE_STATUS_HANDLE handle_;
    SERVICE_STATUS status_;

    //! Service daemon.
    DaemonBuilder<TServerParams, TWorkerParams> daemon_builder_;
    std::unique_ptr<rpc::Daemon> daemon_;

public:
    enum Status {
        OK = 0,
        WINSVC_NODAEMON = 53101, // ERROR_INVALID_FIELD_IN_PARAMETER_LIST 328 (0x148)
        WINSVC_RUN_DAEMON_ERROR, // ERROR_CAN_NOT_COMPLETE 1003 (0x3EB)
        WINSVC_TERMINATE_TIMEOUT, // WAIT_TIMEOUT 258 (0x102)
    };

    Service(
        const TServerParams& server_params,
        const TServiceParams& service_params,
        DaemonBuilder<TServerParams, TWorkerParams> daemon_builder)
        : server_params_(server_params)
        , service_params_(service_params)
        , daemon_builder_(daemon_builder)
        , handle_(nullptr)
    {
        ::ZeroMemory(&status_, sizeof(SERVICE_STATUS));
        status_.dwServiceType = service_params.dwServiceType;
        status_.dwControlsAccepted = service_params.dwControlsAccepted;
        impl::this_ = this;
    }


    virtual ~Service() {
    }


    struct impl {
        //! The one and only one global service pointer.
        static Service* this_;

        static void WINAPI StartServiceCallback(DWORD argc, LPTSTR* argv) {
            if( nullptr != this_ ) {
                this_->StartService(argc, argv);
            }
        }

        static void WINAPI CtrlHandlerCallback(DWORD dwOpcode) {
            if( nullptr != this_ ) {
                this_->CtrlHandler(dwOpcode);
            }
        }
    };


    //! \brief Start the service dispatcher.
    //! \param None.
    //! \return 0 if succeeded or system error code otherwise.
    //! \sa GetLastError() function from Platform SDK: Debugging and Error Handling.
    virtual DWORD Start() {
        // Register StartService dispatcher callback(s)
        SERVICE_TABLE_ENTRY DispatchTable[] = {
            {(LPTSTR)((LPCTSTR)service_params_.name.c_str()), impl::StartServiceCallback},
            {0, 0} // Sentinel
        };
        if( !StartServiceCtrlDispatcher(DispatchTable) ) {
            DWORD dwErrorCode = GetLastError();
            return dwErrorCode;
        }
        return 0;
    }


    //! \brief Set service status.
    //! \param dwStatus [in] - a new status of the service.
    //! \param dwWaitHint [in] - an estimate of the amount of time, in milliseconds,
    //!                          that the service expects a pending start, stop,
    //!                          pause, or continue operation to take before the
    //!                          service makes its next call to the SetServiceStatus function.
    //! \return none
    //! \sa SetServiceStatus() function from Platform SDK: DLLs, Processes, and Threads.
    //! \sa SERVICE_STATUS structure from Platform SDK: DLLs, Processes, and Threads.
    virtual void SetStatus(DWORD dwStatus, DWORD dwWaitHint = 1000) {
        status_.dwCurrentState = dwStatus;

        // If current status is *_PENDING, update status to waiting state
        if( SERVICE_START_PENDING == dwStatus ||
            SERVICE_STOP_PENDING == dwStatus ||
            SERVICE_PAUSE_PENDING == dwStatus ||
            SERVICE_CONTINUE_PENDING == dwStatus ) {
            status_.dwCheckPoint++;
            status_.dwWaitHint = dwWaitHint;
        }
        else {
            // ... else clear it.
            status_.dwCheckPoint = 0;
            status_.dwWaitHint = 0;
        }

        if( SERVICE_STOPPED != dwStatus ) {
            // If service not stopped, clear exit status codes.
            status_.dwWin32ExitCode = 0;
            status_.dwServiceSpecificExitCode = 0;
        }
        else {
            // Service is beeing stopped.
        }

        // Now set the new service status
        if( !SetServiceStatus(handle_, &status_) ) {
            // Cannot set the service status.
        }
        else if( SERVICE_STOPPED == dwStatus ) {
            // The service is stopped.
        }
    }


    //! \brief Set the service status to `stopped' and report the error.
    //! \param dwWin32Code [in] - a system error code.
    //! \param dwSpecCode [in] - a service-specific error code.
    //! \return None.
    //! \sa SetServiceStatus() function from Platform SDK: DLLs, Processes, and Threads
    //! \sa SERVICE_STATUS structure from Platform SDK: DLLs, Processes, and Threads
    virtual void SetError(DWORD dwWin32Code, DWORD dwSpecCode) {
        // Set error codes and change status to stopped.
        status_.dwWin32ExitCode = dwWin32Code;
        status_.dwServiceSpecificExitCode = dwSpecCode;
        SetStatus(SERVICE_STOPPED);
    }


    virtual void ProcessCommandLine(DWORD argc, LPTSTR *argv) {
    }

protected:

    //! \brief Main Start Service callback.
    //! \param argc [in] - service command line argument count.
    //! \param argv [in] - service command line argument array.
    //! \return None.
    //---------------------------------------------------------------------------
    virtual void StartService(DWORD argc, LPTSTR *argv) {
        ProcessCommandLine(argc, argv);

        // Register a service handler.
        handle_ = RegisterServiceCtrlHandler(
            service_params_.name.c_str(),
            impl::CtrlHandlerCallback);

        if( (SERVICE_STATUS_HANDLE)0 == handle_ ) {
            DWORD dwErrorCode = GetLastError();
            // Cannot register the Service Control Handler.
            return;
        }

        // Wait for the service starting state is set.
        SetStatus(SERVICE_START_PENDING, service_params_.dwWaitHint);

        // Run the service.
        DoRun(argc, argv);
    }


    /*-----------------------------------------------------------------
                       Service control handler
    -----------------------------------------------------------------*/

    //! \brief Service Control Handler Callback.
    //! \param dwOpcode [in] - service operation control code.
    //! \return None.
    virtual void CtrlHandler(DWORD dwOpcode) {
        switch( dwOpcode ) {
        case SERVICE_CONTROL_PAUSE:
            SetStatus(SERVICE_PAUSE_PENDING);
            DoPause();
            return;

        case SERVICE_CONTROL_CONTINUE:
            SetStatus(SERVICE_CONTINUE_PENDING);
            DoContinue();
            return;

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            SetStatus(SERVICE_STOP_PENDING);
            DoStop();
            return;

        case SERVICE_CONTROL_INTERROGATE:
            SetStatus(status_.dwCurrentState);
            // Do nothing
            return;

        default:
            // Custom operation code.
            DoCustom(dwOpcode);
        }
    }


    /*-----------------------------------------------------------------
               Service command handlers
    -----------------------------------------------------------------*/

    virtual void DoPause() {
        SetStatus(SERVICE_PAUSED);
    }


    virtual void DoContinue() {
        SetStatus(SERVICE_RUNNING);
    }

    virtual void DoCustom(DWORD dwCustomOpCode) {
        // Do nothing.
    }


    virtual void DoRun(DWORD /*argc*/, LPTSTR* /*argv*/) {
        // Create a server daemon.
        daemon_.reset(daemon_builder_(server_params_, service_params_));
        if( !daemon_ )
        {
            SetError(ERROR_INVALID_FIELD_IN_PARAMETER_LIST, Status::WINSVC_NODAEMON);
        }

        // Run the daemon.
        auto result = daemon_->run();
        if( !result.ok() )
        {
            daemon_->terminate();
            daemon_.reset(nullptr);
            SetError(ERROR_CAN_NOT_COMPLETE, result.code());
            return;
        }

        SetStatus(SERVICE_RUNNING);
    }


    virtual void DoStop() {
        if( daemon_ )
        {
            auto result = daemon_->terminate();
            if( !result.ok() )
            {
                SetError(WAIT_TIMEOUT, result.code());
                return;
            }
        }

        daemon_.reset(nullptr);
        SetStatus(SERVICE_STOPPED);
    }

}; // Service


} // ::winsvc

} // ::tec

#endif // __TEC_WINDOWS__
