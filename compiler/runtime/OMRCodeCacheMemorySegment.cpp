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

#include "runtime/CodeCacheMemorySegment.hpp"
#include "runtime/CodeCacheManager.hpp"
#include "thread_api.h"

TR::CodeCacheMemorySegment *OMR::CodeCacheMemorySegment::self()
{
    return static_cast<TR::CodeCacheMemorySegment *>(this);
}

void OMR::CodeCacheMemorySegment::adjustAlloc(int64_t adjust)
{
    /*
     * Assuming the code cache memory segment is allocated on the code cache memory,
     * we need to modify the memory's permission before and after updating members.
     */
    omrthread_jit_write_protect_disable();
    self()->setSegmentAlloc(self()->segmentAlloc() + adjust);
    omrthread_jit_write_protect_enable();
}

void OMR::CodeCacheMemorySegment::free(TR::CodeCacheManager *manager)
{
    /*
     * Assuming the code cache memory segment is allocated on the code cache memory,
     * we need to modify the memory's permission before and after updating members.
     */
    omrthread_jit_write_protect_disable();
    manager->freeMemory(_base);
    new (static_cast<TR::CodeCacheMemorySegment *>(this)) TR::CodeCacheMemorySegment();
    omrthread_jit_write_protect_enable();
}

void OMR::CodeCacheMemorySegment::setSegmentBase(uint8_t *newBase)
{
    /*
     * Assuming the code cache memory segment is allocated on the code cache memory,
     * we need to modify the memory's permission before and after updating members.
     */
    omrthread_jit_write_protect_disable();
    _base = newBase;
    omrthread_jit_write_protect_enable();
}

void OMR::CodeCacheMemorySegment::setSegmentAlloc(uint8_t *newAlloc)
{
    /*
     * Assuming the code cache memory segment is allocated on the code cache memory,
     * we need to modify the memory's permission before and after updating members.
     */
    omrthread_jit_write_protect_disable();
    _alloc = newAlloc;
    omrthread_jit_write_protect_enable();
}

void OMR::CodeCacheMemorySegment::setSegmentTop(uint8_t *newTop)
{
    /*
     * Assuming the code cache memory segment is allocated on the code cache memory,
     * we need to modify the memory's permission before and after updating members.
     */
    omrthread_jit_write_protect_disable();
    _top = newTop;
    omrthread_jit_write_protect_enable();
}
