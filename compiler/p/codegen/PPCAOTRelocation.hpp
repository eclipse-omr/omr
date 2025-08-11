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

#ifndef PPCAOTRELOCATION_INCL
#define PPCAOTRELOCATION_INCL

#include <stdint.h>
#include "codegen/Relocation.hpp"
#include "env/TRMemory.hpp"
#include "runtime/Runtime.hpp"

namespace TR {
class CodeGenerator;
class Instruction;
class LabelSymbol;
class Node;
}

namespace TR {

class PPCRelocation
   {
   public:
   TR_ALLOC(TR_Memory::PPCRelocation)

   PPCRelocation(TR::Instruction *src,
                 uint8_t           *trg,
       TR_ExternalRelocationTargetKind k):
      _srcInstruction(src), _relTarget(trg), _kind(k)
      {}

   TR::Instruction *getSourceInstruction() {return _srcInstruction;}
   void setSourceInstruction(TR::Instruction *i) {_srcInstruction = i;}

   uint8_t *getRelocationTarget() {return _relTarget;}
   void setRelocationTarget(uint8_t *t) {_relTarget = t;}

   TR_ExternalRelocationTargetKind getKind() {return _kind;}
   void setKind(TR_ExternalRelocationTargetKind k) {_kind = k;}

   virtual void mapRelocation(TR::CodeGenerator *cg) = 0;

   private:
   TR::Instruction               *_srcInstruction;
   uint8_t                         *_relTarget;
   TR_ExternalRelocationTargetKind  _kind;
   };

class PPCPairedRelocation: public TR::PPCRelocation
   {
   public:
   PPCPairedRelocation(TR::Instruction *src1,
                       TR::Instruction *src2,
                       uint8_t           *trg,
                       TR_ExternalRelocationTargetKind k,
                       TR::Node *node) :
      TR::PPCRelocation(src1, trg, k), _src2Instruction(src2), _node(node)
      {}

   TR::Instruction *getSource2Instruction() {return _src2Instruction;}
   void setSource2Instruction(TR::Instruction *src) {_src2Instruction = src;}
   TR::Node* getNode(){return _node;}
   virtual void mapRelocation(TR::CodeGenerator *cg);

   private:
   TR::Instruction               *_src2Instruction;
   TR::Node                         *_node;
   };

// TODO(#5404): The classes defined below here are specific to Power, but are not actually AOT
//              relocations. They are only here for lack of a better place to put them.

// PPC Specific Relocation: update the immediate fields of a pair of ppc instructions
// with the absolute address of a label (extended to 64bit as well).  For example:
//   lis  gr3, addr_hi
//   addi gr3, gr3, addr_lo
//
class PPCPairedLabelAbsoluteRelocation : public TR::LabelRelocation
   {
   public:
   PPCPairedLabelAbsoluteRelocation(TR::Instruction *src1,
                                    TR::Instruction *src2,
                                    TR::Instruction *src3,
                                    TR::Instruction *src4,
                                    TR::LabelSymbol *label)
      : TR::LabelRelocation(0, label), _instr1(src1), _instr2(src2), _instr3(src3), _instr4(src4) {}
   virtual void apply(TR::CodeGenerator *cg);

   private:
   TR::Instruction *_instr1;
   TR::Instruction *_instr2;
   TR::Instruction *_instr3;
   TR::Instruction *_instr4;
   };

/**
 * \brief Represents a relocation on the D34 field of a PC-relative prefixed load/store instruction
 *        that should use an offset from a TR::LabelSymbol.
 */
class PPCD34LabelRelocation : public TR::LabelRelocation
   {
   public:

   /**
    * \brief
    *   Creates a new PPCD34LabelRelocation.
    *
    * \param instr
    *   The TR::Instruction for which this relocation was created, or \c NULL if this relocation is
    *   not associated with an instruction object.
    *
    * \param cursor
    *   The start of the prefixed load/store instruction to relocate.
    *
    * \param label
    *   The label relative to which the load/store should occur.
    *
    * \param offset
    *   The byte offset from the label at which the load/store should occur.
    */
   PPCD34LabelRelocation(TR::Instruction *instr, uint32_t *cursor, TR::LabelSymbol *label, int64_t offset)
      : TR::LabelRelocation(reinterpret_cast<uint8_t*>(cursor), label), _instr(instr), _offset(offset) {}

   virtual void apply(TR::CodeGenerator *cg);

   private:
   TR::Instruction *_instr;
   int64_t _offset;
   };

}

typedef TR::PPCRelocation TR_PPCRelocation;
typedef TR::PPCPairedRelocation TR_PPCPairedRelocation;
typedef TR::PPCPairedLabelAbsoluteRelocation TR_PPCPairedLabelAbsoluteRelocation;

#endif
