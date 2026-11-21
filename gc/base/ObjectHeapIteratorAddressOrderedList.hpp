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
 * @ingroup GC_Base
 */

#if !defined(OBJECTHEAPITERATORADDRESSORDEREDLIST_HPP_)
#define OBJECTHEAPITERATORADDRESSORDEREDLIST_HPP_

#include "omrcfg.h"

#include "GCExtensionsBase.hpp"
#include "HeapRegionDescriptor.hpp"
#include "ObjectHeapIterator.hpp"
#include "ObjectModel.hpp"

/**
 * Iterate over all objects and non-objects in a memory segment which uses
 * an address-ordered free list.
 *
 * @ingroup GC_Base
 */
class GC_ObjectHeapIteratorAddressOrderedList : public GC_ObjectHeapIterator
{
	/*
	 * Data members
	 */
private:
	bool _includeDeadObjects;
#if defined(UNIFY_ABORTED_SCAVENGE) || defined(OMR_GC_CONCURRENT_SCAVENGER)
// TODO: this is causing error under unify. Do we change every occurrence of this under macro (4 in this file)?
	bool _includeForwardedObjects;
#endif /* defined(UNIFY_ABORTED_SCAVENGE) || defined(OMR_GC_CONCURRENT_SCAVENGER) */
	GC_ObjectModel *_objectModel;

	omrobjectptr_t _scanPtr;
	omrobjectptr_t _scanPtrTop;
	bool _isDeadObject;
	uintptr_t _deadObjectSize;
	bool _pastFirstObject;

protected:
	
public:

	/*
	 * Function members
	 */
private:
	/**
	 * Compute the deadObjectSize for the current _scanPtr and store into _deadObjectSize
	 */
	void computeDeadObjectSize();

	/**
	 * Advances the _scanPtr field by the specified amount
	 *
	 * @parm increment - the number of bytes to advance the scanPtr by
	 */
	void advanceScanPtr(uintptr_t increment);

	/**
	 * Helper method that filters out special 'objects' like dead or forwarded ones
	 */
	bool shouldReturnCurrentObject();
	
protected:
	
public:
	/**
	 * @param memorySegment the memory segment to be walked, from <code>heapBase</code>
	 * to <code>heapAlloc</code>.
	 * @param includeLiveObjects if true, objects will be included in the walk
	 * @param includeDeadObjects if true, non-objects will be included in the walk
	 */
	GC_ObjectHeapIteratorAddressOrderedList(MM_GCExtensionsBase *extensions, MM_HeapRegionDescriptor *region, bool includeDeadObjects) :
		/* Iterator base values */
		_includeDeadObjects(includeDeadObjects),
#if defined(UNIFY_ABORTED_SCAVENGE) || defined(OMR_GC_CONCURRENT_SCAVENGER)
// TODO: this is causing error under unify. Do we change every occurrence of this under macro (4 in this file)?
		_includeForwardedObjects(false),
#endif /* defined(UNIFY_ABORTED_SCAVENGE) || defined(OMR_GC_CONCURRENT_SCAVENGER) */
		_objectModel(&extensions->objectModel),
		_isDeadObject(false),
		_deadObjectSize(0),
		_pastFirstObject(false)
	{
		_scanPtrTop = (omrobjectptr_t)region->getHighAddress();
		/* Iterator scan values */
		_scanPtr = (omrobjectptr_t)region->getLowAddress();
	}

	/**
	 * @param base object or free entry at which to begin the walk
	 * @param top walk is complete when this address is reached
	 * @param includeLiveObjects if true, objects will be included in the walk
	 * @param includeDeadObjects if true, non-objects will be included in the walk
	 */
	GC_ObjectHeapIteratorAddressOrderedList(MM_GCExtensionsBase *extensions, omrobjectptr_t base, omrobjectptr_t top, bool includeDeadObjects, bool skipFirstObject = false) :
		/* Iterator base values */
		_includeDeadObjects(includeDeadObjects),
#if defined(UNIFY_ABORTED_SCAVENGE) || defined(OMR_GC_CONCURRENT_SCAVENGER)
// TODO: this is causing error under unify. Do we change every occurrence of this under macro (4 in this file)?
		_includeForwardedObjects(false),
#endif /* defined(UNIFY_ABORTED_SCAVENGE) || defined(OMR_GC_CONCURRENT_SCAVENGER) */
		_objectModel(&extensions->objectModel),
		_scanPtr(base),
		_scanPtrTop(top),
		/* Iterator scan values */
		_isDeadObject(false),
		_deadObjectSize(0),
		_pastFirstObject(skipFirstObject)
	{}

	virtual omrobjectptr_t nextObjectNoAdvance();
	virtual void advance(uintptr_t size);
	virtual void reset(uintptr_t *base, uintptr_t *top);
#if defined(UNIFY_ABORTED_SCAVENGE) || defined(OMR_GC_CONCURRENT_SCAVENGER)
// TODO: this is causing error under unify. Do we change every occurrence of this under macro (4 in this file)?
	void includeForwardedObjects() { _includeForwardedObjects = true; }
#endif /* defined(UNIFY_ABORTED_SCAVENGE) || defined(OMR_GC_CONCURRENT_SCAVENGER) */

	/**
	 * @see GC_ObjectHeapIterator::nextObject()
	 * Unlike nextObjectNoAdvance(), this method does not consider forwarded objects,
	 * since there are no users of this API where such an object could exist.
	 */
	MMINLINE virtual omrobjectptr_t nextObject()
	{
		while (_scanPtr < _scanPtrTop) {
		
			_isDeadObject = _objectModel->isDeadObject(_scanPtr);
			omrobjectptr_t currentObject = _scanPtr;
			if (!_isDeadObject) {
				_scanPtr = (omrobjectptr_t)(((uintptr_t)_scanPtr) + _objectModel->getConsumedSizeInBytesWithHeader(_scanPtr));
				return currentObject;
			} else {
				computeDeadObjectSize();

				_scanPtr = (omrobjectptr_t)(((uintptr_t)_scanPtr) + _deadObjectSize);
				if (_includeDeadObjects) {
					return currentObject;
				}
			}
		}

		return NULL;
	}

	/**
	 * Returns true if the object is dead
	 */
	MMINLINE bool isDeadObject()
	{
		return _isDeadObject;
	}

	/**
	 * Returns the size of the dead object
	 */
	MMINLINE uintptr_t getDeadObjectSize() {
		return _deadObjectSize;
	}
};

#endif /* OBJECTHEAPITERATORADDRESSORDEREDLIST_HPP_ */
