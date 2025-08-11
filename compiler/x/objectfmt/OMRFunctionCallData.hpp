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

#ifndef OMR_X86_FUNCTIONCALLDATA_INCL
#define OMR_X86_FUNCTIONCALLDATA_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_FUNCTIONCALLDATA_CONNECTOR
#define OMR_FUNCTIONCALLDATA_CONNECTOR
namespace OMR {
namespace X86 { class FunctionCallData; }
typedef OMR::X86::FunctionCallData FunctionCallDataConnector;
}
#endif

#include "compiler/objectfmt/OMRFunctionCallData.hpp"
#include "runtime/Runtime.hpp"

namespace TR {
class Instruction;
class SymbolReference;
class Node;
class Register;
class RegisterDependencyConditions;
class CodeGenerator;
}

namespace OMR
{

namespace X86
{

class OMR_EXTENSIBLE FunctionCallData : public OMR::FunctionCallData
   {
public:

   /**
    * @brief X86 specific implementation of FunctionCallData
    *
    * @param[in] cg : \c TR::CodeGenerator object
    * @param[in] methodSymRef : \c TR::SymbolReference of the function to call
    * @param[in] callNode : \c TR::Node this function call is associated with
    * @param[in] scratchReg : a scratch register to use to emit the dispatch sequence
    * @param[in] targetAddress : a specific function address to call that overrides the
    *               address in the symbol reference; or 0 if none
    * @param[in] regDeps : \c TR::RegisterDependency structure for this call
    * @param[in] runtimeHelperIndex : the runtime helper index if calling a helper
    * @param[in] prevInstr : \c TR::Instruction of the previous instruction, if any
    * @param[in] reloKind : the external relocation kind associated with this function call
    * @param[in] adjustsFramePointerBy : if non-zero, the adjustment to be made to the internal
    *               virtual frame pointer by the instruction to perform the function call
    * @param[in] useSymInstruction : if true then use "Sym" form instructions that take a
    *               \c TR::SymbolReference; if false then use equivalent form instructions without
    *               the \c TR::SymbolReference parameter
    * @param[in] bufferAddress : if non-zero and performing an encoding (rather than emitting
    *               \c TR::Instructions) then emit the function call at this address
    * @param[in] useCall : if true, emit a call instruction; if false, emit a jump instruction
    */
   FunctionCallData(
         TR::CodeGenerator *cg,
         TR::SymbolReference *methodSymRef,
         TR::Node *callNode,
         TR::Register *scratchReg,
         uintptr_t targetAddress,
         TR::RegisterDependencyConditions *regDeps,
         TR_RuntimeHelper runtimeHelperIndex,
         TR::Instruction *prevInstr,
         int32_t reloKind,
         int32_t adjustsFramePointerBy,
         bool useSymInstruction,
         uint8_t *bufferAddress,
         bool useCall) :
      OMR::FunctionCallData(cg, methodSymRef, callNode, bufferAddress),
         targetAddress(targetAddress),
         scratchReg(scratchReg),
         regDeps(regDeps),
         prevInstr(prevInstr),
         reloKind(reloKind),
         useSymInstruction(useSymInstruction),
         runtimeHelperIndex(runtimeHelperIndex),
         adjustsFramePointerBy(adjustsFramePointerBy),
         useCall(useCall),
         out_materializeTargetAddressInstr(NULL),
         out_callInstr(NULL) {}

   /**
    * If a non-zero targetAddress is provided, use that as the address of the function to call
    */
   uintptr_t targetAddress;

   /**
    * If set, an available scratch register to use during the call sequence.
    *
    * If NULL, no scratch register is available
    */
   TR::Register *scratchReg;

   /**
    * The \c TR::RegisterDependencyConditions to apply at this call site
    */
   TR::RegisterDependencyConditions *regDeps;

   /**
    * The instruction just prior to the sequence to emit a call to some function
    */
   TR::Instruction *prevInstr;

   /**
    * If an intermediate instruction was used to materialize the address of the
    * function to call then it may be set in this field.
    */
   TR::Instruction *out_materializeTargetAddressInstr;

   /**
    * The call instruction that was generated to dispatch to the target function.
    */
   TR::Instruction *out_callInstr;

   /**
    * Should a CALL instruction be used?  If false then a JMP instruction will
    * be emitted instead.
    */
   bool useCall;

   /**
    * For applicable call sites, use an instruction form that has a \c TR::SymbolReference
    * rather than one that doesn't.
    */
   bool useSymInstruction;

   /**
    * The relocation kind to apply at this call site
    */
   int32_t reloKind;

   /**
    * Number of bytes to adjust the VFP by at this call site
    */
   int32_t adjustsFramePointerBy;

   /**
    * If calling a helper function, the index of the helper function to call
    */
   TR_RuntimeHelper runtimeHelperIndex;

   };

}

}

#endif
