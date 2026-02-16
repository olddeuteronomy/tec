// Time-stamp: <Last changed 2026-02-16 14:20:46 by magnolia>
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
 * @file tec_serialize.hpp
 * @brief The base interface for serializable objects.
 * @author The Emacs Cat
 * @date 2025-11-29
 */

#pragma once

#include <cstdint>
#include <ostream>
#include <string>
#include <type_traits>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {


/**
 * @brief Forward declaration of the network data buffer class.
 *
 * `NetData` is used as the streaming medium for binary serialization/deserialization.
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

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     *                       Binary serialization
     *
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

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

    // /**
    //  * @brief Convert this object to a JSON string representation.
    //  *
    //  * The returned string should be valid JSON and, when possible, human-readable
    //  * (pretty-printed). It is typically used for debugging, logging, or saving
    //  * configuration/state in text form.
    //  *
    //  * @return A std::string containing the JSON representation of the object.
    //  */
    // virtual std::string to_json() const = 0;
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

/**
 * @brief A unique `id` used for RPC calls.
 */
using rpcid_t = uint16_t;

/**
 * @brief
 */
struct NdRoot: public Serializable {
private:
    rpcid_t id_;

public:
    explicit NdRoot(rpcid_t _id)
        : id_{_id}
    {}

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

    // /**
    //  * @brief Overloads << to serialize TObject to JSON via TJson functor.
    //  *
    //  * @tparam TJson Functor for JSON serialization of TObject.
    //  * @tparam TObject Type to serialize.
    //  *
    //  * @param os Output stream.
    //  * @param obj Object to serialize.
    //  * @return Reference to os.
    //  */
    // template <typename TJson, typename TObject>
    // friend std::ostream& operator << (std::ostream& os, const TObject& obj) {
    //     return os << TJson{}(obj);
    // };
};

/**
 * @brief Overloads << to serialize TObject to JSON via TJson functor.
 *
 * @tparam TJson Functor for JSON serialization of TObject.
 * @tparam TObject Type to serialize.
 *
 * @param os Output stream.
 * @param obj Object to serialize.
 * @return Reference to os.
 */
// template <typename TJson, typename TObject>
// std::ostream& operator << (std::ostream& os, const TObject& obj) {
//     return os << TJson{}(obj);
// };

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
