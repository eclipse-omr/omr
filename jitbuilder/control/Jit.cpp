/*******************************************************************************
 * Copyright IBM Corp. and others 2014
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

#include <stdio.h>
#include "control/SimpleJit.hpp"
#include "ilgen/MethodBuilder.hpp"

#if defined(AIXPPC)
#include "p/codegen/PPCTableOfConstants.hpp"
#endif

/*
 _____      _                        _
| ____|_  _| |_ ___ _ __ _ __   __ _| |
|  _| \ \/ / __/ _ \ '__| '_ \ / _` | |
| |___ >  <| ||  __/ |  | | | | (_| | |
|_____/_/\_\\__\___|_|  |_| |_|\__,_|_|

 ___       _             __
|_ _|_ __ | |_ ___ _ __ / _| __ _  ___ ___
 | || '_ \| __/ _ \ '__| |_ / _` |/ __/ _ \
 | || | | | ||  __/ |  |  _| (_| | (_|  __/
|___|_| |_|\__\___|_|  |_|  \__,_|\___\___|

*/

// An individual program should link statically against JitBuilder, then call:
//     initializeJit() or initializeJitWithOptions() to initialize the Jit
//     compileMethodBuilder() as many times as needed to create compiled code
//     shuwdownJit() when the test is complete
//




bool
internal_initializeJitWithOptions(char *options)
   {
   return initializeSimpleJitWithOptions(options);
   }

bool
internal_initializeJit()
   {
   return initializeSimpleJit();
   }

int32_t
internal_compileMethodBuilder(TR::MethodBuilder *m, void **entry)
   {
   auto rc = m->Compile(entry);

#if defined(AIXPPC)
   struct FunctionDescriptor
      {
      void* func;
      void* toc;
      void* environment;
      };

   FunctionDescriptor* fd = new FunctionDescriptor();
   fd->func = *entry;
   // TODO: There should really be a better way to get this. Usually, we would use
   // cg->getTOCBase(), but the code generator has already been destroyed by now...
   fd->toc = toPPCTableOfConstants(TR_PersistentMemory::getNonThreadSafePersistentInfo()->getPersistentTOC())->getTOCBase();
   fd->environment = NULL;

   *entry = (uint8_t*) fd;
#endif

   return rc;
   }

void
internal_shutdownJit()
   {
   shutdownSimpleJit();
   }
