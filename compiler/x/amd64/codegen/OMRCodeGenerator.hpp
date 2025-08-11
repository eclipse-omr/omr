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

#ifndef OMR_AMD64_CODEGENERATOR_INCL
#define OMR_AMD64_CODEGENERATOR_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_CODEGENERATOR_CONNECTOR
#define OMR_CODEGENERATOR_CONNECTOR

namespace OMR {
namespace X86 { namespace AMD64 {
class CodeGenerator;
}} // namespace X86::AMD64

typedef OMR::X86::AMD64::CodeGenerator CodeGeneratorConnector;
} // namespace OMR
#else
#error OMR::X86::AMD64::CodeGenerator expected to be a primary connector, but a OMR connector is already defined
#endif

#include "compiler/x/codegen/OMRCodeGenerator.hpp"

#include "codegen/RealRegister.hpp"
#include "codegen/RegisterConstants.hpp"

namespace TR {
class ILOpCode;
class Node;
} // namespace TR

namespace OMR { namespace X86 { namespace AMD64 {

class OMR_EXTENSIBLE CodeGenerator : public OMR::X86::CodeGenerator {
protected:
    CodeGenerator(TR::Compilation *comp);

public:
    void initialize();

    virtual TR::Register *longClobberEvaluate(TR::Node *node);

    TR_GlobalRegisterNumber getLinkageGlobalRegisterNumber(int8_t linkageRegisterIndex, TR::DataType type);

    TR_BitVector *getGlobalGPRsPreservedAcrossCalls() { return &_globalGPRsPreservedAcrossCalls; }

    TR_BitVector *getGlobalFPRsPreservedAcrossCalls() { return &_globalFPRsPreservedAcrossCalls; }

    using OMR::X86::CodeGenerator::getMaximumNumberOfGPRsAllowedAcrossEdge;
    int32_t getMaximumNumberOfGPRsAllowedAcrossEdge(TR::Node *);

    bool opCodeIsNoOpOnThisPlatform(TR::ILOpCode &opCode);

    bool internalPointerSupportImplemented() { return true; }

protected:
    TR_BitVector _globalGPRsPreservedAcrossCalls;
    TR_BitVector _globalFPRsPreservedAcrossCalls;

private:
    TR_GlobalRegisterNumber _gprLinkageGlobalRegisterNumbers[TR::RealRegister::NumRegisters],
        _fprLinkageGlobalRegisterNumbers[TR::RealRegister::NumRegisters];
    void initLinkageToGlobalRegisterMap();
};

}}} // namespace OMR::X86::AMD64

#endif
