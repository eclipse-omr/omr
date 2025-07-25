/*******************************************************************************
 * Copyright IBM Corp. and others 2000
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

#ifndef TR_SEGMENT_POOL
#define TR_SEGMENT_POOL

#pragma once

#include <deque>
#include <stack>
#include "env/TypedAllocator.hpp"
#include "infra/ReferenceWrapper.hpp"
#include "env/SegmentProvider.hpp"
#include "env/RawAllocator.hpp"

namespace TR {

/**
 * @brief The SegmentPool class maintains a pool of memory segments.
 */

class SegmentPool : public TR::SegmentProvider {
public:
    SegmentPool(TR::SegmentProvider &backingProvider, size_t cacheSize, TR::RawAllocator rawAllocator);
    ~SegmentPool() throw();

    virtual TR::MemorySegment &request(size_t requiredSize);
    virtual void release(TR::MemorySegment &) throw();

private:
    size_t const _poolSize;
    size_t _storedSegments;
    TR::SegmentProvider &_backingProvider;

    typedef TR::typed_allocator<TR::reference_wrapper<TR::MemorySegment>, TR::RawAllocator> DequeAllocator;

    typedef std::deque<TR::reference_wrapper<TR::MemorySegment>, DequeAllocator> StackContainer;

    typedef std::stack<TR::reference_wrapper<TR::MemorySegment>, StackContainer> SegmentStack;

    SegmentStack _segmentStack;
};

} // namespace TR

#endif // TR_SEGMENT_POOL
