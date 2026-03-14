/*******************************************************************************
 * Copyright IBM Corp. and others 2024
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

#include "optimizer/ConstRefPrivatization.hpp"

#include <algorithm>
#include "infra/ILWalk.hpp"
#include "optimizer/Optimization_inlines.hpp"
#include "optimizer/Structure.hpp"

#define INVALID_MUI UINT32_MAX
#define INVALID_CI UINT32_MAX
#define INVALID_FDI UINT32_MAX

// Bound-checked access with a fatal assertion. The method vector::at throws an
// exception if an out-of-bounds access is attempted, which will just fail the
// compilation and could easily go unnoticed.
template<typename T>
static typename TR::vector<T, TR::Region &>::reference at(TR::vector<T, TR::Region &> &v,
    typename TR::vector<T, TR::Region &>::size_type i)
{
    TR_ASSERT_FATAL(i < v.size(), "index out of bounds");
    return v[i];
}

TR::ConstRefPrivatization::ConstRefPrivatization(TR::OptimizationManager *manager)
    : TR::Optimization(manager)
    , _knot(comp()->getKnownObjectTable())
    , _knotSize(_knot == NULL ? 0 : _knot->getEndIndex())
    , _memRegion(NULL) // the per-opt-pass stack region will be available in perform()
    , _sd(NULL)
    , _curRegionKindStr(NULL)
    , _regionNodes(NULL)
{}

TR::ConstRefPrivatization::StackMemData::StackMemData(TR::ConstRefPrivatization *opt)
    : _blockInfos(*opt->_memRegion)
    , _regionNodeStack(*opt->_memRegion)
    , _newConstRefLoads(*opt->_memRegion)
    , _predBlocks(*opt->_memRegion)
    , _criticalEdgeSplitPredBlocks(*opt->_memRegion)
    , _koiIsUsed(*opt->_memRegion)
    , _usedKois(*opt->_memRegion)
    , _koiToMui(*opt->_memRegion)
    , _muiToKoi(*opt->_memRegion)
    , _muiToCi(*opt->_memRegion)
    , _ciToMui(*opt->_memRegion)
    , _curBlockCiToTempLoad(*opt->_memRegion)
    , _loopLoadFrequencyByKoi(*opt->_memRegion)
    , _koiIsHoisted(*opt->_memRegion)
    , _usedMuis(opt)
    , _potentiallyAvailableMuis(opt)
    , _ciFutureLoadP(opt)
    , _ciLoadP(opt)
    , _initHereCis(opt)
    , _initIncomingCis(opt)
    , _initCis(opt)
    , _liveCis(opt)
    , _scratchCisBacking(opt)
{}

int32_t TR::ConstRefPrivatization::perform()
{
    if (!comp()->useConstRefs() || _knot == NULL) {
        return 0;
    }

    TR_ASSERT_FATAL(_knotSize >= 1 && _knot->isNull(0), "expect null index at 0");
    if (_knotSize == 1) {
        return 0;
    }

    _memRegion = &comp()->trMemory()->currentStackRegion();
    _sd = new (*_memRegion) StackMemData(this);

    BlockInfo emptyBlockInfo(*_memRegion);
    _sd->_blockInfos.resize(comp()->getFlowGraph()->getNextNodeNumber(), emptyBlockInfo);

    _sd->_koiIsUsed.resize(_knotSize, false);
    _sd->_koiToMui.resize(_knotSize, INVALID_MUI);
    if (!collectConstRefLoads()) {
        logprints(trace(), comp()->log(), "No significant const ref loads found.\n");
        return 1;
    }

    TR_Structure *rootStructure = comp()->getFlowGraph()->getStructure();
    TR_ASSERT_FATAL(rootStructure != NULL, "constRefPrivatization requires structure");

    TR_RegionStructure *rootRegion = rootStructure->asRegion();
    if (rootRegion != NULL) {
        processRegion(rootRegion);
    }

    return 1;
}

bool TR::ConstRefPrivatization::collectConstRefLoads()
{
    // The block here is the start of the extended block where the load appears.
    // This allows us to ignore old entries instead of needing to clear them
    // every time we encounter a new extended block.
    TR::vector<BlockNodePair, TR::Region &> latestLoadByKoi(*_memRegion);

    // Initialize fields explicitly because with = {} XLC thinks it could be uninitialized somehow.
    BlockNodePair zeroPair;
    zeroPair._block = NULL;
    zeroPair._node = NULL;
    latestLoadByKoi.resize(_knotSize, zeroPair);

    TR::vector<NodeTreePair, TR::Region &> curBlockLoads(*_memRegion);

    TR::Block *curExtBlock = NULL;
    TR::Block *curBlock = NULL;

    bool anySignificantConstRefLoadsFound = false;

    bool trace = this->trace();
    OMR::Logger *log = comp()->log();

    logprints(trace, log, "Detecting and commoning const ref loads...\n");

    TR::NodeChecklist nodesToCommon(comp());
    TR::TreeTop *start = comp()->getStartTree();
    for (TR::PostorderNodeIterator it(start, comp()); it != NULL; ++it) {
        TR::Node *node = it.currentNode();
        if (node->getOpCodeValue() == TR::BBStart) {
            // Move to the block we've just encountered.
            curBlock = node->getBlock();
            if (!curBlock->isExtensionOfPreviousBlock()) {
                curExtBlock = curBlock;
            }

            continue;
        }

        if (node->getOpCodeValue() == TR::BBEnd) {
            if (!curBlock->isCold() && !curBlock->hasExceptionPredecessors()) {
                BlockInfo &bi = at(_sd->_blockInfos, curBlock->getNumber());
                bi._constRefLoads = curBlockLoads;
                if (!curBlockLoads.empty()) {
                    anySignificantConstRefLoadsFound = true;
                }
            }

            curBlockLoads.clear();
        }

        if (node->getOpCodeValue() != TR::aload) {
            // Check for children that should be replaced with an earlier load.
            int32_t numChildren = node->getNumChildren();
            for (int32_t childIndex = 0; childIndex < numChildren; childIndex++) {
                TR::Node *child = node->getChild(childIndex);
                if (nodesToCommon.contains(child)) {
                    TR::KnownObjectTable::Index koi = child->getSymbolReference()->getKnownObjectIndex();

                    TR_ASSERT_FATAL_WITH_NODE(child, koi != TR::KnownObjectTable::UNKNOWN, "no known object index");

                    const BlockNodePair &latestLoad = at(latestLoadByKoi, koi);
                    TR_ASSERT_FATAL_WITH_NODE(child, latestLoad._block == curExtBlock, "no earlier load in same EBB");

                    node->setAndIncChild(childIndex, latestLoad._node);
                    child->recursivelyDecReferenceCount();

                    logprintf(trace, log, "n%un [%p]: replaced child %d n%un [%p] with n%un [%p]\n",
                        node->getGlobalIndex(), node, childIndex, child->getGlobalIndex(), child,
                        latestLoad._node->getGlobalIndex(), latestLoad._node);
                }
            }

            // Beyond replacing children for commoning, only aload nodes are of interest.
            continue;
        }

        TR::SymbolReference *symRef = node->getSymbolReference();
        if (!symRef->hasKnownObjectIndex()) {
            continue;
        }

        if (symRef->getSymbol()->isAuto()) {
            // Ignore known object temps. They should have been replaced with
            // const static loads by now.
            continue;
        }

        TR::KnownObjectTable::Index koi = symRef->getKnownObjectIndex();
        TR_ASSERT_FATAL_WITH_NODE(node, symRef == _knot->constSymRef(koi),
            "known object index exists on unexpected symref");

        BlockNodePair &latestLoad = at(latestLoadByKoi, koi);
        if (latestLoad._block != curExtBlock) {
            // This is the first load of this const ref in the extended block.
            latestLoad._block = curExtBlock;
            latestLoad._node = node;
            logprintf(trace, log, "n%un [%p]: first obj%d load in extended block_%d\n", node->getGlobalIndex(), node,
                koi, curExtBlock->getNumber());
        } else {
            // We already have an earlier load of this object in the same extended
            // block. Usually it should have already been commoned, but it isn't
            // too hard to common it here just in case.
            if (performTransformation(comp(), "%s: Replacing n%un [%p] aload #%d (obj%d) with n%un [%p]\n",
                    optDetailString(), node->getGlobalIndex(), node, symRef->getReferenceNumber(), koi,
                    latestLoad._node->getGlobalIndex(), latestLoad._node)) {
                nodesToCommon.add(node);
                continue;
            }
        }

        NodeTreePair pair = { node, it.currentTree() };
        curBlockLoads.push_back(pair);
        logprintf(trace, log, "n%un [%p]: noted obj%d load at tree n%un [%p] in block_%d\n", node->getGlobalIndex(),
            node, koi, it.currentTree()->getNode()->getGlobalIndex(), it.currentTree()->getNode(),
            curBlock->getNumber());
    }

    return anySignificantConstRefLoadsFound;
}

TR::ConstRefPrivatization::RegionStackEntry::RegionStackEntry(TR_StructureSubGraphNode *node)
    : _node(node)
{
    TR::CFGEdgeList &successors = node->getSuccessors();
    _it = successors.begin();
    _end = successors.end();
}

void TR::ConstRefPrivatization::processRegion(TR_RegionStructure *region)
{
    // Mark all of the subnodes as ignored. The ones we actually want to process
    // will be found in populateRegionNodes() and sequential indices will be
    // assigned.
    uint32_t numSubnodes = 0;
    for (auto it = region->begin(); it != region->end(); it++) {
        BlockInfo &blockInfo = at(_sd->_blockInfos, (*it)->getNumber());
        blockInfo._flowDataIndex = (uint32_t)-1;
        blockInfo._ignored = true;
        blockInfo._isExit = false;
        numSubnodes++;
    }

    // Mark exit subnodes as ignored as well. That way the exit successor case
    // won't need to be handled explicitly (except in populateRegionNodes()).
    ListIterator<TR::CFGEdge> exitIt(&region->getExitEdges());
    for (TR::CFGEdge *edge = exitIt.getFirst(); edge != NULL; edge = exitIt.getNext()) {
        BlockInfo &blockInfo = at(_sd->_blockInfos, edge->getTo()->getNumber());
        blockInfo._flowDataIndex = (uint32_t)-1;
        blockInfo._ignored = true;
        blockInfo._isExit = true;
    }

    // Search through the region to find the subnodes that we want to process.
    // Subnodes that we should skip will still be marked as ignored, so the
    // ignored flag can be used to determine which subregions to skip.
    SubNodeVector regionNodes(*_memRegion);
    regionNodes.reserve(numSubnodes);
    populateRegionNodes(region, regionNodes);
    for (auto it = regionNodes.begin(); it != regionNodes.end(); it++) {
        at(_sd->_blockInfos, (*it)->getNumber())._ignored = false;
    }

    // Process subregions before this one.
    SubNodeVector unignoredSubregionExits(*_memRegion);
    for (auto it = regionNodes.begin(); it != regionNodes.end(); it++) {
        TR_RegionStructure *subregion = (*it)->getStructure()->asRegion();
        if (subregion != NULL) {
            if (at(_sd->_blockInfos, subregion->getNumber())._ignored) {
                ignoreRegion(subregion);
            } else {
                // Processing the subregion will set the ignored flag on its exit
                // edge targets. Remember which are supposed to be unignored and
                // set them back afterward.
                unignoredSubregionExits.clear();
                ListIterator<TR::CFGEdge> subExitIt(&subregion->getExitEdges());
                for (TR::CFGEdge *edge = subExitIt.getFirst(); edge != NULL; edge = subExitIt.getNext()) {
                    TR_StructureSubGraphNode *exit = toStructureSubGraphNode(edge->getTo());
                    if (!at(_sd->_blockInfos, exit->getNumber())._ignored) {
                        unignoredSubregionExits.push_back(exit);
                    }
                }

                processRegion(subregion);

                for (auto it = unignoredSubregionExits.begin(); it != unignoredSubregionExits.end(); it++) {
                    at(_sd->_blockInfos, (*it)->getNumber())._ignored = false;
                }
            }
        }
    }

    // Process this region now that subregions have been processed.
    if (!region->isAcyclic() && !region->isNaturalLoop()) {
        // We could process part of the improper region if we went through
        // regionNodes to remove and ignore everything reachable from a cycle,
        // which thanks to reverse postorder would be straightforward. However,
        // the main concern for improper regions is most likely just to process
        // nested loops within them, e.g. hot loops from which loop transfer has
        // been done, and that has already happened above. So for the time being,
        // just give up here.
        logprintf(trace(), comp()->log(), "\nSkip improper region %d [%p]\n", region->getNumber(), region);
        return;
    }

    _curRegionKindStr = regionKindStr(region);
    logprintf(trace(), comp()->log(), "\nProcessing %s %d [%p]\n", _curRegionKindStr, region->getNumber(), region);

    // Set flow data indices to the index of each node in regionNodes. The count
    // (and so these indices) fit in uint32_t because block numbers are int32_t.
    TR_ASSERT_FATAL(regionNodes.size() <= UINT32_MAX, "region [%p]: too many subnodes", region);

    uint32_t nextFlowDataIndex = 0;
    for (auto it = regionNodes.begin(); it != regionNodes.end(); it++) {
        TR_StructureSubGraphNode *node = *it;
        BlockInfo &blockInfo = at(_sd->_blockInfos, node->getNumber());
        TR_ASSERT_FATAL(!blockInfo._ignored, "region [%p]: subnode %d unexpectedly became ignored", region,
            node->getNumber());

        blockInfo._flowDataIndex = nextFlowDataIndex++;
    }

    // Finally we can actually look for opportunities in this region.
    _regionNodes = &regionNodes;
    findMultiplyUsedConstRefs();
    doAcyclicAnalysisAndTransform(region);

    if (region->isNaturalLoop()) {
        hoistConstRefLoads(region);
    }

    // Reset _koiToMui, _muiToKoi for the next region.
    for (auto it = _sd->_muiToKoi.begin(); it != _sd->_muiToKoi.end(); it++) {
        TR::KnownObjectTable::Index koi = *it;
        at(_sd->_koiToMui, koi) = INVALID_MUI;
    }

    _sd->_muiToKoi.clear();
}

void TR::ConstRefPrivatization::populateRegionNodes(TR_RegionStructure *region, SubNodeVector &regionNodes)
{
    // Populate _regionNodes with a reverse postorder traversal of the subnodes
    // within this region ignoring cold blocks and exception edges.
    //
    // For an acyclic region, this will be a topological sort.
    //
    // For a natural loop, it will be a topological sort of the loop body
    // ignoring back-edges. The analysis will ignore all back-edges and treat
    // the body as an acyclic region representing any single iteration.
    //
    // In an improper region, this will not be a topological sort, but it still
    // has the property that if x, y are nodes, and if there is a path from x to
    // y but not vice versa, then x precedes y in the reverse postorder.
    //
    regionNodes.clear();

    for (auto it = region->begin(); it != region->end(); it++) {
        at(_sd->_blockInfos, (*it)->getNumber())._seen = false;
    }

    TR_ASSERT_FATAL(_sd->_regionNodeStack.empty(), "stray elements left in stack");

    at(_sd->_blockInfos, region->getEntry()->getNumber())._seen = true;
    _sd->_regionNodeStack.push_back(RegionStackEntry(region->getEntry()));
    while (!_sd->_regionNodeStack.empty()) {
        RegionStackEntry *top = &_sd->_regionNodeStack.back();
        while (top->_it != top->_end) {
            TR_StructureSubGraphNode *succ = toStructureSubGraphNode((*top->_it++)->getTo());
            BlockInfo &succInfo = at(_sd->_blockInfos, succ->getNumber());
            if (!succInfo._seen) {
                succInfo._seen = true;
                if (!succInfo._isExit && !succ->getStructure()->getEntryBlock()->isCold()) {
                    _sd->_regionNodeStack.push_back(RegionStackEntry(succ));
                    top = &_sd->_regionNodeStack.back();
                }
            }
        }

        regionNodes.push_back(top->_node);
        _sd->_regionNodeStack.pop_back();
    }

    std::reverse(regionNodes.begin(), regionNodes.end());
}

void TR::ConstRefPrivatization::ignoreRegion(TR_RegionStructure *region)
{
    logprintf(trace(), comp()->log(), "Ignore %s %d [%p]\n", regionKindStr(region), region->getNumber(), region);
    ignoreRegionRec(region);
}

void TR::ConstRefPrivatization::ignoreRegionRec(TR_RegionStructure *region)
{
    for (auto it = region->begin(); it != region->end(); it++) {
        TR_StructureSubGraphNode *node = *it;
        at(_sd->_blockInfos, node->getNumber())._ignored = true;

        TR_RegionStructure *subRegion = node->getStructure()->asRegion();
        if (subRegion != NULL) {
            ignoreRegionRec(subRegion);
        }
    }
}

void TR::ConstRefPrivatization::findMultiplyUsedConstRefs()
{
    bool trace = this->trace();
    OMR::Logger *log = comp()->log();

    // Look at all const ref loads in (unignored) blocks directly within this
    // region, and find any that are loaded in multiple places. Assign dense
    // indices ("MUI" or "multiply-used index") to any that are found.
    for (auto nodeIt = _regionNodes->begin(); nodeIt != _regionNodes->end(); nodeIt++) {
        TR_BlockStructure *s = (*nodeIt)->getStructure()->asBlock();
        if (s == NULL) {
            continue;
        }

        const auto &loads = at(_sd->_blockInfos, s->getNumber())._constRefLoads;
        for (auto it = loads.begin(); it != loads.end(); it++) {
            TR::KnownObjectTable::Index koi = it->_node->getSymbolReference()->getKnownObjectIndex();

            if (!at(_sd->_koiIsUsed, koi)) {
                at(_sd->_koiIsUsed, koi) = true;
                _sd->_usedKois.push_back(koi);
                logprintf(trace, log, "block_%d: obj%d is used\n", s->getNumber(), koi);
            } else if (at(_sd->_koiToMui, koi) == INVALID_MUI) {
                // Make sure that truncation preserves the value. Checking that the
                // size is strictly less than the max now ensures it will still be
                // at most the max after pushing.
                //
                // This will succeed because node global indices are uint32_t, so
                // there are no more than UINT32_MAX nodes in the trees. Each
                // multiply-used const ref uses up two nodes, so there are at most
                // UINT32_MAX / 2 of them.
                //
                TR_ASSERT_FATAL(_sd->_muiToKoi.size() < UINT32_MAX, "too many multiply-used const refs");

                uint32_t mui = (uint32_t)_sd->_muiToKoi.size();
                at(_sd->_koiToMui, koi) = mui;
                _sd->_muiToKoi.push_back(koi);
                logprintf(trace, log, "block_%d: obj%d is multiply used: mui=%u\n", s->getNumber(), koi, mui);
            }
        }
    }

    // Reset all of the kois to unused for next time.
    for (auto it = _sd->_usedKois.begin(); it != _sd->_usedKois.end(); it++) {
        at(_sd->_koiIsUsed, *it) = false;
    }

    _sd->_usedKois.clear();
}

// For an overview of the acyclic analysis with examples,
// see doc/compiler/optimizer/ConstRefPrivatization.md.
void TR::ConstRefPrivatization::doAcyclicAnalysisAndTransform(TR_RegionStructure *region)
{
    bool trace = this->trace();
    OMR::Logger *log = comp()->log();

    if (_sd->_muiToKoi.empty()) {
        logprints(trace, log, "\nNo multiply used const refs; skip acyclic analysis\n");
        return;
    }

    // Make sure we won't overflow when multiplying to size FlowVectors.
    // numMuis fits in uint32_t because of the assertion in findMultiplyUsedConstRefs().
    uint32_t numMuis = (uint32_t)_sd->_muiToKoi.size();
    if ((size_t)numMuis > SIZE_MAX / _regionNodes->size()) {
        logprints(trace, log, "\nToo many subnodes * MUIs; give up on acyclic analysis\n\n");
        return;
    }

    // First pass: populate _usedMuis, compute _potentiallyAvailableMuis and
    // determine whether there is potentially an opportunity in this region.
    logprints(trace, log, "\nAcyclic analysis: first pass: used and potentially available MUIs\n");

    TR_StructureSubGraphNode *regionEntry = region->getEntry();
    uint32_t numCandidates = 0;

    _sd->_muiToCi.clear();
    _sd->_muiToCi.resize(numMuis, INVALID_CI);
    _sd->_ciToMui.clear();

    _sd->_usedMuis.reset(numMuis);
    _sd->_potentiallyAvailableMuis.reset(numMuis);
    for (auto nodeIt = _regionNodes->begin(); nodeIt != _regionNodes->end(); nodeIt++) {
        TR_StructureSubGraphNode *node = *nodeIt;
        BlockInfo &blockInfo = at(_sd->_blockInfos, node->getNumber());

        // Determine the set of MUIs loaded in this block, ignoring nested regions.
        FlowBitSet::NodeView curUsedMuis = _sd->_usedMuis.atNode(blockInfo);
        if (node->getStructure()->asBlock() != NULL) {
            auto loadsEnd = blockInfo._constRefLoads.end();
            for (auto it = blockInfo._constRefLoads.begin(); it != loadsEnd; it++) {
                TR::KnownObjectTable::Index koi = it->_node->getSymbolReference()->getKnownObjectIndex();

                uint32_t mui = at(_sd->_koiToMui, koi);
                if (mui != INVALID_MUI) {
                    curUsedMuis.setBit(mui);
                }
            }
        }

        // Determine the set of MUIs that might have been loaded (by an unignored
        // block directly in this region) before entering this node.
        FlowBitSet::NodeView curPotentiallyAvailableMuis = _sd->_potentiallyAvailableMuis.atNode(blockInfo);

        if (node != regionEntry) // ignore back-edges
        {
            const TR::CFGEdgeList &preds = node->getPredecessors();
            for (auto it = preds.begin(); it != preds.end(); it++) {
                BlockInfo &predInfo = at(_sd->_blockInfos, (*it)->getFrom()->getNumber());
                if (!predInfo._ignored) {
                    curPotentiallyAvailableMuis.unionWith(_sd->_potentiallyAvailableMuis.atNode(predInfo));
                    curPotentiallyAvailableMuis.unionWith(_sd->_usedMuis.atNode(predInfo));
                }
            }
        }

        // There is potentially an opportunity if a block loads a known object
        // that might have already been loaded beforehand.
        bool potentialOpportunityHere = curUsedMuis.overlaps(curPotentiallyAvailableMuis);
        if (trace) {
            log->printf("subnode %d:\n  used MUIs:", node->getNumber());
            traceBitSet(curUsedMuis);
            log->prints("\n  potentially available:");
            traceBitSet(curPotentiallyAvailableMuis);
            log->printf("\n  potential opportunity: %s\n", potentialOpportunityHere ? "yes" : "no");
        }

        if (potentialOpportunityHere) {
            auto it = curUsedMuis.iter();
            uint32_t mui = 0;
            while (it.next(&mui)) {
                if (curPotentiallyAvailableMuis.isBitSet(mui) && at(_sd->_muiToCi, mui) == INVALID_CI) {
                    // The candidate index fits into uint32_t because the candidates
                    // are a strict subset of the multiply used objects.
                    TR_ASSERT_FATAL(numCandidates < UINT32_MAX, "candidate index overflow");
                    uint32_t ci = numCandidates++;
                    at(_sd->_muiToCi, mui) = ci;
                    _sd->_ciToMui.push_back(mui);
                    logprintf(trace, log, "  mui%u is a candidate, ci%u\n", mui, ci);
                }
            }
        }
    }

    if (numCandidates == 0) {
        logprints(trace, log, "\nAcyclic analysis: no candidates found; stop\n\n");
        return;
    }

    // Second pass: compute _ciLoadP, _ciFutureLoadP, and find blocks doing
    // first-time loads that can initialize the corresponding temps.
    logprintf(trace, log,
        "\nAcyclic analysis: %u candidates found\n  second pass: candidate load probability, temp init blocks\n",
        numCandidates);

    _sd->_ciFutureLoadP.reset(numCandidates, 0.0f);
    _sd->_ciLoadP.reset(numCandidates, 0.0f);
    _sd->_initHereCis.reset(numCandidates);
    bool didSetAtLeastOneInitCi = false;
    for (auto nodeIt = _regionNodes->rbegin(); nodeIt != _regionNodes->rend(); nodeIt++) {
        TR_StructureSubGraphNode *node = *nodeIt;
        BlockInfo &blockInfo = at(_sd->_blockInfos, node->getNumber());

        // Look at successors for prob. of loading candidates after exiting node.
        // Successors with only a single predecessor (namely node) will get the
        // corresponding fraction of curNodeFreq. All of the remaining frequency
        // will be divided amongst successors with other predecessors in
        // proportion to their frequencies.
        FlowVector<float>::NodeView curCiFutureLoadP = _sd->_ciFutureLoadP.atNode(blockInfo);
        int32_t curNodeFreq = node->getStructure()->getEntryBlock()->getFrequency();
        int32_t totalFreqOfSuccsWith1Pred = 0;
        int32_t totalFreqOfOtherSuccs = 0;
        int32_t significantSuccCount = 0;
        TR::CFGEdgeList &succs = node->getSuccessors();
        for (auto sIt = succs.begin(); sIt != succs.end(); sIt++) {
            TR_StructureSubGraphNode *succ = toStructureSubGraphNode((*sIt)->getTo());
            if (succ == regionEntry) {
                continue; // ignore back-edges
            }

            BlockInfo &succInfo = at(_sd->_blockInfos, succ->getNumber());
            if (succInfo._ignored) {
                continue;
            }

            significantSuccCount++;
            int32_t succFreq = succ->getStructure()->getEntryBlock()->getFrequency();
            if (succ->getPredecessors().size() == 1) {
                totalFreqOfSuccsWith1Pred += succFreq;
            } else {
                totalFreqOfOtherSuccs += succFreq;
            }
        }

        totalFreqOfSuccsWith1Pred = (int32_t)clamp(0.0f, (float)totalFreqOfSuccsWith1Pred, (float)curNodeFreq);

        for (auto sIt = succs.begin(); sIt != succs.end(); sIt++) {
            TR_StructureSubGraphNode *succ = toStructureSubGraphNode((*sIt)->getTo());
            if (succ == regionEntry) {
                continue; // ignore back-edges
            }

            BlockInfo &succInfo = at(_sd->_blockInfos, succ->getNumber());
            if (succInfo._ignored) {
                continue;
            }

            int32_t succFreq = succ->getStructure()->getEntryBlock()->getFrequency();
            float succP = 0.0f;
            if (significantSuccCount == 1) {
                succP = 1.0f;
            } else if (succ->getPredecessors().size() == 1) {
                succP = (float)succFreq / (float)curNodeFreq;
            } else {
                succP = ((float)succFreq / (float)totalFreqOfOtherSuccs)
                    * ((float)(curNodeFreq - totalFreqOfSuccsWith1Pred) / (float)curNodeFreq);
            }

            succP = clamp(0.0f, succP, 1.0f);

            FlowVector<float>::NodeView succCiLoadP = _sd->_ciLoadP.atNode(succInfo);
            for (uint32_t ci = 0; ci < numCandidates; ci++) {
                curCiFutureLoadP[ci] += succP * succCiLoadP[ci];
            }
        }

        // Clamp all entries to [0, 1].
        for (uint32_t ci = 0; ci < numCandidates; ci++) {
            float &p = curCiFutureLoadP[ci];
            p = clamp(0.0f, p, 1.0f);
        }

        // Determine the prob. of loading candidates here or later.
        // Candidates used directly by this block are loaded with probability 1,
        // ignoring the possibility of an exception or nonterminating call.
        FlowVector<float>::NodeView curCiLoadP = _sd->_ciLoadP.atNode(blockInfo);
        FlowBitSet::NodeView curUsedMuis = _sd->_usedMuis.atNode(blockInfo);
        for (uint32_t ci = 0; ci < numCandidates; ci++) {
            if (curUsedMuis.isBitSet(at(_sd->_ciToMui, ci))) {
                curCiLoadP[ci] = 1.0f;
            } else {
                curCiLoadP[ci] = curCiFutureLoadP[ci];
            }
        }

        // Identify which candidates to consider storing to a temp in this node.
        const float mightLoadLaterThresholdP = 0.05;

        FlowBitSet::NodeView curInitHereCis = _sd->_initHereCis.atNode(blockInfo);
        FlowBitSet::NodeView curPotentiallyAvailableMuis = _sd->_potentiallyAvailableMuis.atNode(blockInfo);

        for (uint32_t ci = 0; ci < numCandidates; ci++) {
            uint32_t mui = at(_sd->_ciToMui, ci);
            if (curUsedMuis.isBitSet(mui) && !curPotentiallyAvailableMuis.isBitSet(mui)
                && curCiFutureLoadP[ci] >= mightLoadLaterThresholdP) {
                // This is a first-time load with a chance of being reused later.
                curInitHereCis.setBit(ci);
                didSetAtLeastOneInitCi = true;
            }
        }

        if (trace) {
            log->printf("subnode %d:\n  future load p:", node->getNumber());
            traceCiLoadPs(curCiFutureLoadP);
            log->prints("\n         load p:");
            traceCiLoadPs(curCiLoadP);
            log->prints("\n  init here CIs:");
            traceBitSet(curInitHereCis);
            log->println();
        }
    }

    // If we were to set the init CI bit based on mere reachability of a later
    // use, then there would be a guarantee that at least one such bit would
    // have been set at this point. But it's possible that none were set because
    // this is actually based on a probability threshold with a probability
    // estimate from block frequencies.
    if (!didSetAtLeastOneInitCi) {
        logprints(trace, log, "\nAcyclic analysis: no temp init blocks found; stop\n\n");
        return;
    }

    // Third pass: compute _initCis, add merge points that are expected to
    // load frequently enough in the future to _initIncomingCis if possible, and
    // add any use point where initialization isn't guaranteed to _initHereCis if
    // it seems beneficial.
    logprints(trace, log, "\nAcyclic analysis: third pass: find temp init points\n");

    _sd->_scratchCisBacking.reset(numCandidates, 10);
    FlowBitSet::NodeView maybeInitCis = _sd->_scratchCisBacking.atNode(0);
    FlowBitSet::NodeView wantToInitCis = _sd->_scratchCisBacking.atNode(1);
    FlowBitSet::NodeView edgeWantCis = _sd->_scratchCisBacking.atNode(2);
    FlowBitSet::NodeView criticalEdgeWantCis = _sd->_scratchCisBacking.atNode(3);
    FlowBitSet::NodeView uninitCis = _sd->_scratchCisBacking.atNode(4);
    FlowBitSet::NodeView liveAnywhereCis = _sd->_scratchCisBacking.atNode(5);
    FlowBitSet::NodeView liveOnExitCis = _sd->_scratchCisBacking.atNode(6);
    FlowBitSet::NodeView toTransformCis = _sd->_scratchCisBacking.atNode(7);
    FlowBitSet::NodeView toTransformHereCis = _sd->_scratchCisBacking.atNode(8);
    FlowBitSet::NodeView toTransformIncomingCis = _sd->_scratchCisBacking.atNode(9);

    _sd->_initIncomingCis.reset(numCandidates);
    _sd->_initCis.reset(numCandidates);
    _sd->_liveCis.reset(numCandidates);
    bool liveCandidatesExist = false;
    for (auto nodeIt = _regionNodes->begin(); nodeIt != _regionNodes->end(); nodeIt++) {
        TR_StructureSubGraphNode *node = *nodeIt;
        BlockInfo &blockInfo = at(_sd->_blockInfos, node->getNumber());
        FlowBitSet::NodeView curInitCis = _sd->_initCis.atNode(blockInfo);

        maybeInitCis.clearAllBits();

        bool hasIgnoredPred = false;
        if (node != regionEntry) { // ignore back-edges
            const TR::CFGEdgeList &preds = node->getPredecessors();
            bool firstPred = true;
            for (auto predIt = preds.begin(); predIt != preds.end(); predIt++) {
                BlockInfo &predInfo = at(_sd->_blockInfos, (*predIt)->getFrom()->getNumber());
                if (predInfo._ignored) {
                    // We'll init along edges incoming from ignored subnodes later
                    // if/where necessary.
                    hasIgnoredPred = true;
                    continue;
                }

                FlowBitSet::NodeView predInitCis = _sd->_initCis.atNode(predInfo);
                if (firstPred) {
                    curInitCis.copyFrom(predInitCis);
                    firstPred = false;
                } else {
                    curInitCis.intersectWith(predInitCis);
                }

                // Collect all of the possibly initialized candidates here.
                maybeInitCis.unionWith(predInitCis);
            }
        }

        // Find all of the candidates that were initialized on the way in from
        // some but not all predecessors, and decide which of them, if any, to
        // initialize along incoming edges.
        maybeInitCis.subtract(curInitCis);

        FlowBitSet::NodeView curInitHereCis = _sd->_initHereCis.atNode(blockInfo);
        FlowBitSet::NodeView curInitIncomingCis = _sd->_initIncomingCis.atNode(blockInfo);
        FlowVector<float>::NodeView curCiFutureLoadP = _sd->_ciFutureLoadP.atNode(blockInfo);

        // If this subnode has one or more ignored predecessors, then everything
        // that's guaranteed to be initialized when coming from the happy path
        // would need to be initialized in the ignored predecessors as well.
        //
        // NOTE: It's always possible to guarantee these will be initialized
        // because they're only (potentially) uninitialized when coming from an
        // ignored predecessor, and it's fine to store in an ignored predecessor
        // even if there's a critical edge.
        //
        if (hasIgnoredPred) {
            curInitIncomingCis.unionWith(curInitCis);
        }

        const float willLoadLaterThresholdP = 0.95;

        bool haveWantToInitCandidates = false;
        wantToInitCis.clearAllBits();

        auto maybeInitCiIter = maybeInitCis.iter();
        uint32_t ci = 0;
        while (maybeInitCiIter.next(&ci)) {
            // TODO: This should probably consult _ciLoadP instead of _ciFutureLoadP.
            // If the current block loads the candidate itself, then surely that's at
            // least as good a justification as a load in a highly likely successor.
            // If this is updated, also update the analysis overview doc.
            if (curCiFutureLoadP[ci] >= willLoadLaterThresholdP) {
                wantToInitCis.setBit(ci);
                haveWantToInitCandidates = true;
            }
        }

        if (haveWantToInitCandidates) {
            bool haveSetCriticalEdgeWantCis = false;
            bool incomingOk = true;
            const TR::CFGEdgeList &preds = node->getPredecessors();
            for (auto predIt = preds.begin(); predIt != preds.end() && incomingOk; predIt++) {
                TR_StructureSubGraphNode *pred = toStructureSubGraphNode((*predIt)->getFrom());

                BlockInfo &predInfo = at(_sd->_blockInfos, pred->getNumber());
                if (predInfo._ignored) {
                    edgeWantCis.copyFrom(wantToInitCis);
                } else if (!_sd->_initCis.atNode(predInfo).contains(wantToInitCis)) {
                    edgeWantCis.copyFrom(wantToInitCis);
                    edgeWantCis.subtract(_sd->_initCis.atNode(predInfo));
                } else {
                    // All the candidates we want are already initialized when
                    // coming from this predecessor.
                    continue;
                }

                // This edge could represent multiple predecessor blocks if pred is
                // a region structure.
                collectPredBlocks(node, pred);
                for (auto it = _sd->_predBlocks.begin(); it != _sd->_predBlocks.end(); it++) {
                    // Any predecessor with only a single successor (namely node) is
                    // fine because we can generate exactly the needed stores in
                    // that block.
                    TR::Block *predBlock = *it;
                    if (predBlock->getSuccessors().size() == 1) {
                        continue;
                    }

                    // It's fine to generate the needed stores in ignored blocks
                    // regardless of the number of successors they have. An ignored
                    // block is only reachable via a cold block or exception edge.
                    if (at(_sd->_blockInfos, predBlock->getNumber())._ignored) {
                        continue;
                    }

                    // Restrict unignored predecessor blocks with multiple successors
                    // to a single set of candidate stores. Don't split tons of edges.
                    if (!haveSetCriticalEdgeWantCis) {
                        // This is the first predecessor block with a critical edge
                        // (that we care about). Record the candidates that would
                        // need to be initialized on the way in from this one.
                        criticalEdgeWantCis.copyFrom(edgeWantCis);
                        haveSetCriticalEdgeWantCis = true;
                    } else if (!edgeWantCis.equals(criticalEdgeWantCis)) {
                        incomingOk = false;
                        break;
                    }
                }
            }

            if (incomingOk) {
                curInitIncomingCis.copyFrom(wantToInitCis);
            }
        }

        // Take note of the initialized status of candidates to be initialized in
        // this subnode.
        curInitCis.unionWith(curInitHereCis);
        curInitCis.unionWith(curInitIncomingCis);

        // If we can't establish that something is initialized on entry, it might
        // still be beneficial to initialize it in this subnode.
        FlowBitSet::NodeView curUsedMuis = _sd->_usedMuis.atNode(blockInfo);
        uninitCis.setAllBits();
        uninitCis.subtract(curInitCis);
        auto uninitCiIter = uninitCis.iter();
        while (uninitCiIter.next(&ci)) {
            if (curUsedMuis.isBitSet(at(_sd->_ciToMui, ci)) && curCiFutureLoadP[ci] >= willLoadLaterThresholdP) {
                curInitHereCis.setBit(ci);
                curInitCis.setBit(ci);
            }
        }

        // Determine the set of candidates live on entry to this subnode simply
        // as a result of uses within it - essentially the gen set. Liveness will
        // be propagated backward afterward if any live candidates are found.
        FlowBitSet::NodeView curLiveCis = _sd->_liveCis.atNode(blockInfo);
        FlowBitSet::NodeView curPotentiallyAvailableMuis = _sd->_potentiallyAvailableMuis.atNode(blockInfo);

        auto initCiIter = curInitCis.iter();
        while (initCiIter.next(&ci)) {
            uint32_t mui = at(_sd->_ciToMui, ci);
            // TODO: This should probably also require !curInitHereCis.isBitSet(ci).
            // Setting the bit in _liveCis is supposed to mean that the temp will be
            // live on entry to node, but it won't be if node itself is supposed to
            // initialize the temp. And in that case, even if the const ref load were
            // transformed into a temp load, that transformation would be pointless.
            // Temp loads are needed only to reuse the result from an earlier block.
            // If this is updated, the corresponding part of the analysis overview
            // documentation should also be updated.
            if (curUsedMuis.isBitSet(mui) && curPotentiallyAvailableMuis.isBitSet(mui)) {
                curLiveCis.setBit(ci);
                liveAnywhereCis.setBit(ci);
                liveCandidatesExist = true;
            }
        }

        if (trace) {
            log->printf("subnode %d:\n  init here CIs:", node->getNumber());
            traceBitSet(curInitHereCis);
            log->prints("\n  init inc. CIs:");
            traceBitSet(curInitIncomingCis);
            log->prints("\n   all init CIs:");
            traceBitSet(curInitCis);
            log->prints("\n   gen live CIs:");
            traceBitSet(curLiveCis);
            log->println();
        }
    }

    if (!liveCandidatesExist) {
        logprints(trace, log, "\nAcyclic analysis: no live candidates; stop\n\n");
        return;
    }

    // Fourth pass: propagate _liveCis.
    logprints(trace, log, "\nAcyclic analysis: fourth pass: candidate temp liveness\n");

    for (auto nodeIt = _regionNodes->rbegin(); nodeIt != _regionNodes->rend(); nodeIt++) {
        TR_StructureSubGraphNode *node = *nodeIt;
        BlockInfo &blockInfo = at(_sd->_blockInfos, node->getNumber());
        FlowBitSet::NodeView curLiveCis = _sd->_liveCis.atNode(blockInfo);

        TR::CFGEdgeList &succs = node->getSuccessors();
        for (auto sIt = succs.begin(); sIt != succs.end(); sIt++) {
            TR_StructureSubGraphNode *succ = toStructureSubGraphNode((*sIt)->getTo());
            if (succ == regionEntry) {
                continue; // ignore back-edges
            }

            BlockInfo &succInfo = at(_sd->_blockInfos, succ->getNumber());
            if (!succInfo._ignored) {
                curLiveCis.unionWith(_sd->_liveCis.atNode(succInfo));
            }
        }

        // Kill any that will have a temp initialized at the first use in this node.
        // Earlier definitions won't be seen here. Also kill any that are not
        // guaranteed to be initialized by the time control leaves this node.
        curLiveCis.subtract(_sd->_initHereCis.atNode(blockInfo));
        curLiveCis.intersectWith(_sd->_initCis.atNode(blockInfo));

        if (trace) {
            log->printf("subnode %d:\n  live CIs:", node->getNumber());
            traceBitSet(curLiveCis);
            log->println();
        }
    }

    logprintln(trace, log);

    //
    // Transformation!
    //

    // Determine which candidates to transform.
    toTransformCis.clearAllBits();

    BlockInfo &regionEntryInfo = at(_sd->_blockInfos, region->getNumber());
    auto liveAnywhereCiIter = liveAnywhereCis.iter();
    uint32_t liveAnywhereCi = 0;
    while (liveAnywhereCiIter.next(&liveAnywhereCi)) {
        uint32_t mui = at(_sd->_ciToMui, liveAnywhereCi);
        TR::KnownObjectTable::Index koi = at(_sd->_muiToKoi, mui);
        if (performTransformation(comp(), "%sPrivatize obj%d (mui%u, ci%u) in %s %d [%p] using temp #%d\n",
                optDetailString(), koi, mui, liveAnywhereCi, _curRegionKindStr, region->getNumber(), region,
                knownObjTemp(koi)->getReferenceNumber())) {
            toTransformCis.setBit(liveAnywhereCi);
        }
    }

    // Initialize fields explicitly because with = {} XLC thinks it could be uninitialized somehow.
    BlockNodePair zeroBlockNodePair;
    zeroBlockNodePair._block = NULL;
    zeroBlockNodePair._node = NULL;
    _sd->_curBlockCiToTempLoad.clear();
    _sd->_curBlockCiToTempLoad.resize(numCandidates, zeroBlockNodePair);

    for (auto nodeIt = _regionNodes->begin(); nodeIt != _regionNodes->end(); nodeIt++) {
        TR_StructureSubGraphNode *node = *nodeIt;
        TR::Block *curBlock = node->getStructure()->getEntryBlock();
        BlockInfo &blockInfo = at(_sd->_blockInfos, node->getNumber());

        toTransformHereCis.copyFrom(_sd->_initHereCis.atNode(blockInfo));
        toTransformHereCis.intersectWith(toTransformCis);

        toTransformIncomingCis.copyFrom(_sd->_initIncomingCis.atNode(blockInfo));
        toTransformIncomingCis.intersectWith(toTransformCis);

        if (!toTransformHereCis.areAllBitsClear() || !toTransformIncomingCis.areAllBitsClear()) {
            // There might be stores to do in or just prior to this node.

            // Along incoming edges, we only need to initialize candidates that
            // are live on entry to node.
            toTransformIncomingCis.intersectWith(_sd->_liveCis.atNode(blockInfo));

            // Within node itself, we only need to initialize candidates that are
            // live on exit from node.
            liveOnExitCis.clearAllBits();

            TR::CFGEdgeList &succs = node->getSuccessors();
            for (auto sIt = succs.begin(); sIt != succs.end(); sIt++) {
                TR_StructureSubGraphNode *succ = toStructureSubGraphNode((*sIt)->getTo());
                if (succ == regionEntry) {
                    continue; // ignore back-edges
                }

                BlockInfo &succInfo = at(_sd->_blockInfos, succ->getNumber());
                if (!succInfo._ignored) {
                    liveOnExitCis.unionWith(_sd->_liveCis.atNode(succInfo));
                }
            }

            toTransformHereCis.intersectWith(liveOnExitCis);
        }

        if (!toTransformHereCis.areAllBitsClear()) {
            // There are stores to generate within node at the first point of use.
            for (auto it = blockInfo._constRefLoads.begin(); it != blockInfo._constRefLoads.end(); it++) {
                TR::Node *loadNode = it->_node;
                TR::TreeTop *loadTree = it->_tree;
                TR::KnownObjectTable::Index koi = loadNode->getSymbolReference()->getKnownObjectIndex();

                uint32_t mui = at(_sd->_koiToMui, koi);
                if (mui == INVALID_MUI) {
                    continue;
                }

                uint32_t ci = at(_sd->_muiToCi, mui);
                if (ci == INVALID_CI || !toTransformHereCis.isBitSet(ci)) {
                    continue;
                }

                // This is a first-time load that should be stored into a temp.
                TR::SymbolReference *temp = knownObjTemp(koi);
                TR::Node *storeNode = TR::Node::createStore(loadNode, temp, loadNode);
                TR::TreeTop *storeTree = TR::TreeTop::create(comp(), storeNode);
                loadTree->insertBefore(storeTree);
                dumpOptDetails(comp(),
                    "%sStore obj%d into temp #%d at point of use in block_%d, store node n%un [%p]\n",
                    optDetailString(), koi, temp->getReferenceNumber(), node->getNumber(), storeNode->getGlobalIndex(),
                    storeNode);
            }
        }

        if (!toTransformIncomingCis.areAllBitsClear()) {
            // There are stores to generate along incoming edges.
            //
            // This can only happen at a merge point, since we only try to set
            // temps along incoming edges when they're already initialized in some
            // predecessors but not others.
            //
            TR_ASSERT_FATAL(!curBlock->isExtensionOfPreviousBlock() && curBlock->getPredecessors().size() >= 2,
                "region [%p]: extension block_%d has temps to initialize along incoming edges", region,
                curBlock->getNumber());

            // This can't be the region entry, since (ignoring back-edges) the
            // entry has no predecessors within the region.
            TR_ASSERT_FATAL(node != regionEntry,
                "region [%p]: entry node %d has temps to initialize along incoming edges", region, node->getNumber());

            // curBlock can't be the first block in the trees. The first
            // block dominates everything (except the synthetic entry block)
            // and so any edge targeting it is a back-edge, which is ignored.
            // Don't change existing edges yet. They'll be fixed afterward.
            TR::Block *prevBlock = curBlock->getPrevBlock();
            TR_ASSERT_FATAL(prevBlock != NULL, "region [%p]: block_%d should have a previous block", region,
                curBlock->getNumber());

            TR::Block *criticalEdgeSplitBlock = NULL;
            _sd->_criticalEdgeSplitPredBlocks.clear();

            const TR::CFGEdgeList &preds = node->getPredecessors();
            for (auto predIt = preds.begin(); predIt != preds.end(); predIt++) {
                TR_StructureSubGraphNode *pred = toStructureSubGraphNode((*predIt)->getFrom());

                BlockInfo &predInfo = at(_sd->_blockInfos, pred->getNumber());
                if (predInfo._ignored) {
                    edgeWantCis.copyFrom(toTransformIncomingCis);
                } else if (!_sd->_initCis.atNode(predInfo).contains(toTransformIncomingCis)) {
                    edgeWantCis.copyFrom(toTransformIncomingCis);
                    edgeWantCis.subtract(_sd->_initCis.atNode(predInfo));
                } else {
                    // All the candidates we want are already initialized when
                    // coming from this predecessor.
                    continue;
                }

                // This edge could represent multiple predecessor blocks if pred is
                // a region structure.
                collectPredBlocks(node, pred);
                for (auto it = _sd->_predBlocks.begin(); it != _sd->_predBlocks.end(); it++) {
                    // This must split incoming edges only when expected by the
                    // prior analysis ("third pass") that computes _initIncomingCis.
                    TR::Block *predBlock = *it;
                    if (predBlock->getSuccessors().size() == 1
                        || at(_sd->_blockInfos, predBlock->getNumber())._ignored) {
                        // We can put the stores in the predecessor.
                        appendTempStores(predBlock, edgeWantCis);
                        continue;
                    }

                    _sd->_criticalEdgeSplitPredBlocks.push_back(predBlock);

                    if (criticalEdgeSplitBlock != NULL) {
                        TR_ASSERT_FATAL(edgeWantCis.equals(criticalEdgeWantCis),
                            "region [%p]: subnode %d: predecessor blocks with critical edges "
                            "were supposed to agree on the set of initializations needed",
                            region, node->getNumber());

                        continue;
                    }

                    // This is the first predecessor block with a critical edge.
                    // Record this choice of candidates and create a new block just
                    // before this one where the temps will be stored. The frequency
                    // can't really be determined based on the frequencies of the
                    // existing blocks, so just guess it's half of curBlock.
                    criticalEdgeWantCis.copyFrom(edgeWantCis);

                    int32_t freq = curBlock->getFrequency() / 2;
                    if (freq <= MAX_COLD_BLOCK_COUNT) {
                        freq = MAX_COLD_BLOCK_COUNT + 1;
                    }

                    criticalEdgeSplitBlock = TR::Block::createEmptyBlock(comp(), freq);
                    comp()->getFlowGraph()->addNode(criticalEdgeSplitBlock, region);

                    dumpOptDetails(comp(), "%sCreate block_%d to store temps before block_%d\n", optDetailString(),
                        criticalEdgeSplitBlock->getNumber(), curBlock->getNumber());

                    prevBlock->getExit()->join(criticalEdgeSplitBlock->getEntry());
                    criticalEdgeSplitBlock->getExit()->join(curBlock->getEntry());
                    comp()->getFlowGraph()->addEdge(criticalEdgeSplitBlock, curBlock);

                    appendTempStores(criticalEdgeSplitBlock, edgeWantCis);
                }
            }

            if (criticalEdgeSplitBlock != NULL) {
                bool prevBlockUsesSplit = false;
                for (auto it = _sd->_criticalEdgeSplitPredBlocks.begin(); it != _sd->_criticalEdgeSplitPredBlocks.end();
                     it++) {
                    TR::Block *predBlock = *it;
                    if (predBlock == prevBlock) {
                        prevBlockUsesSplit = true;
                    }

                    TR::Block::redirectFlowToNewDestination(comp(), predBlock->getEdge(curBlock),
                        criticalEdgeSplitBlock, false);

                    dumpOptDetails(comp(),
                        "%sRedirect control flow: block_%d -> block_%d becomes block_%d -> block_%d\n",
                        optDetailString(), predBlock->getNumber(), curBlock->getNumber(), predBlock->getNumber(),
                        criticalEdgeSplitBlock->getNumber());
                }

                if (!prevBlockUsesSplit) {
                    // prevBlock may have been falling through to curBlock. In that
                    // case, it should still go to curBlock whenever it would have
                    // fallen through, but now the new block is in between. Can't
                    // use redirectFlowToNewDestination() here because prevBlock's
                    // successor edges are already correct.
                    TR::CFGEdge *prevToCur = prevBlock->getEdge(curBlock);
                    if (prevToCur != NULL) {
                        TR::TreeTop *prevLastTree = prevBlock->getLastRealTreeTop();
                        TR::Node *prevLastNode = prevLastTree->getNode();
                        TR::ILOpCode prevLastOp = prevLastNode->getOpCode();
                        if (!prevLastOp.isGoto() && !prevLastOp.isSwitch() && !prevLastOp.isJumpWithMultipleTargets()) {
                            // prevBlock does fall through
                            TR::Block *gotoBlock = prevBlock;
                            if (prevLastOp.isBranch()) {
                                int32_t gotoFreq = prevBlock->getFrequency();
                                if (!prevBlock->isCold()) {
                                    gotoFreq /= 2; // just guess it's 50%
                                    if (gotoFreq <= MAX_COLD_BLOCK_COUNT) {
                                        gotoFreq = MAX_COLD_BLOCK_COUNT + 1;
                                    }
                                }

                                gotoBlock = TR::Block::createEmptyBlock(comp(), gotoFreq);
                                gotoBlock->setIsExtensionOfPreviousBlock();
                                if (prevBlock->isCold()) {
                                    gotoBlock->setIsCold();
                                }

                                prevBlock->getExit()->join(gotoBlock->getEntry());
                                gotoBlock->getExit()->join(criticalEdgeSplitBlock->getEntry());
                                comp()->getFlowGraph()->addNode(gotoBlock, region);
                                comp()->getFlowGraph()->addEdge(prevBlock, gotoBlock);
                                comp()->getFlowGraph()->addEdge(gotoBlock, curBlock);
                                comp()->getFlowGraph()->removeEdge(prevBlock, curBlock);
                            }

                            TR::Node *gotoNode = TR::Node::create(prevLastNode, TR::Goto, 0, curBlock->getEntry());

                            gotoBlock->append(TR::TreeTop::create(comp(), gotoNode));
                        }
                    }
                }
            }
        }

        // Change const ref loads to load temps instead where applicable
        // (in blocks only, not subregions).
        if (node->getStructure()->asBlock() != NULL) {
            bool didUpdateThisBlock = false;
            FlowBitSet::NodeView curInitCis = _sd->_initCis.atNode(blockInfo);
            FlowBitSet::NodeView curInitHereCis = _sd->_initHereCis.atNode(blockInfo);
            for (auto it = blockInfo._constRefLoads.begin(); it != blockInfo._constRefLoads.end(); it++) {
                TR::Node *load = it->_node;
                TR::KnownObjectTable::Index koi = load->getSymbolReference()->getKnownObjectIndex();

                uint32_t mui = at(_sd->_koiToMui, koi);
                if (mui == INVALID_MUI) {
                    continue;
                }

                uint32_t ci = at(_sd->_muiToCi, mui);
                if (ci == INVALID_CI || !curInitCis.isBitSet(ci) || curInitHereCis.isBitSet(ci)) {
                    continue;
                }

                // The temp for koi is initialized at entry to this block, so we can
                // change the symref in-place, effectively updating all occurrences.
                //
                // We don't do this when the temp is initialized in the same block for
                // two reasons:
                // 1. The static load is still needed in order to feed the temp store.
                // 2. There's no point, since later occurrences are commoned anyway.
                //
                // TODO: is _curBlockCiToTempLoad effectively unused?
                //
                BlockNodePair &tempLoad = at(_sd->_curBlockCiToTempLoad, ci);
                if (tempLoad._node == NULL || tempLoad._block != curBlock) {
                    // This updates _curBlockCiToTempLoad.
                    tempLoad._node = TR::Node::createLoad(load, knownObjTemp(koi));
                    tempLoad._block = curBlock;
                }

                changeStaticLoadToTempLoad(load, koi);
                didUpdateThisBlock = true;
            }

            if (didUpdateThisBlock) {
                pruneTransformedConstRefLoads(blockInfo);
            }
        }

        // Done transforming this subnode. We might have created new const ref
        // loads in predecessors. Put those into the BlockInfo now that there is
        // no live BlockInfo reference.
        flushNewConstRefLoads();
    }
}

void TR::ConstRefPrivatization::appendTempStores(TR::Block *predBlock, FlowBitSet::NodeView edgeWantCis)
{
    auto edgeWantCiIter = edgeWantCis.iter();
    uint32_t ci = 0;
    while (edgeWantCiIter.next(&ci)) {
        TR::KnownObjectTable::Index koi = at(_sd->_muiToKoi, at(_sd->_ciToMui, ci));
        appendSingleTempStore(predBlock, koi);
    }
}

void TR::ConstRefPrivatization::appendSingleTempStore(TR::Block *storeBlock, TR::KnownObjectTable::Index koi)
{
    // Look for an existing load earlier in the extended block of storeBlock.
    // Maybe we can avoid creating a new load.
    TR::Node *load = NULL;
    TR::Block *searchBlock = storeBlock;
    while (true) {
        if (searchBlock->getNumber() < _sd->_blockInfos.size()) {
            BlockInfo &searchBlockInfo = at(_sd->_blockInfos, searchBlock->getNumber());
            for (auto it = searchBlockInfo._constRefLoads.begin(); it != searchBlockInfo._constRefLoads.end(); it++) {
                if (it->_node->getSymbolReference()->getKnownObjectIndex() == koi) {
                    load = it->_node;
                    break;
                }
            }
        }

        if (load != NULL || !searchBlock->isExtensionOfPreviousBlock()) {
            break;
        }

        searchBlock = searchBlock->getPrevBlock();
    }

    bool createdNewLoad = false;
    if (load == NULL) {
        load = TR::Node::createLoad(_knot->constSymRef(koi));
        createdNewLoad = true;
    }

    TR::TreeTop *insertPoint = storeBlock->getExit();
    TR::TreeTop *lastTree = storeBlock->getLastRealTreeTop();
    TR::ILOpCode lastTreeOp = lastTree->getNode()->getOpCode();
    if (lastTreeOp.isBranch() || lastTreeOp.isJumpWithMultipleTargets()) {
        insertPoint = lastTree;
    }

    TR::SymbolReference *temp = knownObjTemp(koi);
    TR::Node *store = TR::Node::createStore(load, temp, load);
    TR::TreeTop *storeTree = TR::TreeTop::create(comp(), store);
    insertPoint->insertBefore(storeTree);

    dumpOptDetails(comp(), "%sStore obj%d into temp #%d at end of block_%d, store node n%un [%p]\n", optDetailString(),
        koi, temp->getReferenceNumber(), storeBlock->getNumber(), store->getGlobalIndex(), store);

    // Arrange to include this load in the block info, but don't put it into the
    // block info quite yet. If storeBlock is a new block (i.e. from splitting
    // to store along incoming edges), then we might need to resize _blockInfos
    // to accommodate it, and resizing might reallocate, which would invalidate
    // any references/pointers to the BlockInfo of an existing block. Instead,
    // just take note of it and these can be batch-inserted at a safe time.
    if (createdNewLoad) {
        NewConstRefLoad newLoad = {
            storeBlock, { load, storeTree }
        };
        _sd->_newConstRefLoads.push_back(newLoad);
    }
}

void TR::ConstRefPrivatization::flushNewConstRefLoads()
{
    for (auto it = _sd->_newConstRefLoads.begin(); it != _sd->_newConstRefLoads.end(); it++) {
        int32_t blockNumber = it->_block->getNumber();
        NodeTreePair pair = it->_load;

        if (blockNumber >= _sd->_blockInfos.size()) {
            BlockInfo emptyBlockInfo(*_memRegion);
            _sd->_blockInfos.resize(blockNumber + 1, emptyBlockInfo);
        }

        at(_sd->_blockInfos, blockNumber)._constRefLoads.push_back(pair);
    }

    _sd->_newConstRefLoads.clear();
}

void TR::ConstRefPrivatization::collectPredBlocks(TR_StructureSubGraphNode *node, TR_StructureSubGraphNode *pred)
{
    _sd->_predBlocks.clear();
    collectPredBlocksRec(node, pred);
}

void TR::ConstRefPrivatization::collectPredBlocksRec(TR_StructureSubGraphNode *node, TR_StructureSubGraphNode *pred)
{
    TR_BlockStructure *predBlockStructure = pred->getStructure()->asBlock();
    if (predBlockStructure != NULL) {
        _sd->_predBlocks.push_back(predBlockStructure->getBlock());
        return;
    }

    TR_RegionStructure *predRegion = pred->getStructure()->asRegion();
    ListIterator<TR::CFGEdge> exitIt(&predRegion->getExitEdges());
    for (TR::CFGEdge *edge = exitIt.getFirst(); edge != NULL; edge = exitIt.getNext()) {
        if (edge->getTo()->getNumber() == node->getNumber()) {
            collectPredBlocksRec(node, toStructureSubGraphNode(edge->getFrom()));
        }
    }
}

void TR::ConstRefPrivatization::hoistConstRefLoads(TR_RegionStructure *region)
{
    bool trace = this->trace();
    OMR::Logger *log = comp()->log();

    auto &loopLoadFrequencyByKoi = _sd->_loopLoadFrequencyByKoi;
    auto &koiIsHoisted = _sd->_koiIsHoisted;

    TR::Block *header = region->getEntryBlock();
    TR::Block *preheader = NULL;
    auto &headerPreds = header->getPredecessors();
    for (auto it = headerPreds.begin(); it != headerPreds.end(); it++) {
        TR::Block *pred = toBlock((*it)->getFrom());
        if (region->contains(pred->getStructureOf())) {
            continue;
        }

        if (preheader != NULL) {
            preheader = NULL; // multiple predecessors outside the loop
            break;
        }

        if (pred->getSuccessors().size() != 1) {
            break; // branch in predecessor outside the loop
        }

        preheader = pred;
    }

    if (preheader == NULL || preheader == comp()->getFlowGraph()->getStart()) {
        logprintf(trace, log, "\nLoop %d: no preheader found.\n", region->getNumber());
        return;
    }

    const float iterThreshold = 4.0f;
    float avgIters = (float)header->getFrequency() / (float)preheader->getFrequency();
    if (avgIters < iterThreshold) {
        logprintf(trace, log, "\nLoop %d: average iteration count %.1f below is threshold %.1f.\n", region->getNumber(),
            avgIters, iterThreshold);
        return;
    }

    logprintf(trace, log,
        "\nLoop %d: average iteration count %.1f meets threshold %.1f.\n"
        "Look for constant references to hoist.\n",
        region->getNumber(), avgIters, iterThreshold);

    // Don't use the "regionNodes" vector. The acyclic analysis may have created
    // new blocks and added new subnodes to the body. The order doesn't matter
    // here anyway.
    loopLoadFrequencyByKoi.resize(_knotSize, 0);
    for (auto regionIt = region->begin(); regionIt != region->end(); regionIt++) {
        TR_StructureSubGraphNode *node = *regionIt;
        TR_BlockStructure *curBlockStructure = node->getStructure()->asBlock();
        if (curBlockStructure == NULL) {
            continue; // ignore subregions
        }

        TR::Block *curBlock = curBlockStructure->getBlock();
        if (curBlock->getNumber() >= _sd->_blockInfos.size()) {
            continue; // newly created block with no const ref loads, e.g. goto block.
        }

        BlockInfo &bi = at(_sd->_blockInfos, curBlock->getNumber());
        if (bi._ignored) {
            continue; // don't care about these loads
        }

        for (auto it = bi._constRefLoads.begin(); it != bi._constRefLoads.end(); it++) {
            TR::KnownObjectTable::Index koi = it->_node->getSymbolReference()->getKnownObjectIndex();
            if (!at(_sd->_koiIsUsed, koi)) {
                at(_sd->_koiIsUsed, koi) = true;
                _sd->_usedKois.push_back(koi);
                logprintf(trace, log, "block_%d: obj%d is used\n", curBlock->getNumber(), koi);
            }

            at(loopLoadFrequencyByKoi, koi) += curBlock->getFrequency();
        }
    }

    const float shouldHoistRelFreqThreshold = 0.95;
    bool didHoistSomething = false;
    koiIsHoisted.resize(_knotSize, 0);
    for (auto it = _sd->_usedKois.begin(); it != _sd->_usedKois.end(); it++) {
        TR::KnownObjectTable::Index koi = *it;
        float relFreq = (float)at(loopLoadFrequencyByKoi, koi) / (float)header->getFrequency();
        logprintf(trace, log, "obj%d is loaded on average %.2f times per iteration\n", koi, relFreq);
        if (relFreq >= shouldHoistRelFreqThreshold
            && performTransformation(comp(), "%sHoist obj%d out of loop %d\n", optDetailString(), koi,
                region->getNumber())) {
            appendSingleTempStore(preheader, koi);
            at(koiIsHoisted, koi) = true;
            didHoistSomething = true;
        }
    }

    // Replace loads of hoisted refs within the loop
    if (didHoistSomething) {
        flushNewConstRefLoads();

        // Avoid regionNodes. See prior comment.
        for (auto regionIt = region->begin(); regionIt != region->end(); regionIt++) {
            TR_StructureSubGraphNode *node = *regionIt;
            TR_BlockStructure *curBlockStructure = node->getStructure()->asBlock();
            if (curBlockStructure == NULL) {
                continue; // ignore subregions
            }

            TR::Block *curBlock = curBlockStructure->getBlock();
            if (curBlock->getNumber() >= _sd->_blockInfos.size()) {
                continue; // newly created block with no const ref loads, e.g. goto block.
            }

            BlockInfo &bi = at(_sd->_blockInfos, curBlock->getNumber());
            if (bi._ignored) {
                continue; // don't care about these loads
            }

            bool didUpdateThisBlock = false;
            for (auto it = bi._constRefLoads.begin(); it != bi._constRefLoads.end(); it++) {
                TR::Node *load = it->_node;
                TR::KnownObjectTable::Index koi = load->getSymbolReference()->getKnownObjectIndex();

                if (at(koiIsHoisted, koi)) {
                    changeStaticLoadToTempLoad(load, koi);
                    didUpdateThisBlock = true;
                }
            }

            if (didUpdateThisBlock) {
                pruneTransformedConstRefLoads(bi);
            }
        }
    }

    // Reset _koiIsUsed, _loopLoadFrequencyByKoi, _koiIsHoisted, _usedKois for later.
    for (auto it = _sd->_usedKois.begin(); it != _sd->_usedKois.end(); it++) {
        TR::KnownObjectTable::Index koi = *it;
        at(_sd->_koiIsUsed, koi) = false;
        at(loopLoadFrequencyByKoi, koi) = 0;
        at(koiIsHoisted, koi) = false;
    }

    _sd->_usedKois.clear();
}

TR::SymbolReference *TR::ConstRefPrivatization::knownObjTemp(TR::KnownObjectTable::Index koi)
{
    TR::SymbolReferenceTable *symRefTab = comp()->getSymRefTab();
    TR::ResolvedMethodSymbol *methodSym = comp()->getMethodSymbol();
    return symRefTab->findOrCreateTemporaryWithKnowObjectIndex(methodSym, koi);
}

void TR::ConstRefPrivatization::changeStaticLoadToTempLoad(TR::Node *load, TR::KnownObjectTable::Index koi)
{
    TR_ASSERT_FATAL_WITH_NODE(load, load->getOpCodeValue() == TR::aload, "expected aload");

    TR::SymbolReference *origSymRef = load->getSymbolReference();
    TR_ASSERT_FATAL_WITH_NODE(load, origSymRef == _knot->constSymRef(koi), "expected a const ref load of obj%d", koi);

    // Actually loading the temp is always optional.
    TR::SymbolReference *temp = knownObjTemp(koi);
    if (performTransformation(comp(), "%sChange static load n%un [%p] #%d (obj%d) into load of temp #%d\n",
            optDetailString(), load->getGlobalIndex(), load, origSymRef->getReferenceNumber(), koi,
            temp->getReferenceNumber())) {
        load->setSymbolReference(temp);
    }
}

void TR::ConstRefPrivatization::pruneTransformedConstRefLoads(BlockInfo &blockInfo)
{
    auto &loads = blockInfo._constRefLoads;
    size_t n = loads.size();
    size_t kept = 0;
    for (size_t i = 0; i < n; i++) {
        NodeTreePair entry = at(loads, i);
        TR::Node *load = entry._node;
        TR_ASSERT_FATAL_WITH_NODE(load, load->getOpCodeValue() == TR::aload, "expected aload");

        TR::SymbolReference *symRef = load->getSymbolReference();
        TR::KnownObjectTable::Index koi = symRef->getKnownObjectIndex();
        TR_ASSERT_FATAL_WITH_NODE(load, koi != TR::KnownObjectTable::UNKNOWN, "expected a known object");

        if (symRef->getSymbol()->isStatic()) {
            at(loads, kept++) = entry; // keep it
        }
    }

    loads.resize(kept); // truncation
}

const char *TR::ConstRefPrivatization::regionKindStr(TR_RegionStructure *region)
{
    if (region->isAcyclic()) {
        return "acyclic region";
    } else if (region->isNaturalLoop()) {
        return "loop";
    } else {
        return "improper region";
    }
}

void TR::ConstRefPrivatization::traceBitSet(FlowBitSet::NodeView set)
{
    OMR::Logger *log = comp()->log();
    bool valuePrinted = false;
    uint32_t value = 0;
    auto it = set.iter();
    while (it.next(&value)) {
        log->printf(" %u", value);
        valuePrinted = true;
    }

    if (!valuePrinted) {
        log->prints(" (none)");
    }
}

void TR::ConstRefPrivatization::traceCiLoadPs(FlowVector<float>::NodeView xs)
{
    OMR::Logger *log = comp()->log();
    bool valuePrinted = false;
    for (uint32_t ci = 0; ci < xs.len(); ci++) {
        float x = xs[ci];
        if (x != 0.0f) {
            log->printf(" ci%u:%.2f", ci, x);
            valuePrinted = true;
        }
    }

    if (!valuePrinted) {
        log->prints(" (all zero)");
    }
}

const char *TR::ConstRefPrivatization::optDetailString() const throw() { return "O^O CONST REF PRIVATIZATION: "; }

TR::ConstRefPrivatization::BlockInfo::BlockInfo(TR::Region &region)
    : _constRefLoads(region)
    , _flowDataIndex(INVALID_FDI)
    , _seen(false)
    , _ignored(false)
{
    // empty
}

