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

#ifndef OMR_Z_REGISTER_DEPENDENCY_STRUCT_INCL
#define OMR_Z_REGISTER_DEPENDENCY_STRUCT_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_REGISTER_DEPENDENCY_STRUCT_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_STRUCT_CONNECTOR
namespace OMR {
namespace Z { struct RegisterDependency; }
typedef OMR::Z::RegisterDependency RegisterDependencyConnector;
}
#else
#error OMR::Z::RegisterDependency expected to be a primary connector, but an OMR connector is already defined
#endif

#include "compiler/codegen/OMRRegisterDependencyStruct.hpp"

#include <stddef.h>
#include "codegen/CodeGenerator.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/Register.hpp"
#include "infra/Array.hpp"
#include "infra/Assert.hpp"

namespace OMR
{
namespace Z
{
struct RegisterDependency : OMR::RegisterDependency
   {

   /**
    * @return Answers \c true if this register dependency represents an even/odd pair;
    *         \c false otherwise.
    */
   bool isEvenOddPair() { return _realRegister == static_cast<TR::RealRegister::RegNum>(TR::RealRegister::EvenOddPair); }

   /**
    * @return Answers \c true if this register dependency requires an even register
    *         that is preceded by an unlocked odd register; \c false otherwise.
    */
   bool isLegalEvenOfPair() { return _realRegister == static_cast<TR::RealRegister::RegNum>(TR::RealRegister::LegalEvenOfPair); }

   /**
    * @return Answers \c true if this register dependency requires an odd register
    *         that is preceded by an unlocked even register; \c false otherwise.
    */
   bool isLegalOddOfPair() { return _realRegister == static_cast<TR::RealRegister::RegNum>(TR::RealRegister::LegalOddOfPair); }

   /**
    * @return Answers \c true if this register dependency represents an FP pair;
    *         \c false otherwise.
    */
   bool isFPPair() { return _realRegister == static_cast<TR::RealRegister::RegNum>(TR::RealRegister::FPPair); }

   /**
    * @return Answers \c true if this register dependency represents the first FP
    *         register of a FP register pair; \c false otherwise.
    */
   bool isLegalFirstOfFPPair() { return _realRegister == static_cast<TR::RealRegister::RegNum>(TR::RealRegister::LegalFirstOfFPPair); }

   /**
    * @return Answers \c true if this register dependency represents the second FP
    *         register of a FP register pair; \c false otherwise.
    */
   bool isLegalSecondOfFPPair() { return _realRegister == static_cast<TR::RealRegister::RegNum>(TR::RealRegister::LegalSecondOfFPPair); }

   /**
    * @return Answers \c true if this register dependency could be assigned any
    *         available real register; \c false otherwise.
    */
   bool isAssignAny() { return _realRegister == static_cast<TR::RealRegister::RegNum>(TR::RealRegister::AssignAny); }

   /**
    * @return Answers \c true if this register dependency represents the requirement
    *         to kill all volatile high registers; \c false otherwise.
    */
   bool isKillVolHighRegs() { return _realRegister == static_cast<TR::RealRegister::RegNum>(TR::RealRegister::KillVolHighRegs); }

   /**
    * @return Answers \c true if this register dependency records a register in
    *         spill state; \c false otherwise.
    */
   bool isSpilledReg() { return _realRegister == static_cast<TR::RealRegister::RegNum>(TR::RealRegister::SpilledReg); }

   };
}
}

#endif
