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

#ifndef OMR_Z_REGISTER_DEPENDENCY_INCL
#define OMR_Z_REGISTER_DEPENDENCY_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_REGISTER_DEPENDENCY_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_CONNECTOR
namespace OMR {
namespace Z { class RegisterDependencyConditions; }
typedef OMR::Z::RegisterDependencyConditions RegisterDependencyConditionsConnector;
}
#else
#error OMR::Z::RegisterDependencyConditions expected to be a primary connector, but a OMR connector is already defined
#endif

#ifndef OMR_REGISTER_DEPENDENCY_GROUP_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_GROUP_CONNECTOR
namespace OMR {
namespace Z { class RegisterDependencyGroup; }
typedef OMR::Z::RegisterDependencyGroup RegisterDependencyGroupConnector;
}
#endif

#include "compiler/codegen/OMRRegisterDependency.hpp"

#include <stddef.h>
#include <stdint.h>
#include "codegen/CodeGenerator.hpp"
#include "env/FrontEnd.hpp"
#include "codegen/MemoryReference.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/Register.hpp"
#include "codegen/RegisterConstants.hpp"
#include "codegen/RegisterDependencyStruct.hpp"
#include "compile/Compilation.hpp"
#include "env/TRMemory.hpp"
#include "infra/Assert.hpp"

#define RefsAndDefsDependentRegister  (ReferencesDependentRegister | DefinesDependentRegister)

namespace TR {
class Instruction;
class Node;
class RegisterDependencyConditions;
class RegisterPair;
}
template <typename ListKind> class List;

#define RefsAndDefsDependentRegister  (ReferencesDependentRegister | DefinesDependentRegister)

#define NUM_VM_THREAD_REG_DEPS 1
namespace OMR
{
namespace Z
{
class OMR_EXTENSIBLE RegisterDependencyGroup : public OMR::RegisterDependencyGroup
   {
   public:

   RegisterDependencyGroup() : OMR::RegisterDependencyGroup() {}

   uint32_t genBitMapOfAssignableGPRs(TR::CodeGenerator *cg, uint32_t numberOfRegisters);

   void assignRegisters(TR::Instruction  *currentInstruction, TR_RegisterKinds kindToBeAssigned, uint32_t numberOfRegisters, TR::CodeGenerator *cg);

   int8_t getNumUses()
      {
      return _numUses;
      }

   void incNumUses(int8_t n = 1)
      {
      _numUses += n;
      }

   protected:

   };

class RegisterDependencyConditions: public OMR::RegisterDependencyConditions
   {
   TR::RegisterDependencyGroup *_preConditions;
   TR::RegisterDependencyGroup *_postConditions;
   uint16_t                         _numPreConditions;
   uint16_t                         _addCursorForPre;
   uint16_t                         _numPostConditions;
   uint16_t                         _addCursorForPost;
   bool                            _isUsed;
   TR::CodeGenerator               *_cg;

   public:

   TR_ALLOC(TR_Memory::RegisterDependencyConditions)

   RegisterDependencyConditions(TR::CodeGenerator *cg,
                                       TR::Node          *node,
                                       uint32_t          extranum,
                                       TR::Instruction  **cursorPtr);

   // Make a copy of and exiting dep list and make room for new deps.

   RegisterDependencyConditions(TR::RegisterDependencyConditions* iConds, uint16_t numNewPreConds, uint16_t numNewPostConds, TR::CodeGenerator *cg);


   RegisterDependencyConditions( TR::RegisterDependencyConditions * conds_1,
                                        TR::RegisterDependencyConditions * conds_2,
                                        TR::CodeGenerator *cg);

   RegisterDependencyConditions(TR::RegisterDependencyGroup *_preConditions,
                                TR::RegisterDependencyGroup *_postConditions,
                                uint16_t numPreConds, uint16_t numPostConds, TR::CodeGenerator *cg)
      : _preConditions(_preConditions),
        _postConditions(_postConditions),
        _numPreConditions(numPreConds),
        _addCursorForPre(numPreConds),
        _numPostConditions(numPostConds),
        _addCursorForPost(numPostConds),
        _isUsed(false),
        _cg(cg)
      {}

   RegisterDependencyConditions()
      : _preConditions(NULL),
        _postConditions(NULL),
        _numPreConditions(0),
        _addCursorForPre(0),
        _numPostConditions(0),
        _addCursorForPost(0),
        _isUsed(false),
        _cg(NULL)
      {}

   //VMThread work: implicitly add an extra post condition for a possible vm thread
   //register post dependency.  Do not need for pre because they are so
   //infrequent.
   RegisterDependencyConditions(uint16_t numPreConds, uint16_t numPostConds, TR::CodeGenerator *cg);

   void resolveSplitDependencies(
      TR::CodeGenerator* cg,
      TR::Node* Noce, TR::Node* child,
      TR::RegisterPair * regPair, List<TR::Register>& regList,
      TR::Register*& reg, TR::Register*& highReg,
      TR::Register*& copyReg, TR::Register*& highCopyReg,
      TR::Instruction*& iCursor,
      TR::RealRegister::RegNum& regNum);

   bool getIsUsed() {return _isUsed;}
   void setIsUsed() {_isUsed=true;}
   void resetIsUsed() {_isUsed=false;}

   TR::RegisterDependencyGroup *getPreConditions()  {return _preConditions;}

   void unionNoRegPostCondition(TR::Register *reg, TR::CodeGenerator *cg)
      {
      addPostCondition(reg, TR::RealRegister::AssignAny);
      }

   uint32_t getNumPreConditions() {return _numPreConditions;}

   uint32_t setNumPreConditions(uint16_t n, TR_Memory * m);

   uint32_t getNumPostConditions() {return _numPostConditions;}

   uint32_t setNumPostConditions(uint16_t n, TR_Memory * m);

   uint32_t getAddCursorForPre() {return _addCursorForPre;}
   uint32_t setAddCursorForPre(uint16_t a) {return _addCursorForPre = a;}

   uint32_t getAddCursorForPost() {return _addCursorForPost;}
   uint32_t setAddCursorForPost(uint16_t a) {return _addCursorForPost = a;}

   void addAssignAnyPreCondOnMemRef(TR::MemoryReference* memRef)
      {
      TR::Register* base = memRef->getBaseRegister();
      TR::Register* indx = memRef->getIndexRegister();

      if (base != NULL)
         {
         addPreConditionIfNotAlreadyInserted(base, TR::RealRegister::AssignAny);
         }
      if (indx != NULL)
         {
         addPreConditionIfNotAlreadyInserted(indx, TR::RealRegister::AssignAny);
         }
      }

   void addPreCondition(TR::Register                              *vr,
                        TR::RealRegister::RegNum rr,
                        uint8_t                                   flag = ReferencesDependentRegister)
       {
       addPreCondition(vr, static_cast<TR::RealRegister::RegDep>(rr), flag);
       }

   void addPreCondition(TR::Register                              *vr,
                        TR::RealRegister::RegDep rr,
                        uint8_t                                   flag = ReferencesDependentRegister);

   TR::RegisterDependencyGroup *getPostConditions() {return _postConditions;}

   void addAssignAnyPostCondOnMemRef(TR::MemoryReference* memRef)
      {
      TR::Register* base = memRef->getBaseRegister();
      TR::Register* indx = memRef->getIndexRegister();

      if (base != NULL)
         {
         addPostConditionIfNotAlreadyInserted(base, TR::RealRegister::AssignAny, ReferencesDependentRegister);
         }
      if (indx != NULL)
         {
         addPostConditionIfNotAlreadyInserted(indx, TR::RealRegister::AssignAny, ReferencesDependentRegister);
         }
      }

   void addPostCondition(TR::Register                              *vr,
                         TR::RealRegister::RegNum rr,
                         uint8_t                                   flag = ReferencesDependentRegister)
      {
      addPostCondition(vr, static_cast<TR::RealRegister::RegDep>(rr), flag);
      }

   void addPostCondition(TR::Register *vr, TR::RealRegister::RegDep rr, uint8_t flag = ReferencesDependentRegister);

   void assignPreConditionRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned, TR::CodeGenerator *cg);

   void assignPostConditionRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned, TR::CodeGenerator *cg);

   TR::Register *searchPreConditionRegister(TR::RealRegister::RegNum rr, uint8_t flag = RefsAndDefsDependentRegister);

   TR::Register *searchPostConditionRegister(TR::RealRegister::RegNum rr, uint8_t flag = RefsAndDefsDependentRegister);

   TR::Register *searchPreConditionRegister(TR::Register * vr, uint8_t flag = RefsAndDefsDependentRegister);

   TR::Register *searchPostConditionRegister(TR::Register * vr, uint8_t flag = RefsAndDefsDependentRegister);

   int32_t searchPostConditionRegisterPos(TR::Register * vr, uint8_t flag = RefsAndDefsDependentRegister);

   void stopUsingDepRegs(TR::CodeGenerator *cg, TR::Register *ret1=NULL, TR::Register *ret2=NULL);


   // These methods are temporary until dependencies are re-engineered down the road
   // We only add in the post condition if the desired virt reg is not already spoken for

   /**
    * @brief Adds the provided \c TR::RegisterDependency to the set of preconditions if it
    *        has not been added already
    *
    * @param[in] \a regDep : the \c TR::RegisterDependency to add
    *
    * @return \c true if successfully added; \c false otherwise
    */
   bool addPreConditionIfNotAlreadyInserted(TR::RegisterDependency *regDep);
   bool addPreConditionIfNotAlreadyInserted(TR::Register *vr,
                                            TR::RealRegister::RegNum rr,
                                            uint8_t flag = ReferencesDependentRegister);
   bool addPreConditionIfNotAlreadyInserted(TR::Register *vr,
                                            TR::RealRegister::RegDep rr,
                                            uint8_t flag = ReferencesDependentRegister);

   /**
    * @brief Adds the provided \c TR::RegisterDependency to the set of postconditions if it
    *        has not been added already
    *
    * @param[in] \a regDep : the \c TR::RegisterDependency to add
    *
    * @return \c true if successfully added; \c false otherwise
    */
   bool addPostConditionIfNotAlreadyInserted(TR::RegisterDependency *regDep);

   bool addPostConditionIfNotAlreadyInserted(TR::Register *vr,
                                             TR::RealRegister::RegNum rr,
                                             uint8_t flag = ReferencesDependentRegister);
   bool addPostConditionIfNotAlreadyInserted(TR::Register *vr,
                                             TR::RealRegister::RegDep rr,
                                             uint8_t flag = ReferencesDependentRegister);

   TR::RegisterDependencyConditions *clone(TR::CodeGenerator *cg, int32_t additionalRegDeps);

   bool refsRegister(TR::Register *r);
   bool defsRegister(TR::Register *r);
   bool usesRegister(TR::Register *r);

   void bookKeepingRegisterUses(TR::Instruction *instr, TR::CodeGenerator *cg, int32_t oldPreCursor=0, int32_t oldPostCursor=0);
   void createRegisterAssociationDirective(TR::Instruction *instruction, TR::CodeGenerator *cg);

   /**
    * @brief Inquires whether a register dependency exists within a given \c TR::RegisterDependencyGroup
    *
    * @param[in] \a regDepArr : the \c TR::RegisterDependencyGroup to check
    * @param[in] \a regDep : the \c TR::RegisterDependency to check for inclusion
    * @param[in] \a numberOfRegisters : the number of registers in the dependency group to check
    *
    * @return \c true if the register dependency is found; \c false otherwise
    */
   bool doesConditionExist( TR::RegisterDependencyGroup * regDepArr, TR::RegisterDependency *regDep, uint32_t numberOfRegisters );

   /**
    * @brief Inquires whether a given register dependency exists within the preconditions
    *
    * @param[in] \a regDep : the \c TR::RegisterDependency to check for inclusion
    *
    * @return \c true if the register dependency is found; \c false otherwise
    */
   bool doesPreConditionExist( TR::RegisterDependency *regDep );

   /**
    * @brief Inquires whether a given register dependency exists within the postconditions
    *
    * @param[in] \a regDep : the \c TR::RegisterDependency to check for inclusion
    *
    * @return \c true if the register dependency is found; \c false otherwise
    */
   bool doesPostConditionExist( TR::RegisterDependency *regDep );

   TR::CodeGenerator *cg()   { return _cg; }
   };

}
}

#endif /* OMR_Z_REGISTER_DEPENDENCY_INCL */
