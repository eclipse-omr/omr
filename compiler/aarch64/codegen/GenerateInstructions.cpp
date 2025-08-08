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

#include "aarch64/codegen/GenerateInstructions.hpp"

#include <stdint.h>
#include "codegen/ARM64ConditionCode.hpp"
#include "codegen/ARM64Instruction.hpp"
#include "codegen/CodeGenerator.hpp"
#include "codegen/CodeGeneratorUtils.hpp"
#include "codegen/InstOpCode.hpp"
#include "codegen/Linkage.hpp"
#include "codegen/RegisterDependency.hpp"
#include "il/DataTypes_inlines.hpp"
#include "il/Node_inlines.hpp"

namespace TR {
class LabelSymbol;
class Node;
class RegisterDependencyConditions;
class SymbolReference;
}

TR::Instruction *generateInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::Instruction(op, node, preced, cg);
   return new (cg->trHeapMemory()) TR::Instruction(op, node, cg);
   }

TR::ARM64ImmInstruction *generateImmInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node, uint32_t imm,
                                       TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64ImmInstruction(op, node, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64ImmInstruction(op, node, imm, cg);
   }

TR::Instruction *generateRelocatableImmInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   uintptr_t imm, TR_ExternalRelocationTargetKind relocationKind, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64RelocatableImmInstruction(op, node, imm, relocationKind, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64RelocatableImmInstruction(op, node, imm, relocationKind, cg);
   }

TR::Instruction *generateRelocatableImmInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   uintptr_t imm, TR_ExternalRelocationTargetKind relocationKind, TR::SymbolReference *sr, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64RelocatableImmInstruction(op, node, imm, relocationKind, sr, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64RelocatableImmInstruction(op, node, imm, relocationKind, sr, cg);
   }

TR::Instruction *generateImmSymInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   uintptr_t imm, TR::RegisterDependencyConditions *cond, TR::SymbolReference *sr, TR::Snippet *s,
   TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64ImmSymInstruction(op, node, imm, cond, sr, s, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64ImmSymInstruction(op, node, imm, cond, sr, s, cg);
   }

TR::Instruction *generateLabelInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::LabelSymbol *sym, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64LabelInstruction(op, node, sym, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64LabelInstruction(op, node, sym, cg);
   }

TR::Instruction *generateLabelInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::LabelSymbol *sym, TR::RegisterDependencyConditions *cond, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64LabelInstruction(op, node, sym, cond, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64LabelInstruction(op, node, sym, cond, cg);
   }

TR::Instruction *generateConditionalBranchInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::LabelSymbol *sym, TR::ARM64ConditionCode cc, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64ConditionalBranchInstruction(op, node, sym, cc, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64ConditionalBranchInstruction(op, node, sym, cc, cg);
   }

TR::Instruction *generateConditionalBranchInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::LabelSymbol *sym, TR::ARM64ConditionCode cc, TR::RegisterDependencyConditions *cond, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64ConditionalBranchInstruction(op, node, sym, cc, cond, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64ConditionalBranchInstruction(op, node, sym, cc, cond, cg);
   }

TR::Instruction *generateCompareBranchInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *sreg, TR::LabelSymbol *sym, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64CompareBranchInstruction(op, node, sreg, sym, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64CompareBranchInstruction(op, node, sreg, sym, cg);
   }

TR::Instruction *generateCompareBranchInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *sreg, TR::LabelSymbol *sym, TR::RegisterDependencyConditions *cond, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64CompareBranchInstruction(op, node, sreg, sym, cond, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64CompareBranchInstruction(op, node, sreg, sym, cond, cg);
   }

TR::Instruction *generateTestBitBranchInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *sreg, uint32_t bitpos, TR::LabelSymbol *sym, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64TestBitBranchInstruction(op, node, sreg, bitpos, sym, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64TestBitBranchInstruction(op, node, sreg, bitpos, sym, cg);
   }

TR::Instruction *generateTestBitBranchInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *sreg, uint32_t bitpos, TR::LabelSymbol *sym, TR::RegisterDependencyConditions *cond, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64TestBitBranchInstruction(op, node, sreg, bitpos, sym, cond, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64TestBitBranchInstruction(op, node, sreg, bitpos, sym, cond, cg);
   }

TR::Instruction *generateRegBranchInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64RegBranchInstruction(op, node, treg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64RegBranchInstruction(op, node, treg, cg);
   }

TR::Instruction *generateRegBranchInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::RegisterDependencyConditions *cond, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64RegBranchInstruction(op, node, treg, cond, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64RegBranchInstruction(op, node, treg, cond, cg);
   }

TR::Instruction *generateAdminInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Node *fenceNode, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64AdminInstruction(op, node, fenceNode, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64AdminInstruction(op, node, fenceNode, cg);
   }

TR::Instruction *generateAdminInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::RegisterDependencyConditions *cond, TR::Node *fenceNode, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64AdminInstruction(op, cond, node, fenceNode, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64AdminInstruction(op, cond, node, fenceNode, cg);
   }

TR::Instruction *generateTrgInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Instruction(op, node, treg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Instruction(op, node, treg, cg);
   }

TR::Instruction *generateTrg1ImmInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, uint32_t imm, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1ImmInstruction(op, node, treg, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1ImmInstruction(op, node, treg, imm, cg);
   }

TR::Instruction *generateTrg1ImmShiftedInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, uint32_t imm, uint32_t shiftAmount, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1ImmShiftedInstruction(op, node, treg, imm, shiftAmount, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1ImmShiftedInstruction(op, node, treg, imm, shiftAmount, cg);
   }

TR::Instruction *generateTrg1ImmSymInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, uint32_t imm, TR::Symbol *sym, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1ImmSymInstruction(op, node, treg, imm, sym, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1ImmSymInstruction(op, node, treg, imm, sym, cg);
   }

TR::Instruction *generateTrg1Src1Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src1Instruction(op, node, treg, s1reg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src1Instruction(op, node, treg, s1reg, cg);
   }

TR::Instruction *generateTrg1Src1ImmInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, uint32_t imm, TR::Instruction *preced)
   {
   bool isShifted = false;

   if ((op == TR::InstOpCode::addimmx) || (op == TR::InstOpCode::addimmw) ||
      (op == TR::InstOpCode::addsimmx) || (op == TR::InstOpCode::addsimmw) ||
      (op == TR::InstOpCode::subimmx) || (op == TR::InstOpCode::subimmw) ||
      (op == TR::InstOpCode::subsimmx) || (op == TR::InstOpCode::subsimmw))
      {
      if (constantIsUnsignedImm12(imm))
         {
         isShifted = false;
         }
      else
         {
         TR_ASSERT_FATAL(constantIsUnsignedImm12Shifted(imm), "immediate value out of range");
         isShifted = true;
         imm = imm >> 12;
         }
      }

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src1ImmInstruction(op, node, treg, s1reg, isShifted, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src1ImmInstruction(op, node,treg, s1reg, isShifted, imm, cg);
   }

TR::Instruction *generateTrg1Src2Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, TR::Register *s2reg, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src2Instruction(op, node, treg, s1reg, s2reg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src2Instruction(op, node, treg, s1reg, s2reg, cg);
   }

TR::Instruction *generateCondTrg1Src2Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, TR::Register *s2reg, TR::ARM64ConditionCode cc, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64CondTrg1Src2Instruction(op, node, treg, s1reg, s2reg, cc, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64CondTrg1Src2Instruction(op, node, treg, s1reg, s2reg, cc, cg);
   }

TR::Instruction *generateCondTrg1Src2Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, TR::Register *s2reg, TR::ARM64ConditionCode cc, TR::RegisterDependencyConditions *cond, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64CondTrg1Src2Instruction(op, node, treg, s1reg, s2reg, cc, cond, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64CondTrg1Src2Instruction(op, node, treg, s1reg, s2reg, cc, cond, cg);
   }

TR::Instruction *generateTrg1Src2ImmInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, TR::Register *s2reg,
   uint32_t imm, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src2ImmInstruction(op, node, treg, s1reg, s2reg, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src2ImmInstruction(op, node, treg, s1reg, s2reg, imm, cg);
   }

TR::Instruction *generateTrg1Src2ShiftedInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, TR::Register *s2reg,
   TR::ARM64ShiftCode shiftType, uint32_t shiftAmount, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src2ShiftedInstruction(op, node, treg, s1reg, s2reg, shiftType, shiftAmount, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src2ShiftedInstruction(op, node, treg, s1reg, s2reg, shiftType, shiftAmount, cg);
   }

TR::Instruction *generateTrg1Src2ExtendedInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, TR::Register *s2reg,
   TR::ARM64ExtendCode extendType, uint32_t shiftAmount, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src2ExtendedInstruction(op, node, treg, s1reg, s2reg, extendType, shiftAmount, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src2ExtendedInstruction(op, node, treg, s1reg, s2reg, extendType, shiftAmount, cg);
   }

TR::Instruction *generateTrg1Src2IndexedElementInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, TR::Register *s2reg,
   uint32_t index, TR::Instruction *preced)
   {
   if ((op >= TR::InstOpCode::fmulelem_4s) && (op <= TR::InstOpCode::vfmulelem_2d))
      {
      if ((op == TR::InstOpCode::fmulelem_4s) || (op == TR::InstOpCode::vfmulelem_4s))
         {
         TR_ASSERT_FATAL_WITH_NODE(node, index <= 3, "index is out of range: %d", index);
         }
      else
         {
         TR_ASSERT_FATAL_WITH_NODE(node, index <= 1, "index is out of range: %d", index);
         }
      }
   else
      {
      TR_ASSERT_FATAL_WITH_NODE(node, false, "unsupported opcode: %d", op);
      }
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src2IndexedElementInstruction(op, node, treg, s1reg, s2reg, index, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src2IndexedElementInstruction(op, node, treg, s1reg, s2reg, index, cg);
   }

TR::Instruction *generateTrg1Src3Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, TR::Register *s2reg, TR::Register *s3reg, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src3Instruction(op, node, treg, s1reg, s2reg, s3reg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src3Instruction(op, node, treg, s1reg, s2reg, s3reg, cg);
   }

TR::Instruction *generateTrg1Src3Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, TR::Register *s2reg, TR::Register *s3reg,
   TR::RegisterDependencyConditions *cond, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src3Instruction(op, node, treg, s1reg, s2reg, s3reg, cond, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src3Instruction(op, node, treg, s1reg, s2reg, s3reg, cond, cg);
   }

TR::Instruction *generateTrg1MemInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::MemoryReference *mr, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1MemInstruction(op, node, treg, mr, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1MemInstruction(op, node, treg, mr, cg);
   }

TR::Instruction *generateTrg2MemInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg1, TR::Register *treg2, TR::MemoryReference *mr, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg2MemInstruction(op, node, treg1, treg2, mr, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg2MemInstruction(op, node, treg1, treg2, mr, cg);
   }

TR::Instruction *generateMemImmInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::MemoryReference *mr, uint32_t imm, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64MemImmInstruction(op, node, mr, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64MemImmInstruction(op, node, mr, imm, cg);
   }

TR::Instruction *generateMemSrc1Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::MemoryReference *mr, TR::Register *sreg, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64MemSrc1Instruction(op, node, mr, sreg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64MemSrc1Instruction(op, node, mr, sreg, cg);
   }

TR::Instruction *generateMemSrc2Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::MemoryReference *mr, TR::Register *s1reg, TR::Register *s2reg, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64MemSrc2Instruction(op, node, mr, s1reg, s2reg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64MemSrc2Instruction(op, node, mr, s1reg, s2reg, cg);
   }

TR::Instruction *generateTrg1MemSrc1Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::MemoryReference *mr, TR::Register *sreg, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1MemSrc1Instruction(op, node, treg, mr, sreg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1MemSrc1Instruction(op, node, treg, mr, sreg, cg);
   }

TR::Instruction *generateSrc1Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *s1reg, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Src1Instruction(op, node, s1reg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Src1Instruction(op, node, s1reg, cg);
   }

TR::Instruction *generateSrc2Instruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *s1reg, TR::Register *s2reg, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Src2Instruction(op, node, s1reg, s2reg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Src2Instruction(op, node, s1reg, s2reg, cg);
   }

TR::Instruction *generateArithmeticShiftRightImmInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, uint32_t shiftAmount, bool is64bit, TR::Instruction *preced)
   {
   /* Alias of SBFM instruction */

   TR_ASSERT_FATAL(shiftAmount < (is64bit ? 64 : 32), "Shift amount out of range.");

   TR::InstOpCode::Mnemonic op = is64bit ? TR::InstOpCode::sbfmx : TR::InstOpCode::sbfmw;
   uint32_t imms = is64bit ? 0x3f : 0x1f;
   uint32_t immr = shiftAmount;
   uint32_t imm = (immr << 6) | imms;

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src1ImmInstruction(op, node, treg, sreg, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src1ImmInstruction(op, node, treg, sreg, imm, cg);
   }

TR::Instruction *generateLogicalShiftRightImmInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, uint32_t shiftAmount, bool is64bit, TR::Instruction *preced)
   {
   /* Alias of UBFM instruction */

   TR_ASSERT_FATAL(shiftAmount < (is64bit ? 64 : 32), "Shift amount out of range.");

   TR::InstOpCode::Mnemonic op = is64bit ? TR::InstOpCode::ubfmx : TR::InstOpCode::ubfmw;
   uint32_t imms = is64bit ? 0x3f : 0x1f;
   uint32_t immr = shiftAmount;
   uint32_t imm = (immr << 6) | imms;

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src1ImmInstruction(op, node, treg, sreg, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src1ImmInstruction(op, node, treg, sreg, imm, cg);
   }

TR::Instruction *generateLogicalShiftLeftImmInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, uint32_t shiftAmount, bool is64bit, TR::Instruction *preced)
   {
   /* Alias of UBFM instruction */

   TR_ASSERT_FATAL(shiftAmount < (is64bit ? 64 : 32), "Shift amount out of range.");

   TR::InstOpCode::Mnemonic op = is64bit ? TR::InstOpCode::Mnemonic::ubfmx : TR::InstOpCode::Mnemonic::ubfmw;
   uint32_t imms = (is64bit ? 63 : 31) - shiftAmount;
   uint32_t immr = imms + 1;
   uint32_t imm = (immr << 6) | imms;

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src1ImmInstruction(op, node, treg, sreg, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src1ImmInstruction(op, node, treg, sreg, imm, cg);
   }

TR::Instruction *generateLogicalImmInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op,
   TR::Node *node, TR::Register *treg, TR::Register *s1reg, bool N, uint32_t imm, TR::Instruction *preced)
   {
   TR::ARM64Trg1Src1ImmInstruction *intr;
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src1ImmInstruction(op, node, treg, s1reg, N, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src1ImmInstruction(op, node,treg, s1reg, N, imm, cg);
   }

TR::Instruction *generateCompareImmInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *sreg, int32_t imm, bool is64bit, TR::Instruction *preced)
   {
   TR::InstOpCode::Mnemonic op;
   bool isShifted = false;

   if (constantIsUnsignedImm12(imm))
      {
      /* Alias of SUBS instruction */
      op = is64bit ? TR::InstOpCode::subsimmx : TR::InstOpCode::subsimmw;
      }
   else if (constantIsUnsignedImm12Shifted(imm))
      {
      op = is64bit ? TR::InstOpCode::subsimmx : TR::InstOpCode::subsimmw;
      isShifted = true;
      imm = imm >> 12;
      }
   else if (constantIsUnsignedImm12(-imm))
      {
      /* Alias of ADDS instruction */
      op = is64bit ? TR::InstOpCode::addsimmx : TR::InstOpCode::addsimmw;
      imm = -imm;
      }
   else
      {
      TR_ASSERT_FATAL(constantIsUnsignedImm12Shifted(-imm), "Immediate value is out of range for cmp/cmn");

      /* Alias of ADDS instruction */
      op = is64bit ? TR::InstOpCode::addsimmx : TR::InstOpCode::addsimmw;
      isShifted = true;
      imm = (-imm) >> 12;
      }

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64ZeroSrc1ImmInstruction(op, node, sreg, isShifted, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64ZeroSrc1ImmInstruction(op, node, sreg, isShifted, imm, cg);
   }

TR::Instruction *generateTestImmInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *sreg, int32_t imm, bool N, bool is64bit, TR::Instruction *preced)
   {
   /* Alias of ANDS instruction */

   TR::InstOpCode::Mnemonic op = is64bit ? TR::InstOpCode::andsimmx : TR::InstOpCode::andsimmw;

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64ZeroSrc1ImmInstruction(op, node, sreg, N, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64ZeroSrc1ImmInstruction(op, node, sreg, N, imm, cg);
   }

TR::Instruction *generateCompareInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *s1reg, TR::Register *s2reg, bool is64bit, TR::Instruction *preced)
   {
   /* Alias of SUBS instruction */

   TR::InstOpCode::Mnemonic op = is64bit ? TR::InstOpCode::subsx : TR::InstOpCode::subsw;

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64ZeroSrc2Instruction(op, node, s1reg, s2reg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64ZeroSrc2Instruction(op, node, s1reg, s2reg, cg);
   }

TR::Instruction *generateTestInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *s1reg, TR::Register *s2reg, bool is64bit, TR::Instruction *preced)
   {
   /* Alias of ANDS instruction */

   TR::InstOpCode::Mnemonic op = is64bit ? TR::InstOpCode::andsx : TR::InstOpCode::andsw;

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64ZeroSrc2Instruction(op, node, s1reg, s2reg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64ZeroSrc2Instruction(op, node, s1reg, s2reg, cg);
   }

TR::Instruction *generateConditionalCompareImmInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *sreg, uint32_t imm, uint32_t conditionFlags, TR::ARM64ConditionCode cc,
   bool is64bit, bool isNegative, TR::Instruction *preced)
   {
   TR::InstOpCode::Mnemonic op = is64bit ? (isNegative ? TR::InstOpCode::ccmnimmx : TR::InstOpCode::ccmpimmx) :
                                           (isNegative ? TR::InstOpCode::ccmnimmw : TR::InstOpCode::ccmpimmw);

   TR_ASSERT_FATAL(constantIsUnsignedImm5(imm), "Immediate value is out of range for ccmp/ccmn");

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Src1ImmCondInstruction(op, node, sreg, imm, cc, conditionFlags, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Src1ImmCondInstruction(op, node, sreg, imm, cc, conditionFlags, cg);
   }

TR::Instruction *generateConditionalCompareInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *sreg1, TR::Register *sreg2, uint32_t conditionFlags, TR::ARM64ConditionCode cc,
   bool is64bit, bool isNegative, TR::Instruction *preced)
   {
   TR::InstOpCode::Mnemonic op = is64bit ? (isNegative ? TR::InstOpCode::ccmnx : TR::InstOpCode::ccmpx) :
                                           (isNegative ? TR::InstOpCode::ccmnw : TR::InstOpCode::ccmpw);

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Src2CondInstruction(op, node, sreg1, sreg2, cc, conditionFlags, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Src2CondInstruction(op, node, sreg1, sreg2, cc, conditionFlags, cg);
   }

TR::Instruction *generateMovInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, bool is64bit, TR::Instruction *preced)
   {
   /* Alias of ORR instruction */

   TR::InstOpCode::Mnemonic op = is64bit ? TR::InstOpCode::orrx : TR::InstOpCode::orrw;

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1ZeroSrc1Instruction(op, node, treg, sreg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1ZeroSrc1Instruction(op, node, treg, sreg, cg);
   }

TR::Instruction *generateMvnInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, bool is64bit, TR::Instruction *preced)
   {
   /* Alias of ORN instruction */

   TR::InstOpCode::Mnemonic op = is64bit ? TR::InstOpCode::ornx : TR::InstOpCode::ornw;

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1ZeroSrc1Instruction(op, node, treg, sreg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1ZeroSrc1Instruction(op, node, treg, sreg, cg);
   }

TR::Instruction *generateNegInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, bool is64bit, TR::Instruction *preced)
   {
   /* Alias of SUB instruction */

   TR::InstOpCode::Mnemonic op = is64bit ? TR::InstOpCode::subx : TR::InstOpCode::subw;

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1ZeroSrc1Instruction(op, node, treg, sreg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1ZeroSrc1Instruction(op, node, treg, sreg, cg);
   }

TR::Instruction *generateMovBitMaskInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, bool N, uint32_t imm, bool is64bit, TR::Instruction *preced)
   {
   /* Alias of ORR instruction */
   TR::InstOpCode::Mnemonic op = is64bit ? TR::InstOpCode::orrimmx : TR::InstOpCode::orrimmw;

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1ZeroImmInstruction(op, node, treg, N, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1ZeroImmInstruction(op, node,treg, N, imm, cg);
   }

TR::Instruction *generateMulInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, TR::Register *s2reg, TR::Instruction *preced)
   {
   return generateMulInstruction(cg, node, treg, s1reg, s2reg, node->getDataType().isInt64(), preced);
   }

TR::Instruction *generateMulInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::Register *s1reg, TR::Register *s2reg, bool is64bit, TR::Instruction *preced)
   {
   /* Alias of MADD instruction */

   TR::InstOpCode::Mnemonic op = is64bit ? TR::InstOpCode::maddx : TR::InstOpCode::maddw;

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1Src2ZeroInstruction(op, node, treg, s1reg, s2reg, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1Src2ZeroInstruction(op, node, treg, s1reg, s2reg, cg);
   }

TR::Instruction *generateCSetInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::ARM64ConditionCode cc, TR::Instruction *preced)
   {
   /* Alias of CSINC instruction with inverted condition code */
   TR::InstOpCode::Mnemonic op = TR::InstOpCode::csincx;

   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64Trg1CondInstruction(op, node, treg, cc_invert(cc), preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64Trg1CondInstruction(op, node, treg, cc_invert(cc), cg);
   }

TR::Instruction *generateCIncInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, TR::ARM64ConditionCode cc, bool is64bit, TR::Instruction *preced)
   {
   /* Alias of CSINC instruction with inverted condition code */
   TR::InstOpCode::Mnemonic op = is64bit ? TR::InstOpCode::csincx : TR::InstOpCode::csincw;
   return generateCondTrg1Src2Instruction(cg, op, node, treg, sreg, sreg, cc_invert(cc), preced);
   }

TR::ARM64SynchronizationInstruction *generateSynchronizationInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op,
   TR::Node *node, TR::InstOpCode::AArch64BarrierLimitation lim, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64SynchronizationInstruction(op, node, lim, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64SynchronizationInstruction(op, node, lim, cg);
   }

TR::ARM64ExceptionInstruction *generateExceptionInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op,
   TR::Node *node, uint32_t imm, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64ExceptionInstruction(op, node, imm, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64ExceptionInstruction(op, node, imm, cg);
   }

TR::Instruction *generateUBFXInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, uint32_t lsb, uint32_t width, bool is64bit, TR::Instruction *preced)
   {
   uint32_t imms = (lsb + width - 1);
   uint32_t immr = lsb;
   TR_ASSERT_FATAL((is64bit && (immr <= 63) && (imms <= 63)) || ((!is64bit) && (immr <= 31) && (imms <= 31)),
                   "immediate field for ubfm is out of range: is64bit=%d, immr=%d, imms=%d", is64bit, immr, imms);
   return generateTrg1Src1ImmInstruction(cg, is64bit ? TR::InstOpCode::ubfmx : TR::InstOpCode::ubfmw, node, treg, sreg, (immr << 6) | imms, preced);
   }

TR::Instruction *generateUBFIZInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, uint32_t lsb, uint32_t width, bool is64bit, TR::Instruction *preced)
   {
   uint32_t imms = width - 1;
   uint32_t immr = (is64bit ? 64 : 32) - lsb;
   TR_ASSERT_FATAL((is64bit && (immr <= 63) && (imms <= 63)) || ((!is64bit) && (immr <= 31) && (imms <= 31)),
                   "immediate field for ubfm is out of range: is64bit=%d, immr=%d, imms=%d", is64bit, immr, imms);
   return generateTrg1Src1ImmInstruction(cg, is64bit ? TR::InstOpCode::ubfmx : TR::InstOpCode::ubfmw, node, treg, sreg, (immr << 6) | imms, preced);
   }

TR::Instruction *generateBFIInstruction(TR::CodeGenerator *cg, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, uint32_t lsb, uint32_t width, bool is64bit, TR::Instruction *preced)
   {
   uint32_t imms = width - 1;
   uint32_t immr = (is64bit ? 64 : 32) - lsb;
   TR_ASSERT_FATAL((is64bit && (immr <= 63) && (imms <= 63)) || ((!is64bit) && (immr <= 31) && (imms <= 31)),
                   "immediate field for bfm is out of range: is64bit=%d, immr=%d, imms=%d", is64bit, immr, imms);
   return generateTrg1Src1ImmInstruction(cg, is64bit ? TR::InstOpCode::bfmx : TR::InstOpCode::bfmw, node, treg, sreg, (immr << 6) | imms, preced);
   }

TR::Instruction *generateVectorShiftImmediateInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, uint32_t shiftAmount, TR::Instruction *preced)
   {
   TR_ASSERT_FATAL_WITH_NODE(node, (op >= TR::InstOpCode::vshl16b) && (op <= TR::InstOpCode::vsri2d), "Illegal opcode for generateVectorShiftImmediateInstruction: %d", op);

   bool isShiftLeft = (op <= TR::InstOpCode::vsli2d);
   const auto opcodeBinaryEncoding = TR::InstOpCode::getOpCodeBinaryEncoding(op);
   /* If bit 11 - 15 is 0b100xx, then it is a variant of shift right narrow instructions. */
   const bool isShiftRightNarrow = ((opcodeBinaryEncoding >> 11) & 0x1c) == 0x10;
   uint32_t immh = (opcodeBinaryEncoding >> 19) & 0xf;
   uint32_t elementSize = 8 << (31 - leadingZeroes(immh));
   TR_ASSERT_FATAL_WITH_NODE(node, (elementSize == 8) || (elementSize == 16) || (elementSize == 32) || (elementSize == 64), "Illegal element size: %d", elementSize);
   const uint32_t shiftAmountLowerLimit = isShiftRightNarrow ? 1 : 0;
   const uint32_t shiftAmountUpperLimit = isShiftRightNarrow ? elementSize : elementSize - 1;
   TR_ASSERT_FATAL_WITH_NODE(node, (shiftAmount >= shiftAmountLowerLimit) && (shiftAmount <= shiftAmountUpperLimit), "Illegal shift amount: %d", shiftAmount);

   uint32_t imm = isShiftLeft ? (shiftAmount + elementSize) : (elementSize * 2 - shiftAmount);
   return generateTrg1Src1ImmInstruction(cg, op, node, treg, sreg, imm, preced);
   }

TR::Instruction *generateVectorUXTLInstruction(TR::CodeGenerator *cg, TR::DataType elementType, TR::Node *node, TR::Register *treg, TR::Register *sreg,
                  bool isUXTL2, TR::Instruction *preced)
   {
   TR::InstOpCode::Mnemonic op;
   switch (elementType)
      {
      case TR::Int8:
         op = isUXTL2 ? TR::InstOpCode::vushll2_8h : TR::InstOpCode::vushll_8h;
         break;
      case TR::Int16:
         op = isUXTL2 ? TR::InstOpCode::vushll2_4s : TR::InstOpCode::vushll_4s;
         break;
      case TR::Int32:
         op = isUXTL2 ? TR::InstOpCode::vushll2_2d : TR::InstOpCode::vushll_2d;
         break;
      default:
         TR_ASSERT_FATAL_WITH_NODE(node, false, "Unexpected element type");
         break;
      }
   return generateVectorShiftImmediateInstruction(cg, op, node, treg, sreg, 0, preced);
   }

static
bool isVectorRegister(TR::Register *reg)
   {
   return (reg->getRealRegister() != NULL) ? (reg->getKind() == TR_FPR) : (reg->getKind() == TR_VRF);
   }

TR::Instruction *generateVectorDupElementInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, uint32_t srcIndex, TR::Instruction *preced)
   {
   TR_ASSERT_FATAL_WITH_NODE(node, (op >= TR::InstOpCode::vdupe16b) && (op <= TR::InstOpCode::vdupe2d), "Illegal opcode for generateVectorDupElementInstruction: %d", op);
   TR_ASSERT_FATAL_WITH_NODE(node, isVectorRegister(treg) && isVectorRegister(sreg), "The target and source register must be VRF");
   const uint32_t elementSizeShift = op - TR::InstOpCode::vdupe16b;
   const uint32_t nelements = 16 >> elementSizeShift;
   TR_ASSERT_FATAL_WITH_NODE(node, (srcIndex < nelements), "srcIndex (%d) must be less than the number of elements (%d)", srcIndex, nelements);

   uint32_t imm5 = (srcIndex << (elementSizeShift + 1)) & 0x1f;
   return generateTrg1Src1ImmInstruction(cg, op, node, treg, sreg, imm5, preced);
   }

TR::Instruction *generateMovVectorElementToGPRInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, uint32_t srcIndex, TR::Instruction *preced)
   {
   TR_ASSERT_FATAL_WITH_NODE(node, (op >= TR::InstOpCode::smovwb) && (op <= TR::InstOpCode::umovxd), "Illegal opcode for generateMovVectorElementToGPRInstruction: %d", op);
   TR_ASSERT_FATAL_WITH_NODE(node, (treg->getKind() == TR_GPR) && isVectorRegister(sreg), "The target register must be GPR and the source register must be VRF");
   const uint32_t elementSizeShift = (op >= TR::InstOpCode::umovwb) ? (op - TR::InstOpCode::umovwb) :
                                                                      ((op >= TR::InstOpCode::smovxb) ? (op - TR::InstOpCode::smovxb) : (op - TR::InstOpCode::smovwb));
   const uint32_t nelements = 16 >> elementSizeShift;
   TR_ASSERT_FATAL_WITH_NODE(node, (srcIndex < nelements), "srcIndex (%d) must be less than the number of elements (%d)", srcIndex, nelements);

   uint32_t imm5 = (srcIndex << (elementSizeShift + 1)) & 0x1f;
   return generateTrg1Src1ImmInstruction(cg, op, node, treg, sreg, imm5, preced);
   }

TR::Instruction *generateMovGPRToVectorElementInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, uint32_t trgIndex, TR::Instruction *preced)
   {
   TR_ASSERT_FATAL_WITH_NODE(node, (op >= TR::InstOpCode::vinswb) && (op <= TR::InstOpCode::vinsxd), "Illegal opcode for generateMovGPRToVectorElementInstruction: %d", op);
   TR_ASSERT_FATAL_WITH_NODE(node, isVectorRegister(treg) && (sreg->getKind() == TR_GPR), "The target register must be VRF and the source register must be GPR");
   const uint32_t elementSizeShift = op - TR::InstOpCode::vinswb;
   const uint32_t nelements = 16 >> elementSizeShift;
   TR_ASSERT_FATAL_WITH_NODE(node, (trgIndex < nelements), "trgIndex (%d) must be less than the number of elements (%d)", trgIndex, nelements);

   uint32_t imm5 = (trgIndex << (elementSizeShift + 1)) & 0x1f;
   return generateTrg1Src1ImmInstruction(cg, op, node, treg, sreg, imm5, preced);
   }

TR::Instruction *generateMovVectorElementInstruction(TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic op, TR::Node *node,
   TR::Register *treg, TR::Register *sreg, uint32_t trgIndex, uint32_t srcIndex, TR::Instruction *preced)
   {
   TR_ASSERT_FATAL_WITH_NODE(node, (op >= TR::InstOpCode::vinseb) && (op <= TR::InstOpCode::vinsed), "Illegal opcode for generateMovVectorElementInstruction: %d", op);
   TR_ASSERT_FATAL_WITH_NODE(node, isVectorRegister(treg) && isVectorRegister(sreg), "The target and source register must be VRF");
   const uint32_t elementSizeShift = op - TR::InstOpCode::vinseb;
   const uint32_t nelements = 16 >> elementSizeShift;
   TR_ASSERT_FATAL_WITH_NODE(node, (srcIndex < nelements) && (trgIndex < nelements), "srcIndex (%d) and trgIndex (%d) must be less than the number of elements (%d)", srcIndex, trgIndex, nelements);

   /* bit 16-20 */
   uint32_t imm5 = (trgIndex << (elementSizeShift + 1)) & 0x1f;
   /* bit 11-14 */
   uint32_t imm4 = (srcIndex << elementSizeShift) & 0xf;
   /*
    * We expect imm to be inserted into bit 11-20 of the instruction.
    *
    * +------+--------------+--+-----------+
    * |bitpos|20|19|18|17|16|15|14|13|12|11|
    * +------+--------------+--+-----------+
    * | value|    imm5      | 0|   imm4    |
    * +------+--------------+--+-----------+
    */
   uint32_t imm = (imm5 << 5) | imm4;
   return generateTrg1Src1ImmInstruction(cg, op, node, treg, sreg, imm, preced);
   }

#ifdef J9_PROJECT_SPECIFIC
TR::Instruction *generateVirtualGuardNOPInstruction(TR::CodeGenerator *cg,  TR::Node *n, TR_VirtualGuardSite *site,
   TR::RegisterDependencyConditions *cond, TR::LabelSymbol *sym, TR::Instruction *preced)
   {
   if (preced)
      return new (cg->trHeapMemory()) TR::ARM64VirtualGuardNOPInstruction(n, site, cond, sym, preced, cg);
   return new (cg->trHeapMemory()) TR::ARM64VirtualGuardNOPInstruction(n, site, cond, sym, cg);
   }
#endif
