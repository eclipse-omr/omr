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

/**
 * @file
 * @ingroup GC_Base_Core
 */

#if !defined(SPARSEVIRTUALMEMORY_HPP_)
#define SPARSEVIRTUALMEMORY_HPP_

#include "omrcomp.h"
#include "omrport.h"
#include "modronbase.h"
#include "modronopt.h"

#include "BaseVirtual.hpp"
#include "Heap.hpp"
#include "HeapRegionManager.hpp"
#include "ModronAssertions.h"
#include "VirtualMemory.hpp"

class GC_HashTableIterator;
class MM_GCExtensions;
class MM_GCExtensionsBase;
class MM_Heap;
class MM_SparseAddressOrderedFixedSizeDataPool;
struct J9PortVmemParams;

/**
 * Large virtual memory for allocation of large objects (their data portion). It's sparsely
 * committed only for live objects (memory is eagerly committed/de-committed on allocate/free).
 *
 * It also maintains mapping from the data portion to the in-heap object pointer.
 * The reverse mapping is not maintained (it's expected that the heap object has an easy way to
 * find the data portion, for example, via the dataAddr pointer in the object header).
 *
 * @ingroup GC_Base_Core
 */
class MM_SparseVirtualMemory : public MM_VirtualMemory {
/*
 * Data members
 */
private:
	MM_Heap *_heap; /**< reference to in-heap */
	MM_SparseAddressOrderedFixedSizeDataPool *_sparseDataPool; /**< Structure that manages data and free region of sparse virtual memory */
	omrthread_monitor_t _largeObjectVirtualMemoryMutex; /**< Monitor that manages access to sparse virtual memory */

	void **_allocationContextArray;
	uintptr_t _allocationContextArraySize;
protected:
public:
/*
 * Function members
 */
private:
	MMINLINE uintptr_t adjustSize(uintptr_t size)
	{
		/* Committing and de-committing memory sizes must be a multiple of region size. */
		return MM_Math::roundToCeiling(_heap->getHeapRegionManager()->getRegionSize(), size);
	}

protected:
	bool initialize(MM_EnvironmentBase *env, uint32_t memoryCategory);
	void tearDown(MM_EnvironmentBase *env);

	MM_SparseVirtualMemory(MM_EnvironmentBase *env, uintptr_t pageSize, uintptr_t pageFlags, MM_Heap *in_heap)
		: MM_VirtualMemory(env, in_heap->getHeapRegionManager()->getRegionSize(), pageSize, pageFlags, 0, OMRPORT_VMEM_MEMORY_MODE_READ | OMRPORT_VMEM_MEMORY_MODE_WRITE)
		, _heap(in_heap)
		, _sparseDataPool(NULL)
		, _largeObjectVirtualMemoryMutex(NULL)
		, _allocationContextArray(NULL)
		, _allocationContextArraySize(0)
	{
		_typeId = __FUNCTION__;
	}

public:
	static MM_SparseVirtualMemory *newInstance(MM_EnvironmentBase *env, uint32_t memoryCategory, MM_Heap *in_heap);
	virtual void kill(MM_EnvironmentBase *env);

	/**
	 * After the in-heap proxy object pointer has moved, update the proxyObjPtr for the sparse data entry associated with the dataPtr.
	 * Verify if the entry is consistent(the size and associated the object) before updating.
	 * Assert if no entry is found or the verifying is failed.
	 *
	 * @param dataPtr          void*       Data pointer
	 * @param oldproxyObjPtr   void*       Proxy object associated with dataPtr
	 * @param size             uintptr_t   Size of region consumed by dataPtr
	 * @param newProxyObjPtr   void*       Updated in-heap proxy object pointer for the data pointer
	 *
	 * @return true if the table entry was successfully updated, false otherwise
	 */
	bool updateSparseDataEntryAfterObjectHasMoved(void *dataPtr, void *oldProxyObjPtr, uintptr_t size, void *newProxyObjPtr);
	/**
	 * Find free space at sparse heap address space that satisfies the given size
	 *
	 * @param size		uintptr_t	size requested by object pointer to be allocated at sparse heap
	 * @param proxyObjPtr	void*	Proxy object that will be associated to the data at sparse heap
	 *
	 * @return data pointer at sparse heap that satisfies the requested size
	 */
	void *allocateSparseFreeEntryAndMapToHeapObject(void *proxyObjPtr, uintptr_t size);

	/**
	 * Once object is collected by GC, we need to free the sparse region associated
	 * with the object pointer. Therefore we decommit sparse region and return free
	 * region to the sparse free region pool.
	 * Verify if the entry is consistent(the size and associated the object) before freeing.
	 * Assert if no entry is found or the verifying is failed.
	 *
	 * @param dataPtr       void*       Data pointer
	 * @param proxyObjPtr   void*       Proxy object associated with dataPtr
	 * @param size          uintptr_t   Size of region consumed by dataPtr
 	 * @param sparseDataEntryIterator GC_HashTableIterator*   if it is not NULL, For UnmapFromHeapObject
	 *
	 * @return true if region associated to object was decommited and freed successfully, false otherwise
	 */
	bool freeSparseRegionAndUnmapFromHeapObject(MM_EnvironmentBase *env, void *dataPtr, void *proxyObjPtr, uintptr_t size, GC_HashTableIterator *sparseDataEntryIterator = NULL);
	/**
	 * Decommits/Releases memory, returning the associated pages to the OS
	 *
	 * @param address	void*		Address to be decommited
	 * @param size		uintptr_t	Size of region to be decommited
	 *
	 * @return true if memory was successfully decommited, false otherwise
	 */
	virtual bool decommitMemory(MM_EnvironmentBase *env, void *address, uintptr_t size);
	/* tell the compiler we want both decommit from Base class and our class */
	using MM_VirtualMemory::decommitMemory;

	/**
	 * Get the total number of bytes reserved for sparse virtual memory
	 */
	MMINLINE uintptr_t getReservedSize()
	{
		return _reserveSize;
	}

	/**
	 * Get the sparseDataPool for sparse virtual memory
	 */
	MMINLINE MM_SparseAddressOrderedFixedSizeDataPool *getSparseDataPool()
	{
		return _sparseDataPool;
	}

	MMINLINE uintptr_t getAllocationContextIndexForAddress(const void *address)
	{
		const uintptr_t regionSize = _heap->getHeapRegionManager()->getRegionSize();
		return ((uintptr_t)address - (uintptr_t)getHeapBase()) / regionSize;
	}

	MMINLINE uintptr_t getAllocationContextCount(uintptr_t size)
	{
		const uintptr_t regionSize = _heap->getHeapRegionManager()->getRegionSize();
		return size / regionSize;
	}

	MMINLINE void* getAllocationContextForAddress(const void *address, uintptr_t index)
	{
		uintptr_t offset = getAllocationContextIndexForAddress(address);
		Assert_MM_true((offset + index) < _allocationContextArraySize);
		return _allocationContextArray[offset + index];
	}

	MMINLINE void setAllocationContextForAddress(const void *address, void *allocationContext, uintptr_t index)
	{
		uintptr_t offset = getAllocationContextIndexForAddress(address);
		Assert_MM_true((offset + index) < _allocationContextArraySize);
		_allocationContextArray[offset + index] = allocationContext;
	}

	MMINLINE void resetAllocationContextForAddress(const void *address, uintptr_t size)
	{
		uintptr_t offset = getAllocationContextIndexForAddress(address);
		uintptr_t count = getAllocationContextCount(size);
		Assert_MM_true((offset + count) <= _allocationContextArraySize);
		for (uintptr_t index = 0; index < count; index++) {
			_allocationContextArray[offset + index] = 0;
		}
	}
};

#endif /* SPARSEVIRTUALMEMORY_HPP_ */
