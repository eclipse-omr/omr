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

#ifndef OMR_ARM_REGISTER_DEPENDENCY_INCL
#define OMR_ARM_REGISTER_DEPENDENCY_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_REGISTER_DEPENDENCY_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_CONNECTOR

namespace OMR {
namespace ARM {
class RegisterDependencyConditions;
}

typedef OMR::ARM::RegisterDependencyConditions RegisterDependencyConditionsConnector;
} // namespace OMR
#else
#error OMR::ARM::RegisterDependencyConditions expected to be a primary connector, but a OMR connector is already defined
#endif

#ifndef OMR_REGISTER_DEPENDENCY_GROUP_CONNECTOR
#define OMR_REGISTER_DEPENDENCY_GROUP_CONNECTOR

namespace OMR {
namespace ARM {
class RegisterDependencyGroup;
}

typedef OMR::ARM::RegisterDependencyGroup RegisterDependencyGroupConnector;
} // namespace OMR
#endif

#include "compiler/codegen/OMRRegisterDependency.hpp"

#include "codegen/CodeGenerator.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/RegisterDependencyStruct.hpp"
#include "env/IO.hpp"

namespace TR {
class Register;
}

namespace OMR { namespace ARM {
class OMR_EXTENSIBLE RegisterDependencyGroup : public OMR::RegisterDependencyGroup {
public:
    void assignRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned,
        uint32_t numberOfRegisters, TR::CodeGenerator *cg);
};

class RegisterDependencyConditions : public OMR::RegisterDependencyConditions {
    TR::RegisterDependencyGroup *_preConditions;
    TR::RegisterDependencyGroup *_postConditions;
    uint8_t _numPreConditions;
    uint8_t _addCursorForPre;
    uint8_t _numPostConditions;
    uint8_t _addCursorForPost;

public:
    TR_ALLOC(TR_Memory::ARMRegisterDependencyConditions)

    RegisterDependencyConditions()
        : _preConditions(NULL)
        , _postConditions(NULL)
        , _numPreConditions(0)
        , _addCursorForPre(0)
        , _numPostConditions(0)
        , _addCursorForPost(0)
    {}

    RegisterDependencyConditions(uint8_t numPreConds, uint8_t numPostConds, TR_Memory *m);

    RegisterDependencyConditions(TR::Node *node, uint32_t extranum, TR::Instruction **cursorPtr, TR::CodeGenerator *cg);

    TR::RegisterDependencyConditions *clone(TR::CodeGenerator *, TR::RegisterDependencyConditions *added = NULL);
    TR::RegisterDependencyConditions *cloneAndFix(TR::CodeGenerator *, TR::RegisterDependencyConditions *added = NULL);

    void unionNoRegPostCondition(TR::Register *reg, TR::CodeGenerator *cg); /* @@@@ */

    TR::RegisterDependencyGroup *getPreConditions() { return _preConditions; }

    uint32_t getNumPreConditions() { return _numPreConditions; }

    uint32_t setNumPreConditions(uint8_t n, TR_Memory *m);

    uint32_t getNumPostConditions() { return _numPostConditions; }

    uint32_t setNumPostConditions(uint8_t n, TR_Memory *m);

    uint32_t getAddCursorForPre() { return _addCursorForPre; }

    uint32_t setAddCursorForPre(uint8_t a) { return (_addCursorForPre = a); }

    uint32_t getAddCursorForPost() { return _addCursorForPost; }

    uint32_t setAddCursorForPost(uint8_t a) { return (_addCursorForPost = a); }

    void addPreCondition(TR::Register *vr, TR::RealRegister::RegNum rr, uint8_t flag = UsesDependentRegister);

    TR::RegisterDependencyGroup *getPostConditions() { return _postConditions; }

    void addPostCondition(TR::Register *vr, TR::RealRegister::RegNum rr, uint8_t flag = UsesDependentRegister);

    void assignPreConditionRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned,
        TR::CodeGenerator *cg);

    void assignPostConditionRegisters(TR::Instruction *currentInstruction, TR_RegisterKinds kindToBeAssigned,
        TR::CodeGenerator *cg);

    TR::Register *searchPreConditionRegister(TR::RealRegister::RegNum rr);

    TR::Register *searchPostConditionRegister(TR::RealRegister::RegNum rr);

    void stopAddingPreConditions() { _numPreConditions = _addCursorForPre; }

    void stopAddingPostConditions() { _numPostConditions = _addCursorForPost; }

    void stopAddingConditions()
    {
        stopAddingPreConditions();
        stopAddingPostConditions();
    }

    bool refsRegister(TR::Register *r);
    bool defsRegister(TR::Register *r);
    bool defsRealRegister(TR::Register *r);
    bool usesRegister(TR::Register *r);

    void incRegisterTotalUseCounts(TR::CodeGenerator *);
};
}} // namespace OMR::ARM

#endif
