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

#ifndef OMR_SIMPLIFIERHELPERS_INCL
#define OMR_SIMPLIFIERHELPERS_INCL

#include "il/DataTypes.hpp"
#include "il/ILOps.hpp"
#include "optimizer/Simplifier.hpp"

namespace TR {
class Block;
class Compilation;
class Node;
class Optimization;
class TreeTop;
} // namespace TR
class TR_RegionStructure;
class TR_FrontEnd;

enum {
    XXCMP_EQ = 0,
    XXCMP_LT = 1,
    XXCMP_GT = 2
};

#define XXCMP_TABLE { 0, -1, 1 }

#define XXCMP_SIMPLIFIER(node, block, s, Type)                                                      \
    {                                                                                               \
        s->simplifyChildren(node, block);                                                           \
        TR::Node *firstChild = node->getFirstChild();                                               \
        TR::Node *secondChild = node->getSecondChild();                                             \
        int8_t table[3] = XXCMP_TABLE;                                                              \
        if (firstChild == secondChild)                                                              \
            foldByteConstant(node, table[XXCMP_EQ], s, true /* anchorChildren */);                  \
        else if (firstChild->getOpCode().isLoadConst() && secondChild->getOpCode().isLoadConst()) { \
            if (firstChild->get##Type() > secondChild->get##Type())                                 \
                foldByteConstant(node, table[XXCMP_GT], s, false /* !anchorChildren*/);             \
            else if (firstChild->get##Type() < secondChild->get##Type())                            \
                foldByteConstant(node, table[XXCMP_LT], s, false /* !anchorChildren*/);             \
            else if (firstChild->get##Type() == secondChild->get##Type())                           \
                foldByteConstant(node, table[XXCMP_EQ], s, false /* !anchorChildren*/);             \
        }                                                                                           \
    }

/*
 * Helper functions needed by simplifier handlers across projects
 */

template<typename T> inline void setCCAddSigned(T value, T operand1, T operand2, TR::Node *node, TR::Simplifier *s)
{
    // for an add (T = A + B) an overflow occurs iff the sign(A) == sign(B) and sign(T) != sign(A)
    if (((operand1 < 0) == (operand2 < 0)) && !((value < 0) == (operand1 < 0)))
        s->setCC(node, OMR::ConditionCode3);
    else if (value < 0)
        s->setCC(node, OMR::ConditionCode1);
    else if (value > 0)
        s->setCC(node, OMR::ConditionCode2);
    else
        s->setCC(node, OMR::ConditionCode0);
}

template<typename T> inline void setCCAddUnsigned(T value, T operand1, TR::Node *node, TR::Simplifier *s)
{
    bool carry = value < operand1;

    if (value == 0 && !carry)
        s->setCC(node, OMR::ConditionCode0);
    else if (value != 0 && !carry)
        s->setCC(node, OMR::ConditionCode1);
    else if (value == 0 && carry)
        s->setCC(node, OMR::ConditionCode2);
    else
        s->setCC(node, OMR::ConditionCode3);
}

template<typename T> inline void setCCSubSigned(T value, T operand1, T operand2, TR::Node *node, TR::Simplifier *s)
{
    // for a sub (T = A - B) an overflow occurs iff the sign(A) != sign(B) and sign(T) == sign(B)
    if (!((operand1 < 0) == (operand2 < 0)) && ((value < 0) == (operand2 < 0)))
        s->setCC(node, OMR::ConditionCode3);
    else if (value < 0)
        s->setCC(node, OMR::ConditionCode1);
    else if (value > 0)
        s->setCC(node, OMR::ConditionCode2);
    else
        s->setCC(node, OMR::ConditionCode0);
}

template<typename T> inline void setCCSubUnsigned(T value, T operand1, TR::Node *node, TR::Simplifier *s)
{
    bool borrow = value > operand1;

    if (value != 0 && borrow)
        s->setCC(node, OMR::ConditionCode1);
    else if (value == 0 && !borrow)
        s->setCC(node, OMR::ConditionCode2);
    else if (value != 0 && !borrow)
        s->setCC(node, OMR::ConditionCode3);
    else
        TR_ASSERT(0, "condition code of 0 is not possible for logical sub");
}

template<typename T> inline void setCCOr(T value, TR::Node *node, TR::Simplifier *s)
{
    if (value == (T)0)
        s->setCC(node, OMR::ConditionCode0);
    else
        s->setCC(node, OMR::ConditionCode1);
}

inline OMR::TR_ConditionCodeNumber calculateSignedCC(int64_t result, bool overflow)
{
    if ((result == 0) && !overflow)
        return OMR::ConditionCode0;
    else if ((result < 0) && !overflow)
        return OMR::ConditionCode1;
    else if ((result > 0) && !overflow)
        return OMR::ConditionCode2;
    else // (overflow)
        return OMR::ConditionCode3;
}

inline OMR::TR_ConditionCodeNumber calculateUnsignedCC(uint64_t result, bool carryNotBorrow)
{
    if (result == 0 && !carryNotBorrow)
        return OMR::ConditionCode0;
    else if ((result != 0) && !carryNotBorrow)
        return OMR::ConditionCode1;
    else if ((result == 0) && carryNotBorrow)
        return OMR::ConditionCode2;
    else // if ((result != 0) && carryNotBorrow)
        return OMR::ConditionCode3;
}

bool performTransformationSimplifier(TR::Node *node, TR::Simplifier *s);
void setIsHighWordZero(TR::Node *node, TR::Simplifier *s);
TR::Node *_gotoSimplifier(TR::Node *node, TR::Block *block, TR::TreeTop *curTree, TR::Optimization *s);
void foldIntConstant(TR::Node *node, int32_t value, TR::Simplifier *s, bool anchorChildrenP);
void foldUIntConstant(TR::Node *node, uint32_t value, TR::Simplifier *s, bool anchorChildrenP);
void foldLongIntConstant(TR::Node *node, int64_t value, TR::Simplifier *s, bool anchorChildrenP);
void foldFloatConstant(TR::Node *node, float value, TR::Simplifier *s);
void foldDoubleConstant(TR::Node *node, double value, TR::Simplifier *s);
void foldByteConstant(TR::Node *node, int8_t value, TR::Simplifier *s, bool anchorChildrenP);
void foldUByteConstant(TR::Node *node, uint8_t value, TR::Simplifier *s, bool anchorChildrenP);
void foldCharConstant(TR::Node *node, uint16_t value, TR::Simplifier *s, bool anchorChildrenP);
void foldShortIntConstant(TR::Node *node, int16_t value, TR::Simplifier *s, bool anchorChildrenP);
bool swapChildren(TR::Node *node, TR::Node *&firstChild, TR::Node *&secondChild, TR::Simplifier *s);
bool isExprInvariant(TR_RegionStructure *region, TR::Node *node);
void orderChildren(TR::Node *node, TR::Node *&firstChild, TR::Node *&secondChild, TR::Simplifier *s);
TR::Node *foldRedundantAND(TR::Node *node, TR::ILOpCodes andOpCode, TR::ILOpCodes constOpCode, int64_t andVal,
    TR::Simplifier *s);
TR::Node *tryFoldAndWidened(TR::Simplifier *simplifier, TR::Node *node);
bool branchToFollowingBlock(TR::Node *node, TR::Block *block, TR::Compilation *comp);
bool fallthroughGoesToBranchBlock(TR::Node *node, TR::Block *block, TR::Compilation *comp);
void makeConstantTheRightChild(TR::Node *node, TR::Node *&firstChild, TR::Node *&secondChild, TR::Simplifier *s);
void makeConstantTheRightChildAndSetOpcode(TR::Node *node, TR::Node *&firstChild, TR::Node *&secondChild,
    TR::Simplifier *s);
TR::Node *replaceChild(int32_t childIndex, TR::Node *node, TR::Node *newChild, TR::Simplifier *s);
TR::Node *postWalkLowerTreeSimplifier(TR::TreeTop *tt, TR::Node *node, TR::Block *block, TR::Simplifier *s);
void foldFloatConstantEmulate(TR::Node *node, uint32_t value, TR::Simplifier *s);
void foldDoubleConstantEmulate(TR::Node *node, uint64_t value, TR::Simplifier *s);
bool isNaNFloat(TR::Node *node);
bool isNaNDouble(TR::Node *node);
bool isNZFloatPowerOfTwo(float value);
bool isNZDoublePowerOfTwo(double value);
bool decodeConversionOpcode(TR::ILOpCode op, TR::DataType nodeDataType, TR::DataType &sourceDataType,
    TR::DataType &targetDataType);
int32_t floatToInt(float value, bool roundUp);
int32_t doubleToInt(double value, bool roundUp);
void removePaddingNode(TR::Node *node, TR::Simplifier *s);
void stopUsingSingleNode(TR::Node *node, bool removePadding, TR::Simplifier *s);
TR::TreeTop *findTreeTop(TR::Node *callNode, TR::Block *block);
TR::Node *removeIfToFollowingBlock(TR::Node *node, TR::Block *block, TR::Simplifier *s);
void normalizeShiftAmount(TR::Node *node, int32_t normalizationConstant, TR::Simplifier *s);

#endif
