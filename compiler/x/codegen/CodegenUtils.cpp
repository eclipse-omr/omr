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

#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "x/codegen/CodegenUtils.hpp"

extern bool existsNextInstructionToTestFlags(TR::Instruction *startInstr,
                                             uint8_t testMask);

namespace OMR
{
namespace X86
{

TR::Instruction *insertLoadConstant(TR::Node *node,
                                    TR::Register *target,
                                    intptr_t value,
                                    TR_RematerializableTypes type,
                                    TR::CodeGenerator *cg,
                                    TR::Instruction *currentInstruction)
   {
   TR::Compilation *comp = cg->comp();
   static const TR::InstOpCode::Mnemonic ops[TR_NumRematerializableTypes+1][3] =
      //    load 0      load -1     load c
      { { TR::InstOpCode::UD2,  TR::InstOpCode::UD2,  TR::InstOpCode::UD2   },   // LEA; should not seen here
        { TR::InstOpCode::XOR4RegReg, TR::InstOpCode::OR4RegImms, TR::InstOpCode::MOV4RegImm4 },   // Byte constant
        { TR::InstOpCode::XOR4RegReg, TR::InstOpCode::OR4RegImms, TR::InstOpCode::MOV4RegImm4 },   // Short constant
        { TR::InstOpCode::XOR4RegReg, TR::InstOpCode::OR4RegImms, TR::InstOpCode::MOV4RegImm4 },   // Char constant
        { TR::InstOpCode::XOR4RegReg, TR::InstOpCode::OR4RegImms, TR::InstOpCode::MOV4RegImm4 },   // Int constant
        { TR::InstOpCode::XOR4RegReg, TR::InstOpCode::OR4RegImms, TR::InstOpCode::MOV4RegImm4 },   // 32-bit address constant
        { TR::InstOpCode::XOR4RegReg, TR::InstOpCode::OR8RegImms, TR::InstOpCode::UD2   } }; // Long address constant; MOVs handled specially

   enum { XOR = 0, OR  = 1, MOV = 2 };

   bool is64Bit = false;

   int opsRow = type;
   if (cg->comp()->target().is64Bit())
      {
      if (type == TR_RematerializableAddress)
         {
         // Treat 64-bit addresses as longs
         opsRow++;
         is64Bit = true;
         }
      else
         {
         is64Bit = (type == TR_RematerializableLong);
         }
      }
   else
      {
      TR_ASSERT(type != TR_RematerializableLong, "Longs are rematerialized as pairs of ints on IA32");
      }

   TR_ExternalRelocationTargetKind reloKind = TR_NoRelocation;
   if (cg->profiledPointersRequireRelocation() && node && node->getOpCodeValue() == TR::aconst &&
         (node->isClassPointerConstant() || node->isMethodPointerConstant()))
      {
      if (node->isClassPointerConstant())
         reloKind = TR_ClassPointer;
      else if (node->isMethodPointerConstant())
         reloKind = TR_MethodPointer;
      else
         TR_ASSERT(0, "Unexpected node, don't know how to relocate");
      }

   if (currentInstruction)
      {
      // Optimized loads inserted arbitrarily into the instruction stream must be checked
      // to ensure they don't modify any eflags needed by surrounding instructions.
      //
      if ((value == 0 || value == -1))
         {
         uint8_t EFlags = TR::InstOpCode::getModifiedEFlags(ops[opsRow][((value == 0) ? XOR : OR)]);

         if (existsNextInstructionToTestFlags(currentInstruction, EFlags) || cg->requiresCarry())
            {
            // Can't alter flags, so must use MOV.  Fall through.
            }
         else if (value == 0)
            return generateRegRegInstruction(currentInstruction, ops[opsRow][XOR], target, target, cg);
         else if (value == -1)
            return generateRegImmInstruction(currentInstruction, ops[opsRow][OR], target, (uint32_t)-1, cg);
         }

      // No luck optimizing this.  Just use a MOV
      //
      TR::Instruction *movInstruction = NULL;
      if (is64Bit)
         {
         if (cg->constantAddressesCanChangeSize(node) && node && node->getOpCodeValue() == TR::aconst &&
             (node->isClassPointerConstant() || node->isMethodPointerConstant()))
            {
            movInstruction = generateRegImm64Instruction(currentInstruction, TR::InstOpCode::MOV8RegImm64, target, value, cg, reloKind);
            }
         else if (IS_32BIT_UNSIGNED(value))
            {
            // zero-extended 4-byte MOV
            movInstruction = generateRegImmInstruction(currentInstruction, TR::InstOpCode::MOV4RegImm4, target, static_cast<int32_t>(value), cg, reloKind);
            }
         else if (IS_32BIT_SIGNED(value)) // TODO:AMD64: Is there some way we could get RIP too?
            {
            movInstruction = generateRegImmInstruction(currentInstruction, TR::InstOpCode::MOV8RegImm4, target, static_cast<int32_t>(value), cg, reloKind);
            }
         else
            {
            movInstruction = generateRegImm64Instruction(currentInstruction, TR::InstOpCode::MOV8RegImm64, target, value, cg, reloKind);
            }
         }
      else
         {
         movInstruction = generateRegImmInstruction(currentInstruction, ops[opsRow][MOV], target, static_cast<int32_t>(value), cg, reloKind);
         }

      if (target && node &&
          node->getOpCodeValue() == TR::aconst &&
          node->isClassPointerConstant() &&
          (cg->fe()->isUnloadAssumptionRequired((TR_OpaqueClassBlock *) node->getAddress(),
                                       comp->getCurrentMethod()) ||
           cg->profiledPointersRequireRelocation()))
         {
         comp->getStaticPICSites()->push_front(movInstruction);
         }

      if (target && node &&
          node->getOpCodeValue() == TR::aconst &&
          node->isMethodPointerConstant() &&
          (cg->fe()->isUnloadAssumptionRequired(cg->fe()->createResolvedMethod(cg->trMemory(), (TR_OpaqueMethodBlock *) node->getAddress(), comp->getCurrentMethod())->classOfMethod(), comp->getCurrentMethod()) ||
           cg->profiledPointersRequireRelocation()))
         {
         traceMsg(comp, "Adding instr %p to MethodPICSites for node %p\n", movInstruction, node);
         comp->getStaticMethodPICSites()->push_front(movInstruction);
         }

      return movInstruction;
      }
   else
      {
      // constant loads between a compare and a branch cannot clobber the EFLAGS register
      bool canClobberEFLAGS = !(cg->getCurrentEvaluationTreeTop()->getNode()->getOpCode().isIf() || cg->requiresCarry());

      if (value == 0 && canClobberEFLAGS)
         {
         return generateRegRegInstruction(ops[opsRow][XOR], node, target, target, cg);
         }
      else if (value == -1 && canClobberEFLAGS)
         {
         return generateRegImmInstruction(ops[opsRow][OR], node, target, (uint32_t)-1, cg);
         }
      else
         {
         TR::Instruction *movInstruction = NULL;
         if (is64Bit)
            {
            if (cg->constantAddressesCanChangeSize(node) && node && node->getOpCodeValue() == TR::aconst &&
                (node->isClassPointerConstant() || node->isMethodPointerConstant()))
               {
               movInstruction = generateRegImm64Instruction(TR::InstOpCode::MOV8RegImm64, node, target, value, cg, reloKind);
               }
            else if (IS_32BIT_UNSIGNED(value))
               {
               // zero-extended 4-byte MOV
               movInstruction = generateRegImmInstruction(TR::InstOpCode::MOV4RegImm4, node, target, static_cast<int32_t>(value), cg, reloKind);
               }
            else if (IS_32BIT_SIGNED(value)) // TODO:AMD64: Is there some way we could get RIP too?
               {
               movInstruction = generateRegImmInstruction(TR::InstOpCode::MOV8RegImm4, node, target, static_cast<int32_t>(value), cg, reloKind);
               }
            else
               {
               movInstruction = generateRegImm64Instruction(TR::InstOpCode::MOV8RegImm64, node, target, value, cg, reloKind);
               }
            }
         else
            {
            movInstruction = generateRegImmInstruction(ops[opsRow][MOV], node, target, static_cast<int32_t>(value), cg, reloKind);
            }

         // HCR register PIC site in TR::TreeEvaluator::insertLoadConstant
         TR::Symbol *symbol = NULL;
         if (node && node->getOpCode().hasSymbolReference())
            symbol = node->getSymbol();
         bool isPICCandidate = symbol ? target && symbol->isStatic() && symbol->isClassObject() : false;
         if (isPICCandidate && comp->getOption(TR_EnableHCR))
            {
            comp->getStaticHCRPICSites()->push_front(movInstruction);
            }

         if (target &&
             node &&
             node->getOpCodeValue() == TR::aconst &&
             node->isClassPointerConstant() &&
             (cg->fe()->isUnloadAssumptionRequired((TR_OpaqueClassBlock *) node->getAddress(),
                               comp->getCurrentMethod()) ||
              cg->profiledPointersRequireRelocation()))
            {
            comp->getStaticPICSites()->push_front(movInstruction);
            }

        if (target && node &&
            node->getOpCodeValue() == TR::aconst &&
            node->isMethodPointerConstant() &&
            (cg->fe()->isUnloadAssumptionRequired(cg->fe()->createResolvedMethod(cg->trMemory(), (TR_OpaqueMethodBlock *) node->getAddress(), comp->getCurrentMethod())->classOfMethod(), comp->getCurrentMethod()) ||
             cg->profiledPointersRequireRelocation()))
            {
            traceMsg(comp, "Adding instr %p to MethodPICSites for node %p\n", movInstruction, node);
            comp->getStaticMethodPICSites()->push_front(movInstruction);
            }

         return movInstruction;
         }
      }
   }

TR::Register *loadConstant(TR::Node *node,
                           intptr_t value,
                           TR_RematerializableTypes type,
                           TR::CodeGenerator *cg,
                           TR::Register *targetRegister)
   {
   if (targetRegister == NULL)
      {
      targetRegister = cg->allocateRegister();
      }

   TR::Instruction *instr = OMR::X86::insertLoadConstant(node, targetRegister, value, type, cg);

   // Do not rematerialize register for class pointer or method pointer if
   // it's AOT compilation because it doesn't have node info in register
   // rematerialization to create relocation record for the class pointer
   // or the method pointer.
   if (cg->enableRematerialisation() &&
       !(cg->comp()->compileRelocatableCode() && node && node->getOpCodeValue() == TR::aconst &&
         (node->isClassPointerConstant() || node->isMethodPointerConstant())))
      {
      if (node && node->getOpCode().hasSymbolReference() && node->getSymbol() && node->getSymbol()->isClassObject())
         (TR::Compiler->om.generateCompressedObjectHeaders() || cg->comp()->target().is32Bit())
         ? type = TR_RematerializableInt : type = TR_RematerializableLong;

      setDiscardableIfPossible(type, targetRegister, node, instr, value, cg);
      }

   return targetRegister;
   }

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

   loadConstant(node, begin, TR_RematerializableInt, cg, indexReg);
   loadConstant(node, end, TR_RematerializableInt, cg, loopBoundReg);

   OMR::X86::generateLoop(indexReg, loopBoundReg, node, cg, std::move(genBodyFunction));
   generateLabelInstruction(TR::InstOpCode::label, node, label, deps, cg);

   cg->stopUsingRegister(indexReg);
   cg->stopUsingRegister(loopBoundReg);
   }

}
}
