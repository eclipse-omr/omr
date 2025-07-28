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

#ifndef OMR_PPCCONSTANTDATASNIPPET_INCL
#define OMR_PPCCONSTANTDATASNIPPET_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_PPCCONSTANTDATASNIPPET_CONNECTOR
#define OMR_PPCCONSTANTDATASNIPPET_CONNECTOR

namespace OMR {
class ConstantDataSnippet;
typedef OMR::ConstantDataSnippet ConstantDataSnippetConnector;
} // namespace OMR
#endif

#include <stddef.h>
#include <stdint.h>
#include "codegen/CodeGenerator.hpp"
#include "codegen/Instruction.hpp"
#include "codegen/Relocation.hpp"
#include "env/TRMemory.hpp"
#include "env/jittypes.h"
#include "il/DataTypes.hpp"
#include "infra/Array.hpp"
#include "infra/List.hpp"

namespace TR {
class Node;
}

namespace OMR {

template<class T> class PPCConstant {
    TR_Array<TR::Instruction *> _instructionsToPatch;
    T _value;
    TR::Node *_node;
    bool _isUnloadablePicSite;

public:
    TR_ALLOC(TR_Memory::PPCConstant)

    PPCConstant(TR::CodeGenerator *cg, T v, TR::Node *n = NULL, bool ps = false)
        : _instructionsToPatch(cg->trMemory())
        , _value(v)
        , _node(n)
        , _isUnloadablePicSite(ps) {};

    T getConstantValue() { return _value; }

    bool isUnloadablePicSite() { return _isUnloadablePicSite; }

    TR_Array<TR::Instruction *> &getRequestors() { return _instructionsToPatch; }

    void addValueRequest(TR::Instruction *n0, TR::Instruction *n1, TR::Instruction *n2, TR::Instruction *n3)
    {
        if (n0 != NULL) {
            _instructionsToPatch.add(n0);
            n0->setWillBePatched();
        }

        if (n1 != NULL) {
            _instructionsToPatch.add(n1);
            n1->setWillBePatched();
        }

        if (n2 != NULL) {
            _instructionsToPatch.add(n2);
            n2->setWillBePatched();
        }

        if (n3 != NULL) {
            _instructionsToPatch.add(n3);
            n3->setWillBePatched();
        }
    }

    void patchRequestors(TR::CodeGenerator *cg, intptr_t addr)
    {
        if (cg->comp()->target().cpu.isAtLeast(OMR_PROCESSOR_PPC_P10)) {
            for (int32_t i = 0; i < _instructionsToPatch.size(); i++) {
                TR::Instruction *instr = _instructionsToPatch[i];

                // Since prefixed instructions may emit an extra nop for alignment reasons, the actual
                // instruction to be patched may not necessarily be found at the pointer returned by
                // getBinaryEncoding().
                uint32_t *cursor
                    = reinterpret_cast<uint32_t *>(instr->getBinaryEncoding() + instr->getBinaryLength() - 8);
                intptr_t offset = reinterpret_cast<uint8_t *>(addr) - reinterpret_cast<uint8_t *>(cursor);

                TR_ASSERT_FATAL_WITH_INSTRUCTION(instr, offset >= LOWER_IMMED_34 && offset <= UPPER_IMMED_34,
                    "Offset to ConstantDataSnippet is out of range");

                cursor[0] |= (offset >> 16) & 0x3ffff;
                cursor[1] |= offset & 0xffff;
            }
        } else if (cg->comp()->target().is64Bit()) {
            TR_ASSERT_FATAL(_instructionsToPatch.size() % 4 == 0, "Expected groups of 4 requestors");

            intptr_t addrHi = cg->hiValue(addr);
            intptr_t addrLo = LO_VALUE(addr);

            for (int32_t i = 0; i < _instructionsToPatch.size(); i += 4) {
                TR::Instruction *instr1 = _instructionsToPatch[i];
                TR::Instruction *instr2 = _instructionsToPatch[i + 1];
                TR::Instruction *instr3 = _instructionsToPatch[i + 2];
                TR::Instruction *instr4 = _instructionsToPatch[i + 3];

                TR_ASSERT_FATAL_WITH_INSTRUCTION(instr2, instr2->getBinaryEncoding() == instr1->getBinaryEncoding() + 8,
                    "Unexpected ConstantDataSnippet load sequence");
                TR_ASSERT_FATAL_WITH_INSTRUCTION(instr3, instr3->getBinaryEncoding() == instr1->getBinaryEncoding() + 4,
                    "Unexpected ConstantDataSnippet load sequence");
                TR_ASSERT_FATAL_WITH_INSTRUCTION(instr4,
                    instr4->getBinaryEncoding() == instr1->getBinaryEncoding() + 16,
                    "Unexpected ConstantDataSnippet load sequence");

                if (cg->canEmitDataForExternallyRelocatableInstructions()) {
                    *reinterpret_cast<uint32_t *>(instr1->getBinaryEncoding()) |= (addrHi >> 32) & 0xffff;
                    *reinterpret_cast<uint32_t *>(instr2->getBinaryEncoding()) |= (addrHi >> 16) & 0xffff;
                    *reinterpret_cast<uint32_t *>(instr3->getBinaryEncoding()) |= addrHi & 0xffff;
                    *reinterpret_cast<uint32_t *>(instr4->getBinaryEncoding()) |= addrLo & 0xffff;
                } else {
                    cg->addExternalRelocation(new (cg->trHeapMemory())
                                                  TR::BeforeBinaryEncodingExternalRelocation(instr1, (uint8_t *)(addr),
                                                      (uint8_t *)fixedSequence4, TR_FixedSequenceAddress2, cg),
                        __FILE__, __LINE__, instr1->getNode());
                }
            }
        } else {
            TR_ASSERT_FATAL(_instructionsToPatch.size() % 2 == 0, "Expected groups of 2 requestors");

            intptr_t addrHi = cg->hiValue(addr);
            intptr_t addrLo = LO_VALUE(addr);

            for (int32_t i = 0; i < _instructionsToPatch.size(); i += 2) {
                TR::Instruction *instr1 = _instructionsToPatch[i];
                TR::Instruction *instr2 = _instructionsToPatch[i + 1];

                *reinterpret_cast<uint32_t *>(instr1->getBinaryEncoding()) |= addrHi & 0xffff;
                *reinterpret_cast<uint32_t *>(instr2->getBinaryEncoding()) |= addrLo & 0xffff;

                TR_RelocationRecordInformation *recordInfo
                    = (TR_RelocationRecordInformation *)cg->trMemory()->allocateMemory(
                        sizeof(TR_RelocationRecordInformation), heapAlloc);
                recordInfo->data3 = orderedPairSequence1;
                cg->addExternalRelocation(
                    new (cg->trHeapMemory()) TR::ExternalOrderedPair32BitRelocation(instr1->getBinaryEncoding(),
                        instr2->getBinaryEncoding(), (uint8_t *)recordInfo, TR_AbsoluteMethodAddressOrderedPair, cg),
                    __FILE__, __LINE__, instr1->getNode());
            }
        }
    }

    TR::Node *getNode() { return _node; }
};

class ConstantDataSnippet {
    List<PPCConstant<double> > _doubleConstants;
    List<PPCConstant<float> > _floatConstants;
    List<PPCConstant<intptr_t> > _addressConstants;
    uint8_t *_snippetBinaryStart;
    TR::CodeGenerator *_cg;

public:
    TR_ALLOC(TR_Memory::PPCConstantDataSnippet)

    ConstantDataSnippet(TR::CodeGenerator *cg)
        : _cg(cg)
        , _doubleConstants(cg->trMemory())
        , _floatConstants(cg->trMemory())
        , _addressConstants(cg->trMemory()) {};

    uint8_t *getSnippetBinaryStart() { return _snippetBinaryStart; }

    uint8_t *setSnippetBinaryStart(uint8_t *p) { return _snippetBinaryStart = p; }

    void addConstantRequest(void *v, TR::DataType type, TR::Instruction *nibble0, TR::Instruction *nibble1,
        TR::Instruction *nibble2, TR::Instruction *nibble3, TR::Node *node, bool isUnloadablePicSite);

    virtual void emitAddressConstant(PPCConstant<intptr_t> *acursor, uint8_t *codeCursor);

    bool getRequestorsFromNibble(TR::Instruction *nibble, TR::Instruction **q, bool remove);

    virtual uint8_t *emitSnippetBody();
    virtual uint32_t getLength();

    TR::CodeGenerator *cg() { return _cg; }

#ifdef DEBUG
    virtual void print(TR::FILE *outFile);
#endif
};

} // namespace OMR

#endif
