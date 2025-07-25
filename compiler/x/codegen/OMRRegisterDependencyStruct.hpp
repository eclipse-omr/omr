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

#ifndef OMR_X86_REGISTER_DEPENDENCY_STRUCT_INCL
#define OMR_X86_REGISTER_DEPENDENCY_STRUCT_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_REGISTER_DEPENDENCY_STRUCT_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_STRUCT_CONNECTOR
namespace OMR {
namespace X86 { struct RegisterDependency; }
typedef OMR::X86::RegisterDependency RegisterDependencyConnector;
}
#else
#error OMR::X86::RegisterDependency expected to be a primary connector, but an OMR connector is already defined
#endif

#include "compiler/codegen/OMRRegisterDependencyStruct.hpp"

#include <stdint.h>

namespace OMR
{
namespace X86
{

struct RegisterDependency : OMR::RegisterDependency
   {
   /**
    * @return Answers \c true if this register dependency refers to all x87 floating
    *         point registers collectively; \c false otherwise.
    */
   bool isAllFPRegisters() { return _realRegister == TR::RealRegister::AllFPRegisters; }

   /**
    * @return Answers \c true if this register dependency is a request for the
    *         best free register from the perspective of the register assigner;
    *         \c false otherwise.
    */
   bool isBestFreeReg() { return _realRegister == TR::RealRegister::BestFreeReg; }

   /**
    * @return Answers \c true if this register dependency is a request for a
    *         register that can be used as the byte operand in certain machine
    *         instructions; \c false otherwise.
    */
   bool isByteReg() { return _realRegister == TR::RealRegister::ByteReg; }
   };
}
}

#endif
