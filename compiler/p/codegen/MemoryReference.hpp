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

#ifndef TR_MEMORYREFERENCE_INCL
#define TR_MEMORYREFERENCE_INCL

#include "codegen/OMRMemoryReference.hpp"

#include <stdint.h>

namespace TR {
class SymbolReference;
class CodeGenerator;
class Node;
class Register;
}

namespace TR
{

class OMR_EXTENSIBLE MemoryReference : public OMR::MemoryReferenceConnector
   {
   private:

   MemoryReference(TR::Register *br,
      int64_t disp,
      uint8_t len,
      TR::CodeGenerator *cg) :
         OMR::MemoryReferenceConnector(br, disp, len, cg) {}

   MemoryReference(TR::LabelSymbol *label, int64_t disp, int8_t len, TR::CodeGenerator *cg) :
      OMR::MemoryReferenceConnector(label, disp, len, cg) {}

   MemoryReference(TR::CodeGenerator *cg) :
      OMR::MemoryReferenceConnector(cg) {}

   MemoryReference(TR::Register *br,
      TR::Register *ir,
      uint8_t len,
      TR::CodeGenerator *cg) :
         OMR::MemoryReferenceConnector(br, ir, len, cg) {}

   MemoryReference(TR::Node *rootLoadOrStore, uint32_t len, TR::CodeGenerator *cg):
      OMR::MemoryReferenceConnector(rootLoadOrStore, len, cg) {}

   MemoryReference(TR::Node *node, TR::SymbolReference *symRef, uint32_t len, TR::CodeGenerator *cg):
      OMR::MemoryReferenceConnector(node, symRef, len, cg) {}

   MemoryReference(TR::Node *node, MemoryReference& mr, int32_t n, uint32_t len, TR::CodeGenerator *cg):
      OMR::MemoryReferenceConnector(node, mr, n, len, cg) {}

   public:

   static TR::MemoryReference *create(TR::CodeGenerator *cg);
   static TR::MemoryReference *createWithLabel(TR::CodeGenerator *cg, TR::LabelSymbol *label, int64_t offset, int8_t length);
   static TR::MemoryReference *createWithIndexReg(TR::CodeGenerator *cg, TR::Register *baseReg, TR::Register *indexReg, uint8_t length);
   static TR::MemoryReference *createWithDisplacement(TR::CodeGenerator *cg, TR::Register *baseReg, int64_t displacement, int8_t length);
   static TR::MemoryReference *createWithRootLoadOrStore(TR::CodeGenerator *cg, TR::Node *rootLoadOrStore, uint32_t length);
   static TR::MemoryReference *createWithSymRef(TR::CodeGenerator *cg, TR::Node *node, TR::SymbolReference *symRef, uint32_t length);
   static TR::MemoryReference *createWithMemRef(TR::CodeGenerator *cg, TR::Node *node, TR::MemoryReference& memRef, int32_t displacement, uint32_t length);
   };
}

#endif
