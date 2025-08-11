/*******************************************************************************
 * Copyright IBM Corp. and others 2018
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

#ifndef OMR_ARM64_MACHINE_INCL
#define OMR_ARM64_MACHINE_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_MACHINE_CONNECTOR
#define OMR_MACHINE_CONNECTOR
namespace OMR {
namespace ARM64 { class Machine; }
typedef OMR::ARM64::Machine MachineConnector;
}
#else
#error OMR::ARM64::Machine expected to be a primary connector, but an OMR connector is already defined
#endif

#include "compiler/codegen/OMRMachine.hpp"

#include "codegen/RealRegister.hpp"
#include "infra/Annotations.hpp"

namespace TR {
class CodeGenerator;
class Instruction;
class Register;
class RegisterDependencyConditions;
}

#define NUM_ARM64_MAXR 32

namespace OMR
{

namespace ARM64
{

class OMR_EXTENSIBLE Machine : public OMR::Machine
   {

public:

   /**
    * @brief Constructor
    * @param[in] cg : the TR::CodeGenerator object
    */
   Machine(TR::CodeGenerator *cg);

   /**
    * @brief Finds the best free register
    * @param[in] currentInstruction : current instruction
    * @param[in] rk : register kind
    * @param[in] considerUnlatched : consider unlatched state or not
    * @param[in] virtualReg : virtual register
    * @return Free RealRegister
    */
   TR::RealRegister *findBestFreeRegister(TR::Instruction *currentInstruction,
                                            TR_RegisterKinds rk,
                                            bool considerUnlatched = false,
                                            TR::Register *virtualReg = NULL);

   /**
    * @brief Finds the best free register
    * @param[in] rk : register kind
    * @param[in] considerUnlatched : consider unlatched state or not
    * @return Free RealRegister
    */
   TR::RealRegister *findBestFreeRegister(TR_RegisterKinds rk,
                                            bool considerUnlatched = false)
      {
      return findBestFreeRegister(NULL, rk, considerUnlatched);
      }

   /**
    * @brief Frees the best register
    * @param[in] currentInstruction : current instruction
    * @param[in] virtualRegister : virtual register
    * @param[in] forced : register to be freed
    * @return Freed RealRegister
    */
   TR::RealRegister *freeBestRegister(TR::Instruction *currentInstruction,
                                        TR::Register *virtualRegister,
                                        TR::RealRegister *forced = NULL);

   /**
    * @brief Reverses the spill state
    * @param[in] currentInstruction : current instruction
    * @param[in] spilledRegister : spilled register
    * @param[in] targetRegister : target register
    * @return Target register
    */
   TR::RealRegister *reverseSpillState(TR::Instruction *currentInstruction,
                                         TR::Register *spilledRegister,
                                         TR::RealRegister *targetRegister = NULL);

   /**
    * @brief Assign a RealRegister for specified Register
    * @param[in] currentInstruction : current instruction
    * @param[in] virtualRegister : virtual register
    * @return Assigned RealRegister
    */
   TR::RealRegister *assignOneRegister(TR::Instruction *currentInstruction,
                                         TR::Register *virtualRegister);

   /**
    * @brief Coerce register assignment
    * @param[in] currentInstruction : current instruction
    * @param[in] virtualRegister : virtual register
    * @param[in] registerNumber : register number
    */
   void coerceRegisterAssignment(TR::Instruction *currentInstruction,
                                 TR::Register *virtualRegister,
                                 TR::RealRegister::RegNum registerNumber);

   /**
    * @brief Returns the "killed" state of Link Register
    * @return true if LR is killed in the method, false otherwise
    */
   bool getLinkRegisterKilled()
      {
      return _registerFile[TR::RealRegister::lr]->getHasBeenAssignedInMethod();
      }

   /**
    * @brief Changes the "killed" state of Link Register
    * @param[in] b : true if LR is killed in the method, false otherwise
    * @return The "killed" state set by the function
    */
   bool setLinkRegisterKilled(bool b)
      {
      return _registerFile[TR::RealRegister::lr]->setHasBeenAssignedInMethod(b);
      }

   /**
    * @brief Take snapshot of the register file
    */
   void takeRegisterStateSnapShot();

   /**
    * @brief Restore the register file from snapshot
    */
   void restoreRegisterStateFromSnapShot();

   /**
    * @brief Creates register dependency conditions for the entry label of cold path of OutOfLineCodeSection
    *
    * @param spilledRegisterList : the list of spilled registers in main line and hot path
    * @return register dependency conditions
    */
   TR::RegisterDependencyConditions *createCondForLiveAndSpilledGPRs(TR::list<TR::Register*> *spilledRegisterList);

   /**
    * @brief Decrease future use count of the register and unlatch it if necessary
    *
    * @param currentInstruction     : instruction
    * @param virtualRegister        : virtual register
    */
   void decFutureUseCountAndUnlatch(TR::Instruction *currentInstruction, TR::Register *virtualRegister);

   // Register Association Stuff ///////////////

   /**
    * @brief Sets register weights from register associations
    *
    */
   void setRegisterWeightsFromAssociations();

   /**
    * @brief Creates a regassoc pseudo instruction
    *
    */
   void createRegisterAssociationDirective(TR::Instruction *cursor);

   /**
    * @brief Returns a virtual register associted to the passed real register.
    *
    * @param regNum                 : real register number
    */
   TR::Register *getVirtualAssociatedWithReal(TR::RealRegister::RegNum regNum)
      {
      return _registerAssociations[regNum];
      }

   /**
    * @brief Associates the virtual register to the real register.
    *
    * @param regNum                 : real register number
    * @param virtReg                : virtual register
    * @returns virtual register
    */
   TR::Register *setVirtualAssociatedWithReal(TR::RealRegister::RegNum regNum, TR::Register *virtReg);


   /**
    * @brief Clears register association
    *
    */
   void clearRegisterAssociations()
      {
      memset(_registerAssociations, 0, sizeof(TR::Register *) * (TR::RealRegister::NumRegisters));
      }

   /**
    * @brief Spills all vector registers
    *
    * @param[in] currentInstruction : current instruction
    */
   void spillAllVectorRegisters(TR::Instruction  *currentInstruction);

private:

   // For register snap shot
   uint16_t                   _registerFlagsSnapShot[TR::RealRegister::NumRegisters];
   TR::RealRegister::RegState _registerStatesSnapShot[TR::RealRegister::NumRegisters];
   TR::Register               *_assignedRegisterSnapShot[TR::RealRegister::NumRegisters];
   TR::Register               *_registerAssociationsSnapShot[TR::RealRegister::NumRegisters];
   uint16_t                   _registerWeightSnapShot[TR::RealRegister::NumRegisters];

   // register association
   TR::Register               *_registerAssociations[TR::RealRegister::NumRegisters];

   void initializeRegisterFile();

   /**
    * @brief Spills the specified virtual register
    *
    * @param[in] currentInstruction : current instruction
    * @param[in] virtReg            : virtual register to spill
    */
   void spillRegister(TR::Instruction *currentInstruction, TR::Register *virtReg);
   };
}
}
#endif
