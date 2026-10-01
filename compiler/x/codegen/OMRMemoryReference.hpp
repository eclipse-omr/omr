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

#ifndef OMR_X86_MEMORY_REFERENCE_INCL
#define OMR_X86_MEMORY_REFERENCE_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_MEMREF_CONNECTOR
#define OMR_MEMREF_CONNECTOR

namespace OMR {
namespace X86 {
class MemoryReference;
}

typedef OMR::X86::MemoryReference MemoryReferenceConnector;
} // namespace OMR
#endif

#include "compiler/codegen/OMRMemoryReference.hpp"

#include <stddef.h>
#include <stdint.h>
#include "codegen/CodeGenerator.hpp"
#include "codegen/Instruction.hpp"
#include "codegen/Machine.hpp"
#include "codegen/RealRegister.hpp"
#include "codegen/Register.hpp"
#include "codegen/Snippet.hpp"
#include "codegen/UnresolvedDataSnippet.hpp"
#include "compile/Compilation.hpp"
#include "env/TRMemory.hpp"
#include "env/jittypes.h"
#include "il/Node.hpp"
#include "il/SymbolReference.hpp"
#include "infra/Bit.hpp"
#include "infra/Flags.hpp"
#include "x/codegen/DataSnippet.hpp"

#include "x/codegen/ConstantDataSnippet.hpp"

#define HIGHEST_STRIDE_MULTIPLIER 8
#define HIGHEST_STRIDE_SHIFT 3

#define IA32SIBPresent 0x04
#define IA32SIBNoIndex 0x20

#define MemRef_ForceWideDisplacement 0x0001
#define MemRef_UnresolvedDataSnippet 0x0002
#define MemRef_NeedExternalCodeAbsoluteRelocation 0x0004
#define MemRef_ForceSIBByte 0x0008
#define MemRef_IgnoreVolatile 0x0020
#define MemRef_ProcessAsFPVolatile 0x0040
#define MemRef_ProcessAsLongVolatileLow 0x0040
#define MemRef_ProcessAsLongVolatileHigh 0x0080
#define MemRef_RequiresLockPrefix 0x0100
#define MemRef_UpcastingMode 0x0200

class TR_OpaqueClassBlock;
class TR_ScratchRegisterManager;

namespace TR {
class LabelSymbol;
class MemoryReference;
} // namespace TR

namespace OMR { namespace X86 {

class OMR_EXTENSIBLE MemoryReference : public OMR::MemoryReference {
protected:
    TR::Register *_baseRegister;
    TR::Node *_baseNode;
    TR::Register *_indexRegister;
    TR::Node *_indexNode;
    TR::Snippet *_dataSnippet;
    TR::LabelSymbol *_label;

    TR::SymbolReference _symbolReference;
    int32_t _reloKind;
    flags16_t _flags;
    uint8_t _stride;

    static const uint8_t _multiplierToStrideMap[HIGHEST_STRIDE_MULTIPLIER + 1];

    /**
     * @brief Initialize the MemoryReference from fields in the provided TR::SymbolReference
     *
     * @param[in] symRef : \c TR::SymbolReference to initialize from
     * @param[in] cg : \c TR::CodeGenerator object
     */
    void initializeFromSymRef(TR::SymbolReference *symRef, TR::CodeGenerator *cg);

public:
    TR_ALLOC(TR_Memory::MemoryReference)

    MemoryReference(TR::CodeGenerator *cg)
        : _baseRegister(NULL)
        , _baseNode(NULL)
        , _indexRegister(NULL)
        , _indexNode(NULL)
        , _dataSnippet(NULL)
        , _label(NULL)
        , _symbolReference(cg->comp()->getSymRefTab())
        , _stride(0)
        , _flags(0)
        , _reloKind(-1)
    {}

    MemoryReference(TR::Register *br, TR::SymbolReference *sr, TR::Register *ir, uint8_t s, TR::CodeGenerator *cg)
        : _baseRegister(br)
        , _baseNode(NULL)
        , _indexRegister(ir)
        , _indexNode(NULL)
        , _dataSnippet(NULL)
        , _label(NULL)
        , _symbolReference(cg->comp()->getSymRefTab())
        , _stride(s)
        , _flags(0)
        , _reloKind(-1)
    {
        _symbolReference.setSymbol(sr->getSymbol());
        _symbolReference.setOffset(sr->getOffset());
        // TODO: Figure out what to do with the aliasing info here. Should replicate,
        //       but may be able to get away without
    }

    MemoryReference(TR::Register *br, TR::Register *ir, uint8_t s, TR::CodeGenerator *cg)
        : _baseRegister(br)
        , _baseNode(NULL)
        , _indexRegister(ir)
        , _indexNode(NULL)
        , _dataSnippet(NULL)
        , _label(NULL)
        , _symbolReference(cg->comp()->getSymRefTab())
        , _stride(s)
        , _flags(0)
        , _reloKind(-1)
    {}

    MemoryReference(TR::Register *br, intptr_t disp, TR::CodeGenerator *cg)
        : _baseRegister(br)
        , _baseNode(NULL)
        , _indexRegister(NULL)
        , _indexNode(NULL)
        , _dataSnippet(NULL)
        , _label(NULL)
        , _symbolReference(cg->comp()->getSymRefTab())
        , _stride(0)
        , _flags(0)
        , _reloKind(-1)
    {
        _symbolReference.setOffset(disp);
    }

    MemoryReference(intptr_t disp, TR::CodeGenerator *cg)
        : _baseRegister(NULL)
        , _baseNode(NULL)
        , _indexRegister(NULL)
        , _indexNode(NULL)
        , _dataSnippet(NULL)
        , _label(NULL)
        , _symbolReference(cg->comp()->getSymRefTab())
        , _stride(0)
        , _flags(0)
        , _reloKind(-1)
    {
        _symbolReference.setOffset(disp);
    }

    MemoryReference(TR::Register *br, TR::Register *ir, uint8_t s, intptr_t disp, TR::CodeGenerator *cg)
        : _baseRegister(br)
        , _baseNode(NULL)
        , _indexRegister(ir)
        , _indexNode(NULL)
        , _dataSnippet(NULL)
        , _label(NULL)
        , _symbolReference(cg->comp()->getSymRefTab())
        , _stride(s)
        , _flags(0)
        , _reloKind(-1)
    {
        _symbolReference.setOffset(disp);
    }

    MemoryReference(TR::X86DataSnippet *cds, TR::CodeGenerator *cg);

    MemoryReference(TR::LabelSymbol *label, TR::CodeGenerator *cg);

    MemoryReference(TR::Node *rootLoadOrStore, TR::CodeGenerator *cg, bool canRematerializeAddressAdds);

    MemoryReference(TR::SymbolReference *symRef, TR::CodeGenerator *cg);

    MemoryReference(TR::SymbolReference *symRef, intptr_t extraOffset, TR::CodeGenerator *cg);

    MemoryReference(TR::MemoryReference &mr, intptr_t extraOffset, TR::CodeGenerator *cg,
        TR_ScratchRegisterManager *srm = NULL);

    OMR_FINAL TR::Register *getBaseRegister() { return _baseRegister; }

    OMR_FINAL TR::Register *setBaseRegister(TR::Register *br) { return (_baseRegister = br); }

    OMR_FINAL TR::Node *getBaseNode() { return _baseNode; }

    OMR_FINAL TR::Node *setBaseNode(TR::Node *bn) { return (_baseNode = bn); }

    OMR_FINAL TR::Register *getIndexRegister() { return _indexRegister; }

    OMR_FINAL TR::Register *setIndexRegister(TR::Register *ir) { return (_indexRegister = ir); }

    OMR_FINAL TR::Node *getIndexNode() { return _indexNode; }

    OMR_FINAL TR::Node *setIndexNode(TR::Node *in) { return (_indexNode = in); }

    TR::Register *getAddressRegister() { return NULL; }

    OMR_FINAL intptr_t getDisplacement();

    OMR_FINAL TR::UnresolvedDataSnippet *getUnresolvedDataSnippet();

    OMR_FINAL TR::UnresolvedDataSnippet *setUnresolvedDataSnippet(TR::UnresolvedDataSnippet *s);

    OMR_FINAL TR::LabelSymbol *getLabel() { return _label; }

    OMR_FINAL TR::LabelSymbol *setLabel(TR::LabelSymbol *l) { return (_label = l); }

    OMR_FINAL bool getFlags() { return _flags.getValue() != 0; }

    OMR_FINAL void setFlags(uint8_t f) { _flags.setValue(0xff, f); }

    OMR_FINAL bool isForceWideDisplacement() { return _flags.testAny(MemRef_ForceWideDisplacement); }

    OMR_FINAL void setForceWideDisplacement() { _flags.set(MemRef_ForceWideDisplacement); }

    OMR_FINAL void resetForceWideDisplacement() { _flags.reset(MemRef_ForceWideDisplacement); }

    OMR_FINAL bool isForceSIBByte() { return _flags.testAny(MemRef_ForceSIBByte); }

    OMR_FINAL void setForceSIBByte() { _flags.set(MemRef_ForceSIBByte); }

    OMR_FINAL bool hasUnresolvedDataSnippet() { return _flags.testAny(MemRef_UnresolvedDataSnippet); }

    OMR_FINAL void setHasUnresolvedDataSnippet() { _flags.set(MemRef_UnresolvedDataSnippet); }

    OMR_FINAL bool inUpcastingMode() { return _flags.testAny(MemRef_UpcastingMode); }

    OMR_FINAL void setInUpcastingMode(bool b = true) { _flags.set(MemRef_UpcastingMode, b); }

    // relevant only for 32 bit
    //
    OMR_FINAL bool processAsLongVolatileLow() { return _flags.testAny(MemRef_ProcessAsLongVolatileLow); }

    OMR_FINAL void setProcessAsLongVolatileLow() { _flags.set(MemRef_ProcessAsLongVolatileLow); }

    // relevant only for 32 bit
    //
    OMR_FINAL bool processAsLongVolatileHigh() { return _flags.testAny(MemRef_ProcessAsLongVolatileHigh); }

    OMR_FINAL void setProcessAsLongVolatileHigh() { _flags.set(MemRef_ProcessAsLongVolatileHigh); }

    // relevant only for 64 bit
    //
    OMR_FINAL bool processAsFPVolatile() { return _flags.testAny(MemRef_ProcessAsFPVolatile); }

    OMR_FINAL void setProcessAsFPVolatile() { _flags.set(MemRef_ProcessAsFPVolatile); }

    OMR_FINAL bool requiresLockPrefix() { return _flags.testAny(MemRef_RequiresLockPrefix); }

    OMR_FINAL void setRequiresLockPrefix() { return _flags.set(MemRef_RequiresLockPrefix); }

    OMR_FINAL bool needsCodeAbsoluteExternalRelocation()
    {
        return _flags.testAny(MemRef_NeedExternalCodeAbsoluteRelocation);
    }

    OMR_FINAL void setNeedsCodeAbsoluteExternalRelocation() { _flags.set(MemRef_NeedExternalCodeAbsoluteRelocation); }

    OMR_FINAL void resetNeedsCodeAbsoluteExternalRelocation()
    {
        _flags.reset(MemRef_NeedExternalCodeAbsoluteRelocation);
    }

    OMR_FINAL bool ignoreVolatile() { return _flags.testAny(MemRef_IgnoreVolatile); }

    OMR_FINAL void setIgnoreVolatile() { _flags.set(MemRef_IgnoreVolatile); }

    OMR_FINAL bool refsRegister(TR::Register *reg)
    {
        if (reg == _baseRegister || reg == _indexRegister) {
            return true;
        }
        return false;
    }

    // Call this method iteratively to get the registers that this memory reference
    // uses. Use an argument of NULL on the first call, and after that use the
    // result of the previous call.
    //
    OMR_FINAL TR::Register *getNextRegister(TR::Register *cur);

    OMR_FINAL uint8_t getStride() { return _stride; }

    OMR_FINAL uint8_t setStride(uint8_t s) { return (_stride = s); }

    OMR_FINAL uint8_t getStrideMultiplier() { return 1 << _stride; }

    OMR_FINAL uint8_t setStrideFromMultiplier(uint8_t s);

    OMR_FINAL TR::SymbolReference &getSymbolReference() { return _symbolReference; }

    OMR_FINAL int32_t isValidStrideMultiplier(int32_t constFactor)
    {
        return ((uint32_t)(constFactor - 1) < HIGHEST_STRIDE_MULTIPLIER) ? isNonNegativePowerOf2(constFactor) : 0;
    }

    OMR_FINAL int32_t isValidStrideShift(int32_t shiftAmount) { return (uint32_t)shiftAmount <= HIGHEST_STRIDE_SHIFT; }

    OMR_FINAL static int32_t getStrideForNode(TR::Node *node, TR::CodeGenerator *cg);

    uint32_t getBinaryLengthLowerBound(TR::CodeGenerator *cg);
    virtual uint32_t estimateBinaryLength(TR::Instruction *containingInstruction, TR::CodeGenerator *cg);
    virtual OMR::X86::EnlargementResult enlarge(TR::CodeGenerator *cg, TR::Instruction *containingInstruction,
        int32_t requestedEnlargementSize, int32_t maxEnlargementSize, bool allowPartialEnlargement);

    virtual void decNodeReferenceCounts(TR::CodeGenerator *cg);

    virtual void stopUsingRegisters(TR::CodeGenerator *cg);

    virtual void useRegisters(TR::Instruction *instr, TR::CodeGenerator *cg);

    bool needsSIBByte() { return _indexRegister ? true : false; } // more needed here for ESP base, etc

    void checkAndDecReferenceCount(TR::Node *node, rcount_t refCount, TR::CodeGenerator *cg);

    void populateMemoryReference(TR::Node *subTree, TR::CodeGenerator *cg, TR::Node *parent = NULL);

    TR::Register *evaluate(TR::Node *subTree, TR::CodeGenerator *cg, TR::Node *parent = NULL);

    void consolidateRegisters(TR::Node *, TR::CodeGenerator *cg);

    /**
     * @brief Perform any necessary final tasks when this memory reference is
     *     attached to an instruction.
     *
     * @details
     *    The provided \c containingInstr should not be considered to be a completely
     *    initialized object as this function is typically called during the
     *    Instruction constructor.  For example, basic inquiries and manipulations
     *    of the prev/next Instruction pointers and the opcode are permitted, but
     *    calling a virtual function on the \c containingInstr is not.
     *
     * @param[in] containingInst : the \c TR::Instruction this memory reference is
     *     attached to
     * @param[in] cg : the \c TR::CodeGenerator object
     *
     */
    void finalizeInstrAttachment(TR::Instruction *containingInstr, TR::CodeGenerator *cg);

    virtual void assignRegisters(TR::Instruction *currentInstruction, TR::CodeGenerator *cg);

    virtual uint8_t *generateBinaryEncoding(uint8_t *modRM, TR::Instruction *containingInstruction,
        TR::CodeGenerator *cg);

    void addMetaDataForCodeAddress(uint32_t addressTypes, uint8_t *cursor, TR::Node *node, TR::CodeGenerator *cg);

    OMR_FINAL inline TR::Instruction::ModRM *ModRM(uint8_t *modrm) const { return (TR::Instruction::ModRM *)modrm; }

    OMR_FINAL inline TR::Instruction::SIB *SIB(uint8_t *sib) const { return (TR::Instruction::SIB *)sib; }

    OMR_FINAL void setStrideFieldInSIB(uint8_t *SIBByte) { ((TR::Instruction::SIB *)SIBByte)->setScale(_stride); }

    virtual void blockRegisters()
    {
        if (_baseRegister) {
            _baseRegister->block();
        }
        if (_indexRegister) {
            _indexRegister->block();
        }
    }

    virtual void unblockRegisters()
    {
        if (_baseRegister) {
            _baseRegister->unblock();
        }
        if (_indexRegister) {
            _indexRegister->unblock();
        }
    }

#ifdef DEBUG
    uint32_t getNumMRReferencedGPRegisters();
#endif

    OMR_FINAL static int32_t convertMultiplierToStride(int32_t multiplier)
    {
        return _multiplierToStrideMap[multiplier];
    }

#if defined(TR_TARGET_64BIT)
    uint8_t rexBits()
    {
        uint8_t rxbBits = 0;
        if (_baseRegister) {
            // Handle the case where the vfp register has not been assigned yet.
            //
            TR::Register *baseRegister;
            if (toRealRegister(_baseRegister)->getRegisterNumber() == TR::RealRegister::vfp) {
                baseRegister = toRealRegister(_baseRegister)->getAssignedRealRegister();
                TR_ASSERT_FATAL(baseRegister, "virtual frame pointer must be assigned before binary encoding!\n");
            } else {
                baseRegister = _baseRegister;
            }
            rxbBits |= toRealRegister(baseRegister)->rexBits(TR::RealRegister::REX_B, false);
        }
        if (_indexRegister)
            rxbBits |= toRealRegister(_indexRegister)->rexBits(TR::RealRegister::REX_X, false);
        return rxbBits ? (TR::RealRegister::REX | rxbBits) : 0;
    }
#else
    uint8_t rexBits() { return 0; }
#endif

    OMR_FINAL int32_t getReloKind() { return _reloKind; }

    OMR_FINAL void setReloKind(int32_t reloKind) { _reloKind = reloKind; }
};
}} // namespace OMR::X86

// Factory functions

namespace TR {
TR::MemoryReference *MRef(TR::CodeGenerator *cg);
TR::MemoryReference *MRef_abs(intptr_t disp, TR::CodeGenerator *cg);
TR::MemoryReference *MRef_Bdisp32(TR::Register *br, intptr_t disp, TR::CodeGenerator *cg);
TR::MemoryReference *MRef_BIS(TR::Register *br, TR::Register *ir, uint8_t s, TR::CodeGenerator *cg);
TR::MemoryReference *MRef_BISdisp32(TR::Register *br, TR::Register *ir, uint8_t s, intptr_t disp32,
    TR::CodeGenerator *cg);
TR::MemoryReference *MRef_const(TR::X86DataSnippet *cds, TR::CodeGenerator *cg);
TR::MemoryReference *MRef_label(TR::LabelSymbol *label, TR::CodeGenerator *cg);
TR::MemoryReference *MRef_node(TR::Node *node, TR::CodeGenerator *cg, bool canRematerializeAddressAdds = true);
TR::MemoryReference *MRef_sym(TR::SymbolReference *sr, TR::CodeGenerator *cg);
TR::MemoryReference *MRef_symOff(TR::SymbolReference *sr, intptr_t extraOffset, TR::CodeGenerator *cg);
TR::MemoryReference *MRef_MRefOff(TR::MemoryReference &mr, intptr_t extraOffset, TR::CodeGenerator *cg);
} // namespace TR

#endif
