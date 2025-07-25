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

#ifndef OMR_Power_PEEPHOLE_INCL
#define OMR_Power_PEEPHOLE_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_PEEPHOLE_CONNECTOR
#define OMR_PEEPHOLE_CONNECTOR

namespace OMR {
namespace Power {
class Peephole;
}

typedef OMR::Power::Peephole PeepholeConnector;
} // namespace OMR
#else
#error OMR::Power::Peephole expected to be a primary connector, but an OMR connector is already defined
#endif

#include "compiler/codegen/OMRPeephole.hpp"

namespace TR {
class Compilation;
class Instruction;
} // namespace TR

namespace OMR { namespace Power {

class OMR_EXTENSIBLE Peephole : public OMR::Peephole {
public:
    Peephole(TR::Compilation *comp);

    virtual bool performOnInstruction(TR::Instruction *cursor);

private:
    /** \brief
     *     Tries to eliminate compare instructions by reducing previous instructions into record form. For example:
     *
     *     <code>
     *     <op> r2,...
     *     ... <no modification of r2 or cr0>
     *     cmpi cr0, r2, 0
     *     </code>
     *
     *     can be reduced to:
     *
     *     <code>
     *     <op_r> r2,...
     *     ... <no modification of r2 or cr0>
     *     </code>
     *
     *     where the \c cmpi is eliminated and \c <op_r> is the record form version of \c <op> instruction.
     *
     *  \return
     *     true if the reduction was successful; false otherwise.
     */
    bool tryToReduceCompareToRecordForm();

    /** \brief
     *     Tries to remove redundant loads after stores which have the same source and target. For example:
     *
     *     <code>
     *     std r10,8(r12)
     *     ld r10,8(r12)
     *     </code>
     *
     *     can be reduced to:
     *
     *     <code>
     *     std r10,8(r12)
     *     </code>
     *
     *  \return
     *     true if the reduction was successful; false otherwise.
     */
    bool tryToRemoveRedundantLoadAfterStore();

    /** \brief
     *     Tries to remove redundant move register instructions. This peephole carries out several optimizations which
     *     can be categorized as follows:
     *
     *     1. Remove NOP \c mr
     *
     *        <code>
     *        mr rX,rX
     *        </code>
     *
     *        Can be removed since this is a NOP.
     *
     *     2. Remove redundant copyback
     *
     *        <code>
     *        mr rY,rX
     *        ... <no modification of rY or rX>
     *        mr rX,rY
     *        </code>
     *
     *        The latter \c mr can be removed.
     *
     *     3. Rewrite base or index register
     *
     *        <code>
     *        mr rY,rX
     *        ... <no modification of rY or rX>
     *        <load or store with rY as a base or index register>
     *        </code>
     *
     *        The load or store base or index register can be replaced with rX (except for the special cases where a
     *        base register cannot be changed to gr0 or in an update form).
     *
     *     4. Rewrite source operand
     *
     *        <code>
     *        mr rY,rX
     *        ... <no modification of rY or rX>
     *        <op> with rY as a source register
     *        </code>
     *
     *        We can rewrite the <op> to replace source rY with rX (which potentially allows the mr and <op> to be
     *        dispatched/issued together), except for the special cases where rX is gr0 and cannot be used.
     *
     *     5. Remove \c mr if all uses rewritten
     *
     *        <code>
     *        mr rY,rX
     *        ... <no modification of rY or rX> <all uses of rY rewritten to rX>
     *        <op> with rY as a target register
     *        </code>
     *
     *        We can remove the \c mr and change any intervening register maps that contain rY to rX.
     *
     *  \return
     *     true if the reduction was successful; false otherwise.
     */
    bool tryToRemoveRedundantMoveRegister();

    /** \brief
     *     Tries to remove redundant synchronization instructions.
     *
     *  \param window
     *     The size of the peephole window to look through before giving up.
     *
     *  \return
     *     true if the reduction was successful; false otherwise.
     */
    bool tryToRemoveRedundantSync(int32_t window);

    /** \brief
     *     Tries to remove redundant consecutive instruction which write to the same target register.
     *
     *  \return
     *     true if the reduction was successful; false otherwise.
     */
    bool tryToRemoveRedundantWriteAfterWrite();

    /** \brief
     *     Tries to swap trap and load instructions.
     *
     *  \return
     *     true if the reduction was successful; false otherwise.
     */
    bool tryToSwapTrapAndLoad();

private:
    /// The instruction cursor currently being processed by the peephole optimization
    TR::Instruction *cursor;
};

}} // namespace OMR::Power

#endif
