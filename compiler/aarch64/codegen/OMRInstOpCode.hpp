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

#ifndef OMR_ARM64_INSTOPCODE_INCL
#define OMR_ARM64_INSTOPCODE_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_INSTOPCODE_CONNECTOR
#define OMR_INSTOPCODE_CONNECTOR

namespace OMR {
namespace ARM64 {
class InstOpCode;
}

typedef OMR::ARM64::InstOpCode InstOpCodeConnector;
} // namespace OMR
#else
#error OMR::ARM64::InstOpCode expected to be a primary connector, but a OMR connector is already defined
#endif

#include "compiler/codegen/OMRInstOpCode.hpp"

namespace OMR { namespace ARM64 {

class InstOpCode : public OMR::InstOpCode {
protected:
    /**
     * @brief Constructor
     */
    InstOpCode()
        : OMR::InstOpCode(bad)
    {}

    /**
     * @brief Constructor
     * @param[in] m : mnemonic
     */
    InstOpCode(Mnemonic m)
        : OMR::InstOpCode(m)
    {}

public:
    enum AArch64BarrierLimitation {
        sy = 0xF, // Full System full barrier
        st = 0xE, // Full System write barrier
        ld = 0xD, // Full System read barrier
        ish = 0xB, // Inner Shareable full barrier
        ishst = 0xA, // Inner Shareable write barrier
        ishld = 0x9, // Inner Shareable read barrier
        nsh = 0x7, // Non-shareable full barrier
        nshst = 0x6, // Non-shareable write barrier
        nshld = 0x5, // Non-shareable read barrier
        osh = 0x3, // Outer Shareable full barrier
        oshst = 0x2, // Outer Shareable write barrier
        oshld = 0x1 // Outer Shareable load barrier
    };

    typedef uint32_t OpCodeBinaryEntry;
    static const OpCodeBinaryEntry binaryEncodings[ARM64NumOpCodes];

    /*
     * @brief Answers binary encoding of Mnemonic
     * @param[in] m : mnemonic
     * @return binary encoding
     */
    static const OpCodeBinaryEntry getOpCodeBinaryEncoding(Mnemonic m) { return binaryEncodings[m]; }

    /*
     * @brief Answers binary encoding of InstOpCode
     * @return binary encoding
     */
    const OpCodeBinaryEntry getOpCodeBinaryEncoding() { return getOpCodeBinaryEncoding(_mnemonic); }

    /*
     * @brief Copies binary encoding of the opcode to buffer
     * @param[in] cursor : instruction cursor
     * @return instruction cursor
     */
    uint8_t *copyBinaryToBuffer(uint8_t *cursor)
    {
        *(uint32_t *)cursor = *(uint32_t *)&binaryEncodings[_mnemonic];
        return cursor;
    }
};

}} // namespace OMR::ARM64
#endif
