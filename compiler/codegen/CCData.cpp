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

#include "omrcomp.h"
#include "OMR/Bytes.hpp"

#include "codegen/CCData.hpp"

#include "infra/Monitor.hpp"
#include "infra/CriticalSection.hpp"

using TR::CCData;

size_t CCData::dataSizeFromBytesSize(size_t sizeBytes) { return (sizeBytes + sizeof(data_t) - 1) / sizeof(data_t); }

size_t CCData::dataAlignmentFromBytesAlignment(size_t alignmentBytes)
{
    return (alignmentBytes + OMR_ALIGNOF(data_t) - 1) / OMR_ALIGNOF(data_t);
}

size_t CCData::byteIndexFromDataIndex(size_t dataIndex) { return dataIndex * sizeof(data_t); }

CCData::key_t CCData::key(const void * const data, const size_t sizeBytes)
{
    return key_t(static_cast<const key_t::value_type *>(data), sizeBytes);
}

CCData::key_t CCData::key(const char * const str) { return key_t(static_cast<const key_t::value_type *>(str)); }

CCData::CCData(void * const storage, const size_t sizeBytes)
    : _putIndex(0)
    , _lock(TR::Monitor::create("CCDataMutex"))
{
    if (sizeBytes > 0) {
        void *alignedStorage = storage;
        size_t sizeBytesAfterAlignment = sizeBytes;
        bool success = OMR::align(OMR_ALIGNOF(data_t), sizeof(data_t), alignedStorage, sizeBytesAfterAlignment) != NULL;
        TR_ASSERT_FATAL(success, "Can't align CCData storage to required boundary");
        _data = static_cast<char *>(alignedStorage);
        _capacity = dataSizeFromBytesSize(sizeBytesAfterAlignment);
    } else {
        _data = NULL;
        _capacity = 0;
    }
}

bool CCData::put_impl(const void * const value, const size_t sizeBytes, const size_t alignmentBytes,
    const key_t * const key, index_t &index)
{
    const OMR::CriticalSection critsec(_lock);

    /**
     * Multiple compilation threads may be attempting to update the same value with
     * the same key.  Once we have the mutex, check if the value already exists and
     * return successfully.
     */
    if (key && find_unsafe(*key, &index)) {
        return true;
    }

    // The following feels like it could be simplified and calculated more efficiently. If you're reading this, have a
    // go at it.
    size_t putByteIndex = byteIndexFromDataIndex(_putIndex);
    const size_t sizeDataUnits = dataSizeFromBytesSize(sizeBytes);
    const size_t alignmentDataUnits = dataAlignmentFromBytesAlignment(alignmentBytes);
    const size_t alignmentMask = alignmentDataUnits - 1;
    const size_t alignmentPadding
        = (alignmentDataUnits
              - ((reinterpret_cast<uintptr_t>(_data + putByteIndex) / OMR_ALIGNOF(data_t)) & alignmentMask))
        & alignmentMask;
    const size_t remainingCapacity = _capacity - _putIndex;

    if (sizeDataUnits + alignmentPadding > remainingCapacity)
        return false;

    _putIndex += alignmentPadding;
    index = _putIndex;
    putByteIndex = byteIndexFromDataIndex(_putIndex);

    if (key != NULL)
        _mappings[*key] = _putIndex;

    if (value != NULL) {
        std::copy(static_cast<const char *>(value), static_cast<const char *>(value) + sizeBytes, _data + putByteIndex);
    }

    _putIndex += sizeDataUnits;

    return true;
}

bool CCData::get(const index_t index, void * const value, const size_t sizeBytes) const
{
    if (index >= _capacity)
        return false;

    const size_t byteIndex = byteIndexFromDataIndex(index);

    std::copy(_data + byteIndex, _data + byteIndex + sizeBytes, static_cast<char *>(value));

    return true;
}

bool CCData::find(const key_t key, index_t * const index) const
{
    const OMR::CriticalSection critsec(_lock);
    return find_unsafe(key, index);
}

bool CCData::find_unsafe(const key_t key, index_t * const index) const
{
    auto e = _mappings.find(key);
    if (e != _mappings.end()) {
        if (index != NULL)
            *index = e->second;
        return true;
    }

    return false;
}
