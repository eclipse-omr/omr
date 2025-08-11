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

#include "optimizer/GlobalRegister.hpp"
#include "optimizer/GlobalRegister_inlines.hpp"

#include "compile/Compilation.hpp"
#include "optimizer/GlobalRegisterAllocator.hpp"
#include "optimizer/Optimizations.hpp"
#include "optimizer/Optimizer.hpp"
#include "optimizer/RegisterCandidate.hpp"
#include "il/Block.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "infra/Cfg.hpp"

#define keepAllStores false
#define OPT_DETAILS "O^O GLOBAL REGISTER ASSIGNER: "

bool OMR::GlobalRegister::getAutoContainsRegisterValue()
{
    return _autoContainsRegisterValue;
    // &&
    //      (!getValue()->getOpCode().isFloatingPoint() ||
    //       comp()->cg()->getSupportsJavaFloatSemantics()));
}

void OMR::GlobalRegister::setCurrentRegisterCandidate(TR::RegisterCandidate *rc, vcount_t visitCount,
    TR::Block *currentBlock, int32_t i, TR::Compilation *comp, bool resetOtherHalfOfLong)
{
    if (_rcCurrent != rc) {
        bool liveOnSomeSucc = true;
        if (_rcCurrent && getValue() && !getAutoContainsRegisterValue()) {
            if (liveOnSomeSucc)
                createStoreFromRegister(visitCount, optimalPlacementForStore(currentBlock, comp), i, comp);
        }
        if (_rcCurrent)
            _rcCurrent->getSymbolReference()->getSymbol()->setIsInGlobalRegister(false);

        if (resetOtherHalfOfLong && _rcCurrent != NULL && _rcCurrent->rcNeeds2Regs(comp)) {
            //  TR_ASSERT(currentBlock->startOfExtendedBlock() ==
            //  _candidates->getStartOfExtendedBBForBB()[currentBlock->getNumber()], "Incorrect value in
            //  _startOfExtendedBBForBB");
            TR::Block *startOfExtendedBlock = currentBlock->startOfExtendedBlock();
            TR_Array<TR::GlobalRegister> &extRegisters = startOfExtendedBlock->getGlobalRegisters(comp);

            int32_t highRegNum = _rcCurrent->getHighGlobalRegisterNumber();
            if (i == highRegNum) {
                int32_t lowRegNum = _rcCurrent->getLowGlobalRegisterNumber();
                TR::GlobalRegister *grLow = &extRegisters[lowRegNum];
                grLow->setCurrentRegisterCandidate(NULL, visitCount, currentBlock, lowRegNum, comp, false);
                // grLow.copyCurrentRegisterCandidate(gr);
            } else {
                TR::GlobalRegister *grHigh = &extRegisters[highRegNum];
                grHigh->setCurrentRegisterCandidate(NULL, visitCount, currentBlock, highRegNum, comp, false);
                // grHigh.copyCurrentRegisterCandidate(gr);
            }
        }

        _rcCurrent = rc;
        setValue(0); // regardless of whether or not we created a store we must setValue to NULL, else we can use the
                     // wrong value

        if (currentBlock)
            comp->setCurrentBlock(currentBlock);
        if (rc && (rc->getSymbol()->dontEliminateStores(comp) || keepAllStores || rc->isLiveAcrossExceptionEdge()))
            setAutoContainsRegisterValue(true);
        else
            setAutoContainsRegisterValue(false);
    }
}

void OMR::GlobalRegister::copyCurrentRegisterCandidate(TR::GlobalRegister *gr)
{
    _rcCurrent = gr->getCurrentRegisterCandidate();
    //_rcOnExit = gr->getRegisterCandidateOnExit();
    //_rcOnEntry = gr->getRegisterCandidateOnEntry();
    setValue(gr->getValue());
    // setLastRefTreeTop(gr->getLastRefTreeTop());
    //_mappings = gr->getMappings();
    setAutoContainsRegisterValue(gr->getAutoContainsRegisterValue());
}

TR::TreeTop *OMR::GlobalRegister::optimalPlacementForStore(TR::Block *currentBlock, TR::Compilation *comp)
{
    bool traceGRA = comp->getOptions()->trace(OMR::tacticalGlobalRegisterAllocator);
    if (traceGRA)
        traceMsg(comp, "           optimalPlacementForStore([%p], block_%d)\n", getValue(), currentBlock->getNumber());

    TR::TreeTop *lastRefTreeTop = getLastRefTreeTop();

    TR::Block *lastRefBlock = lastRefTreeTop->getEnclosingBlock();

    // if lastRefTreeTop is the last tree in the previous block,
    // the store should be in the current block
    if (lastRefBlock != currentBlock) {
        TR::Node *lastRefNode = lastRefTreeTop->getNode();
        if (lastRefNode->getOpCodeValue() == TR::treetop)
            lastRefNode = lastRefNode->getFirstChild();

        TR::ILOpCode &prevOpCode = lastRefTreeTop->getNode()->getOpCode();
        if (prevOpCode.isBranch() || prevOpCode.isJumpWithMultipleTargets() || prevOpCode.isReturn()
            || prevOpCode.getOpCodeValue() == TR::athrow || prevOpCode.getOpCodeValue() == TR::BBEnd) {
            lastRefTreeTop = lastRefTreeTop->getNextTreeTop();
        }

        if (lastRefTreeTop->getNode()->getOpCodeValue() == TR::BBEnd)
            lastRefTreeTop = lastRefTreeTop->getNextTreeTop();
    }

    if (lastRefBlock == currentBlock) {
        if (traceGRA)
            traceMsg(comp, "           - lastRefBlock == currentBlock: returning [%p]\n", lastRefTreeTop->getNode());
        return lastRefTreeTop;
    }

    int32_t lastRefFreq = 1, currentFreq = 1;
    if (!lastRefBlock->getStructureOf() || !currentBlock->getStructureOf()) {
        if (traceGRA)
            traceMsg(comp, "           - Structure info missing: returning [%p]\n", lastRefTreeTop->getNode());
        return lastRefTreeTop;
    }

    TR::Optimizer *optimizer = (TR::Optimizer *)comp->getOptimizer();

    optimizer->getStaticFrequency(lastRefBlock, &lastRefFreq);
    optimizer->getStaticFrequency(currentBlock, &currentFreq);

    if (lastRefFreq <= currentFreq) // used to be ==
    {
        if (traceGRA)
            traceMsg(comp, "           - Frequency is low enough: returning [%p]\n", lastRefTreeTop->getNode());
        return lastRefTreeTop;
    }

    for (TR::Block *b = lastRefBlock->getNextBlock(); b; b = b->getNextBlock()) {
        if (b != currentBlock) {
            int32_t freq = 1;
            optimizer->getStaticFrequency(b, &freq);
            if (freq > currentFreq)
                continue;
        }
        if (traceGRA)
            traceMsg(comp, "           - Found a suitable block: returning [%p]\n", b->getEntry()->getNode());
        return b->getEntry();
    }

    TR_ASSERT(0, "optimalPlacementForStore didn't find optimal placement");
    return 0;
}

TR::Node *OMR::GlobalRegister::createLoadFromRegister(TR::Node *n, TR::Compilation *comp)
{
    TR::RegisterCandidate *rc = getCurrentRegisterCandidate();
    TR::DataType dt = rc->getDataType();
    if (dt == TR::Aggregate) {
        switch (rc->getSymbol()->getSize()) {
            case 1:
                dt = TR::Int8;
                break;
            case 2:
                dt = TR::Int16;
                break;
            case 4:
                dt = TR::Int32;
                break;
            case 8:
                dt = TR::Int64;
                break;
        }
    }

    TR::Node *load;
    load = TR::Node::create(n, comp->il.opCodeForRegisterLoad(dt));
    load->setRegLoadStoreSymbolReference(rc->getSymbolReference());

    if (load->requiresRegisterPair(comp)) {
        load->setLowGlobalRegisterNumber(rc->getLowGlobalRegisterNumber());
        load->setHighGlobalRegisterNumber(rc->getHighGlobalRegisterNumber());
    } else
        load->setGlobalRegisterNumber(rc->getGlobalRegisterNumber());
    if (!rc->is8BitGlobalGPR())
        load->setIsInvalid8BitGlobalRegister(true);
    setValue(load);
    if (load->requiresRegisterPair(comp))
        dumpOptDetails(comp, "%s create load [%p] from Register %d (low word) and Register %d (high word)\n",
            OPT_DETAILS, load, rc->getLowGlobalRegisterNumber(), rc->getHighGlobalRegisterNumber());
    else
        dumpOptDetails(comp, "%s create load [%p] %s from Register %d\n", OPT_DETAILS, load,
            rc->getSymbolReference()->getSymbol()->isMethodMetaData()
                ? rc->getSymbolReference()->getSymbol()->castToMethodMetaDataSymbol()->getName()
                : "",
            rc->getGlobalRegisterNumber());
    return load;
}

TR::Node *OMR::GlobalRegister::createStoreToRegister(TR::TreeTop *prevTreeTop, TR::Node *node, vcount_t visitCount,
    TR::Compilation *comp, TR_GlobalRegisterAllocator *gra)
{
    TR::Node *n = prevTreeTop->getNode();
    TR::RegisterCandidate *rc = getCurrentRegisterCandidate();
    TR::Node *load = NULL;

    TR::DataType dt = rc->getDataType();
    if (dt == TR::Aggregate) {
        switch (rc->getSymbol()->getSize()) {
            case 1:
                dt = TR::Int8;
                break;
            case 2:
                dt = TR::Int16;
                break;
            case 4:
                dt = TR::Int32;
                break;
            case 8:
                dt = TR::Int64;
                break;
            default:
                TR_ASSERT(false, "unsupported size %d for aggregate in GRA\n", rc->getSymbol()->getSize());
                break;
        }
    }

    if (node) {
        load = node;
    } else {
        load = TR::Node::createWithSymRef(n, comp->il.opCodeForDirectLoad(dt), 0, rc->getSymbolReference());
    }

    load = gra->resolveTypeMismatch(dt, load);

    TR::Node *store;
    store = TR::Node::create(comp->il.opCodeForRegisterStore(dt), 1, load);
    store->setRegLoadStoreSymbolReference(rc->getSymbolReference());

    bool enableSignExtGRA = false; // enable for other platforms later
    static char *doit = feGetEnv("TR_SIGNEXTGRA");
    if (NULL != doit)
        enableSignExtGRA = true;

    if (comp->target().cpu.isZ()) {
        enableSignExtGRA = true;
        static char *doit2 = feGetEnv("TR_NSIGNEXTGRA");
        if (NULL != doit2)
            enableSignExtGRA = false;
    }

    if (comp->target().is64Bit() && (store->getOpCodeValue() == TR::iRegStore)
        && gra->candidateCouldNeedSignExtension(rc->getSymbolReference()->getReferenceNumber()) && enableSignExtGRA) {
        store->setNeedsSignExtension(true);
    }

    if (store->requiresRegisterPair(comp)) {
        store->setLowGlobalRegisterNumber(rc->getLowGlobalRegisterNumber());
        store->setHighGlobalRegisterNumber(rc->getHighGlobalRegisterNumber());
    } else
        store->setGlobalRegisterNumber(rc->getGlobalRegisterNumber());

    if (store->needsSignExtension())
        gra->setSignExtensionRequired(true, rc->getGlobalRegisterNumber());
    else
        gra->setSignExtensionNotRequired(true, rc->getGlobalRegisterNumber());

    TR::TreeTop *tt = TR::TreeTop::create(comp, prevTreeTop, store);
    load->setVisitCount(visitCount);
    TR::Symbol *sym = rc->getSymbolReference()->getSymbol();
    if (!rc->is8BitGlobalGPR())
        load->setIsInvalid8BitGlobalRegister(true);
    setValue(load);
    setAutoContainsRegisterValue(true);

    if (store->requiresRegisterPair(comp))
        dumpOptDetails(comp,
            "%s create store [%p] of symRef#%d to Register %d (low word) and Register %d (high word)\n", OPT_DETAILS,
            store, rc->getSymbolReference()->getReferenceNumber(), rc->getLowGlobalRegisterNumber(),
            rc->getHighGlobalRegisterNumber());
    else
        dumpOptDetails(comp, "%s create store [%p] of %s symRef#%d to Register %d\n", OPT_DETAILS, store,
            rc->getSymbolReference()->getSymbol()->isMethodMetaData()
                ? rc->getSymbolReference()->getSymbol()->castToMethodMetaDataSymbol()->getName()
                : "",
            rc->getSymbolReference()->getReferenceNumber(), rc->getGlobalRegisterNumber());

    return load;
}

TR::Node *OMR::GlobalRegister::createStoreFromRegister(vcount_t visitCount, TR::TreeTop *prevTreeTop, int32_t i,
    TR::Compilation *comp, bool storeUnconditionally)
{
    if (!storeUnconditionally) {
        TR_ASSERT(!getAutoContainsRegisterValue(), "don't need to store the register back into the auto");
        TR_ASSERT(getLastRefTreeTop(), "expected getLastRefTreeTop to be nonzero");
    } else
        TR_ASSERT(prevTreeTop, "expected prevTreeTop to be nonzero");

    if (!prevTreeTop)
        prevTreeTop = getLastRefTreeTop();

    TR::Node *prevNode = prevTreeTop->getNode();
    if ((prevNode->getOpCodeValue() == TR::NULLCHK) || (prevNode->getOpCodeValue() == TR::treetop))
        prevNode = prevNode->getFirstChild();

    TR::ILOpCode &prevOpCode = prevNode->getOpCode();
    if (prevOpCode.isBranch() || prevOpCode.isJumpWithMultipleTargets() || prevOpCode.isReturn()
        || prevOpCode.getOpCodeValue() == TR::athrow || prevOpCode.getOpCodeValue() == TR::BBEnd)
        prevTreeTop = prevTreeTop->getPrevTreeTop();

    TR::RegisterCandidate *rc = getCurrentRegisterCandidate();
    TR::Node *node = getValue();
    TR::Node *store = TR::Node::createWithSymRef(comp->il.opCodeForDirectStore(rc->getDataType()), 1, 1, node,
        rc->getSymbolReference());
    store->setVisitCount(visitCount);
    rc->addStore(TR::TreeTop::create(comp, prevTreeTop, store));
    setAutoContainsRegisterValue(true);
    rc->setExtendedLiveRange(true);

    if (i != -1) {
        if (store->requiresRegisterPair(comp))
            dumpOptDetails(comp, "%s create store [%p] from Register %d (low word) and Register %d (high word)\n",
                OPT_DETAILS, store, rc->getLowGlobalRegisterNumber(), rc->getHighGlobalRegisterNumber());
        else
            dumpOptDetails(comp, "%s create store [%p] from Register %d for %s #%d\n", OPT_DETAILS, store,
                rc->getGlobalRegisterNumber(),
                rc->getSymbolReference()->getSymbol()->isMethodMetaData()
                    ? rc->getSymbolReference()->getSymbol()->castToMethodMetaDataSymbol()->getName()
                    : "",
                rc->getSymbolReference()->getReferenceNumber());
    }

    return store;
}
