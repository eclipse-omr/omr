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

#ifndef OMR_FULLOPTIMIZER_INCL
#define OMR_FULLOPTIMIZER_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_FULLOPTIMIZER_CONNECTOR
#define OMR_FULLOPTIMIZER_CONNECTOR

namespace OMR {
class FullOptimizer;
typedef OMR::FullOptimizer FullOptimizerConnector;
} // namespace OMR
#endif

#include <stddef.h>
#include <stdint.h>
#include "optimizer/SmallOptimizer.hpp"

namespace TR {
class Compilation;
class ResolvedMethodSymbol;
} // namespace TR
struct OptimizationStrategy;

namespace OMR {

class FullOptimizer : public TR::SmallOptimizer {
public:
    TR_ALLOC(TR_Memory::Machine)

    FullOptimizer(TR::Compilation *comp, TR::ResolvedMethodSymbol *methodSymbol, bool isIlGen,
        const OptimizationStrategy *strategy = NULL, uint16_t VNType = 0);

    virtual void enableAllLocalOpts();
};
} // namespace OMR

/*temp for openj9 static*/ extern const OptimizationStrategy cheapObjectAllocationOpts[];
/*temp for openj9 static*/ extern const OptimizationStrategy expensiveObjectAllocationOpts[];
/*temp for openj9 static*/ extern const OptimizationStrategy cheapGlobalValuePropagationOpts[];
/*temp for openj9 static*/ extern const OptimizationStrategy expensiveGlobalValuePropagationOpts[];
/*temp for openj9 static*/ extern const OptimizationStrategy isolatedStoreOpts[];
/*temp for openj9 static*/ extern const OptimizationStrategy loopSpecializerOpts[];
/*temp for openj9 static*/ extern const OptimizationStrategy loopAliasRefinerOpts[];
/*temp for openj9 static*/ extern const OptimizationStrategy eachEscapeAnalysisPassOpts[];

#endif // defined(OMR_FULLOPTIMIZER_INCL)
