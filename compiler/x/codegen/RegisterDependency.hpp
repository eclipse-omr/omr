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

#ifndef TR_REGISTER_DEPENDENCY_INCL
#define TR_REGISTER_DEPENDENCY_INCL

#include "codegen/OMRRegisterDependency.hpp"

#include "codegen/RegisterDependencyStruct.hpp"

class TR_Memory;
namespace TR {
class CodeGenerator;
class Node;
class Register;
}

namespace TR
{
class OMR_EXTENSIBLE RegisterDependencyGroup : public OMR::RegisterDependencyGroupConnector {};

class RegisterDependencyConditions : public OMR::RegisterDependencyConditionsConnector
   {
   public:

   RegisterDependencyConditions() : OMR::RegisterDependencyConditionsConnector () {}

   RegisterDependencyConditions(uint32_t numPreConds, uint32_t numPostConds, TR_Memory * m) :
      OMR::RegisterDependencyConditionsConnector(numPreConds, numPostConds, m) {}

   RegisterDependencyConditions(TR::Node *node, TR::CodeGenerator *cg, uint32_t additionalRegDeps = 0) :
      OMR::RegisterDependencyConditionsConnector(node, cg, additionalRegDeps) {}

   };
}

#endif
