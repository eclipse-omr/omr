/*******************************************************************************
 * Copyright IBM Corp. and others 2022
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include <string.h>

#include "omrcomp.h"
#include "omrport.h"
#include "omr.h"

#include "EnvironmentBase.hpp"
#include "Forge.hpp"
#include "GCExtensionsBase.hpp"
#include "HashTableIterator.hpp"
#include "Math.hpp"
#include "ModronAssertions.h"
#include "SparseVirtualMemory.hpp"
#include "SparseAddressOrderedFixedSizeDataPool.hpp"

/****************************************
 * Initialization
 ****************************************
 */

MM_SparseVirtualMemory *
MM_SparseVirtualMemory::newInstance(MM_EnvironmentBase *env, uint32_t memoryCategory, MM_Heap *in_heap)
{
	MM_GCExtensionsBase *ext = env->getExtensions();
	MM_SparseVirtualMemory *vmem = NULL;
	vmem = (MM_SparseVirtualMemory *)env->getForge()->allocate(sizeof(MM_SparseVirtualMemory), OMR::GC::AllocationCategory::FIXED, OMR_GET_CALLSITE());

	if (vmem) {
		new (vmem) MM_SparseVirtualMemory(env, ext->sparseHeapPageSize, ext->sparseHeapPageFlags, in_heap);
		if (!vmem->initialize(env, memoryCategory)) {
			vmem->kill(env);
			vmem = NULL;
		}
	}

	return vmem;
}

bool
MM_SparseVirtualMemory::initialize(MM_EnvironmentBase *env, uint32_t memoryCategory)
{
	MM_GCExtensionsBase *ext = env->getExtensions();
	uintptr_t in_heap_size = (uintptr_t)_heap->getHeapTop() - (uintptr_t)_heap->getHeapBase();
	uintptr_t maxHeapSize = _heap->getMaximumMemorySize();
	uintptr_t regionSize = _heap->getHeapRegionManager()->getRegionSize();
	uintptr_t regionCount = in_heap_size / regionSize;

	/* Ideally this should be ceilLog2, however this is the best alternative as ceilLog2 currently does not exist */
	uintptr_t ceilLog2 = MM_Math::floorLog2(regionCount) + 1;

	uintptr_t off_heap_size = (uintptr_t)((ceilLog2 * in_heap_size) / 2);

	if (UDATA_MAX != ext->sparseHeapSizeRatio) {
		off_heap_size = MM_Math::roundToCeiling(regionSize, (in_heap_size / 100) * ext->sparseHeapSizeRatio);
	}

	bool success = false;

	_allocationContextArraySize = sizeof(void *) * (off_heap_size / regionSize);
	_allocationContextArray = (void **)env->getForge()->allocate(_allocationContextArraySize, OMR::GC::AllocationCategory::FIXED, OMR_GET_CALLSITE());
	if (NULL != _allocationContextArray) {
		memset(_allocationContextArray, 0, _allocationContextArraySize);
	} else {
		_allocationContextArraySize = 0;
		return success;
	}

	success = MM_VirtualMemory::initialize(env, off_heap_size, NULL, NULL, 0, memoryCategory);

	if (success) {
		void *sparseHeapBase = getHeapBase();
		_sparseDataPool = MM_SparseAddressOrderedFixedSizeDataPool::newInstance(env, sparseHeapBase, off_heap_size);
		if ((NULL == _sparseDataPool) || omrthread_monitor_init_with_name(&_largeObjectVirtualMemoryMutex, 0, "SparseVirtualMemory::_largeObjectVirtualMemoryMutex")) {
			success = false;
		}
	}

	if (success) {
		Trc_MM_SparseVirtualMemory_createSparseVirtualMemory_success((void *)_heap->getHeapTop(), (void *)_heap->getHeapBase(), (void *)in_heap_size, (void *)maxHeapSize, (void *)regionSize, regionCount, (void *)off_heap_size, _sparseDataPool);
	} else {
		Trc_MM_SparseVirtualMemory_createSparseVirtualMemory_failure((void *)_heap->getHeapTop(), (void *)_heap->getHeapBase(), (void *)in_heap_size, (void *)maxHeapSize, (void *)regionSize, regionCount, (void *)off_heap_size, _sparseDataPool);
	}

	return success;
}

void
MM_SparseVirtualMemory::tearDown(MM_EnvironmentBase *env)
{
	if (NULL != _allocationContextArray) {
		env->getForge()->free(_allocationContextArray);
		_allocationContextArray = NULL;
		_allocationContextArraySize = 0;
	}

	if (NULL != _sparseDataPool) {
		_sparseDataPool->kill(env);
		_sparseDataPool = NULL;
	}

	if (NULL != _largeObjectVirtualMemoryMutex) {
		omrthread_monitor_destroy(_largeObjectVirtualMemoryMutex);
		_largeObjectVirtualMemoryMutex = (omrthread_monitor_t) NULL;
	}

	MM_VirtualMemory::tearDown(env);
}

void
MM_SparseVirtualMemory::kill(MM_EnvironmentBase *env)
{
	tearDown(env);
	env->getForge()->free(this);
}

bool
MM_SparseVirtualMemory::updateSparseDataEntryAfterObjectHasMoved(void *dataPtr, void *oldProxyObjPtr, uintptr_t size, void *newProxyObjPtr)
{
	omrthread_monitor_enter(_largeObjectVirtualMemoryMutex);
	bool ret = _sparseDataPool->updateSparseDataEntryAfterObjectHasMoved(dataPtr, oldProxyObjPtr, size, newProxyObjPtr);
	omrthread_monitor_exit(_largeObjectVirtualMemoryMutex);
	return ret;
}

void *
MM_SparseVirtualMemory::allocateSparseFreeEntryAndMapToHeapObject(void *proxyObjPtr, uintptr_t size)
{
	uintptr_t adjustedSize = adjustSize(size);

	omrthread_monitor_enter(_largeObjectVirtualMemoryMutex);

	void *sparseHeapAddr = _sparseDataPool->findFreeListEntry(adjustedSize);

	if (NULL != sparseHeapAddr) {
		/* While the allocate and commit will work with _pageSize aligned memory, the map will contain exact size of the object.
		 * The size will be verified on updates.
		 */
		_sparseDataPool->mapSparseDataPtrToHeapProxyObjectPtr(sparseHeapAddr, proxyObjPtr, size);

		/* Zeroing (if needed at all) is safe to do after monitor release, since object has not been exposed yet. */
		omrthread_monitor_exit(_largeObjectVirtualMemoryMutex);

		bool success = MM_VirtualMemory::commitMemory(sparseHeapAddr, adjustedSize);

		if (success) {
#if defined(OSX) || defined(OMRZTPF)
			/* Most platforms will perform an implicit zero through the preceding commit. */
			OMRZeroMemory(sparseHeapAddr, adjustedSize);
#endif /* defined(OSX) || defined(OMRZTPF) */
			Trc_MM_SparseVirtualMemory_commitMemory_success(sparseHeapAddr, (void *)adjustedSize, proxyObjPtr);
		} else {
			Trc_MM_SparseVirtualMemory_commitMemory_failure(sparseHeapAddr, (void *)adjustedSize, proxyObjPtr);
		}
	} else {
		omrthread_monitor_exit(_largeObjectVirtualMemoryMutex);

		/* Although not a commit failure, the NULL value of sparseHeapAddr serves as a distinct trace point. */
		Trc_MM_SparseVirtualMemory_commitMemory_failure(NULL, (void *)adjustedSize, proxyObjPtr);
	}

	return sparseHeapAddr;
}

bool
MM_SparseVirtualMemory::freeSparseRegionAndUnmapFromHeapObject(MM_EnvironmentBase *env, void *dataPtr, void *proxyObjPtr, uintptr_t size, GC_HashTableIterator *sparseDataEntryIterator)
{
	uintptr_t dataSize = _sparseDataPool->findObjectDataSizeForSparseDataPtr(dataPtr);
	bool ret = true;

	if ((NULL != dataPtr) && (0 != dataSize)) {
		uintptr_t adjustedSize = adjustSize(dataSize);

		resetAllocationContextForAddress(dataPtr, dataSize);

		ret = decommitMemory(env, dataPtr, adjustedSize);
		if (ret) {
			omrthread_monitor_enter(_largeObjectVirtualMemoryMutex);
			ret = (_sparseDataPool->returnFreeListEntry(dataPtr, adjustedSize) && _sparseDataPool->unmapSparseDataPtrFromHeapProxyObjectPtr(dataPtr, proxyObjPtr, size, sparseDataEntryIterator));
			omrthread_monitor_exit(_largeObjectVirtualMemoryMutex);
			Trc_MM_SparseVirtualMemory_decommitMemory_success(dataPtr, (void *)adjustedSize);
		} else {
			Trc_MM_SparseVirtualMemory_decommitMemory_failure(dataPtr, (void *)adjustedSize);
			Assert_MM_unreachable();
		}
	}

	return ret;
}

bool
MM_SparseVirtualMemory::decommitMemory(MM_EnvironmentBase *env, void *address, uintptr_t size)
{
	bool ret = false;
	void *highValidAddress = (void *)((uintptr_t)address + size);
	ret = MM_VirtualMemory::decommitMemory(address, size, address, highValidAddress);

	return ret;
}
