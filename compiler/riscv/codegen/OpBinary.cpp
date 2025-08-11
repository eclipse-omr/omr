/*******************************************************************************
 * Copyright IBM Corp. and others 2019
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

#include "codegen/InstOpCode.hpp"

const OMR::RV::InstOpCode::OpCodeBinaryEntry OMR::RV::InstOpCode::binaryEncodings[RVNumOpCodes] = {
    //		BINARY			Opcode    	Opcode		comments
    /* UNALLOCATED */
    0x00000000, /* Register Association */
    0x00000000, /* Bad Opcode */
    0x00000000, /* Define Doubleword */
    0x00000000, /* Fence */
    0x00000000, /* Destination of a jump */
    0x00000000, /* Entry to the method */
    0x00000000, /* Return */
    0x00000000, /* Virtual Guard NOP */
/*
 * RISC-V instructions
 */
#define DECLARE_INSN(mnemonic, match, mask) match,
#include <riscv-opc.h>
#undef DECLARE_INSN
};
