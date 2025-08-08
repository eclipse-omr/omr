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

#ifndef IA32COMPAREANALYSER_INCL
#define IA32COMPAREANALYSER_INCL

#include <stdint.h>
#include "codegen/Analyser.hpp"
#include "codegen/CodeGenerator.hpp"
#include "codegen/InstOpCode.hpp"

#define EvalChild1 0x01
#define EvalChild2 0x02
#define CmpReg1Reg2 0x04
#define CmpReg1Mem2 0x08
#define CmpMem1Reg2 0x10

namespace TR {
class LabelSymbol;
class Machine;
class Node;
class Register;
} // namespace TR

class TR_X86CompareAnalyser : public TR_Analyser {
    static const uint8_t _actionMap[NUM_ACTIONS];
    TR::CodeGenerator *_cg;
    TR::Machine *_machine;

public:
    TR_X86CompareAnalyser(TR::CodeGenerator *cg)
        : _cg(cg)
        , _machine(cg->machine())
    {}

    void integerCompareAnalyser(TR::Node *root, TR::InstOpCode::Mnemonic regRegOpCode,
        TR::InstOpCode::Mnemonic regMemOpCode, TR::InstOpCode::Mnemonic memRegOpCode);

    void integerCompareAnalyser(TR::Node *root, TR::Node *firstChild, TR::Node *secondChild,
        bool determineEvaluationOrder, TR::InstOpCode::Mnemonic regRegOpCode, TR::InstOpCode::Mnemonic regMemOpCode,
        TR::InstOpCode::Mnemonic memRegOpCode);

    void longOrderedCompareAndBranchAnalyser(TR::Node *root, TR::InstOpCode::Mnemonic lowBranchOpCode,
        TR::InstOpCode::Mnemonic highBranchOpCode, TR::InstOpCode::Mnemonic highReversedBranchOpCode);

    void longEqualityCompareAndBranchAnalyser(TR::Node *root, TR::LabelSymbol *firstBranchLabel,
        TR::LabelSymbol *secondBranchLabel, TR::InstOpCode::Mnemonic secondBranchOp);

    TR::Register *longEqualityBooleanAnalyser(TR::Node *root, TR::InstOpCode::Mnemonic setOpCode,
        TR::InstOpCode::Mnemonic combineOpCode);

    TR::Register *longOrderedBooleanAnalyser(TR::Node *root, TR::InstOpCode::Mnemonic highSetOpCode,
        TR::InstOpCode::Mnemonic lowSetOpCode);

    TR::Register *longCMPAnalyser(TR::Node *root);

    bool getEvalChild1() { return (_actionMap[getInputs()] & EvalChild1) ? true : false; }

    bool getEvalChild2() { return (_actionMap[getInputs()] & EvalChild2) ? true : false; }

    bool getCmpReg1Reg2() { return (_actionMap[getInputs()] & CmpReg1Reg2) ? true : false; }

    bool getCmpReg1Mem2() { return (_actionMap[getInputs()] & CmpReg1Mem2) ? true : false; }

    bool getCmpMem1Reg2() { return (_actionMap[getInputs()] & CmpMem1Reg2) ? true : false; }

    TR::CodeGenerator *cg() { return _cg; }
};

#endif
