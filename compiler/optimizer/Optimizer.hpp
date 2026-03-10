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

#ifndef TR_OPTIMIZER_INCL
#define TR_OPTIMIZER_INCL

#if defined(OMR_OPTIMIZER_SMALL)

#include "optimizer/SmallOptimizer.hpp"

namespace TR {
class Optimizer : public TR::SmallOptimizer {
public:
    Optimizer(TR::Compilation *comp, TR::ResolvedMethodSymbol *methodSymbol, bool isIlGen)
        : TR::SmallOptimizer(comp, methodSymbol, isIlGen)
    {}
};
} // namespace TR

#else // defined(OMR_OPTIMIZER_SMALL)

#include "optimizer/FullOptimizer.hpp"

namespace TR {
class Optimizer : public TR::FullOptimizer {
public:
    Optimizer(TR::Compilation *comp, TR::ResolvedMethodSymbol *methodSymbol, bool isIlGen)
        : TR::FullOptimizer(comp, methodSymbol, isIlGen)
    {}
};
} // namespace TR

#endif // defined(OMR_OPTIMIZER_SMALL)

#endif // defined(TR_OPTIMIZER_INCL)
