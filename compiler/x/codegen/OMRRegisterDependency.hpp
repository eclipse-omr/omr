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

#ifndef OMR_X86_REGISTER_DEPENDENCY_INCL
#define OMR_X86_REGISTER_DEPENDENCY_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_REGISTER_DEPENDENCY_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_CONNECTOR

namespace OMR {
namespace X86 {
class RegisterDependencyConditions;
}

typedef OMR::X86::RegisterDependencyConditions RegisterDependencyConditionsConnector;
} // namespace OMR
#else
#error OMR::X86::RegisterDependencyConditions expected to be a primary connector, but a OMR connector is already defined
#endif

#ifndef OMR_REGISTER_DEPENDENCY_GROUP_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_GROUP_CONNECTOR

namespace OMR {
namespace X86 {
class RegisterDependencyGroup;
}

typedef OMR::X86::RegisterDependencyGroup RegisterDependencyGroupConnector;
} // namespace OMR
#endif

#include "compiler/codegen/OMRRegisterDependency.hpp"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
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
} // namespace TR
template<typename ListKind> class List;

namespace OMR { namespace X86 {
class OMR_EXTENSIBLE RegisterDependencyGroup : public OMR::RegisterDependencyGroup {
public:
    RegisterDependencyGroup()
        : OMR::RegisterDependencyGroup()
    {}

    void setDependencyInfo(uint32_t index, TR::Register *vr, TR::RealRegister::RegNum rr, TR::CodeGenerator *cg,
        uint8_t flag = UsesDependentRegister, bool isAssocRegDependency = false);

    TR::RegisterDependency *findDependency(TR::Register *vr, uint32_t stop)
    {
        TR::RegisterDependency *result = NULL;
        for (uint32_t i = 0; !result && (i < stop); i++)
            if (_dependencies[i].getRegister() == vr)
                result = _dependencies + i;
        return result;
    }

    TR::RegisterDependency *findDependency(TR::RealRegister::RegNum rr, uint32_t stop)
    {
        TR::RegisterDependency *result = NULL;
        for (uint32_t i = 0; !result && (i < stop); i++)
            if (_dependencies[i].getRealRegister() == rr)
                result = _dependencies + i;
        return result;
    }

    void assignRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindsToBeAssigned,
        uint32_t numberOfRegisters, TR::CodeGenerator *cg);

    void assignFPRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindsToBeAssigned,
        uint32_t numberOfRegisters, TR::CodeGenerator *cg);

    void blockRealDependencyRegisters(uint32_t numberOfRegisters, TR::CodeGenerator *cg);
    void unblockRealDependencyRegisters(uint32_t numberOfRegisters, TR::CodeGenerator *cg);
};

class RegisterDependencyConditions : public OMR::RegisterDependencyConditions {
    TR::RegisterDependencyGroup *_preConditions;
    TR::RegisterDependencyGroup *_postConditions;
    uint16_t _numPreConditions;
    uint16_t _addCursorForPre;
    uint16_t _numPostConditions;
    uint16_t _addCursorForPost;

    uint32_t unionDependencies(TR::RegisterDependencyGroup *deps, uint32_t cursor, TR::Register *vr,
        TR::RealRegister::RegNum rr, TR::CodeGenerator *cg, uint8_t flag, bool isAssocRegDependency);
    uint32_t unionRealDependencies(TR::RegisterDependencyGroup *deps, uint32_t cursor, TR::Register *vr,
        TR::RealRegister::RegNum rr, TR::CodeGenerator *cg, uint8_t flag, bool isAssocRegDependency);

public:
    TR_ALLOC(TR_Memory::RegisterDependencyConditions)

    RegisterDependencyConditions()
        : _preConditions(NULL)
        , _postConditions(NULL)
        , _numPreConditions(0)
        , _addCursorForPre(0)
        , _numPostConditions(0)
        , _addCursorForPost(0)
    {}

    RegisterDependencyConditions(uint16_t numPreConds, uint16_t numPostConds, TR_Memory *m);

    RegisterDependencyConditions(TR::Node *node, TR::CodeGenerator *cg, uint16_t additionalRegDeps = 0);

    void unionNoRegPostCondition(TR::Register *reg, TR::CodeGenerator *cg);

    TR::RegisterDependencyConditions *clone(TR::CodeGenerator *cg, uint32_t additionalRegDeps = 0);

    TR::RegisterDependencyGroup *getPreConditions() { return _preConditions; }

    uint32_t getNumPreConditions() { return _numPreConditions; }

    uint32_t setNumPreConditions(uint32_t n, TR_Memory *m);

    uint32_t getNumPostConditions() { return _numPostConditions; }

    uint32_t setNumPostConditions(uint32_t n, TR_Memory *m);

    uint32_t getAddCursorForPre() { return _addCursorForPre; }

    uint32_t setAddCursorForPre(uint32_t a) { return (_addCursorForPre = a); }

    uint32_t getAddCursorForPost() { return _addCursorForPost; }

    uint32_t setAddCursorForPost(uint32_t a) { return (_addCursorForPost = a); }

    void addPreCondition(TR::Register *vr, TR::RealRegister::RegNum rr, TR::CodeGenerator *cg,
        uint8_t flag = UsesDependentRegister, bool isAssocRegDependency = false)
    {
        uint32_t newCursor
            = unionRealDependencies(_preConditions, _addCursorForPre, vr, rr, cg, flag, isAssocRegDependency);
        TR_ASSERT(newCursor <= _numPreConditions, "Too many dependencies");
        if (newCursor == _addCursorForPre)
            _numPreConditions--; // A vmThread/ebp dependency was displaced, so there is now one less condition.
        else
            _addCursorForPre = newCursor;
    }

    void unionPreCondition(TR::Register *vr, TR::RealRegister::RegNum rr, TR::CodeGenerator *cg,
        uint8_t flag = UsesDependentRegister, bool isAssocRegDependency = false)
    {
        uint32_t newCursor
            = unionDependencies(_preConditions, _addCursorForPre, vr, rr, cg, flag, isAssocRegDependency);
        TR_ASSERT(newCursor <= _numPreConditions, "Too many dependencies");
        if (newCursor == _addCursorForPre)
            _numPreConditions--; // A union occurred, so there is now one less condition
        else
            _addCursorForPre = newCursor;
    }

    TR::RegisterDependencyGroup *getPostConditions() { return _postConditions; }

    void addPostCondition(TR::Register *vr, TR::RealRegister::RegNum rr, TR::CodeGenerator *cg,
        uint8_t flag = UsesDependentRegister, bool isAssocRegDependency = false)
    {
        uint32_t newCursor
            = unionRealDependencies(_postConditions, _addCursorForPost, vr, rr, cg, flag, isAssocRegDependency);
        TR_ASSERT(newCursor <= _numPostConditions, "Too many dependencies");
        if (newCursor == _addCursorForPost)
            _numPostConditions--; // A vmThread/ebp dependency was displaced, so there is now one less condition.
        else
            _addCursorForPost = newCursor;
    }

    void unionPostCondition(TR::Register *vr, TR::RealRegister::RegNum rr, TR::CodeGenerator *cg,
        uint8_t flag = UsesDependentRegister, bool isAssocRegDependency = false)
    {
        uint32_t newCursor
            = unionDependencies(_postConditions, _addCursorForPost, vr, rr, cg, flag, isAssocRegDependency);
        TR_ASSERT(newCursor <= _numPostConditions, "Too many dependencies");
        if (newCursor == _addCursorForPost)
            _numPostConditions--; // A union occurred, so there is now one less condition
        else
            _addCursorForPost = newCursor;
    }

    TR::RegisterDependency *findPreCondition(TR::Register *vr);
    TR::RegisterDependency *findPostCondition(TR::Register *vr);
    TR::RegisterDependency *findPreCondition(TR::RealRegister::RegNum rr);
    TR::RegisterDependency *findPostCondition(TR::RealRegister::RegNum rr);

    void assignPreConditionRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindsToBeAssigned,
        TR::CodeGenerator *cg);
    void assignPostConditionRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindsToBeAssigned,
        TR::CodeGenerator *cg);

    void blockPreConditionRegisters();
    void unblockPreConditionRegisters();

    void blockPostConditionRegisters();
    void unblockPostConditionRegisters();

    void blockPostConditionRealDependencyRegisters(TR::CodeGenerator *cg);
    void unblockPostConditionRealDependencyRegisters(TR::CodeGenerator *cg);

    void blockPreConditionRealDependencyRegisters(TR::CodeGenerator *cg);
    void unblockPreConditionRealDependencyRegisters(TR::CodeGenerator *cg);

    // All conditions are added - set the number of conditions to be the number
    // currently added
    //
    void stopAddingPreConditions() { _numPreConditions = _addCursorForPre; }

    void stopAddingPostConditions() { _numPostConditions = _addCursorForPost; }

    void stopAddingConditions()
    {
        stopAddingPreConditions();
        stopAddingPostConditions();
    }

    void createRegisterAssociationDirective(TR::Instruction *instruction, TR::CodeGenerator *cg);

    TR::RealRegister *getRealRegisterFromVirtual(TR::Register *virtReg, TR::CodeGenerator *cg);

    bool refsRegister(TR::Register *r);
    bool defsRegister(TR::Register *r);
    bool usesRegister(TR::Register *r);

    void useRegisters(TR::Instruction *instr, TR::CodeGenerator *cg);

#if defined(DEBUG) || defined(PROD_WITH_ASSUMES)
    uint32_t numReferencedGPRegisters(TR::CodeGenerator *);
    uint32_t numReferencedFPRegisters(TR::CodeGenerator *);
    void printFullRegisterDependencyInfo(FILE *pOutFile);
    void printDependencyConditions(TR::RegisterDependencyGroup *conditions, uint32_t numConditions, char *prefix,
        FILE *pOutFile);
#endif /* defined(DEBUG) || defined(PROD_WITH_ASSUMES) */
};
}} // namespace OMR::X86

////////////////////////////////////////////////////
// Generate Routines
////////////////////////////////////////////////////

TR::RegisterDependencyConditions *generateRegisterDependencyConditions(TR::Node *, TR::CodeGenerator *, uint32_t = 0,
    List<TR::Register> * = 0);
TR::RegisterDependencyConditions *generateRegisterDependencyConditions(uint32_t, uint32_t, TR::CodeGenerator *);

#endif /* OMR_X86_REGISTER_DEPENDENCY_INCL */
