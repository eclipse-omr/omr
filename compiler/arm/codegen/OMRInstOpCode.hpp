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

#ifndef OMR_ARM_INSTOPCODE_INCL
#define OMR_ARM_INSTOPCODE_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_INSTOPCODE_CONNECTOR
#define OMR_INSTOPCODE_CONNECTOR

namespace OMR {
namespace ARM {
class InstOpCode;
}

typedef OMR::ARM::InstOpCode InstOpCodeConnector;
} // namespace OMR
#else
#error OMR::ARM::InstOpCode expected to be a primary connector, but a OMR connector is already defined
#endif

#include "compiler/codegen/OMRInstOpCode.hpp"

namespace OMR { namespace ARM {

#define ARMOpProp_HasRecordForm 0x00000001
#define ARMOpProp_SetsCarryFlag 0x00000002
#define ARMOpProp_SetsOverflowFlag 0x00000004
#define ARMOpProp_ReadsCarryFlag 0x00000008
#define ARMOpProp_BranchOp 0x00000040
#define ARMOpProp_VFP 0x00000080
#define ARMOpProp_DoubleFP 0x00000100
#define ARMOpProp_SingleFP 0x00000200
#define ARMOpProp_UpdateForm 0x00000400
#define ARMOpProp_Arch4 0x00000800
#define ARMOpProp_IsRecordForm 0x00001000
#define ARMOpProp_Label 0x00002000
#define ARMOpProp_Arch6 0x00004000

class InstOpCode : public OMR::InstOpCode {
    typedef uint32_t TR_OpCodeBinaryEntry;

    static const uint32_t properties[NumOpCodes];
    static const TR_OpCodeBinaryEntry binaryEncodings[NumOpCodes];

protected:
    InstOpCode()
        : OMR::InstOpCode(bad)
    {}

    InstOpCode(Mnemonic m)
        : OMR::InstOpCode(m)
    {}

public:
    struct OpCodeMetaData {};

    static const OpCodeMetaData metadata[NumOpCodes];

    OMR::InstOpCode::Mnemonic getOpCodeValue() { return _mnemonic; }

    OMR::InstOpCode::Mnemonic setOpCodeValue(OMR::InstOpCode::Mnemonic op) { return (_mnemonic = op); }

    OMR::InstOpCode::Mnemonic getRecordFormOpCodeValue() { return (OMR::InstOpCode::Mnemonic)(_mnemonic + 1); }

    uint32_t isRecordForm() { return properties[_mnemonic] & ARMOpProp_IsRecordForm; }

    uint32_t hasRecordForm() { return properties[_mnemonic] & ARMOpProp_HasRecordForm; }

    uint32_t singleFPOp() { return properties[_mnemonic] & ARMOpProp_SingleFP; }

    uint32_t doubleFPOp() { return properties[_mnemonic] & ARMOpProp_DoubleFP; }

    uint32_t isVFPOp() { return properties[_mnemonic] & ARMOpProp_VFP; }

    uint32_t gprOp() { return (properties[_mnemonic] & (ARMOpProp_DoubleFP | ARMOpProp_SingleFP)) == 0; }

    uint32_t fprOp() { return (properties[_mnemonic] & (ARMOpProp_DoubleFP | ARMOpProp_SingleFP)); }

    uint32_t readsCarryFlag() { return properties[_mnemonic] & ARMOpProp_ReadsCarryFlag; }

    uint32_t setsCarryFlag() { return properties[_mnemonic] & ARMOpProp_SetsCarryFlag; }

    uint32_t setsOverflowFlag() { return properties[_mnemonic] & ARMOpProp_SetsOverflowFlag; }

    uint32_t isBranchOp() { return properties[_mnemonic] & ARMOpProp_BranchOp; }

    uint32_t isLabel() { return properties[_mnemonic] & ARMOpProp_Label; }

    uint8_t *copyBinaryToBuffer(uint8_t *cursor)
    {
        *(uint32_t *)cursor = *(uint32_t *)&binaryEncodings[_mnemonic];
        return cursor;
    }
};
}} // namespace OMR::ARM
#endif
