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

#ifndef OMR_ARM64_REAL_REGISTER_INCL
#define OMR_ARM64_REAL_REGISTER_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_REAL_REGISTER_CONNECTOR
#define OMR_REAL_REGISTER_CONNECTOR

namespace OMR {
namespace ARM64 {
class RealRegister;
}

typedef OMR::ARM64::RealRegister RealRegisterConnector;
} // namespace OMR
#else
#error OMR::ARM64::RealRegister expected to be a primary connector, but an OMR connector is already defined
#endif

#include "compiler/codegen/OMRRealRegister.hpp"
#include "infra/Annotations.hpp"

namespace TR {
class CodeGenerator;
}

namespace OMR { namespace ARM64 {

class OMR_EXTENSIBLE RealRegister : public OMR::RealRegister {
protected:
    /**
     * @param[in] cg : the TR::CodeGenerator object
     */
    RealRegister(TR::CodeGenerator *cg)
        : OMR::RealRegister(cg, NoReg)
    {}

    /**
     * @param[in] rk : kind of real register from TR_RegisterKinds
     * @param[in] w : register weight
     * @param[in] s : register state from RegState
     * @param[in] rn : register number from RegNum
     * @param[in] m : register mask from RegMask
     * @param[in] cg : the TR::CodeGenerator object
     */
    RealRegister(TR_RegisterKinds rk, uint16_t w, RegState s, RegNum rn, RegMask m, TR::CodeGenerator *cg)
        : OMR::RealRegister(rk, w, s, (uint16_t)0, rn, m, cg)
    {}

public:
    typedef enum {
        pos_RD = 0,
        pos_RN = 5,
        pos_RM = 16,
        pos_RT = 0,
        pos_RT2 = 10,
        pos_RS = 16,
        pos_RA = 10
    } ARM64OperandPosition;

    /**
     * @brief Set the RealRegister in the Rd field of the specified instruction
     * @param[in] instruction : target instruction
     */
    void setRegisterFieldRD(uint32_t *instruction)
    {
        *instruction |= fullRegBinaryEncodings[_registerNumber] << pos_RD;
    }

    /**
     * @brief Set the RealRegister in the Rn field of the specified instruction
     * @param[in] instruction : target instruction
     */
    void setRegisterFieldRN(uint32_t *instruction)
    {
        *instruction |= fullRegBinaryEncodings[_registerNumber] << pos_RN;
    }

    /**
     * @brief Set the RealRegister in the Rm field of the specified instruction
     * @param[in] instruction : target instruction
     */
    void setRegisterFieldRM(uint32_t *instruction)
    {
        *instruction |= fullRegBinaryEncodings[_registerNumber] << pos_RM;
    }

    /**
     * @brief Set the RealRegister in the Rt field of the specified instruction
     * @param[in] instruction : target instruction
     */
    void setRegisterFieldRT(uint32_t *instruction)
    {
        *instruction |= fullRegBinaryEncodings[_registerNumber] << pos_RT;
    }

    /**
     * @brief Set the RealRegister in the Rt2 field of the specified instruction
     * @param[in] instruction : target instruction
     */
    void setRegisterFieldRT2(uint32_t *instruction)
    {
        *instruction |= fullRegBinaryEncodings[_registerNumber] << pos_RT2;
    }

    /**
     * @brief Set the RealRegister in the Rs field of the specified instruction
     * @param[in] instruction : target instruction
     */
    void setRegisterFieldRS(uint32_t *instruction)
    {
        *instruction |= fullRegBinaryEncodings[_registerNumber] << pos_RS;
    }

    /**
     * @brief Set the RealRegister in the Ra field of the specified instruction
     * @param[in] instruction : target instruction
     */
    void setRegisterFieldRA(uint32_t *instruction)
    {
        *instruction |= fullRegBinaryEncodings[_registerNumber] << pos_RA;
    }

    /**
     * @brief Returns register mask for available registers of specified kind
     * @param[in] rk : register kind
     * @returns register mask
     */
    static TR_RegisterMask getAvailableRegistersMask(TR_RegisterKinds rk);

    /**
     * @brief Returns real register for the passed register mask
     * @param[in] mask : register mask
     * @param[in] rk : register kind
     * @param[in] cg : code generator
     * @returns real register
     */
    static TR::RealRegister *regMaskToRealRegister(TR_RegisterMask mask, TR_RegisterKinds rk, TR::CodeGenerator *cg);

private:
    static const uint8_t fullRegBinaryEncodings[NumRegisters];
};

}} // namespace OMR::ARM64

#endif
