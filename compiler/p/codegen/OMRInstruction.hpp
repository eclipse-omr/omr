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

#ifndef OMR_POWER_INSTRUCTION_INCL
#define OMR_POWER_INSTRUCTION_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_INSTRUCTION_CONNECTOR
#define OMR_INSTRUCTION_CONNECTOR

namespace OMR {
namespace Power {
class Instruction;
}

typedef OMR::Power::Instruction InstructionConnector;
} // namespace OMR
#else
#error OMR::Power::Instruction expected to be a primary connector, but an OMR connector is already defined
#endif

#include "compiler/codegen/OMRInstruction.hpp"

#include <stddef.h>
#include <stdint.h>
#include "codegen/InstOpCode.hpp"
#include "codegen/RegisterConstants.hpp"
#include "infra/Assert.hpp"

namespace TR {
class PPCConditionalBranchInstruction;
class PPCImmInstruction;
class CodeGenerator;
class Instruction;
class MemoryReference;
class Node;
class Register;
class RegisterDependencyConditions;
} // namespace TR
struct TR_RegisterPressureState;

namespace OMR { namespace Power {

class OMR_EXTENSIBLE Instruction : public OMR::Instruction {
public:
    Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node = 0);
    Instruction(TR::CodeGenerator *cg, TR::Instruction *precedingInstruction, TR::InstOpCode::Mnemonic op,
        TR::Node *node = 0);

    virtual const char *description() { return "PPC"; }

    virtual Kind getKind() { return IsNotExtended; }

    virtual int32_t estimateBinaryLength(int32_t currentEstimate);
    virtual uint8_t *generateBinaryEncoding();

    TR::InstOpCode::Mnemonic getRecordFormOpCode() { return _opcode.getRecordFormOpCodeValue(); }

    virtual TR::Register *getTrg1Register() { return NULL; }

    virtual TR::Register *getTargetRegister(uint32_t i);
    virtual TR::Register *getSourceRegister(uint32_t i);

    virtual TR::MemoryReference *getMemoryReference() { return NULL; }

    virtual uint32_t getSourceImmediate() { return 0; }

    virtual TR::Register *getMemoryBase() { return NULL; }

    virtual TR::Register *getMemoryIndex() { return NULL; }

    virtual int64_t getOffset() { return 0; }

    virtual bool usesCountRegister() { return _opcode.usesCountRegister(); }

    virtual bool setsCountRegister();

    virtual TR::RegisterDependencyConditions *getDependencyConditions() { return _conditions; }

    TR::RegisterDependencyConditions *setDependencyConditions(TR::RegisterDependencyConditions *cond)
    {
        return (_conditions = cond);
    }

    bool isRecordForm() { return _opcode.isRecordForm(); }

    bool hasRecordForm() { return _opcode.hasRecordForm(); }

    bool singleFPOp() { return _opcode.singleFPOp(); }

    bool doubleFPOp() { return _opcode.doubleFPOp(); }

    bool gprOp() { return _opcode.gprOp(); }

    bool fprOp() { return _opcode.fprOp(); }

    bool readsCarryFlag() { return _opcode.readsCarryFlag(); }

    bool setsCarryFlag() { return _opcode.setsCarryFlag(); }

    bool setsOverflowFlag() { return _opcode.setsOverflowFlag(); }

    bool isUpdate() { return _opcode.isUpdate(); }

    bool isTrap() { return _opcode.isTrap(); }

    bool isTMAbort() { return _opcode.isTMAbort(); }

    bool isRegCopy() { return _opcode.isRegCopy(); }

    bool isDoubleWord() { return _opcode.isDoubleWord(); }

    bool isCompare() { return _opcode.isCompare(); }

    bool isLongRunningFPOp() { return _opcode.isLongRunningFPOp(); }

    bool isFXMult() { return _opcode.isFXMult(); }

    virtual bool isLoad() { return _opcode.isLoad(); }

    virtual bool isStore() { return _opcode.isStore(); }

    virtual bool isBranchOp() { return _opcode.isBranchOp(); }

    virtual bool isExceptBranchOp() { return false; }

    virtual bool setExceptBranchOp()
    {
        TR_ASSERT(0, "shouldn't be here");
        return false;
    }

    virtual bool isLabel() { return _opcode.getOpCodeValue() == TR::InstOpCode::label; }

    virtual bool isAdmin() { return _opcode.isAdmin(); }

    virtual bool is4ByteLoad();
    virtual int32_t getMachineOpCode();
    virtual bool isBeginBlock();

    virtual bool isFloat() { return _opcode.isFloat(); }

    virtual bool isVMX() { return _opcode.isVMX(); }

    virtual bool isVSX() { return _opcode.isVSX(); }

    virtual bool isSyncSideEffectFree() { return _opcode.isSyncSideEffectFree(); }

    virtual bool isCall();

    virtual bool isAsyncBranch() { return _asyncBranch; }

    virtual bool setAsyncBranch() { return (_asyncBranch = true); }

    void PPCNeedsGCMap(uint32_t mask);

    virtual TR::Register *getMemoryDataRegister();

    virtual void assignRegisters(TR_RegisterKinds kindToBeAssigned);

    virtual bool refsRegister(TR::Register *reg);

    virtual bool defsRegister(TR::Register *reg);

    virtual bool defsRealRegister(TR::Register *reg);

    virtual bool usesRegister(TR::Register *reg);

    virtual bool dependencyRefsRegister(TR::Register *reg);

    virtual TR::PPCConditionalBranchInstruction *getPPCConditionalBranchInstruction();

    virtual TR::Register *getPrimaryTargetRegister() { return NULL; }

    virtual uint8_t getBinaryLengthLowerBound();

// The following safe virtual downcast method is used under debug only
// for assertion checking
#if defined(DEBUG) || defined(PROD_WITH_ASSUMES)
    virtual TR::PPCImmInstruction *getPPCImmInstruction();
#endif

    /*
     * Maps to TIndex in Instruction. Here we set values specific to PPC CodeGen.
     *
     * A 32-bit field where the lower 24-bits contain an integer that represents an
     * approximate ordering of instructions.
     *
     * The upper 8 bits are used for flags.
     * Instruction flags encoded by their bit position.  Subclasses may use any
     * available bits between LastBaseFlag and MaxBaseFlag inclusive.
     */
    enum {
        WillBePatched = 0x08000000
    };

    bool willBePatched() { return (_index & WillBePatched) != 0; }

    void setWillBePatched(bool v = true) { v ? _index |= WillBePatched : _index &= ~WillBePatched; }

protected:
    virtual void fillBinaryEncodingFields(uint32_t *cursor);

private:
    //  TR::InstOpCode   _opcode;
    uint8_t _estimatedBinaryLength;
    TR::RegisterDependencyConditions *_conditions;
    bool _asyncBranch;
};

}} // namespace OMR::Power

#endif
