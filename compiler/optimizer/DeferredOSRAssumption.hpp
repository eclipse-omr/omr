/*******************************************************************************
 * Copyright IBM Corp. and others 2024
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

#ifndef DEFERRED_OSR_ASSUMPTION_INCL
#define DEFERRED_OSR_ASSUMPTION_INCL

namespace TR {
class Compilation;
}

namespace TR {

/**
 * \brief An OSR assumption that has been provisionally used, but that is not
 * necessarily required by the current compilation (yet).
 *
 * One or more deferred OSR assumptions can be associated with an intermediate
 * result that may or may not turn out to be significant. If/when that result
 * is later relied upon, e.g. as the justification for an IL transformation,
 * its assumptions must be committed, at which point the compilation will
 * arrange to create the appropriate runtime assumption on completion. If OTOH
 * the result is never used, its deferred assumptions need not be committed,
 * which helps to avoid creating irrelevant runtime assumptions.
 */
struct DeferredOSRAssumption {
    /**
     * \brief Update \p comp so that this assumption will be required.
     * \param comp the compilation object
     */
    virtual void commit(TR::Compilation *comp) = 0;
};

} // namespace TR

#endif // DEFERRED_OSR_ASSUMPTION_INCL
