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

#include "codegen/OutOfLineCodeSection.hpp"

#include <stddef.h>
#include <stdint.h>
#include "codegen/BackingStore.hpp"
#include "codegen/CodeGenerator.hpp"
#include "codegen/Instruction.hpp"
#include "codegen/RegisterDependency.hpp"
#include "codegen/TreeEvaluator.hpp"
#include "compile/Compilation.hpp"
#include "il/ILOps.hpp"
#include "il/LabelSymbol.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "il/Symbol.hpp"
#include "il/SymbolReference.hpp"
#include "infra/Assert.hpp"

TR_OutOfLineCodeSection::TR_OutOfLineCodeSection(TR::Node *callNode, TR::ILOpCodes callOp, TR::Register *targetReg,
    TR::LabelSymbol *entryLabel, TR::LabelSymbol *restartLabel, TR::CodeGenerator *cg)
    : _restartLabel(restartLabel)
    , _firstInstruction(NULL)
    , _appendInstruction(NULL)
    , _targetReg(targetReg)
    , _entryLabel(entryLabel)
    , _hasBeenRegisterAssigned(false)
    , _cg(cg)
{
    _entryLabel->setStartOfColdInstructionStream();
    _restartLabel->setEndOfColdInstructionStream();
    _block = callNode->getSymbolReference()->canCauseGC() ? cg->getCurrentEvaluationBlock() : 0;

    _callNode = createOutOfLineCallNode(callNode, callOp);
}

TR_OutOfLineCodeSection::TR_OutOfLineCodeSection(TR::LabelSymbol *entryLabel, TR::CodeGenerator *cg)
    : _entryLabel(entryLabel)
    , _cg(cg)
    , _firstInstruction(NULL)
    , _appendInstruction(NULL)
    , _restartLabel(NULL)
    , _block(NULL)
    , _callNode(NULL)
    , _targetReg(NULL)
    , _hasBeenRegisterAssigned(false)
{
    _entryLabel->setStartOfColdInstructionStream();
}

TR_OutOfLineCodeSection::TR_OutOfLineCodeSection(TR::LabelSymbol *entryLabel, TR::LabelSymbol *restartLabel,
    TR::CodeGenerator *cg)
    : _entryLabel(entryLabel)
    , _cg(cg)
    , _firstInstruction(NULL)
    , _appendInstruction(NULL)
    , _restartLabel(restartLabel)
    , _block(NULL)
    , _callNode(NULL)
    , _targetReg(NULL)
    , _hasBeenRegisterAssigned(false)
{
    _entryLabel->setStartOfColdInstructionStream();
    _restartLabel->setEndOfColdInstructionStream();
    _block = cg->getCurrentEvaluationBlock();
}

void TR_OutOfLineCodeSection::swapInstructionListsWithCompilation()
{
    TR::Instruction *temp;

    temp = _cg->getFirstInstruction();
    _cg->setFirstInstruction(_firstInstruction);
    _firstInstruction = temp;

    temp = _cg->getAppendInstruction();
    _cg->setAppendInstruction(_appendInstruction);
    _appendInstruction = temp;
    _cg->toggleIsInOOLSection();
}

// Evaluate every subtree S of a node N that meets the following criteria:
//
//    (1) the first reference to S is in a subtree of N, and
//    (2) not all references to S appear under N
//
// All subtrees will be evaluated as soon as they are discovered.
//
void TR_OutOfLineCodeSection::preEvaluatePersistentHelperArguments()
{
    TR_ASSERT(_callNode,
        "preEvaluatePersistentHelperArguments can only be called for TR_OutOfLineCodeSection which are helper calls");
    TR::TreeEvaluator::initializeStrictlyFutureUseCounts(_callNode, _cg->comp()->incVisitCount(), _cg);
    evaluateNodesWithFutureUses(_callNode);
}

void TR_OutOfLineCodeSection::evaluateNodesWithFutureUses(TR::Node *node)
{
    if (node->getRegister() != NULL) {
        // Node has already been evaluated outside this tree.
        //
        return;
    }

    if (node->getFutureUseCount() > 0) {
        (void)_cg->evaluate(node);

        // Do not decrement the reference count here.  It will be decremented when the call node is evaluated
        // again in the helper instruction stream.
        //

        return;
    }

    for (int32_t i = 0; i < node->getNumChildren(); i++)
        evaluateNodesWithFutureUses(node->getChild(i));
}

TR::Node *TR_OutOfLineCodeSection::createOutOfLineCallNode(TR::Node *callNode, TR::ILOpCodes callOp)
{
    int32_t i;
    vcount_t visitCount = _cg->comp()->incVisitCount();
    TR::Node *child;

    for (i = 0; i < callNode->getNumChildren(); i++) {
        child = callNode->getChild(i);
        TR::TreeEvaluator::initializeStrictlyFutureUseCounts(child, visitCount, _cg);
    }

    TR::Node *newCallNode
        = TR::Node::createWithSymRef(callNode, callOp, callNode->getNumChildren(), callNode->getSymbolReference());
    newCallNode->setReferenceCount(1);

    for (i = 0; i < callNode->getNumChildren(); i++) {
        child = callNode->getChild(i);

        if (child->getRegister() != NULL) {
            // Child has already been evaluated outside this tree.
            //
            newCallNode->setAndIncChild(i, child);
        } else if (child->getOpCode().isLoadConst()) {
            // Copy unevaluated constant nodes.
            //
            child = TR::Node::copy(child);
            child->setReferenceCount(1);
            newCallNode->setChild(i, child);
        } else {
            if ((child->getOpCodeValue() == TR::loadaddr)
                && (callNode->getOpCodeValue() == TR:: instanceof || callNode->getOpCodeValue() == TR::checkcast
                        || callNode->getOpCodeValue() == TR::checkcastAndNULLCHK)
                && (child->getSymbolReference()->getSymbol())
                && (child->getSymbolReference()->getSymbol()->getStaticSymbol())) {
                child = TR::Node::copy(child);
                child->setReferenceCount(1);
                newCallNode->setChild(i, child);
            } else {
                // Be very conservative at this point, even though it is possible to make it less so.  For example, this
                // will catch the case of an unevaluated argument not persisting outside of the outlined region even
                // though one of its subtrees will.
                //
                (void)_cg->evaluate(child);

                // Do not decrement the reference count here.  It will be decremented when the call node is evaluated
                // again in the helper instruction stream.
                //
                newCallNode->setAndIncChild(i, child);
            }
        }
    }

    return newCallNode;
}

void TR_OutOfLineCodeSection::assignRegisters(TR_RegisterKinds kindsToBeAssigned)
{
    TR::Compilation *comp = _cg->comp();
    if (hasBeenRegisterAssigned())
        return;
    // nested internal control flow assert:
    _cg->setInternalControlFlowSafeNestingDepth(_cg->internalControlFlowNestingDepth());

    // Create a dependency list on the first instruction in this stream that captures all current real register
    // associations. This is necessary to get the register assigner back into its original state before the helper
    // stream was processed.
    //
    _cg->incOutOfLineColdPathNestedDepth();

    // this is to prevent the OOL entry label from resetting all register's startOfrange during RA
    _cg->toggleIsInOOLSection();
    TR::RegisterDependencyConditions *liveRealRegDeps
        = _cg->machine()->createCondForLiveAndSpilledGPRs(_cg->getSpilledRegisterList());

    if (liveRealRegDeps) {
        _firstInstruction->setDependencyConditions(liveRealRegDeps);
#if defined(TR_TARGET_ARM64)
        liveRealRegDeps->bookKeepingRegisterUses(_firstInstruction, _cg);
#endif
    }
    _cg->toggleIsInOOLSection(); // toggle it back because there is another toggle inside
                                 // swapInstructionListsWithCompilation()

    // Register assign the helper dispatch instructions.
    swapInstructionListsWithCompilation();
    _cg->doRegisterAssignment(kindsToBeAssigned);

    TR::list<TR::Register *> *firstTimeLiveOOLRegisterList = _cg->getFirstTimeLiveOOLRegisterList();
    TR::list<TR::Register *> *spilledRegisterList = _cg->getSpilledRegisterList();

    for (auto li = firstTimeLiveOOLRegisterList->begin(); li != firstTimeLiveOOLRegisterList->end(); ++li) {
        if ((*li)->getBackingStorage()) {
            (*li)->getBackingStorage()->setMaxSpillDepth(1);
            traceMsg(comp, "Adding virtReg:%s from _firstTimeLiveOOLRegisterList to _spilledRegisterList \n",
                _cg->getDebug()->getName((*li)));
            spilledRegisterList->push_front((*li));
        }
    }
    _cg->getFirstTimeLiveOOLRegisterList()->clear();

    swapInstructionListsWithCompilation();

#if defined(TR_TARGET_ARM)
    // If a real reg is assigned at the end of the hot path and free at the beginning, but unused on the cold path
    // it won't show up in the liveRealRegDeps. If a dead virtual occupies that real reg we need to clean it up.
    for (int32_t i = TR::RealRegister::FirstGPR; i <= TR::RealRegister::LastFPR; ++i) {
        TR::RealRegister *r = _cg->machine()->getRealRegister((TR::RealRegister::RegNum)i);
        TR::Register *v = r->getAssignedRegister();

        if (v && v != r && v->getFutureUseCount() == 0) {
            v->setAssignedRegister(NULL);
            r->setState(TR::RealRegister::Unlatched);
        }
    }
#endif

    _cg->decOutOfLineColdPathNestedDepth();

    // Returning to mainline, reset this counter
    _cg->setInternalControlFlowSafeNestingDepth(0);

    // Link in the helper stream into the mainline code.
    // We will end up with the OOL items attached at the bottom of the instruction stream
    TR::Instruction *appendInstruction = _cg->getAppendInstruction();
    appendInstruction->setNext(_firstInstruction);
    _firstInstruction->setPrev(appendInstruction);
    _cg->setAppendInstruction(_appendInstruction);

    setHasBeenRegisterAssigned(true);
}
