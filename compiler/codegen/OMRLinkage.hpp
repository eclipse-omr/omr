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

#ifndef OMR_LINKAGE_INCL
#define OMR_LINKAGE_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_LINKAGE_CONNECTOR
#define OMR_LINKAGE_CONNECTOR

namespace OMR {
class Linkage;
typedef OMR::Linkage LinkageConnector;
} // namespace OMR
#endif

#include <stddef.h>
#include <stdint.h>
#include "codegen/RegisterConstants.hpp"
#include "env/TRMemory.hpp"
#include "infra/Assert.hpp"
#include "infra/Annotations.hpp"

class TR_BitVector;
class TR_FrontEnd;
class TR_HeapMemory;
class TR_Memory;
class TR_StackMemory;

namespace TR {
class AutomaticSymbol;
class CodeGenerator;
class Compilation;
class Linkage;
class Machine;
class Instruction;
class Node;
class ParameterSymbol;
class ResolvedMethodSymbol;
class Symbol;
class SymbolReference;
} // namespace TR
template<class T> class List;

namespace OMR {
class OMR_EXTENSIBLE Linkage {
public:
    inline TR::Linkage *self();

    /**
     * @return Cached CodeGenerator object
     */
    inline TR::CodeGenerator *cg();

    /**
     * @return Machine object from cached CodeGenerator
     */
    inline TR::Machine *machine();

    /**
     * @return Compilation object from cached CodeGenerator
     */
    inline TR::Compilation *comp();

    /**
     * @return FrontEnd object from cached CodeGenerator
     */
    inline TR_FrontEnd *fe();

    /**
     * @return TR_Memory object from cached CodeGenerator
     */
    inline TR_Memory *trMemory();

    /**
     * @return TR_HeapMemory object
     */
    inline TR_HeapMemory trHeapMemory();

    /**
     * @return TR_StackMemory object
     */
    inline TR_StackMemory trStackMemory();

    TR_ALLOC(TR_Memory::Linkage)

    Linkage(TR::CodeGenerator *cg)
        : _cg(cg)
    {}

    virtual void createPrologue(TR::Instruction *cursor) = 0;
    virtual void createEpilogue(TR::Instruction *cursor) = 0;

    virtual uint32_t getRightToLeft() = 0;
    virtual bool hasToBeOnStack(TR::ParameterSymbol *parm);
    virtual void mapStack(TR::ResolvedMethodSymbol *method) = 0;
    virtual void mapSingleAutomatic(TR::AutomaticSymbol *p, uint32_t &stackIndex) = 0;

    virtual void setParameterLinkageRegisterIndex(TR::ResolvedMethodSymbol *method, List<TR::ParameterSymbol> &parm)
    {
        TR_UNIMPLEMENTED();
    }

    virtual void setParameterLinkageRegisterIndex(TR::ResolvedMethodSymbol *method) { TR_UNIMPLEMENTED(); }

    virtual int32_t numArgumentRegisters(TR_RegisterKinds kind) = 0;

    virtual TR_RegisterKinds argumentRegisterKind(TR::Node *argumentNode);

    /**
     * @brief Perform operations required by this linkage once the code generator
     *    binary encoding phase has completed.  Linkage implementors should override
     *    this function as necessary.
     */
    virtual void performPostBinaryEncoding() {}

    /** @brief
     *    Gets the offset (in number of bytes) from the stack frame pointer to the location on the stack where the first
     *    (closest to the frame pointer) parameter is located.
     *
     *  @details
     *    For example given the following stack frame layout, for a stack which grows towards 0x0000 (we subtract the
     *    stack pointer for each additional frame), and assuming a call to a function with 4 parameters:
     *
     *    @code
     *        0x04E8     +--------+   <- stack pointer
     *                   |  ....  |
     *        0x0518     +--------+   <- frame pointer
     *                   |  ....  |
     *        0x0550     +--------+   <- offset to first parm relative to the frame pointer (0x0550 - 0x0518 = 0x0038)
     *                   |  ARG4  |
     *        0x0558     +--------+
     *                   |  ARG3  |
     *        0x0560     +--------+
     *                   |  ARG2  |
     *        0x0568     +--------+
     *                   |  ARG1  |
     *        0x0570     +--------+
     *    @endcode
     *
     *    The offset returned by this function (0x0038 in the above example) may not be the first argument (in argument
     *    order) passed by the caller on the stack. It is up to the linkage to make use of this function to initialize
     *    parameter offsets depending on the order in which the caller passes the arguments to the callee.
     */
    inline int32_t getOffsetToFirstParm() const;

    /** @brief
     *    Sets the offset (in number of bytes) from the stack frame pointer to the location on the stack where the first
     *    (closest to the frame pointer) parameter is located.
     */
    inline int32_t setOffsetToFirstParm(int32_t offset);

    /**
     * @brief Provides the entry point in a method to use when that method is invoked
     *        from a method compiled with the same linkage.
     *
     * @details
     *    When asked on the method currently being compiled, this API will return 0 if
     *    asked before code memory has been allocated.
     *
     *    The compiled method entry point may be the same as the interpreter entry point.
     *
     *    If this API returns 0 then `entryPointFromInterpetedMethod` must also return 0.
     *
     * @return The entry point for compiled methods to use; 0 if the entry point is unknown
     */
    virtual intptr_t entryPointFromCompiledMethod()
    {
        TR_UNIMPLEMENTED();
        return 0;
    }

    /**
     * @brief Provides the entry point in a method to use when that method is invoked
     *        from an interpreter using the same linkage.
     *
     * @details
     *    When asked on the method currently being compiled, this API will return 0 if
     *    asked before code memory has been allocated.
     *
     *    The compiled method entry point may be the same as the interpreter entry point.
     *
     *    If this API returns 0 then `entryPointFromCompiledMethod` must also return 0.
     *
     * @return The entry point for interpreted methods to use; 0 if the entry point is unknown
     */
    virtual intptr_t entryPointFromInterpretedMethod()
    {
        TR_UNIMPLEMENTED();
        return 0;
    }

protected:
    TR::CodeGenerator *_cg;

    int32_t _offsetToFirstParm;
};
} // namespace OMR

#endif
