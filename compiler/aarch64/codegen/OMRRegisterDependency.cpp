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
#include "codegen/ARM64Instruction.hpp"
#include "codegen/BackingStore.hpp"
#include "codegen/CodeGenerator.hpp"
#include "codegen/GenerateInstructions.hpp"
#include "codegen/Instruction.hpp"
#include "codegen/MemoryReference.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/RegisterDependency.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"

// Replace this by #include "codegen/Instruction.hpp" when available for aarch64
namespace TR {
class Instruction;
}

OMR::ARM64::RegisterDependencyConditions::RegisterDependencyConditions(uint16_t numPreConds, uint16_t numPostConds,
    TR_Memory *m)
    : _preConditions(new(numPreConds, m) TR::RegisterDependencyGroup)
    , _postConditions(new(numPostConds, m) TR::RegisterDependencyGroup)
    , _numPreConditions(numPreConds)
    , _addCursorForPre(0)
    , _numPostConditions(numPostConds)
    , _addCursorForPost(0)
{}

OMR::ARM64::RegisterDependencyConditions::RegisterDependencyConditions(TR::CodeGenerator *cg, TR::Node *node,
    uint32_t extranum, TR::Instruction **cursorPtr)
{
    List<TR::Register> regList(cg->trMemory());
    TR::Instruction *iCursor = (cursorPtr == NULL) ? NULL : *cursorPtr;
    int32_t totalNum = node->getNumChildren() + extranum;
    int32_t i;

    cg->comp()->incVisitCount();

    _preConditions = new (totalNum, cg->trMemory()) TR::RegisterDependencyGroup;
    _postConditions = new (totalNum, cg->trMemory()) TR::RegisterDependencyGroup;
    _numPreConditions = totalNum;
    _addCursorForPre = 0;
    _numPostConditions = totalNum;
    _addCursorForPost = 0;

    // First, handle dependencies that match current association
    for (i = 0; i < node->getNumChildren(); i++) {
        TR::Node *child = node->getChild(i);
        TR::Register *reg = child->getRegister();
        TR::RealRegister::RegNum regNum
            = (TR::RealRegister::RegNum)cg->getGlobalRegister(child->getGlobalRegisterNumber());

        if (reg->getAssociation() != regNum) {
            continue;
        }

        addPreCondition(reg, regNum);
        addPostCondition(reg, regNum);
        regList.add(reg);
    }

    // Second pass to handle dependencies for which association does not exist
    // or does not match
    for (i = 0; i < node->getNumChildren(); i++) {
        TR::Node *child = node->getChild(i);
        TR::Register *reg = child->getRegister();
        TR::Register *copyReg = NULL;
        TR::RealRegister::RegNum regNum
            = (TR::RealRegister::RegNum)cg->getGlobalRegister(child->getGlobalRegisterNumber());

        if (reg->getAssociation() == regNum) {
            continue;
        }

        if (regList.find(reg)) {
            TR_RegisterKinds kind = reg->getKind();

            TR_ASSERT_FATAL((kind == TR_GPR) || (kind == TR_FPR) || (kind == TR_VRF), "Invalid register kind.");

            if (kind == TR_GPR) {
                bool containsInternalPointer = reg->getPinningArrayPointer();
                copyReg = (reg->containsCollectedReference() && !containsInternalPointer)
                    ? cg->allocateCollectedReferenceRegister()
                    : cg->allocateRegister();
                if (containsInternalPointer) {
                    copyReg->setContainsInternalPointer();
                    copyReg->setPinningArrayPointer(reg->getPinningArrayPointer());
                }
                iCursor = generateMovInstruction(cg, node, copyReg, reg, true, iCursor);
            } else if (kind == TR_VRF) {
                copyReg = cg->allocateRegister(TR_VRF);
                iCursor = generateTrg1Src2Instruction(cg, TR::InstOpCode::vorr16b, node, copyReg, reg, reg, iCursor);
            } else {
                bool isSinglePrecision = reg->isSinglePrecision();
                copyReg = isSinglePrecision ? cg->allocateSinglePrecisionRegister() : cg->allocateRegister(TR_FPR);
                iCursor = generateTrg1Src1Instruction(cg,
                    isSinglePrecision ? TR::InstOpCode::fmovs : TR::InstOpCode::fmovd, node, copyReg, reg, iCursor);
            }

            reg = copyReg;
        }

        addPreCondition(reg, regNum);
        addPostCondition(reg, regNum);
        if (copyReg != NULL) {
            cg->stopUsingRegister(copyReg);
        } else {
            regList.add(reg);
        }
    }

    if (iCursor != NULL && cursorPtr != NULL) {
        *cursorPtr = iCursor;
    }
}

void OMR::ARM64::RegisterDependencyConditions::unionNoRegPostCondition(TR::Register *reg, TR::CodeGenerator *cg)
{
    addPostCondition(reg, TR::RealRegister::NoReg);
}

void OMR::ARM64::RegisterDependencyConditions::addPreCondition(TR::Register *vr, TR::RealRegister::RegNum rr,
    uint8_t flag)
{
    TR_ASSERT(_addCursorForPre < _numPreConditions, " Pre Condition array bounds overflow");
    _preConditions->setDependencyInfo(_addCursorForPre++, vr, rr, flag);
}

void OMR::ARM64::RegisterDependencyConditions::addPostCondition(TR::Register *vr, TR::RealRegister::RegNum rr,
    uint8_t flag)
{
    TR_ASSERT(_addCursorForPost < _numPostConditions, " Post Condition array bounds overflow");
    _postConditions->setDependencyInfo(_addCursorForPost++, vr, rr, flag);
}

void OMR::ARM64::RegisterDependencyConditions::assignPreConditionRegisters(TR::Instruction *currentInstruction,
    TR_RegisterKinds kindToBeAssigned, TR::CodeGenerator *cg)
{
    if (_preConditions != NULL) {
        cg->clearRegisterAssignmentFlags();
        cg->setRegisterAssignmentFlag(TR_PreDependencyCoercion);
        _preConditions->assignRegisters(currentInstruction, kindToBeAssigned, _addCursorForPre, cg);
    }
}

void OMR::ARM64::RegisterDependencyConditions::assignPostConditionRegisters(TR::Instruction *currentInstruction,
    TR_RegisterKinds kindToBeAssigned, TR::CodeGenerator *cg)
{
    if (_postConditions != NULL) {
        cg->clearRegisterAssignmentFlags();
        cg->setRegisterAssignmentFlag(TR_PostDependencyCoercion);
        _postConditions->assignRegisters(currentInstruction, kindToBeAssigned, _addCursorForPost, cg);
    }
}

TR::Register *OMR::ARM64::RegisterDependencyConditions::searchPreConditionRegister(TR::RealRegister::RegNum rr)
{
    return (_preConditions == NULL ? NULL : _preConditions->searchForRegister(rr, _addCursorForPre));
}

TR::Register *OMR::ARM64::RegisterDependencyConditions::searchPostConditionRegister(TR::RealRegister::RegNum rr)
{
    return (_postConditions == NULL ? NULL : _postConditions->searchForRegister(rr, _addCursorForPost));
}

uint32_t OMR::ARM64::RegisterDependencyConditions::setNumPreConditions(uint16_t n, TR_Memory *m)
{
    if (_preConditions == NULL) {
        _preConditions = new (n, m) TR::RegisterDependencyGroup;
    }
    if (_addCursorForPre > n) {
        _addCursorForPre = n;
    }
    return (_numPreConditions = n);
}

uint32_t OMR::ARM64::RegisterDependencyConditions::setNumPostConditions(uint16_t n, TR_Memory *m)
{
    if (_postConditions == NULL) {
        _postConditions = new (n, m) TR::RegisterDependencyGroup;
    }
    if (_addCursorForPost > n) {
        _addCursorForPost = n;
    }
    return (_numPostConditions = n);
}

bool OMR::ARM64::RegisterDependencyConditions::refsRegister(TR::Register *r)
{
    for (uint16_t i = 0; i < _addCursorForPre; i++) {
        if (_preConditions->getRegisterDependency(i)->getRegister() == r
            && _preConditions->getRegisterDependency(i)->getRefsRegister()) {
            return true;
        }
    }
    for (uint16_t j = 0; j < _addCursorForPost; j++) {
        if (_postConditions->getRegisterDependency(j)->getRegister() == r
            && _postConditions->getRegisterDependency(j)->getRefsRegister()) {
            return true;
        }
    }
    return false;
}

bool OMR::ARM64::RegisterDependencyConditions::defsRegister(TR::Register *r)
{
    for (uint16_t i = 0; i < _addCursorForPre; i++) {
        if (_preConditions->getRegisterDependency(i)->getRegister() == r
            && _preConditions->getRegisterDependency(i)->getDefsRegister()) {
            return true;
        }
    }
    for (uint16_t j = 0; j < _addCursorForPost; j++) {
        if (_postConditions->getRegisterDependency(j)->getRegister() == r
            && _postConditions->getRegisterDependency(j)->getDefsRegister()) {
            return true;
        }
    }
    return false;
}

bool OMR::ARM64::RegisterDependencyConditions::defsRealRegister(TR::Register *r)
{
    for (uint16_t i = 0; i < _addCursorForPre; i++) {
        if (_preConditions->getRegisterDependency(i)->getRegister()->getAssignedRegister() == r
            && _preConditions->getRegisterDependency(i)->getDefsRegister()) {
            return true;
        }
    }
    for (uint16_t j = 0; j < _addCursorForPost; j++) {
        if (_postConditions->getRegisterDependency(j)->getRegister()->getAssignedRegister() == r
            && _postConditions->getRegisterDependency(j)->getDefsRegister()) {
            return true;
        }
    }
    return false;
}

bool OMR::ARM64::RegisterDependencyConditions::usesRegister(TR::Register *r)
{
    for (uint16_t i = 0; i < _addCursorForPre; i++) {
        if (_preConditions->getRegisterDependency(i)->getRegister() == r
            && _preConditions->getRegisterDependency(i)->getUsesRegister()) {
            return true;
        }
    }
    for (uint16_t j = 0; j < _addCursorForPost; j++) {
        if (_postConditions->getRegisterDependency(j)->getRegister() == r
            && _postConditions->getRegisterDependency(j)->getUsesRegister()) {
            return true;
        }
    }
    return false;
}

void OMR::ARM64::RegisterDependencyConditions::bookKeepingRegisterUses(TR::Instruction *instr, TR::CodeGenerator *cg)
{
    if (instr->getOpCodeValue() == TR::InstOpCode::assocreg)
        return;

    // We create a register association directive for each dependency
    bool isOOL = cg->isOutOfLineColdPath();
    if (cg->enableRegisterAssociations() && !isOOL) {
        TR::Machine *machine = cg->machine();

        machine->createRegisterAssociationDirective(instr->getPrev());

        // Now set up the new register associations as required by the current
        // dependent register instruction onto the machine.
        // Only the registers that this instruction interferes with are modified.
        //
        for (int i = 0; i < _addCursorForPre; i++) {
            TR::RegisterDependency *dependency = _preConditions->getRegisterDependency(i);
            if (dependency->getRegister()) {
                machine->setVirtualAssociatedWithReal(dependency->getRealRegister(), dependency->getRegister());
            }
        }

        for (int j = 0; j < _addCursorForPost; j++) {
            TR::RegisterDependency *dependency = _postConditions->getRegisterDependency(j);
            if (dependency->getRegister()) {
                machine->setVirtualAssociatedWithReal(dependency->getRealRegister(), dependency->getRegister());
            }
        }
    }

    for (int i = 0; i < _addCursorForPre; i++) {
        auto dependency = _preConditions->getRegisterDependency(i);
        TR::Register *virtReg = dependency->getRegister();
        TR::RealRegister::RegNum regNum = dependency->getRealRegister();
        instr->useRegister(virtReg, true);
        if (!isOOL)
            cg->setRealRegisterAssociation(virtReg, regNum);
    }
    for (int j = 0; j < _addCursorForPost; j++) {
        auto dependency = _postConditions->getRegisterDependency(j);
        TR::Register *virtReg = dependency->getRegister();
        TR::RealRegister::RegNum regNum = dependency->getRealRegister();
        instr->useRegister(virtReg, true);
        if (!isOOL)
            cg->setRealRegisterAssociation(virtReg, regNum);
    }
}

TR::RegisterDependencyConditions *OMR::ARM64::RegisterDependencyConditions::clone(TR::CodeGenerator *cg,
    TR::RegisterDependencyConditions *added, bool omitPre, bool omitPost)
{
    int preNum = omitPre ? 0 : getAddCursorForPre();
    int postNum = omitPost ? 0 : getAddCursorForPost();
    int addPreNum = 0;
    int addPostNum = 0;

    if (added != NULL) {
        addPreNum = omitPre ? 0 : added->getAddCursorForPre();
        addPostNum = omitPost ? 0 : added->getAddCursorForPost();
    }

    TR::RegisterDependencyConditions *result = new (cg->trHeapMemory())
        TR::RegisterDependencyConditions(preNum + addPreNum, postNum + addPostNum, cg->trMemory());
    for (int i = 0; i < preNum; i++) {
        auto singlePair = getPreConditions()->getRegisterDependency(i);
        result->addPreCondition(singlePair->getRegister(), singlePair->getRealRegister(), singlePair->getFlags());
    }

    for (int i = 0; i < postNum; i++) {
        auto singlePair = getPostConditions()->getRegisterDependency(i);
        result->addPostCondition(singlePair->getRegister(), singlePair->getRealRegister(), singlePair->getFlags());
    }

    for (int i = 0; i < addPreNum; i++) {
        auto singlePair = added->getPreConditions()->getRegisterDependency(i);
        result->addPreCondition(singlePair->getRegister(), singlePair->getRealRegister(), singlePair->getFlags());
    }

    for (int i = 0; i < addPostNum; i++) {
        auto singlePair = added->getPostConditions()->getRegisterDependency(i);
        result->addPostCondition(singlePair->getRegister(), singlePair->getRealRegister(), singlePair->getFlags());
    }

    return result;
}

void OMR::ARM64::RegisterDependencyConditions::stopUsingDepRegs(TR::CodeGenerator *cg, TR::Register *returnRegister)
{
    if (_preConditions != NULL)
        _preConditions->stopUsingDepRegs(getAddCursorForPre(), NULL, returnRegister, cg);
    if (_postConditions != NULL)
        _postConditions->stopUsingDepRegs(getAddCursorForPost(), NULL, returnRegister, cg);
}

void OMR::ARM64::RegisterDependencyConditions::stopUsingDepRegs(TR::CodeGenerator *cg, int numRetReg,
    TR::Register **retReg)
{
    if (_preConditions != NULL)
        _preConditions->stopUsingDepRegs(getAddCursorForPre(), numRetReg, retReg, cg);
    if (_postConditions != NULL)
        _postConditions->stopUsingDepRegs(getAddCursorForPost(), numRetReg, retReg, cg);
}

/**
 * @brief Answers if the register dependency is KillVectorRegs
 *
 * @param[in] dep : register dependency
 * @return true if the register dependency is KillVectorRegs
 */
static bool killsVectorRegs(TR::RegisterDependency *dep)
{
    return dep->getRealRegister() == TR::RealRegister::KillVectorRegs;
}

void OMR::ARM64::RegisterDependencyGroup::assignRegisters(TR::Instruction *currentInstruction,
    TR_RegisterKinds kindToBeAssigned, uint32_t numberOfRegisters, TR::CodeGenerator *cg)
{
    TR::Compilation *comp = cg->comp();
    TR::Machine *machine = cg->machine();
    TR::Register *virtReg;
    TR::RealRegister::RegNum dependentRegNum;
    TR::RealRegister *dependentRealReg, *assignedRegister;
    uint32_t i, j;
    bool changed;

    for (i = 0; i < numberOfRegisters; i++) {
        virtReg = _dependencies[i].getRegister();
        if (_dependencies[i].isSpilledReg()) {
            TR_ASSERT(virtReg->getBackingStorage(), "should have a backing store if SpilledReg");
            if (virtReg->getAssignedRealRegister()) {
                // this happens when the register was first spilled in main line path then was reverse spilled
                // and assigned to a real register in OOL path. We protected the backing store when doing
                // the reverse spill so we could re-spill to the same slot now
                traceMsg(comp, "\nOOL: Found register spilled in main line and re-assigned inside OOL");
                TR::Node *currentNode = currentInstruction->getNode();
                TR::RealRegister *assignedReg = toRealRegister(virtReg->getAssignedRegister());
                TR::MemoryReference *tempMR = TR::MemoryReference::createWithSymRef(cg, currentNode,
                    (TR::SymbolReference *)virtReg->getBackingStorage()->getSymbolReference());
                TR_RegisterKinds rk = virtReg->getKind();
                TR::InstOpCode::Mnemonic opCode;
                switch (rk) {
                    case TR_GPR:
                        opCode = TR::InstOpCode::ldrimmx;
                        break;
                    case TR_FPR:
                        opCode = TR::InstOpCode::vldrimmd;
                        break;
                    case TR_VRF:
                        opCode = TR::InstOpCode::vldrimmq;
                        break;
                    default:
                        TR_ASSERT(0, "\nRegister kind not supported in OOL spill");
                        break;
                }

                TR::Instruction *inst
                    = generateTrg1MemInstruction(cg, opCode, currentNode, assignedReg, tempMR, currentInstruction);

                assignedReg->setAssignedRegister(NULL);
                virtReg->setAssignedRegister(NULL);
                assignedReg->setState(TR::RealRegister::Free);

                if (comp->getDebug())
                    cg->traceRegisterAssignment("Generate reload of virt %s due to spillRegIndex dep at inst %p\n",
                        cg->comp()->getDebug()->getName(virtReg), currentInstruction);
                cg->traceRAInstruction(inst);
            }

            if (!(std::find(cg->getSpilledRegisterList()->begin(), cg->getSpilledRegisterList()->end(), virtReg)
                    != cg->getSpilledRegisterList()->end()))
                cg->getSpilledRegisterList()->push_front(virtReg);
        }
        // we also need to free up all locked backing storage if we are exiting the OOL during backwards RA assignment
        else if (currentInstruction->isLabel() && virtReg->getAssignedRealRegister()) {
            TR::ARM64LabelInstruction *labelInstr = (TR::ARM64LabelInstruction *)currentInstruction;
            TR_BackingStore *location = virtReg->getBackingStorage();
            TR_RegisterKinds rk = virtReg->getKind();
            int32_t dataSize;
            if (labelInstr->getLabelSymbol()->isStartOfColdInstructionStream() && location) {
                traceMsg(comp, "\nOOL: Releasing backing storage (%p)\n", location);
                if (rk == TR_GPR)
                    dataSize = TR::Compiler->om.sizeofReferenceAddress();
                else if (rk == TR_FPR)
                    dataSize = 8;
                else /* TR_VRF */
                    dataSize = 16;
                location->setMaxSpillDepth(0);
                cg->freeSpill(location, dataSize, 0);
                virtReg->setBackingStorage(NULL);
            }
        }
    }

    for (i = 0; i < numberOfRegisters; i++) {
        virtReg = _dependencies[i].getRegister();

        if (virtReg->getAssignedRealRegister() != NULL) {
            if (_dependencies[i].isNoReg()) {
                virtReg->block();
            } else {
                dependentRegNum = toRealRegister(virtReg->getAssignedRealRegister())->getRegisterNumber();
                for (j = 0; j < numberOfRegisters; j++) {
                    if (dependentRegNum == _dependencies[j].getRealRegister()) {
                        virtReg->block();
                        break;
                    }
                }
            }
        }
        // Check for directive to spill vector registers.
        if (killsVectorRegs(&_dependencies[i])) {
            machine->spillAllVectorRegisters(currentInstruction);
        }
    }

    do {
        changed = false;
        for (i = 0; i < numberOfRegisters; i++) {
            TR::RegisterDependency &regDep = _dependencies[i];
            virtReg = regDep.getRegister();
            dependentRegNum = regDep.getRealRegister();
            dependentRealReg = machine->getRealRegister(dependentRegNum);

            if (!regDep.isNoReg() && !regDep.isSpilledReg() && !killsVectorRegs(&regDep)
                && dependentRealReg->getState() == TR::RealRegister::Free) {
                machine->coerceRegisterAssignment(currentInstruction, virtReg, dependentRegNum);
                virtReg->block();
                changed = true;
            }
        }
    } while (changed == true);

    do {
        changed = false;
        for (i = 0; i < numberOfRegisters; i++) {
            TR::RegisterDependency &regDep = _dependencies[i];
            virtReg = regDep.getRegister();
            assignedRegister = NULL;
            if (virtReg->getAssignedRealRegister() != NULL) {
                assignedRegister = toRealRegister(virtReg->getAssignedRealRegister());
            }
            dependentRegNum = regDep.getRealRegister();
            dependentRealReg = machine->getRealRegister(dependentRegNum);
            if (!regDep.isNoReg() && !regDep.isSpilledReg() && !killsVectorRegs(&regDep)
                && dependentRealReg != assignedRegister) {
                machine->coerceRegisterAssignment(currentInstruction, virtReg, dependentRegNum);
                virtReg->block();
                changed = true;
            }
        }
    } while (changed == true);

    for (i = 0; i < numberOfRegisters; i++) {
        if (_dependencies[i].isNoReg()) {
            TR::RealRegister *realOne;

            virtReg = _dependencies[i].getRegister();
            realOne = virtReg->getAssignedRealRegister();
            if (realOne == NULL) {
                if (virtReg->getTotalUseCount() == virtReg->getFutureUseCount()) {
                    if ((assignedRegister
                            = machine->findBestFreeRegister(currentInstruction, virtReg->getKind(), true, virtReg))
                        == NULL) {
                        assignedRegister = machine->freeBestRegister(currentInstruction, virtReg, NULL);
                    }
                } else {
                    assignedRegister = machine->reverseSpillState(currentInstruction, virtReg, NULL);
                }
                virtReg->setAssignedRegister(assignedRegister);
                assignedRegister->setAssignedRegister(virtReg);
                assignedRegister->setState(TR::RealRegister::Assigned);
                virtReg->block();
            }
        }
    }

    unblockRegisters(numberOfRegisters);
    for (i = 0; i < numberOfRegisters; i++) {
        TR::RegisterDependency *regDep = getRegisterDependency(i);
        TR::Register *dependentRegister = regDep->getRegister();
        if (dependentRegister->getAssignedRegister()) {
            TR::RealRegister *assignedRegister = dependentRegister->getAssignedRegister()->getRealRegister();

            if (regDep->isNoReg())
                regDep->setRealRegister(toRealRegister(assignedRegister)->getRegisterNumber());

            machine->decFutureUseCountAndUnlatch(currentInstruction, dependentRegister);
        } else if (regDep->isSpilledReg()) {
            machine->decFutureUseCountAndUnlatch(currentInstruction, dependentRegister);
        }
    }
}

void OMR::ARM64::RegisterDependencyConditions::stopUsingDepRegs(TR::CodeGenerator *cg, TR::Register *ret1,
    TR::Register *ret2)
{
    if (_preConditions != NULL)
        _preConditions->stopUsingDepRegs(_addCursorForPre, ret1, ret2, cg);
    if (_postConditions != NULL)
        _postConditions->stopUsingDepRegs(_addCursorForPost, ret1, ret2, cg);
}

void TR_ARM64ScratchRegisterDependencyConditions::addDependency(TR::CodeGenerator *cg, TR::Register *vr,
    TR::RealRegister::RegNum rr, uint8_t flag)
{
    TR_ASSERT_FATAL(_numGPRDeps < TR::RealRegister::LastAssignableGPR - TR::RealRegister::FirstGPR + 1,
        "Too many GPR dependencies");

    bool isGPR = rr <= TR::RealRegister::LastAssignableGPR;
    TR_ASSERT_FATAL(isGPR, "Expecting GPR only");
    if (!vr) {
        vr = cg->allocateRegister(TR_GPR);
        cg->stopUsingRegister(vr);
    }
    _gprDeps[_numGPRDeps].setRegister(vr);
    _gprDeps[_numGPRDeps].assignFlags(flag);
    _gprDeps[_numGPRDeps].setRealRegister(rr);
    ++_numGPRDeps;
}

void TR_ARM64ScratchRegisterDependencyConditions::unionDependency(TR::CodeGenerator *cg, TR::Register *vr,
    TR::RealRegister::RegNum rr, uint8_t flag)
{
    TR_ASSERT_FATAL(_numGPRDeps < TR::RealRegister::LastAssignableGPR - TR::RealRegister::FirstGPR + 1,
        "Too many GPR dependencies");

    bool isGPR = rr <= TR::RealRegister::LastAssignableGPR;
    TR_ASSERT_FATAL(isGPR, "Expecting GPR only");
    TR_ASSERT_FATAL(vr, "Expecting non-null virtual register");

    for (int i = 0; i < _numGPRDeps; i++) {
        if (_gprDeps[i].getRegister() == vr) {
            // Keep the stronger of the two constraints
            //
            TR::RealRegister::RegNum min = std::min(rr, _gprDeps[i].getRealRegister());
            TR::RealRegister::RegNum max = std::max(rr, _gprDeps[i].getRealRegister());
            if (min == TR::RealRegister::NoReg) {
                // Anything is stronger than NoReg
                _gprDeps[i].setRegister(vr);
                _gprDeps[i].assignFlags(flag);
                _gprDeps[i].setRealRegister(max);
            } else {
                TR_ASSERT_FATAL(min == max, "Specific register dependency is only compatible with itself");
                // Nothing to do
            }
            return;
        }
    }

    // vr is not in the deps list, add a new one
    _gprDeps[_numGPRDeps].setRegister(vr);
    _gprDeps[_numGPRDeps].assignFlags(flag);
    _gprDeps[_numGPRDeps].setRealRegister(rr);
    ++_numGPRDeps;
}

void TR_ARM64ScratchRegisterDependencyConditions::addScratchRegisters(TR::CodeGenerator *cg,
    TR_ARM64ScratchRegisterManager *srm)
{
    auto list = srm->getManagedScratchRegisterList();
    ListIterator<TR_ManagedScratchRegister> iterator(&list);
    TR_ManagedScratchRegister *msr = iterator.getFirst();

    while (msr) {
        unionDependency(cg, msr->_reg, TR::RealRegister::NoReg);
        msr = iterator.getNext();
    }
}

TR::RegisterDependencyConditions *TR_ARM64ScratchRegisterDependencyConditions::createDependencyConditions(
    TR::CodeGenerator *cg, TR_ARM64ScratchRegisterDependencyConditions *pre,
    TR_ARM64ScratchRegisterDependencyConditions *post)
{
    int32_t preCount = pre ? pre->getNumberOfDependencies() : 0;
    int32_t postCount = post ? post->getNumberOfDependencies() : 0;

    TR::RegisterDependencyConditions *dependencies
        = new (cg->trHeapMemory()) TR::RegisterDependencyConditions(preCount, postCount, cg->trMemory());
    for (int i = 0; i < (pre ? pre->_numGPRDeps : 0); ++i) {
        dependencies->addPreCondition(pre->_gprDeps[i].getRegister(), pre->_gprDeps[i].getRealRegister(),
            pre->_gprDeps[i].getFlags());
    }
    for (int i = 0; i < (post ? post->_numGPRDeps : 0); ++i) {
        dependencies->addPostCondition(post->_gprDeps[i].getRegister(), post->_gprDeps[i].getRealRegister(),
            post->_gprDeps[i].getFlags());
    }

    return dependencies;
}
