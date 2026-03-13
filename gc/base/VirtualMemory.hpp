/*******************************************************************************
 * Copyright IBM Corp. and others 1991
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

#if !defined(VIRTUALMEMORY_HPP_)
#define VIRTUALMEMORY_HPP_

#include "omrcomp.h"
#include "omrport.h"
#include "modronbase.h"
#include "modronopt.h"

#include "BaseVirtual.hpp"
#include "EnvironmentBase.hpp"

class MM_GCExtensions;
class MM_GCExtensionsBase;
struct J9PortVmemParams;

/**
 * @todo Provide class documentation
 * @ingroup GC_Base_Core
 */
class MM_VirtualMemory : public MM_BaseVirtual {
/*
 * Data members
 */
private:
	uintptr_t _pageFlags; /**< Flags describing the pages used for the virtual memory object */
	uintptr_t _tailPadding; /**< The number of bytes of padding at the end of the virtual memory. This padding will be committed into, but not reported as available */
	void* _heapBase; /**< The lowest usable address in the reserved block, once alignment and padding are taken into account */
	void* _heapTop; /**< One byte past the highest usable address in the reserved block, once alignment and padding are taken into account */
	uintptr_t _mode; /**< requested memory mode (memory flags combination) */
	uintptr_t _consumerCount; /**< number of memory consumers attached to this virtual memory instance */
	J9PortVmemIdentifier _identifier;

protected:
	MM_GCExtensionsBase* _extensions;
	void* _baseAddress; /**< The address returned from port - needed for freeing memory at shutdown */
	uintptr_t _heapAlignment;
	uintptr_t _pageSize; /**< Page size for this virtual memory object (before reservation requested, after reservation real) */
	uintptr_t _reserveSize; /**< The total number of bytes reserved, starting from _baseAddress */

public:
/*
 * Function members
 */
private:
	bool freeMemory();

protected:
	bool initialize(MM_EnvironmentBase* env, uintptr_t size, void* preferredAddress, void* ceiling, uintptr_t options, uint32_t memoryCategory);
	virtual void tearDown(MM_EnvironmentBase* env);

	virtual void* reserveMemory(J9PortVmemParams* params);

	MM_VirtualMemory(MM_EnvironmentBase* env, uintptr_t heapAlignment, uintptr_t pageSize, uintptr_t pageFlags, uintptr_t tailPadding, uintptr_t mode)
		: MM_BaseVirtual()
		, _pageFlags(pageFlags)
		, _tailPadding(tailPadding)
		, _heapBase(0)
		, _heapTop(0)
		, _mode(mode)
		, _consumerCount(0)
		, _identifier()
		, _extensions(env->getExtensions())
		, _baseAddress(NULL)
		, _heapAlignment(heapAlignment)
		, _pageSize(pageSize)
		, _reserveSize(0)
	{
		_typeId = __FUNCTION__;
	}

	void roundDownTop(uintptr_t rounding);

	/*
	 * Set the NUMA affinity for the specified range within the receiver.
	 * 
	 * @param numaNode - the node to associate the memory with
	 * @param[in] address - the start of the range to modify, must be aligned to the physical page size
	 * @param byteAmount - the size of the range to modify. Might NOT to be aligned to page size. Will be aligned inside.
	 * 
	 * @return true on success, false on failure
	 */
	virtual bool setNumaAffinity(uintptr_t numaNode, void* address, uintptr_t byteAmount);

	/**
	 * Return the heap file descriptor.
	 */
	MMINLINE int getHeapFileDescriptor()
	{
		return _identifier.fd;
	}


	/**
	 * Return number of memory consumers attached to this virtual memory object
	 * @return consumers number
	 */
	MMINLINE uintptr_t getConsumerCount()
	{
		return _consumerCount;
	}

	/**
	 * Increment number of memory consumers attached to this virtual memory object by one
	 */
	MMINLINE void incrementConsumerCount()
	{
		_consumerCount += 1;
	}

	/**
	 * Decrement number of memory consumers attached to this virtual memory object by one
	 */
	MMINLINE void decrementConsumerCount()
	{
		_consumerCount -= 1;
	}

	/**
	 * Returns true if double map API is available in the system and false otherwise
	 */
	MMINLINE bool isDoubleMapAvailable()
	{
		return 0 != (OMRPORT_VMEM_MEMORY_MODE_DOUBLE_MAP_AVAILABLE & _identifier.mode);
	}

public:
	/**
	 * Create new instance of Virtual Memory
	 *
	 * @param env - current thread environment
	 * @param heapAlignment - virtual memory heap alignment
	 * @param size - required memory size
	 * @param pageSize - desired physical page size. If requested page size is not available an allocation can use pages of smaller size
	 * *param pageFlags - ZOS only: desired physical page property pageable/nonpageable
	 * @param tailPadding - requested tail padding size
	 * @param preferredAddress - request allocation from specified address, fail to allocate if not available
	 * @param ceiling - specify maximum top address for allocated memory
	 * @param mode - request memory options, for example use "OMRPORT_VMEM_MEMORY_MODE_READ | OMRPORT_VMEM_MEMORY_MODE_WRITE"
	 * @param options - extra options for memory allocations
	 * @param memoryCategory - provide memory category
	 *
	 * @return pointer to MM_VirtualMemory instance or NULL if attempt fail
	 *
	 */
	static MM_VirtualMemory* newInstance(
			MM_EnvironmentBase* env, uintptr_t heapAlignment, uintptr_t size, uintptr_t pageSize, uintptr_t pageFlags,
			uintptr_t tailPadding, void* preferredAddress, void* ceiling, uintptr_t mode, uintptr_t options, uint32_t memoryCategory);

	/**
	 * Kill virtual memory instance
	 *
	 * @param env - current thread environment
	 */
	virtual void kill(MM_EnvironmentBase* env);

	/**
	 * Return the heap base of the virtual memory object
	 *
	 * @return base address for allocated virtual memory
	 */
	MMINLINE void* getHeapBase()
	{
		return _heapBase;
	};

	/**
	 * Return the top of the heap of the virtual memory object
	 *
	 * @return top address for allocated virtual memory
	 */
	MMINLINE void* getHeapTop()
	{
		return _heapTop;
	};

	/**
	 * Return the size of the pages used in the virtual memory object
	 *
	 * @return actual physical page size used for allocation, can be smaller that requested
	 */
	MMINLINE uintptr_t getPageSize()
	{
		return _pageSize;
	}

	/**
	 * Return the flags describing the pages used in the virtual memory object
	 *
	 * @return actual physical page flags used for allocation
	 */
	MMINLINE uintptr_t getPageFlags()
	{
		return _pageFlags;
	}

	/**
	 * Commit virtual memory range
	 *
	 * @param address - start address of memory to be committed
	 * @param size - size of memory to be committed
	 *
	 * @return true if succeeded
	 */
	virtual bool commitMemory(void* address, uintptr_t size);

	/**
	 * Decommit virtual memory range
	 *
	 * @param address the start of the block to be decommitted
	 * @param size the size of the block to be decommitted
	 * @param lowValidAddress the end of the previous committed block below address, or NULL if address is the first committed block
	 * @param highValidAddress the start of the next committed block above address, or NULL if address is the last committed block
	 * @return true if successful, false otherwise.
	 */
	virtual bool decommitMemory(void* address, uintptr_t size, void* lowValidAddress, void* highValidAddress);

/*
 * friends
 */
	friend class MM_MemoryManager;
};

#endif /* VIRTUALMEMORY_HPP_ */
