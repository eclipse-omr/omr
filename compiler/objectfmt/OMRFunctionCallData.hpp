/*******************************************************************************
 * Copyright IBM Corp. and others 2021
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

#ifndef OMR_FUNCTIONCALLDATA_INCL
#define OMR_FUNCTIONCALLDATA_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_FUNCTIONCALLDATA_CONNECTOR
#define OMR_FUNCTIONCALLDATA_CONNECTOR
namespace OMR {
class FunctionCallData;
typedef OMR::FunctionCallData FunctionCallDataConnector;
}
#endif

#include <stddef.h>
#include <stdint.h>
#include "env/TRMemory.hpp"
#include "infra/Annotations.hpp"

namespace TR {
class CodeGenerator;
class Node;
class SymbolReference;
}

namespace OMR
{

/**
 * @class FunctionCallData
 *
 * @details
 *    The \c FunctionCallData class is an extensible class used to encapsulate
 *    all the information at particular call site needed to emit a function
 *    call in a particular object format.  Each platform and language
 *    environment is responsible for extending this class to capture whatever
 *    meaningful data is necessary at a call site.
 *
 *    The resulting \c TR::FunctionCallData class is accepted as a parameter to
 *    most \c TR::ObjectFormat APIs.
 */
class OMR_EXTENSIBLE FunctionCallData
   {

public:

   TR_ALLOC(TR_Memory::FunctionCallData)

   FunctionCallData(
         TR::CodeGenerator *cg,
         TR::SymbolReference *methodSymRef,
         TR::Node *callNode) :
      methodSymRef(methodSymRef),
      callNode(callNode),
      bufferAddress(NULL),
      out_encodedMethodAddressLocation(NULL),
      cg(cg) {}

   FunctionCallData(
         TR::CodeGenerator *cg,
         TR::SymbolReference *methodSymRef,
         TR::Node *callNode,
         uint8_t *bufferAddress) :
      methodSymRef(methodSymRef),
      callNode(callNode),
      bufferAddress(bufferAddress),
      out_encodedMethodAddressLocation(NULL),
      cg(cg) {}

   /**
    * The \c TR::SymbolReference of the method to call
    */
   TR::SymbolReference *methodSymRef;

   /**
    * If applicable, the \c TR::Node of the IL opcode that requires this call
    *
    * If not applicable, the field is NULL.
    */
   TR::Node *callNode;

   /**
    * The \c TR::CodeGenerator object
    */
   TR::CodeGenerator *cg;

   /**
    * For a call created post-binary encoding, the address of a buffer
    * where the function call is to be emitted.
    *
    * Otherwise, this field is NULL.
    */
   uint8_t *bufferAddress;

   /**
    * This field is populated during an ObjectFormat dispatch sequence.
    *
    * If the address of the target function is known and it is encoded
    * somewhere as part of the function dispatch sequence then this field
    * records the address where that function address is written.
    */
   uint8_t *out_encodedMethodAddressLocation;

   };

}

#endif
