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

#ifndef TR_FUNCTIONCALLDATA_INCL
#define TR_FUNCTIONCALLDATA_INCL

#include "objectfmt/OMRFunctionCallData.hpp"
#include "infra/Annotations.hpp"
#include "runtime/Runtime.hpp"

namespace TR {
class Instruction;
class SymbolReference;
class Node;
class Register;
class RegisterDependencyConditions;
class CodeGenerator;
} // namespace TR

namespace TR {

class OMR_EXTENSIBLE FunctionCallData : public OMR::FunctionCallDataConnector {
public:
    /**
     * APIs for non-helper function calls
     */

    /**
     * @param[in] cg : \c TR::CodeGenerator object
     * @param[in] methodSymRef : \c TR::SymbolReference of the function to call
     * @param[in] callNode : \c TR::Node this function call is associated with
     * @param[in] scratchReg : a scratch register to use to emit the dispatch sequence
     * @param[in] targetAddress : a specific function address to call that overrides the
     *               address in the symbol reference; or 0 if none
     * @param[in] regDeps : \c TR::RegisterDependency structure for this call
     * @param[in] reloKind : the external relocation kind associated with this function call
     * @param[in] useSymInstruction : if true then use "Sym" form instructions that take a
     *               \c TR::SymbolReference; if false then use equivalent form instructions without
     *               the \c TR::SymbolReference parameter
     * @param[in] adjustsFramePointerBy : if non-zero, the adjustment to be made to the internal
     *               virtual frame pointer by the instruction to perform the function call
     * @param[in] useCall : if true, emit a call instruction; if false, emit a jump instruction
     */
    FunctionCallData(TR::CodeGenerator *cg, TR::SymbolReference *methodSymRef, TR::Node *callNode,
        TR::Register *scratchReg, uintptr_t targetAddress, TR::RegisterDependencyConditions *regDeps, int32_t reloKind,
        bool useSymInstruction, int32_t adjustsFramePointerBy, bool useCall = true)
        : OMR::FunctionCallDataConnector(cg, methodSymRef, callNode, scratchReg, targetAddress, regDeps,
              (TR_RuntimeHelper)0, NULL, reloKind, adjustsFramePointerBy, useSymInstruction, NULL, useCall)
    {}

    /**
     * @param[in] cg : \c TR::CodeGenerator object
     * @param[in] methodSymRef : \c TR::SymbolReference of the function to call
     * @param[in] prevInstr : \c TR::Instruction of the previous instruction, if any
     * @param[in] scratchReg : a scratch register to use to emit the dispatch sequence
     * @param[in] targetAddress : a specific function address to call that overrides the
     *               address in the symbol reference; or 0 if none
     * @param[in] regDeps : \c TR::RegisterDependency structure for this call
     * @param[in] reloKind : the external relocation kind associated with this function call
     * @param[in] useSymInstruction : if true then use "Sym" form instructions that take a
     *               \c TR::SymbolReference; if false then use equivalent form instructions without
     *               the \c TR::SymbolReference parameter
     * @param[in] adjustsFramePointerBy : if non-zero, the adjustment to be made to the internal
     *               virtual frame pointer by the instruction to perform the function call
     * @param[in] useCall : if true, emit a call instruction; if false, emit a jump instruction
     */
    FunctionCallData(TR::CodeGenerator *cg, TR::SymbolReference *methodSymRef, TR::Instruction *prevInstr,
        TR::Register *scratchReg, uintptr_t targetAddress, TR::RegisterDependencyConditions *regDeps, int32_t reloKind,
        bool useSymInstruction, int32_t adjustsFramePointerBy, bool useCall = true)
        : OMR::FunctionCallDataConnector(cg, methodSymRef, NULL, scratchReg, targetAddress, regDeps,
              (TR_RuntimeHelper)0, prevInstr, reloKind, adjustsFramePointerBy, useSymInstruction, NULL, useCall)
    {}

    /**
     * @param[in] cg : \c TR::CodeGenerator object
     * @param[in] methodSymRef : \c TR::SymbolReference of the function to call
     * @param[in] callNode : \c TR::Node this function call is associated with
     * @param[in] scratchReg : a scratch register to use to emit the dispatch sequence
     * @param[in] targetAddress : a specific function address to call that overrides the
     *               address in the symbol reference; or 0 if none
     * @param[in] regDeps : \c TR::RegisterDependency structure for this call
     * @param[in] reloKind : the external relocation kind associated with this function call
     * @param[in] useSymInstruction : if true then use "Sym" form instructions that take a
     *               \c TR::SymbolReference; if false then use equivalent form instructions without
     *               the \c TR::SymbolReference parameter
     * @param[in] useCall : if true, emit a call instruction; if false, emit a jump instruction
     */
    FunctionCallData(TR::CodeGenerator *cg, TR::SymbolReference *methodSymRef, TR::Node *callNode,
        TR::Register *scratchReg, uintptr_t targetAddress, TR::RegisterDependencyConditions *regDeps, int32_t reloKind,
        bool useSymInstruction, bool useCall = true)
        : OMR::FunctionCallDataConnector(cg, methodSymRef, callNode, scratchReg, targetAddress, regDeps,
              (TR_RuntimeHelper)0, NULL, reloKind, 0, useSymInstruction, NULL, useCall)
    {}

    /**
     * @param[in] cg : \c TR::CodeGenerator object
     * @param[in] methodSymRef : \c TR::SymbolReference of the function to call
     * @param[in] prevInstr : \c TR::Instruction of the previous instruction, if any
     * @param[in] scratchReg : a scratch register to use to emit the dispatch sequence
     * @param[in] targetAddress : a specific function address to call that overrides the
     *               address in the symbol reference; or 0 if none
     * @param[in] regDeps : \c TR::RegisterDependency structure for this call
     * @param[in] reloKind : the external relocation kind associated with this function call
     * @param[in] useSymInstruction : if true then use "Sym" form instructions that take a
     *               \c TR::SymbolReference; if false then use equivalent form instructions without
     *               the \c TR::SymbolReference parameter
     * @param[in] useCall : if true, emit a call instruction; if false, emit a jump instruction
     */
    FunctionCallData(TR::CodeGenerator *cg, TR::SymbolReference *methodSymRef, TR::Instruction *prevInstr,
        TR::Register *scratchReg, uintptr_t targetAddress, TR::RegisterDependencyConditions *regDeps, int32_t reloKind,
        bool useSymInstruction, bool useCall = true)
        : OMR::FunctionCallDataConnector(cg, methodSymRef, NULL, scratchReg, targetAddress, regDeps,
              (TR_RuntimeHelper)0, prevInstr, reloKind, 0, useSymInstruction, NULL, useCall)
    {}

    /**
     * APIs for helper function calls
     */

    /**
     * @param[in] cg : \c TR::CodeGenerator object
     * @param[in] runtimeHelperIndex : the runtime helper index if calling a helper
     * @param[in] callNode : \c TR::Node this function call is associated with
     * @param[in] regDeps : \c TR::RegisterDependency structure for this call
     * @param[in] useCall : if true, emit a call instruction; if false, emit a jump instruction
     */
    FunctionCallData(TR::CodeGenerator *cg, TR_RuntimeHelper runtimeHelperIndex, TR::Node *callNode,
        TR::RegisterDependencyConditions *regDeps, bool useCall = true)
        : OMR::FunctionCallDataConnector(cg, NULL, callNode, NULL, 0, regDeps, runtimeHelperIndex, NULL,
              TR_NoRelocation, 0, false, NULL, useCall)
    {}

    /**
     * @param[in] cg : \c TR::CodeGenerator object
     * @param[in] runtimeHelperIndex : the runtime helper index if calling a helper
     * @param[in] prevInstr : \c TR::Instruction of the previous instruction, if any
     * @param[in] regDeps : \c TR::RegisterDependency structure for this call
     * @param[in] useCall : if true, emit a call instruction; if false, emit a jump instruction
     */
    FunctionCallData(TR::CodeGenerator *cg, TR_RuntimeHelper runtimeHelperIndex, TR::Instruction *prevInstr,
        TR::RegisterDependencyConditions *regDeps, bool useCall = true)
        : OMR::FunctionCallDataConnector(cg, NULL, NULL, NULL, 0, regDeps, runtimeHelperIndex, prevInstr,
              TR_NoRelocation, 0, false, NULL, useCall)
    {}

    /**
     * APIs for function calls encoded from snippets
     */

    /**
     * @param[in] methodSymRef : \c TR::SymbolReference of the function to call
     * @param[in] callNode : \c TR::Node this function call is associated with
     * @param[in] bufferAddress : if non-zero, encode the function call at this address
     * @param[in] cg : \c TR::CodeGenerator object
     * @param[in] useCall : if true, emit a call instruction; if false, emit a jump instruction
     */
    FunctionCallData(TR::CodeGenerator *cg, TR::SymbolReference *methodSymRef, TR::Node *callNode,
        uint8_t *bufferAddress, bool useCall = true)
        : OMR::FunctionCallDataConnector(cg, methodSymRef, callNode, NULL, 0, NULL, (TR_RuntimeHelper)0, NULL,
              TR_NoRelocation, 0, false, bufferAddress, useCall)
    {}
};

} // namespace TR

#endif
