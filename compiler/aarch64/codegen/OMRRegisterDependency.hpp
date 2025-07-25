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

#ifndef OMR_ARM64_REGISTER_DEPENDENCY_INCL
#define OMR_ARM64_REGISTER_DEPENDENCY_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_REGISTER_DEPENDENCY_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_CONNECTOR

namespace OMR {
namespace ARM64 {
class RegisterDependencyConditions;
}

typedef OMR::ARM64::RegisterDependencyConditions RegisterDependencyConditionsConnector;
} // namespace OMR
#else
#error OMR::ARM64::RegisterDependencyConditions expected to be a primary connector, but a OMR connector is already defined
#endif

#ifndef OMR_REGISTER_DEPENDENCY_GROUP_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_GROUP_CONNECTOR

namespace OMR {
namespace ARM64 {
class RegisterDependencyGroup;
}

typedef OMR::ARM64::RegisterDependencyGroup RegisterDependencyGroupConnector;
} // namespace OMR
#endif

#include "compiler/codegen/OMRRegisterDependency.hpp"

#include <algorithm>
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
} // namespace TR

namespace OMR { namespace ARM64 {
class OMR_EXTENSIBLE RegisterDependencyGroup : public OMR::RegisterDependencyGroup {
public:
    /**
     * @brief Assigns registers
     * @param[in] currentInstruction : current instruction
     * @param[in] kindToBeAssigned : kind of register to be assigned
     * @param[in] numberOfRegisters : # of registers
     * @param[in] cg : code generator
     */
    void assignRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned,
        uint32_t numberOfRegisters, TR::CodeGenerator *cg);
};

class RegisterDependencyConditions : public OMR::RegisterDependencyConditions {
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
        : _preConditions(NULL)
        , _postConditions(NULL)
        , _numPreConditions(0)
        , _addCursorForPre(0)
        , _numPostConditions(0)
        , _addCursorForPost(0)
    {}

    /**
     * @brief Constructor
     * @param[in] numPreConds : # of pre-conditions
     * @param[in] numPostConds : # of post-conditions
     * @param[in] m : memory
     */
    RegisterDependencyConditions(uint16_t numPreConds, uint16_t numPostConds, TR_Memory *m);

    /**
     * @brief Constructor
     * @param[in] cg : CodeGenerator object
     * @param[in] node : node
     * @param[in] extranum : additional # of conditions
     * @param[in] cursorPtr : instruction cursor
     */
    RegisterDependencyConditions(TR::CodeGenerator *cg, TR::Node *node, uint32_t extranum,
        TR::Instruction **cursorPtr = NULL);

    /**
     * @brief Clones RegisterDependencyConditions
     * @param[in] cg : CodeGenerator
     * @param[in] added : conditions to be added
     * @param[in] omitPre : if true, result will have empty pre conditions
     * @param[in] omitPost : if true, result will have empty post conditions
     * @return cloned RegisterDependencyConditions
     */
    TR::RegisterDependencyConditions *clone(TR::CodeGenerator *cg, TR::RegisterDependencyConditions *added = NULL,
        bool omitPre = false, bool omitPost = false);

    /**
     * @brief Clones only pre-condition part of RegisterDependencyConditions
     * @param[in] cg : CodeGenerator
     * @param[in] added : conditions to be added
     * @return cloned RegisterDependencyConditions
     */
    TR::RegisterDependencyConditions *clonePre(TR::CodeGenerator *cg, TR::RegisterDependencyConditions *added = NULL)
    {
        return clone(cg, added, false, true);
    }

    /**
     * @brief Clones only post-condition part of RegisterDependencyConditions
     * @param[in] cg : CodeGenerator
     * @param[in] added : conditions to be added
     * @return cloned RegisterDependencyConditions
     */
    TR::RegisterDependencyConditions *clonePost(TR::CodeGenerator *cg, TR::RegisterDependencyConditions *added = NULL)
    {
        return clone(cg, added, true, false);
    }

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
    uint32_t getNumPreConditions() { return _numPreConditions; }

    /**
     * @brief Sets the number of pre-conditions
     * @param[in] n : number of pre-conditions
     * @param[in] m : memory
     * @return the number of pre-conditions
     */
    uint32_t setNumPreConditions(uint16_t n, TR_Memory *m);

    /**
     * @brief Gets the number of post-conditions
     * @return the number of post-conditions
     */
    uint32_t getNumPostConditions() { return _numPostConditions; }

    /**
     * @brief Sets the number of post-conditions
     * @param[in] n : number of pre-conditions
     * @param[in] m : memory
     * @return the number of post-conditions
     */
    uint32_t setNumPostConditions(uint16_t n, TR_Memory *m);

    /**
     * @brief Gets the add-cursor for pre-conditions
     * @return add-cursor for pre-conditions
     */
    uint32_t getAddCursorForPre() { return _addCursorForPre; }

    /**
     * @brief Sets the add-cursor for pre-conditions
     * @param[in] a : new value for add-cursor
     * @return add-cursor for pre-conditions
     */
    uint32_t setAddCursorForPre(uint16_t a) { return (_addCursorForPre = a); }

    /**
     * @brief Gets the add-cursor for post-conditions
     * @return add-cursor for post-conditions
     */
    uint32_t getAddCursorForPost() { return _addCursorForPost; }

    /**
     * @brief Sets the add-cursor for post-conditions
     * @param[in] a : new value for add-cursor
     * @return add-cursor for post-conditions
     */
    uint32_t setAddCursorForPost(uint16_t a) { return (_addCursorForPost = a); }

    /**
     * @brief Gets pre-conditions
     * @return pre-conditions
     */
    TR::RegisterDependencyGroup *getPreConditions() { return _preConditions; }

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
    TR::RegisterDependencyGroup *getPostConditions() { return _postConditions; }

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
    void assignPreConditionRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned,
        TR::CodeGenerator *cg);

    /**
     * @brief Assigns registers to post-conditions
     * @param[in] currentInstruction : current instruction
     * @param[in] kindToBeAssigned : register kind to be assigned
     * @param[in] cg : CodeGenerator
     */
    void assignPostConditionRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned,
        TR::CodeGenerator *cg);

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
     * @brief Do bookkeeping of use counts of registers in the RegisterDependencyConditions
     * @param[in] instr : instruction
     * @param[in] cg : CodeGenerator
     */
    void bookKeepingRegisterUses(TR::Instruction *instr, TR::CodeGenerator *cg);

    /**
     * @brief Kills placeholder registers held by the RegisterDependencyConditions
     * @param[in] cg : CodeGenerator
     * @param[in] returnRegister : return register
     */
    void stopUsingDepRegs(TR::CodeGenerator *cg, TR::Register *returnRegister = NULL);

    void stopUsingDepRegs(TR::CodeGenerator *cg, TR::Register *ret1, TR::Register *ret2);

    /**
     * @brief Kills placeholder registers held by the RegisterDependencyConditions
     * @param[in] cg : CodeGenerator
     * @param[in] numRetReg : number of regisgters in retReg
     * @param[in] retReg : an array of return registers
     */
    void stopUsingDepRegs(TR::CodeGenerator *cg, int numRetReg, TR::Register **retReg);
};

}} // namespace OMR::ARM64

// Convenience class to temporarily hold register dependencies.
// This class allocates space for the maximum number of GPR dependencies that an evaluator can use
// and therefore frees you from trying to determine ahead of time how many dependencies you need.
// It can create a TR::RegisterDependencyConditions object with exactly the number of dependencies currently defined.
// Therefore, objects of this class should be stack allocated so that they'll automatically be freed when no longer
// needed.
//
class TR_ARM64ScratchRegisterDependencyConditions {
public:
    // NOTE:
    // No TR_Memory type defined for this class
    // since current is as a stack-alloc'd object.
    // To heap-alloc, update the class def with something like:
    //    TR_ALLOC(TR_Memory::PPCScratchRegisterDependencyConditions)

    TR_ARM64ScratchRegisterDependencyConditions()
        : _numGPRDeps(0)
    {}

    uint32_t getNumberOfDependencies() { return _numGPRDeps; }

    /**
     * @brief Add a new dependency
     *
     * @param[in] cg:   code generator
     * @param[in] vr:   virtual register
     * @param[in] rr:   real register
     * @param[in] flag: flag
     *
     */
    void addDependency(TR::CodeGenerator *cg, TR::Register *vr, TR::RealRegister::RegNum rr,
        uint8_t flag = UsesDependentRegister);

    /**
     * @brief Takes union of current dependencies and specified register
     *
     * @details If the virtual register is not in the current dependencies, simply add it to the dependencies.
     *          Otherwise, if either of real register (one specified as a parameter and one in the dependencies) is
     * NoReg, overwrite the dependency of the virtual register with the stronger real register.
     *
     * @param[in] cg:   code generator
     * @param[in] vr:   virtual register
     * @param[in] rr:   real register
     * @param[in] flag: flag
     *
     */
    void unionDependency(TR::CodeGenerator *cg, TR::Register *vr, TR::RealRegister::RegNum rr,
        uint8_t flag = UsesDependentRegister);

    /**
     * @brief Takes union of current dependencies and each scratch register in the scratch register manager, using NoReg
     * as real regisgter.
     *
     * @param[in] cg:   code generator
     * @param[in] srm:  scratch register manager
     */
    void addScratchRegisters(TR::CodeGenerator *cg, TR_ARM64ScratchRegisterManager *srm);

    /**
     * @brief Creates dependency conditions from the scratch register dependency conditions
     *
     * @param[in] cg:   code generator
     * @param[in] pre:  scratch register dependency conditions for pre conditions
     * @param[in] post: scratch register dependency conditions for post conditions
     *
     */
    static TR::RegisterDependencyConditions *createDependencyConditions(TR::CodeGenerator *cg,
        TR_ARM64ScratchRegisterDependencyConditions *pre, TR_ARM64ScratchRegisterDependencyConditions *post);

private:
    uint32_t _numGPRDeps;
    TR::RegisterDependency _gprDeps[TR::RealRegister::LastAssignableGPR - TR::RealRegister::FirstGPR + 1];
};

#endif
