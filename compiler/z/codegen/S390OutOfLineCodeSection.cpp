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

#include "z/codegen/S390OutOfLineCodeSection.hpp"

#include <stddef.h>
#include <stdint.h>
#include "codegen/CodeGenerator.hpp"
#include "codegen/InstOpCode.hpp"
#include "codegen/Instruction.hpp"
#include "codegen/Machine.hpp"
#include "codegen/OutOfLineCodeSection.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/Register.hpp"
#include "codegen/RegisterConstants.hpp"
#include "codegen/RegisterDependency.hpp"
#include "codegen/RegisterPair.hpp"
#include "codegen/TreeEvaluator.hpp"
#include "compile/Compilation.hpp"
#include "env/CompilerEnv.hpp"
#include "il/ILOpCodes.hpp"
#include "il/ILOps.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "ras/Debug.hpp"
#include "z/codegen/S390GenerateInstructions.hpp"

namespace TR {
class LabelSymbol;
}

TR_S390OutOfLineCodeSection::TR_S390OutOfLineCodeSection(TR::Node *callNode, TR::ILOpCodes callOp,
    TR::Register *targetReg, TR::LabelSymbol *entryLabel, TR::LabelSymbol *restartLabel, TR::CodeGenerator *cg)
    : TR_OutOfLineCodeSection(callNode, callOp, targetReg, entryLabel, restartLabel, cg)
    , _targetRegMovOpcode(cg->comp()->target().is64Bit() ? TR::InstOpCode::LGR : TR::InstOpCode::LR)
{
    // isPreparedForDirectJNI also checks if the node is a call
    if (callNode->isPreparedForDirectJNI()) {
        _callNode->setPreparedForDirectJNI();
    }
}

TR_S390OutOfLineCodeSection::TR_S390OutOfLineCodeSection(TR::Node *callNode, TR::ILOpCodes callOp,
    TR::Register *targetReg, TR::LabelSymbol *entryLabel, TR::LabelSymbol *restartLabel,
    TR::InstOpCode::Mnemonic targetRegMovOpcode, TR::CodeGenerator *cg)
    : TR_OutOfLineCodeSection(callNode, callOp, targetReg, entryLabel, restartLabel, cg)
    , _targetRegMovOpcode(targetRegMovOpcode)
{
    // generateS390OutOfLineCodeSectionDispatch();
}

void TR_S390OutOfLineCodeSection::generateS390OutOfLineCodeSectionDispatch()
{
    // Switch to cold helper instruction stream.
    //
    swapInstructionListsWithCompilation();

    TR::Compilation *comp = _cg->comp();
    TR::Register *vmThreadReg = _cg->getMethodMetaDataRealRegister();
    TR::Instruction *temp = generateS390LabelInstruction(_cg, TR::InstOpCode::label, _callNode, _entryLabel);

    _cg->incOutOfLineColdPathNestedDepth();
    TR_Debug *debugObj = _cg->getDebug();
    if (debugObj) {
        debugObj->addInstructionComment(temp, "Denotes start of OOL sequence");
    }

    TR::Register *resultReg = TR::TreeEvaluator::performCall(_callNode, _callNode->getOpCode().isCallIndirect(), _cg);

    if (_targetReg)
        temp = generateRRInstruction(_cg, _targetRegMovOpcode, _callNode, _targetReg, _callNode->getRegister());

    _cg->decReferenceCount(_callNode);

    temp = generateS390BranchInstruction(_cg, TR::InstOpCode::BRC, TR::InstOpCode::COND_BRC, _callNode, _restartLabel);

    if (debugObj) {
        debugObj->addInstructionComment(temp, "Denotes end of OOL: return to mainline");
    }

    _cg->decOutOfLineColdPathNestedDepth();

    // Switch from cold helper instruction stream.
    swapInstructionListsWithCompilation();
}

/**
 * Create a NoReg dependency for each child of a call that has been evaluated into a register.
 * Ignore children that do not have a register since their live range should not persist outside of
 * the helper call stream.
 */
TR::RegisterDependencyConditions *TR_S390OutOfLineCodeSection::formEvaluatedArgumentDepList()
{
    int32_t i, c = 0;

    for (i = _callNode->getFirstArgumentIndex(); i < _callNode->getNumChildren(); i++) {
        TR::Register *reg = _callNode->getChild(i)->getRegister();
        if (reg) {
            TR::RegisterPair *regPair = reg->getRegisterPair();
            c += regPair ? 2 : 1;
        }
    }

    TR::RegisterDependencyConditions *depConds = NULL;

    if (c) {
        TR::Machine *machine = _cg->machine();
        depConds = generateRegisterDependencyConditions(0, c, _cg);
        for (i = _callNode->getFirstArgumentIndex(); i < _callNode->getNumChildren(); i++) {
            TR::Register *reg = _callNode->getChild(i)->getRegister();
            if (reg) {
                TR::RegisterPair *regPair = reg->getRegisterPair();
                if (regPair) {
                    depConds->addPostCondition(regPair->getLowOrder(), TR::RealRegister::NoReg);
                    depConds->addPostCondition(regPair->getHighOrder(), TR::RealRegister::NoReg);
                } else {
                    depConds->addPostCondition(reg, TR::RealRegister::NoReg);
                }
            }
        }
    }
    return depConds;
}
