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

#include "codegen/Peephole.hpp"

#include "codegen/CodeGenerator.hpp"
#include "codegen/CodeGenerator_inlines.hpp"
#include "codegen/Instruction.hpp"

OMR::Peephole::Peephole(TR::Compilation *comp)
    : _comp(comp)
    , _cg(comp->cg())
    , prevInst(NULL)
{}

TR::Peephole *OMR::Peephole::self() { return static_cast<TR::Peephole *>(this); }

TR::Compilation *OMR::Peephole::comp() const { return _comp; }

TR::CodeGenerator *OMR::Peephole::cg() const { return _cg; }

bool OMR::Peephole::perform()
{
    bool performed = false;

    TR::Instruction *currInst = self()->cg()->getFirstInstruction();

    while (currInst != NULL) {
        performed |= self()->performOnInstruction(currInst);

        // The current instruction being processed may have been removed, moved, or altered in some way by the peephole
        // optimization. In such cases we need to ensure we resume processing instructions from a valid point.
        if (currInst->getPrev() == prevInst) {
            prevInst = currInst;
            currInst = currInst->getNext();
        } else {
            currInst = prevInst->getNext();
        }
    }

    return performed;
}

bool OMR::Peephole::performOnInstruction(TR::Instruction *cursor) { return false; }
