// Time-stamp: <Last changed 2025-12-04 01:03:03 by magnolia>
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
 * @file tec_bytes.hpp
 * @brief A byte buffer class with stream-like read/write semantics.
 * @author The Emacs Cat
 * @date 2025-11-14
 */


#pragma once

#include <cstddef>
#include <array>
#include <vector>
#include <sstream>

#include <stdio.h>
#include <memory.h>

#include "tec/tec_def.hpp" // IWYU pragma: keep


namespace tec {

/**
 * @brief Converts a byte to a 2-character representation suitable for hex dumps.
 *
 * Returns a 2-element array containing either:
 * - The two hexadecimal digits (uppercase) representing the byte value, or
 * - The literal ASCII character preceded by a space, **but only for printable ASCII characters**
 *   in the range 0x21–0x7E (`!` to `~`).
 *   Space (0x20) is deliberately treated as non-printable and rendered in hex.
 *
 * This format is commonly used in classic hex dump tools (e.g. `hexdump -C`, `xxd`, debug output)
 * where printable characters are shown directly and everything else in hex.
 *
 * @param value The byte to convert.
 *
 * @return std::array<char, 2>
 *         A fixed-size array with two characters:
 *         - For printable ASCII (except space): `{ ' ', actual_char }`
 *         - For all other values (including space, control chars, and high bytes): `{ '0'..'F', '0'..'F' }`
 *
 * @par Examples
 * @code
 *     to_hex_chars(std::byte{65});   // 'A'  → {' ', 'A'}
 *     to_hex_chars(std::byte{32});   // ' '  → {'2', '0'}   (space is forced to hex)
 *     to_hex_chars(std::byte{10});   // '\n' → {'0', 'A'}
 *     to_hex_chars(std::byte{255});  // 0xFF → {'F', 'F'}
 * @endcode
 *
 * @note The function is `constexpr` and `inline`, making it usable in both
 *       runtime and compile-time contexts with zero overhead.
 *
 * @see as_hex()
 */
inline constexpr std::array<char, 2> to_hex_chars(std::byte value) noexcept {
    constexpr char table[] = "0123456789ABCDEF";
    int i = std::to_integer<int>(value);
    if(0x20 < i && i < 0x7F) {
        // ASCII characters, except SPC.
        return {' ', char(i & 0xFF)};
    }
    else {
        // Non-printable.
        return { table[i >> 4], table[(i & 0x0F)] };
    }
}


/**
 * @brief A byte buffer class with stream-like read/write semantics.
 *
  * The `Bytes` class provides a dynamically resizing buffer for sequential
 * reading and writing of elements of type `std::byte`. It mimics the behavior of file
 * streams using `seek`, `tell`, `read`, and `write` operations. The buffer grows
 * in blocks of a configurable size to minimize reallocations.
 */
class Bytes {
public:
    /** @brief Default block size for buffer expansion, matches `BUFSIZ` from `<stdio.h>`. */
    static constexpr size_t kDefaultBlockSize{BUFSIZ};

private:
    /** @brief Underlying storage for buffer elements. */
    std::vector<std::byte> buffer_;

    /** @brief Size of each allocation block used when expanding the buffer. */
    size_t blk_size_;

    /** @brief Current read/write position within the buffer (like a file pointer). */
    size_t pos_;

    /** @brief Logical size of the data written into the buffer (may be less than capacity). */
    size_t size_;

private:
    /** @brief Calculates required capacity to hold extra `len` elemants starting from `pos`. */
    size_t calc_required_capacity(long pos, size_t len) {
       return buffer_.capacity()
                + blk_size_
                + ((pos + len) / blk_size_) * blk_size_;
    }

public:
    /**
     * @brief Constructs a buffer with the specified block size.
     *
     * The initial capacity is set to one block. The buffer starts empty with
     * position at 0.
     *
     * @param block_size Size of each growth block. Must be greater than 0.
     *                   Defaults to `kDefaultBlockSize`.
     */
    explicit Bytes(size_t block_size = kDefaultBlockSize)
        : buffer_(block_size)
        , blk_size_{block_size}
        , pos_{0}
        , size_{0}
    {}

    Bytes(const void* src, size_t len, size_t block_size = kDefaultBlockSize)
        : buffer_(block_size)
        , blk_size_{block_size}
        , pos_{0}
        , size_{0}
    {
        write(src, len);
    }

    virtual ~Bytes() = default;

    std::byte* data() {
        return buffer_.data();
    }

    const std::byte* data() const {
        return buffer_.data();
    }

    std::byte* at(size_t pos) {
        return &buffer_.at(pos);
    }

    const std::byte* at(size_t pos) const {
        return &buffer_.at(pos);
    }

    /**
     * @brief Returns the block size used for buffer expansion.
     * @return The current block size in number of elements.
     */
    size_t block_size() const {
        return blk_size_;
    }

    /**
     * @brief Returns the logical size of the data in the buffer.
     *
     * This is the amount of data that has been written and is readable.
     * It may be less than `capacity()`.
     *
     * @return Number of elements currently stored.
     */
    size_t size() const {
        return size_;
    }

    /**
     * @brief Returns the current capacity of the underlying storage.
     * @return Total number of elements the buffer can hold without reallocation.
     */
    size_t capacity() const {
        return buffer_.capacity();
    }

    /**
     * @brief Returns the current read/write position.
     * @return The current position as a signed offset from the start.
     */
    long tell() const {
        return pos_;
    }

    /**
     * @brief Resets the read/write position to the beginning of the buffer.
     */
    void rewind() {
        pos_ = 0;
    }

    /**
     * @brief Moves the read/write position relative to a reference point.
     *
     * Behaves like `fseek`. Supports `SEEK_SET`, `SEEK_CUR`, and `SEEK_END`.
     *
     * @param offset Number of elements to move (can be negative).
     * @param whence Reference point:
     *               - `SEEK_SET`: from beginning of buffer
     *               - `SEEK_CUR`: from current position
     *               - `SEEK_END`: from end of data
     * @return 0 on success, -1 if seeking before start, -2 if seeking beyond end.
     */
    int seek(long offset, int whence) {
        long origin = (whence == SEEK_CUR ? pos_ : (whence == SEEK_END ? size_ : 0));
        size_t new_pos = origin + offset;
        if( new_pos < 0) {
            return -1;
        }
        if( new_pos > size_) {
            return -2;
        }
        pos_ = new_pos;
        return 0;
    }

    void resize(size_t size) {
        if( size > size_ ) {
            size_t new_cap = calc_required_capacity(0, size);
            buffer_.reserve(new_cap);
            size_ = size;
        }
    }

    /**
     * @brief Writes data into the buffer at the current position.
     *
     * The buffer automatically expands if necessary. Expansion occurs in
     * multiples of `blk_size_` to reduce reallocations.
     *
     * @param src Pointer to the source data.
     * @param len Number of elements to write.
     * @return Number of elements actually written. Returns 0 on failure
     *         (e.g., out of memory or `len == 0`).
     */
    size_t write(const void* src, size_t len) {
        if( len == 0 ) {
            return 0;
        }
        if( pos_ + len >= buffer_.capacity() ) {
            // Expand the buffer with extra blocks.
            size_t new_cap = calc_required_capacity(pos_, len);
            if( new_cap > buffer_.max_size() ) {
                return 0;
            }
            buffer_.reserve(new_cap);
        }
        ::memcpy(buffer_.data() + pos_, src, len);
        pos_ += len;
        if( pos_ > size_ ) {
            size_ = pos_;
        }
        return len;
     }

    /**
     * @brief Reads data from the buffer starting at the current position.
     *
     * Does not read past the logical end of data (`size_`).
     *
     * @param dst Pointer to the destination buffer.
     * @param len Maximum number of elements to read.
     * @return Number of elements actually read. Returns 0 if at end of data
     *         or if `len == 0`.
     */
    size_t read(void* dst, size_t len) {
        if( len == 0 ) {
            return 0;
        }
        if( pos_ + len > size_ ) {
            // Reading out of bound.
            return 0;
        }
        ::memcpy(dst, buffer_.data() + pos_, len);
        pos_ += len;
        return len;
    }


    /**
     * @brief Returns a human-readable hex+ASCII dump string.
     *
     * Each byte is represented by exactly two characters:
     * - Printable ASCII characters in the range 0x21–0x7E (`!` to `~`)
     *   are shown as space + the character itself.
     * - All other bytes (including space 0x20, control characters, DEL,
     *   and values ≥ 0x7F) are shown as two uppercase hexadecimal digits.
     *
     * This format is extremely useful for debugging binary data,
     * network packets, or memory contents, because it makes text-like
     * regions immediately recognizable while still showing exact byte
     * values.
     *
     * @return std::string
     *         A string of length exactly `2 * size()`,
     *         containing the mixed hex/ASCII representation.
     *
     * @par Complexity
     *      O(N) where N = `size()`, with a very tight constant factor.
     *
     * @par Example
     * @code
     *     const char hello[] = "Hello\x01\x02World\xFF\0";
     *     Bytes data(hello, strlen(hello));
     *     // data contains: 48 65 6C 6C 6F 01 02 57 6F 72 6C 64 FF
     *     std::cout << data.as_hex() << '\n';
     *     // Output: " H e l l o010203 W o r l dFF"
     * @endcode
     *
     * @note Space (0x20) is deliberately rendered as hex `"20"`, not as `' ' + ' '`,
     *       which matches classic hex dump tools and avoids ambiguity.
     *
     * @see to_hex_chars()
     */
    std::string as_hex() const {
        std::ostringstream os;
        auto ptr = data();
        for(size_t n = 0 ; n < size() ; ++n, ++ptr) {
           auto ch = to_hex_chars(*ptr);
           os << ch[0] << ch[1];
        }
        return os.str();
    }

};


} // namespace tec
