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

#ifndef OMR_Z_FUNCTIONCALLDATA_INCL
#define OMR_Z_FUNCTIONCALLDATA_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_FUNCTIONCALLDATA_CONNECTOR
#define OMR_FUNCTIONCALLDATA_CONNECTOR

namespace OMR {
namespace Z {
class FunctionCallData;
}

typedef OMR::Z::FunctionCallData FunctionCallDataConnector;
} // namespace OMR
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
class Snippet;
} // namespace TR

namespace OMR { namespace Z {

class OMR_EXTENSIBLE FunctionCallData : public OMR::FunctionCallData {
public:
    /**
     * @brief Z specific implementation of FunctionCallData
     *
     * @param methodSymRef : \c TR::SymbolReference of the function to call
     * @param callNode : \c TR::Node associcated with the function call
     * @param bufferAddress : \c In case encoding a function call, instruction
     *          for call will be encoded from this address.
     * @param targetAddress : \c a specific function address to call that
     *          overrides the address in the symbol reference; or 0 if none
     * @param returnAddressReg : \c A register to be used to hold the return
     *          address for the call instruction
     * @param entryPointReg : \c A register to use as Entry Point Register where
     *          target address is out of the relative addresssible range of BRASL
     * @param regDeps : \c TR::RegisterDependencyConditions to be attach to the
     *          call
     * @param prevInstr : \c TR::Instruction of the previous instruction, if any
     * @param runtimeHelperInder : \c the runtime helper index if calling a helper
     * @param reloKind : \c the external relocation kind associated with this
     *          function call
     * @param snippet : \c TR::Snippet in which call is encoded if function is
     *          called from the snippet.
     * @param cg : \c TR::CodeGenerator object
     */
    FunctionCallData(TR::CodeGenerator *cg, TR::SymbolReference *methodSymRef, TR::Node *callNode,
        uint8_t *bufferAddress, intptr_t targetAddress, TR::Register *returnAddressReg, TR::Register *entryPointReg,
        TR::RegisterDependencyConditions *regDeps, TR::Instruction *prevInstr, TR_RuntimeHelper runtimeHelperIndex,
        TR_ExternalRelocationTargetKind reloKind, TR::Snippet *snippet)
        : OMR::FunctionCallData(cg, methodSymRef, callNode, bufferAddress)
        , targetAddress(targetAddress)
        , returnAddressReg(returnAddressReg)
        , entryPointReg(entryPointReg)
        , regDeps(regDeps)
        , prevInstr(prevInstr)
        , runtimeHelperIndex(runtimeHelperIndex)
        , reloKind(reloKind)
        , snippet(snippet)
        , out_loadInstr(NULL)
        , out_callInstr(NULL)
    {}

    /**
     * If a non-zero targetAddress is provided, use that as the address of the function to call
     */
    uintptr_t targetAddress;

    TR::Register *returnAddressReg;

    TR::Register *entryPointReg;

    TR::RegisterDependencyConditions *regDeps;

    TR::Instruction *prevInstr;

    /**
     * If an intermediate load instruction was used to materialize the function to
     * call then it may be set in this field.
     */
    TR::Instruction *out_loadInstr;

    /**
     * The call instruction that was generated to dispatch to the function.
     */
    TR::Instruction *out_callInstr;

    TR_RuntimeHelper runtimeHelperIndex;

    TR::Snippet *snippet;

    TR_ExternalRelocationTargetKind reloKind;
};

}} // namespace OMR::Z

#endif
