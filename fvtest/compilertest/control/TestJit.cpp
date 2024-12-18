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

#include <stdio.h>
#include <string>
#include "compile/CompilationTypes.hpp"
#include "compile/ResolvedMethod.hpp"
#include "control/CompileMethod.hpp"
#include "control/SimpleJit.hpp"
#include "ilgen/IlGeneratorMethodDetails_inlines.hpp"
#include "ilgen/MethodBuilder.hpp"

extern "C"
bool
initializeTestJit()
   {
   return initializeSimpleJit();
   }

extern "C"
bool
initializeJitWithOptions(char *options)
   {
   return initializeSimpleJitWithOptions(options);
   }

extern "C"
bool
initializeJit()
   {
   return initializeSimpleJitWithOptions((char *)"-Xjit:useILValidator");
   }

extern "C"
void
shutdownJit()
   {
   shutdownSimpleJit();
   }

extern "C"
int32_t
compileMethodBuilder(TR::MethodBuilder *m, uint8_t **entry)
   {
   TR::ResolvedMethod resolvedMethod((char *)m->getDefiningFile(),
                                     (char *)m->getDefiningLine(),
                                     (char *)m->GetMethodName(),
                                     m->getNumParameters(),
                                     m->getParameterTypes(),
                                     m->getReturnType(),
                                     0,
                                     static_cast<TR::IlInjector *>(m));
   TR::IlGeneratorMethodDetails details(&resolvedMethod);

   int32_t rc=0;
   *entry = compileMethod(details, warm, rc);
   return rc;
   }
