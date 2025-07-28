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

#ifndef OMR_Power_REGISTER_DEPENDENCY_INCL
#define OMR_Power_REGISTER_DEPENDENCY_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_REGISTER_DEPENDENCY_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_CONNECTOR
namespace OMR {
namespace Power { class RegisterDependencyConditions; }
typedef OMR::Power::RegisterDependencyConditions RegisterDependencyConditionsConnector;
}
#else
#error OMR::Power::RegisterDependencyConditions expected to be a primary connector, but a OMR connector is already defined
#endif

#ifndef OMR_REGISTER_DEPENDENCY_GROUP_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_GROUP_CONNECTOR
namespace OMR {
namespace Power { class RegisterDependencyGroup; }
typedef OMR::Power::RegisterDependencyGroup RegisterDependencyGroupConnector;
}
#endif

#include "compiler/codegen/OMRRegisterDependency.hpp"

#include <stddef.h>
#include <stdint.h>
#include "codegen/CodeGenerator.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/Register.hpp"
#include "codegen/RegisterConstants.hpp"
#include "codegen/RegisterDependencyStruct.hpp"
#include "env/TRMemory.hpp"
#include "infra/Assert.hpp"

namespace TR {
class Instruction;
class Node;
class RegisterDependencyConditions;
}

namespace OMR
{
namespace Power
{
class OMR_EXTENSIBLE RegisterDependencyGroup : public OMR::RegisterDependencyGroup
   {
   public:

   void assignRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned, uint32_t numberOfRegisters, TR::CodeGenerator *cg);

   void setExcludeGPR0(TR::Register *r, uint32_t numberOfRegisters)
      {
      for (uint32_t i = 0; i < numberOfRegisters; ++i)
         {
         if (_dependencies[i].getRegister() == r)
            {
            _dependencies[i].setExcludeGPR0();
            // Even if the virtual reg r is in the dependencies multiple times, it's sufficient to exclude gr0 on the first of such dependencies
            // because !gr0 NoReg dependencies are handled before NoReg dependencies, so we'll assign it to !gr0 first and then any remaining
            // dependencies become no-ops
            break;
            }
         }
      }
   };

class RegisterDependencyConditions: public OMR::RegisterDependencyConditions
   {
   TR::RegisterDependencyGroup *_preConditions;
   TR::RegisterDependencyGroup *_postConditions;
   uint16_t                       _numPreConditions;
   uint16_t                       _addCursorForPre;
   uint16_t                       _numPostConditions;
   uint16_t                       _addCursorForPost;

   public:

   TR_ALLOC(TR_Memory::RegisterDependencyConditions)

   RegisterDependencyConditions()
      : _preConditions(NULL),
        _postConditions(NULL),
        _numPreConditions(0),
        _addCursorForPre(0),
        _numPostConditions(0),
        _addCursorForPost(0)
      {}

   RegisterDependencyConditions(uint16_t numPreConds, uint16_t numPostConds, TR_Memory * m);

   RegisterDependencyConditions(TR::CodeGenerator *cg, TR::Node *node, uint32_t extranum, TR::Instruction **cursorPtr=NULL);
   TR::RegisterDependencyConditions *clone(TR::CodeGenerator *cg, TR::RegisterDependencyConditions *added=NULL);
   TR::RegisterDependencyConditions *cloneAndFix(TR::CodeGenerator *cg, TR::RegisterDependencyConditions *added=NULL);

   void unionNoRegPostCondition(TR::Register *reg, TR::CodeGenerator *cg);

   TR::RegisterDependencyGroup *getPreConditions()  {return _preConditions;}

   uint32_t getNumPreConditions() {return _numPreConditions;}

   uint32_t setNumPreConditions(uint16_t n, TR_Memory * m);

   uint32_t getNumPostConditions() {return _numPostConditions;}

   uint32_t setNumPostConditions(uint16_t n, TR_Memory * m);

   uint32_t getAddCursorForPre() {return _addCursorForPre;}
   uint32_t setAddCursorForPre(uint16_t a) {return (_addCursorForPre = a);}

   uint32_t getAddCursorForPost() {return _addCursorForPost;}
   uint32_t setAddCursorForPost(uint16_t a) {return (_addCursorForPost = a);}

   void addPreCondition(TR::Register *vr, TR::RealRegister::RegNum rr, uint8_t flag = UsesDependentRegister);

   TR::RegisterDependencyGroup *getPostConditions() {return _postConditions;}

   void addPostCondition(TR::Register *vr, TR::RealRegister::RegNum rr, uint8_t flag = UsesDependentRegister);

   void assignPreConditionRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned, TR::CodeGenerator *cg);

   void assignPostConditionRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned, TR::CodeGenerator *cg);

   TR::Register *searchPreConditionRegister(TR::RealRegister::RegNum rr);

   TR::Register *searchPostConditionRegister(TR::RealRegister::RegNum rr);

   bool preConditionContainsVirtual(TR::Register *r);

   bool postConditionContainsVirtual(TR::Register *r);

   TR::Register *getTargetRegister(uint32_t index, TR::CodeGenerator * cg);

   TR::Register *getSourceRegister(uint32_t index);

   void stopUsingDepRegs(TR::CodeGenerator *cg, int numRetReg, TR::Register ** retReg);

   void stopUsingDepRegs(TR::CodeGenerator *cg, TR::Register * ret1=NULL, TR::Register *ret2 = NULL);

   bool refsRegister(TR::Register *r);
   bool defsRegister(TR::Register *r);
   bool defsRealRegister(TR::Register *r);
   bool usesRegister(TR::Register *r);

   void bookKeepingRegisterUses(TR::Instruction *instr, TR::CodeGenerator *cg);

   void setPreDependencyExcludeGPR0(TR::Register *r);
   void setPostDependencyExcludeGPR0(TR::Register *r);

   };
}
}

// Convenience class to temporarily hold register dependencies.
// This class allocates space for the maximum number of GPR and CCR dependencies that an evaluator can use
// and therefore frees you from trying to determine ahead of time how many dependencies you need.
// It can create a TR::RegisterDependencyConditions object with exactly the number of dependencies currently defined.
// Therefore, objects of this class should be stack allocated so that they'll automatically be freed when no longer
// needed.
//
class TR_PPCScratchRegisterDependencyConditions
   {
   public:
   // NOTE:
   // No TR_Memory type defined for this class
   // since current is as a stack-alloc'd object.
   // To heap-alloc, update the class def with something like:
   //    TR_ALLOC(TR_Memory::PPCScratchRegisterDependencyConditions)

   TR_PPCScratchRegisterDependencyConditions() : _numGPRDeps(0), _excludeGPR0(0), _numCCRDeps(0) {}

   uint32_t getNumberOfGPRDependencies() { return _numGPRDeps; }
   uint32_t getNumberOfCCRDependencies() { return _numCCRDeps; }
   uint32_t getNumberOfDependencies() { return _numGPRDeps + _numCCRDeps; }

   void addDependency(TR::CodeGenerator *cg, TR::Register *vr, TR::RealRegister::RegNum rr, bool excludeGPR0 = false, uint8_t flag = UsesDependentRegister)
      {
      TR_ASSERT(sizeof(_gprDeps) / sizeof(TR::RegisterDependency) <= sizeof(_excludeGPR0) * 8, "Too many GPR dependencies, use a bit vector if more are needed");
      TR_ASSERT(_numGPRDeps < TR::RealRegister::LastAssignableGPR - TR::RealRegister::FirstGPR + 1, "Too many GPR dependencies");
      TR_ASSERT(_numCCRDeps < TR::RealRegister::LastAssignableCCR - TR::RealRegister::FirstCCR + 1, "Too many CCR dependencies");
      bool isGPR = rr >= TR::RealRegister::FirstGPR && rr <= TR::RealRegister::LastAssignableGPR;
      TR_ASSERT(!excludeGPR0 || isGPR, "Exclude gr0 doesn't make sense for CCR dependency");
      TR_ASSERT(isGPR || (rr >= TR::RealRegister::FirstCCR && rr <= TR::RealRegister::LastAssignableCCR), "Expecting GPR or CCR only");
      if (!vr)
         {
         vr = cg->allocateRegister(isGPR ? TR_GPR : TR_CCR);
         cg->stopUsingRegister(vr);
         }
      if (isGPR)
         {
         _gprDeps[_numGPRDeps].setRegister(vr);
         _gprDeps[_numGPRDeps].assignFlags(flag);
         _gprDeps[_numGPRDeps].setRealRegister(rr);
         _excludeGPR0 = excludeGPR0 ? _excludeGPR0 | 1 << _numGPRDeps : _excludeGPR0 & ~(1 << _numGPRDeps);
         ++_numGPRDeps;
         }
      else
         {
         _ccrDeps[_numCCRDeps].setRegister(vr);
         _ccrDeps[_numCCRDeps].assignFlags(flag);
         _ccrDeps[_numCCRDeps].setRealRegister(rr);
         ++_numCCRDeps;
         }
      }

   static TR::RegisterDependencyConditions* createDependencyConditions(TR::CodeGenerator *cg,
                                                                         TR_PPCScratchRegisterDependencyConditions *pre,
                                                                         TR_PPCScratchRegisterDependencyConditions *post);

   private:
   uint32_t                     _numGPRDeps;
   TR::RegisterDependency      _gprDeps[TR::RealRegister::LastAssignableGPR - TR::RealRegister::FirstGPR + 1];
   uint32_t                     _excludeGPR0;
   uint32_t                     _numCCRDeps;
   TR::RegisterDependency      _ccrDeps[TR::RealRegister::LastAssignableCCR - TR::RealRegister::FirstCCR + 1];
   };

#endif
