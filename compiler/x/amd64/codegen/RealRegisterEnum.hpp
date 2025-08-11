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

NoReg = 0,

    // The order of the GPRS registers is defined by the linkage
    // convention, and the VM may rely on it (eg. for GC).
    eax = 1, FirstGPR = eax, ebx = 2, ecx = 3, edx = 4, edi = 5, esi = 6, ebp = 7, esp = 8, r8 = 9, r9 = 10, r10 = 11,
    r11 = 12, r12 = 13, r13 = 14, r14 = 15, r15 = 16, LastGPR = r15, Last8BitGPR = r15, LastAssignableGPR = r15,

    vfp = 17,

    FPRMaskOffset = LastGPR, st0 = 18, FirstFPR = st0, st1 = 19, st2 = 20, st3 = 21, st4 = 22, st5 = 23, st6 = 24,
    st7 = 25, LastFPR = st7, LastAssignableFPR = st7,

    XMMRMaskOffset = LastGPR, xmm0 = 26, FirstXMMR = xmm0, xmm1 = 27, xmm2 = 28, xmm3 = 29, xmm4 = 30, xmm5 = 31,
    xmm6 = 32, xmm7 = 33, xmm8 = 34, xmm9 = 35, xmm10 = 36, FirstSpillReg = xmm10, xmm11 = 37, xmm12 = 38, xmm13 = 39,
    xmm14 = 40, xmm15 = 41, LastSpillReg = xmm15, LastXMMR = xmm15,

    YMMRMaskOffset = LastGPR, ymm0 = xmm0, FirstYMMR = ymm0, ymm1 = xmm1, ymm2 = xmm2, ymm3 = xmm3, ymm4 = xmm4,
    ymm5 = xmm5, ymm6 = xmm6, ymm7 = xmm7, ymm8 = xmm8, ymm9 = xmm9, ymm10 = xmm10, ymm11 = xmm11, ymm12 = xmm12,
    ymm13 = xmm13, ymm14 = xmm14, ymm15 = xmm15, LastYMMR = ymm15,

    ZMMRMaskOffset = LastGPR, zmm0 = xmm0, FirstZMMR = zmm0, zmm1 = xmm1, zmm2 = xmm2, zmm3 = xmm3, zmm4 = xmm4,
    zmm5 = xmm5, zmm6 = xmm6, zmm7 = xmm7, zmm8 = xmm8, zmm9 = xmm9, zmm10 = xmm10, zmm11 = xmm11, zmm12 = xmm12,
    zmm13 = xmm13, zmm14 = xmm14, zmm15 = xmm15, LastZMMR = zmm15,

    // avx512 write mask registers
    // k0 -> reserved for unmasked operations
    k0 = 42, k1 = 43, k2 = 44, k3 = 45, k4 = 46, k5 = 47, k6 = 48, k7 = 49,

    AllFPRegisters = 50, ByteReg = 51, BestFreeReg = 52, SpilledReg = 53, NumRegisters = 54,

    NumXMMRegisters = LastXMMR - FirstXMMR + 1,
    MaxAssignableRegisters = NumXMMRegisters + (LastAssignableGPR - FirstGPR + 1) - 1 // -1 for stack pointer
