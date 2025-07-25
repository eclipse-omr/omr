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

#include "control/CompilationController.hpp"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "env/FrontEnd.hpp"
#include "compile/Compilation.hpp"
#include "compile/CompilationTypes.hpp"
#include "control/OptimizationPlan.hpp"
#include "control/Recompilation.hpp"
#include "control/CompilationStrategy.hpp"
#include "env/TRMemory.hpp"
#include "infra/Monitor.hpp"
#include "infra/ThreadLocal.hpp"

int32_t TR::CompilationController::_verbose = 0;
TR::CompilationStrategy *TR::CompilationController::_compilationStrategy = NULL;
TR::CompilationInfo *TR::CompilationController::_compInfo = 0;
bool TR::CompilationController::_useController = false;
bool TR::CompilationController::_tlsCompObjCreated = false;

bool TR::CompilationController::init(TR::CompilationInfo *compInfo)
{
    _compInfo = compInfo;
    _compilationStrategy = new (PERSISTENT_NEW) TR::CompilationStrategy();

    TR_OptimizationPlan::_optimizationPlanMonitor = TR::Monitor::create("OptimizationPlanMonitor");
    _useController = (TR_OptimizationPlan::_optimizationPlanMonitor != 0);
    if (_useController) {
        static char *verboseController = feGetEnv("TR_VerboseController");
        if (verboseController)
            setVerbose(atoi(verboseController));
    }
#ifdef J9_PROJECT_SPECIFIC
    if (TR::Options::getCmdLineOptions() && TR::Options::getCmdLineOptions()->getOption(TR_EnableCompYieldStats))
        TR::Compilation::allocateCompYieldStatsMatrix();
#endif
    TR_TLS_ALLOC(OMR::compilation);
    _tlsCompObjCreated = true;
    return _useController;
}

void TR::CompilationController::shutdown()
{
    if (_tlsCompObjCreated) {
        TR_TLS_FREE(OMR::compilation);
    }

    if (!_useController)
        return;

    // would like to free all entries in the pool of compilation plans
    int32_t remainingPlans = TR_OptimizationPlan::freeEntirePool();

    _compilationStrategy->shutdown();
}
