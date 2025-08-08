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

#include <algorithm>
#include <stdint.h>
#include <string.h>
#include "codegen/BackingStore.hpp"
#include "codegen/CodeGenerator.hpp"
#include "codegen/Instruction.hpp"
#include "codegen/LiveRegister.hpp"
#include "codegen/Machine.hpp"
#include "codegen/MemoryReference.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/Register.hpp"
#include "codegen/RegisterConstants.hpp"
#include "codegen/RegisterDependency.hpp"
#include "codegen/RegisterDependencyStruct.hpp"
#include "codegen/RegisterPair.hpp"
#include "compile/Compilation.hpp"
#include "control/Options.hpp"
#include "control/Options_inlines.hpp"
#include "env/TRMemory.hpp"
#include "il/Block.hpp"
#include "il/DataTypes.hpp"
#include "il/ILOpCodes.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "infra/Assert.hpp"
#include "infra/List.hpp"
#include "ras/Debug.hpp"
#include "ras/DebugCounter.hpp"
#include "x/codegen/X86Instruction.hpp"
#include "codegen/InstOpCode.hpp"
#include "x/codegen/X86Register.hpp"

////////////////////////
// Hack markers
//

// TODO:AMD64: IA32 FPR GRA currently interferes XMM GRA in a somewhat
// confusing way that doesn't occur on AMD64.
#define XMM_GPRS_USE_DISTINCT_NUMBERS(cg) (cg->comp()->target().is64Bit())

static void generateRegcopyDebugCounter(TR::CodeGenerator *cg, const char *category)
{
    if (!cg->comp()->getOptions()->enableDebugCounters())
        return;
    TR::TreeTop *tt = cg->getCurrentEvaluationTreeTop();
    const char *fullName = TR::DebugCounter::debugCounterName(cg->comp(),
        "regcopy/cg.genRegDepConditions/%s/(%s)/%s/block_%d", category, cg->comp()->signature(),
        cg->comp()->getHotnessName(cg->comp()->getMethodHotness()), tt->getEnclosingBlock()->getNumber());
    TR::Instruction *cursor = cg->lastInstructionBeforeCurrentEvaluationTreeTop();
    cg->generateDebugCounter(cursor, fullName, 1, TR::DebugCounter::Undetermined);
}

OMR::X86::RegisterDependencyConditions::RegisterDependencyConditions(uint16_t numPreConds, uint16_t numPostConds,
    TR_Memory *m)
    : _preConditions(new(numPreConds, m) TR::RegisterDependencyGroup)
    , _postConditions(new(numPostConds, m) TR::RegisterDependencyGroup)
    , _numPreConditions(numPreConds)
    , _addCursorForPre(0)
    , _numPostConditions(numPostConds)
    , _addCursorForPost(0)
{}

OMR::X86::RegisterDependencyConditions::RegisterDependencyConditions(TR::Node *node, TR::CodeGenerator *cg,
    uint16_t additionalRegDeps)
    : _numPreConditions(-1)
    , _numPostConditions(-1)
    , _addCursorForPre(0)
    , _addCursorForPost(0)
{
    TR::Register *copyReg = NULL;
    TR::Register *highCopyReg = NULL;
    List<TR::Register> registers(cg->trMemory());
    TR::Machine *machine = cg->machine();
    TR::Compilation *comp = cg->comp();

    int32_t numLongs = 0;

    for (int32_t i = 0; i < node->getNumChildren(); ++i) {
        TR::Node *child = node->getChild(i);

        if (child->getRegister()->getKind() == TR_GPR && child->getHighGlobalRegisterNumber() > -1) {
            numLongs++;
        }
    }

    _preConditions
        = new (node->getNumChildren() + additionalRegDeps + numLongs, cg->trMemory()) TR::RegisterDependencyGroup;
    _postConditions
        = new (node->getNumChildren() + additionalRegDeps + numLongs, cg->trMemory()) TR::RegisterDependencyGroup;
    _numPreConditions = node->getNumChildren() + additionalRegDeps + numLongs;
    _numPostConditions = node->getNumChildren() + additionalRegDeps + numLongs;

    int32_t numLongsAdded = 0;

    for (int32_t i = node->getNumChildren() - 1; i >= 0; i--) {
        TR::Node *child = node->getChild(i);
        TR::Register *globalReg = child->getRegister();
        TR::Register *highGlobalReg = NULL;
        TR_GlobalRegisterNumber globalRegNum = child->getGlobalRegisterNumber();
        TR_GlobalRegisterNumber highGlobalRegNum = child->getHighGlobalRegisterNumber();

        TR::RealRegister::RegNum realRegNum = TR::RealRegister::NoReg, realHighRegNum = TR::RealRegister::NoReg;
        if (globalReg->getKind() == TR_GPR || globalReg->getKind() == TR_VRF || XMM_GPRS_USE_DISTINCT_NUMBERS(cg)) {
            realRegNum = (TR::RealRegister::RegNum)cg->getGlobalRegister(globalRegNum);

            if (highGlobalRegNum > -1)
                realHighRegNum = (TR::RealRegister::RegNum)cg->getGlobalRegister(highGlobalRegNum);
        } else if (globalReg->getKind() == TR_FPR) {
            // Convert the global register number from an st0-based value to an xmm0-based one.
            //
            realRegNum = (TR::RealRegister::RegNum)(
                cg->getGlobalRegister(globalRegNum) - TR::RealRegister::FirstFPR + TR::RealRegister::FirstXMMR);

            // Find the global register that has been allocated in this XMMR, if any.
            //
            TR::Register *xmmGlobalReg = cg->machine()->getXMMGlobalRegister(realRegNum - TR::RealRegister::FirstXMMR);
            if (xmmGlobalReg)
                globalReg = xmmGlobalReg;
        } else {
            TR_ASSERT_FATAL_WITH_NODE(node, false, "invalid global register kind %d, reg=%p", globalReg->getKind(),
                globalReg);
        }

        if (registers.find(globalReg)) {
            if (globalReg->getKind() == TR_GPR) {
                bool containsInternalPointer = false;
                TR::RegisterPair *globalRegPair = globalReg->getRegisterPair();
                if (globalRegPair) {
                    highGlobalReg = globalRegPair->getHighOrder();
                    globalReg = globalRegPair->getLowOrder();
                } else if (child->getHighGlobalRegisterNumber() > -1)
                    TR_ASSERT(0, "Long child does not have a register pair\n");

                if (globalReg->getPinningArrayPointer())
                    containsInternalPointer = true;

                copyReg = (globalReg->containsCollectedReference() && !containsInternalPointer)
                    ? cg->allocateCollectedReferenceRegister()
                    : cg->allocateRegister();

                if (containsInternalPointer) {
                    copyReg->setContainsInternalPointer();
                    copyReg->setPinningArrayPointer(globalReg->getPinningArrayPointer());
                }

                TR::Instruction *prevInstr = cg->getAppendInstruction();
                TR::Instruction *placeToAdd = prevInstr;
                if (prevInstr && prevInstr->getOpCode().isFusableCompare()) {
                    TR::Instruction *prevPrevInstr = prevInstr->getPrev();
                    if (prevPrevInstr) {
                        if (comp->getOption(TR_TraceCG))
                            traceMsg(comp, "Moving reg reg copy earlier (after %p) in %s\n", prevPrevInstr,
                                comp->signature());
                        placeToAdd = prevPrevInstr;
                    }
                }

                generateRegRegInstruction(placeToAdd, TR::InstOpCode::MOVRegReg(), copyReg, globalReg, cg);

                if (highGlobalReg) {
                    generateRegcopyDebugCounter(cg, "gpr-pair");
                    containsInternalPointer = false;
                    if (highGlobalReg->getPinningArrayPointer())
                        containsInternalPointer = true;

                    highCopyReg = (highGlobalReg->containsCollectedReference() && !containsInternalPointer)
                        ? cg->allocateCollectedReferenceRegister()
                        : cg->allocateRegister();

                    if (containsInternalPointer) {
                        highCopyReg->setContainsInternalPointer();
                        highCopyReg->setPinningArrayPointer(highGlobalReg->getPinningArrayPointer());
                    }

                    generateRegRegInstruction(TR::InstOpCode::MOV4RegReg, node, highCopyReg, highGlobalReg, cg);
                } else {
                    generateRegcopyDebugCounter(cg, "gpr");
                    highCopyReg = NULL;
                }
            } else if (globalReg->getKind() == TR_FPR) {
                generateRegcopyDebugCounter(cg, "fpr");
                if (globalReg->isSinglePrecision()) {
                    copyReg = cg->allocateSinglePrecisionRegister(TR_FPR);
                    generateRegRegInstruction(TR::InstOpCode::MOVAPSRegReg, node, copyReg, child->getRegister(), cg);
                } else {
                    copyReg = cg->allocateRegister(TR_FPR);
                    generateRegRegInstruction(TR::InstOpCode::MOVAPDRegReg, node, copyReg, child->getRegister(), cg);
                }
            } else if (globalReg->getKind() == TR_VRF) {
                generateRegcopyDebugCounter(cg, "vrf");
                copyReg = cg->allocateRegister(TR_VRF);
                TR::InstOpCode::Mnemonic op
                    = cg->comp()->target().cpu.supportsAVX() ? InstOpCode::VMOVDQUYmmYmm : TR::InstOpCode::MOVDQURegReg;
                op = cg->comp()->target().cpu.supportsFeature(OMR_FEATURE_X86_AVX512F) ? InstOpCode::VMOVDQUZmmZmm : op;
                generateRegRegInstruction(op, node, copyReg, child->getRegister(), cg);
            } else if (globalReg->getKind() == TR_VMR) {
                generateRegcopyDebugCounter(cg, "vmr");
                copyReg = cg->allocateRegister(TR_VMR);
                TR::InstOpCode::Mnemonic op = cg->comp()->target().cpu.supportsFeature(OMR_FEATURE_X86_AVX512BW)
                    ? InstOpCode::KMOVQMaskMask
                    : TR::InstOpCode::KMOVWMaskMask;
                generateRegRegInstruction(op, node, copyReg, child->getRegister(), cg);
            }

            globalReg = copyReg;
            highGlobalReg = highCopyReg;
        } else {
            registers.add(globalReg);
            TR::RegisterPair *globalRegPair = globalReg->getRegisterPair();
            if (globalRegPair) {
                highGlobalReg = globalRegPair->getHighOrder();
                globalReg = globalRegPair->getLowOrder();
            } else if (child->getHighGlobalRegisterNumber() > -1)
                TR_ASSERT(0, "Long child does not have a register pair\n");
        }

        if (globalReg->getKind() == TR_GPR) {
            addPreCondition(globalReg, realRegNum, cg);
            addPostCondition(globalReg, realRegNum, cg);
            if (highGlobalReg) {
                numLongsAdded++;
                addPreCondition(highGlobalReg, realHighRegNum, cg);
                addPostCondition(highGlobalReg, realHighRegNum, cg);
            }
        } else if (globalReg->getKind() == TR_FPR || globalReg->getKind() == TR_VRF) {
            addPreCondition(globalReg, realRegNum, cg);
            addPostCondition(globalReg, realRegNum, cg);
            cg->machine()->setXMMGlobalRegister(realRegNum - TR::RealRegister::FirstXMMR, globalReg);
        }

        // If the register dependency isn't actually used
        // (dead store elimination probably removed it)
        // and this is a float then we have to pop it off the stack.
        //
        if (child->getOpCodeValue() == TR::PassThrough) {
            child = child->getFirstChild();
        }

        if (copyReg) {
            cg->stopUsingRegister(copyReg);
            if (highCopyReg)
                cg->stopUsingRegister(highCopyReg);
        }
    }

    if (numLongs != numLongsAdded)
        TR_ASSERT(0, "Mismatch numLongs %d numLongsAdded %d\n", numLongs, numLongsAdded);
}

uint32_t OMR::X86::RegisterDependencyConditions::unionDependencies(TR::RegisterDependencyGroup *deps, uint32_t cursor,
    TR::Register *vr, TR::RealRegister::RegNum rr, TR::CodeGenerator *cg, uint8_t flag, bool isAssocRegDependency)
{
    TR::Machine *machine = cg->machine();

    // If vr is already in deps, combine the existing association with rr.
    //
    if (vr) {
        if (vr->getRealRegister()) {
            TR::RealRegister::RegNum vrIndex = toRealRegister(vr)->getRegisterNumber();
            switch (vrIndex) {
                case TR::RealRegister::esp:
                case TR::RealRegister::vfp:
                    // esp and vfp are ok, and are ignorable, since local RA already
                    // deals with those without help from regdeps.
                    //
                    //
                    TR_ASSERT(rr == vrIndex || rr == TR::RealRegister::NoReg,
                        "esp and vfp can be assigned only to themselves or NoReg, not %s",
                        cg->getDebug()->getRealRegisterName(rr));
                    break;
                default:
                    TR_ASSERT(0, "Unexpected real register %s in regdep",
                        cg->getDebug()->getName(vr, TR_UnknownSizeReg));
                    break;
            }
            return cursor;
        }

        for (uint32_t candidate = 0; candidate < cursor; candidate++) {
            TR::RegisterDependency *dep = deps->getRegisterDependency(candidate);
            if (dep->getRegister() == vr) {
                // Keep the stronger of the two constraints
                //
                TR::RealRegister::RegNum min = std::min(rr, dep->getRealRegister());
                TR::RealRegister::RegNum max = std::max(rr, dep->getRealRegister());
                if (min == TR::RealRegister::NoReg) {
                    // Anything is stronger than NoReg
                    deps->setDependencyInfo(candidate, vr, max, cg, flag, isAssocRegDependency);
                } else if (max == TR::RealRegister::ByteReg) {
                    // Specific regs are stronger than ByteReg
                    TR_ASSERT(min <= TR::RealRegister::Last8BitGPR,
                        "Only byte registers are compatible with ByteReg dependency");
                    deps->setDependencyInfo(candidate, vr, min, cg, flag, isAssocRegDependency);
                } else {
                    // This assume fires for some assocRegs instructions
                    // TR_ASSERT(min == max, "Specific register dependency is only compatible with itself");
                    if (min != max)
                        continue;

                    // Nothing to do
                }

                return cursor;
            }
        }
    }

    // vr is not already in deps, so add a new dep
    //
    deps->setDependencyInfo(cursor++, vr, rr, cg, flag, isAssocRegDependency);
    return cursor;
}

void OMR::X86::RegisterDependencyConditions::unionNoRegPostCondition(TR::Register *reg, TR::CodeGenerator *cg)
{
    unionPostCondition(reg, TR::RealRegister::NoReg, cg);
}

uint32_t OMR::X86::RegisterDependencyConditions::unionRealDependencies(TR::RegisterDependencyGroup *deps,
    uint32_t cursor, TR::Register *vr, TR::RealRegister::RegNum rr, TR::CodeGenerator *cg, uint8_t flag,
    bool isAssocRegDependency)
{
    TR::Machine *machine = cg->machine();
    // A vmThread/ebp dependency can be ousted by any other ebp dependency.
    // This situation should only occur when ebp is assigned as a global register.
    static TR::RealRegister::RegNum vmThreadRealRegisterIndex = TR::RealRegister::ebp;
    if (rr == vmThreadRealRegisterIndex) {
        uint16_t candidate;
        TR::Register *vmThreadRegister = cg->getVMThreadRegister();
        for (candidate = 0; candidate < cursor; candidate++) {
            TR::RegisterDependency *dep = deps->getRegisterDependency(candidate);
            // Check for a pre-existing ebp dependency.
            if (dep->getRealRegister() == vmThreadRealRegisterIndex) {
                // Any ebp dependency is stronger than a vmThread/ebp dependency.
                // Oust any vmThread dependency.
                if (dep->getRegister() == vmThreadRegister) {
                    // diagnostic("\nEnvicting virt reg %s dep for %s replaced with virt reg %s\n      {\"%s\"}",
                    //    getDebug()->getName(dep->getRegister()),getDebug()->getName(machine->getRealRegister(rr)),getDebug()->getName(vr),
                    //    cg->comp()->getCurrentMethod()->signature());
                    deps->setDependencyInfo(candidate, vr, rr, cg, flag, isAssocRegDependency);
                } else {
                    // diagnostic("\nSkipping virt reg %s dep for %s in favour of %s\n     {%s}}\n",
                    //    getDebug()->getName(vr),getDebug()->getName(machine->getRealRegister(rr)),getDebug()->getName(dep->getRegister()),
                    //    cg->comp()->getCurrentMethod()->signature());
                    TR_ASSERT(vr == vmThreadRegister, "Conflicting EBP register dependencies.\n");
                }
                return cursor;
            }
        }
    }

    // Not handled above, so add a new dependency.
    //
    deps->setDependencyInfo(cursor++, vr, rr, cg, flag, isAssocRegDependency);
    return cursor;
}

TR::RegisterDependencyConditions *OMR::X86::RegisterDependencyConditions::clone(TR::CodeGenerator *cg,
    uint32_t additionalRegDeps)
{
    TR::RegisterDependencyConditions *other = new (cg->trHeapMemory()) TR::RegisterDependencyConditions(
        _numPreConditions + additionalRegDeps, _numPostConditions + additionalRegDeps, cg->trMemory());
    for (int32_t i = _numPreConditions - 1; i >= 0; --i) {
        TR::RegisterDependency *dep = getPreConditions()->getRegisterDependency(i);
        other->getPreConditions()->setDependencyInfo(i, dep->getRegister(), dep->getRealRegister(), cg,
            dep->getFlags());
    }

    for (int32_t i = _numPostConditions - 1; i >= 0; --i) {
        TR::RegisterDependency *dep = getPostConditions()->getRegisterDependency(i);
        other->getPostConditions()->setDependencyInfo(i, dep->getRegister(), dep->getRealRegister(), cg,
            dep->getFlags());
    }

    other->setAddCursorForPre(_addCursorForPre);
    other->setAddCursorForPost(_addCursorForPost);
    return other;
}

uint32_t OMR::X86::RegisterDependencyConditions::setNumPreConditions(uint32_t n, TR_Memory *m)
{
    if (_preConditions == NULL) {
        _preConditions = new (n, m) TR::RegisterDependencyGroup;
    }
    return (_numPreConditions = n);
}

uint32_t OMR::X86::RegisterDependencyConditions::setNumPostConditions(uint32_t n, TR_Memory *m)
{
    if (_postConditions == NULL) {
        _postConditions = new (n, m) TR::RegisterDependencyGroup;
    }
    return (_numPostConditions = n);
}

TR::RegisterDependency *OMR::X86::RegisterDependencyConditions::findPreCondition(TR::Register *vr)
{
    return _preConditions->findDependency(vr, _addCursorForPre);
}

TR::RegisterDependency *OMR::X86::RegisterDependencyConditions::findPostCondition(TR::Register *vr)
{
    return _postConditions->findDependency(vr, _addCursorForPost);
}

TR::RegisterDependency *OMR::X86::RegisterDependencyConditions::findPreCondition(TR::RealRegister::RegNum rr)
{
    return _preConditions->findDependency(rr, _addCursorForPre);
}

TR::RegisterDependency *OMR::X86::RegisterDependencyConditions::findPostCondition(TR::RealRegister::RegNum rr)
{
    return _postConditions->findDependency(rr, _addCursorForPost);
}

void OMR::X86::RegisterDependencyConditions::assignPreConditionRegisters(TR::Instruction *currentInstruction,
    TR_RegisterKinds kindsToBeAssigned, TR::CodeGenerator *cg)
{
    if (_preConditions != NULL) {
        cg->clearRegisterAssignmentFlags();
        cg->setRegisterAssignmentFlag(TR_PreDependencyCoercion);
        _preConditions->assignRegisters(currentInstruction, kindsToBeAssigned, _numPreConditions, cg);
    }
}

void OMR::X86::RegisterDependencyConditions::assignPostConditionRegisters(TR::Instruction *currentInstruction,
    TR_RegisterKinds kindsToBeAssigned, TR::CodeGenerator *cg)
{
    if (_postConditions != NULL) {
        cg->clearRegisterAssignmentFlags();
        cg->setRegisterAssignmentFlag(TR_PostDependencyCoercion);
        _postConditions->assignRegisters(currentInstruction, kindsToBeAssigned, _numPostConditions, cg);
    }
}

void OMR::X86::RegisterDependencyConditions::blockPreConditionRegisters()
{
    _preConditions->blockRegisters(_numPreConditions);
}

void OMR::X86::RegisterDependencyConditions::unblockPreConditionRegisters()
{
    _preConditions->unblockRegisters(_numPreConditions);
}

void OMR::X86::RegisterDependencyConditions::blockPostConditionRegisters()
{
    _postConditions->blockRegisters(_numPostConditions);
}

void OMR::X86::RegisterDependencyConditions::unblockPostConditionRegisters()
{
    _postConditions->unblockRegisters(_numPostConditions);
}

void OMR::X86::RegisterDependencyConditions::blockPostConditionRealDependencyRegisters(TR::CodeGenerator *cg)
{
    _postConditions->blockRealDependencyRegisters(_numPostConditions, cg);
}

void OMR::X86::RegisterDependencyConditions::unblockPostConditionRealDependencyRegisters(TR::CodeGenerator *cg)
{
    _postConditions->unblockRealDependencyRegisters(_numPostConditions, cg);
}

void OMR::X86::RegisterDependencyConditions::blockPreConditionRealDependencyRegisters(TR::CodeGenerator *cg)
{
    _preConditions->blockRealDependencyRegisters(_numPreConditions, cg);
}

void OMR::X86::RegisterDependencyConditions::unblockPreConditionRealDependencyRegisters(TR::CodeGenerator *cg)
{
    _preConditions->unblockRealDependencyRegisters(_numPreConditions, cg);
}

bool OMR::X86::RegisterDependencyConditions::refsRegister(TR::Register *r)
{
    for (int i = 0; i < _numPreConditions; i++) {
        if (_preConditions->getRegisterDependency(i)->getRegister() == r
            && _preConditions->getRegisterDependency(i)->getRefsRegister()) {
            return true;
        }
    }

    for (int j = 0; j < _numPostConditions; j++) {
        if (_postConditions->getRegisterDependency(j)->getRegister() == r
            && _postConditions->getRegisterDependency(j)->getRefsRegister()) {
            return true;
        }
    }

    return false;
}

bool OMR::X86::RegisterDependencyConditions::defsRegister(TR::Register *r)
{
    for (int i = 0; i < _numPreConditions; i++) {
        if (_preConditions->getRegisterDependency(i)->getRegister() == r
            && _preConditions->getRegisterDependency(i)->getDefsRegister()) {
            return true;
        }
    }

    for (int j = 0; j < _numPostConditions; j++) {
        if (_postConditions->getRegisterDependency(j)->getRegister() == r
            && _postConditions->getRegisterDependency(j)->getDefsRegister()) {
            return true;
        }
    }

    return false;
}

bool OMR::X86::RegisterDependencyConditions::usesRegister(TR::Register *r)
{
    for (int i = 0; i < _numPreConditions; i++) {
        if (_preConditions->getRegisterDependency(i)->getRegister() == r
            && _preConditions->getRegisterDependency(i)->getUsesRegister()) {
            return true;
        }
    }

    for (int j = 0; j < _numPostConditions; j++) {
        if (_postConditions->getRegisterDependency(j)->getRegister() == r
            && _postConditions->getRegisterDependency(j)->getUsesRegister()) {
            return true;
        }
    }

    return false;
}

void OMR::X86::RegisterDependencyConditions::useRegisters(TR::Instruction *instr, TR::CodeGenerator *cg)
{
    int32_t i;

    for (i = 0; i < _numPreConditions; i++) {
        TR::Register *virtReg = _preConditions->getRegisterDependency(i)->getRegister();
        if (virtReg) {
            instr->useRegister(virtReg);
        }
    }

    for (i = 0; i < _numPostConditions; i++) {
        TR::Register *virtReg = _postConditions->getRegisterDependency(i)->getRegister();
        if (virtReg) {
            instr->useRegister(virtReg);
        }
    }
}

void OMR::X86::RegisterDependencyGroup::blockRealDependencyRegisters(uint32_t numberOfRegisters, TR::CodeGenerator *cg)
{
    TR::Machine *machine = cg->machine();
    for (uint32_t i = 0; i < numberOfRegisters; i++) {
        if (!_dependencies[i].isNoReg()) {
            machine->getRealRegister(_dependencies[i].getRealRegister())->block();
        }
    }
}

void OMR::X86::RegisterDependencyGroup::unblockRealDependencyRegisters(uint32_t numberOfRegisters,
    TR::CodeGenerator *cg)
{
    TR::Machine *machine = cg->machine();
    for (uint32_t i = 0; i < numberOfRegisters; i++) {
        if (!_dependencies[i].isNoReg()) {
            machine->getRealRegister(_dependencies[i].getRealRegister())->unblock();
        }
    }
}

void OMR::X86::RegisterDependencyGroup::assignRegisters(TR::Instruction *currentInstruction,
    TR_RegisterKinds kindsToBeAssigned, uint32_t numberOfRegisters, TR::CodeGenerator *cg)
{
    TR::Register *virtReg = NULL;
    TR::RealRegister *assignedReg = NULL;
    TR::RealRegister::RegNum dependentRegNum = TR::RealRegister::NoReg;
    TR::RealRegister *dependentRealReg = NULL;
    TR::RealRegister *bestFreeRealReg = NULL;
    TR::RealRegister::RegNum bestFreeRealRegIndex = TR::RealRegister::NoReg;
    bool changed;

    TR::Compilation *comp = cg->comp();

    TR::Machine *machine = cg->machine();
    self()->blockRegisters(numberOfRegisters);

    // Build a work list of assignable register dependencies so the test does not
    // have to be repeated on each pass.  Also, order the list so that real
    // associations appear first, followed by byte reg associations, followed by
    // NoReg associations.
    //
    TR::RegisterDependency **dependencies = (TR::RegisterDependency **)cg->trMemory()->allocateStackMemory(
        numberOfRegisters * sizeof(TR::RegisterDependency *));

    bool hasByteDeps = false;
    bool hasNoRegDeps = false;
    bool hasBestFreeRegDeps = false;
    int32_t numRealAssocRegisters;
    int32_t numDependencyRegisters = 0;
    int32_t firstByteRegAssoc = 0, lastByteRegAssoc = 0;
    int32_t firstNoRegAssoc = 0, lastNoRegAssoc = 0;
    int32_t bestFreeRegAssoc = 0;

    for (auto i = 0U; i < numberOfRegisters; i++) {
        TR::RegisterDependency &regDep = _dependencies[i];
        virtReg = regDep.getRegister();

        if (virtReg && (kindsToBeAssigned & virtReg->getKindAsMask()) && !regDep.isNoReg() && !regDep.isByteReg()
            && !regDep.isSpilledReg() && !regDep.isBestFreeReg()) {
            dependencies[numDependencyRegisters++] = self()->getRegisterDependency(i);
        } else if (regDep.isNoReg())
            hasNoRegDeps = true;
        else if (regDep.isByteReg())
            hasByteDeps = true;
        else if (regDep.isBestFreeReg())
            hasBestFreeRegDeps = true;

        // Handle spill registers first.
        //
        if (regDep.isSpilledReg()) {
            if (cg->getUseNonLinearRegisterAssigner()) {
                if (virtReg->getAssignedRegister()) {
                    TR_ASSERT(virtReg->getBackingStorage(), "should have a backing store for spilled reg virtuals");
                    assignedReg = toRealRegister(virtReg->getAssignedRegister());

                    TR::MemoryReference *tempMR
                        = generateX86MemoryReference(virtReg->getBackingStorage()->getSymbolReference(), cg);

                    TR::InstOpCode::Mnemonic op;
                    if (assignedReg->getKind() == TR_FPR) {
                        op = (assignedReg->isSinglePrecision()) ? TR::InstOpCode::MOVSSRegMem
                                                                : (cg->getXMMDoubleLoadOpCode());
                    } else if (assignedReg->getKind() == TR_VMR) {
                        op = TR::InstOpCode::KMOVWMaskMem;

                        if (cg->comp()->target().cpu.supportsFeature(OMR_FEATURE_X86_AVX512BW)) {
                            op = TR::InstOpCode::KMOVQMaskMem;
                        }
                    } else if (assignedReg->getKind() == TR_VRF) {
                        op = cg->comp()->target().cpu.supportsAVX() ? InstOpCode::VMOVDQUYmmMem
                                                                    : TR::InstOpCode::MOVDQURegMem;
                        op = cg->comp()->target().cpu.supportsFeature(OMR_FEATURE_X86_AVX512F)
                            ? InstOpCode::VMOVDQUZmmMem
                            : op;
                    } else {
                        op = TR::InstOpCode::LRegMem();
                    }

                    TR::X86RegMemInstruction *inst = new (cg->trHeapMemory())
                        TR::X86RegMemInstruction(currentInstruction, op, assignedReg, tempMR, cg);

                    assignedReg->setAssignedRegister(NULL);
                    virtReg->setAssignedRegister(NULL);
                    assignedReg->setState(TR::RealRegister::Free);

                    if (comp->getOption(TR_TraceNonLinearRegisterAssigner)) {
                        cg->traceRegisterAssignment("Generate reload of virt %s due to spillRegIndex dep at inst %p\n",
                            comp->getDebug()->getName(virtReg), currentInstruction);
                        cg->traceRAInstruction(inst);
                    }
                }

                if (!(std::find(cg->getSpilledRegisterList()->begin(), cg->getSpilledRegisterList()->end(), virtReg)
                        != cg->getSpilledRegisterList()->end())) {
                    cg->getSpilledRegisterList()->push_front(virtReg);
                }
            }
        }
    }

    numRealAssocRegisters = numDependencyRegisters;

    if (hasByteDeps) {
        firstByteRegAssoc = numDependencyRegisters;
        for (auto i = 0U; i < numberOfRegisters; i++) {
            virtReg = _dependencies[i].getRegister();
            if (virtReg && (kindsToBeAssigned & virtReg->getKindAsMask()) && _dependencies[i].isByteReg()) {
                dependencies[numDependencyRegisters++] = self()->getRegisterDependency(i);
            }
        }

        lastByteRegAssoc = numDependencyRegisters - 1;
    }

    if (hasNoRegDeps) {
        firstNoRegAssoc = numDependencyRegisters;
        for (auto i = 0U; i < numberOfRegisters; i++) {
            virtReg = _dependencies[i].getRegister();
            if (virtReg && (kindsToBeAssigned & virtReg->getKindAsMask()) && _dependencies[i].isNoReg()) {
                dependencies[numDependencyRegisters++] = self()->getRegisterDependency(i);
            }
        }

        lastNoRegAssoc = numDependencyRegisters - 1;
    }

    if (hasBestFreeRegDeps) {
        bestFreeRegAssoc = numDependencyRegisters;
        for (auto i = 0U; i < numberOfRegisters; i++) {
            virtReg = _dependencies[i].getRegister();
            if (virtReg && (kindsToBeAssigned & virtReg->getKindAsMask()) && _dependencies[i].isBestFreeReg()) {
                dependencies[numDependencyRegisters++] = self()->getRegisterDependency(i);
            }
        }

        TR_ASSERT((bestFreeRegAssoc == numDependencyRegisters - 1),
            "there can only be one bestFreeRegDep in a dependency list");
    }

    // First look for long shot where two registers can be xchg'ed into their
    // proper real registers in one shot (seems to happen more often than one would think)
    //
    if (numRealAssocRegisters > 1) {
        for (auto i = 0; i < numRealAssocRegisters - 1; i++) {
            virtReg = dependencies[i]->getRegister();

            // We can only perform XCHG on GPRs.
            //
            if (virtReg->getKind() == TR_GPR) {
                dependentRegNum = dependencies[i]->getRealRegister();
                dependentRealReg = machine->getRealRegister(dependentRegNum);
                assignedReg = NULL;
                TR::RealRegister::RegNum assignedRegNum = TR::RealRegister::NoReg;
                if (virtReg->getAssignedRegister()) {
                    assignedReg = toRealRegister(virtReg->getAssignedRegister());
                    assignedRegNum = assignedReg->getRegisterNumber();
                }

                if (dependentRealReg->getState() == TR::RealRegister::Blocked && dependentRegNum != assignedRegNum) {
                    for (auto j = i + 1; j < numRealAssocRegisters; j++) {
                        TR::Register *virtReg2 = dependencies[j]->getRegister();
                        TR::RealRegister::RegNum dependentRegNum2 = dependencies[j]->getRealRegister();
                        TR::RealRegister *assignedReg2 = NULL;
                        TR::RealRegister::RegNum assignedRegNum2 = TR::RealRegister::NoReg;
                        if (virtReg2 && virtReg2->getAssignedRegister()) {
                            assignedReg2 = toRealRegister(virtReg2->getAssignedRegister());
                            assignedRegNum2 = assignedReg2->getRegisterNumber();
                        }

                        if (assignedRegNum2 == dependentRegNum && assignedRegNum == dependentRegNum2) {
                            machine->swapGPRegisters(currentInstruction, assignedRegNum, assignedRegNum2);
                            break;
                        }
                    }
                }
            }
        }
    }

    // Next find registers for which there is an identified real register
    // and that register is currently free.  Want to do these first to get these
    // registers out of their existing registers and possibly freeing those registers
    // up to also be filled by the proper candidates as simply as possible
    //
    do {
        changed = false;
        for (auto i = 0; i < numRealAssocRegisters; i++) {
            virtReg = dependencies[i]->getRegister();
            dependentRegNum = dependencies[i]->getRealRegister();
            dependentRealReg = machine->getRealRegister(dependentRegNum);

            if (dependentRealReg->getState() == TR::RealRegister::Free) {
                if (virtReg->getKind() == TR_FPR || virtReg->getKind() == TR_VRF)
                    machine->coerceXMMRegisterAssignment(currentInstruction, virtReg, dependentRegNum);
                else
                    machine->coerceGPRegisterAssignment(currentInstruction, virtReg, dependentRegNum);

                virtReg->block();
                changed = true;
            }
        }
    } while (changed);

    // Next find registers for which there is an identified real register, but the
    // register is not yet assigned.  We could do this at the same time as the previous
    // loop, but that could lead to some naive changes that would have been easier once
    // all the opportunities found by the first loop have been found.
    //
    do {
        changed = false;
        for (auto i = 0; i < numRealAssocRegisters; i++) {
            virtReg = dependencies[i]->getRegister();
            assignedReg = toRealRegister(virtReg->getAssignedRealRegister());
            dependentRegNum = dependencies[i]->getRealRegister();
            dependentRealReg = machine->getRealRegister(dependentRegNum);
            if (dependentRealReg != assignedReg) {
                if (virtReg->getKind() == TR_FPR || virtReg->getKind() == TR_VRF)
                    machine->coerceXMMRegisterAssignment(currentInstruction, virtReg, dependentRegNum);
                else
                    machine->coerceGPRegisterAssignment(currentInstruction, virtReg, dependentRegNum);
                virtReg->block();
                changed = true;
            }
        }
    } while (changed);

    // Now all virtual registers which require a particular real register should
    // be in that register, so now find registers for which there is no required
    // real assignment, but which are not currently assigned to a real register
    // and make sure they are assigned.
    //
    // Assign byte register dependencies first.  Unblock any NoReg dependencies to not
    // constrain the byte register choices.
    //
    if (hasByteDeps) {
        if (hasNoRegDeps) {
            for (auto i = firstNoRegAssoc; i <= lastNoRegAssoc; i++)
                dependencies[i]->getRegister()->unblock();
        }

        do {
            changed = false;
            for (auto i = firstByteRegAssoc; i <= lastByteRegAssoc; i++) {
                virtReg = dependencies[i]->getRegister();
                if (toRealRegister(virtReg->getAssignedRealRegister()) == NULL) {
                    machine->coerceGPRegisterAssignment(currentInstruction, virtReg, TR_ByteReg);
                    virtReg->block();
                    changed = true;
                }
            }
        } while (changed);

        if (hasNoRegDeps) {
            for (auto i = firstNoRegAssoc; i <= lastNoRegAssoc; i++)
                dependencies[i]->getRegister()->block();
        }
    }

    // Assign the remaining NoReg dependencies.
    //
    if (hasNoRegDeps) {
        do {
            changed = false;
            for (auto i = firstNoRegAssoc; i <= lastNoRegAssoc; i++) {
                virtReg = dependencies[i]->getRegister();
                if (toRealRegister(virtReg->getAssignedRealRegister()) == NULL) {
                    if (virtReg->getKind() == TR_FPR || virtReg->getKind() == TR_VRF || virtReg->getKind() == TR_VMR)
                        machine->coerceGPRegisterAssignment(currentInstruction, virtReg, TR_QuadWordReg);
                    else
                        machine->coerceGPRegisterAssignment(currentInstruction, virtReg);

                    virtReg->block();
                    changed = true;
                }
            }
        } while (changed);
    }

    // Recommend the best available register without actually assigning it.
    //
    if (hasBestFreeRegDeps) {
        virtReg = dependencies[bestFreeRegAssoc]->getRegister();
        bestFreeRealReg = machine->findBestFreeGPRegister(currentInstruction, virtReg);
        bestFreeRealRegIndex = bestFreeRealReg ? bestFreeRealReg->getRegisterNumber() : TR::RealRegister::NoReg;
    }

    self()->unblockRegisters(numberOfRegisters);

    for (auto i = 0; i < numDependencyRegisters; i++) {
        TR::Register *virtRegister = dependencies[i]->getRegister();
        assignedReg = toRealRegister(virtRegister->getAssignedRealRegister());

        // Document the registers to which NoReg registers get assigned for use
        // by snippets that need to look back and figure out which virtual
        // registers ended up assigned to which real registers.
        //
        if (dependencies[i]->isNoReg())
            dependencies[i]->setRealRegister(assignedReg->getRegisterNumber());

        if (dependencies[i]->isBestFreeReg()) {
            dependencies[i]->setRealRegister(bestFreeRealRegIndex);
            virtRegister->decFutureUseCount();
            continue;
        }

        if (virtRegister->decFutureUseCount() == 0 && assignedReg->getState() != TR::RealRegister::Locked) {
            virtRegister->setAssignedRegister(NULL);
            assignedReg->setAssignedRegister(NULL);
            assignedReg->setState(TR::RealRegister::Free);
        }
    }

    if (cg->getUseNonLinearRegisterAssigner()) {
        for (auto i = 0U; i < numberOfRegisters; i++) {
            virtReg = _dependencies[i].getRegister();

            if (virtReg && (kindsToBeAssigned & virtReg->getKindAsMask()) && _dependencies[i].isSpilledReg()) {
                virtReg->decFutureUseCount();
            }
        }
    }
}

void OMR::X86::RegisterDependencyGroup::setDependencyInfo(uint32_t index, TR::Register *vr, TR::RealRegister::RegNum rr,
    TR::CodeGenerator *cg, uint8_t flag, bool isAssocRegDependency)
{
    _dependencies[index].setRegister(vr);
    _dependencies[index].assignFlags(flag);
    _dependencies[index].setRealRegister(rr);

    if (vr && vr->isLive() && rr != TR::RealRegister::NoReg && rr != TR::RealRegister::ByteReg) {
        TR::RealRegister *realReg = cg->machine()->getRealRegister(rr);
        if ((vr->getKind() == TR_GPR) && !isAssocRegDependency) {
            // Remember this association so that we can build interference info for
            // other live registers.
            //
            cg->getLiveRegisters(TR_GPR)->setAssociation(vr, realReg);
        }
    }
}

void OMR::X86::RegisterDependencyConditions::createRegisterAssociationDirective(TR::Instruction *instruction,
    TR::CodeGenerator *cg)
{
    TR::Machine *machine = cg->machine();

    machine->createRegisterAssociationDirective(instruction->getPrev());

    // Now set up the new register associations as required by the current
    // dependent register instruction onto the machine.
    // Only the registers that this instruction interferes with are modified.
    //
    TR::RegisterDependencyGroup *depGroup = getPreConditions();
    for (auto j = 0U; j < getNumPreConditions(); j++) {
        TR::RegisterDependency *dependency = depGroup->getRegisterDependency(j);
        if (dependency->getRegister())
            machine->setVirtualAssociatedWithReal(dependency->getRealRegister(), dependency->getRegister());
    }

    depGroup = getPostConditions();
    for (auto k = 0U; k < getNumPostConditions(); k++) {
        TR::RegisterDependency *dependency = depGroup->getRegisterDependency(k);
        if (dependency->getRegister())
            machine->setVirtualAssociatedWithReal(dependency->getRealRegister(), dependency->getRegister());
    }
}

TR::RealRegister *OMR::X86::RegisterDependencyConditions::getRealRegisterFromVirtual(TR::Register *virtReg,
    TR::CodeGenerator *cg)
{
    TR::Machine *machine = cg->machine();

    TR::RegisterDependencyGroup *depGroup = getPostConditions();
    for (auto j = 0U; j < getNumPostConditions(); j++) {
        TR::RegisterDependency *dependency = depGroup->getRegisterDependency(j);

        if (dependency->getRegister() == virtReg) {
            return machine->getRealRegister(dependency->getRealRegister());
        }
    }

    depGroup = getPreConditions();
    for (auto k = 0U; k < getNumPreConditions(); k++) {
        TR::RegisterDependency *dependency = depGroup->getRegisterDependency(k);

        if (dependency->getRegister() == virtReg) {
            return machine->getRealRegister(dependency->getRealRegister());
        }
    }

    TR_ASSERT(0, "getRealRegisterFromVirtual: shouldn't get here");
    return 0;
}

void OMR::X86::RegisterDependencyGroup::assignFPRegisters(TR::Instruction *prevInstruction,
    TR_RegisterKinds kindsToBeAssigned, uint32_t numberOfRegisters, TR::CodeGenerator *cg)
{
    TR::Machine *machine = cg->machine();
    TR::Instruction *cursor = prevInstruction;

    if (numberOfRegisters > 0) {
        for (auto i = 0U; i < numberOfRegisters; i++) {
            TR::Register *virtReg = _dependencies[i].getRegister();
            if (virtReg && kindsToBeAssigned & virtReg->getKindAsMask()) {
                if (((virtReg->getFutureUseCount() != 0)
                        && (virtReg->getTotalUseCount() != virtReg->getFutureUseCount()))
                    && !virtReg->getAssignedRealRegister()) {
                    cursor = machine->reverseFPRSpillState(cursor, virtReg);
                }
            }
        }

        for (auto i = 0U; i < numberOfRegisters; i++) {
            TR::Register *virtReg = _dependencies[i].getRegister();
            if (virtReg && kindsToBeAssigned & virtReg->getKindAsMask()) {
                if (virtReg->getTotalUseCount() != virtReg->getFutureUseCount()) {
                    if (!machine->isFPRTopOfStack(virtReg)) {
                        cursor = machine->fpStackFXCH(cursor, virtReg);
                    }

                    if (virtReg->decFutureUseCount() == 0) {
                        machine->fpStackPop();
                    }
                } else {
                    // If this is the first reference of a register, then this must be the caller
                    // side return value.  Assume it already exists on the FP stack.  The required
                    // stack must be available at this point.
                    //
                    if (virtReg->decFutureUseCount() != 0) {
                        machine->fpStackPush(virtReg);
                    }
                }
            } else if (_dependencies[i].isAllFPRegisters()) {
                // Spill the entire FP stack to memory.
                //
                cursor = machine->fpSpillStack(cursor);
            }
        }
    }
}

#if defined(DEBUG) || defined(PROD_WITH_ASSUMES)
uint32_t OMR::X86::RegisterDependencyConditions::numReferencedFPRegisters(TR::CodeGenerator *cg)
{
    TR::Machine *machine = cg->machine();
    uint32_t total = 0;
    TR::Register *reg;

    for (int32_t i = 0; i < _numPreConditions; i++) {
        reg = _preConditions->getRegisterDependency(i)->getRegister();
        if ((reg && reg->getKind() == TR_X87)
            || (!reg && _preConditions->getRegisterDependency(i)->isAllFPRegisters())) {
            total++;
        }
    }

    for (int32_t i = 0; i < _numPostConditions; i++) {
        reg = _postConditions->getRegisterDependency(i)->getRegister();
        if ((reg && reg->getKind() == TR_X87)
            || (!reg && _postConditions->getRegisterDependency(i)->isAllFPRegisters())) {
            total++;
        }
    }

    return total;
}

uint32_t OMR::X86::RegisterDependencyConditions::numReferencedGPRegisters(TR::CodeGenerator *cg)
{
    uint32_t total = 0;
    TR::Register *reg;

    for (int32_t i = 0; i < _numPreConditions; i++) {
        reg = _preConditions->getRegisterDependency(i)->getRegister();
        if (reg && (reg->getKind() == TR_GPR || reg->getKind() == TR_FPR || reg->getKind() == TR_VRF)) {
            total++;
        }
    }

    for (int32_t i = 0; i < _numPostConditions; i++) {
        reg = _postConditions->getRegisterDependency(i)->getRegister();
        if (reg && (reg->getKind() == TR_GPR || reg->getKind() == TR_FPR || reg->getKind() == TR_VRF)) {
            total++;
        }
    }

    return total;
}

#endif // defined(DEBUG) || defined(PROD_WITH_ASSUMES)

////////////////////////////////////////////////////////////////////////////////
// Generate methods
////////////////////////////////////////////////////////////////////////////////

TR::RegisterDependencyConditions *generateRegisterDependencyConditions(uint32_t numPreConds, uint32_t numPostConds,
    TR::CodeGenerator *cg)
{
    return new (cg->trHeapMemory()) TR::RegisterDependencyConditions(numPreConds, numPostConds, cg->trMemory());
}

TR::RegisterDependencyConditions *generateRegisterDependencyConditions(TR::Node *node, TR::CodeGenerator *cg,
    uint32_t additionalRegDeps, List<TR::Register> *popRegisters)
{
    return new (cg->trHeapMemory()) TR::RegisterDependencyConditions(node, cg, additionalRegDeps);
}
