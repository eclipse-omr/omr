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

#include <stdlib.h>
#include <string.h>
#include "env/CPU.hpp"
#include "env/CompilerEnv.hpp"
#include "infra/Bit.hpp"
#include "infra/Flags.hpp"
#include "x/runtime/X86Runtime.hpp"
#include "codegen/CodeGenerator.hpp"

TR::CPU
OMR::X86::CPU::detect(OMRPortLibrary * const omrPortLib)
   {
   if (omrPortLib == NULL)
      return TR::CPU();

   OMRPORT_ACCESS_FROM_OMRPORT(omrPortLib);
   OMRProcessorDesc processorDescription;
   omrsysinfo_get_processor_description(&processorDescription);

   if (!omrsysinfo_processor_has_feature(&processorDescription, OMR_FEATURE_X86_XSAVE_AVX))
      {
      // Unset AVX/AVX2 if not enabled via XCR0 or otherwise disabled
      omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_AVX, FALSE);
      omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_AVX2, FALSE);
      }

   if (!omrsysinfo_processor_has_feature(&processorDescription, OMR_FEATURE_X86_XSAVE_AVX512))
      {
      // Unset AVX-512 if not enabled via XCR0 or otherwise disabled
      // If other AVX-512 extensions are supported in the port library, they need to be disabled here
      omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_AVX512F, FALSE);
      omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_AVX512VL, FALSE);
      omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_AVX512BW, FALSE);
      omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_AVX512CD, FALSE);
      omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_AVX512DQ, FALSE);
      omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_AVX512_BITALG, FALSE);
      omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_AVX512_VBMI, FALSE);
      omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_AVX512_VBMI2, FALSE);
      omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_AVX512_VNNI, FALSE);
      omrsysinfo_processor_set_feature(&processorDescription, OMR_FEATURE_X86_AVX512_VPOPCNTDQ, FALSE);
      }

   return TR::CPU::customize(processorDescription);
   }

OMRProcessorDesc OMR::X86::CPU::getEnabledFeatures()
   {
   // Only enable the features that compiler currently uses
   const uint32_t enabledFeatures[] = {
      OMR_FEATURE_X86_FPU, OMR_FEATURE_X86_CX8, OMR_FEATURE_X86_CMOV,
      OMR_FEATURE_X86_POPCNT, OMR_FEATURE_X86_AESNI, OMR_FEATURE_X86_OSXSAVE,
      OMR_FEATURE_X86_FMA, OMR_FEATURE_X86_HLE, OMR_FEATURE_X86_RTM, OMR_FEATURE_X86_BMI2,
      OMR_FEATURE_X86_MMX, OMR_FEATURE_X86_SSE, OMR_FEATURE_X86_SSE2,
      OMR_FEATURE_X86_SSE3, OMR_FEATURE_X86_SSSE3, OMR_FEATURE_X86_SSE4_1,
      OMR_FEATURE_X86_SSE4_2, OMR_FEATURE_X86_AVX, OMR_FEATURE_X86_AVX2,
      OMR_FEATURE_X86_AVX512F, OMR_FEATURE_X86_AVX512VL, OMR_FEATURE_X86_AVX512BW,
      OMR_FEATURE_X86_AVX512DQ, OMR_FEATURE_X86_AVX512CD, OMR_FEATURE_X86_AVX512_VBMI2,
      OMR_FEATURE_X86_AVX512_VPOPCNTDQ, OMR_FEATURE_X86_AVX512_BITALG, OMR_FEATURE_X86_CLWB
   };

   OMRProcessorDesc featureMasks;
   memset(featureMasks.features, 0, OMRPORT_SYSINFO_FEATURES_SIZE * sizeof(uint32_t));

   for (size_t i = 0; i < sizeof(enabledFeatures) / sizeof(uint32_t); i++)
      {
      TR_ASSERT_FATAL(enabledFeatures[i] < OMRPORT_SYSINFO_FEATURES_SIZE * sizeof(uint32_t) * 8, "Illegal cpu feature mask");
      featureMasks.features[i / sizeof(uint32_t)] |= enabledFeatures[i] % sizeof(uint32_t);
      }

   return featureMasks;
   }

TR::CPU
OMR::X86::CPU::customize(OMRProcessorDesc processorDescription)
   {
   OMRProcessorDesc featureMasks = getEnabledFeatures();

   for (size_t i = 0; i < OMRPORT_SYSINFO_FEATURES_SIZE; i++)
      {
      processorDescription.features[i] &= featureMasks.features[i];
      }

   return TR::CPU(processorDescription);
   }

const char *
OMR::X86::CPU::getX86ProcessorVendorId()
   {
   static char buf[13];
   static bool init = false;

   if (!init)
      {
      jitGetVendorID(buf);
      init = true;
      }

   return buf;
   }

bool
OMR::X86::CPU::getSupportsHardwareSQRT()
   {
   return true;
   }

bool
OMR::X86::CPU::hasPopulationCountInstruction()
   {
   return self()->supportsFeature(OMR_FEATURE_X86_POPCNT);
   }

bool
OMR::X86::CPU::supportsTransactionalMemoryInstructions()
   {
   return self()->supportsFeature(OMR_FEATURE_X86_RTM);
   }

bool
OMR::X86::CPU::getSupportsHardware32bitCompress()
   {
   return self()->supportsFeature(OMR_FEATURE_X86_BMI2);
   }

bool
OMR::X86::CPU::getSupportsHardware64bitCompress()
   {
   if (self()->isI386())
      return false;

   return self()->supportsFeature(OMR_FEATURE_X86_BMI2);
   }

bool
OMR::X86::CPU::getSupportsHardware32bitExpand()
   {
   return self()->supportsFeature(OMR_FEATURE_X86_BMI2);
   }

bool
OMR::X86::CPU::getSupportsHardware64bitExpand()
   {
   if (self()->isI386())
      return false;

   return self()->supportsFeature(OMR_FEATURE_X86_BMI2);
   }

bool
OMR::X86::CPU::isGenuineIntel()
   {
   if (TR::Compiler->omrPortLib == NULL)
      {
      const char *vendor = self()->getX86ProcessorVendorId();
      return strncmp(vendor, "GenuineIntel", 12) == 0;
      }

   return self()->isAtLeast(OMR_PROCESSOR_X86_INTEL_FIRST) && self()->isAtMost(OMR_PROCESSOR_X86_INTEL_LAST);
   }

bool
OMR::X86::CPU::isAuthenticAMD()
   {
   if (TR::Compiler->omrPortLib == NULL)
      {
      const char *vendor = self()->getX86ProcessorVendorId();
      return strncmp(vendor, "AuthenticAMD", 12) == 0;
      }

   return self()->isAtLeast(OMR_PROCESSOR_X86_AMD_FIRST) && self()->isAtMost(OMR_PROCESSOR_X86_AMD_LAST);
   }

bool
OMR::X86::CPU::requiresLFence()
   {
   return false;  /* Dummy for now, we may need TR::InstOpCode::LFENCE in future processors*/
   }

bool
OMR::X86::CPU::supportsMFence()
   {
   return self()->supportsFeature(OMR_FEATURE_X86_SSE2);
   }

bool
OMR::X86::CPU::supportsLFence()
   {
   return self()->supportsFeature(OMR_FEATURE_X86_SSE2);
   }

bool
OMR::X86::CPU::supportsSFence()
   {
   return self()->supportsFeature(OMR_FEATURE_X86_SSE2) || self()->supportsFeature(OMR_FEATURE_X86_MMX);
   }

bool
OMR::X86::CPU::prefersMultiByteNOP()
   {
   return self()->isGenuineIntel() && !self()->is(OMR_PROCESSOR_X86_INTEL_PENTIUM);
   }

bool
OMR::X86::CPU::supportsAVX()
   {
   return self()->supportsFeature(OMR_FEATURE_X86_AVX) && self()->supportsFeature(OMR_FEATURE_X86_OSXSAVE);
   }

bool
OMR::X86::CPU::is(OMRProcessorArchitecture p)
   {
   return _processorDescription.processor == p;
   }

bool
OMR::X86::CPU::isFeatureDisabledByOption(uint32_t feature)
   {
   TR_CompilationOptions option = (TR_CompilationOptions) 0;

   switch (feature)
      {
      case OMR_FEATURE_X86_SSE3:
         option = TR_DisableSSE3;
         break;
      case OMR_FEATURE_X86_SSE4_1:
         option = TR_DisableSSE4_1;
         break;
      case OMR_FEATURE_X86_SSE4_2:
         option = TR_DisableSSE4_2;
         break;
      case OMR_FEATURE_X86_AVX:
         option = TR_DisableAVX;
         break;
      case OMR_FEATURE_X86_AVX2:
         option = TR_DisableAVX2;
         break;
      case OMR_FEATURE_X86_AVX512F:
      case OMR_FEATURE_X86_AVX512VL:
      case OMR_FEATURE_X86_AVX512BW:
      case OMR_FEATURE_X86_AVX512CD:
      case OMR_FEATURE_X86_AVX512DQ:
      case OMR_FEATURE_X86_AVX512ER:
      case OMR_FEATURE_X86_AVX512PF:
      case OMR_FEATURE_X86_AVX512_BITALG:
      case OMR_FEATURE_X86_AVX512_IFMA:
      case OMR_FEATURE_X86_AVX512_VBMI:
      case OMR_FEATURE_X86_AVX512_VBMI2:
      case OMR_FEATURE_X86_AVX512_VNNI:
      case OMR_FEATURE_X86_AVX512_VPOPCNTDQ:
         option = TR_DisableAVX512;
         break;
      default:
         return false;
      }

   if (!_comp)
      {
      // Lazy initialize thread local compilation object
      _comp = TR::comp();
      }

   return _comp && _comp->getOption(option);
   }

bool
OMR::X86::CPU::supportsFeature(uint32_t feature)
   {
   if (isFeatureDisabledByOption(feature))
      return false;

   if (TR::Compiler->omrPortLib == NULL)
      return false; // TODO; return true for minimum target features;

   OMRPORT_ACCESS_FROM_OMRPORT(TR::Compiler->omrPortLib);
   return TRUE == omrsysinfo_processor_has_feature(&_processorDescription, feature);
   }

const char*
OMR::X86::CPU::getProcessorName()
   {
   const char* returnString = "";
   switch(_processorDescription.processor)
      {
      case OMR_PROCESSOR_X86_INTEL_PENTIUM:
         returnString = "X86 Intel Pentium";
         break;

      case OMR_PROCESSOR_X86_INTEL_P6:
         returnString = "X86 Intel P6";
         break;

      case OMR_PROCESSOR_X86_INTEL_PENTIUM4:
         returnString = "X86 Intel Netburst Microarchitecture";
         break;

      case OMR_PROCESSOR_X86_INTEL_CORE2:
         returnString = "X86 Intel Core2 Microarchitecture";
         break;

      case OMR_PROCESSOR_X86_INTEL_TULSA:
         returnString = "X86 Intel Tulsa";
         break;

      case OMR_PROCESSOR_X86_INTEL_NEHALEM:
         returnString = "X86 Intel Nehalem";
         break;

      case OMR_PROCESSOR_X86_INTEL_WESTMERE:
         returnString = "X86 Intel Westmere";
         break;

      case OMR_PROCESSOR_X86_INTEL_SANDYBRIDGE:
         returnString = "X86 Intel Sandy Bridge";
         break;

      case OMR_PROCESSOR_X86_INTEL_IVYBRIDGE:
         returnString = "X86 Intel Ivy Bridge";
         break;

      case OMR_PROCESSOR_X86_INTEL_HASWELL:
         returnString = "X86 Intel Haswell";
         break;

      case OMR_PROCESSOR_X86_INTEL_BROADWELL:
         returnString = "X86 Intel Broadwell";
         break;

      case OMR_PROCESSOR_X86_INTEL_SKYLAKE:
         returnString = "X86 Intel Skylake";
         break;

      case OMR_PROCESSOR_X86_INTEL_CASCADELAKE:
         returnString = "X86 Intel Cascade Lake";
         break;

      case OMR_PROCESSOR_X86_INTEL_COOPERLAKE:
         returnString = "X86 Intel Cooper Lake";
         break;

      case OMR_PROCESSOR_X86_INTEL_ICELAKE:
         returnString = "X86 Intel Ice Lake";
         break;

      case OMR_PROCESSOR_X86_INTEL_SAPPHIRERAPIDS:
         returnString = "X86 Intel Sapphire Rapids";
         break;

      case OMR_PROCESSOR_X86_INTEL_EMERALDRAPIDS:
         returnString = "X86 Intel Emerald Rapids";
         break;

      case OMR_PROCESSOR_X86_AMD_K5:
         returnString = "X86 AMDK5";
         break;

      case OMR_PROCESSOR_X86_AMD_K6:
         returnString = "X86 AMDK6";
         break;

      case OMR_PROCESSOR_X86_AMD_ATHLONDURON:
         returnString = "X86 AMD Athlon-Duron";
         break;

      case OMR_PROCESSOR_X86_AMD_OPTERON:
         returnString = "X86 AMD Opteron";
         break;

      case OMR_PROCESSOR_X86_AMD_FAMILY15H:
         returnString = "X86 AMD Family 15h";
         break;

      default:
         returnString = "Unknown X86 Processor";
         break;
      }
   return returnString;
   }
