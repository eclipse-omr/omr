/*******************************************************************************
 * Copyright IBM Corp. and others 2019
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

#ifndef OMR_RV_REGISTER_DEPENDENCY_INCL
#define OMR_RV_REGISTER_DEPENDENCY_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_REGISTER_DEPENDENCY_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_CONNECTOR
namespace OMR {
namespace RV { class RegisterDependencyConditions; }
typedef OMR::RV::RegisterDependencyConditions RegisterDependencyConditionsConnector;
}
#else
#error OMR::RV::RegisterDependencyConditions expected to be a primary connector, but a OMR connector is already defined
#endif

#ifndef OMR_REGISTER_DEPENDENCY_GROUP_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_GROUP_CONNECTOR
namespace OMR {
namespace RV { class RegisterDependencyGroup; }
typedef OMR::RV::RegisterDependencyGroup RegisterDependencyGroupConnector;
}
#endif

#include "compiler/codegen/OMRRegisterDependency.hpp"

#include <stddef.h>
#include "codegen/CodeGenerator.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/Register.hpp"
#include "env/TRMemory.hpp"
#include "infra/Assert.hpp"

namespace TR {
class Instruction;
class Node;
class RegisterDependencyConditions;
}
namespace OMR
{
namespace RV
{
class OMR_EXTENSIBLE RegisterDependencyGroup : public OMR::RegisterDependencyGroup
   {
   public:

   /**
    * @brief Assigns registers
    * @param[in] currentInstruction : current instruction
    * @param[in] kindToBeAssigned : kind of register to be assigned
    * @param[in] numberOfRegisters : # of registers
    * @param[in] cg : code generator
    */
   void assignRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned, uint32_t numberOfRegisters, TR::CodeGenerator *cg);
   };

class RegisterDependencyConditions: public OMR::RegisterDependencyConditions
   {
   TR::RegisterDependencyGroup *_preConditions;
   TR::RegisterDependencyGroup *_postConditions;
   uint16_t _numPreConditions;
   uint16_t _addCursorForPre;
   uint16_t _numPostConditions;
   uint16_t _addCursorForPost;

   public:

   TR_ALLOC(TR_Memory::RegisterDependencyConditions)

   /**
    * @brief Constructor
    */
   RegisterDependencyConditions()
      : _preConditions(NULL),
        _postConditions(NULL),
        _numPreConditions(0),
        _addCursorForPre(0),
        _numPostConditions(0),
        _addCursorForPost(0)
      {}

   /**
    * @brief Constructor
    * @param[in] numPreConds : # of pre-conditions
    * @param[in] numPostConds : # of post-conditions
    * @param[in] m : memory
    */
   RegisterDependencyConditions(uint16_t numPreConds, uint16_t numPostConds, TR_Memory * m);

   /**
    * @brief Constructor
    * @param[in] cg : CodeGenerator object
    * @param[in] node : node
    * @param[in] extranum : additional # of conditions
    * @param[in] cursorPtr : instruction cursor
    */
   RegisterDependencyConditions(TR::CodeGenerator *cg, TR::Node *node, uint32_t extranum, TR::Instruction **cursorPtr=NULL);

   /**
    * @brief Clones RegisterDependencyConditions
    * @param[in] cg : CodeGenerator
    * @param[in] added : conditions to be added
    * @return cloned RegisterDependencyConditions
    */
   TR::RegisterDependencyConditions *clone(TR::CodeGenerator *cg, TR::RegisterDependencyConditions *added=NULL);

   /**
    * @brief Adds NoReg to post-condition
    * @param[in] reg : register
    * @param[in] cg : CodeGenerator
    */
   void unionNoRegPostCondition(TR::Register *reg, TR::CodeGenerator *cg);

   /**
    * @brief Gets the number of pre-conditions
    * @return the number of pre-conditions
    */
   uint32_t getNumPreConditions() {return _numPreConditions;}

   /**
    * @brief Sets the number of pre-conditions
    * @param[in] n : number of pre-conditions
    * @param[in] m : memory
    * @return the number of pre-conditions
    */
   uint32_t setNumPreConditions(uint16_t n, TR_Memory * m);

   /**
    * @brief Gets the number of post-conditions
    * @return the number of post-conditions
    */
   uint32_t getNumPostConditions() {return _numPostConditions;}

   /**
    * @brief Sets the number of post-conditions
    * @param[in] n : number of pre-conditions
    * @param[in] m : memory
    * @return the number of post-conditions
    */
   uint32_t setNumPostConditions(uint16_t n, TR_Memory * m);

   /**
    * @brief Gets the add-cursor for pre-conditions
    * @return add-cursor for pre-conditions
    */
   uint32_t getAddCursorForPre() {return _addCursorForPre;}
   /**
    * @brief Sets the add-cursor for pre-conditions
    * @param[in] a : new value for add-cursor
    * @return add-cursor for pre-conditions
    */
   uint32_t setAddCursorForPre(uint16_t a) {return (_addCursorForPre = a);}

   /**
    * @brief Gets the add-cursor for post-conditions
    * @return add-cursor for post-conditions
    */
   uint32_t getAddCursorForPost() {return _addCursorForPost;}
   /**
    * @brief Sets the add-cursor for post-conditions
    * @param[in] a : new value for add-cursor
    * @return add-cursor for post-conditions
    */
   uint32_t setAddCursorForPost(uint16_t a) {return (_addCursorForPost = a);}

   /**
    * @brief Gets pre-conditions
    * @return pre-conditions
    */
   TR::RegisterDependencyGroup *getPreConditions()  {return _preConditions;}

   /**
    * @brief Adds to pre-conditions
    * @param[in] vr : register
    * @param[in] rr : register number
    * @param[in] flag : flag
    */
   void addPreCondition(TR::Register *vr, TR::RealRegister::RegNum rr, uint8_t flag = UsesDependentRegister);

   /**
    * @brief Gets post-conditions
    * @return post-conditions
    */
   TR::RegisterDependencyGroup *getPostConditions() {return _postConditions;}

   /**
    * @brief Adds to post-conditions
    * @param[in] vr : register
    * @param[in] rr : register number
    * @param[in] flag : flag
    */
   void addPostCondition(TR::Register *vr, TR::RealRegister::RegNum rr, uint8_t flag = UsesDependentRegister);

   /**
    * @brief Assigns registers to pre-conditions
    * @param[in] currentInstruction : current instruction
    * @param[in] kindToBeAssigned : register kind to be assigned
    * @param[in] cg : CodeGenerator
    */
   void assignPreConditionRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned, TR::CodeGenerator *cg);

   /**
    * @brief Assigns registers to post-conditions
    * @param[in] currentInstruction : current instruction
    * @param[in] kindToBeAssigned : register kind to be assigned
    * @param[in] cg : CodeGenerator
    */
   void assignPostConditionRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned, TR::CodeGenerator *cg);

   /**
    * @brief Searches a register in pre-conditions
    * @param[in] rr : register number
    * @return register
    */
   TR::Register *searchPreConditionRegister(TR::RealRegister::RegNum rr);

   /**
    * @brief Searches a register in post-conditions
    * @param[in] rr : register number
    * @return register
    */
   TR::Register *searchPostConditionRegister(TR::RealRegister::RegNum rr);

   /**
    * @brief References the register or not
    * @param[in] reg : register
    * @return true when it references the register
    */
   bool refsRegister(TR::Register *r);
   /**
    * @brief Defines the register or not
    * @param[in] reg : register
    * @return true when it defines the register
    */
   bool defsRegister(TR::Register *r);
   /**
    * @brief Defines the real register or not
    * @param[in] reg : register
    * @return true when it defines the real register
    */
   bool defsRealRegister(TR::Register *r);
   /**
    * @brief Uses the register or not
    * @param[in] reg : register
    * @return true when it uses the register
    */
   bool usesRegister(TR::Register *r);

   /**
    * @brief Increment totalUseCounts of registers in the RegisterDependencyConditions
    * @param[in] cg : CodeGenerator
    */
   void incRegisterTotalUseCounts(TR::CodeGenerator *cg);

   /**
    * @brief Kills all placeholder registers held by the RegisterDependencyConditions
    * except @param notKilledReg (usually the return register).
    *
    * @param[in] cg : CodeGenerator
    * @param[in] notKilledReg : not-killed register
    */
   void stopUsingDepRegs(TR::CodeGenerator *cg, TR::Register *notKilledReg = NULL);

   };

} // RV
} // OMR

#endif
