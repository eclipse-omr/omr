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

#include "codegen/ARM64OutOfLineCodeSection.hpp"
#include "codegen/BackingStore.hpp"
#include "codegen/CodeGenerator.hpp"
#include "codegen/GenerateInstructions.hpp"
#include "codegen/Machine.hpp"
#include "codegen/Register.hpp"

TR_ARM64OutOfLineCodeSection::TR_ARM64OutOfLineCodeSection(TR::Node *callNode, TR::ILOpCodes callOp,
    TR::Register *targetReg, TR::LabelSymbol *entryLabel, TR::LabelSymbol *restartLabel, TR::CodeGenerator *cg)
    : TR_OutOfLineCodeSection(callNode, callOp, targetReg, entryLabel, restartLabel, cg)
{
    generateARM64OutOfLineCodeSectionDispatch();
}

void TR_ARM64OutOfLineCodeSection::generateARM64OutOfLineCodeSectionDispatch()
{
    // Switch to cold helper instruction stream.
    //
    swapInstructionListsWithCompilation();

    TR::Instruction *entryLabelInstruction
        = generateLabelInstruction(_cg, TR::InstOpCode::label, _callNode, _entryLabel);

    _cg->incOutOfLineColdPathNestedDepth();
    TR_Debug *debugObj = _cg->getDebug();
    if (debugObj) {
        debugObj->addInstructionComment(entryLabelInstruction, "Denotes start of OOL sequence");
    }

    TR::Register *resultReg = TR::TreeEvaluator::performCall(_callNode, _callNode->getOpCode().isCallIndirect(), _cg);

    if (_targetReg) {
        TR_ASSERT(resultReg, "resultReg must not be a NULL");
        generateMovInstruction(_cg, _callNode, _targetReg, resultReg);
    }
    _cg->decReferenceCount(_callNode);

    TR::Instruction *returnBranchInstruction
        = generateLabelInstruction(_cg, TR::InstOpCode::b, _callNode, _restartLabel);

    if (debugObj) {
        debugObj->addInstructionComment(returnBranchInstruction, "Denotes end of OOL: return to mainline");
    }

    _cg->decOutOfLineColdPathNestedDepth();

    // Switch from cold helper instruction stream.
    swapInstructionListsWithCompilation();
}
