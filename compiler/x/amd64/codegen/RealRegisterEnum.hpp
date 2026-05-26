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

/*
 * This file will be included within an enum.  Only comments and enumerator
 * definitions are permitted.
 */

// Pseudo register indicating any register
//
NoReg = 0,

    // Some consumers of OMR may rely on this register ordering (e.g., for
    // register masks for live references for garbage collection), so be
    // mindful of altering it.
    //
    eax = 1, FirstGPR = eax, ebx = 2, ecx = 3, edx = 4, edi = 5, esi = 6, ebp = 7, esp = 8, r8 = 9, r9 = 10, r10 = 11,
    r11 = 12, r12 = 13, r13 = 14, r14 = 15, r15 = 16, LastGPR = r15, Last8BitGPR = r15, LastAssignableGPR = r15,

    NumGPRs = LastGPR - FirstGPR + 1,

    // Virtual frame pointer used as a placeholder for frame references before
    // their offsets are known for rsp mapping
    //
    vfp = 17,

    // st0Return is used for 32-bit linkages where floating point values are
    // returned in x87 register st0. It is a special register that cannot be
    // assigned.
    //
    st0Return = 18,

    xmm0 = 19, FirstXMMR = xmm0, xmm1 = 20, xmm2 = 21, xmm3 = 22, xmm4 = 23, xmm5 = 24, xmm6 = 25, xmm7 = 26, xmm8 = 27,
    xmm9 = 28, xmm10 = 29, xmm11 = 30, xmm12 = 31, xmm13 = 32, xmm14 = 33, xmm15 = 34, LastXMMR = xmm15,

    NumXMMRs = LastXMMR - FirstXMMR + 1,

    // Alias ymm registers with xmm registers as they architecturally overlap
    //
    ymm0 = xmm0, ymm1 = xmm1, ymm2 = xmm2, ymm3 = xmm3, ymm4 = xmm4, ymm5 = xmm5, ymm6 = xmm6, ymm7 = xmm7, ymm8 = xmm8,
    ymm9 = xmm9, ymm10 = xmm10, ymm11 = xmm11, ymm12 = xmm12, ymm13 = xmm13, ymm14 = xmm14, ymm15 = xmm15,

    // Alias zmm registers with xmm registers as they architecturally overlap
    //
    zmm0 = xmm0, zmm1 = xmm1, zmm2 = xmm2, zmm3 = xmm3, zmm4 = xmm4, zmm5 = xmm5, zmm6 = xmm6, zmm7 = xmm7, zmm8 = xmm8,
    zmm9 = xmm9, zmm10 = xmm10, zmm11 = xmm11, zmm12 = xmm12, zmm13 = xmm13, zmm14 = xmm14, zmm15 = xmm15,

    // AVX-512 write mask registers
    //
    k0 = 35, FirstVMR = k0, k1 = 36, k2 = 37, k3 = 38, k4 = 39, k5 = 40, k6 = 41, k7 = 42, LastVMR = k7,

    NumVMRs = LastVMR - FirstVMR + 1,

    // Pseudo register indicating any byte register
    //
    ByteReg = 43,

    // Pseudo register indicating a register in spill state
    //
    SpilledReg = 44,

    NumRegisters = 45,
