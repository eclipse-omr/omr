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

#ifndef OMR_ARRAYSTOREFORWARDING_INCL
#define OMR_ARRAYSTOREFORWARDING_INCL

#include <stdint.h>
#include "cs2/hashtab.h"
#include "env/TRMemory.hpp"
#include "infra/List.hpp"

class TR_Dominators;

namespace TR {
class Block;
class Compilation;
class Node;
class TreeTop;
} // namespace TR

namespace OMR {

/**
 * \brief Book-keeping for array store-spread elimination (array store forwarding) in Value Propagation.
 *
 * Small constant-size arrays allocated by \c anewarray are tracked.  Every constant-offset
 * \c awrtbari into such an array is recorded as a slot, and every constant-offset \c aloadi of the array is rewritten
 * to the stored value.  If the array never escapes and every slot is both written once and read, the allocation
 * and the stores are removed in ValuePropagation::doDelayedTransformations.
 *
 * This class owns all of the state for that optimization.  It does not inspect IL to decide whether a use is an
 * escape, does not check whether the pass is a global propagation, and does not log; the Value Propagation handlers
 * make those decisions and call into this class.  The class is responsible for keeping its own tables consistent:
 *
 *   - Arrays are identified by the value number of their \c anewarray node.
 *   - An array that has been invalidated (it escaped, a slot was written twice, ...) is dropped atomically: all of
 *     its slots and every queued transformation are discarded and it is never tracked again in this pass.  A
 *     VN that has been registered once is never registered again, whether or not it is still live.
 *   - Slots are keyed by the constant byte offset of the access.
 *   - A load is only forwarded from a slot's store if the store's block dominates the load's block, since nothing
 *     else guarantees that the store executed on every path to the load.
 */
class ArrayStoreForwarding {
public:
    // The value written to a constant offset of an array
    struct Slot {
        TR_ALLOC(TR_Memory::ValuePropagation)

        Slot(TR::Node *value, TR::TreeTop *storeTT, TR::Block *storeBlock)
            : _value(value)
            , _storeTT(storeTT)
            , _storeBlock(storeBlock)
            , _wasRead(false)
        {}

        // Leaf node (or temp load) whose value is forwarded to loads of this slot
        TR::Node *_value;

        // Treetop of the awrtbari that wrote the slot
        TR::TreeTop *_storeTT;

        // Block that contains the store.  A load is only forwarded from the store if this block dominates the load's
        // block, since nothing else guarantees that the store executed on every path to the load
        TR::Block *_storeBlock;

        // A load of this slot has been queued for forwarding
        bool _wasRead;
    };

    // An astore of a value to a fresh temp, to be inserted after _anchorTT in doDelayedTransformations
    struct PendingAstore {
        TR_ALLOC(TR_Memory::ValuePropagation)

        PendingAstore(TR::TreeTop *anchorTT, TR::TreeTop *astoreTT)
            : _anchorTT(anchorTT)
            , _astoreTT(astoreTT)
        {}

        TR::TreeTop *_anchorTT;
        TR::TreeTop *_astoreTT;
    };

    // An aloadi to be rewritten in place into a copy of \c _value in doDelayedTransformations
    struct LoadRewrite {
        TR_ALLOC(TR_Memory::ValuePropagation)

        LoadRewrite(TR::Node *load, TR::Node *value)
            : _load(load)
            , _value(value)
        {}

        TR::Node *_load;
        TR::Node *_value;
    };

    // The values that must be re-written to a fresh array in a cold block that cannot see the warm-path array
    struct ColdArrayRebuild {
        TR_ALLOC(TR_Memory::ValuePropagation)

        ColdArrayRebuild(TR::TreeTop *anewarrayTT, TR::TreeTop *insertBeforeTT, TR::Node *callNode, int32_t argIndex,
            uint32_t origArrayVN, TR::Allocator allocator)
            : _anewarrayTT(anewarrayTT)
            , _insertBeforeTT(insertBeforeTT)
            , _callNode(callNode)
            , _argIndex(argIndex)
            , _origArrayVN(origArrayVN)
            , _slotValues(allocator)
        {}

        TR::TreeTop *_anewarrayTT; // uninserted anewarray TreeTop
        TR::TreeTop *_insertBeforeTT; // insert _anewarrayTT before this treetop
        TR::Node *_callNode; // the call whose arg is to be redirected
        int32_t _argIndex; // index of the arg to redirect
        uint32_t _origArrayVN; // VN of the warm-path original

        // The values stored to the original array, keyed by the offset into the array
        CS2::HashTable<int64_t, TR::Node *, TR::Allocator> _slotValues;
    };

    // Everything known about one tracked array.
    class TrackedArray {
    public:
        TR_ALLOC(TR_Memory::ValuePropagation)

        TrackedArray(int32_t arrayVN, TR::Node *arrayNode, TR::TreeTop *anewarrayTT, TR::Compilation *comp);

        int32_t getArrayVN() const { return _arrayVN; }

        TR::Node *getArrayNode() const { return _arrayNode; }

        TR::TreeTop *getAnewarrayTreeTop() const { return _anewarrayTT; }

        bool isLive() const { return _live; }

        // awrtbari treetops (and their compressedRefs anchors) that become dead once every load is forwarded
        TR_ScratchList<TR::TreeTop> &getForwardedStoreTrees() { return _forwardedStoreTrees; }

        // astores of the anewarray to a local, dead once the allocation is eliminated
        TR_ScratchList<TR::TreeTop> &getLocalStoreTrees() { return _localStoreTrees; }

        TR_ScratchList<PendingAstore> &getPendingAstores() { return _pendingAstores; }

        TR_ScratchList<LoadRewrite> &getLoadRewrites() { return _loadRewrites; }

        /**
         * \brief Checks that removing the allocation is sound
         *
         * Removal requires that no store or load of the array survives, which requires that every slot was written
         * exactly once and read (forwarded) at least once:
         *   - a slot that is written but never read leaves its awrtbari in the IL pointing at a deleted anewarray
         *   - a slot that is read but never written would be forwarded from nothing
         *   - a slot that is never touched means the array is not fully accounted for
         *
         * \param[out] numSlots  number of elements of the array
         * \param[out] written   number of distinct slots written
         * \param[out] read      number of slots with at least one forwarded load
         * \return \c true if \c written and \c read both equal \c numSlots
         */
        bool isComplete(int32_t &numSlots, int32_t &written, int32_t &read);

    private:
        friend class ArrayStoreForwarding;

        typedef CS2::HashTable<int32_t, Slot *, TR::Allocator> SlotTable;

        /// Discards all slots and queued work.  Called by ArrayStoreForwarding::invalidateIfLive
        void invalidate();

        int32_t _arrayVN;
        TR::Node *_arrayNode;
        TR::TreeTop *_anewarrayTT;
        bool _live;
        SlotTable _slots;
        TR_ScratchList<TR::TreeTop> _forwardedStoreTrees;
        TR_ScratchList<TR::TreeTop> _localStoreTrees;
        TR_ScratchList<PendingAstore> _pendingAstores;
        TR_ScratchList<LoadRewrite> _loadRewrites;
    };

    /// Iterates the arrays that are still live, in no particular order
    /// Invalidating arrays during iteration is safe: it never adds or removes entries from the set of arrays
    /// Registering or reset() during iteration is not
    class LiveArrayIterator {
    public:
        LiveArrayIterator(ArrayStoreForwarding &forwarding);

        TrackedArray *getFirst();
        TrackedArray *getNext();

    private:
        typedef CS2::HashTable<int32_t, TrackedArray *, TR::Allocator> ArrayTable;

        TrackedArray *skipDeadArrays();

        const ArrayTable &_arrays;
        ArrayTable::Cursor _cursor;
    };

    ArrayStoreForwarding(TR::Compilation *comp);

    // true if no array has been registered since construction or the last reset()
    // Invalidated arrays still count as registered.  This is the cheap test used to skip all other work.
    bool isEmpty() const { return _arrays.IsEmpty(); }

    /// Forgets every array and every queued transformation
    void reset();

    /**
     * \brief Starts tracking an array.
     *
     * \param arrayVN       value number of \p arrayNode
     * \param arrayNode     the \c anewarray node
     * \param anewarrayTT   the treetop that anchors \p arrayNode; removed if the allocation is eliminated
     * \return \c true if the array is now tracked; \c false if \p arrayVN was already registered (live or not),
     *         in which case nothing changes.
     */
    bool registerArray(int32_t arrayVN, TR::Node *arrayNode, TR::TreeTop *anewarrayTT);

    // returns the  anewarray node if arrayVN is a tracked array that has not been invalidated, else NULL.
    TR::Node *getLiveArray(int32_t arrayVN);

    bool isLive(int32_t arrayVN) { return getLiveArray(arrayVN) != NULL; }

    /**
     * \brief Gets the value stored to a slot of a live array.
     * \return the value to forward to loads of the slot, or NULL if \p arrayVN is not a live array or the slot has
     *         no recorded store.
     */
    TR::Node *getForwardedValue(int32_t arrayVN, int64_t offset);

    /**
     * \brief Determines whether the store to a slot is guaranteed to have executed before \p loadBlock.
     *
     * That is the case if the store is in \p loadBlock or if the store's block dominates \p loadBlock.  Exception
     * edges count as edges, so a catch block that can be entered from before the store is not dominated by it.
     * The dominator tree is built the first time it is needed, so the CFG must not change until reset().
     *
     * \return \c true if the slot's store is known to have executed on every path to \p loadBlock; \c false if not,
     *         or if \p arrayVN is not a live array or the slot has no recorded store.
     */
    bool slotStoreDominates(int32_t arrayVN, int64_t offset, TR::Block *loadBlock);

    /**
     * \brief Stops tracking an array and cancels all work queued for it.
     *
     * Drops the array's slots, queued store/astore/local-astore removals, pending astore insertions and pending
     * load rewrites.  The astores that were created but will now never be inserted release their reference to the
     * value they store.  Cold-block rebuilds are kept but are discarded in doDelayedTransformations because
     * isLive() is false for the array.
     *
     * \return the \c anewarray node that was invalidated, or NULL if \p arrayVN is not a live array (no change).
     */
    TR::Node *invalidateIfLive(int32_t arrayVN);

    /**
     * \brief Records the awrtbari that writes a constant-offset slot.
     *
     * \param value        value forwarded to loads of the slot (a constant, an address, or a load from a temp)
     * \param storeTT      treetop of the awrtbari
     * \param storeBlock   block that contains the store
     * \return \c true if the store was recorded.  \c false if the slot was already written; the array has then been
     *         invalidated, since neither store can be removed.
     */
    bool recordSlotStore(int32_t arrayVN, int64_t offset, TR::Node *value, TR::TreeTop *storeTT, TR::Block *storeBlock);

    // Records a store of the anewarray to a local; it is removed with the allocation.
    void recordLocalAstore(int32_t arrayVN, TR::TreeTop *storeTT);

    // Queues astoreTT for insertion after anchorTT.
    void recordPendingAstore(int32_t arrayVN, TR::TreeTop *anchorTT, TR::TreeTop *astoreTT);

    /**
     * \brief Records that \p load, a load of a slot with a recorded store, is to be rewritten to the slot's value.
     *
     * Marks the slot as read, queues the load for rewriting, and queues the slot's awrtbari for removal the first
     * time the slot is read.
     *
     * \return the awrtbari treetop that was queued for removal by this call, or NULL if it had been queued by an
     *         earlier load of the same slot.  The caller queues any treetop that must be removed with it (such as
     *         a compressedRefs anchor) with queueStoreRemoval().
     */
    TR::TreeTop *recordForwardedLoad(int32_t arrayVN, int64_t offset, TR::Node *load);

    // Queues tt for removal along with the array's stores.
    void queueStoreRemoval(int32_t arrayVN, TR::TreeTop *tt);

    /**
     * \brief Records that a call in a cold block uses the array.
     *
     * The array itself is unaffected.  Instead a fresh array to be inserted before \p insertBeforeTT and passed in
     * argument \p argIndex of \p callNode is recorded, together with a snapshot of the values stored to the array
     * so far (only stores that precede the call can be replayed).
     */
    void recordColdRebuild(int32_t arrayVN, TR::TreeTop *clonedArrayTT, TR::TreeTop *insertBeforeTT, TR::Node *callNode,
        int32_t argIndex);

    // Cold-block rebuilds, in doDelayedTransformations.  Includes rebuilds of arrays that were later invalidated.
    TR_ScratchList<ColdArrayRebuild> &getColdRebuilds() { return _coldRebuilds; }

private:
    typedef CS2::HashTable<int32_t, TrackedArray *, TR::Allocator> ArrayTable;

    // Slots are keyed by the 32-bit byte offset of the access.
    static int32_t slotKey(int64_t offset) { return (int32_t)(uint32_t)offset; }

    TrackedArray *findLive(int32_t arrayVN);

    TR::Compilation *_comp;
    ArrayTable _arrays;
    TR_ScratchList<ColdArrayRebuild> _coldRebuilds;

    /// Built on demand by slotStoreDominates(); NULL until then and after reset().
    TR_Dominators *_dominators;
};

} // namespace OMR

#endif
