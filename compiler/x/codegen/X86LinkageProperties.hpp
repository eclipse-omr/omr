/*******************************************************************************
 * Copyright IBM Corp. and others 2021
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

#ifndef X86LINKAGEPROPERTIES_HPP
#define X86LINKAGEPROPERTIES_HPP

#include <stdint.h>
#include "codegen/RealRegister.hpp"
#include "infra/Assert.hpp"

// linkage properties
#define CallerCleanup 0x0001
#define RightToLeft 0x0002
#define IntegersInRegisters 0x0004
#define LongsInRegisters 0x0008
#define FloatsInRegisters 0x0010
#define EightBytePointers 0x0020
#define EightByteParmSlots 0x0040
#define LinkageRegistersAssignedByCardinalPosition 0x0080
#define CallerFrameAllocatesSpaceForLinkageRegs 0x0100
#define AlwaysDedicateFramePointerRegister 0x0200
#define NeedsThunksForIndirectCalls 0x0400
#define UsesPushesForPreservedRegs 0x0800
#define ReservesOutgoingArgsInPrologue 0x1000
#define UsesRegsForHelperArgs 0x2000

// register flags
#define Preserved 0x01
#define IntegerReturn 0x02
#define IntegerArgument 0x04
#define FloatReturn 0x08
#define FloatArgument 0x10
#define CallerAllocatesBackingStore 0x20
#define CalleeVolatile 0x40

namespace TR {

struct X86LinkageProperties {
    // Limits applicable to all linkages.  They must cover both integer and float registers.
    //
    enum {
        MaxArgumentRegisters = 30,
        MaxReturnRegisters = 3,
        MaxVolatileRegisters = 38,
        MaxScratchRegisters = 5
    };

    uint32_t _properties;
    uint32_t _registerFlags[TR::RealRegister::NumRegisters];

    TR::RealRegister::RegNum _preservedRegisters[TR::RealRegister::NumRegisters];
    TR::RealRegister::RegNum _argumentRegisters[MaxArgumentRegisters];
    TR::RealRegister::RegNum _returnRegisters[MaxReturnRegisters];
    TR::RealRegister::RegNum _volatileRegisters[MaxVolatileRegisters];
    TR::RealRegister::RegNum _scratchRegisters[MaxScratchRegisters];
    TR::RealRegister::RegNum _vtableIndexArgumentRegister; // for icallVMprJavaSendPatchupVirtual
    TR::RealRegister::RegNum _j9methodArgumentRegister; // for icallVMprJavaSendStatic

    uint32_t _preservedRegisterMapForGC;
    TR::RealRegister::RegNum _framePointerRegister;
    TR::RealRegister::RegNum _methodMetaDataRegister;
    int8_t _offsetToFirstParm;
    int8_t _offsetToFirstLocal; // Points immediately after the local with highest address

    uint8_t _numScratchRegisters;

    uint8_t _numberOfVolatileGPRegisters;
    uint8_t _numberOfVolatileXMMRegisters;
    uint8_t _numVolatileRegisters;

    uint8_t _numberOfPreservedGPRegisters;
    uint8_t _numberOfPreservedXMMRegisters;
    uint8_t _numPreservedRegisters;

    uint8_t _maxRegistersPreservedInPrologue;

    uint8_t _numIntegerArgumentRegisters;
    uint8_t _numFloatArgumentRegisters;

    uint8_t _firstIntegerArgumentRegister;
    uint8_t _firstFloatArgumentRegister;

    uint32_t _allocationOrder[TR::RealRegister::NumRegisters];

    uint32_t _OutgoingArgAlignment;

    uint32_t getProperties() const { return _properties; }

    uint32_t getCallerCleanup() const { return (_properties & CallerCleanup); }

    uint32_t passArgsRightToLeft() const { return (_properties & RightToLeft); }

    uint32_t getIntegersInRegisters() const { return (_properties & IntegersInRegisters); }

    uint32_t getLongsInRegisters() const { return (_properties & LongsInRegisters); }

    uint32_t getFloatsInRegisters() const { return (_properties & FloatsInRegisters); }

    uint32_t getAlwaysDedicateFramePointerRegister() const
    {
        return (_properties & AlwaysDedicateFramePointerRegister);
    }

    uint32_t getNeedsThunksForIndirectCalls() const { return (_properties & NeedsThunksForIndirectCalls); }

    uint32_t getUsesPushesForPreservedRegs() const { return (_properties & UsesPushesForPreservedRegs); }

    uint32_t getReservesOutgoingArgsInPrologue() const { return (_properties & ReservesOutgoingArgsInPrologue); }

    uint32_t getUsesRegsForHelperArgs() const { return (_properties & UsesRegsForHelperArgs); }

    uint8_t getPointerSize() const { return (_properties & EightBytePointers) ? 8 : 4; }

    uint8_t getPointerShift() const { return (_properties & EightBytePointers) ? 3 : 2; }

    uint8_t getParmSlotSize() const { return (_properties & EightByteParmSlots) ? 8 : 4; }

    uint8_t getParmSlotShift() const { return (_properties & EightByteParmSlots) ? 3 : 2; }

    uint8_t getGPRWidth() const { return getPointerSize(); }

    uint8_t getRetAddressWidth() const { return getGPRWidth(); }

    uint32_t getLinkageRegistersAssignedByCardinalPosition() const
    {
        return (_properties & LinkageRegistersAssignedByCardinalPosition);
    }

    uint32_t getCallerFrameAllocatesSpaceForLinkageRegisters() const
    {
        return (_properties & CallerFrameAllocatesSpaceForLinkageRegs);
    }

    uint32_t getRegisterFlags(TR::RealRegister::RegNum regNum) const { return _registerFlags[regNum]; }

    uint32_t isPreservedRegister(TR::RealRegister::RegNum regNum) const { return (_registerFlags[regNum] & Preserved); }

    uint32_t isCalleeVolatileRegister(TR::RealRegister::RegNum regNum) const
    {
        return (_registerFlags[regNum] & CalleeVolatile);
    }

    uint32_t isIntegerReturnRegister(TR::RealRegister::RegNum regNum) const
    {
        return (_registerFlags[regNum] & IntegerReturn);
    }

    uint32_t isIntegerArgumentRegister(TR::RealRegister::RegNum regNum) const
    {
        return (_registerFlags[regNum] & IntegerArgument);
    }

    uint32_t isFloatReturnRegister(TR::RealRegister::RegNum regNum) const
    {
        return (_registerFlags[regNum] & FloatReturn);
    }

    uint32_t isFloatArgumentRegister(TR::RealRegister::RegNum regNum) const
    {
        return (_registerFlags[regNum] & FloatArgument);
    }

    uint32_t doesCallerAllocatesBackingStore(TR::RealRegister::RegNum regNum) const
    {
        return (_registerFlags[regNum] & CallerAllocatesBackingStore);
    }

    uint32_t getKilledAndNonReturn(TR::RealRegister::RegNum regNum) const
    {
        return ((_registerFlags[regNum] & (Preserved | IntegerReturn | FloatReturn)) == 0);
    }

    uint32_t getPreservedRegisterMapForGC() const { return _preservedRegisterMapForGC; }

    TR::RealRegister::RegNum getPreservedRegister(uint32_t index) const { return _preservedRegisters[index]; }

    TR::RealRegister::RegNum getArgument(uint32_t index) const { return _argumentRegisters[index]; }

    TR::RealRegister::RegNum getIntegerReturnRegister() const { return _returnRegisters[0]; }

    TR::RealRegister::RegNum getLongLowReturnRegister() const { return _returnRegisters[0]; }

    TR::RealRegister::RegNum getLongHighReturnRegister() const { return _returnRegisters[2]; }

    TR::RealRegister::RegNum getFloatReturnRegister() const { return _returnRegisters[1]; }

    TR::RealRegister::RegNum getDoubleReturnRegister() const { return _returnRegisters[1]; }

    TR::RealRegister::RegNum getFramePointerRegister() const { return _framePointerRegister; }

    TR::RealRegister::RegNum getMethodMetaDataRegister() const { return _methodMetaDataRegister; }

    int32_t getOffsetToFirstParm() const { return _offsetToFirstParm; }

    int32_t getOffsetToFirstLocal() const { return _offsetToFirstLocal; }

    uint8_t getNumIntegerArgumentRegisters() const { return _numIntegerArgumentRegisters; }

    uint8_t getNumFloatArgumentRegisters() const { return _numFloatArgumentRegisters; }

    uint8_t getNumPreservedRegisters() const { return _numPreservedRegisters; }

    uint8_t getNumVolatileRegisters() const { return _numVolatileRegisters; }

    uint8_t getNumScratchRegisters() const { return _numScratchRegisters; }

    uint32_t *getRegisterAllocationOrder() const { return (uint32_t *)_allocationOrder; }

    uint32_t getOutgoingArgAlignment() const { return _OutgoingArgAlignment; }

    uint32_t setOutgoingArgAlignment(uint32_t s) { return (_OutgoingArgAlignment = s); }

    TR::RealRegister::RegNum getIntegerArgumentRegister(uint8_t index) const
    {
        TR_ASSERT(index < getNumIntegerArgumentRegisters(), "assertion failure");
        return _argumentRegisters[_firstIntegerArgumentRegister + index];
    }

    TR::RealRegister::RegNum getFloatArgumentRegister(uint8_t index) const
    {
        TR_ASSERT(index < getNumFloatArgumentRegisters(), "assertion failure");
        return _argumentRegisters[_firstFloatArgumentRegister + index];
    }

    TR::RealRegister::RegNum getArgumentRegister(uint8_t index, bool isFloat) const
    {
        return isFloat ? getFloatArgumentRegister(index) : getIntegerArgumentRegister(index);
    }

    TR::RealRegister::RegNum getIntegerScratchRegister(uint8_t index) const
    {
        TR_ASSERT(index < getNumScratchRegisters(), "assertion failure");
        return _scratchRegisters[index];
    }

    TR::RealRegister::RegNum getVTableIndexArgumentRegister() const { return _vtableIndexArgumentRegister; }

    TR::RealRegister::RegNum getJ9MethodArgumentRegister() const { return _j9methodArgumentRegister; }

    uint8_t getMaxRegistersPreservedInPrologue() const { return _maxRegistersPreservedInPrologue; }

    uint32_t getNumberOfVolatileGPRegisters() const { return _numberOfVolatileGPRegisters; }

    uint32_t getNumberOfVolatileXMMRegisters() const { return _numberOfVolatileXMMRegisters; }

    uint32_t getNumberOfPreservedGPRegisters() const { return _numberOfPreservedGPRegisters; }

    uint32_t getNumberOfPreservedXMMRegisters() const { return _numberOfPreservedXMMRegisters; }
};

} // namespace TR

#endif // X86LINKAGEPROPERTIES_HPP
