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

#include <errno.h>

#include "TestDriver.hpp"
#include "compile/ResolvedMethod.hpp"
#include "ilgen/IlGeneratorMethodDetails.hpp"
#include "ilgen/IlType.hpp"
#include "ilgen/MethodBuilder.hpp"

namespace TestCompiler
{
void
TestDriver::RunTest()
   {
   compileTestMethods();
   invokeTests();
   }

int32_t
TestDriver::compileMethodBuilder(TR::MethodBuilder *m, uint8_t ** entry)
   {
   const char **paramNames = new const char *[m->getNumParameters()];
   TR::DataType *paramTypes = new TR::DataType[m->getNumParameters()];
   for (int32_t p=0;p < m->getNumParameters();p++)
      {
      paramNames[p] = m->getSymbolName(p);
      paramTypes[p] = m->getParameterTypes()[p]->getPrimitiveType();
      }
   
   TR::ResolvedMethod resolvedMethod((char *)m->getDefiningFile(),
                                     (char *)m->getDefiningLine(),
                                     (char *)m->GetMethodName(),
                                     m->getNumParameters(),
                                     paramNames,
                                     paramTypes,
                                     m->getReturnType()->getPrimitiveType(),
                                     0,
                                     static_cast<TR::IlInjector *>(m));
   TR::IlGeneratorMethodDetails details(&resolvedMethod);

   int32_t rc=0;
   *entry = (uint8_t *) compileMethod(details, warm, rc);

   delete[] paramNames;
   return rc;
   }

} //namespace TestCompiler
