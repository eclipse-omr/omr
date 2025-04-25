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

#ifndef X86RUNTIME_INCL
#define X86RUNTIME_INCL

#if defined(OMR_OS_WINDOWS)
#include <intrin.h>
#define cpuid(CPUInfo, EAXValue)             __cpuid(CPUInfo, EAXValue)
#define cpuidex(CPUInfo, EAXValue, ECXValue) __cpuidex(CPUInfo, EAXValue, ECXValue)
#else
#include <cpuid.h>
#include <emmintrin.h>
#define cpuid(CPUInfo, EAXValue)             __cpuid(EAXValue, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3])
#define cpuidex(CPUInfo, EAXValue, ECXValue) __cpuid_count(EAXValue, ECXValue, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3])
inline unsigned long long _xgetbv(unsigned int ecx)
   {
   unsigned int eax, edx;
   __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(ecx));
   return ((unsigned long long)edx << 32) | eax;
   }
#endif /* defined(OMR_OS_WINDOWS) */

char* feGetEnv(const char*);

inline void jitGetVendorID(char buf[13])
   {
   enum
      {
      EAX = 0,
      EBX = 1,
      ECX = 2,
      EDX = 3,
      };
   int CPUInfo[4];

   // EAX = 0
   cpuid(CPUInfo, 0);
   int* VendorID = (int*) buf;

   VendorID[0] = CPUInfo[EBX];
   VendorID[1] = CPUInfo[EDX];
   VendorID[2] = CPUInfo[ECX];

   buf[12] = 0; // Null terminate the string.
   }

inline bool AtomicCompareAndSwap(volatile uint32_t* ptr, uint32_t old_val, uint32_t new_val)
   {
#if defined(OMR_OS_WINDOWS)
   return old_val == (uint32_t)_InterlockedCompareExchange((volatile long*)ptr, (long)new_val, (long)old_val);
#else
   return __sync_bool_compare_and_swap(ptr, old_val, new_val);
#endif /* defined(OMR_OS_WINDOWS) */
   }

inline bool AtomicCompareAndSwap(volatile uint16_t* ptr, uint16_t old_val, uint16_t new_val)
   {
#if defined(OMR_OS_WINDOWS)
   return old_val == (uint16_t)_InterlockedCompareExchange16((volatile short*)ptr, (short)new_val, (short)old_val);
#else
   return __sync_bool_compare_and_swap(ptr, old_val, new_val);
#endif /* defined(OMR_OS_WINDOWS) */
   }

inline void patchingFence16(void* addr)
   {
   _mm_mfence();
   _mm_clflush(addr);
   _mm_clflush(static_cast<char*>(addr)+8);
   _mm_mfence();
   }

#endif
