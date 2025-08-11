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

#ifndef OMR_CCDATA_INLINES_INCL
#define OMR_CCDATA_INLINES_INCL

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <type_traits>
#include "omrcomp.h"
#endif

namespace TR {

template<typename T> CCData::key_t CCData::key(const T value)
{
#if __cpp_lib_has_unique_object_representations
    // std::has_unique_object_representations is only available in C++17.
    static_assert(std::has_unique_object_representations<T>::value == true,
        "T must have unique object representations.");
#endif
    return key(&value, sizeof(value));
}

template<typename T> bool CCData::put(const T value, const key_t * const key, index_t &index)
{
#if defined(__cplusplus) && (__cplusplus >= 201103L)
    static_assert(OMR_IS_TRIVIALLY_COPYABLE(T), "T must be trivially copyable.");
#endif
    return put(&value, sizeof(value), alignof(value), key, index);
}

template<typename T> bool CCData::get(const index_t index, T &value) const
{
#if defined(__cplusplus) && (__cplusplus >= 201103L)
    static_assert(OMR_IS_TRIVIALLY_COPYABLE(T), "T must be trivially copyable.");
#endif
    return get(index, &value, sizeof(value));
}

template<typename T> T *CCData::get(const index_t index) const
{
    // Don't have to check if T is trivially_copyable here since we're not copying to/from a T.
    // The caller might, but it is then their responsibility to make sure.
    if (index >= _capacity)
        return NULL;

    const size_t byteIndex = byteIndexFromDataIndex(index);

    return reinterpret_cast<T *>(_data + byteIndex);
}

inline bool CCData::put(const void * const value, const size_t sizeBytes, const size_t alignmentBytes,
    const key_t * const key, index_t &index)
{
    if (value == NULL) {
        return false;
    }
    return put_impl(value, sizeBytes, alignmentBytes, key, index);
}

inline bool CCData::reserve(const size_t sizeBytes, const size_t alignmentBytes, const key_t * const key,
    index_t &index)
{
    return put_impl(NULL, sizeBytes, alignmentBytes, key, index);
}

} // namespace TR

#endif // OMR_CCDATA_INLINES_INCL
