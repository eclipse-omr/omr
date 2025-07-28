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

#ifndef OMR_X86_LINKAGE_INCL
#define OMR_X86_LINKAGE_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_LINKAGE_CONNECTOR
#define OMR_LINKAGE_CONNECTOR
namespace OMR {
namespace X86 { class Linkage; }
typedef OMR::X86::Linkage LinkageConnector;
}
#endif

#include "compiler/codegen/OMRLinkage.hpp"

#include <algorithm>
#include <stddef.h>
#include <stdint.h>
#include "codegen/RealRegister.hpp"
#include "codegen/Register.hpp"
#include "codegen/RegisterConstants.hpp"
#include "env/CompilerEnv.hpp"
#include "env/TRMemory.hpp"
#include "il/DataTypes.hpp"
#include "infra/Annotations.hpp"
#include "infra/Assert.hpp"
#include "codegen/InstOpCode.hpp"
#include "codegen/X86LinkageProperties.hpp"

class TR_FrontEnd;
namespace TR {
class AutomaticSymbol;
class CodeGenerator;
class Compilation;
class Instruction;
class MethodSymbol;
class Node;
class ParameterSymbol;
class RegisterDependencyConditions;
class ResolvedMethodSymbol;
}

enum { NOT_LINKAGE = -1 };

typedef enum { Int4, Int8, Float4, Float8, NumMovDataTypes } TR_MovDataTypes;

typedef enum { MemReg, RegMem, RegReg, NumMovOperandTypes }  TR_MovOperandTypes;

namespace TR {

struct MovStatus // What kind of RegReg movs must be applied to a given register
   {
   // NOTE: sourceReg is not to be moved to targetReg.  Rather, we're building
   // a kind of doubly-linked list, with a TR::MovStatus for each register R such
   // that sourceReg will be moved to R and R will be moved to targetReg.
   //
   TR::RealRegister::RegNum sourceReg;        // Which reg will be copied over the given reg
   TR::RealRegister::RegNum targetReg;        // Which reg will receive the given reg's value
   TR_MovDataTypes          outgoingDataType; // Type of data originally in the given reg
   };

}

namespace OMR
{

namespace X86
{

class OMR_EXTENSIBLE Linkage : public OMR::Linkage
   {
   public:

   virtual bool needsFrameDeallocation() { return false; }
   virtual TR::Instruction *deallocateFrameIfNeeded(TR::Instruction *cursor, int32_t size) {return NULL;}

   virtual uint32_t getRightToLeft();

   virtual void mapStack(TR::ResolvedMethodSymbol *method);
   void mapCompactedStack(TR::ResolvedMethodSymbol *method);

   virtual void mapSingleAutomatic(TR::AutomaticSymbol *p, uint32_t &stackIndex);
   void mapSingleAutomatic(TR::AutomaticSymbol *p, uint32_t size, uint32_t &stackIndex);

   virtual TR::Instruction *copyStackParametersToLinkageRegisters(TR::Instruction *procEntryInstruction)
      {
      // Unless overridden, assume jitted code takes parameters on stack, so
      // interpreter entry point needs no special code; thus, it's the same as
      // the jitted->jitted entry point.
      //
      return procEntryInstruction;
      }

   // Generates code to copy arguments to/from the outgoing argument area
   // from/to registers prior to a method call.
   // (load = stack->reg, store = reg->stack)
   //
   uint8_t *loadArguments(TR::Node *callNode, uint8_t *cursor, bool calculateSizeOnly, int32_t *sizeOfFlushArea, bool isReturnAddressOnStack);

   uint8_t *storeArguments(TR::Node *callNode, uint8_t *cursor, bool calculateSizeOnly, int32_t *sizeOfFlushArea);

   TR::Instruction *loadArguments(TR::Instruction *prev, TR::ResolvedMethodSymbol *methodSymbol);

   TR::Instruction *storeArguments(TR::Instruction *prev, TR::ResolvedMethodSymbol *methodSymbol);

   virtual void copyLinkageInfoToParameterSymbols()
      {
      // Unless overridden, just leave all parameter linkage registers at the
      // default of -1.
      }

   virtual void copyGlRegDepsToParameterSymbols(TR::Node *bbStart, TR::CodeGenerator *cg)
      {
      // Unless overridden, just leave all parameter assigned registers at the
      // default of -1.
      }

   virtual const ::TR::X86LinkageProperties& getProperties() = 0;

   virtual int32_t numArgumentRegisters(TR_RegisterKinds kind);

   virtual void createPrologue(TR::Instruction *cursor) = 0;

   virtual void createEpilogue(TR::Instruction *cursor) = 0;

   virtual TR::Register *buildIndirectDispatch(TR::Node *callNode) = 0;

   virtual TR::Register *buildDirectDispatch(TR::Node *callNode, bool spillFPRegs) = 0;

   // Something (eg. recompilation or preexistence) may need the first
   // instruction of a method to be a certain size so it can be safely
   // overwritten without leaving another thread executing in the middle of the
   // new instruction.
   //
   uint8_t getMinimumFirstInstructionSize() { return _minimumFirstInstructionSize; }
   void ensureMinimumFirstInstructionSize(uint8_t mifs){ _minimumFirstInstructionSize = std::max(_minimumFirstInstructionSize, mifs); }

   bool isFloat(TR_MovDataTypes mdt)
      {
      return (mdt == Float4 || mdt == Float8);
      }

   TR_RegisterKinds movRegisterKind(TR_MovDataTypes mdt);

   TR_MovDataTypes movType(TR::DataType type)
      {
      switch(type)
         {
         case TR::Double:
            return Float8;
         case TR::Float:
            return Float4;
         case TR::Int64:
            return Int8;
         case TR::Address:
            return OMR::X86::Linkage::getTargetFromComp().is64Bit() ? Int8 : Int4;
         default:
            return Int4;
         }
      }

   TR_MovDataTypes paramMovType(TR::ParameterSymbol *param);

   TR_MovDataTypes fullRegisterMovType(TR::Register *reg)
      {
      switch(reg->getKind())
         {
         case TR_GPR:
            return OMR::X86::Linkage::getTargetFromComp().is64Bit() ? Int8 : Int4;
         case TR_FPR:
            return Float8;
         default:
            TR_ASSERT(0, "unsupported register kind");
            return Int8;
         }
      }

   static inline TR::InstOpCode::Mnemonic movOpcodes(TR_MovOperandTypes operandType, TR_MovDataTypes dataType)
      {
      TR_ASSERT(OMR::X86::Linkage::getTargetFromComp().is64Bit() || dataType != Int8, "MOV Int8 should not occur on X86-32");
      return _movOpcodes[operandType][dataType];
      }

   void alignLocalObjectWithCollectedFields(uint32_t & stackIndex) {}
   void alignLocalObjectWithoutCollectedFields(uint32_t & stackIndex) {}

   protected:

   Linkage(TR::CodeGenerator *cg);

   void stopUsingKilledRegisters(TR::RegisterDependencyConditions  *deps, TR::Register *returnRegister);
   void associatePreservedRegisters(TR::RegisterDependencyConditions  *deps, TR::Register *returnRegister);
   void coerceFPReturnValueToXMMR(TR::Node *, TR::RegisterDependencyConditions  *, TR::MethodSymbol *, TR::Register *);
   TR::Register *findReturnRegisterFromDependencies(TR::Node *, TR::RegisterDependencyConditions  *, TR::MethodSymbol *, bool);

   virtual void mapIncomingParms(TR::ResolvedMethodSymbol *method);

   virtual uint8_t *flushArguments(TR::Node  *callNode,
                                   uint8_t  *cursor,
                                   bool      calculateSizeOnly,
                                   int32_t  *sizeOfFlushArea,
                                   bool      isReturnAddressOnStack,
                                   bool      isLoad)
      {
      // Unless overridden, assume that all parameters are already in the right
      // place, so no extra code needs to be generated to store them.
      return cursor;
      }

   virtual TR::Instruction *flushArguments(TR::Instruction          *prev,
                                          TR::ResolvedMethodSymbol *methodSymbol,
                                          bool                     isReturnAddressOnStack,
                                          bool                     isLoad)
      {
      // Unless overridden, assume that all parameters are already in the right
      // place, so no extra code needs to be generated to store them.
      return prev;
      }

   virtual void setParameterLinkageRegisterIndex(TR::ResolvedMethodSymbol *method) { }

   private:

   static TR::Environment& getTargetFromComp();

   static TR::InstOpCode::Mnemonic _movOpcodes[NumMovOperandTypes][NumMovDataTypes];
   uint8_t              _minimumFirstInstructionSize;

   };
}
}

#endif
