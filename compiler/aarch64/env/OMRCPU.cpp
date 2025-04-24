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

#include "env/CPU.hpp"
#include "env/CompilerEnv.hpp"
#include "env/jittypes.h"
#include "omrport.h"

OMRProcessorDesc OMR::ARM64::CPU::getEnabledFeatures()
   {
   // Only enable the features that compiler currently uses
   const uint32_t enabledFeatures [] = {OMR_FEATURE_ARM64_FP, OMR_FEATURE_ARM64_ASIMD, OMR_FEATURE_ARM64_CRC32, OMR_FEATURE_ARM64_LSE};

   OMRProcessorDesc featureMasks;
   memset(featureMasks.features, 0, OMRPORT_SYSINFO_FEATURES_SIZE * sizeof(uint32_t));

   for (size_t i = 0; i < sizeof(enabledFeatures) / sizeof(uint32_t); i++)
      {
      TR_ASSERT_FATAL(enabledFeatures[i] < OMRPORT_SYSINFO_FEATURES_SIZE * sizeof(uint32_t) * 8, "Illegal cpu feature mask");
      featureMasks.features[i / sizeof(uint32_t)] |= enabledFeatures[i] % sizeof(uint32_t);
      }

   return featureMasks;
   }

bool
OMR::ARM64::CPU::isTargetWithinUnconditionalBranchImmediateRange(intptr_t targetAddress, intptr_t sourceAddress)
   {
   intptr_t range = targetAddress - sourceAddress;
   return range <= self()->maxUnconditionalBranchImmediateForwardOffset() &&
          range >= self()->maxUnconditionalBranchImmediateBackwardOffset();
   }

bool
OMR::ARM64::CPU::supportsFeature(uint32_t feature)
   {
   if (TR::Compiler->omrPortLib == NULL)
      {
      return false;
      }

   OMRPORT_ACCESS_FROM_OMRPORT(TR::Compiler->omrPortLib);
   return (TRUE == omrsysinfo_processor_has_feature(&_processorDescription, feature));
   }

const char*
OMR::ARM64::CPU::getProcessorName()
   {
   const char* returnString = "";
   switch(_processorDescription.processor)
      {
      case OMR_PROCESSOR_ARM64_UNKNOWN:
         returnString = "Unknown ARM64 processor";
         break;
      case OMR_PROCESSOR_ARM64_V8_A:
         returnString = "ARMv8-A processor";
         break;
      default:
         returnString = "Unknown ARM64 processor";
         break;
      }
   return returnString;
   }
