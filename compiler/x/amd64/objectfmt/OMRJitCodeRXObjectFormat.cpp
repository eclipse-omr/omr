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
#include "objectfmt/JitCodeRXObjectFormat.hpp"
#include "runtime/Runtime.hpp"
#include "omrformatconsts.h"

TR::Instruction *OMR::X86::AMD64::JitCodeRXObjectFormat::emitFunctionCall(TR::FunctionCallData &data)
{
    TR::SymbolReference *methodSymRef = NULL;

    if (data.runtimeHelperIndex > 0) {
        methodSymRef = data.cg->symRefTab()->findOrCreateRuntimeHelper(data.runtimeHelperIndex);
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
        targetAddress || (!targetAddress && data.cg->comp()->isRecursiveMethodTarget(methodSymRef->getSymbol())),
        "function address is unknown");

    data.cg->resetIsLeafMethod();

    TR::Compilation *comp = data.cg->comp();

    /**
     * If this function won't be recompiled then allow a recursive call to
     * be done directly with a relative displacement.
     */
    if (comp->isRecursiveMethodTarget(methodSymRef->getSymbol()) && !comp->couldBeRecompiled()) {
        TR::X86ImmSymInstruction *callImmSymInstr = NULL;

        if (data.prevInstr) {
            callImmSymInstr = generateImmSymInstruction(data.prevInstr, TR::InstOpCode::CALLImm4, 0, data.methodSymRef,
                data.regDeps, data.cg);
        } else {
            callImmSymInstr = generateImmSymInstruction(TR::InstOpCode::CALLImm4, data.callNode, 0, data.methodSymRef,
                data.regDeps, data.cg);
        }

        if (data.adjustsFramePointerBy != 0) {
            callImmSymInstr->setAdjustsFramePointerBy(data.adjustsFramePointerBy);
        }

        data.out_callInstr = callImmSymInstr;
        return callImmSymInstr;
    }

    ccFunctionData *ccFunctionDataAddress
        = reinterpret_cast<ccFunctionData *>(data.cg->allocateCodeMemory(sizeof(ccFunctionData), false));

    if (!ccFunctionDataAddress) {
        comp->failCompilation<TR::CompilationException>("Could not allocate function data");
    }

    ccFunctionDataAddress->address = targetAddress;

    TR::StaticSymbol *functionDataSymbol = TR::StaticSymbol::createWithAddress(comp->trHeapMemory(), TR::Address,
        reinterpret_cast<void *>(ccFunctionDataAddress));
    functionDataSymbol->setNotDataAddress();
    TR::SymbolReference *functionDataSymRef
        = new (comp->trHeapMemory()) TR::SymbolReference(comp->getSymRefTab(), functionDataSymbol, 0);

    TR::X86CallMemInstruction *callInstr = NULL;
    TR::MemoryReference *callMR = new (comp->trHeapMemory()) TR::MemoryReference(functionDataSymRef, data.cg, true);

    if (data.prevInstr) {
        callInstr = generateCallMemInstruction(data.prevInstr, TR::InstOpCode::CALLMem, callMR, data.regDeps, data.cg);
    } else {
        callInstr = generateCallMemInstruction(TR::InstOpCode::CALLMem, data.callNode, callMR, data.regDeps, data.cg);
    }

    if (data.adjustsFramePointerBy != 0) {
        callInstr->setAdjustsFramePointerBy(data.adjustsFramePointerBy);
    }

    data.out_callInstr = callInstr;

    return callInstr;
}

uint8_t *OMR::X86::AMD64::JitCodeRXObjectFormat::encodeFunctionCall(TR::FunctionCallData &data)
{
    TR::SymbolReference *methodSymRef = NULL;

    if (data.runtimeHelperIndex > 0) {
        methodSymRef = data.cg->symRefTab()->findOrCreateRuntimeHelper(data.runtimeHelperIndex);
    } else {
        methodSymRef = data.methodSymRef;
    }

    /**
     * If a targetAddress has been provided then use that to override whatever would be
     * derived from the symbol reference.
     */
    TR_ASSERT_FATAL_WITH_NODE(data.callNode, !(data.runtimeHelperIndex && data.targetAddress),
        "a runtime helper (%" OMR_PRId32 ") and target address (%" OMR_PRIuPTR ") cannot both be provided",
        data.runtimeHelperIndex, data.targetAddress);

    uintptr_t targetAddress
        = data.targetAddress ? data.targetAddress : reinterpret_cast<uintptr_t>(methodSymRef->getMethodAddress());

    TR_ASSERT_FATAL_WITH_NODE(data.callNode, targetAddress, "function address is unknown");

    data.cg->resetIsLeafMethod();

    TR::Compilation *comp = data.cg->comp();

    ccFunctionData *ccFunctionDataAddress
        = reinterpret_cast<ccFunctionData *>(data.cg->allocateCodeMemory(sizeof(ccFunctionData), false));

    if (!ccFunctionDataAddress) {
        comp->failCompilation<TR::CompilationException>("Could not allocate function data");
    }

    ccFunctionDataAddress->address = targetAddress;

    *data.bufferAddress++ = 0xff; // CALL | JMP Mem
    const uint8_t modRM = data.useCall ? 0x15 : 0x25; // RIP addressing mode
    *data.bufferAddress++ = modRM;

    intptr_t functionAddress = reinterpret_cast<intptr_t>(ccFunctionDataAddress);
    intptr_t nextInstructionAddress = reinterpret_cast<intptr_t>(data.bufferAddress + 4);

    TR_ASSERT_FATAL_WITH_NODE(data.callNode,
        comp->target().cpu.isTargetWithinRIPRange(functionAddress, nextInstructionAddress),
        "ccFunctionData must be reachable directly: ccFunctionDataAddress=%" OMR_PRIxPTR
        ", nextInstructionAddress=%" OMR_PRIxPTR,
        functionAddress, nextInstructionAddress);

    const int32_t disp32 = (int32_t)(functionAddress - nextInstructionAddress);
    *(reinterpret_cast<int32_t *>(data.bufferAddress)) = disp32;

    data.out_encodedMethodAddressLocation = reinterpret_cast<uint8_t *>(ccFunctionDataAddress);
    data.bufferAddress += 4;

    return data.bufferAddress;
}

void OMR::X86::AMD64::JitCodeRXObjectFormat::printEncodedFunctionCall(TR::FILE *pOutFile, TR::FunctionCallData &data,
    uint8_t *bufferPos)
{
    if (!pOutFile) {
        return;
    }

    TR_Debug *debug = data.cg->getDebug();

    debug->printPrefix(pOutFile, NULL, bufferPos, 5);
    trfprintf(pOutFile, data.useCall ? "call\t" : "jmp\t");
    trfprintf(pOutFile, "[%#" OMR_PRIxPTR "]", data.out_encodedMethodAddressLocation);

    if (data.methodSymRef && data.methodSymRef->getSymbol()->castToMethodSymbol()->isHelper()) {
        trfprintf(pOutFile, " (%s)", debug->getRuntimeHelperName(data.methodSymRef->getReferenceNumber()));
    } else {
        trfprintf(pOutFile, " (%#" OMR_PRIxPTR ")", data.methodSymRef->getMethodAddress());
    }
}

