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

#include "p/codegen/PPCAOTRelocation.hpp"

#include <stddef.h>
#include <stdint.h>
#include "codegen/CodeGenerator.hpp"
#include "codegen/Instruction.hpp"
#include "codegen/Relocation.hpp"
#include "compile/Compilation.hpp"
#include "control/Options.hpp"
#include "control/Options_inlines.hpp"
#include "env/CompilerEnv.hpp"
#include "env/TRMemory.hpp"
#include "env/jittypes.h"
#include "il/LabelSymbol.hpp"
#include "p/codegen/PPCInstruction.hpp"
#include "runtime/Runtime.hpp"

void TR::PPCPairedRelocation::mapRelocation(TR::CodeGenerator *cg)
{
    if (cg->comp()->compileRelocatableCode()) {
        // The validation and inlined method ExternalRelocations are added after
        // binary encoding is finished so that their actual relocation records
        // will end up at the start. This way other relocations can depend on the
        // appropriate validations already having completed.
        //
        // This relocation may also need to depend on previous validations, but
        // those are already in the AOT relocation list by this point. Add it at
        // the beginning to maintain the correct relative ordering.

        cg->addExternalRelocation(
            new (cg->trHeapMemory()) TR::ExternalOrderedPair32BitRelocation(getSourceInstruction()->getBinaryEncoding(),
                getSource2Instruction()->getBinaryEncoding(), getRelocationTarget(), getKind(), cg),
            __FILE__, __LINE__, getNode(), TR::ExternalRelocationAtFront);
    }
}

static void update16BitImmediate(TR::Instruction *instr, uint16_t imm)
{
    int32_t extImm;

    switch (instr->getOpCode().getFormat()) {
        case FORMAT_RT_RA_SI16:
        case FORMAT_RT_SI16:
        case FORMAT_RT_D16_RA:
        case FORMAT_FRT_D16_RA:
        case FORMAT_RS_D16_RA:
        case FORMAT_FRS_D16_RA:
            extImm = (int16_t)imm;
            break;
        case FORMAT_RA_RS_UI16:
            extImm = imm;
            break;
        default:
            TR_ASSERT_FATAL_WITH_INSTRUCTION(instr, false, "Unhandled instruction format in update16BitImmediate");
    }

    switch (instr->getKind()) {
        case TR::Instruction::IsTrg1Imm:
            static_cast<TR::PPCTrg1ImmInstruction *>(instr)->setSourceImmediate(extImm);
            break;
        case TR::Instruction::IsTrg1Src1Imm:
            static_cast<TR::PPCTrg1Src1ImmInstruction *>(instr)->setSourceImmediate(extImm);
            break;
        case TR::Instruction::IsTrg1Mem:
            static_cast<TR::PPCTrg1MemInstruction *>(instr)->getMemoryReference()->setOffset(extImm);
            break;
        case TR::Instruction::IsMemSrc1:
            static_cast<TR::PPCMemSrc1Instruction *>(instr)->getMemoryReference()->setOffset(extImm);
            break;
        default:
            TR_ASSERT_FATAL_WITH_INSTRUCTION(instr, false, "Unhandled instruction kind in update16BitImmediate");
    }

    TR_ASSERT_FATAL_WITH_INSTRUCTION(instr, instr->getBinaryEncoding(),
        "Attempt to patch unencoded instruction in update16BitImmediate");
    *reinterpret_cast<uint32_t *>(instr->getBinaryEncoding()) |= extImm & 0xffff;
}

void TR::PPCPairedLabelAbsoluteRelocation::apply(TR::CodeGenerator *cg)
{
    intptr_t p = (intptr_t)getLabel()->getCodeLocation();

    if (cg->comp()->target().is32Bit()) {
        // We're patching one of the following sequences:
        //
        // 1. _instr1 = lis rX, HI_VALUE(p)
        //    _instr2 = addi rX, rX, LO_VALUE(p)
        //
        // 2. _instr1 = addis rX, rX, HI_VALUE(p)
        //    _instr2 = addi rX, rX, LO_VALUE(p)

        update16BitImmediate(_instr1, cg->hiValue(p) & 0xffff);
        update16BitImmediate(_instr2, LO_VALUE(p));
    } else {
        intptr_t hi = cg->hiValue(p);

        // We're patching one of the following sequences:
        //
        // 1. _instr1 = lis rX, (int16_t)(HI_VALUE(p) >> 32)
        //    _instr2 = ori rX, rX, (HI_VALUE(p) >> 16) & 0xffff
        //              rldicr rX, rX, 32, 31
        //    _instr3 = oris rX, rX, HI_VALUE(p) & 0xffff
        //    _instr4 = addi rX, rX, LO_VALUE(p)
        //
        // 2. _instr1 = lis rX, (int16_t)(HI_VALUE(p) >> 32)
        //    _instr3 = lis rY, (int16_t)HI_VALUE(p)
        //    _instr2 = ori rX, rX, (HI_VALUE(p) >> 16) & 0xffff
        //    _instr4 = ori rX, rX, LO_VALUE(p)
        //              rldimi rX, rX, 32, 0

        update16BitImmediate(_instr1, (hi >> 32) & 0xffff);
        update16BitImmediate(_instr2, (hi >> 16) & 0xffff);
        update16BitImmediate(_instr3, hi & 0xffff);
        update16BitImmediate(_instr4, LO_VALUE(p));
    }
}
