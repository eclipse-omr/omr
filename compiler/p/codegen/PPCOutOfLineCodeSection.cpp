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

#include "p/codegen/PPCOutOfLineCodeSection.hpp"

#include <stddef.h>
#include "codegen/CodeGenerator.hpp"
#include "codegen/Instruction.hpp"
#include "codegen/Machine.hpp"
#include "codegen/TreeEvaluator.hpp"
#include "compile/Compilation.hpp"
#include "env/TRMemory.hpp"
#include "il/ILOps.hpp"
#include "il/LabelSymbol.hpp"
#include "il/Node.hpp"
#include "infra/Assert.hpp"
#include "p/codegen/GenerateInstructions.hpp"
#include "p/codegen/PPCInstruction.hpp"

namespace TR {
class RegisterDependencyConditions;
}

TR_PPCOutOfLineCodeSection::TR_PPCOutOfLineCodeSection(TR::Node *callNode, TR::ILOpCodes callOp,
    TR::Register *targetReg, TR::LabelSymbol *entryLabel, TR::LabelSymbol *restartLabel, TR::CodeGenerator *cg)
    : TR_OutOfLineCodeSection(callNode, callOp, targetReg, entryLabel, restartLabel, cg)
{
    if (callNode->isPreparedForDirectJNI()) {
        _callNode->setPreparedForDirectJNI();
    }

    generatePPCOutOfLineCodeSectionDispatch();
}

void TR_PPCOutOfLineCodeSection::generatePPCOutOfLineCodeSectionDispatch()
{
    // Switch to cold helper instruction stream.
    //
    swapInstructionListsWithCompilation();

    new (_cg->trHeapMemory()) TR::PPCLabelInstruction(TR::InstOpCode::label, _callNode, _entryLabel, _cg);

    TR::Register *resultReg = NULL;
    if (_callNode->getOpCode().isCallIndirect())
        resultReg = TR::TreeEvaluator::performCall(_callNode, true, _cg);
    else
        resultReg = TR::TreeEvaluator::performCall(_callNode, false, _cg);

    if (_targetReg) {
        TR_ASSERT(resultReg, "assertion failure");
        generateTrg1Src1Instruction(_cg, TR::InstOpCode::mr, _callNode, _targetReg, resultReg);
    }
    _cg->decReferenceCount(_callNode);

    if (_restartLabel)
        generateLabelInstruction(_cg, TR::InstOpCode::b, _callNode, _restartLabel);

    generateLabelInstruction(_cg, TR::InstOpCode::label, _callNode, TR::LabelSymbol::create(_cg->trHeapMemory(), _cg));

    // Switch from cold helper instruction stream.
    //
    swapInstructionListsWithCompilation();
}
