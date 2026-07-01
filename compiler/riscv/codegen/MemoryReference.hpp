/*******************************************************************************
 * Copyright IBM Corp. and others 2019
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
#include "codegen/CodeGenerator.hpp"

#include <stdint.h>

namespace TR {
class SymbolReference;
class CodeGenerator;
class Node;
class Register;
} // namespace TR

namespace TR {

class OMR_EXTENSIBLE MemoryReference : public OMR::MemoryReferenceConnector {
public:
    /**
     * @brief Constructor
     * @param[in] cg : CodeGenerator object
     */
    MemoryReference(TR::CodeGenerator *cg)
        : OMR::MemoryReferenceConnector(cg)
    {}

    /**
     * @brief Constructor
     * @param[in] br : base register
     * @param[in] cg : CodeGenerator object
     */
    MemoryReference(TR::Register *br, TR::CodeGenerator *cg)
        : OMR::MemoryReferenceConnector(br, cg)
    {}

    /**
     * @brief Constructor
     * @param[in] br : base register
     * @param[in] disp : displacement
     * @param[in] cg : CodeGenerator object
     */
    MemoryReference(TR::Register *br, int32_t disp, TR::CodeGenerator *cg)
        : OMR::MemoryReferenceConnector(br, disp, cg)
    {}

    /**
     * @brief Constructor
     * @param[in] rootLoadOrStore : load or store node
     * @param[in] cg : CodeGenerator object
     */
    MemoryReference(TR::Node *rootLoadOrStore, TR::CodeGenerator *cg)
        : OMR::MemoryReferenceConnector(rootLoadOrStore, cg)
    {}

    /**
     * @brief Constructor
     * @param[in] node : node
     * @param[in] symRef : symbol reference
     * @param[in] cg : CodeGenerator object
     */
    MemoryReference(TR::Node *node, TR::SymbolReference *symRef, TR::CodeGenerator *cg)
        : OMR::MemoryReferenceConnector(node, symRef, cg)
    {}
};

/**
 * @brief Create a memory reference using base register and displacement.
 *
 * @param[in] br : base register
 * @param[in] disp : displacement
 * @param[in] cg : CodeGenerator object
 * @return New memory reference.
 */
inline TR::MemoryReference *MRef_BaseDisp(TR::Register *br, int32_t disp, TR::CodeGenerator *cg)
{
    return new (cg->trHeapMemory()) TR::MemoryReference(br, disp, cg);
}

/**
 * @brief Create a memory reference using base register only - the displacement is zero.
 * Equivalent to `MRef_Base(br, 0, cg)`.
 *
 * @param[in] br : base register
 * @param[in] cg : CodeGenerator object
 * @return New memory reference.
 */
inline TR::MemoryReference *MRef_Base(TR::Register *br, TR::CodeGenerator *cg) { return MRef_BaseDisp(br, 0, cg); }

/**
 * @brief Creates a memory reference from given load or store IL node.
 *
 * @param[in] node : load or store node
 * @param[in] cg : CodeGenerator object
 * @return New memory reference.
 */
inline TR::MemoryReference *MRef_Node(TR::Node *node, TR::CodeGenerator *cg)
{
    return new (cg->trHeapMemory()) TR::MemoryReference(node, cg);
}

/**
 * @brief Creates a memory reference from given node and symbol reference.
 *
 * @param[in] node : node
 * @param[in] symRef : symbol reference.
 * @param[in] cg : CodeGenerator object
 * @return New memory reference.
 */
inline TR::MemoryReference *MRef_Sym(TR::Node *node, TR::SymbolReference *symRef, TR::CodeGenerator *cg)
{
    return new (cg->trHeapMemory()) TR::MemoryReference(node, cg);
}

} // namespace TR

#endif
