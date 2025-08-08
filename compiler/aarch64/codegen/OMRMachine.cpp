/*******************************************************************************
 * Copyright IBM Corp. and others 2018
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

#include <stddef.h>
#include <stdint.h>

#include "codegen/ARM64Instruction.hpp"
#include "codegen/BackingStore.hpp"
#include "codegen/CodeGenerator.hpp"
#include "codegen/GenerateInstructions.hpp"
#include "codegen/Linkage.hpp"
#include "codegen/Machine.hpp"
#include "codegen/Machine_inlines.hpp"
#include "codegen/RealRegister.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "infra/Assert.hpp"

// Register Association
#define ARM64_REGISTER_HEAVIEST_WEIGHT 0x0000ffff
#define ARM64_REGISTER_INITIAL_PRESERVED_WEIGHT 0x00001000
#define ARM64_REGISTER_ASSOCIATED_WEIGHT 0x00000800
#define ARM64_REGISTER_PLACEHOLDER_WEIGHT 0x00000100
#define ARM64_REGISTER_BASIC_WEIGHT 0x00000080

OMR::ARM64::Machine::Machine(TR::CodeGenerator *cg)
    : OMR::Machine(cg)
{
    self()->initializeRegisterFile();
    self()->clearRegisterAssociations();
}

/**
 * @brief Determines if the passed real register is specified
 *        as the dependency to the passed virtual register
 *        at the nearest register dependency before the current instruction.
 *
 * @param currentInstruction : current instruction
 * @param realNum            : real register number
 * @param virtReg            : virtual register
 * @return true if the real register is specified as the dependency to the virtual register
 */
static bool boundNext(TR::Instruction *currentInstruction, int32_t realNum, TR::Register *virtReg)
{
    TR::Instruction *cursor = currentInstruction;
    TR::RealRegister::RegNum realReg = static_cast<TR::RealRegister::RegNum>(realNum);
    TR::Node *nodeBBStart = NULL;

    while (cursor->getOpCodeValue() != TR::InstOpCode::proc) {
        TR::RegisterDependencyConditions *conditions;
        if ((conditions = cursor->getDependencyConditions()) != NULL) {
            TR::Register *boundReg = conditions->searchPostConditionRegister(realReg);
            if (boundReg == NULL) {
                boundReg = conditions->searchPreConditionRegister(realReg);
            }
            if (boundReg != NULL) {
                return (boundReg == virtReg);
            }
        }

        TR::Node *node = cursor->getNode();
        if (nodeBBStart != NULL && node != nodeBBStart) {
            return true;
        }
        if (node != NULL && node->getOpCodeValue() == TR::BBStart) {
            TR::Block *block = node->getBlock();
            if (!block->isExtensionOfPreviousBlock()) {
                nodeBBStart = node;
            }
        }
        cursor = cursor->getPrev();
        // OOL entry label could cause this
        if (!cursor)
            return true;
    }

    return true;
}

// `currentInstruction` argument will be required when we implement live register analysis
TR::RealRegister *OMR::ARM64::Machine::findBestFreeRegister(TR::Instruction *currentInstruction, TR_RegisterKinds rk,
    bool considerUnlatched, TR::Register *virtualReg)
{
    uint32_t preference = (virtualReg != NULL) ? virtualReg->getAssociation() : 0;
    bool liveRegOn = (self()->cg()->getLiveRegisters(rk) != NULL);
    uint32_t interference = 0;
    if (liveRegOn && virtualReg != NULL)
        interference = virtualReg->getInterference();

    int32_t first, last;
    uint32_t maskI;

    switch (rk) {
        case TR_GPR:
            first = maskI = TR::RealRegister::FirstGPR;
            last = TR::RealRegister::LastAssignableGPR;
            break;
        case TR_FPR:
        case TR_VRF:
            first = maskI = TR::RealRegister::FirstFPR;
            last = TR::RealRegister::LastFPR;
            break;
        default:
            TR_ASSERT(false, "Unsupported RegisterKind.");
    }

    uint32_t bestWeightSoFar = 0xffffffff;
    TR::RealRegister *freeRegister = NULL;
    TR::RealRegister *bestRegister = NULL;
    /****************************************************************************************************************/
    /*            STEP 1                         Register Associations                                              */
    /****************************************************************************************************************/
    // Register Associations are best effort. If you really need to map a virtual to a real, use register pre/post
    // dependency conditions.

    // If preference register is also marked as an interference at some point, check to see if the interference still
    // applies.
    if (liveRegOn && preference != 0 && (interference & (1 << (preference - maskI)))) {
        if (!boundNext(currentInstruction, preference, virtualReg)) {
            preference = 0;
        }
    }

    // Check if the preferred register is free
    if ((preference != 0) && (_registerFile[preference] != NULL)
        && ((_registerFile[preference]->getState() == TR::RealRegister::Free)
            || (considerUnlatched && (_registerFile[preference]->getState() == TR::RealRegister::Unlatched)))) {
        bestRegister = _registerFile[preference];

        if (bestRegister->getState() == TR::RealRegister::Unlatched) {
            bestRegister->setAssignedRegister(NULL);
            bestRegister->setState(TR::RealRegister::Free);
        }

        self()->cg()->traceRegisterAssignment("BEST FREE REG by pref for %R is %R", virtualReg, bestRegister);

        return bestRegister;
    }

    /*******************************************************************************************************************/
    /*            STEP 2                         Good 'ol linear search                                              */
    /****************************************************************************************************************/
    // If no association or assoc fails, find any other free register
    int32_t iOld = 0, iNew;
    for (int32_t i = first; i <= last; i++) {
        uint32_t tRegMask = _registerFile[i]->getRealRegisterMask();

        iNew = interference & (1 << (i - maskI));

        if ((_registerFile[i]->getState() == TR::RealRegister::Free
                || (considerUnlatched && _registerFile[i]->getState() == TR::RealRegister::Unlatched))
            && (freeRegister == NULL || (iOld && !iNew)
                || ((iOld || !iNew) && _registerFile[i]->getWeight() < bestWeightSoFar))) {
            iOld = iNew;
            freeRegister = _registerFile[i];
            bestWeightSoFar = freeRegister->getWeight();
        }
    }
    if (freeRegister != NULL && freeRegister->getState() == TR::RealRegister::Unlatched) {
        freeRegister->setAssignedRegister(NULL);
        freeRegister->setState(TR::RealRegister::Free);
    }

    if (freeRegister != NULL)
        self()->cg()->traceRegisterAssignment("BEST FREE REG for %R is %R", virtualReg, freeRegister);
    else
        self()->cg()->traceRegisterAssignment("BEST FREE REG for %R is NULL (could not find one)", virtualReg);

    return freeRegister;
}

TR::RealRegister *OMR::ARM64::Machine::freeBestRegister(TR::Instruction *currentInstruction,
    TR::Register *virtualRegister, TR::RealRegister *forced)
{
    TR::Register *candidates[NUM_ARM64_MAXR];
    TR::CodeGenerator *cg = self()->cg();
    TR::RealRegister *best;
    TR::Instruction *cursor;
    TR::Node *currentNode = currentInstruction->getNode();
    TR_RegisterKinds rk = (virtualRegister == NULL) ? TR_GPR : virtualRegister->getKind();
    int numCandidates = 0;

    cg->traceRegisterAssignment("FREE BEST REGISTER FOR %R", virtualRegister);

    if (forced != NULL) {
        best = forced;
        candidates[0] = best->getAssignedRegister();
    } else {
        int first, last;
        uint32_t maskI;
        uint32_t interference = 0; // TODO implement live register analysis for aarch64
        uint32_t preference = 0;
        bool pref_favored = false;

        switch (rk) {
            case TR_GPR:
                first = maskI = TR::RealRegister::FirstGPR;
                last = TR::RealRegister::LastGPR;
                break;
            case TR_FPR:
            case TR_VRF:
                first = maskI = TR::RealRegister::FirstFPR;
                last = TR::RealRegister::LastFPR;
                break;
            default:
                TR_ASSERT(false, "Unsupported RegisterKind.");
                break;
        }
        if ((cg->getLiveRegisters(rk) != NULL) && (virtualRegister != NULL)) {
            interference = virtualRegister->getInterference();
            preference = virtualRegister->getAssociation();

            // Consider yielding
            if (preference != 0 && boundNext(currentInstruction, preference, virtualRegister)) {
                pref_favored = true;
                interference &= ~(1 << (preference - maskI));
            }
        }

        for (int i = first; i <= last; i++) {
            uint32_t iInterfere = interference & (1 << (i - maskI));
            TR::RealRegister *realReg = self()->getRealRegister(static_cast<TR::RealRegister::RegNum>(i));
            TR::Register *tempReg;

            if (realReg->getState() == TR::RealRegister::Assigned) {
                TR::Register *associatedVirtual = realReg->getAssignedRegister();

                if (!iInterfere && (i == preference) && pref_favored) {
                    if (numCandidates == 0) {
                        candidates[0] = associatedVirtual;
                    } else {
                        tempReg = candidates[0];
                        candidates[0] = associatedVirtual;
                        candidates[numCandidates] = tempReg;
                    }
                } else {
                    candidates[numCandidates] = associatedVirtual;
                }
                numCandidates++;
            }
        }
        TR_ASSERT(numCandidates != 0, "All registers are blocked");

        cursor = currentInstruction;
        while (numCandidates > 1 && cursor != NULL && cursor->getOpCodeValue() != TR::InstOpCode::label
            && cursor->getOpCodeValue() != TR::InstOpCode::proc) {
            for (int i = 0; i < numCandidates; i++) {
                if (cursor->refsRegister(candidates[i])) {
                    candidates[i] = candidates[--numCandidates];
                }
            }
            cursor = cursor->getPrev();
        }
        best = toRealRegister(candidates[0]->getAssignedRegister());
    }

    self()->spillRegister(currentInstruction, candidates[0]);
    return best;
}

void OMR::ARM64::Machine::spillRegister(TR::Instruction *currentInstruction, TR::Register *virtReg)
{
    TR::Register *registerToSpill = virtReg;
    TR::CodeGenerator *cg = self()->cg();
    TR::Compilation *comp = cg->comp();
    TR_Debug *debugObj = cg->getDebug();
    TR_RegisterKinds rk = virtReg->getKind();
    TR::Node *currentNode = currentInstruction->getNode();

    const bool containsInternalPointer = registerToSpill->containsInternalPointer();
    const bool containsCollectedReference = registerToSpill->containsCollectedReference();

    TR_BackingStore *location = registerToSpill->getBackingStorage();
    switch (rk) {
        case TR_GPR:
            if ((cg->isOutOfLineColdPath() || cg->isOutOfLineHotPath()) && registerToSpill->getBackingStorage()) {
                // reuse the spill slot
                if (debugObj)
                    cg->traceRegisterAssignment("\nOOL: Reuse backing store (%p) for %R inside OOL\n", location,
                        registerToSpill);
            } else if (!containsInternalPointer) {
                location = cg->allocateSpill(TR::Compiler->om.sizeofReferenceAddress(),
                    registerToSpill->containsCollectedReference(), NULL);

                if (debugObj)
                    cg->traceRegisterAssignment("\nSpilling %R to (%p)\n", registerToSpill, location);
            } else {
                location = cg->allocateInternalPointerSpill(registerToSpill->getPinningArrayPointer());
                if (debugObj)
                    cg->traceRegisterAssignment("\nSpilling internal pointer %R to (%p)\n", registerToSpill, location);
            }
            break;
        case TR_FPR:
        case TR_VRF:
            if ((cg->isOutOfLineColdPath() || cg->isOutOfLineHotPath()) && registerToSpill->getBackingStorage()) {
                // reuse the spill slot
                if (debugObj)
                    cg->traceRegisterAssignment("\nOOL: Reuse backing store (%p) for %R inside OOL\n", location,
                        registerToSpill);
            } else {
                location = cg->allocateSpill((rk == TR_FPR) ? 8 : 16, false, NULL);
                if (debugObj)
                    cg->traceRegisterAssignment("\nSpilling FPR %R to (%p)\n", registerToSpill, location);
            }
            break;
        default:
            TR_ASSERT(false, "Unsupported RegisterKind.");
            break;
    }

    registerToSpill->setBackingStorage(location);

    TR::MemoryReference *tmemref
        = TR::MemoryReference::createWithSymRef(cg, currentNode, location->getSymbolReference());

    if (!cg->isOutOfLineColdPath()) {
        // the spilledRegisterList contains all registers that are spilled before entering
        // the OOL cold path, post dependencies will be generated using this list
        cg->getSpilledRegisterList()->push_front(registerToSpill);

        // OOL cold path: depth = 3, hot path: depth = 2,  main line: depth = 1
        // if the spill is outside of the OOL cold/hot path, we need to protect the spill slot
        // if we reverse spill this register inside the OOL cold/hot path
        if (!cg->isOutOfLineHotPath()) { // main line
            location->setMaxSpillDepth(1);
        } else {
            // hot path
            // do not overwrite main line spill depth
            if (location->getMaxSpillDepth() != 1) {
                location->setMaxSpillDepth(2);
            }
        }
        if (debugObj)
            cg->traceRegisterAssignment("OOL: adding %R to the spilledRegisterList, maxSpillDepth = %d ",
                registerToSpill, location->getMaxSpillDepth());
    } else {
        // do not overwrite mainline and hot path spill depth
        // if this spill is inside OOL cold path, we do not need to protecting the spill slot
        // because the post condition at OOL entry does not expect this register to be spilled
        if (location->getMaxSpillDepth() != 1 && location->getMaxSpillDepth() != 2) {
            location->setMaxSpillDepth(3);
            cg->traceRegisterAssignment("OOL: In OOL cold path, spilling %R not adding to spilledRegisterList",
                registerToSpill);
        }
    }

    if (comp->getOption(TR_TraceCG)) {
        diagnostic("\n\tspilling %s (%s)", best->getAssignedRegister()->getRegisterName(comp),
            best->getRegisterName(comp));
    }
    TR::InstOpCode::Mnemonic loadOp;
    switch (rk) {
        case TR_GPR:
            loadOp = TR::InstOpCode::ldrimmx;
            break;
        case TR_FPR:
            loadOp = TR::InstOpCode::vldrimmd;
            break;
        case TR_VRF:
            loadOp = TR::InstOpCode::vldrimmq;
            break;
        default:
            TR_ASSERT(false, "Unsupported RegisterKind.");
            break;
    }

    TR::RealRegister *realReg = toRealRegister(virtReg->getAssignedRegister());
    generateTrg1MemInstruction(cg, loadOp, currentNode, realReg, tmemref, currentInstruction);

    cg->traceRegFreed(registerToSpill, realReg);

    realReg->setAssignedRegister(NULL);
    realReg->setState(TR::RealRegister::Free);
    registerToSpill->setAssignedRegister(NULL);
}

TR::RealRegister *OMR::ARM64::Machine::reverseSpillState(TR::Instruction *currentInstruction,
    TR::Register *spilledRegister, TR::RealRegister *targetRegister)
{
    TR::Compilation *comp = self()->cg()->comp();
    TR::MemoryReference *tmemref;
    TR::RealRegister *sameReg;
    TR_BackingStore *location = spilledRegister->getBackingStorage();
    TR::Node *currentNode = currentInstruction->getNode();
    TR_RegisterKinds rk = spilledRegister->getKind();
    TR_Debug *debugObj = self()->cg()->getDebug();
    int32_t dataSize = 0;
    TR::InstOpCode::Mnemonic storeOp;

    if (targetRegister == NULL) {
        targetRegister = self()->findBestFreeRegister(currentInstruction, rk, false, spilledRegister);
        if (targetRegister == NULL) {
            targetRegister = self()->freeBestRegister(currentInstruction, spilledRegister, NULL);
        }
        targetRegister->setState(TR::RealRegister::Assigned);
    }

    if (self()->cg()->isOutOfLineColdPath()) {
        // the future and total use count might not always reflect register spill state
        // for example a new register assignment in the hot path would cause FC != TC
        // in this case, assign a new register and return
        if (!location) {
            if (debugObj)
                self()->cg()->traceRegisterAssignment("OOL: Not generating reverse spill for (%s)\n",
                    debugObj->getName(spilledRegister));
            return targetRegister;
        }
    }

    if (comp->getOption(TR_TraceCG)) {
        diagnostic("\n\tre-assigning spilled %s to %s", spilledRegister->getRegisterName(comp),
            targetRegister->getRegisterName(comp));
    }

    tmemref = TR::MemoryReference::createWithSymRef(self()->cg(), currentNode, location->getSymbolReference());

    switch (rk) {
        case TR_GPR:
            dataSize = TR::Compiler->om.sizeofReferenceAddress();
            break;
        case TR_FPR:
            dataSize = 8;
            break;
        case TR_VRF:
            dataSize = 16;
            break;
        default:
            TR_ASSERT(false, "Unsupported RegisterKind.");
            break;
    }
    if (self()->cg()->isOutOfLineColdPath()) {
        bool isOOLentryReverseSpill = false;
        if (currentInstruction->isLabel()) {
            if (((TR::ARM64LabelInstruction *)currentInstruction)->getLabelSymbol()->isStartOfColdInstructionStream()) {
                // indicates that we are at OOL entry point post conditions. Since
                // we are now exiting the OOL cold path (going reverse order)
                // and we called reverseSpillState(), the main line path
                // expects the Virt reg to be assigned to a real register
                // we can now safely unlock the protected backing storage
                // This prevents locking backing storage for future OOL blocks
                isOOLentryReverseSpill = true;
            }
        }
        // OOL: only free the spill slot if the register was spilled in the same or less dominant path
        // ex: spilled in cold path, reverse spill in hot path or main line
        // we have to spill this register again when we reach OOL entry point due to post
        // conditions. We want to guarantee that the same spill slot will be protected and reused.
        // maxSpillDepth: 3:cold path, 2:hot path, 1:main line
        // Also free the spill if maxSpillDepth==0, which will be the case if the reverse spill also occured on the hot
        // path. If the reverse spill occured on both paths then this is the last chance we have to free the spill slot.
        if (location->getMaxSpillDepth() == 3 || location->getMaxSpillDepth() == 0 || isOOLentryReverseSpill) {
            if (location->getMaxSpillDepth() != 0)
                location->setMaxSpillDepth(0);
            else if (debugObj)
                self()->cg()->traceRegisterAssignment("\nOOL: reverse spill %s in less dominant path (%d / 3), reverse "
                                                      "spill on both paths indicated, free spill slot (%p)\n",
                    debugObj->getName(spilledRegister), location->getMaxSpillDepth(), location);
            self()->cg()->freeSpill(location, dataSize, 0);

            if (!self()->cg()->isFreeSpillListLocked()) {
                spilledRegister->setBackingStorage(NULL);
            }
        } else {
            if (debugObj)
                self()->cg()->traceRegisterAssignment(
                    "\nOOL: reverse spill %s in less dominant path (%d / 3), protect spill slot (%p)\n",
                    debugObj->getName(spilledRegister), location->getMaxSpillDepth(), location);
        }
    } else if (self()->cg()->isOutOfLineHotPath()) {
        // the spilledRegisterList contains all registers that are spilled before entering
        // the OOL path (in backwards RA). Post dependencies will be generated using this list.
        // Any registers reverse spilled before entering OOL should be removed from the spilled list
        if (debugObj)
            self()->cg()->traceRegisterAssignment("\nOOL: removing %s from the spilledRegisterList\n",
                debugObj->getName(spilledRegister));
        self()->cg()->getSpilledRegisterList()->remove(spilledRegister);

        // Reset maxSpillDepth here so that in the cold path we know to free the spill
        // and so that the spill is not included in future GC points in the hot path while it is protected
        location->setMaxSpillDepth(0);
        if (location->getMaxSpillDepth() == 2) {
            self()->cg()->freeSpill(location, dataSize, 0);
            if (!self()->cg()->isFreeSpillListLocked()) {
                spilledRegister->setBackingStorage(NULL);
            }
        } else {
            if (debugObj)
                self()->cg()->traceRegisterAssignment(
                    "\nOOL: reverse spilling %s in less dominant path (%d / 2), protect spill slot (%p)\n",
                    debugObj->getName(spilledRegister), location->getMaxSpillDepth(), location);
        }
    } else // main line
    {
        if (debugObj)
            self()->cg()->traceRegisterAssignment("\nOOL: removing %s from the spilledRegisterList)\n",
                debugObj->getName(spilledRegister));
        self()->cg()->getSpilledRegisterList()->remove(spilledRegister);
        location->setMaxSpillDepth(0);
        self()->cg()->freeSpill(location, dataSize, 0);

        if (!self()->cg()->isFreeSpillListLocked()) {
            spilledRegister->setBackingStorage(NULL);
        }
    }
    switch (rk) {
        case TR_GPR:
            storeOp = TR::InstOpCode::strimmx;
            break;
        case TR_FPR:
            storeOp = TR::InstOpCode::vstrimmd;
            break;
        case TR_VRF:
            storeOp = TR::InstOpCode::vstrimmq;
            break;
        default:
            TR_ASSERT(false, "Unsupported RegisterKind.");
            break;
    }
    generateMemSrc1Instruction(self()->cg(), storeOp, currentNode, tmemref, targetRegister, currentInstruction);

    return targetRegister;
}

TR::RealRegister *OMR::ARM64::Machine::assignOneRegister(TR::Instruction *currentInstruction,
    TR::Register *virtualRegister)
{
    TR_RegisterKinds rk = virtualRegister->getKind();
    TR::RealRegister *assignedRegister = virtualRegister->getAssignedRealRegister();
    TR::CodeGenerator *cg = self()->cg();
    TR::Compilation *comp = cg->comp();

    if (assignedRegister == NULL) {
        cg->clearRegisterAssignmentFlags();
        cg->setRegisterAssignmentFlag(TR_NormalAssignment);

        if (virtualRegister->getTotalUseCount() != virtualRegister->getFutureUseCount()) {
            cg->setRegisterAssignmentFlag(TR_RegisterReloaded);
            assignedRegister = self()->reverseSpillState(currentInstruction, virtualRegister, NULL);
        } else {
            assignedRegister = self()->findBestFreeRegister(currentInstruction, rk, true, virtualRegister);
            if (assignedRegister == NULL) {
                cg->setRegisterAssignmentFlag(TR_RegisterSpilled);
                assignedRegister = self()->freeBestRegister(currentInstruction, virtualRegister, NULL);
            }
            if (cg->isOutOfLineColdPath()) {
                cg->getFirstTimeLiveOOLRegisterList()->push_front(virtualRegister);
            }
        }

        virtualRegister->setAssignedRegister(assignedRegister);
        assignedRegister->setAssignedRegister(virtualRegister);
        assignedRegister->setState(TR::RealRegister::Assigned);
        cg->traceRegAssigned(virtualRegister, assignedRegister);
    } else {
        TR_Debug *debugObj = cg->getDebug();
        auto registerName = (debugObj != NULL) ? debugObj->getName(assignedRegister) : "NULL";

        TR_ASSERT_FATAL(assignedRegister->getAssignedRegister(),
            "assignedRegister(%s) does not have assigned virtual register", registerName);
    }
    // Do bookkeeping register use count
    decFutureUseCountAndUnlatch(currentInstruction, virtualRegister);

    return assignedRegister;
}

/* generate instruction for register copy */
static void registerCopy(TR::Instruction *precedingInstruction, TR_RegisterKinds rk, TR::RealRegister *targetReg,
    TR::RealRegister *sourceReg, TR::CodeGenerator *cg)
{
    TR::Node *node = precedingInstruction->getNode();
    switch (rk) {
        case TR_GPR:
            generateMovInstruction(cg, node, targetReg, sourceReg, true, precedingInstruction);
            break;
        case TR_FPR:
            generateTrg1Src1Instruction(cg, TR::InstOpCode::fmovd, node, targetReg, sourceReg, precedingInstruction);
            break;
        case TR_VRF:
            generateTrg1Src2Instruction(cg, TR::InstOpCode::vorr16b, node, targetReg, sourceReg, sourceReg,
                precedingInstruction);
            break;
        default:
            TR_ASSERT(false, "Unsupported RegisterKind.");
            break;
    }
}

/* generate instructions for register exchange */
static void registerExchange(TR::Instruction *precedingInstruction, TR_RegisterKinds rk, TR::RealRegister *targetReg,
    TR::RealRegister *sourceReg, TR::CodeGenerator *cg)
{
    // middleReg is not used if rk==TR_GPR.

    TR::Node *node = precedingInstruction->getNode();
    if (rk == TR_GPR) {
        generateTrg1Src2Instruction(cg, TR::InstOpCode::eorx, node, targetReg, targetReg, sourceReg,
            precedingInstruction);
        generateTrg1Src2Instruction(cg, TR::InstOpCode::eorx, node, sourceReg, targetReg, sourceReg,
            precedingInstruction);
        generateTrg1Src2Instruction(cg, TR::InstOpCode::eorx, node, targetReg, targetReg, sourceReg,
            precedingInstruction);
    } else {
        generateTrg1Src2Instruction(cg, TR::InstOpCode::veor16b, node, targetReg, targetReg, sourceReg,
            precedingInstruction);
        generateTrg1Src2Instruction(cg, TR::InstOpCode::veor16b, node, sourceReg, targetReg, sourceReg,
            precedingInstruction);
        generateTrg1Src2Instruction(cg, TR::InstOpCode::veor16b, node, targetReg, targetReg, sourceReg,
            precedingInstruction);
    }
}

void OMR::ARM64::Machine::coerceRegisterAssignment(TR::Instruction *currentInstruction, TR::Register *virtualRegister,
    TR::RealRegister::RegNum registerNumber)
{
    TR::Compilation *comp = self()->cg()->comp();
    TR::RealRegister *targetRegister = _registerFile[registerNumber];
    TR::RealRegister *realReg = virtualRegister->getAssignedRealRegister();
    TR::RealRegister *currentAssignedRegister = realReg ? toRealRegister(realReg) : NULL;
    TR_RegisterKinds rk = virtualRegister->getKind();

    if (comp->getOption(TR_TraceCG)) {
        if (currentAssignedRegister)
            diagnostic("\n\tcoercing %s from %s to %s", virtualRegister->getRegisterName(comp),
                currentAssignedRegister->getRegisterName(comp), targetRegister->getRegisterName(comp));
        else
            diagnostic("\n\tcoercing %s to %s", virtualRegister->getRegisterName(comp),
                targetRegister->getRegisterName(comp));
    }

    if (currentAssignedRegister == targetRegister)
        return;

    if (targetRegister->getState() == TR::RealRegister::Free
        || targetRegister->getState() == TR::RealRegister::Unlatched) {
#ifdef DEBUG
        if (comp->getOption(TR_TraceCG))
            diagnostic(", which is free");
#endif
        if (currentAssignedRegister == NULL) {
            if (virtualRegister->getTotalUseCount() != virtualRegister->getFutureUseCount()) {
                self()->cg()->setRegisterAssignmentFlag(TR_RegisterReloaded);
                self()->reverseSpillState(currentInstruction, virtualRegister, targetRegister);
            } else {
                if (self()->cg()->isOutOfLineColdPath()) {
                    self()->cg()->getFirstTimeLiveOOLRegisterList()->push_front(virtualRegister);
                }
            }
        } else {
            registerCopy(currentInstruction, rk, currentAssignedRegister, targetRegister, self()->cg());
            currentAssignedRegister->setState(TR::RealRegister::Free);
            currentAssignedRegister->setAssignedRegister(NULL);
        }
    } else {
        TR::RealRegister *spareReg = NULL;
        TR::Register *currentTargetVirtual = targetRegister->getAssignedRegister();

        if (targetRegister->getState() == TR::RealRegister::Blocked) {
#ifdef DEBUG
            if (comp->getOption(TR_TraceCG))
                diagnostic(", which is blocked and assigned to %s", currentTargetVirtual->getRegisterName(comp));
#endif
            if (currentAssignedRegister) {
                self()->cg()->traceRegAssigned(currentTargetVirtual, currentAssignedRegister);
                registerExchange(currentInstruction, rk, targetRegister, currentAssignedRegister, self()->cg());
                currentAssignedRegister->setState(TR::RealRegister::Blocked);
                currentAssignedRegister->setAssignedRegister(currentTargetVirtual);
                currentTargetVirtual->setAssignedRegister(currentAssignedRegister);
                // For Non-GPR, spareReg remains FREE.
            } else {
                spareReg = self()->findBestFreeRegister(currentInstruction, rk, false, currentTargetVirtual);
                self()->cg()->setRegisterAssignmentFlag(TR_IndirectCoercion);
                if (spareReg == NULL) {
                    self()->cg()->setRegisterAssignmentFlag(TR_RegisterSpilled);
                    virtualRegister->block();
                    spareReg = self()->freeBestRegister(currentInstruction, currentTargetVirtual);
                    virtualRegister->unblock();
                }
                self()->cg()->traceRegAssigned(currentTargetVirtual, spareReg);
                registerCopy(currentInstruction, rk, targetRegister, spareReg, self()->cg());
                spareReg->setState(TR::RealRegister::Blocked);
                currentTargetVirtual->setAssignedRegister(spareReg);
                spareReg->setAssignedRegister(currentTargetVirtual);
                // spareReg is assigned.

                if (virtualRegister->getTotalUseCount() != virtualRegister->getFutureUseCount()) {
                    self()->cg()->setRegisterAssignmentFlag(TR_RegisterReloaded);
                    self()->reverseSpillState(currentInstruction, virtualRegister, targetRegister);
                } else {
                    if (self()->cg()->isOutOfLineColdPath()) {
                        self()->cg()->getFirstTimeLiveOOLRegisterList()->push_front(virtualRegister);
                    }
                }
            }
        } else if (targetRegister->getState() == TR::RealRegister::Assigned) {
#ifdef DEBUG
            if (comp->getOption(TR_TraceCG))
                diagnostic(", which is assigned to %s", currentTargetVirtual->getRegisterName(comp));
#endif

            self()->cg()->setRegisterAssignmentFlag(TR_IndirectCoercion);
            if (currentAssignedRegister) {
                self()->cg()->traceRegAssigned(currentTargetVirtual, currentAssignedRegister);
                registerExchange(currentInstruction, rk, targetRegister, currentAssignedRegister, self()->cg());
                currentAssignedRegister->setState(TR::RealRegister::Assigned);
                currentAssignedRegister->setAssignedRegister(currentTargetVirtual);
                currentTargetVirtual->setAssignedRegister(currentAssignedRegister);
            } else {
                spareReg = self()->findBestFreeRegister(currentInstruction, rk, false, currentTargetVirtual);
                if (spareReg == NULL) {
                    self()->cg()->setRegisterAssignmentFlag(TR_RegisterSpilled);
                    self()->freeBestRegister(currentInstruction, currentTargetVirtual, targetRegister);
                } else {
                    self()->cg()->traceRegAssigned(currentTargetVirtual, spareReg);
                    registerCopy(currentInstruction, rk, targetRegister, spareReg, self()->cg());
                    spareReg->setState(TR::RealRegister::Assigned);
                    spareReg->setAssignedRegister(currentTargetVirtual);
                    currentTargetVirtual->setAssignedRegister(spareReg);
                    // spareReg is assigned.
                }

                if (virtualRegister->getTotalUseCount() != virtualRegister->getFutureUseCount()) {
                    self()->cg()->setRegisterAssignmentFlag(TR_RegisterReloaded);
                    self()->reverseSpillState(currentInstruction, virtualRegister, targetRegister);
                } else {
                    if (self()->cg()->isOutOfLineColdPath()) {
                        self()->cg()->getFirstTimeLiveOOLRegisterList()->push_front(virtualRegister);
                    }
                }
            }
            self()->cg()->resetRegisterAssignmentFlag(TR_IndirectCoercion);
        } else {
#ifdef DEBUG
            if (comp->getOption(TR_TraceCG))
                diagnostic(", which is in an unknown state %d", targetRegister->getState());
#endif
        }
    }

    targetRegister->setState(TR::RealRegister::Assigned);
    targetRegister->setAssignedRegister(virtualRegister);
    virtualRegister->setAssignedRegister(targetRegister);
    self()->cg()->traceRegAssigned(virtualRegister, targetRegister);
}

void OMR::ARM64::Machine::spillAllVectorRegisters(TR::Instruction *currentInstruction)
{
    int32_t first = TR::RealRegister::FirstFPR;
    int32_t last = TR::RealRegister::LastFPR;
    TR::Node *node = currentInstruction->getNode();

    for (int32_t i = first; i <= last; i++) {
        TR::Register *virtReg = NULL;
        TR::RealRegister *realReg = self()->getRealRegister(static_cast<TR::RealRegister::RegNum>(i));

        if ((realReg->getState() == TR::RealRegister::Assigned) && (virtReg = realReg->getAssignedRegister())
            && (virtReg->getKind() == TR_VRF)) {
            self()->spillRegister(currentInstruction, virtReg);
        }
    }
}

void OMR::ARM64::Machine::initializeRegisterFile()
{
    _registerFile[TR::RealRegister::NoReg] = NULL;
    _registerFile[TR::RealRegister::SpilledReg] = NULL;

    _registerFile[TR::RealRegister::x0] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x0, TR::RealRegister::x0Mask, self()->cg());

    _registerFile[TR::RealRegister::x1] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x1, TR::RealRegister::x1Mask, self()->cg());

    _registerFile[TR::RealRegister::x2] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x2, TR::RealRegister::x2Mask, self()->cg());

    _registerFile[TR::RealRegister::x3] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x3, TR::RealRegister::x3Mask, self()->cg());

    _registerFile[TR::RealRegister::x4] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x4, TR::RealRegister::x4Mask, self()->cg());

    _registerFile[TR::RealRegister::x5] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x5, TR::RealRegister::x5Mask, self()->cg());

    _registerFile[TR::RealRegister::x6] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x6, TR::RealRegister::x6Mask, self()->cg());

    _registerFile[TR::RealRegister::x7] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x7, TR::RealRegister::x7Mask, self()->cg());

    _registerFile[TR::RealRegister::x8] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x8, TR::RealRegister::x8Mask, self()->cg());

    _registerFile[TR::RealRegister::x9] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x9, TR::RealRegister::x9Mask, self()->cg());

    _registerFile[TR::RealRegister::x10] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x10, TR::RealRegister::x10Mask, self()->cg());

    _registerFile[TR::RealRegister::x11] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x11, TR::RealRegister::x11Mask, self()->cg());

    _registerFile[TR::RealRegister::x12] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x12, TR::RealRegister::x12Mask, self()->cg());

    _registerFile[TR::RealRegister::x13] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x13, TR::RealRegister::x13Mask, self()->cg());

    _registerFile[TR::RealRegister::x14] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x14, TR::RealRegister::x14Mask, self()->cg());

    _registerFile[TR::RealRegister::x15] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x15, TR::RealRegister::x15Mask, self()->cg());

    _registerFile[TR::RealRegister::x16] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x16, TR::RealRegister::x16Mask, self()->cg());

    _registerFile[TR::RealRegister::x17] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x17, TR::RealRegister::x17Mask, self()->cg());

    _registerFile[TR::RealRegister::x18] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x18, TR::RealRegister::x18Mask, self()->cg());

    _registerFile[TR::RealRegister::x19] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x19, TR::RealRegister::x19Mask, self()->cg());

    _registerFile[TR::RealRegister::x20] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x20, TR::RealRegister::x20Mask, self()->cg());

    _registerFile[TR::RealRegister::x21] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x21, TR::RealRegister::x21Mask, self()->cg());

    _registerFile[TR::RealRegister::x22] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x22, TR::RealRegister::x22Mask, self()->cg());

    _registerFile[TR::RealRegister::x23] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x23, TR::RealRegister::x23Mask, self()->cg());

    _registerFile[TR::RealRegister::x24] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x24, TR::RealRegister::x24Mask, self()->cg());

    _registerFile[TR::RealRegister::x25] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x25, TR::RealRegister::x25Mask, self()->cg());

    _registerFile[TR::RealRegister::x26] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x26, TR::RealRegister::x26Mask, self()->cg());

    _registerFile[TR::RealRegister::x27] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x27, TR::RealRegister::x27Mask, self()->cg());

    _registerFile[TR::RealRegister::x28] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x28, TR::RealRegister::x28Mask, self()->cg());

    _registerFile[TR::RealRegister::x29] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::x29, TR::RealRegister::x29Mask, self()->cg());

    /* x30 is used as LR on ARM64 */
    _registerFile[TR::RealRegister::lr] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::lr, TR::RealRegister::x30Mask, self()->cg());

    /* SP */
    _registerFile[TR::RealRegister::sp] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::sp, TR::RealRegister::x31Mask, self()->cg());

    /* XZR */
    _registerFile[TR::RealRegister::xzr] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_GPR, 0,
        TR::RealRegister::Free, TR::RealRegister::xzr, TR::RealRegister::x31Mask, self()->cg());

    _registerFile[TR::RealRegister::v0] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v0, TR::RealRegister::v0Mask, self()->cg());

    _registerFile[TR::RealRegister::v1] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v1, TR::RealRegister::v1Mask, self()->cg());

    _registerFile[TR::RealRegister::v2] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v2, TR::RealRegister::v2Mask, self()->cg());

    _registerFile[TR::RealRegister::v3] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v3, TR::RealRegister::v3Mask, self()->cg());

    _registerFile[TR::RealRegister::v4] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v4, TR::RealRegister::v4Mask, self()->cg());

    _registerFile[TR::RealRegister::v5] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v5, TR::RealRegister::v5Mask, self()->cg());

    _registerFile[TR::RealRegister::v6] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v6, TR::RealRegister::v6Mask, self()->cg());

    _registerFile[TR::RealRegister::v7] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v7, TR::RealRegister::v7Mask, self()->cg());

    _registerFile[TR::RealRegister::v8] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v8, TR::RealRegister::v8Mask, self()->cg());

    _registerFile[TR::RealRegister::v9] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v9, TR::RealRegister::v9Mask, self()->cg());

    _registerFile[TR::RealRegister::v10] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v10, TR::RealRegister::v10Mask, self()->cg());

    _registerFile[TR::RealRegister::v11] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v11, TR::RealRegister::v11Mask, self()->cg());

    _registerFile[TR::RealRegister::v12] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v12, TR::RealRegister::v12Mask, self()->cg());

    _registerFile[TR::RealRegister::v13] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v13, TR::RealRegister::v13Mask, self()->cg());

    _registerFile[TR::RealRegister::v14] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v14, TR::RealRegister::v14Mask, self()->cg());

    _registerFile[TR::RealRegister::v15] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v15, TR::RealRegister::v15Mask, self()->cg());

    _registerFile[TR::RealRegister::v16] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v16, TR::RealRegister::v16Mask, self()->cg());

    _registerFile[TR::RealRegister::v17] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v17, TR::RealRegister::v17Mask, self()->cg());

    _registerFile[TR::RealRegister::v18] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v18, TR::RealRegister::v18Mask, self()->cg());

    _registerFile[TR::RealRegister::v19] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v19, TR::RealRegister::v19Mask, self()->cg());

    _registerFile[TR::RealRegister::v20] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v20, TR::RealRegister::v20Mask, self()->cg());

    _registerFile[TR::RealRegister::v21] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v21, TR::RealRegister::v21Mask, self()->cg());

    _registerFile[TR::RealRegister::v22] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v22, TR::RealRegister::v22Mask, self()->cg());

    _registerFile[TR::RealRegister::v23] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v23, TR::RealRegister::v23Mask, self()->cg());

    _registerFile[TR::RealRegister::v24] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v24, TR::RealRegister::v24Mask, self()->cg());

    _registerFile[TR::RealRegister::v25] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v25, TR::RealRegister::v25Mask, self()->cg());

    _registerFile[TR::RealRegister::v26] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v26, TR::RealRegister::v26Mask, self()->cg());

    _registerFile[TR::RealRegister::v27] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v27, TR::RealRegister::v27Mask, self()->cg());

    _registerFile[TR::RealRegister::v28] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v28, TR::RealRegister::v28Mask, self()->cg());

    _registerFile[TR::RealRegister::v29] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v29, TR::RealRegister::v29Mask, self()->cg());

    _registerFile[TR::RealRegister::v30] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v30, TR::RealRegister::v30Mask, self()->cg());

    _registerFile[TR::RealRegister::v31] = new (self()->cg()->trHeapMemory()) TR::RealRegister(TR_FPR, 0,
        TR::RealRegister::Free, TR::RealRegister::v31, TR::RealRegister::v31Mask, self()->cg());
}

// Register Association ////////////////////////////////////////////
void OMR::ARM64::Machine::setRegisterWeightsFromAssociations()
{
    TR::ARM64LinkageProperties linkageProperties = self()->cg()->getProperties();
    int32_t first = TR::RealRegister::FirstGPR;
    TR::Compilation *comp = self()->cg()->comp();
    int32_t last = TR::RealRegister::LastAssignableFPR;

    for (int32_t i = first; i <= last; ++i) {
        TR::Register *assocReg = getVirtualAssociatedWithReal(static_cast<TR::RealRegister::RegNum>(i));
        if (linkageProperties.getPreserved(static_cast<TR::RealRegister::RegNum>(i))
            && _registerFile[i]->getHasBeenAssignedInMethod() == false) {
            if (assocReg) {
                assocReg->setAssociation(i);
            }
            _registerFile[i]->setWeight(ARM64_REGISTER_INITIAL_PRESERVED_WEIGHT);
        } else if (assocReg == NULL) {
            _registerFile[i]->setWeight(ARM64_REGISTER_BASIC_WEIGHT);
        } else {
            assocReg->setAssociation(i);
            if (assocReg->isPlaceholderReg()) {
                // placeholder register and is only needed at the specific dependency
                // site (usually a killed register on a call)
                // so defer this register's weight to that of registers
                // where the associated register has a longer life
                _registerFile[i]->setWeight(ARM64_REGISTER_PLACEHOLDER_WEIGHT);
            } else {
                _registerFile[i]->setWeight(ARM64_REGISTER_ASSOCIATED_WEIGHT);
            }
        }
    }
}

void OMR::ARM64::Machine::createRegisterAssociationDirective(TR::Instruction *cursor)
{
    TR::Compilation *comp = self()->cg()->comp();
    int32_t first = TR::RealRegister::FirstGPR;
    int32_t last = TR::RealRegister::LastAssignableFPR;
    TR::RegisterDependencyConditions *associations
        = new (self()->cg()->trHeapMemory()) TR::RegisterDependencyConditions(0, last, self()->cg()->trMemory());

    // Go through the current associations held in the machine and put a copy of
    // that state out into the stream after the cursor
    // so that when the register assigner goes backwards through this point
    // it updates the machine and register association states properly
    //
    for (int32_t i = first; i <= last; i++) {
        TR::RealRegister::RegNum regNum = static_cast<TR::RealRegister::RegNum>(i);
        associations->addPostCondition(self()->getVirtualAssociatedWithReal(regNum), regNum);
    }

    generateAdminInstruction(self()->cg(), TR::InstOpCode::assocreg, cursor->getNode(), associations, NULL, cursor);
}

TR::Register *OMR::ARM64::Machine::setVirtualAssociatedWithReal(TR::RealRegister::RegNum regNum, TR::Register *virtReg)
{
    if (virtReg) {
        // disable previous association
        if (virtReg->getAssociation() && (_registerAssociations[virtReg->getAssociation()] == virtReg))
            _registerAssociations[virtReg->getAssociation()] = NULL;

        if (regNum == TR::RealRegister::NoReg || regNum == TR::RealRegister::xzr
            || regNum >= TR::RealRegister::NumRegisters) {
            virtReg->setAssociation(TR::RealRegister::NoReg);
            return NULL;
        }
    }

    return _registerAssociations[regNum] = virtReg;
}

void OMR::ARM64::Machine::takeRegisterStateSnapShot()
{
    int32_t i;
    for (i = TR::RealRegister::FirstGPR; i <= TR::RealRegister::LastAssignableFPR; i++) {
        _registerStatesSnapShot[i] = _registerFile[i]->getState();
        _assignedRegisterSnapShot[i] = _registerFile[i]->getAssignedRegister();
        _registerFlagsSnapShot[i] = _registerFile[i]->getFlags();
        _registerAssociationsSnapShot[i]
            = self()->getVirtualAssociatedWithReal(static_cast<TR::RealRegister::RegNum>(i));
        _registerWeightSnapShot[i] = _registerFile[i]->getWeight();
    }
}

void OMR::ARM64::Machine::restoreRegisterStateFromSnapShot()
{
    int32_t i;
    for (i = TR::RealRegister::FirstGPR; i <= TR::RealRegister::LastAssignableFPR; i++) {
        _registerFile[i]->setWeight(_registerWeightSnapShot[i]);
        _registerFile[i]->setFlags(_registerFlagsSnapShot[i]);
        _registerFile[i]->setState(_registerStatesSnapShot[i]);
        self()->setVirtualAssociatedWithReal(static_cast<TR::RealRegister::RegNum>(i),
            _registerAssociationsSnapShot[i]);

        if (_registerFile[i]->getState() == TR::RealRegister::Free) {
            if (_registerFile[i]->getAssignedRegister() != NULL) {
                // clear the Virt -> Real reg assignment if we restored the Real reg state to FREE
                _registerFile[i]->getAssignedRegister()->setAssignedRegister(NULL);
            }
        } else if (_registerFile[i]->getState() == TR::RealRegister::Assigned) {
            if (_registerFile[i]->getAssignedRegister() != NULL
                && _registerFile[i]->getAssignedRegister() != _assignedRegisterSnapShot[i]) {
                // If the virtual register associated with the _registerFile[i] is not assigned to the current real
                // register it must have been updated by prior _registerFile.... Do NOT Clear.
                //   Ex:
                //     RegFile starts as:
                //       _registerFile[12] -> GPR_3555
                //       _registerFile[15] -> GPR_3545
                //     SnapShot:
                //       _registerFile[12] -> GPR_3545
                //       _registerFile[15] -> GPR_3562
                //  When we handled _registerFile[12], we would have updated the assignment of GPR_3545 (currently to
                //  GPR15) to GPR12. When we subsequently handle _registerFile[15], we cannot blindly reset GPR_3545's
                //  assigned register to NULL, as that will incorrectly break the assignment to GPR12.
                if (_registerFile[i]->getAssignedRegister()->getAssignedRegister() == _registerFile[i]) {
                    // clear the Virt -> Real reg assignment for any newly assigned virtual register (due to spills) in
                    // the hot path
                    _registerFile[i]->getAssignedRegister()->setAssignedRegister(NULL);
                }
            }
        }
        _registerFile[i]->setAssignedRegister(_assignedRegisterSnapShot[i]);
        // make sure to double link virt - real reg if assigned
        if (_registerFile[i]->getState() == TR::RealRegister::Assigned) {
            _registerFile[i]->getAssignedRegister()->setAssignedRegister(_registerFile[i]);
        }
        // Don't restore registers that died after the snapshot was taken since they are guaranteed to not be used in
        // the outlined path
        if (_registerFile[i]->getState() == TR::RealRegister::Assigned
            && _registerFile[i]->getAssignedRegister()->getFutureUseCount() == 0) {
            _registerFile[i]->setState(TR::RealRegister::Free);
            _registerFile[i]->getAssignedRegister()->setAssignedRegister(NULL);
            _registerFile[i]->setAssignedRegister(NULL);
        }
    }
}

TR::RegisterDependencyConditions *OMR::ARM64::Machine::createCondForLiveAndSpilledGPRs(
    TR::list<TR::Register *> *spilledRegisterList)
{
    int32_t i, c = 0;
    // Calculate number of register dependencies required. This step is not really necessary, but
    // it is space conscious
    //
    TR::Compilation *comp = self()->cg()->comp();
    for (i = TR::RealRegister::FirstGPR; i <= TR::RealRegister::LastAssignableFPR; i++) {
        TR::RealRegister *realReg = self()->getRealRegister(static_cast<TR::RealRegister::RegNum>(i));

        TR_ASSERT(realReg->getState() == TR::RealRegister::Assigned || realReg->getState() == TR::RealRegister::Free
                || realReg->getState() == TR::RealRegister::Locked,
            "cannot handle realReg state %d, (block state is %d)\n", realReg->getState(), TR::RealRegister::Blocked);

        if (realReg->getState() == TR::RealRegister::Assigned)
            c++;
    }

    c += spilledRegisterList ? spilledRegisterList->size() : 0;

    TR::RegisterDependencyConditions *deps = NULL;

    if (c) {
        deps = new (self()->cg()->trHeapMemory()) TR::RegisterDependencyConditions(0, c, self()->cg()->trMemory());
        for (i = TR::RealRegister::FirstGPR; i <= TR::RealRegister::LastAssignableFPR; i++) {
            TR::RealRegister *realReg = self()->getRealRegister(static_cast<TR::RealRegister::RegNum>(i));
            if (realReg->getState() == TR::RealRegister::Assigned) {
                TR::Register *virtReg = realReg->getAssignedRegister();
                TR_ASSERT(!spilledRegisterList
                        || !(std::find(spilledRegisterList->begin(), spilledRegisterList->end(), virtReg)
                            != spilledRegisterList->end()),
                    "a register should not be in both an assigned state and in the spilled list\n");

                deps->addPostCondition(virtReg, realReg->getRegisterNumber());

                // This method is called by ARM64OutOfLineCodeSection::assignRegister only.
                // Inside the caller, the register dependency condition this method returns
                // is set to the entry label instruction of the cold path, and bookkeeping of
                // register use count is done. During bookkeeping, only total/out of line use count of
                // registers are increased, so we need to manually increase future use count here.
                virtReg->incFutureUseCount();
            }
        }
    }

    if (spilledRegisterList) {
        for (auto li = spilledRegisterList->begin(); li != spilledRegisterList->end(); ++li) {
            TR::Register *virtReg = *li;
            deps->addPostCondition(virtReg, TR::RealRegister::SpilledReg);

            // we need to manually increase future use count here too.
            virtReg->incFutureUseCount();
        }
    }

    return deps;
}

/**
 * @brief Decrease future use count of the register and unlatch it if necessary
 *
 * @param currentInstruction     : instruction
 * @param virtualRegister        : virtual register
 *
 * @details
 * This method decrements the future use count of the given virtual register. If register
 * assignment is currently stepping through an out of line code section it also decrements
 * the out of line use count. If the future use count has reached 0, or if register assignment
 * is currently stepping through the 'hot path' of a corresponding out of line code section
 * and the future use count is equal to the out of line use count (indicating that there are
 * no further uses of this virtual register in any non-OOL path) it will unlatch the register.
 * (If the register has any OOL uses remaining it will be restored to its previous assignment
 * elsewhere.)
 * We borrowed the code from p codegen regarding out of line use count.
 * P codegen uses the out of line use count of the register to judge if there are no more uses of the register.
 * Z codegen does it differently. It uses the start range of the instruction.
 * We cannot use the same approach with z because we would have problem when the instruction
 * uses the same virtual register multiple times (e.g. same register for source and target).
 * Thus, we rely on the out of line use count as p codegen does.
 */
void OMR::ARM64::Machine::decFutureUseCountAndUnlatch(TR::Instruction *currentInstruction,
    TR::Register *virtualRegister)
{
    TR::CodeGenerator *cg = self()->cg();
    TR_Debug *debugObj = cg->getDebug();

    virtualRegister->decFutureUseCount();

    TR_ASSERT(virtualRegister->getFutureUseCount() >= 0,
        "\nRegister assignment: register [%s] futureUseCount should not be negative (for node [%s], ref count=%d) !\n",
        cg->getDebug()->getName(virtualRegister), cg->getDebug()->getName(currentInstruction->getNode()),
        currentInstruction->getNode()->getReferenceCount());

    if (cg->isOutOfLineColdPath())
        virtualRegister->decOutOfLineUseCount();

    TR_ASSERT(virtualRegister->getFutureUseCount() >= virtualRegister->getOutOfLineUseCount(),
        "\nRegister assignment: register [%s] Future use count (%d) less than out of line use count (%d)\n",
        cg->getDebug()->getName(virtualRegister), virtualRegister->getFutureUseCount(),
        virtualRegister->getOutOfLineUseCount());

    // This register should be unlatched if there are no more uses
    // or
    // if we're currently in the hot path and all remaining uses are out of line.
    //
    // If the only remaining uses are out of line, then this register should be unlatched
    // here, and when the register allocator reaches the branch to the outlined code it
    // will revive the register and proceed to allocate registers in the outlined code,
    // where presumably the future use count will finally hit 0.
    if (virtualRegister->getFutureUseCount() == 0
        || (self()->cg()->isOutOfLineHotPath()
            && virtualRegister->getFutureUseCount() == virtualRegister->getOutOfLineUseCount())) {
        if (virtualRegister->getFutureUseCount() != 0) {
            if (debugObj) {
                self()->cg()->traceRegisterAssignment("\nOOL: %s's remaining uses are out-of-line, unlatching\n",
                    debugObj->getName(virtualRegister));
            }
        }
        virtualRegister->getAssignedRealRegister()->setAssignedRegister(NULL);
        virtualRegister->getAssignedRealRegister()->setState(TR::RealRegister::Unlatched);
        virtualRegister->setAssignedRegister(NULL);
    }
}
