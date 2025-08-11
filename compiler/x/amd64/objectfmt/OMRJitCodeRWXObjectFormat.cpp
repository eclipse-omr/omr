/*******************************************************************************
 * Copyright IBM Corp. and others 2021
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

#include "codegen/CodeGenerator.hpp"
#include "codegen/MemoryReference.hpp"
#include "codegen/X86Instruction.hpp"
#include "compile/Compilation.hpp"
#include "il/MethodSymbol.hpp"
#include "il/SymbolReference.hpp"
#include "infra/Assert.hpp"
#include "objectfmt/FunctionCallData.hpp"
#include "objectfmt/JitCodeRWXObjectFormat.hpp"
#include "runtime/Runtime.hpp"
#include "omrformatconsts.h"

TR::Instruction *OMR::X86::AMD64::JitCodeRWXObjectFormat::emitFunctionCall(TR::FunctionCallData &data)
{
    TR::SymbolReference *methodSymRef = NULL;

    if (data.runtimeHelperIndex > 0) {
        methodSymRef = data.cg->symRefTab()->findOrCreateRuntimeHelper(data.runtimeHelperIndex, false, false, false);
    } else {
        methodSymRef = data.methodSymRef;
    }

    /**
     * If a targetAddress has been provided then use that to override whatever would be
     * derived from the symbol reference.
     */
    TR_ASSERT_FATAL_WITH_NODE(data.callNode, !(data.runtimeHelperIndex && data.targetAddress),
        "a runtime helper (%" OMR_PRId32 ") and target address (%#" OMR_PRIxPTR ") cannot both be provided",
        data.runtimeHelperIndex, data.targetAddress);

    uintptr_t targetAddress
        = data.targetAddress ? data.targetAddress : reinterpret_cast<uintptr_t>(methodSymRef->getMethodAddress());

    /**
     * The targetAddress must be known at this point.  The only exception is a recursive
     * call to the method being compiled, as that won't be known until binary encoding
     * has completed.
     */
    TR_ASSERT_FATAL_WITH_NODE(data.callNode,
        (targetAddress || data.cg->comp()->isRecursiveMethodTarget(methodSymRef->getSymbol())),
        "function address is unknown");

    data.cg->resetIsLeafMethod();

    TR::Instruction *callInstr = NULL;

    if (data.runtimeHelperIndex || methodSymRef->getSymbol()->castToMethodSymbol()->isHelper()) {
        // Helper call
        //
        TR::X86ImmSymInstruction *callImmSym = NULL;
        const TR::InstOpCode::Mnemonic op = data.useCall ? TR::InstOpCode::CALLImm4 : TR::InstOpCode::JMP4;

        if (data.prevInstr) {
            callImmSym = generateImmSymInstruction(data.prevInstr, op, static_cast<int32_t>(targetAddress),
                methodSymRef, data.regDeps, data.cg);
        } else {
            callImmSym = generateImmSymInstruction(op, data.callNode, static_cast<int32_t>(targetAddress), methodSymRef,
                data.regDeps, data.cg);
        }

        if (data.adjustsFramePointerBy != 0) {
            callImmSym->setAdjustsFramePointerBy(data.adjustsFramePointerBy);
        }

        callInstr = callImmSym;
    } else {
        // Non-helper call
        //
        TR_ASSERT_FATAL_WITH_NODE(data.callNode, data.scratchReg, "scratch register is not available");

        TR_ASSERT_FATAL_WITH_NODE(data.callNode, (data.adjustsFramePointerBy == 0),
            "frame pointer adjustment not supported for TR::InstOpCode::CALLReg instructions");

        if (targetAddress == 0) {
            /**
             * The only situation where targetAddress could be 0 at this point is for
             * a recursive call.  This is confirmed/protected by the fatal assert above.
             * Emit a direct call to the start of the function whose displacement
             * will be properly relocated by the binary encoding of the instruction.
             */
            if (data.prevInstr) {
                callInstr = generateImmSymInstruction(data.prevInstr, TR::InstOpCode::CALLImm4, 0, data.methodSymRef,
                    data.regDeps, data.cg);
            } else {
                callInstr = generateImmSymInstruction(TR::InstOpCode::CALLImm4, data.callNode, 0, data.methodSymRef,
                    data.regDeps, data.cg);
            }
        } else {
            /**
             * Ideally, only the RegImm64Sym instruction form would be used.  However, there are
             * subtleties in the handling of relocations between these different forms of instructions
             * that need to be sorted out first..
             */
            if (data.useSymInstruction) {
                TR::AMD64RegImm64SymInstruction *loadInstr;

                if (data.prevInstr) {
                    loadInstr = generateRegImm64SymInstruction(data.prevInstr, TR::InstOpCode::MOV8RegImm64,
                        data.scratchReg, targetAddress, data.methodSymRef, data.cg);
                } else {
                    loadInstr = generateRegImm64SymInstruction(TR::InstOpCode::MOV8RegImm64, data.callNode,
                        data.scratchReg, targetAddress, data.methodSymRef, data.cg);
                }

                if (data.reloKind != TR_NoRelocation) {
                    loadInstr->setReloKind(data.reloKind);
                }

                data.out_materializeTargetAddressInstr = loadInstr;
            } else {
                TR::AMD64RegImm64Instruction *loadInstr;

                if (data.prevInstr) {
                    loadInstr = generateRegImm64Instruction(data.prevInstr, TR::InstOpCode::MOV8RegImm64,
                        data.scratchReg, targetAddress, data.cg, data.reloKind);
                } else {
                    loadInstr = generateRegImm64Instruction(TR::InstOpCode::MOV8RegImm64, data.callNode,
                        data.scratchReg, targetAddress, data.cg, data.reloKind);
                }

                data.out_materializeTargetAddressInstr = loadInstr;
            }

            const TR::InstOpCode::Mnemonic op = data.useCall ? TR::InstOpCode::CALLReg : TR::InstOpCode::JMPReg;
            if (data.prevInstr) {
                /**
                 * An instruction to materialize the function address was already inserted after
                 * the provided \c prevInstr, so the call or branch instruction must be inserted
                 * after the materialization instruction.
                 */
                callInstr = generateRegInstruction(data.out_materializeTargetAddressInstr, op, data.scratchReg,
                    data.regDeps, data.cg);
            } else {
                callInstr = generateRegInstruction(op, data.callNode, data.scratchReg, data.regDeps, data.cg);
            }
        }
    }

    data.out_callInstr = callInstr;

    return callInstr;
}

uint8_t *OMR::X86::AMD64::JitCodeRWXObjectFormat::encodeFunctionCall(TR::FunctionCallData &data)
{
    const uint8_t op = data.useCall ? 0xe8 : 0xe9; // CALL rel32 | JMP rel32
    *data.bufferAddress = op;

    int32_t disp32 = 0;

    if (data.methodSymRef && data.methodSymRef->getSymbol()->castToMethodSymbol()->isHelper()) {
        disp32 = data.cg->branchDisplacementToHelperOrTrampoline(data.bufferAddress, data.methodSymRef);
    } else {
        intptr_t targetAddress = reinterpret_cast<intptr_t>(data.methodSymRef->getMethodAddress());
        intptr_t nextInstructionAddress = reinterpret_cast<intptr_t>(data.bufferAddress + 5);

        TR_ASSERT_FATAL(data.cg->comp()->target().cpu.isTargetWithinRIPRange(targetAddress, nextInstructionAddress),
            "Target function address %" OMR_PRIxPTR " not reachable from %" OMR_PRIxPTR, targetAddress,
            data.bufferAddress);

        disp32 = static_cast<int32_t>(targetAddress - nextInstructionAddress);
    }

    *(reinterpret_cast<int32_t *>(++data.bufferAddress)) = disp32;

    data.out_encodedMethodAddressLocation = data.bufferAddress;

    data.bufferAddress += 4;

    return data.bufferAddress;
}

void OMR::X86::AMD64::JitCodeRWXObjectFormat::printEncodedFunctionCall(TR::FILE *pOutFile, TR::FunctionCallData &data,
    uint8_t *bufferPos)
{
    if (!pOutFile) {
        return;
    }

    TR_Debug *debug = data.cg->getDebug();

    debug->printPrefix(pOutFile, NULL, bufferPos, 5);
    trfprintf(pOutFile, data.useCall ? "call\t" : "jmp\t");

    if (data.methodSymRef && data.methodSymRef->getSymbol()->castToMethodSymbol()->isHelper()) {
        trfprintf(pOutFile, "%s", debug->getRuntimeHelperName(data.methodSymRef->getReferenceNumber()));
    } else {
        trfprintf(pOutFile, "%#" OMR_PRIxPTR, data.methodSymRef->getMethodAddress());
    }
}
