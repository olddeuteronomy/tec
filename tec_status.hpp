// Time-stamp: <Last changed 2025-09-27 14:40:43 by magnolia>
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
 * @file tec_status.hpp
 * @brief Defines error handling types and utilities for the tec namespace.
 * @author The Emacs Cat
 * @date 2025-09-17
 */

#pragma once

#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/**
 * @struct Error
 * @brief Defines error types and codes for error handling in the tec library.
 * @details Provides an enumeration of error kinds and a templated structure for error codes.
 */
struct Error {
    /**
     * @enum Kind
     * @brief Enumerates possible error categories.
     * @details Defines a set of error kinds used to categorize errors in the tec library.
     */
    enum class Kind : int {
        Ok,          //!< Indicates successful execution.
        Err,         //!< Generic error.
        IOErr,       //!< Input/output operation failure.
        RuntimeErr,  //!< Runtime error during execution.
        NetErr,      //!< Network-related error.
        RpcErr,      //!< Remote procedure call error.
        TimeoutErr,  //!< Timeout during an operation.
        Invalid,     //!< Invalid data or state.
        System,      //!< System-level error.
        NotImplemented, //!< Not implemented.
        Unsupported  //!< The feature is upsupported.
    };

    /**
     * @struct Code
     * @brief Defines error codes with a default unspecified value.
     * @details A templated structure to hold error codes of type TCode, with a
     * constant for an unspecified error code.
     * @tparam TCode The type used for error codes (e.g., int).
     */
    template <typename TCode = int>
    struct Code {
        constexpr static const TCode Unspecified{-1}; //!< Default unspecified error code.
    };
};

/**
 * @brief Converts an Error::Kind value to its string representation.
 * @details Returns a human-readable string for the given error kind. Returns
 * "Unspecified" for unknown kinds.
 * @param k The Error::Kind to convert.
 * @return const char* The string representation of the error kind.
 */
inline constexpr const char* kind_as_string(Error::Kind k) {
    switch (k) {
    case Error::Kind::Ok: return "Success";
    case Error::Kind::Err: return "Generic";
    case Error::Kind::IOErr: return "IO";
    case Error::Kind::RuntimeErr: return "Runtime";
    case Error::Kind::NetErr: return "Network";
    case Error::Kind::RpcErr: return "Rpc";
    case Error::Kind::TimeoutErr: return "Timeout";
    case Error::Kind::Invalid: return "Invalid";
    case Error::Kind::System: return "System";
    case Error::Kind::Unsupported: return "Unsupported";
    default: return "Unspecified";
    }
}

/**
 * @class TStatus
 * @brief Represents the status of an execution with error details.
 * @details A templated class that encapsulates an error kind, an optional error code,
 * and an optional description to represent the outcome of an operation.
 * @tparam TCode The type used for error codes (e.g., int).
 * @tparam TDesc The type used for error descriptions (e.g., std::string).
 */
template <typename TCode, typename TDesc>
struct TStatus {
    Error::Kind kind;          //!< The error category.
    std::optional<TCode> code; //!< Optional error code.
    std::optional<TDesc> desc; //!< Optional error description.

    /**
     * @brief Checks if the status indicates success.
     * @details Returns true if the error kind is Error::Kind::Ok, false otherwise.
     * @return bool True if the status is successful, false otherwise.
     */
    constexpr bool ok() const { return kind == Error::Kind::Ok; }

    /**
     * @brief Conversion operator to check if the status is successful.
     * @details Implicitly converts the status to a boolean, equivalent to calling ok().
     * @return bool True if the status is successful, false otherwise.
     */
    constexpr operator bool() const { return ok(); }

    /**
     * @brief Outputs the status to an output stream.
     * @details Formats the status as a string, including the error kind, and optionally
     * the error code and description if the status is not Ok.
     * @param out The output stream to write to.
     * @param status The TStatus object to output.
     * @return std::ostream& The modified output stream.
     */
    friend std::ostream& operator<<(std::ostream& out, const TStatus& status) {
        out << "[" << kind_as_string(status.kind) << "]";
        if (!status.ok()) {
            out << " Code=" << status.code.value_or(Error::Code<TCode>::Unspecified)
                << " Desc=\"" << (status.desc.has_value() ? status.desc.value() : "") << "\"";
        }
        return out;
    }

    /**
     * @brief Converts the status to a string representation.
     * @details Uses the stream output operator to create a string representation of the status.
     * @return std::string The string representation of the status.
     */
    std::string as_string() {
        std::ostringstream buf;
        buf << *this;
        return buf.str();
    }

    /**
     * @brief Constructs a successful status.
     * @details Initializes the status with Error::Kind::Ok and no code or description.
     * @snippet snp_status.cpp OK
     */
    TStatus()
        : kind{Error::Kind::Ok}
    {}

    /**
     * @brief Constructs an error status with an unspecified code.
     * @details Initializes the status with the specified error kind and an unspecified error code.
     * @param _kind The error kind (from Error::Kind).
     * @snippet snp_status.cpp Unspecified
     */
    TStatus(Error::Kind _kind)
        : kind{_kind}
        , code{Error::Code<TCode>::Unspecified}
    {}

    /**
     * @brief Constructs an error status with a description.
     * @details Initializes the status with a generic error kind (or specified kind),
     * an unspecified error code, and the provided description.
     * @param _desc The error description.
     * @param _kind The error kind (defaults to Error::Kind::Err).
     * @snippet snp_status.cpp Description
     */
    TStatus(const TDesc& _desc, Error::Kind _kind = Error::Kind::Err)
        : kind{_kind}
        , code{Error::Code<TCode>::Unspecified}
        , desc{_desc}
    {}

    /**
     * @brief Constructs an error status with a code.
     * @details Initializes the status with a generic error kind (or specified kind)
     * and the provided error code, with no description.
     * @param _code The error code.
     * @param _kind The error kind (defaults to Error::Kind::Err).
     * @snippet snp_status.cpp Errcode
     */
    TStatus(const TCode& _code, Error::Kind _kind = Error::Kind::Err)
        : kind{_kind}
        , code{_code}
    {}

    /**
     * @brief Constructs an error status with code and description.
     * @details Initializes the status with the specified error kind, code, and description.
     * @param _code The error code.
     * @param _desc The error description.
     * @param _kind The error kind (defaults to Error::Kind::Err).
     * @snippet snp_status.cpp CD
     */
    TStatus(const TCode& _code, const TDesc& _desc, Error::Kind _kind = Error::Kind::Err)
        : kind{_kind}
        , code{_code}
        , desc{_desc}
    {}
}; // struct TStatus

/**
 * @brief Type alias for a default TStatus with int code and std::string description.
 * @details Specializes TStatus for common use with integer error codes and string descriptions.
 */
using Status = TStatus<int, std::string>;

} // namespace tec
