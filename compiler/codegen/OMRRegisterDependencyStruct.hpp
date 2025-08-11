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

#ifndef OMR_REGISTER_DEPENDENCY_STRUCT_INCL
#define OMR_REGISTER_DEPENDENCY_STRUCT_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_REGISTER_DEPENDENCY_STRUCT_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_STRUCT_CONNECTOR
namespace OMR {
struct RegisterDependency;
typedef OMR::RegisterDependency RegisterDependencyConnector;
}
#endif

#include <stdint.h>
#include "codegen/Register.hpp"
#include "codegen/RealRegister.hpp"

#define DefinesDependentRegister    0x01
#define ReferencesDependentRegister 0x02
#define UsesDependentRegister      (ReferencesDependentRegister | DefinesDependentRegister)

namespace OMR
{

struct RegisterDependency
   {
   uint8_t _flags;

   TR::Register *_virtualRegister;

   TR::RealRegister::RegNum _realRegister;

   uint32_t getFlags()             {return _flags;}
   uint32_t assignFlags(uint8_t f) {return _flags = f;}
   uint32_t setFlags(uint8_t f)    {return _flags |= f;}
   uint32_t resetFlags(uint8_t f)  {return _flags &= ~f;}

   uint32_t getDefsRegister()   {return _flags & DefinesDependentRegister;}
   uint32_t setDefsRegister()   {return _flags |= DefinesDependentRegister;}
   uint32_t resetDefsRegister() {return _flags &= ~DefinesDependentRegister;}

   uint32_t getRefsRegister()   {return _flags & ReferencesDependentRegister;}
   uint32_t setRefsRegister()   {return _flags |= ReferencesDependentRegister;}
   uint32_t resetRefsRegister() {return _flags &= ~ReferencesDependentRegister;}

   uint32_t getUsesRegister()   {return _flags & UsesDependentRegister;}

   TR::Register *getRegister()               {return _virtualRegister;}
   TR::Register *setRegister(TR::Register *r) {return (_virtualRegister = r);}

   /**
    * @return RealRegister enum for this register dependency
    */
   TR::RealRegister::RegNum getRealRegister() {return _realRegister;}

   /**
    * @brief Set RealRegister enum value for this register dependency
    *
    * @param[in] r : RealRegister enum value
    */
   TR::RealRegister::RegNum setRealRegister(TR::RealRegister::RegNum r) { return (_realRegister = r); }

#ifndef TR_TARGET_S390
   /**
    * Z already has a \c isSpilledReg() function that is implemented differently.
    * Avoid compiling the common version for now on Z.  Eventually, there should
    * be a unified solution across all platforms.
    */

   /**
    * @return Answers \c true if this register dependency records a register in
    *         spill state; \c false otherwise.
    */
   bool isSpilledReg() { return _realRegister == TR::RealRegister::SpilledReg; }
#endif

   /**
    * @return Answers \c true if this register dependency does not specify an
    *         actual real register.  Some architectures may use this to request
    *         that any available register be assigned.  Answers \c false otherwise.
    */
   bool isNoReg() { return _realRegister == TR::RealRegister::NoReg; }

   };
}

#endif
