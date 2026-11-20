/*******************************************************************************
 * Copyright IBM Corp. and others 2026
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

#include "optimizer/ArrayStoreForwarding.hpp"

#include "compile/Compilation.hpp"
#include "env/TRMemory.hpp"
#include "il/Block.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "il/TreeTop.hpp"
#include "il/TreeTop_inlines.hpp"
#include "infra/Assert.hpp"
#include "optimizer/Dominators.hpp"

OMR::ArrayStoreForwarding::TrackedArray::TrackedArray(int32_t arrayVN, TR::Node *arrayNode, TR::TreeTop *anewarrayTT,
    TR::Compilation *comp)
    : _arrayVN(arrayVN)
    , _arrayNode(arrayNode)
    , _anewarrayTT(anewarrayTT)
    , _live(true)
    , _slots(comp->allocator())
    , _forwardedStoreTrees(comp->trMemory())
    , _localStoreTrees(comp->trMemory())
    , _pendingAstores(comp->trMemory())
    , _loadRewrites(comp->trMemory())
{}

bool OMR::ArrayStoreForwarding::TrackedArray::isComplete(int32_t &numSlots, int32_t &written, int32_t &read)
{
    numSlots = _arrayNode->getFirstChild()->getInt();
    written = 0;
    read = 0;

    SlotTable::Cursor cursor(_slots);
    for (cursor.SetToFirst(); cursor.Valid(); cursor.SetToNext()) {
        ++written;
        if (_slots.DataAt(cursor)->_wasRead)
            ++read;
    }

    return written == numSlots && read == numSlots;
}

void OMR::ArrayStoreForwarding::TrackedArray::invalidate()
{
    _live = false;
    _slots.MakeEmpty();
    _forwardedStoreTrees.init();
    _localStoreTrees.init();

    // The pending astore will never be inserted, but creating it incremented the reference count of the value it
    // stores.  Release that reference, otherwise the value is left with a reference count that exceeds its parents.
    ListIterator<PendingAstore> astoreIt(&_pendingAstores);
    for (PendingAstore *p = astoreIt.getFirst(); p; p = astoreIt.getNext())
        p->_astoreTT->getNode()->removeAllChildren();
    _pendingAstores.init();

    _loadRewrites.init();
}

OMR::ArrayStoreForwarding::LiveArrayIterator::LiveArrayIterator(ArrayStoreForwarding &forwarding)
    : _arrays(forwarding._arrays)
    , _cursor(forwarding._arrays)
{}

OMR::ArrayStoreForwarding::TrackedArray *OMR::ArrayStoreForwarding::LiveArrayIterator::getFirst()
{
    _cursor.SetToFirst();
    return skipDeadArrays();
}

OMR::ArrayStoreForwarding::TrackedArray *OMR::ArrayStoreForwarding::LiveArrayIterator::getNext()
{
    _cursor.SetToNext();
    return skipDeadArrays();
}

OMR::ArrayStoreForwarding::TrackedArray *OMR::ArrayStoreForwarding::LiveArrayIterator::skipDeadArrays()
{
    for (; _cursor.Valid(); _cursor.SetToNext()) {
        TrackedArray *array = _arrays.DataAt(_cursor);
        if (array->isLive())
            return array;
    }
    return NULL;
}

OMR::ArrayStoreForwarding::ArrayStoreForwarding(TR::Compilation *comp)
    : _comp(comp)
    , _arrays(comp->allocator())
    , _coldRebuilds(comp->trMemory())
    , _dominators(NULL)
{}

void OMR::ArrayStoreForwarding::reset()
{
    _arrays.MakeEmpty();
    _coldRebuilds.init();
    _dominators = NULL;
}

OMR::ArrayStoreForwarding::TrackedArray *OMR::ArrayStoreForwarding::findLive(int32_t arrayVN)
{
    CS2::HashIndex idx;
    if (!_arrays.Locate(arrayVN, idx))
        return NULL;
    TrackedArray *array = _arrays[idx];
    return array->isLive() ? array : NULL;
}

bool OMR::ArrayStoreForwarding::registerArray(int32_t arrayVN, TR::Node *arrayNode, TR::TreeTop *anewarrayTT)
{
    CS2::HashIndex idx;
    if (_arrays.Locate(arrayVN, idx))
        return false;

    _arrays.Add(arrayVN, new (_comp->trHeapMemory()) TrackedArray(arrayVN, arrayNode, anewarrayTT, _comp));
    return true;
}

TR::Node *OMR::ArrayStoreForwarding::getLiveArray(int32_t arrayVN)
{
    TrackedArray *array = findLive(arrayVN);
    return array != NULL ? array->getArrayNode() : NULL;
}

TR::Node *OMR::ArrayStoreForwarding::getForwardedValue(int32_t arrayVN, int64_t offset)
{
    TrackedArray *array = findLive(arrayVN);
    if (array == NULL)
        return NULL;

    CS2::HashIndex slotIdx;
    if (!array->_slots.Locate(slotKey(offset), slotIdx))
        return NULL;
    return array->_slots[slotIdx]->_value;
}

bool OMR::ArrayStoreForwarding::slotStoreDominates(int32_t arrayVN, int64_t offset, TR::Block *loadBlock)
{
    TrackedArray *array = findLive(arrayVN);
    if (array == NULL)
        return false;

    CS2::HashIndex slotIdx;
    if (!array->_slots.Locate(slotKey(offset), slotIdx))
        return false;

    TR::Block *storeBlock = array->_slots[slotIdx]->_storeBlock;
    if (storeBlock == loadBlock)
        return true;

    if (_dominators == NULL)
        _dominators = new (_comp->trHeapMemory()) TR_Dominators(_comp);
    return _dominators->dominates(storeBlock, loadBlock) != 0;
}

TR::Node *OMR::ArrayStoreForwarding::invalidateIfLive(int32_t arrayVN)
{
    TrackedArray *array = findLive(arrayVN);
    if (array == NULL)
        return NULL;

    array->invalidate();
    return array->getArrayNode();
}

bool OMR::ArrayStoreForwarding::recordSlotStore(int32_t arrayVN, int64_t offset, TR::Node *value, TR::TreeTop *storeTT,
    TR::Block *storeBlock)
{
    TrackedArray *array = findLive(arrayVN);
    TR_ASSERT_FATAL(array != NULL, "recordSlotStore on array VN=%d that is not live", arrayVN);

    CS2::HashIndex slotIdx;
    if (array->_slots.Locate(slotKey(offset), slotIdx)) {
        // the array slot is written to more than once. we cannot tell which value is valid
        // abort the opt for this array
        invalidateIfLive(arrayVN);
        return false;
    }

    array->_slots.Add(slotKey(offset), new (_comp->trHeapMemory()) Slot(value, storeTT, storeBlock));
    return true;
}

void OMR::ArrayStoreForwarding::recordLocalAstore(int32_t arrayVN, TR::TreeTop *storeTT)
{
    TrackedArray *array = findLive(arrayVN);
    TR_ASSERT_FATAL(array != NULL, "recordLocalAstore on array VN=%d that is not live", arrayVN);
    array->_localStoreTrees.add(storeTT);
}

void OMR::ArrayStoreForwarding::recordPendingAstore(int32_t arrayVN, TR::TreeTop *anchorTT, TR::TreeTop *astoreTT)
{
    TrackedArray *array = findLive(arrayVN);
    TR_ASSERT_FATAL(array != NULL, "recordPendingAstore on array VN=%d that is not live", arrayVN);
    array->_pendingAstores.add(new (_comp->trHeapMemory()) PendingAstore(anchorTT, astoreTT));
}

TR::TreeTop *OMR::ArrayStoreForwarding::recordForwardedLoad(int32_t arrayVN, int64_t offset, TR::Node *load)
{
    TrackedArray *array = findLive(arrayVN);
    TR_ASSERT_FATAL(array != NULL, "recordForwardedLoad on array VN=%d that is not live", arrayVN);

    CS2::HashIndex slotIdx;
    bool found = array->_slots.Locate(slotKey(offset), slotIdx);
    TR_ASSERT_FATAL(found, "recordForwardedLoad of array VN=%d offset %lld with no recorded store", arrayVN,
        (long long)offset);
    Slot *slot = array->_slots[slotIdx];

    slot->_wasRead = true;
    array->_loadRewrites.add(new (_comp->trHeapMemory()) LoadRewrite(load, slot->_value));

    TR::TreeTop *storeTT = slot->_storeTT;
    if (storeTT != NULL) {
        // Cleared so that multiple loads of the same slot do not queue the same awrtbari for removal more than once.
        slot->_storeTT = NULL;
        array->_forwardedStoreTrees.add(storeTT);
    }
    return storeTT;
}

void OMR::ArrayStoreForwarding::queueStoreRemoval(int32_t arrayVN, TR::TreeTop *tt)
{
    TrackedArray *array = findLive(arrayVN);
    TR_ASSERT_FATAL(array != NULL, "queueStoreRemoval on array VN=%d that is not live", arrayVN);
    array->_forwardedStoreTrees.add(tt);
}

void OMR::ArrayStoreForwarding::recordColdRebuild(int32_t arrayVN, TR::TreeTop *clonedArrayTT,
    TR::TreeTop *insertBeforeTT, TR::Node *callNode, int32_t argIndex)
{
    TrackedArray *array = findLive(arrayVN);
    TR_ASSERT_FATAL(array != NULL, "recordColdRebuild on array VN=%d that is not live", arrayVN);

    ColdArrayRebuild *rebuild = new (_comp->trHeapMemory())
        ColdArrayRebuild(clonedArrayTT, insertBeforeTT, callNode, argIndex, (uint32_t)arrayVN, _comp->allocator());

    TrackedArray::SlotTable::Cursor cursor(array->_slots);
    for (cursor.SetToFirst(); cursor.Valid(); cursor.SetToNext())
        rebuild->_slotValues.Add((int64_t)array->_slots.KeyAt(cursor), array->_slots.DataAt(cursor)->_value);

    _coldRebuilds.add(rebuild);
}
