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
#include "env/FilePointerDecl.hpp"
#include "il/MethodSymbol.hpp"
#include "il/SymbolReference.hpp"
#include "infra/Assert.hpp"
#include "objectfmt/FunctionCallData.hpp"
#include "objectfmt/JitCodeRWXObjectFormat.hpp"
#include "objectfmt/OMRJitCodeRWXObjectFormat.hpp"
#include "runtime/CodeMetaDataManager.hpp"
#include "runtime/Runtime.hpp"
#include "z/codegen/CallSnippet.hpp"
#include "z/codegen/S390GenerateInstructions.hpp"

TR::Instruction *OMR::Z::JitCodeRWXObjectFormat::emitFunctionCall(TR::FunctionCallData &data)
{
    TR::SymbolReference *callSymRef = data.methodSymRef;
    TR::CodeGenerator *cg = data.cg;
    TR::Node *callNode = data.callNode;

    if (callSymRef == NULL) {
        TR_ASSERT_FATAL_WITH_NODE(callNode, data.runtimeHelperIndex > 0,
            "FunctionCallData does not contain symbol reference for call or valid TR_RuntimeHelper");
        callSymRef = cg->symRefTab()->findOrCreateRuntimeHelper(data.runtimeHelperIndex, false, false, false);
    }

    uintptr_t targetAddress
        = data.targetAddress != NULL ? data.targetAddress : reinterpret_cast<uintptr_t>(callSymRef->getMethodAddress());

    TR_ASSERT_FATAL_WITH_NODE(callNode, targetAddress != NULL,
        "Unable to make a call as targetAddress for the Call is not found.");

    // Now as we are going to make call, we need Return Address register.
    TR_ASSERT_FATAL_WITH_NODE(callNode, data.returnAddressReg != NULL,
        "returnAddressReg should be set to make a function call.");

    // We do not need to check if the address is addressible using RIL type
    // instruction, as the instruction API will check for it and generate
    // trampolines if needed.

    TR::Instruction *callInstr = generateRILInstruction(cg, TR::InstOpCode::BRASL, callNode, data.returnAddressReg,
        callSymRef, reinterpret_cast<void *>(targetAddress), data.prevInstr);

    data.out_callInstr = callInstr;

    if (data.regDeps != NULL) {
        callInstr->setDependencyConditions(data.regDeps);
    }

    return callInstr;
}

uint8_t *OMR::Z::JitCodeRWXObjectFormat::encodeFunctionCall(TR::FunctionCallData &data)
{
    TR::SymbolReference *callSymRef = data.methodSymRef;
    TR::CodeGenerator *cg = data.cg;
    TR::Node *callNode = data.callNode;

    if (callSymRef == NULL) {
        TR_ASSERT_FATAL_WITH_NODE(callNode, data.runtimeHelperIndex > 0,
            "FunctionCallData does not contain symbol reference for call or valid TR_RuntimeHelper");
        callSymRef = cg->symRefTab()->findOrCreateRuntimeHelper(data.runtimeHelperIndex, false, false, false);
    }

    uintptr_t targetAddress
        = data.targetAddress != NULL ? data.targetAddress : reinterpret_cast<uintptr_t>(callSymRef->getMethodAddress());

    uint8_t *cursor = data.bufferAddress;
    TR_ASSERT_FATAL_WITH_NODE(callNode, targetAddress != NULL,
        "Unable to make a call as targetAddress for the Call is not found.");

    if (data.snippet->getKind() == TR::Snippet::IsUnresolvedCall
        || data.snippet->getKind() == TR::Snippet::IsUnresolvedData) {
        // LARL r14,AddressOfConstantDataArea
        uintptr_t larlInstructionAddress = reinterpret_cast<uintptr_t>(cursor);
        *reinterpret_cast<int16_t *>(cursor) = 0xC0E0;
        cursor += sizeof(int16_t);
        uint8_t *larlOffsetAddress = cursor;
        // For now skipping the encoding of offsetInHalfwords for LARL, will be updated later.
        cursor += sizeof(int32_t);

        *reinterpret_cast<int32_t *>(cursor)
            = 0xe300e000 + ((static_cast<int32_t>(cg->getEntryPointRegister()) - 1) << 20);
        cursor += sizeof(int32_t);

        *reinterpret_cast<int16_t *>(cursor) = cg->comp()->target().is64Bit() ? 0x0004 : 0x0014;
        cursor += sizeof(int16_t);

        // BCR   rEP
        *reinterpret_cast<int16_t *>(cursor) = 0x07F0 + static_cast<int16_t>(cg->getEntryPointRegister() - 1);
        cursor += sizeof(int16_t);

        // Now updating the offset in LARL instruction
        *reinterpret_cast<int32_t *>(larlOffsetAddress)
            = static_cast<int32_t>(((uintptr_t)cursor + data.snippet->getPadBytes() - larlInstructionAddress) / 2);
    } else {
        uint8_t *instrAddr = cursor;
        *reinterpret_cast<int16_t *>(cursor) = 0xC0E5;
        cursor += sizeof(int16_t);
        *reinterpret_cast<int32_t *>(cursor)
            = TR::S390CallSnippet::adjustCallOffsetWithTrampoline(targetAddress, instrAddr, callSymRef, data.snippet);
        // TODO: We should be able to use reloKind field from the FunctionCallData to add relocation.
        // Probably only change apart from the hardcoded TR_HelperAddress is to get the correct Diagnostic message for
        // AOT.
        cg->addProjectSpecializedRelocation(cursor, (uint8_t *)callSymRef, NULL, TR_HelperAddress, __FILE__, __LINE__,
            callNode);
        cursor += sizeof(int32_t);
    }

    return cursor;
}

uint8_t *OMR::Z::JitCodeRWXObjectFormat::printEncodedFunctionCall(TR::FILE *pOutFile, TR::FunctionCallData &data)
{
    uint8_t *bufferPos = data.bufferAddress;
    TR_Debug *debug = data.cg->getDebug();
    if (data.snippet->getKind() == TR::Snippet::IsUnresolvedCall
        || data.snippet->getKind() == TR::Snippet::IsUnresolvedData) {
        debug->printPrefix(pOutFile, NULL, bufferPos, 6);
        trfprintf(pOutFile, "LARL \tGPR14, <%p>\t# Start of Data Const.",
            bufferPos + 6 /*LARL*/ + 6 /*LG/LGF*/ + 2 /*BRC*/ + data.snippet->getPadBytes());
        bufferPos += 6;

        debug->printPrefix(pOutFile, NULL, bufferPos, 6);
        trfprintf(pOutFile, data.cg->comp()->target().is64Bit() ? "LG \tGPR_EP, 0(GPR14)" : "LGF \tGPR_EP, 0(GPR14)");
        bufferPos += 6;

        debug->printPrefix(pOutFile, NULL, bufferPos, 2);
        trfprintf(pOutFile, "BCR \tGPR_EP");
        bufferPos += 2;
    } else {
        debug->printPrefix(pOutFile, NULL, bufferPos, 6);
        trfprintf(pOutFile, "BRASL \tGPR14, <%p>\t# Branch to Helper Method %s", data.snippet->getSnippetDestAddr(),
            data.snippet->usedTrampoline() ? "- Trampoline Used. " : "");
        bufferPos += 6;
    }
    return bufferPos;
}
