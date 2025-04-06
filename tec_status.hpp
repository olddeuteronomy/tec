// Time-stamp: <Last changed 2025-04-01 16:04:25 by magnolia>
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
 *   @file tec_status.hpp
 *   @brief A generalized status of execution.
*/



#pragma once

#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {


struct Error {
    //! Error classes.
    enum class Kind: int {
        Ok  //!< Success
        , Err        //!< Generic error
        , IOErr      //!< IO failure
        , RuntimeErr //!< Runtime error
        , NetErr     //!< Network error
        , RpcErr     //!< RPC error
        , TimeoutErr //!< Timeout error
        , Invalid    //!< Invalid data or state
        , System     //!< System error
    };

    //! Generic error codes.
    template <typename TCode = int>
    struct Code {
        constexpr static const TCode Unspecified{-1}; //!< Unspecified error code.
    };
};

//! Returns Error::Kind as a string.
inline constexpr const char* kind_as_string(Error::Kind k)  {
    switch (k) {
    case Error::Kind::Ok: { return "Success"; }
    case Error::Kind::Err: { return "Generic"; }
    case Error::Kind::IOErr: { return "IO"; }
    case Error::Kind::RuntimeErr: { return "Runtime"; }
    case Error::Kind::NetErr: { return "Network"; }
    case Error::Kind::RpcErr: { return "Rpc"; }
    case Error::Kind::TimeoutErr: { return "Timeout"; }
    case Error::Kind::Invalid: { return "Invalid"; }
    case Error::Kind::System: { return "System"; }
    default: { return "Unspecified"; }
    }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*                       Status of execution
*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @class TStatus
 * @brief Declares a generalized status of execution.
 */
template <typename TCode, typename TDesc>
struct TStatus {

    Error::Kind kind;          //!< Error class.
    std::optional<TCode> code; //!< Error code (optional).
    std::optional<TDesc> desc; //!< Error description (optional).

    //! Is everything OK?
    constexpr bool ok() const { return kind == Error::Kind::Ok; }
    //! Operator: Is everything OK?
    constexpr operator bool() const { return ok(); }

    //! Output Status to a stream.
    friend std::ostream& operator << (std::ostream& out, const TStatus& status) {
        out << "[" << kind_as_string(status.kind) << "]";
        if( !status.ok() ) {
            out
            << " Code=" << status.code.value_or(Error::Code<TCode>::Unspecified)
            << " Desc=\"" << (status.desc.has_value() ? status.desc.value() : "") << "\"";
        }
        return out;
    }

    //! Return Status as a string.
    std::string as_string() {
        std::ostringstream buf;
        buf << *this;
        return buf.str();
    }

    /**
     * @brief      Constructs a successful TStatus (class is Error::Kind::Ok).
     * @snippet snp_status.cpp OK
     */
    TStatus()
        : kind{Error::Kind::Ok}
    {}

    /**
     * @brief      Constructs an error TStatus with unspecified error code.
     * @param      _kind *Error::Kind* class.
     * @snippet snp_status.cpp Unspecified
     */
    TStatus(Error::Kind _kind)
        : kind{_kind}
        , code{Error::Code<TCode>::Unspecified}
    {}

    /**
     * @brief      Constructs an unspecified error TStatus with description.
     * @param      _desc *TDesc* Error description.
     * @param      _kind *Error::Kind* Error class (default Error::Kind::Err, a generic error).
     * @snippet snp_status.cpp Description
     */
    TStatus(const TDesc& _desc, Error::Kind _kind = Error::Kind::Err)
        : kind{_kind}
        , code{Error::Code<TCode>::Unspecified}
        , desc{_desc}
    {}

    /**
     * @brief      Constructs an error TStatus w/o description.
     * @param      _code *TCode* Error code.
     * @param      _kind *Error::Kind* Error class (default Error::Kind::Err, a generic error).
     * @snippet snp_status.cpp Errcode
     */
    TStatus(const TCode& _code, Error::Kind _kind = Error::Kind::Err)
        : kind{_kind}
        , code{_code}
    {}

    /**
     * @brief      Constructs an error TStatus with description.
     * @param      _code *TCode* Error code.
     * @param      _desc *TDesc* Error description.
     * @param      _kind *Error::Kind* Error class (default Error::Kind::Err, a generic error).
     * @snippet snp_status.cpp CD
     */
    TStatus(const TCode& _code, const TDesc& _desc, Error::Kind _kind = Error::Kind::Err)
        : kind{_kind}
        , code{_code}
        , desc{_desc}
    {}

}; // ::TStatus


/**
 * @brief Specializes the default TStatus<int, std::string>.
 */
typedef TStatus<int, std::string> Status;


} // ::tec
