// Time-stamp: <Last changed 2026-02-16 23:48:49 by magnolia>
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
 * @file tec_serialize.hpp
 * @brief The base interface for serializable objects.
 * @author The Emacs Cat
 * @date 2025-11-29
 */

#pragma once

#include <cstdint>
#include <string>
#include <type_traits>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *                       Binary serialization
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @brief Forward declaration of the network data buffer class.
 *
 * `NetData` is used as the streaming medium for binary serialization/deserialization.
 *
 * @see NetData
 */
class NetData;

/**
 * @brief Base interface for objects that support binary and JSON serialization.
 *
 * This is a pure abstract interface. Derived classes must
 * implement all pure virtual methods to be instantiable.
 *
 * @note All types that can be binary serialized must inherit from this class.
 */
struct Serializable {

    Serializable() = default;
    virtual ~Serializable() = default;

    /**
     * @brief Serialize this object into a binary stream.
     *
     * Implementations should write all necessary member data into the provided
     * `NetData` object in a deterministic order. The stream must remain valid
     * and writable for the duration of the call.
     *
     * @param s The output stream to write into (guaranteed to be non-const)
     * @return Reference to the modified stream (for chaining)
     */
    virtual NetData& store(NetData& s) const = 0;

    /**
     * @brief Deserialize this object from a binary stream.
     *
     * Implementations must read exactly the data that was previously written by
     * `store()`. The stream is expected to contain valid data produced by a
     * compatible version of `store()`.
     *
     * @param s The input stream to read from (guaranteed to be non-const)
     * @return Reference to the modified stream (for chaining)
     */
    virtual NetData& load(NetData& s) = 0;
};

/**
 * @brief Variable template to check at compile-time whether a type is serializable.
 *
 * A type `T` is considered serializable if it publicly inherits from `tec::Serializable`.
 *
 * @tparam T Type to test
 *
 * @code{.cpp}
 * struct MyComponent : tec::Serializable { ... };
 * static_assert(tec::is_serializable_v<MyComponent>);
 * static_assert(!tec::is_serializable_v<int>);
 * @endcode
 */
template <typename T>
inline constexpr bool is_serializable_v = std::is_base_of<Serializable, T>::value;


/// @brief Type alias for RPC identifiers, represented as a 16-bit unsigned integer.
using rpcid_t = uint16_t;

/**
 * @brief Root class for serializable objects used in RPC communications.
 *
 * This struct serves as the base class for all serializable objects involved in
 * RPC (Remote Procedure Call) communications. It provides a unique identifier
 * for each RPC instance, ensuring proper tracking, dispatching and serialization.
 *
 * @note This class inherits from Serializable to enable serialization capabilities.
 */
struct NdRoot: public Serializable {
private:
    rpcid_t id_;

public:
    /**
     * @brief Constructs an NdRoot instance with the specified RPC identifier.
     *
     * This explicit constructor initializes the object with a given RPC ID.
     *
     * @param _id The RPC identifier to assign to this object.
     */
    explicit NdRoot(rpcid_t _id)
        : id_{_id}
    {}

    /**
     * @brief Retrieves the RPC identifier.
     *
     * This constexpr member function returns the stored RPC ID. It is marked
     * as constexpr to allow evaluation at compile-time when possible.
     *
     * @return The RPC identifier.
     */
    constexpr rpcid_t id() const { return id_; }
};

/**
 * @brief Variable template to check at compile-time whether a type is
 *        a root object for binary serizalization.
 *
 * A type `T` is considered *a root object* if it publicly inherits from `tec::NdRoot`.
 *
 * @tparam T Type to test
 */
template <typename T>
inline constexpr bool is_root_v = std::is_base_of<NdRoot, T>::value;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *                       JSON serialization
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @brief Base interface for objects that support JSON serialization.
 *
 * This is a pure abstract interface. Derived classes must
 * implement all pure virtual methods to be instantiable.
 */
struct JsonSerializable {

    JsonSerializable() = default;
    virtual ~JsonSerializable() = default;

    /**
     * @brief Convert this object to a JSON string representation.
     *
     * The returned string should be valid JSON.
     * It is typically used for debugging, logging, or saving
     * configuration/state in text form.
     *
     * @return A std::string containing the JSON representation of the object.
     */
    virtual std::string to_json() const = 0;
};

/**
 * @brief Variable template to check at compile-time whether a type is JSON serializable.
 *
 * A type `T` is considered serializable if it publicly inherits from
 * `tec::JsonSerializable`.
 *
 * @tparam T Type to test
 */
template <typename T>
inline constexpr bool is_json_serializable_v = std::is_base_of<JsonSerializable, T>::value;


} // namespace tec
