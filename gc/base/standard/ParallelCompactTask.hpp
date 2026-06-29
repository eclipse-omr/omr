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

/**
 * @file
 * @ingroup GC_Modron_Standard
 */

#if !defined(COMPACTTASK_HPP_)
#define COMPACTTASK_HPP_

#include "objectdescription.h"

#include "ParallelTask.hpp"

#if defined(OMR_GC_MODRON_COMPACTION)

class MM_CompactScheme;
class MM_EnvironmentBase;
class MM_ParallelDispatcher;

/**
 * @todo Provide class documentation
 * @ingroup GC_Modron_Standard
 */
class MM_ParallelCompactTask : public MM_ParallelTask
{
private:
	MM_CompactScheme *_compactScheme;
	bool _rebuildMarkBits;
	bool _aggressive;
	bool _nurseryOnly; /**< if true, Nursery is only compacted and Tenure fixed-up via RS, otherwise whole heap is compacted/fixed-up */

public:
	virtual uintptr_t getVMStateID();
	
	virtual void run(MM_EnvironmentBase *env);
	virtual void setup(MM_EnvironmentBase *env);
	virtual void cleanup(MM_EnvironmentBase *env);

	/**
	 * Create an ParallelCompactTask object.
	 */
	MM_ParallelCompactTask(MM_EnvironmentBase *env, MM_ParallelDispatcher *dispatcher, MM_CompactScheme *compactScheme, bool rebuildMarkBits, bool aggressive, bool nurseryOnly) :
		MM_ParallelTask(env, dispatcher),
		_compactScheme(compactScheme),
		_rebuildMarkBits(rebuildMarkBits),
		_aggressive(aggressive),
		_nurseryOnly(nurseryOnly)
	{
		_typeId = __FUNCTION__;
	};
};

#endif /* OMR_GC_MODRON_COMPACTION */

#endif /* COMPACTTASK_HPP_ */
