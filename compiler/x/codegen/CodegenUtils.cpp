/*******************************************************************************
 * Copyright IBM Corp. and others 2025
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

#include "CodegenUtils.hpp"
#include "OMRX86Instruction.hpp"

namespace OMR
{
namespace X86
{

void generateLoop(int32_t unrollFactor,
                  int32_t elementsPerIteration,
                  TR::Register *indexReg,
                  TR::Register *maxIndexReg,
                  TR::Node *node,
                  TR::CodeGenerator *cg,
                  std::function<void()> loopInitializerFunction,
                  std::function<void(int32_t)> genBodyFunction,
                  std::function<void(int32_t)> residueGenBodyFunction)
   {
   TR_ASSERT_FATAL((unrollFactor > 0) && ((unrollFactor & (unrollFactor - 1)) == 0), "Unroll count must be power of 2");
   TR_ASSERT_FATAL(elementsPerIteration == 1 ||
                   (elementsPerIteration > 0) && ((elementsPerIteration & (elementsPerIteration - 1)) == 0),
                   "elementsPerIteration must be 1 or a power of 2");
   TR_ASSERT_FATAL(unrollFactor <= 16, "Excessive unrolling detected (unrollFactor > 16)");

   TR::RegisterDependencyConditions *deps = generateRegisterDependencyConditions(0, 1, cg);
   TR::Register *loopLimitReg = cg->allocateRegister();
   TR::LabelSymbol *begLabel = generateLabelSymbol(cg);
   TR::LabelSymbol *endLabel = generateLabelSymbol(cg);
   TR::LabelSymbol *loopLabel = generateLabelSymbol(cg);
   int32_t numElements = elementsPerIteration * unrollFactor; /* Per iteration of unrolled main loop */

   deps->addPostCondition(loopLimitReg, RealRegister::NoReg, cg);
   begLabel->setStartInternalControlFlow();
   endLabel->setEndInternalControlFlow();

   generateLabelInstruction(InstOpCode::label, node, begLabel, cg);

   generateRegRegInstruction(InstOpCode::MOVRegReg(), node, loopLimitReg, maxIndexReg, cg);

   if (numElements != 1)
      {
      // Adjust loop bound to be a multiple of (elementsPerIteration * unrollFactor)
      // This ensures the main loop processes all elements up to the nearest multiple
      generateRegImmInstruction(InstOpCode::ANDRegImms(), node, loopLimitReg, ~(numElements - 1), cg);
      }

   generateRegRegInstruction(InstOpCode::CMPRegReg(), node, indexReg, loopLimitReg, cg);
   generateLabelInstruction(InstOpCode::JGE4, node, endLabel, cg);

   if (loopInitializerFunction)
      {
      // Call initialization logic before first iteration.
      // This might be needed if the initialization logic is too expensive
      // to run before checking if the main loop even executes.
      loopInitializerFunction();
      }

   generateLabelInstruction(InstOpCode::label, node, loopLabel, cg);

   for (int32_t i = 0; i < unrollFactor; i++)
      {
      // Unroll the loop by invoking the body function for each unroll iteration
      genBodyFunction(i);
      }

   // Update the loop index based on the number of elements processed in the unrolled main loop
   if (numElements == 1)
      {
      generateRegInstruction(InstOpCode::INCReg(), node, indexReg, cg);
      }
   else
      {
      generateRegImmInstruction(InstOpCode::ADDRegImm4(), node, indexReg, numElements, cg);
      }

   // Compare index with numElements and loop back if necessary
   generateRegRegInstruction(InstOpCode::CMP4RegReg, node, indexReg, loopLimitReg, cg);
   generateLabelInstruction(InstOpCode::JL4, node, loopLabel, cg);

   generateLabelInstruction(InstOpCode::label, node, endLabel, deps, cg);
   cg->stopUsingRegister(loopLimitReg);

   if (residueGenBodyFunction != NULL && numElements > 1)
      {
      TR_ASSERT_FATAL(residueGenBodyFunction, "Missing function to generate residue");

      // Generate a second loop to process residual iterations
      OMR::X86::generateLoop(1, 1, indexReg, maxIndexReg, node, cg, loopInitializerFunction, residueGenBodyFunction, NULL);
      }
   }

void generateLoop(int32_t begin,
                  int32_t end,
                  TR::Node *node,
                  TR::CodeGenerator *cg,
                  std::function<void(int32_t)> genBodyFunction)
   {
   TR::RegisterDependencyConditions *deps = generateRegisterDependencyConditions(0, 2, cg);

   TR::Register *indexReg = cg->allocateRegister();
   TR::Register *loopBoundReg = cg->allocateRegister();
   TR::LabelSymbol *label = generateLabelSymbol(cg);

   deps->addPostCondition(indexReg, RealRegister::NoReg, cg);
   deps->addPostCondition(loopBoundReg, RealRegister::NoReg, cg);

   TreeEvaluator::loadConstant(node, begin, TR_RematerializableInt, cg, indexReg);
   TreeEvaluator::loadConstant(node, end, TR_RematerializableInt, cg, loopBoundReg);

   OMR::X86::generateLoop(indexReg, loopBoundReg, node, cg, std::move(genBodyFunction));
   generateLabelInstruction(TR::InstOpCode::label, node, label, deps, cg);

   cg->stopUsingRegister(indexReg);
   cg->stopUsingRegister(loopBoundReg);
   }

}
}
