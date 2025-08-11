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

#include "infra/Annotations.hpp"
#include "objectfmt/OMRFunctionCallData.hpp"
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
     * @brief Constructor for FunctionCallData used to generate instruction for
     *     helper calls
     *
     * @param methodSymRef : \c TR::SymbolReference of the function to call.
     * @param callNode : \c TR::Node node associated with this function call
     * @param cg : \c TR::CodeGenerator object.
     * @param returnAddressReg : \c TR::Register used to hold the return address
     *           for the call instruction.
     * @param entryPointReg : \c TR::Register used as entry point register where
     *           target address can not be addressible using Long relative instruction.
     * @param regDeps : \c TR::RegisterDependencyConditions to be attached to the
     *           call instruction.
     * @param prevInstr : \c TR::Instruction preceding to function call.
     */
    FunctionCallData(TR::CodeGenerator *cg, TR::SymbolReference *methodSymRef, TR::Node *callNode,
        TR::Register *returnAddressReg, TR::Register *entryPointReg = NULL,
        TR::RegisterDependencyConditions *regDeps = NULL, TR::Instruction *prevInstr = NULL)
        : OMR::FunctionCallDataConnector(cg, methodSymRef, callNode, NULL, static_cast<intptr_t>(0), returnAddressReg,
              entryPointReg, regDeps, prevInstr, (TR_RuntimeHelper)0, TR_NoRelocation, NULL)
    {}

    /**
     * @brief Constructor for FunctionCallData used to encode a function call
     *
     * @param methodSymRef : \c TR::SymbolReference of the function to call
     * @param callNode : \c TR::Node node associated with this function call
     * @param bufferAddress : \c Buffer address where this function call will be
     *          encoded from.
     * @param cg : \c TR::CodeGenerator object
     * @param snippet : \c TR::Snippet where this function call is encoded
     * @param reloKind : \c External relocation kind
     */
    FunctionCallData(TR::CodeGenerator *cg, TR::SymbolReference *methodSymRef, TR::Node *callNode,
        uint8_t *bufferAddress, TR::Snippet *snippet, TR_ExternalRelocationTargetKind reloKind = TR_NoRelocation)
        : OMR::FunctionCallDataConnector(cg, methodSymRef, callNode, bufferAddress, static_cast<intptr_t>(0), NULL,
              NULL, NULL, NULL, (TR_RuntimeHelper)0, reloKind, snippet)
    {}
};

} // namespace TR
#endif
