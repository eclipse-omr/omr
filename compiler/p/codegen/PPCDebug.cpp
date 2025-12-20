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

#ifndef TR_TARGET_POWER
int jitDebugPPC;
#else

#include <stdint.h>
#include <string.h>
#include "codegen/CodeGenPhase.hpp"
#include "codegen/CodeGenerator.hpp"
#include "env/FrontEnd.hpp"
#include "codegen/GCRegisterMap.hpp"
#include "codegen/InstOpCode.hpp"
#include "codegen/Instruction.hpp"
#include "codegen/Linkage.hpp"
#include "codegen/Machine.hpp"
#include "codegen/MemoryReference.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/Register.hpp"
#include "codegen/RegisterConstants.hpp"
#include "codegen/RegisterDependency.hpp"
#include "codegen/RegisterDependencyStruct.hpp"
#include "codegen/UnresolvedDataSnippet.hpp"
#include "compile/Compilation.hpp"
#include "control/Options.hpp"
#include "control/Options_inlines.hpp"
#include "env/CompilerEnv.hpp"
#include "env/IO.hpp"
#include "env/jittypes.h"
#include "il/Block.hpp"
#include "il/DataTypes.hpp"
#include "il/ILOpCodes.hpp"
#include "il/LabelSymbol.hpp"
#include "il/Node.hpp"
#include "il/Symbol.hpp"
#include "il/SymbolReference.hpp"
#include "infra/Assert.hpp"
#include "infra/List.hpp"
#include "p/codegen/PPCHelperCallSnippet.hpp"
#include "p/codegen/PPCInstruction.hpp"
#include "p/codegen/PPCOpsDefines.hpp"
#include "p/codegen/PPCOutOfLineCodeSection.hpp"
#include "codegen/Snippet.hpp"
#include "ras/Debug.hpp"
#include "ras/Logger.hpp"
#include "runtime/CodeCacheManager.hpp"
#include "runtime/Runtime.hpp"

#ifdef J9_PROJECT_SPECIFIC
#include "p/codegen/CallSnippet.hpp"
#include "p/codegen/InterfaceCastSnippet.hpp"
#include "p/codegen/StackCheckFailureSnippet.hpp"
#endif

namespace TR {
class PPCForceRecompilationSnippet;
class PPCRecompilationSnippet;
} // namespace TR

const char *TR_Debug::getOpCodeName(TR::InstOpCode *opCode)
{
    return TR::InstOpCode::metadata[opCode->getOpCodeValue()].name;
}

void TR_Debug::printMemoryReferenceComment(OMR::Logger *log, TR::MemoryReference *mr)
{
    TR::Symbol *symbol = mr->getSymbolReference()->getSymbol();

    if (symbol == NULL && mr->getSymbolReference()->getOffset() == 0)
        return;

    if (symbol && symbol->isSpillTempAuto()) {
        const char *prefix = (symbol->getDataType() == TR::Float || symbol->getDataType() == TR::Double) ? "#FP" : "#";

        log->printf(" # %sSPILL%d", prefix, symbol->getSize());
    }

    log->prints("\t\t# SymRef");
    print(log, mr->getSymbolReference());
}

void TR_Debug::print(OMR::Logger *log, TR::Instruction *instr)
{
    switch (instr->getKind()) {
        case OMR::Instruction::IsAlignmentNop:
            print(log, (TR::PPCAlignmentNopInstruction *)instr);
            break;
        case OMR::Instruction::IsImm:
            print(log, (TR::PPCImmInstruction *)instr);
            break;
        case OMR::Instruction::IsImm2:
            print(log, (TR::PPCImm2Instruction *)instr);
            break;
        case OMR::Instruction::IsSrc1:
            print(log, (TR::PPCSrc1Instruction *)instr);
            break;
        case OMR::Instruction::IsDep:
            print(log, (TR::PPCDepInstruction *)instr);
            break;
        case OMR::Instruction::IsDepImmSym:
            print(log, (TR::PPCDepImmSymInstruction *)instr);
            break;
        case OMR::Instruction::IsLabel:
            print(log, (TR::PPCLabelInstruction *)instr);
            break;
        case OMR::Instruction::IsDepLabel:
            print(log, (TR::PPCDepLabelInstruction *)instr);
            break;
#ifdef J9_PROJECT_SPECIFIC
        case OMR::Instruction::IsVirtualGuardNOP:
            print(log, (TR::PPCVirtualGuardNOPInstruction *)instr);
            break;
#endif
        case OMR::Instruction::IsConditionalBranch:
            print(log, (TR::PPCConditionalBranchInstruction *)instr);
            break;
        case OMR::Instruction::IsDepConditionalBranch:
            print(log, (TR::PPCDepConditionalBranchInstruction *)instr);
            break;
        case OMR::Instruction::IsAdmin:
            print(log, (TR::PPCAdminInstruction *)instr);
            break;
        case OMR::Instruction::IsTrg1:
            print(log, (TR::PPCTrg1Instruction *)instr);
            break;
        case OMR::Instruction::IsTrg1Imm:
            print(log, (TR::PPCTrg1ImmInstruction *)instr);
            break;
        case OMR::Instruction::IsTrg1Src1:
            print(log, (TR::PPCTrg1Src1Instruction *)instr);
            break;
        case OMR::Instruction::IsTrg1Src1Imm:
            print(log, (TR::PPCTrg1Src1ImmInstruction *)instr);
            break;
        case OMR::Instruction::IsTrg1Src1Imm2:
            print(log, (TR::PPCTrg1Src1Imm2Instruction *)instr);
            break;
        case OMR::Instruction::IsTrg1Src2:
            print(log, (TR::PPCTrg1Src2Instruction *)instr);
            break;
        case OMR::Instruction::IsTrg1Src2Imm:
            print(log, (TR::PPCTrg1Src2ImmInstruction *)instr);
            break;
        case OMR::Instruction::IsTrg1Src3:
            print(log, (TR::PPCTrg1Src3Instruction *)instr);
            break;
        case OMR::Instruction::IsTrg1Mem:
            print(log, (TR::PPCTrg1MemInstruction *)instr);
            break;
        case OMR::Instruction::IsSrc2:
            print(log, (TR::PPCSrc2Instruction *)instr);
            break;
        case OMR::Instruction::IsSrc3:
            print(log, (TR::PPCSrc3Instruction *)instr);
            break;
        case OMR::Instruction::IsMem:
            print(log, (TR::PPCMemInstruction *)instr);
            break;
        case OMR::Instruction::IsMemSrc1:
            print(log, (TR::PPCMemSrc1Instruction *)instr);
            break;
        case OMR::Instruction::IsControlFlow:
            print(log, (TR::PPCControlFlowInstruction *)instr);
            break;
        default:
            TR_ASSERT(0, "unexpected instruction kind");
            // fall thru
        case OMR::Instruction::IsNotExtended: {
            printPrefix(log, instr);
            log->prints(getOpCodeName(&instr->getOpCode()));
            log->flush();
        }
    }
}

void TR_Debug::printInstructionComment(OMR::Logger *log, int32_t tabStops, TR::Instruction *instr)
{
    while (tabStops-- > 0)
        log->printc('\t');

    dumpInstructionComments(log, instr);
}

void TR_Debug::printPrefix(OMR::Logger *log, TR::Instruction *instr)
{
    printPrefix(log, instr, instr->getBinaryEncoding(), instr->getBinaryLength());
    if (instr->getNode()) {
        log->printf("%d \t", instr->getNode()->getByteCodeIndex());
    } else
        log->prints("0 \t");
}

void TR_Debug::print(OMR::Logger *log, TR::PPCAlignmentNopInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s\t; Align to %u bytes", getOpCodeName(&instr->getOpCode()), instr->getAlignment());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCDepInstruction *instr)
{
    printPrefix(log, instr);
    log->prints(getOpCodeName(&instr->getOpCode()));
    if (instr->getDependencyConditions())
        print(log, instr->getDependencyConditions());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCLabelInstruction *instr)
{
    printPrefix(log, instr);

    TR::LabelSymbol *label = instr->getLabelSymbol();
    TR::Snippet *snippet = label ? label->getSnippet() : NULL;
    if (instr->getOpCodeValue() == TR::InstOpCode::label) {
        print(log, label);
        log->printc(':');
        if (label->isStartInternalControlFlow())
            log->prints("\t; (Start of internal control flow)");
        else if (label->isEndInternalControlFlow())
            log->prints("\t; (End of internal control flow)");
    } else {
        log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
        print(log, label);
        if (snippet) {
            TR::SymbolReference *callSym = NULL;
            switch (snippet->getKind()) {
#ifdef J9_PROJECT_SPECIFIC
                case TR::Snippet::IsCall:
                case TR::Snippet::IsUnresolvedCall:
                    callSym = ((TR::PPCCallSnippet *)snippet)->getNode()->getSymbolReference();
                    break;
                case TR::Snippet::IsVirtual:
                case TR::Snippet::IsVirtualUnresolved:
                case TR::Snippet::IsInterfaceCall:
                    callSym = ((TR::PPCCallSnippet *)snippet)->getNode()->getSymbolReference();
                    break;
#endif
                case TR::Snippet::IsHelperCall:
                case TR::Snippet::IsMonitorEnter:
                case TR::Snippet::IsMonitorExit:
                case TR::Snippet::IsReadMonitor:
                case TR::Snippet::IsLockReservationEnter:
                case TR::Snippet::IsLockReservationExit:
                case TR::Snippet::IsArrayCopyCall:
                    callSym = ((TR::PPCHelperCallSnippet *)snippet)->getDestination();
                    break;
                default:
                    break;
            }
            if (callSym)
                log->printf("\t; Call \"%s\"", getName(callSym));
        }
    }
    printInstructionComment(log, 1, instr);

    if (instr->getDependencyConditions())
        print(log, instr->getDependencyConditions());

    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCDepLabelInstruction *instr)
{
    print(log, (TR::PPCLabelInstruction *)instr);
    if (instr->getDependencyConditions())
        print(log, instr->getDependencyConditions());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCConditionalBranchInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getConditionRegister(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getLabelSymbol());
    if (instr->haveHint())
        log->printf(" # with prediction hints: %s", instr->getLikeliness() ? "Likely" : "Unlikely");
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCDepConditionalBranchInstruction *instr)
{
    print(log, (TR::PPCConditionalBranchInstruction *)instr);
    if (instr->getDependencyConditions())
        print(log, instr->getDependencyConditions());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCAdminInstruction *instr)
{
    int i;
    printPrefix(log, instr);
    log->printf("%s ", getOpCodeName(&instr->getOpCode()));

    if (instr->getOpCodeValue() == TR::InstOpCode::fence && instr->getFenceNode() != NULL) {
        log->printf("\t%s[",
            (instr->getFenceNode()->getRelocationType() == TR_AbsoluteAddress) ? "Absolute" : "Relative");
        for (i = 0; i < instr->getFenceNode()->getNumRelocations(); i++) {
            log->printf(" %08x", instr->getFenceNode()->getRelocationDestination(i));
        }
        log->prints(" ]");
    }

    TR::Node *node = instr->getNode();
    if (node && node->getOpCodeValue() == TR::BBStart) {
        TR::Block *block = node->getBlock();
        log->printf(" (BBStart (block_%d))", block->getNumber());
    } else if (node && node->getOpCodeValue() == TR::BBEnd) {
        log->printf(" (BBEnd (block_%d))", node->getBlock()->getNumber());
    }

    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCImmInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t0x%08x", getOpCodeName(&instr->getOpCode()), instr->getSourceImmediate());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCSrc1Instruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getSource1Register(), TR_WordReg);
    if (!(instr->getOpCodeValue() == TR::InstOpCode::mtlr || instr->getOpCodeValue() == TR::InstOpCode::mtctr))
        log->printf(", " POINTER_PRINTF_FORMAT, (intptr_t)(int32_t)instr->getSourceImmediate());

    log->flush();
}

// Returns true if the given call symbol reference refers to a method address
// that is outside of the range of legal branch offsets. Sets distance to the
// branch distance (either to the method directly or to the trampoline) as a
// side effect.
bool TR_Debug::isBranchToTrampoline(TR::SymbolReference *symRef, uint8_t *cursor, int32_t &distance)
{
    intptr_t methodAddress = (intptr_t)(symRef->getMethodAddress());
    bool requiresTrampoline = false;

    if (_cg->directCallRequiresTrampoline(methodAddress, (intptr_t)cursor)) {
        methodAddress
            = TR::CodeCacheManager::instance()->findHelperTrampoline(symRef->getReferenceNumber(), (void *)cursor);
        requiresTrampoline = true;
    }

    distance = (int32_t)(methodAddress - (intptr_t)cursor);
    return requiresTrampoline;
}

void TR_Debug::print(OMR::Logger *log, TR::PPCDepImmSymInstruction *instr)
{
    intptr_t targetAddress = instr->getAddrImmediate();
    uint8_t *cursor = instr->getBinaryEncoding();
    TR::Symbol *target = instr->getSymbolReference()->getSymbol();
    TR::LabelSymbol *label = target->getLabelSymbol();

    printPrefix(log, instr);

    if (cursor) {
        if (label) {
            targetAddress = (intptr_t)label->getCodeLocation();
        } else if (targetAddress == 0) {
            targetAddress = _cg->getLinkage()->entryPointFromCompiledMethod();
        } else if (_cg->directCallRequiresTrampoline(targetAddress, (intptr_t)cursor)) {
            int32_t refNum = instr->getSymbolReference()->getReferenceNumber();
            if (refNum < TR_PPCnumRuntimeHelpers) {
                targetAddress = TR::CodeCacheManager::instance()->findHelperTrampoline(refNum, (void *)cursor);
            } else {
                targetAddress = _comp->fe()->methodTrampolineLookup(_comp, instr->getSymbolReference(), (void *)cursor);
            }
        }
    }

    intptr_t distance = targetAddress - (intptr_t)cursor;

    const char *name = target ? getName(instr->getSymbolReference()) : 0;
    if (name)
        log->printf("%s \t" POINTER_PRINTF_FORMAT "\t\t; Direct Call \"%s\"", getOpCodeName(&instr->getOpCode()),
            (intptr_t)cursor + distance, name);
    else
        log->printf("%s \t" POINTER_PRINTF_FORMAT, getOpCodeName(&instr->getOpCode()), (intptr_t)cursor + distance);

    if (instr->getDependencyConditions())
        print(log, instr->getDependencyConditions());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCTrg1Instruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getTargetRegister(), TR_WordReg);
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCTrg1Src1Instruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));

    // Use the VSR name instead of the FPR/VR
    bool isVSX = instr->getOpCode().isVSX();
    if (instr->getTargetRegister()->getRealRegister())
        toRealRegister(instr->getTargetRegister())->setUseVSR(isVSX);
    if (instr->getSource1Register()->getRealRegister())
        toRealRegister(instr->getSource1Register())->setUseVSR(isVSX);

    print(log, instr->getTargetRegister(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource1Register(), TR_WordReg);
    if (strcmp("ori", getOpCodeName(&instr->getOpCode())) == 0)
        log->prints(", 0x0");
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCTrg1ImmInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));

    print(log, instr->getTargetRegister(), TR_WordReg);
    log->printf(", " POINTER_PRINTF_FORMAT, (intptr_t)(int32_t)instr->getSourceImmediate());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCTrg1Src1ImmInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getTargetRegister(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource1Register(), TR_WordReg);
    TR::InstOpCode::Mnemonic op = instr->getOpCodeValue();

    log->printf(", "
                "%d",
        (intptr_t)(int32_t)instr->getSourceImmediate());

    if (instr->getDependencyConditions())
        print(log, instr->getDependencyConditions());

    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCTrg1Src1Imm2Instruction *instr)
{
    uint64_t lmask;
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getTargetRegister(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource1Register(), TR_WordReg);

    lmask = instr->getLongMask();
    if (instr->cg()->comp()->target().is64Bit())
        log->printf(", " POINTER_PRINTF_FORMAT ", " POINTER_PRINTF_FORMAT, instr->getSourceImmediate(), lmask);
    else
        log->printf(", " POINTER_PRINTF_FORMAT ", 0x%x", instr->getSourceImmediate(), (uint32_t)lmask);

    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCSrc2Instruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getSource1Register(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource2Register(), TR_WordReg);
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCSrc3Instruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getSource1Register(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource2Register(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource3Register(), TR_WordReg);
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCTrg1Src2Instruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));

    // Use the VSR name instead of the FPR/VR
    bool isVSX = instr->getOpCode().isVSX();
    if (instr->getTargetRegister()->getRealRegister())
        toRealRegister(instr->getTargetRegister())->setUseVSR(isVSX);
    if (instr->getSource1Register()->getRealRegister())
        toRealRegister(instr->getSource1Register())->setUseVSR(isVSX);
    if (instr->getSource2Register()->getRealRegister())
        toRealRegister(instr->getSource2Register())->setUseVSR(isVSX);

    print(log, instr->getTargetRegister(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource1Register(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource2Register(), TR_WordReg);
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCTrg1Src2ImmInstruction *instr)
{
    uint64_t lmask;
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getTargetRegister(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource1Register(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource2Register(), TR_WordReg);
    lmask = instr->getLongMask();
    if (instr->cg()->comp()->target().is64Bit())
        log->printf(", " POINTER_PRINTF_FORMAT, lmask);
    else
        log->printf(", 0x%x", (uint32_t)lmask);
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCTrg1Src3Instruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getTargetRegister(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource1Register(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource2Register(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource3Register(), TR_WordReg);
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCMemSrc1Instruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));

    // Use the VSR name instead of the FPR/VR
    if (instr->getSourceRegister()->getRealRegister())
        toRealRegister(instr->getSourceRegister())->setUseVSR(instr->getOpCode().isVSX());

    print(log, instr->getMemoryReference());
    log->prints(", ");
    print(log, instr->getSourceRegister(), TR_WordReg);

    printMemoryReferenceComment(log, instr->getMemoryReference());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCMemInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getMemoryReference());
    printMemoryReferenceComment(log, instr->getMemoryReference());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCTrg1MemInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));

    // Use the VSR name instead of the FPR/VR
    if (instr->getTargetRegister()->getRealRegister())
        toRealRegister(instr->getTargetRegister())->setUseVSR(instr->getOpCode().isVSX());

    print(log, instr->getTargetRegister(), TR_WordReg);
    log->prints(", ");

    print(log, instr->getMemoryReference(), strncmp("addi", getOpCodeName(&instr->getOpCode()), 4) != 0);
    TR::Symbol *symbol = instr->getMemoryReference()->getSymbolReference()->getSymbol();
    if (symbol && symbol->isSpillTempAuto()) {
        log->printf("\t\t; spilled for %s", getName(instr->getNode()->getOpCode()));
    }
    if (instr->getSnippetForGC() != NULL) {
        log->printf("\t\t; Backpatched branch to Unresolved Data %s",
            getName(instr->getSnippetForGC()->getSnippetLabel()));
    }

    if (instr->haveHint()) {
        int32_t hint = instr->getHint();
        if (hint == PPCOpProp_LoadReserveAtomicUpdate)
            log->prints(" # with hint: Reserve Atomic Update (Default)");
        if (hint == PPCOpProp_LoadReserveExclusiveAccess)
            log->prints(" # with hint: Exclusive Access");
    }

    printMemoryReferenceComment(log, instr->getMemoryReference());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::PPCControlFlowInstruction *instr)
{
    int i;
    printPrefix(log, instr);

    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    int numTargets = instr->getNumTargets();
    int numSources = instr->getNumSources();
    for (i = 0; i < numTargets; i++) {
        print(log, instr->getTargetRegister(i), TR_WordReg);
        if (i != numTargets + numSources - 1)
            log->prints(", ");
    }
    for (i = 0; i < numSources; i++) {
        if (instr->isSourceImmediate(i))
            log->printf("0x%08x", instr->getSourceImmediate(i));
        else
            print(log, instr->getSourceRegister(i), TR_WordReg);
        if (i != numSources - 1)
            log->prints(", ");
    }
    if (instr->getOpCode2Value() != TR::InstOpCode::bad) {
        log->printf(", %s", getOpCodeName(&instr->getOpCode2()));
    }
    if (instr->getLabelSymbol() != NULL) {
        log->prints(", ");
        print(log, instr->getLabelSymbol());
    }
    log->flush();
}

#ifdef J9_PROJECT_SPECIFIC
void TR_Debug::print(OMR::Logger *log, TR::PPCVirtualGuardNOPInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s Site:" POINTER_PRINTF_FORMAT ", ", getOpCodeName(&instr->getOpCode()), instr->getSite());
    print(log, instr->getLabelSymbol());
    if (instr->getDependencyConditions())
        print(log, instr->getDependencyConditions());
    log->flush();
}
#endif

void TR_Debug::print(OMR::Logger *log, TR::RegisterDependency *dep)
{
    TR::Machine *machine = _cg->machine();
    log->printc('[');
    print(log, dep->getRegister(), TR_WordReg);
    log->prints(" : ");
    log->printf("%s] ", getPPCRegisterName(dep->getRealRegister()));
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::RegisterDependencyConditions *conditions)
{
    if (conditions) {
        int i;
        log->prints("\n PRE: ");
        for (i = 0; i < conditions->getAddCursorForPre(); i++) {
            print(log, conditions->getPreConditions()->getRegisterDependency(i));
        }
        log->prints("\nPOST: ");
        for (i = 0; i < conditions->getAddCursorForPost(); i++) {
            print(log, conditions->getPostConditions()->getRegisterDependency(i));
        }
        log->flush();
    }
}

void TR_Debug::print(OMR::Logger *log, TR::MemoryReference *mr, bool d_form)
{
    log->printc('[');

    if (mr->getBaseRegister() != NULL) {
        print(log, mr->getBaseRegister());
        log->prints(", ");
    } else if (mr->getLabel() != NULL) {
        print(log, mr->getLabel());
        log->prints(", ");
    }

    if (mr->getIndexRegister() != NULL)
        print(log, mr->getIndexRegister());
    else
        log->printf("%lld", mr->getOffset(*_comp));

    log->printc(']');
}

void TR_Debug::printPPCGCRegisterMap(OMR::Logger *log, TR::GCRegisterMap *map)
{
    TR::Machine *machine = _cg->machine();

    log->prints("    registers: {");
    for (int i = 31; i >= 0; i--) {
        if (map->getMap() & (1 << i))
            log->printf("%s ",
                getName(machine->getRealRegister((TR::RealRegister::RegNum)(31 - i + TR::RealRegister::FirstGPR))));
    }

    log->prints("}\n");
}

void TR_Debug::print(OMR::Logger *log, TR::RealRegister *reg, TR_RegisterSizes size)
{
    log->prints(getName(reg, size));
}

static const char *getRegisterName(TR::RealRegister::RegNum num, bool isVSR = false)
{
    switch (num) {
        case TR::RealRegister::gr0:
            return "gr0";
        case TR::RealRegister::gr1:
            return "gr1";
        case TR::RealRegister::gr2:
            return "gr2";
        case TR::RealRegister::gr3:
            return "gr3";
        case TR::RealRegister::gr4:
            return "gr4";
        case TR::RealRegister::gr5:
            return "gr5";
        case TR::RealRegister::gr6:
            return "gr6";
        case TR::RealRegister::gr7:
            return "gr7";
        case TR::RealRegister::gr8:
            return "gr8";
        case TR::RealRegister::gr9:
            return "gr9";
        case TR::RealRegister::gr10:
            return "gr10";
        case TR::RealRegister::gr11:
            return "gr11";
        case TR::RealRegister::gr12:
            return "gr12";
        case TR::RealRegister::gr13:
            return "gr13";
        case TR::RealRegister::gr14:
            return "gr14";
        case TR::RealRegister::gr15:
            return "gr15";
        case TR::RealRegister::gr16:
            return "gr16";
        case TR::RealRegister::gr17:
            return "gr17";
        case TR::RealRegister::gr18:
            return "gr18";
        case TR::RealRegister::gr19:
            return "gr19";
        case TR::RealRegister::gr20:
            return "gr20";
        case TR::RealRegister::gr21:
            return "gr21";
        case TR::RealRegister::gr22:
            return "gr22";
        case TR::RealRegister::gr23:
            return "gr23";
        case TR::RealRegister::gr24:
            return "gr24";
        case TR::RealRegister::gr25:
            return "gr25";
        case TR::RealRegister::gr26:
            return "gr26";
        case TR::RealRegister::gr27:
            return "gr27";
        case TR::RealRegister::gr28:
            return "gr28";
        case TR::RealRegister::gr29:
            return "gr29";
        case TR::RealRegister::gr30:
            return "gr30";
        case TR::RealRegister::gr31:
            return "gr31";

        case TR::RealRegister::fp0:
            return (isVSR ? "vsr0" : "fp0");
        case TR::RealRegister::fp1:
            return (isVSR ? "vsr1" : "fp1");
        case TR::RealRegister::fp2:
            return (isVSR ? "vsr2" : "fp2");
        case TR::RealRegister::fp3:
            return (isVSR ? "vsr3" : "fp3");
        case TR::RealRegister::fp4:
            return (isVSR ? "vsr4" : "fp4");
        case TR::RealRegister::fp5:
            return (isVSR ? "vsr5" : "fp5");
        case TR::RealRegister::fp6:
            return (isVSR ? "vsr6" : "fp6");
        case TR::RealRegister::fp7:
            return (isVSR ? "vsr7" : "fp7");
        case TR::RealRegister::fp8:
            return (isVSR ? "vsr8" : "fp8");
        case TR::RealRegister::fp9:
            return (isVSR ? "vsr9" : "fp9");
        case TR::RealRegister::fp10:
            return (isVSR ? "vsr10" : "fp10");
        case TR::RealRegister::fp11:
            return (isVSR ? "vsr11" : "fp11");
        case TR::RealRegister::fp12:
            return (isVSR ? "vsr12" : "fp12");
        case TR::RealRegister::fp13:
            return (isVSR ? "vsr13" : "fp13");
        case TR::RealRegister::fp14:
            return (isVSR ? "vsr14" : "fp14");
        case TR::RealRegister::fp15:
            return (isVSR ? "vsr15" : "fp15");
        case TR::RealRegister::fp16:
            return (isVSR ? "vsr16" : "fp16");
        case TR::RealRegister::fp17:
            return (isVSR ? "vsr17" : "fp17");
        case TR::RealRegister::fp18:
            return (isVSR ? "vsr18" : "fp18");
        case TR::RealRegister::fp19:
            return (isVSR ? "vsr19" : "fp19");
        case TR::RealRegister::fp20:
            return (isVSR ? "vsr20" : "fp20");
        case TR::RealRegister::fp21:
            return (isVSR ? "vsr21" : "fp21");
        case TR::RealRegister::fp22:
            return (isVSR ? "vsr22" : "fp22");
        case TR::RealRegister::fp23:
            return (isVSR ? "vsr23" : "fp23");
        case TR::RealRegister::fp24:
            return (isVSR ? "vsr24" : "fp24");
        case TR::RealRegister::fp25:
            return (isVSR ? "vsr25" : "fp25");
        case TR::RealRegister::fp26:
            return (isVSR ? "vsr26" : "fp26");
        case TR::RealRegister::fp27:
            return (isVSR ? "vsr27" : "fp27");
        case TR::RealRegister::fp28:
            return (isVSR ? "vsr28" : "fp28");
        case TR::RealRegister::fp29:
            return (isVSR ? "vsr29" : "fp29");
        case TR::RealRegister::fp30:
            return (isVSR ? "vsr30" : "fp30");
        case TR::RealRegister::fp31:
            return (isVSR ? "vsr31" : "fp31");

        case TR::RealRegister::cr0:
            return "cr0";
        case TR::RealRegister::cr1:
            return "cr1";
        case TR::RealRegister::cr2:
            return "cr2";
        case TR::RealRegister::cr3:
            return "cr3";
        case TR::RealRegister::cr4:
            return "cr4";
        case TR::RealRegister::cr5:
            return "cr5";
        case TR::RealRegister::cr6:
            return "cr6";
        case TR::RealRegister::cr7:
            return "cr7";

        case TR::RealRegister::lr:
            return "lr";

        case TR::RealRegister::vr0:
            return (isVSR ? "vsr32" : "vr0");
        case TR::RealRegister::vr1:
            return (isVSR ? "vsr33" : "vr1");
        case TR::RealRegister::vr2:
            return (isVSR ? "vsr34" : "vr2");
        case TR::RealRegister::vr3:
            return (isVSR ? "vsr35" : "vr3");
        case TR::RealRegister::vr4:
            return (isVSR ? "vsr36" : "vr4");
        case TR::RealRegister::vr5:
            return (isVSR ? "vsr37" : "vr5");
        case TR::RealRegister::vr6:
            return (isVSR ? "vsr38" : "vr6");
        case TR::RealRegister::vr7:
            return (isVSR ? "vsr39" : "vr7");
        case TR::RealRegister::vr8:
            return (isVSR ? "vsr40" : "vr8");
        case TR::RealRegister::vr9:
            return (isVSR ? "vsr41" : "vr9");
        case TR::RealRegister::vr10:
            return (isVSR ? "vsr42" : "vr10");
        case TR::RealRegister::vr11:
            return (isVSR ? "vsr43" : "vr11");
        case TR::RealRegister::vr12:
            return (isVSR ? "vsr44" : "vr12");
        case TR::RealRegister::vr13:
            return (isVSR ? "vsr45" : "vr13");
        case TR::RealRegister::vr14:
            return (isVSR ? "vsr46" : "vr14");
        case TR::RealRegister::vr15:
            return (isVSR ? "vsr47" : "vr15");
        case TR::RealRegister::vr16:
            return (isVSR ? "vsr48" : "vr16");
        case TR::RealRegister::vr17:
            return (isVSR ? "vsr49" : "vr17");
        case TR::RealRegister::vr18:
            return (isVSR ? "vsr50" : "vr18");
        case TR::RealRegister::vr19:
            return (isVSR ? "vsr51" : "vr19");
        case TR::RealRegister::vr20:
            return (isVSR ? "vsr52" : "vr20");
        case TR::RealRegister::vr21:
            return (isVSR ? "vsr53" : "vr21");
        case TR::RealRegister::vr22:
            return (isVSR ? "vsr54" : "vr22");
        case TR::RealRegister::vr23:
            return (isVSR ? "vsr55" : "vr23");
        case TR::RealRegister::vr24:
            return (isVSR ? "vsr56" : "vr24");
        case TR::RealRegister::vr25:
            return (isVSR ? "vsr57" : "vr25");
        case TR::RealRegister::vr26:
            return (isVSR ? "vsr58" : "vr26");
        case TR::RealRegister::vr27:
            return (isVSR ? "vsr59" : "vr27");
        case TR::RealRegister::vr28:
            return (isVSR ? "vsr60" : "vr28");
        case TR::RealRegister::vr29:
            return (isVSR ? "vsr61" : "vr29");
        case TR::RealRegister::vr30:
            return (isVSR ? "vsr62" : "vr30");
        case TR::RealRegister::vr31:
            return (isVSR ? "vsr63" : "vr31");

        default:
            return "???";
    }
}

const char *TR_Debug::getName(TR::RealRegister *reg, TR_RegisterSizes size)
{
    return getRegisterName(reg->getRegisterNumber(), reg->getUseVSR());
}

const char *TR_Debug::getPPCRegisterName(uint32_t regNum, bool useVSR)
{
    return getRegisterName((TR::RealRegister::RegNum)regNum, useVSR);
}

TR::Instruction *TR_Debug::getOutlinedTargetIfAny(TR::Instruction *instr)
{
    switch (instr->getKind()) {
        case TR::Instruction::IsLabel:
        case TR::Instruction::IsDepLabel:
        case TR::Instruction::IsVirtualGuardNOP:
        case TR::Instruction::IsConditionalBranch:
        case TR::Instruction::IsDepConditionalBranch:
            break;
        default:
            return NULL;
    }

    TR::PPCLabelInstruction *labelInstr = (TR::PPCLabelInstruction *)instr;
    TR::LabelSymbol *label = labelInstr->getLabelSymbol();

    if (!label || !label->isStartOfColdInstructionStream())
        return NULL;

    return label->getInstruction();
}

void TR_Debug::printPPCOOLSequences(OMR::Logger *log)
{
    auto oiIterator = _cg->getPPCOutOfLineCodeSectionList().begin();
    while (oiIterator != _cg->getPPCOutOfLineCodeSectionList().end()) {
        log->prints("\n------------ start out-of-line instructions\n");
        TR::Instruction *instr = (*oiIterator)->getFirstInstruction();

        do {
            print(log, instr);
            instr = instr->getNext();
        } while (instr != (*oiIterator)->getAppendInstruction());

        if ((*oiIterator)->getAppendInstruction()) {
            print(log, (*oiIterator)->getAppendInstruction());
        }
        log->prints("\n------------ end out-of-line instructions\n");

        ++oiIterator;
    }
}

void TR_Debug::printp(OMR::Logger *log, TR::Snippet *snippet)
{
    switch (snippet->getKind()) {
#ifdef J9_PROJECT_SPECIFIC
        case TR::Snippet::IsCall:
            print(log, (TR::PPCCallSnippet *)snippet);
            break;
        case TR::Snippet::IsUnresolvedCall:
            print(log, (TR::PPCUnresolvedCallSnippet *)snippet);
            break;
        case TR::Snippet::IsVirtual:
            print(log, (TR::PPCVirtualSnippet *)snippet);
            break;
        case TR::Snippet::IsVirtualUnresolved:
            print(log, (TR::PPCVirtualUnresolvedSnippet *)snippet);
            break;
        case TR::Snippet::IsInterfaceCall:
            print(log, (TR::PPCInterfaceCallSnippet *)snippet);
            break;
        case TR::Snippet::IsInterfaceCastSnippet:
            print(log, (TR::PPCInterfaceCastSnippet *)snippet);
            break;
        case TR::Snippet::IsStackCheckFailure:
            print(log, (TR::PPCStackCheckFailureSnippet *)snippet);
            break;
        case TR::Snippet::IsForceRecompilation:
            print(log, (TR::PPCForceRecompilationSnippet *)snippet);
            break;
        case TR::Snippet::IsRecompilation:
            print(log, (TR::PPCRecompilationSnippet *)snippet);
            break;
#endif
        case TR::Snippet::IsUnresolvedData:
            print(log, (TR::UnresolvedDataSnippet *)snippet);
            break;

        case TR::Snippet::IsHeapAlloc:
        case TR::Snippet::IsMonitorExit:
        case TR::Snippet::IsMonitorEnter:
        case TR::Snippet::IsReadMonitor:
        case TR::Snippet::IsLockReservationEnter:
        case TR::Snippet::IsLockReservationExit:
        case TR::Snippet::IsAllocPrefetch:
        case TR::Snippet::IsNonZeroAllocPrefetch:
        case TR::Snippet::IsHelperCall:
        case TR::Snippet::IsArrayCopyCall:
            snippet->print(log, this);
            break;
        default:
            TR_ASSERT(0, "unexpected snippet kind");
    }
}

#endif
