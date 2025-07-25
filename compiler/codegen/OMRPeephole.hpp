/*******************************************************************************
 * Copyright IBM Corp. and others 2020
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

#ifndef OMR_PEEPHOLE_INCL
#define OMR_PEEPHOLE_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_PEEPHOLE_CONNECTOR
#define OMR_PEEPHOLE_CONNECTOR

namespace OMR {
class Peephole;
typedef OMR::Peephole PeepholeConnector;
} // namespace OMR
#endif

#include "env/TRMemory.hpp"
#include "infra/Flags.hpp"
#include "infra/Annotations.hpp"

namespace TR {
class CodeGenerator;
class Instruction;
class Peephole;
} // namespace TR

namespace OMR {

class OMR_EXTENSIBLE Peephole {
public:
    TR_ALLOC(TR_Memory::UnknownType)

    Peephole(TR::Compilation *comp);

    TR::Peephole *self();

    TR::CodeGenerator *cg() const;
    TR::Compilation *comp() const;

public:
    /** \brief
     *     Performs peephole optimizations on the entire instruction stream by traversing instructions from start until
     *     the end of the method and repeatedly calling performOnInstruction API for each instruction. If the result of
     *     the API call performOnInstruction indicates that a transformation was performed, the peephole window restarts
     *     on the previous instruction.
     *
     *  \return
     *     true if any transformation was performed; false otherwise.
     */
    virtual bool perform();

    /** \brief
     *     Performs peephole optimizations on an instruction cursor.
     *
     *  \param cursor
     *     The instruction cursor currently being processed.
     *
     *  \return
     *     true if any transformation was performed; false otherwise.
     */
    virtual bool performOnInstruction(TR::Instruction *cursor);

private:
    TR::Compilation *_comp;
    TR::CodeGenerator *_cg;

protected:
    /// Represents the previous instruction on which peepholes were performed, which is also the restart point in case a
    /// transformation was performed on the current instruction being processed
    TR::Instruction *prevInst;
};

} // namespace OMR

#endif
