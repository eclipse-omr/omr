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

#ifndef OMR_REGISTERCANDIDATE_INCL
#define OMR_REGISTERCANDIDATE_INCL

#include <stddef.h>
#include <stdint.h>
#include "codegen/RegisterConstants.hpp"
#include "env/TRMemory.hpp"
#include "il/Block.hpp"
#include "il/DataTypes.hpp"
#include "il/Node.hpp"
#include "il/SymbolReference.hpp"
#include "infra/Array.hpp"
#include "infra/Assert.hpp"
#include "infra/BitVector.hpp"
#include "infra/Cfg.hpp"
#include "infra/Flags.hpp"
#include "infra/Link.hpp"
#include "infra/List.hpp"
#include <map>

class TR_GlobalRegisterAllocator;
class TR_Structure;

namespace TR {
class Compilation;
class Symbol;
class TreeTop;
class NodeChecklist;
class RegisterCandidate;
class RegisterCandidates;
class GlobalSet;
} // namespace TR

enum TR_RegisterCandidateTypes {
    TR_InductionVariableCandidate = 0,
    TR_PRECandidate = 1,
    TR_LastRegisterCandidateType = TR_PRECandidate,
    TR_NumRegisterCandidateTypes = TR_LastRegisterCandidateType + 1
};

namespace OMR {

class GlobalSet {
public:
    GlobalSet(TR::Compilation *comp, TR::Region &region);

    TR_BitVector *operator[](uint32_t symRefNum)
    {
        if (!_collected)
            collectBlocks();

        auto lookup = _blocksPerAuto.find(symRefNum);
        if (lookup != _blocksPerAuto.end())
            return lookup->second;
        return &_EMPTY;
    }

    bool isEmpty() { return _blocksPerAuto.empty(); }

    void makeEmpty()
    {
        _collected = false;
        _blocksPerAuto.clear();
    }

protected:
    void collectBlocks();
    virtual void collectReferencedAutoSymRefs(TR::Node *node, TR_BitVector &referencedAutoSymRefs,
        TR::NodeChecklist &visited);

    TR::Region &_region;
    TR_BitVector _empty;
    typedef TR::typed_allocator<std::pair<uint32_t const, TR_BitVector *>, TR::Region &> RefMapAllocator;
    typedef std::less<uint32_t> RefMapComparator;
    typedef std::map<uint32_t, TR_BitVector *, RefMapComparator, RefMapAllocator> RefMap;
    RefMap _blocksPerAuto;
    TR_BitVector _EMPTY;
    bool _collected;
    TR::Compilation *_comp;
};

class RegisterCandidate : public TR_Link<TR::RegisterCandidate> {
public:
    RegisterCandidate(TR::SymbolReference *, TR::Region &r);

    class BlockInfo {
        typedef TR::typed_allocator<std::pair<uint32_t const, uint32_t>, TR::Region &> InfoMapAllocator;
        typedef std::less<uint32_t> InfoMapComparator;
        typedef std::map<uint32_t, uint32_t, InfoMapComparator, InfoMapAllocator> InfoMap;
        InfoMap _blockMap;
        TR_BitVector _candidateBlocks;

    public:
        typedef TR_BitVectorIterator iterator;

        BlockInfo(TR::Region &region)
            : _blockMap((InfoMapComparator()), (InfoMapAllocator(region)))
            , _candidateBlocks(region)
        {}

        void setNumberOfLoadsAndStores(uint32_t block, uint32_t count)
        {
            _candidateBlocks.set(block);

            auto lookup = _blockMap.find(block);
            if (lookup != _blockMap.end())
                lookup->second = count;
            else if (count > 0)
                _blockMap[block] = count;
        }

        void incNumberOfLoadsAndStores(uint32_t block, uint32_t count)
        {
            _candidateBlocks.set(block);
            if (count > 0)
                _blockMap[block] += count;
        }

        void removeBlock(uint32_t block)
        {
            _candidateBlocks.reset(block);
            _blockMap.erase(block);
        }

        bool find(uint32_t block) { return _candidateBlocks.get(block) != 0; }

        uint32_t getNumberOfLoadsAndStores(uint32_t block)
        {
            if (_candidateBlocks.get(block)) {
                auto result = _blockMap.find(block);
                return result != _blockMap.end() ? result->second : 0;
            }
            return 0;
        }

        TR_BitVector &getCandidateBlocks() { return _candidateBlocks; }

        iterator getIterator() { return iterator(_candidateBlocks); }
    };

    struct LoopInfo : TR_Link<LoopInfo> {
        LoopInfo(TR_Structure *l, int32_t n)
            : _loop(l)
            , _numberOfLoadsAndStores(n)
        {}

        TR_Structure *_loop;
        int32_t _numberOfLoadsAndStores;
    };

    void addAllBlocks() { setAllBlocks(true); }

    void addBlock(TR::Block *b, int32_t numberOfLoadsAndStores);

    void addLoopExitBlock(TR::Block *b);
    int32_t removeBlock(TR::Block *b);
    void removeLoopExitBlock(TR::Block *b);
    int32_t countNumberOfLoadsAndStoresInBlocks(List<TR::Block> *);

#ifdef TRIM_ASSIGNED_CANDIDATES
    void addLoop(TR_Structure *l, int32_t numberOfLoadsAndStores, TR_Memory *);
    LoopInfo *find(TR_Structure *l);
    void countNumberOfLoadsAndStoresInLoops(TR_Memory *m);
    void printLoopInfo(TR::Compilation *comp);
    int32_t getNumberOfLoadsAndStoresInLoop(TR_Structure *);
#endif

    void addAllBlocksInStructure(TR_Structure *, TR::Compilation *, const char *description,
        vcount_t count = MAX_VCOUNT, bool recursiveCall = false);

    bool find(TR::Block *b);

    bool hasBlock(TR::Block *b) { return find(b) != 0; }

    bool hasLoopExitBlock(TR::Block *b);

    TR_GlobalRegisterNumber getGlobalRegisterNumber() { return _lowRegNumber; }

    TR_GlobalRegisterNumber getHighGlobalRegisterNumber() { return _highRegNumber; }

    TR_GlobalRegisterNumber getLowGlobalRegisterNumber() { return _lowRegNumber; }

    bool hasSameGlobalRegisterNumberAs(TR::Node *node, TR::Compilation *comp);

    TR::SymbolReference *getSymbolReference() { return _symRef; }

    TR::SymbolReference *getSplitSymbolReference() { return _splitSymRef; }

    void setSplitSymbolReference(TR::SymbolReference *symRef) { _splitSymRef = symRef; }

    TR::SymbolReference *getRestoreSymbolReference() { return _restoreSymRef; }

    void setRestoreSymbolReference(TR::SymbolReference *symRef) { _restoreSymRef = symRef; }

    TR_BitVector &getBlocksLiveOnEntry() { return _liveOnEntry; }

    TR_BitVector &getBlocksLiveOnExit() { return _liveOnExit; }

    TR::Symbol *getSymbol() { return _symRef->getSymbol(); }

    TR::DataType getDataType();
    TR::DataType getType();
    bool rcNeeds2Regs(TR::Compilation *);
    TR_RegisterKinds getRegisterKinds();

    uint32_t getWeight() { return _weight; }

    virtual bool symbolIsLive(TR::Block *);

    bool canBeReprioritized() { return (_reprioritized > 0); }

    void setGlobalRegisterNumber(TR_GlobalRegisterNumber n) { _lowRegNumber = n; }

    void setLowGlobalRegisterNumber(TR_GlobalRegisterNumber n) { _lowRegNumber = n; }

    void setHighGlobalRegisterNumber(TR_GlobalRegisterNumber n) { _highRegNumber = n; }

    void setWeight(TR::Block **, int32_t *, TR::Compilation *, TR_Array<int32_t> &, TR_Array<int32_t> &,
        TR_Array<int32_t> &, TR_BitVector *, TR_Array<TR::Block *> &startOfExtendedBB, TR_BitVector &, TR_BitVector &);

    void setReprioritized() { _reprioritized--; }

    void setMaxReprioritized(uint8_t n) { _reprioritized = n; }

    uint8_t getReprioritized() { return _reprioritized; }

    virtual void processLiveOnEntryBlocks(TR::Block **, int32_t *, TR::Compilation *, TR_Array<int32_t> &,
        TR_Array<int32_t> &, TR_Array<int32_t> &, TR_BitVector *, TR_Array<TR::Block *> &startOfExtendedBB,
        bool removeUnusedLoops = false);
    void removeAssignedCandidateFromLoop(TR::Compilation *, TR_Structure *, int32_t, TR_BitVector *, TR_BitVector *,
        bool);

    void extendLiveRangesForLiveOnExit(TR::Compilation *, TR::Block **, TR_Array<TR::Block *> &startOfExtendedBBForBB);

    void recalculateWeight(TR::Block **, int32_t *, TR::Compilation *, TR_Array<int32_t> &, TR_Array<int32_t> &,
        TR_Array<int32_t> &, TR_BitVector *, TR_Array<TR::Block *> &startOfExtendedBB);
    bool prevBlockTooRegisterConstrained(TR::Compilation *comp, TR::Block *, TR_Array<int32_t> &, TR_Array<int32_t> &,
        TR_Array<int32_t> &);

    List<TR::TreeTop> &getStores() { return _stores; }

    List<TR_Structure> &getLoopsWithHoles() { return _loopsWithHoles; }

    void addLoopWithHole(TR_Structure *s)
    {
        if (!_loopsWithHoles.find(s))
            _loopsWithHoles.add(s);
    }

    void addStore(TR::TreeTop *tt) { _stores.add(tt); }

    TR::Node *getMostRecentValue() { return _mostRecentValue; }

    void setMostRecentValue(TR::Node *node) { _mostRecentValue = node; }

    TR::Node *getLastLoad() { return _lastLoad; }

    void setLastLoad(TR::Node *node) { _lastLoad = node; }

    bool canAllocateDespiteAliases(TR::Compilation *);

    bool extendedLiveRange() { return _flags.testAny(extendLiveRange); }

    bool is8BitGlobalGPR() { return _flags.testAny(eightBitGlobalGPR); }

    bool initialBlocksWeightComputed() { return _flags.testAny(initialBlocksWeightComputedFlag); }

    bool getValueModified() { return _flags.testAny(valueModified); }

    bool isHighWordZero() { return _flags.testAny(highWordZero); }

    bool isLiveAcrossExceptionEdge() { return _flags.testAny(liveAcrossExceptionEdge); }

    void setExtendedLiveRange(bool b) { _flags.set(extendLiveRange, b); }

    void setValueModified(bool b) { _flags.set(valueModified, b); }

    void setHighWordZero(bool b) { _flags.set(highWordZero, b); }

    void setLiveAcrossExceptionEdge(bool b) { _flags.set(liveAcrossExceptionEdge, b); }

    void setAllBlocks(bool b) { _flags.set(allBlocks, b); }

    void setIs8BitGlobalGPR(bool b) { _flags.set(eightBitGlobalGPR, b); }

    void setInitialBlocksWeightComputed(bool b) { _flags.set(initialBlocksWeightComputedFlag, b); }

    bool isAllBlocks() { return _flags.testAny(allBlocks); }

    BlockInfo &getBlocks() { return _blocks; }

protected:
    TR::SymbolReference *_symRef;
    TR::SymbolReference *_splitSymRef;
    TR::SymbolReference *_restoreSymRef;
    uint32_t _weight;
    TR_GlobalRegisterNumber _lowRegNumber;
    TR_GlobalRegisterNumber _highRegNumber;
    BlockInfo _blocks;
    List<TR::Block> _loopExitBlocks;
    TR_BitVector _liveOnEntry;
    TR_BitVector _liveOnExit;
    TR_BitVector _originalLiveOnEntry;
    List<TR::TreeTop> _stores;
    List<TR_Structure> _loopsWithHoles;
    TR::Node *_mostRecentValue;
    TR::Node *_lastLoad;
    flags16_t _flags;

    enum {
        allBlocks = 0x0001,
        eightBitGlobalGPR = 0x0002,
        valueModified = 0x0004,
        liveAcrossExceptionEdge = 0x0008,
        highWordZero = 0x0010,
        // AVAILABLE                = 0x0020,
        extendLiveRange = 0x0040,
        initialBlocksWeightComputedFlag = 0x0080,
    };

    uint8_t _reprioritized;

#ifdef TRIM_ASSIGNED_CANDIDATES
    TR_LinkHead<LoopInfo> _loops; // loops candidate is used in
#endif
};

class RegisterCandidates {
public:
    TR_ALLOC(TR_Memory::RegisterCandidates)

    RegisterCandidates(TR::Compilation *);

    TR::Compilation *comp() { return _compilation; }

    TR_Memory *trMemory() { return _trMemory; }

    TR_StackMemory trStackMemory() { return _trMemory; }

    TR_HeapMemory trHeapMemory() { return _trMemory; }

    TR_PersistentMemory *trPersistentMemory() { return _trMemory->trPersistentMemory(); }

    virtual TR::RegisterCandidate *findOrCreate(TR::SymbolReference *symRef);
    virtual TR::RegisterCandidate *find(TR::SymbolReference *symRef);
    TR::RegisterCandidate *find(TR::Symbol *sym);

    TR::GlobalSet &getReferencedAutoSymRefs(TR::Region &region);
    TR_BitVector *getBlocksReferencingSymRef(uint32_t symRefNum);

    virtual bool assign(TR::Block **, int32_t, int32_t &, int32_t &);
    virtual void computeAvailableRegisters(TR::RegisterCandidate *, int32_t, int32_t, TR::Block **, TR_BitVector *);

    static int32_t getWeightForType(TR_RegisterCandidateTypes type) { return _candidateTypeWeights[type]; }

    TR::RegisterCandidate *getFirst() { return _candidates.getFirst(); }

    TR::RegisterCandidate *newCandidate(TR::SymbolReference *ref);

    void releaseCandidates() { TR::Region::reset(_candidateRegion, _trMemory->heapMemoryRegion()); }

    void collectCfgProperties(TR::Block **, int32_t);

    typedef TR::typed_allocator<std::pair<uint32_t const, TR::RegisterCandidate *>, TR::Region &>
        SymRefCandidateMapAllocator;
    typedef std::less<uint32_t> SymRefCandidateMapComparator;
    typedef std::map<uint32_t, TR::RegisterCandidate *, SymRefCandidateMapComparator, SymRefCandidateMapAllocator>
        SymRefCandidateMap;

    void initCandidateForSymRefs();
    void initStartOfExtendedBBForBB();

    SymRefCandidateMap *getCandidateForSymRefs() { return _candidateForSymRefs; }

    void setCandidateForSymRefs(int32_t key, TR::RegisterCandidate *rc) { (*_candidateForSymRefs)[key] = rc; }

    TR_Array<TR::Block *> &getStartOfExtendedBBForBB() { return _startOfExtendedBBForBB; }

    void setStartOfExtendedBBForBB(int32_t index, TR::Block *block) { _startOfExtendedBBForBB[index] = block; }

    bool aliasesPreventAllocation(TR::Compilation *comp, TR::SymbolReference *symRef);

protected:
    bool candidatesOverlap(TR::Block *, TR::RegisterCandidate *, TR::RegisterCandidate *, bool);
    void lookForCandidates(TR::Node *, TR::Symbol *, TR::Symbol *, bool &, bool &);
    bool prioritizeCandidate(TR::RegisterCandidate *, TR::RegisterCandidate *&);
    TR::RegisterCandidate *reprioritizeCandidates(TR::RegisterCandidate *, TR::Block **, int32_t *, int32_t,
        TR::Block *, TR::Compilation *, bool reprioritizeFP, bool onlyReprioritizeLongs, TR_BitVector *referencedBlocks,
        TR_Array<int32_t> &blockGPRCount, TR_Array<int32_t> &blockFPRCount, TR_Array<int32_t> &blockVRFCount,
        TR_BitVector *, bool);

    TR::Compilation *_compilation;
    TR_Memory *_trMemory;
    TR::Region _candidateRegion;
    TR_LinkHead<TR::RegisterCandidate> _candidates;

    static int32_t _candidateTypeWeights[TR_NumRegisterCandidateTypes];
    TR::GlobalSet *_referencedAutoSymRefsInBlock;

    SymRefCandidateMap *_candidateForSymRefs;
    TR_Array<TR::Block *> _startOfExtendedBBForBB;

    // scratch arrays for calculating register conflicts
    TR_Array<TR_BitVector> _liveOnEntryConflicts;
    TR_Array<TR_BitVector> _liveOnExitConflicts;
    TR_Array<TR_BitVector> _exitEntryConflicts;
    TR_Array<TR_BitVector> _entryExitConflicts;
    TR_Array<TR_BitVector> _liveOnEntryUsage;
    TR_Array<TR_BitVector> _liveOnExitUsage;
#ifdef TRIM_ASSIGNED_CANDIDATES
    TR_Array<TR_BitVector> _availableBlocks; // blocks available due to empty loops
#endif

    // Candidate invariant info based on CFG
    TR_BitVector _firstBlock;
    TR_BitVector _isExtensionOfPreviousBlock;

public:
    struct coordinates {
        coordinates(uint32_t f, uint32_t l)
            : first(f)
            , last(l)
        {}

        uint32_t first, last;
    };

    typedef TR::typed_allocator<std::pair<int32_t const, struct coordinates>, TR::Region &> CoordinatesAllocator;
    typedef std::less<int32_t> CoordinatesComparator;
    typedef std::map<int32_t, struct coordinates, CoordinatesComparator, CoordinatesAllocator> Coordinates;

    typedef TR::typed_allocator<std::pair<uint32_t const, Coordinates *>, TR::Region &> ReferenceTableAllocator;
    typedef std::less<uint32_t> ReferenceTableComparator;
    typedef std::map<uint32_t, Coordinates *, ReferenceTableComparator, ReferenceTableAllocator> ReferenceTable;

protected:
    ReferenceTable *overlapTable;
};

} // namespace OMR

#endif
