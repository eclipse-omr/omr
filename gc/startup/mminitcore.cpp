/*******************************************************************************
 * Copyright IBM Corp. and others 1991
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include "mminitcore.h"

#include "Configuration.hpp"
#include "EnvironmentBase.hpp"
#include "GCExtensionsBase.hpp"
#include "Heap.hpp"
#include "ModronAssertions.h"
#include "ObjectAllocationInterface.hpp"

/* OMRTODO temporary workaround to allow both ut_j9mm.h and ut_omrmm.h to be included.
 *                 Dependency on ut_j9mm.h should be removed in the future.
 */
#undef UT_MODULE_LOADED
#undef UT_MODULE_UNLOADED
#include "ut_omrmm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the threads mutator information (RS pointers, reference list pointers etc) for GC/MM purposes.
 *
 * @note vmThread MAY NOT be initialized completely from an execution model perspective.
 * @return 0 if OK, or non 0 if error
 */
intptr_t
initializeMutatorModel(OMR_VMThread *omrVMThread)
{
	MM_GCExtensionsBase* extensions = MM_GCExtensionsBase::getExtensions(omrVMThread->_vm);
	omrVMThread->_gcOmrVMThreadExtensions = extensions->configuration->createEnvironment(extensions, omrVMThread);
	if (NULL != omrVMThread->_gcOmrVMThreadExtensions) {
		omrVMThread->memorySpace = extensions->heap->getDefaultMemorySpace();
	} else {
		return -1;
	}
	return 0;
}

/**
 * Cleanup Mutator specific resources (TLH, thread extension, etc) on shutdown.
 */
void
cleanupMutatorModel(OMR_VMThread *omrVMThread, uintptr_t flushCaches)
{
	MM_EnvironmentBase *env = MM_EnvironmentBase::getEnvironment(omrVMThread);

	if (NULL != env) {
		if (flushCaches) {
			env->_objectAllocationInterface->flushCache(env);
		}
		env->kill();
	}

	omrVMThread->_gcOmrVMThreadExtensions = NULL;
}


intptr_t
gcOmrInitializeDefaults(OMR_VM* omrVM)
{
	MM_EnvironmentBase env(omrVM);

	/* allocate extension structure */

	MM_GCExtensionsBase* extensions = MM_GCExtensionsBase::newInstance(&env);
	if (NULL == extensions) {
		goto error;
	}
	extensions->setOmrVM(omrVM);
	omrVM->_gcOmrVMExtensions = (void *)extensions;

	return 0;

error:
	return -1;
}
#if defined(OMR_RAS_TDF_TRACE)
void
gcOmrInitializeTrace(OMR_VMThread *omrVMThread)
{
	UT_OMRMM_MODULE_LOADED(omrVMThread->_vm->utIntf);
}
#endif /*OMR_RAS_TDF_TRACE */

#ifdef __cplusplus
} /* extern "C" { */
#endif

