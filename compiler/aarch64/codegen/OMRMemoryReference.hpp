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

#ifndef OMR_ARM64_MEMORY_REFERENCE_INCL
#define OMR_ARM64_MEMORY_REFERENCE_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_MEMREF_CONNECTOR
#define OMR_MEMREF_CONNECTOR

namespace OMR {
namespace ARM64 {
class MemoryReference;
}

typedef OMR::ARM64::MemoryReference MemoryReferenceConnector;
} // namespace OMR
#else
#error OMR::ARM64::MemoryReference expected to be a primary connector, but a OMR connector is already defined
#endif

#include "compiler/codegen/OMRMemoryReference.hpp"

#include <stddef.h>
#include <stdint.h>
#include "codegen/ARM64ShiftCode.hpp"
#include "codegen/InstOpCode.hpp"
#include "codegen/Register.hpp"
#include "env/TRMemory.hpp"
#include "il/SymbolReference.hpp"

namespace TR {
class CodeGenerator;
class Instruction;
class MemoryReference;
class Node;
class UnresolvedDataSnippet;
} // namespace TR

namespace OMR { namespace ARM64 {

class OMR_EXTENSIBLE MemoryReference : public OMR::MemoryReference {
    TR::Register *_baseRegister;
    TR::Node *_baseNode;
    TR::Register *_indexRegister;
    TR::Node *_indexNode;
    intptr_t _offset;
    uint32_t _length;

    TR::UnresolvedDataSnippet *_unresolvedSnippet;
    TR::SymbolReference *_symbolReference;

    // Any downstream project can use this extra register to associate an additional
    // register to a memory reference that can be used for project-specific purposes.
    // This register is in addition to the base and index registers and isn't directly
    // part of the addressing expression in the memory reference.
    // An example of what this could be used for is to help synthesize unresolved data
    // reference addresses at runtime.
    TR::Register *_extraRegister;

    uint8_t _flag;
    uint8_t _scale;

protected:
    /**
     * @brief Constructor
     * @param[in] cg : CodeGenerator object
     */
    MemoryReference(TR::CodeGenerator *cg);

    /**
     * @brief Constructor
     * @param[in] br : base register
     * @param[in] ir : index register
     * @param[in] cg : CodeGenerator object
     */
    MemoryReference(TR::Register *br, TR::Register *ir, TR::CodeGenerator *cg);

    /**
     * @brief Constructor
     * @param[in] br : base register
     * @param[in] ir : index register
     * @param[in] scale : scale of index
     * @param[in] cg : CodeGenerator object
     */
    MemoryReference(TR::Register *br, TR::Register *ir, uint8_t scale, TR::CodeGenerator *cg);

    /**
     * @brief Constructor
     * @param[in] br : base register
     * @param[in] disp : displacement
     * @param[in] cg : CodeGenerator object
     */
    MemoryReference(TR::Register *br, intptr_t disp, TR::CodeGenerator *cg);

    /**
     * @brief Constructor
     * @param[in] node : load or store node
     * @param[in] cg : CodeGenerator object
     */
    MemoryReference(TR::Node *node, TR::CodeGenerator *cg);

    /**
     * @brief Constructor
     * @param[in] node : node
     * @param[in] symRef : symbol reference
     * @param[in] cg : CodeGenerator object
     */
    MemoryReference(TR::Node *node, TR::SymbolReference *symRef, TR::CodeGenerator *cg);

public:
    TR_ALLOC(TR_Memory::MemoryReference)

    typedef enum {
        TR_ARM64MemoryReferenceControl_Base_Modifiable = 0x01,
        TR_ARM64MemoryReferenceControl_Index_Modifiable = 0x02,
        TR_ARM64MemoryReferenceControl_Index_SignExtendedByte = 0x04,
        TR_ARM64MemoryReferenceControl_Index_SignExtendedHalf = 0x08,
        TR_ARM64MemoryReferenceControl_Index_SignExtendedWord = 0x10,
        TR_ARM64MemoryReferenceControl_DelayedOffsetDone = 0x20
        /* To be added more if necessary */
    } TR_ARM64MemoryReferenceControl;

    /**
     * @brief Gets base register
     * @return base register
     */
    TR::Register *getBaseRegister() { return _baseRegister; }

    /**
     * @brief Sets base register
     * @param[in] br : base register
     * @return base register
     */
    TR::Register *setBaseRegister(TR::Register *br) { return (_baseRegister = br); }

    /**
     * @brief Gets base node
     * @return base node
     */
    TR::Node *getBaseNode() { return _baseNode; }

    /**
     * @brief Sets base node
     * @param[in] bn : base node
     * @return base node
     */
    TR::Node *setBaseNode(TR::Node *bn) { return (_baseNode = bn); }

    /**
     * @brief Gets index register
     * @return index register
     */
    TR::Register *getIndexRegister() { return _indexRegister; }

    /**
     * @brief Sets index register
     * @param[in] ir : index register
     * @return index register
     */
    TR::Register *setIndexRegister(TR::Register *ir) { return (_indexRegister = ir); }

    /**
     * @brief Gets index node
     * @return index node
     */
    TR::Node *getIndexNode() { return _indexNode; }

    /**
     * @brief Sets index node
     * @param[in] in : index node
     * @return index node
     */
    TR::Node *setIndexNode(TR::Node *in) { return (_indexNode = in); }

    /**
     * @brief Gets extra register
     * @return extra register
     */
    TR::Register *getExtraRegister() { return _extraRegister; }

    /**
     * @brief Sets extra register
     * @param[in] er : extra register
     * @return extra register
     */
    TR::Register *setExtraRegister(TR::Register *er) { return (_extraRegister = er); }

    /**
     * @brief Gets scale
     * @return scale
     */
    uint8_t getScale() { return _scale; }

    /**
     * @brief Uses indexed form or not
     * @return true when index form is used
     */
    bool useIndexedForm() { return (_indexRegister != NULL); }

    /**
     * @brief Has delayed offset or not
     * @return true when it has delayed offset
     */
    bool hasDelayedOffset()
    {
        if (_symbolReference->getSymbol() != NULL && _symbolReference->getSymbol()->isRegisterMappedSymbol()) {
            return true;
        }

        return false;
    }

    /**
     * @brief Gets offset
     * @param[in] withRegSym : add offset of register mapped symbol if true
     * @return offset
     */
    intptr_t getOffset(bool withRegSym = false)
    {
        intptr_t displacement = _offset;
        if (withRegSym && !isDelayedOffsetDone() && _symbolReference->getSymbol() != NULL
            && _symbolReference->getSymbol()->isRegisterMappedSymbol())
            displacement += _symbolReference->getSymbol()->getOffset();

        return displacement;
    }

    /**
     * @brief Sets offset
     * @param[in] o : offset
     * @return offset
     */
    intptr_t setOffset(intptr_t o) { return _offset = o; }

    /**
     * @brief Answers if MemoryReference refs specified register
     * @param[in] reg : register
     * @return true if MemoryReference refs the register, false otherwise
     */
    bool refsRegister(TR::Register *reg)
    {
        return (reg == _baseRegister || reg == _indexRegister || reg == _extraRegister);
    }

    /**
     * @brief Blocks registers used by MemoryReference
     */
    void blockRegisters()
    {
        if (_baseRegister != NULL) {
            _baseRegister->block();
        }
        if (_indexRegister != NULL) {
            _indexRegister->block();
        }
        if (_extraRegister != NULL) {
            _extraRegister->block();
        }
    }

    /**
     * @brief Unblocks registers used by MemoryReference
     */
    void unblockRegisters()
    {
        if (_baseRegister != NULL) {
            _baseRegister->unblock();
        }
        if (_indexRegister != NULL) {
            _indexRegister->unblock();
        }
        if (_extraRegister != NULL) {
            _extraRegister->unblock();
        }
    }

    /**
     * @brief Base register is modifiable or not
     * @return true when base register is modifiable
     */
    bool isBaseModifiable() { return ((_flag & TR_ARM64MemoryReferenceControl_Base_Modifiable) != 0); }

    /**
     * @brief Sets the BaseModifiable flag
     */
    void setBaseModifiable() { _flag |= TR_ARM64MemoryReferenceControl_Base_Modifiable; }

    /**
     * @brief Clears the BaseModifiable flag
     */
    void clearBaseModifiable() { _flag &= ~TR_ARM64MemoryReferenceControl_Base_Modifiable; }

    /**
     * @brief Index register is modifiable or not
     * @return true when index register is modifiable
     */
    bool isIndexModifiable() { return ((_flag & TR_ARM64MemoryReferenceControl_Index_Modifiable) != 0); }

    /**
     * @brief Sets the IndexModifiable flag
     */
    void setIndexModifiable() { _flag |= TR_ARM64MemoryReferenceControl_Index_Modifiable; }

    /**
     * @brief Clears the IndexModifiable flag
     */
    void clearIndexModifiable() { _flag &= ~TR_ARM64MemoryReferenceControl_Index_Modifiable; }

    /**
     * @brief Index register is sign extended or not
     * @return true when index register is sign extended
     */
    bool isIndexSignExtended()
    {
        return (isIndexSignExtendedWord() || isIndexSignExtendedHalf() || isIndexSignExtendedByte());
    }

    /**
     * @brief The extend code for the index register is SXTB or not
     * @return true when the extend code for index register is SXTB
     */
    bool isIndexSignExtendedByte() { return ((_flag & TR_ARM64MemoryReferenceControl_Index_SignExtendedByte) != 0); }

    /**
     * @brief The extend code for the index register is SXTH or not
     * @return true when the extend code for index register is SXTH
     */
    bool isIndexSignExtendedHalf() { return ((_flag & TR_ARM64MemoryReferenceControl_Index_SignExtendedHalf) != 0); }

    /**
     * @brief The extend code for the index register is SXTW or not
     * @return true when the extend code for index register is SXTW
     */
    bool isIndexSignExtendedWord() { return ((_flag & TR_ARM64MemoryReferenceControl_Index_SignExtendedWord) != 0); }

    /**
     * @brief Sets the IndexSignExtendedByte flag
     */
    void setIndexSignExtendedByte() { _flag |= TR_ARM64MemoryReferenceControl_Index_SignExtendedByte; }

    /**
     * @brief Sets the IndexSignExtendedHalf flag
     */
    void setIndexSignExtendedHalf() { _flag |= TR_ARM64MemoryReferenceControl_Index_SignExtendedHalf; }

    /**
     * @brief Sets the IndexSignExtendedWord flag
     */
    void setIndexSignExtendedWord() { _flag |= TR_ARM64MemoryReferenceControl_Index_SignExtendedWord; }

    /**
     * @brief Clears the IndexSignExtende flag
     */
    void clearIndexSignExtended()
    {
        _flag &= ~(TR_ARM64MemoryReferenceControl_Index_SignExtendedByte
            | TR_ARM64MemoryReferenceControl_Index_SignExtendedHalf
            | TR_ARM64MemoryReferenceControl_Index_SignExtendedWord);
    }

    /**
     * @brief Gets the flag indicating whether the delayed offset has already been applied.
     *
     * When this flag is set, the delayed offset of this MemoryReference from its register-mapped
     * symbol has already been applied to this MemoryReference's internal offset and should not be
     * applied again. When true, getOffset(false) and getOffset(true) are guaranteed to
     * return the same value.
     *
     * @see setDelayedOffsetDone()
     */
    bool isDelayedOffsetDone() { return (_flag & TR_ARM64MemoryReferenceControl_DelayedOffsetDone) != 0; }

    /**
     * @brief Sets the flag to indicate that the delayed offset has been applied.
     *
     * @see isDelayedOffsetDone()
     */
    void setDelayedOffsetDone() { _flag |= TR_ARM64MemoryReferenceControl_DelayedOffsetDone; }

    /**
     * @brief Returns the extend code for the index register
     * @returns the extend code for the index register
     */
    TR::ARM64ExtendCode getIndexExtendCode()
    {
        if (isIndexSignExtendedWord()) {
            return TR::EXT_SXTW;
        } else if (isIndexSignExtendedHalf()) {
            return TR::EXT_SXTH;
        } else if (isIndexSignExtendedByte()) {
            return TR::EXT_SXTB;
        } else {
            return TR::EXT_UXTX;
        }
    }

    /**
     * @brief Gets the unresolved data snippet
     * @return the unresolved data snippet
     */
    TR::UnresolvedDataSnippet *getUnresolvedSnippet() { return _unresolvedSnippet; }

    /**
     * @brief Sets an unresolved data snippet
     * @return the unresolved data snippet
     */
    TR::UnresolvedDataSnippet *setUnresolvedSnippet(TR::UnresolvedDataSnippet *s) { return (_unresolvedSnippet = s); }

    /**
     * @brief Gets the symbol reference
     * @return the symbol reference
     */
    TR::SymbolReference *getSymbolReference() { return _symbolReference; }

    /**
     * @brief Sets symbol
     * @param[in] symbol : symbol to be set
     * @param[in] cg : CodeGenerator
     */
    void setSymbol(TR::Symbol *symbol, TR::CodeGenerator *cg);

    /**
     * @brief Adds to offset
     * @param[in] node : node
     * @param[in] amount : amount to be added to offset
     * @param[in] cg : CodeGenerator
     */
    void addToOffset(TR::Node *node, intptr_t amount, TR::CodeGenerator *cg);

    /**
     * @brief Decrements node reference counts
     * @param[in] cg : CodeGenerator
     */
    void decNodeReferenceCounts(TR::CodeGenerator *cg);

    /**
     * @brief Populates memory reference
     * @param[in] subTree : sub-tree node
     * @param[in] cg : CodeGenerator
     */
    void populateMemoryReference(TR::Node *subTree, TR::CodeGenerator *cg);

    /**
     * @brief Consolidates registers
     * @details Consolidates registers to the a new base register.
     *          Expects that _indexRegister is not NULL.
     *          After consoliation, _indexRegister and _indexNode are set to NULL.
     *
     * @param[in] subTree : sub tree node
     * @param[in] cg : CodeGenerator
     */
    void consolidateRegisters(TR::Node *subTree, TR::CodeGenerator *cg);

    /**
     * @brief Do bookkeeping of use counts of registers in the MemoryReference
     * @param[in] instr : instruction
     * @param[in] cg : CodeGenerator
     */
    void bookKeepingRegisterUses(TR::Instruction *instr, TR::CodeGenerator *cg);

    /**
     * @brief Returns the appropriate opcode mnemonic for specified opcode mnemonic
     * @param[in] mnemonic : mnemonic
     * @return mnemonic
     */
    TR::InstOpCode::Mnemonic mapOpCode(TR::InstOpCode::Mnemonic op);

    /**
     * @brief Assigns registers
     * @param[in] currentInstruction : current instruction
     * @param[in] cg : CodeGenerator
     */
    void assignRegisters(TR::Instruction *currentInstruction, TR::CodeGenerator *cg);

    /**
     * @brief Estimates the length of generated binary
     * @param[in] op : opcode of the instruction to attach this memory reference to
     * @return estimated binary length
     */
    uint32_t estimateBinaryLength(TR::InstOpCode op);

    /**
     * @brief Generates binary encoding
     * @param[in] ci : current instruction
     * @param[in] cursor : instruction cursor
     * @param[in] cg : CodeGenerator
     * @return instruction cursor after encoding
     */
    uint8_t *generateBinaryEncoding(TR::Instruction *ci, uint8_t *cursor, TR::CodeGenerator *cg);

    /**
     * @brief Returns the scale factor for node
     * @param[in] node: node
     * @param[in] cg: CodeGenerator
     * @return scale factor for node
     */
    int32_t getScaleForNode(TR::Node *node, TR::CodeGenerator *cg);

    /**
     * @brief Expands the instruction which uses this memory reference into multiple instructions if necessary
     *
     * @param currentInstruction: current instruction
     * @param cg : CodeGenerator
     * @return the last instruction in the expansion or this instruction if no expansion was
     *          performed.
     */
    TR::Instruction *expandInstruction(TR::Instruction *currentInstruction, TR::CodeGenerator *cg);

    /**
     * @brief Validates the alignment of the immediate offset.
     * @param[in] node: node
     * @param[in] alignment: alignment in bytes
     * @param[in] cg: CodeGenerator
     */
    void validateImmediateOffsetAlignment(TR::Node *node, uint32_t alignment, TR::CodeGenerator *cg);

private:
    /**
     * @brief Moves index register and node to base register and node.
     * @param[in] node: node
     * @param[in] cg: CodeGenerator
     */
    void moveIndexToBase(TR::Node *node, TR::CodeGenerator *cg);

    /**
     * @brief Normalizes the memory reference so that it can be encoded into instruction.
     * @param[in] node: node
     * @param[in] cg: CodeGenerator
     */
    void normalize(TR::Node *node, TR::CodeGenerator *cg);
};

}} // namespace OMR::ARM64

#endif
