/*******************************************************************************
 * Copyright IBM Corp. and others 2019
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

#include <stdint.h>
#include <string.h>

#include "ras/Debug.hpp"

#include "codegen/RVInstruction.hpp"
#include "codegen/CodeGenerator.hpp"
#include "codegen/InstOpCode.hpp"
#include "codegen/MemoryReference.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/Register.hpp"
#include "codegen/RegisterConstants.hpp"
#include "codegen/RegisterDependency.hpp"
#include "codegen/RegisterDependencyStruct.hpp"
#include "env/IO.hpp"
#include "il/Block.hpp"
#include "ras/Logger.hpp"
#include "runtime/CodeCacheManager.hpp"

static const char *opCodeToNameMap[] = {
    "assocreg",
    "bad",
    "dd",
    "fence",
    "label",
    "proc",
    "retn",
    "vgnop",
/*
 * RISC-V instructions
 */
#define DECLARE_INSN(mnemonic, match, mask) #mnemonic,
#include <riscv-opc.h>
#undef DECLARE_INSN
};

const char *TR_Debug::getOpCodeName(TR::InstOpCode *opCode) { return opCodeToNameMap[opCode->getMnemonic()]; }

void TR_Debug::printMemoryReferenceComment(OMR::Logger *log, TR::MemoryReference *mr)
{
    TR::Symbol *symbol = mr->getSymbolReference()->getSymbol();
    if (symbol == NULL && mr->getSymbolReference()->getOffset() == 0)
        return;

    log->prints("\t\t# SymRef");
    print(log, mr->getSymbolReference());
}

void TR_Debug::print(OMR::Logger *log, TR::DataInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t0x%08x (%d)", getOpCodeName(&instr->getOpCode()), instr->getSourceImmediate(),
        instr->getSourceImmediate());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::RtypeInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getTargetRegister(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource1Register(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource2Register(), TR_WordReg);
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::ItypeInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getTargetRegister(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource1Register(), TR_WordReg);
    log->printf(", 0x%04x (%d)", instr->getSourceImmediate(), instr->getSourceImmediate());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::StypeInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getSource1Register(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource2Register(), TR_WordReg);
    log->printf(", 0x%04x (%d)", instr->getSourceImmediate(), instr->getSourceImmediate());
    log->flush();
}

void

TR_Debug::print(OMR::Logger *log, TR::BtypeInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getSource1Register(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getSource2Register(), TR_WordReg);
    log->prints(", ");
    print(log, instr->getLabelSymbol());
    log->flush();
}

void

TR_Debug::print(OMR::Logger *log, TR::UtypeInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getTargetRegister(), TR_WordReg);
    log->printf(", 0x%08x (%d)", instr->getSourceImmediate(), instr->getSourceImmediate());
    log->flush();
}

void

TR_Debug::print(OMR::Logger *log, TR::JtypeInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getTargetRegister(), TR_WordReg);
    log->prints(", ");
    if (instr->getLabelSymbol()) {
        print(log, instr->getLabelSymbol());
    } else if (instr->getSymbolReference()) {
        print(log, instr->getSymbolReference());
    } else {
        log->printf("0x%08x (%d)", instr->getSourceImmediate(), instr->getSourceImmediate());
    }
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::LoadInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getTargetRegister(), TR_WordReg);
    log->prints(" <- ");
    print(log, instr->getMemoryReference());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::StoreInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
    print(log, instr->getSource1Register(), TR_WordReg);
    log->prints(" -> ");
    print(log, instr->getMemoryReference());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::Instruction *instr)
{
    switch (instr->getKind()) {
        case OMR::Instruction::IsLabel:
            print(log, (TR::LabelInstruction *)instr);
            break;
        case OMR::Instruction::IsAdmin:
            print(log, (TR::AdminInstruction *)instr);
            break;
        case OMR::Instruction::IsData:
            print(log, (TR::DataInstruction *)instr);
            break;
        case OMR::Instruction::IsRTYPE:
            print(log, (TR::RtypeInstruction *)instr);
            break;
        case OMR::Instruction::IsITYPE:
            print(log, (TR::ItypeInstruction *)instr);
            break;
        case OMR::Instruction::IsSTYPE:
            print(log, (TR::StypeInstruction *)instr);
            break;
        case OMR::Instruction::IsBTYPE:
            print(log, (TR::BtypeInstruction *)instr);
            break;
        case OMR::Instruction::IsUTYPE:
            print(log, (TR::UtypeInstruction *)instr);
            break;
        case OMR::Instruction::IsJTYPE:
            print(log, (TR::JtypeInstruction *)instr);
            break;
        case OMR::Instruction::IsLOAD:
            print(log, (TR::LoadInstruction *)instr);
            break;
        case OMR::Instruction::IsSTORE:
            print(log, (TR::StoreInstruction *)instr);
            break;
        default: {
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
    TR::Node *node = instr->getNode();
    log->printf("%d \t", node ? node->getByteCodeIndex() : 0);
}

void TR_Debug::print(OMR::Logger *log, TR::LabelInstruction *instr)
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
            TR_UNIMPLEMENTED();
        }
    }
    printInstructionComment(log, 1, instr);
    if (instr->getDependencyConditions())
        print(log, instr->getDependencyConditions());
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::AdminInstruction *instr)
{
    printPrefix(log, instr);
    log->printf("%s ", getOpCodeName(&instr->getOpCode()));

    TR::Node *node = instr->getNode();
    if (node) {
        if (node->getOpCodeValue() == TR::BBStart) {
            log->printf(" (BBStart (block_%d))", node->getBlock()->getNumber());
        } else if (node->getOpCodeValue() == TR::BBEnd) {
            log->printf(" (BBEnd (block_%d))", node->getBlock()->getNumber());
        }
    }

    if (instr->getDependencyConditions())
        print(log, instr->getDependencyConditions());

    log->flush();
}

/*
void
TR_Debug::print(OMR::Logger *log, TR::RVTrg1Instruction *instr)
   {
   printPrefix(log, instr);
   log->printf("%s \t", getOpCodeName(&instr->getOpCode()));
   print(log, instr->getTargetRegister(), TR_WordReg);
   log->flush();
   }
*/

void TR_Debug::print(OMR::Logger *log, TR::RegisterDependency *dep)
{
    log->printc('[');
    print(log, dep->getRegister(), TR_WordReg);
    log->prints(" : ");
    log->printf("%s] ", getRVRegisterName(dep->getRealRegister()));
    log->flush();
}

void TR_Debug::print(OMR::Logger *log, TR::RegisterDependencyConditions *conditions)
{
    if (conditions) {
        uint32_t i;
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

void TR_Debug::print(OMR::Logger *log, TR::MemoryReference *mr)
{
    log->printc('[');

    if (mr->getBaseRegister() != NULL) {
        print(log, mr->getBaseRegister());
        log->prints(", ");
    }

    log->printf("%d", mr->getOffset(true));

    log->printc(']');
}

void TR_Debug::printRVGCRegisterMap(OMR::Logger *log, TR::GCRegisterMap *map) { TR_UNIMPLEMENTED(); }

void TR_Debug::print(OMR::Logger *log, TR::RealRegister *reg, TR_RegisterSizes size)
{
    log->prints(getName(reg, size));
}

static const char *getRegisterName(TR::RealRegister::RegNum num, bool is64bit)
{
    switch (num) {
#define DECLARE_GPR(regname, abiname, encoding) \
    case TR::RealRegister::regname:             \
        return #abiname;
#define DECLARE_FPR(regname, abiname, encoding) \
    case TR::RealRegister::regname:             \
        return #abiname;
#include "codegen/riscv-regs.h"
#undef DECLARE_GPR
#undef DECLARE_FPR
        default:
            return "???";
    }
}

const char *TR_Debug::getName(TR::RealRegister *reg, TR_RegisterSizes size)
{
    return getRegisterName(reg->getRegisterNumber(), (size == TR_DoubleWordReg || size == TR_DoubleReg));
}

const char *TR_Debug::getRVRegisterName(uint32_t regNum, bool is64bit)
{
    return getRegisterName((TR::RealRegister::RegNum)regNum, is64bit);
}

void TR_Debug::printRVOOLSequences(OMR::Logger *log) { TR_UNIMPLEMENTED(); }

// This function assumes that if a trampoline is required then it must be to a helper function.
// Use this API only for inquiring about branches to helpers.
bool TR_Debug::isBranchToTrampoline(TR::SymbolReference *symRef, uint8_t *cursor, int32_t &distance)
{
    uintptr_t target = (uintptr_t)symRef->getMethodAddress();
    bool requiresTrampoline = false;

    if (_cg->directCallRequiresTrampoline(target, (intptr_t)cursor)) {
        target = TR::CodeCacheManager::instance()->findHelperTrampoline(symRef->getReferenceNumber(), (void *)cursor);
        requiresTrampoline = true;
    }

    distance = (int32_t)(target - (intptr_t)cursor);
    return requiresTrampoline;
}
