/*******************************************************************************
 * Copyright IBM Corp. and others 2023
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

#ifndef RVINSTRUCTIONUTILS_INCL
#define RVINSTRUCTIONUTILS_INCL

#include <stddef.h>
#include <stdint.h>
#include <riscv.h>

#include "codegen/Register.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/InstOpCode.hpp"

#define RISCV_INSTRUCTION_LENGTH 4

/**
 * ==== R-type ====
 */

static inline uint32_t TR_RISCV_RTYPE(uint32_t insn, uint32_t rd, uint32_t rs1, uint32_t rs2)
{
    return ((insn) | ((rd) << OP_SH_RD) | ((rs1) << OP_SH_RS1) | ((rs2) << OP_SH_RS2));
}

static inline uint32_t TR_RISCV_RTYPE(TR::InstOpCode::Mnemonic insn, TR::RealRegister::RegNum rd,
    TR::RealRegister::RegNum rs1, TR::RealRegister::RegNum rs2)
{
    return TR_RISCV_RTYPE(TR::InstOpCode::getOpCodeBinaryEncoding(insn), TR::RealRegister::binaryRegCode(rd),
        TR::RealRegister::binaryRegCode(rs1), TR::RealRegister::binaryRegCode(rs2));
}

static inline uint32_t TR_RISCV_RTYPE(TR::InstOpCode::Mnemonic insn, TR::Register *rd, TR::Register *rs1,
    TR::Register *rs2)
{
    return TR_RISCV_RTYPE(insn, toRealRegister(rd)->getRegisterNumber(), toRealRegister(rs1)->getRegisterNumber(),
        toRealRegister(rs2)->getRegisterNumber());
}

/**
 * ==== I-type ====
 */

static inline uint32_t TR_RISCV_ITYPE(uint32_t insn, uint32_t rd, uint32_t rs1, uint32_t imm)
{
    return ((insn) | ((rd) << OP_SH_RD) | ((rs1) << OP_SH_RS1) | ENCODE_ITYPE_IMM(imm));
}

static inline uint32_t TR_RISCV_ITYPE(TR::InstOpCode::Mnemonic insn, TR::RealRegister::RegNum rd,
    TR::RealRegister::RegNum rs1, uint32_t imm)
{
    return TR_RISCV_ITYPE(TR::InstOpCode::getOpCodeBinaryEncoding(insn), TR::RealRegister::binaryRegCode(rd),
        TR::RealRegister::binaryRegCode(rs1), imm);
}

static inline uint32_t TR_RISCV_ITYPE(TR::InstOpCode::Mnemonic insn, TR::Register *rd, TR::Register *rs1, uint32_t imm)
{
    return TR_RISCV_ITYPE(insn, toRealRegister(rd)->getRegisterNumber(), toRealRegister(rs1)->getRegisterNumber(), imm);
}

/**
 * ==== S-type ====
 */

static inline uint32_t TR_RISCV_STYPE(uint32_t insn, uint32_t rs1, uint32_t rs2, uint32_t imm)
{
    return ((insn) | ((rs1) << OP_SH_RS1) | ((rs2) << OP_SH_RS2) | ENCODE_STYPE_IMM(imm));
}

static inline uint32_t TR_RISCV_STYPE(TR::InstOpCode::Mnemonic insn, TR::RealRegister::RegNum rs1,
    TR::RealRegister::RegNum rs2, uint32_t imm)
{
    return TR_RISCV_STYPE(TR::InstOpCode::getOpCodeBinaryEncoding(insn), TR::RealRegister::binaryRegCode(rs1),
        TR::RealRegister::binaryRegCode(rs2), imm);
}

static inline uint32_t TR_RISCV_STYPE(TR::InstOpCode::Mnemonic insn, TR::Register *rs1, TR::Register *rs2, uint32_t imm)
{
    return TR_RISCV_STYPE(insn, toRealRegister(rs1)->getRegisterNumber(), toRealRegister(rs2)->getRegisterNumber(),
        imm);
}

/**
 * ==== B-type ====
 */

static inline uint32_t TR_RISCV_SBTYPE(uint32_t insn, uint32_t rs1, uint32_t rs2, uint32_t imm)
{
    return ((insn) | ((rs1) << OP_SH_RS1) | ((rs2) << OP_SH_RS2) | ENCODE_SBTYPE_IMM(imm));
}

static inline uint32_t TR_RISCV_SBTYPE(TR::InstOpCode::Mnemonic insn, TR::RealRegister::RegNum rs1,
    TR::RealRegister::RegNum rs2, uint32_t imm)
{
    return TR_RISCV_SBTYPE(TR::InstOpCode::getOpCodeBinaryEncoding(insn), TR::RealRegister::binaryRegCode(rs1),
        TR::RealRegister::binaryRegCode(rs2), imm);
}

static inline uint32_t TR_RISCV_SBTYPE(TR::InstOpCode::Mnemonic insn, TR::Register *rs1, TR::Register *rs2,
    uint32_t imm)
{
    return TR_RISCV_SBTYPE(insn, toRealRegister(rs1)->getRegisterNumber(), toRealRegister(rs2)->getRegisterNumber(),
        imm);
}

/**
 * ==== U-type ====
 */

static inline uint32_t TR_RISCV_UTYPE(uint32_t insn, uint32_t rd, uint32_t bigimm)
{
    return ((insn) | ((rd) << OP_SH_RD) | ENCODE_UTYPE_IMM(bigimm));
}

static inline uint32_t TR_RISCV_UTYPE(TR::InstOpCode::Mnemonic insn, TR::RealRegister::RegNum rd, uint32_t bigimm)
{
    return TR_RISCV_UTYPE(TR::InstOpCode::getOpCodeBinaryEncoding(insn), TR::RealRegister::binaryRegCode(rd), bigimm);
}

static inline uint32_t TR_RISCV_UTYPE(TR::InstOpCode::Mnemonic insn, TR::Register *rd, uint32_t bigimm)
{
    return TR_RISCV_UTYPE(insn, toRealRegister(rd)->getRegisterNumber(), bigimm);
}

/**
 * ==== J-type ====
 */

static inline uint32_t TR_RISCV_UJTYPE(uint32_t insn, uint32_t rd, uint32_t target)
{
    return ((insn) | ((rd) << OP_SH_RD) | ENCODE_UJTYPE_IMM(target));
}

static inline uint32_t TR_RISCV_UJTYPE(TR::InstOpCode::Mnemonic insn, TR::RealRegister::RegNum rd, uint32_t bigimm)
{
    return TR_RISCV_UJTYPE(TR::InstOpCode::getOpCodeBinaryEncoding(insn), TR::RealRegister::binaryRegCode(rd), bigimm);
}

static inline uint32_t TR_RISCV_UJTYPE(TR::InstOpCode::Mnemonic insn, TR::Register *rd, uint32_t target)
{
    return TR_RISCV_UJTYPE(insn, toRealRegister(rd)->getRegisterNumber(), target);
}

static inline uint32_t TR_RISCV_UJTYPE(TR::InstOpCode &insn, TR::Register *rd, uint32_t target)
{
    return TR_RISCV_UJTYPE(insn.getMnemonic(), rd, target);
}

#endif // RVINSTRUCTIONUTILS_INCL
