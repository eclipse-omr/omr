/*******************************************************************************
 * Copyright IBM Corp. and others 2020
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#ifndef OMR_CCDATA_INCL
#define OMR_CCDATA_INCL

#include <map>
#include <string>
#include <cstddef>
#include <OMR/TypeTraits.hpp>

namespace TR {
class Monitor;
}

namespace TR {

/**
 * @class CCData
 *
 * @brief This class represents a table that can be used to store data that will be referenced from generated code.
 *
 * When generating code, we typically have the option to embed constants in the instruction stream in the form of
 * immediate operands, data snippets, and by allocating chunks of memory directly from the code cache. Alternatively,
 * data can be allocated as static variables accessible via the symbol table. This class represents a third method for
 * allocating data in a single, contiguous table. Allocations can specify custom alignment requirements and optionally
 * be associated with a key.
 *
 * The intention is for CCData tables to store constants, addresses, and other site-specific data, such as inline caches
 * separately from the instruction stream itself such that the code can be made read-only (i.e. never needs to be
 * patched), can be made position-independent, and process-independent.
 *
 * The memory backing a CCData table must be allocated externally and should be done so to facilitate access from a
 * particular code cache. On architectures that provide PC-relative data access instructions, the memory should be
 * allocated within the addressible distance of those instructions.
 *
 * Alternatively, a register may be used as a base pointer, however note that currently data is allocated from the table
 * at increasing addresses, which may be sub-optimal for architectures that use signed displacements to access data.
 */
class CCData {
private:
    /** \typedef data_t Implementation detail. This type represents the units of the table. Typically bytes, but can be
     * some other data type, as long as it can be default-constructed. */
    typedef char data_t;

public:
    /** \typedef index_t This type represents the indices defined in the public interface of this class. They must
     * behave like integral types. */
    typedef size_t index_t;

    /** \typedef key_t This type represents the keys defined in the public interface of this class. The constructor is
     * unspecified, use CCData_t::key() to create keys. */
    typedef std::string key_t;

    static_assert(OMR::IsSame<key_t::value_type, char>::VALUE,
        "Keys need to be constructible from a sequence of chars.");

private:
    /** \typedef map_t Implementation detail. This type represents the associative container that maps keys to indices.
     * It must behave like std::unordered_map. */
    typedef std::map<key_t, index_t> map_t;

public:
    /**
     * @brief Creates a key_t from the given value. The value's type must have a unique object representation.
     *
     * Unique Object Representations
     *
     * Structs and classes usually have padding bytes which are going to be included in their memory representations.
     * Even if the padding is at the end of the instance, sizeof() will include it. Calculating a key based on the
     * memory representation is going to include the padding, which is incorrect. If the key includes the padding, two
     * otherwise equal objects will result in different keys if their padding bytes differ.
     *
     * This also applies to floats and doubles because of various "don't care" (DC) bits in the binary representation.
     * Logically equal float values might have different bit patterns and would result in different keys.
     *
     * Types that don't have any padding or DC bits have unique object representations in C++ standard parlance.
     *
     * Guidance
     *
     * On capable (C++17) compilers, a static assertion will check for the std::has_unique_object_representations<T>
     * type trait. On incapable compilers, only simple integrals should be used to create keys. Avoid floats, doubles,
     * structs, classes, unions, and any other type who's object representation may include undefined bits or bytes.
     *
     * @tparam T The type of the value to create the key from. The type must have a unique object representation.
     * @param[In] value The value to create the key from.
     * @return The key.
     */
    template<typename T> static key_t key(const T value);

    /**
     * @brief Creates a key_t from the given buffer of data. The entire buffer will be used as input, including any
     * unused bits/bytes that you may not be aware of. See key(const T value) for info on why this is important to note.
     *
     * @param[In] data A pointer to the buffer of data.
     * @param[In] sizeBytes The size (in bytes) of the data.
     * @return The key.
     */
    static key_t key(const void * const data, const size_t sizeBytes);

    /**
     * @brief Creates a key_t from the given null-terminated C string.
     *
     * @param[In] data A pointer to the string.
     * @return The key.
     */
    static key_t key(const char * const str);

    /**
     * @brief Constructs a CCData and accepts a pointer to a memory buffer that will be used to hold the data.
     *
     * The lifetime of the memory buffer must exceed the lifetime of the resulting CCData object. The memory buffer will
     * still be valid after the CCData object is destructed (i.e. the memory buffer will not be freed, do it yourself).
     *
     * @param[In] storage A pointer to a memory buffer that CCData will use.
     * @param[In] sizeBytes The amount of data (in bytes) that the buffer contains.
     */
    CCData(void * const storage, const size_t sizeBytes);

    /**
     * @brief Puts the given value in the table, optionally mapped to the given key (if any), aligned to the value's
     * natural type, and returns the index to the value. Synchronized.
     *
     * @tparam T The type of the value to put in the table. The type must be TriviallyCopyable, otherwise the runtime
     * behaviour is undefined. A static assertion checks for the std::is_trivially_copyable<T> type trait on capable
     * compilers.
     * @param[In] value The value to put.
     * @param[In] key Optional. The key to map the resulting index to. Without a key the index is the only reference to
     * the data.
     * @param[Out] index The index that refers to the value.
     * @return True if the value exists in the table, or the key was already mapped to an index, false otherwise.
     */
    template<typename T> bool put(const T value, const key_t * const key, index_t &index);

    /**
     * @brief Puts the given value in the table, optionally mapped to the given key (if any), aligned to the given
     * boundary (in bytes), and returns the index to the value. Synchronized.
     *
     * @param[In] value A pointer to the value to put.
     * @param[In] sizeBytes The size of the value pointed to.
     * @param[In] alignmentBytes The alignment (in bytes) to align the value to.
     * @param[In] key Optional. The key to map the resulting index to. Without a key the index is the only reference to
     * the data. If the key is already mapped to an index the operation will return the index and true, but no data will
     * be written.
     * @param[Out] index The index that refers to the value.
     * @return True if the value pointer is non-NULL, and either the value exists in the table, or the key was already
     * mapped to an index, false otherwise.
     */
    bool put(const void * const value, const size_t sizeBytes, const size_t alignmentBytes, const key_t * const key,
        index_t &index);

    /**
     * @brief Reserves space in the table, optionally mapped to the given key (if any), aligned to the given boundary
     (in bytes), and returns the index to the space. Synchronized.

     * @param[In] sizeBytes The size to reserve.
     * @param[In] alignmentBytes The alignment (in bytes) to align the reserved space to.
     * @param[In] key Optional. The key to map the resulting index to. Without a key the index is the only reference to
     the data. If the key is already mapped to an index the operation will return the index and true, but no space will
     be reserved.
     * @param[Out] index The index that refers to the space.
     * @return True if the space was reserved, or the key was already mapped to an index, false otherwise.
     */
    bool reserve(const size_t sizeBytes, const size_t alignmentBytes, const key_t * const key, index_t &index);

    /**
     * @brief Gets the value referred to by the index from the table.
     *
     * @param[In] T The type of the value to get from the table. The type must be TriviallyCopyable, otherwise the
     * runtime behaviour is undefined. A static assertion checks for the std::is_trivially_copyable<T> type trait on
     * capable compilers.
     * @param[In] index The index that refers to the value.
     * @param[Out] value A reference to the value to write the result to.
     * @return True if the value exists in the table, false otherwise.
     */
    template<typename T> bool get(const index_t index, T &value) const;

    /**
     * @brief Gets the value referred to by the index from the table.
     *
     * @param[In] index The index that refers to the value.
     * @param[Out] value A pointer to the value to write the result to. This parameter is ignored and the value is not
     * changed unless the operation succeeds and this function returns true.
     * @param[In] sizeBytes The size (in bytes) of the value pointed to. This parameter is ignored unless the operation
     * succeeds and this function returns true.
     * @return True if the index refers to an existing value, false otherwise.
     */
    bool get(const index_t index, void * const value, const size_t sizeBytes) const;

    /**
     * @brief Gets a pointer to the value referred to by the index from the table.
     *
     * @param[In] T The type of the value to get from the table.
     * @param[In] index The index that refers to the value.
     * @return A pointer to the value if the index refers to an existing value, NULL otherwise.
     */
    template<typename T> T *get(const index_t index) const;

    /**
     * @brief Checks if the given key maps to an index in this table and returns the index. Synchronized.
     *
     * @param[In] key The key to check.
     * @param[Out] index Optional. A pointer to write the index to. This parameter is ignored unless this function
     * returns true.
     * @return True if the given key maps to an index, false otherwise.
     */
    bool find(const key_t key, index_t * const index = NULL) const;

private:
    /**
     * @brief Checks if the given key maps to an index in this table and returns the index.
     *        This function is NOT synchronized (hence unsafe).
     *
     * @param[In] key The key to check.
     * @param[Out] index Optional. A pointer to an index to write to. This parameter is ignored and the index is not
     * changed unless the operation succeeds and this function returns true.
     * @return True if the given key maps to an index, false otherwise.
     */
    bool find_unsafe(const key_t key, index_t * const index = NULL) const;

    /**
     * @brief This function implements the actual work behind both put() and reserve().
     *
     * @param[In] value Optional. A pointer to the value to put.
     * @param[In] sizeBytes The size of the value pointed to.
     * @param[In] alignmentBytes The alignment (in bytes) to align the value to.
     * @param[In] key Optional. The key to map the resulting index to. Without a key the index is the only reference to
     * the data. If the key is already mapped to an index the operation will return the index and true, but no data will
     * be written.
     * @param[Out] index The index that refers to the value.
     * @return True if the value exists in the table, or the key was already mapped to an index, false otherwise.
     */
    bool put_impl(const void * const value, const size_t sizeBytes, const size_t alignmentBytes,
        const key_t * const key, index_t &index);

    /**
     * @brief Converts a size in units of bytes to a size in units of data_t.
     *
     * @param[In] sizeBytes A size in bytes.
     * @return A size in units of data_t.
     */
    static size_t dataSizeFromBytesSize(size_t sizeBytes);

    /**
     * @brief Converts an alignment in units of bytes to an alignment in units of data_t.
     *
     * @param[In] alignmentBytes An alignment in bytes.
     * @return An alignment in units of data_t.
     */
    static size_t dataAlignmentFromBytesAlignment(size_t alignmentBytes);

    /**
     * @brief Converts a data index into a byte index.
     *
     * @param[In] dataIndex An index into an array of data_t elements.
     * @return The equivalent index into an array of bytes.
     */
    static size_t byteIndexFromDataIndex(index_t dataIndex);

private:
    char *_data;
    size_t _capacity;
    size_t _putIndex;
    map_t _mappings;
    TR::Monitor *_lock;
};

} // namespace TR

#endif // OMR_CCDATA_INCL
