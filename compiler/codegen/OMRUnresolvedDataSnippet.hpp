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

#ifndef OMR_UNRESOLVEDDATASNIPPET_INCL
#define OMR_UNRESOLVEDDATASNIPPET_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_UNRESOLVEDDATASNIPPET_CONNECTOR
#define OMR_UNRESOLVEDDATASNIPPET_CONNECTOR
namespace OMR {
class UnresolvedDataSnippet;
typedef OMR::UnresolvedDataSnippet UnresolvedDataSnippetConnector;
}
#endif

#include "codegen/Snippet.hpp"

#include <stddef.h>
#include <stdint.h>
#include "infra/Flags.hpp"

namespace TR {
class SymbolReference;
class CodeGenerator;
class Instruction;
class Node;
class UnresolvedDataSnippet;
}

namespace OMR
{

class UnresolvedDataSnippet : public TR::Snippet
   {

public:

   UnresolvedDataSnippet(TR::CodeGenerator * cg,
                         TR::Node * node,
                         TR::SymbolReference * symRef,
                         bool isStore,
                         bool isGCSafePoint);

   TR::UnresolvedDataSnippet * self();

   static TR::UnresolvedDataSnippet * create(TR::CodeGenerator * cg, TR::Node * node, TR::SymbolReference *s, bool isStore, bool canCauseGC);

   TR::SymbolReference * getDataSymbolReference()                        { return _dataSymbolReference; }
   TR::SymbolReference * setDataSymbolReference(TR::SymbolReference *sr) { return (_dataSymbolReference = sr); }

   TR::Instruction * getDataReferenceInstruction()                   { return _dataReferenceInstruction; }
   TR::Instruction * setDataReferenceInstruction(TR::Instruction *i) { return (_dataReferenceInstruction = i); }

   uint8_t * getAddressOfDataReference()           { return _addressOfDataReference; }
   uint8_t * setAddressOfDataReference(uint8_t *p) { return (_addressOfDataReference = p); }

   bool isUnresolvedStore()    { return _flags.testAll(TO_MASK32(IsUnresolvedStore)); }
   void setUnresolvedStore()   { _flags.set(TO_MASK32(IsUnresolvedStore)); }
   void resetUnresolvedStore() { _flags.reset(TO_MASK32(IsUnresolvedStore)); }

   virtual uint8_t *emitSnippetBody() { return NULL; }

   virtual uint32_t getLength(int32_t estimatedSnippetStart) { return 0; }

protected:

   enum
      {
      IsUnresolvedStore = TR::Snippet::NextSnippetFlag,
      NextSnippetFlag
      };

   static_assert((int32_t)NextSnippetFlag <= (int32_t)TR::Snippet::MaxSnippetFlag,
      "OMR::UnresolvedDataSnippet too many flag bits for flag width");

private:

   TR::Instruction * _dataReferenceInstruction;

   TR::SymbolReference * _dataSymbolReference;

   uint8_t * _addressOfDataReference;

   };

}

#endif
