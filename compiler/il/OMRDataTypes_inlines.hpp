/*******************************************************************************
 * Copyright IBM Corp. and others 2017
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

#ifndef OMR_DATATYPES_INLINES_INCL
#define OMR_DATATYPES_INLINES_INCL

#include "il/OMRDataTypes.hpp"
#include "infra/Assert.hpp"

TR::DataType *OMR::DataType::self() { return static_cast<TR::DataType *>(this); }

const TR::DataType *OMR::DataType::self() const { return static_cast<const TR::DataType *>(this); }

OMR::DataType::operator int() { return self()->getDataType(); }

bool OMR::DataType::isInt8() { return self()->getDataType() == TR::Int8; }

bool OMR::DataType::isInt16() { return self()->getDataType() == TR::Int16; }

bool OMR::DataType::isInt32() { return self()->getDataType() == TR::Int32; }

bool OMR::DataType::isInt64() { return self()->getDataType() == TR::Int64; }

bool OMR::DataType::isIntegral()
{
    return self()->isInt8() || self()->isInt16() || self()->isInt32() || self()->isInt64();
}

bool OMR::DataType::isFloatingPoint() { return self()->isBFPorHFP(); }

bool OMR::DataType::isVector() { return _type >= TR::FirstVectorType && _type <= TR::LastVectorType; }

bool OMR::DataType::isVectorElement() { return (_type > TR::NoType) && (_type <= TR::NumVectorElementTypes); }

bool OMR::DataType::isBFPorHFP() { return self()->getDataType() == TR::Float || self()->getDataType() == TR::Double; }

bool OMR::DataType::isDouble() { return self()->getDataType() == TR::Double; }

bool OMR::DataType::isFloat() { return self()->getDataType() == TR::Float; }

bool OMR::DataType::isAddress() { return self()->getDataType() == TR::Address; }

bool OMR::DataType::isAggregate() { return self()->getDataType() == TR::Aggregate; }

TR::DataType &OMR::DataType::operator=(const TR::DataType &rhs)
{
    _type = rhs._type;
    return *(static_cast<TR::DataType *>(this));
}

TR::DataType &OMR::DataType::operator=(TR::DataTypes rhs)
{
    _type = rhs;
    return *(static_cast<TR::DataType *>(this));
}

bool OMR::DataType::operator==(const TR::DataType &rhs) { return _type == rhs._type; }

bool OMR::DataType::operator==(TR::DataTypes rhs) { return _type == rhs; }

bool OMR::DataType::operator!=(const TR::DataType &rhs) { return _type != rhs._type; }

bool OMR::DataType::operator!=(TR::DataTypes rhs) { return _type != rhs; }

bool OMR::DataType::operator<=(const TR::DataType &rhs) { return _type <= rhs._type; }

bool OMR::DataType::operator<=(TR::DataTypes rhs) { return _type <= rhs; }

bool OMR::DataType::operator<(const TR::DataType &rhs) { return _type < rhs._type; }

bool OMR::DataType::operator<(TR::DataTypes rhs) { return _type < rhs; }

bool OMR::DataType::operator>=(const TR::DataType &rhs) { return _type >= rhs._type; }

bool OMR::DataType::operator>=(TR::DataTypes rhs) { return _type >= rhs; }

bool OMR::DataType::operator>(const TR::DataType &rhs) { return _type > rhs._type; }

bool OMR::DataType::operator>(TR::DataTypes rhs) { return _type > rhs; }

TR::DataTypes OMR::DataType::createVectorType(TR::DataType elementType, TR::VectorLength length)
{
    TR::DataTypes et = elementType.getDataType();

    TR_ASSERT_FATAL(et > TR::NoType && et <= TR::NumVectorElementTypes, "Invalid vector element type %d\n", et);
    TR_ASSERT_FATAL(length > TR::NoVectorLength && length <= TR::NumVectorLengths, "Invalid vector length %d\n",
        length);

    TR::DataTypes type
        = static_cast<TR::DataTypes>(TR::FirstVectorType + (length - 1) * TR::NumVectorElementTypes + et - 1);

    return type;
}

TR::DataType OMR::DataType::getVectorElementType()
{
    TR_ASSERT_FATAL(isVectorOrMask(), "getVectorElementType() is called for non-vector or non-mask type\n");

    if (isVector())
        return static_cast<TR::DataTypes>((_type - TR::FirstVectorType) % TR::NumVectorElementTypes + 1);
    else
        return static_cast<TR::DataTypes>((_type - TR::FirstMaskType) % TR::NumVectorElementTypes + 1);
}

TR::VectorLength OMR::DataType::getVectorLength()
{
    TR_ASSERT_FATAL(isVectorOrMask(), "getVectorLength() is called for non-vector or non-mask type\n");

    if (isVector())
        return static_cast<TR::VectorLength>((_type - TR::FirstVectorType) / TR::NumVectorElementTypes + 1);
    else
        return static_cast<TR::VectorLength>((_type - TR::FirstMaskType) / TR::NumVectorElementTypes + 1);
}

TR::VectorLength OMR::DataType::bitsToVectorLength(int32_t bits)
{
    TR::VectorLength length = TR::NoVectorLength;
    switch (bits) {
        case 64:
            length = TR::VectorLength64;
            break;
        case 128:
            length = TR::VectorLength128;
            break;
        case 256:
            length = TR::VectorLength256;
            break;
        case 512:
            length = TR::VectorLength512;
            break;
    }

    return length;
}

bool OMR::DataType::isMask() { return _type >= TR::FirstMaskType && _type <= TR::LastMaskType; }

TR::DataTypes OMR::DataType::createMaskType(TR::DataType elementType, TR::VectorLength length)
{
    TR::DataTypes et = elementType.getDataType();

    if (et == TR::Float) {
        et = TR::Int32;
    } else if (et == TR::Double) {
        et = TR::Int64;
    }

    TR_ASSERT_FATAL(et > TR::NoType && et <= TR::NumVectorElementTypes, "Invalid vector element type %d\n", et);
    TR_ASSERT_FATAL(length > TR::NoVectorLength && length <= TR::NumVectorLengths, "Invalid vector length %d\n",
        length);

    TR::DataTypes type
        = static_cast<TR::DataTypes>(TR::FirstMaskType + (length - 1) * TR::NumVectorElementTypes + et - 1);

    return type;
}

TR::DataTypes OMR::DataType::vectorFromMaskType(TR::DataType maskType)
{
    TR_ASSERT_FATAL(maskType.isMask(), "vectorFromMaskType should take mask type\n");

    return static_cast<TR::DataTypes>(maskType - TR::NumVectorTypes);
}

#endif // OMR_DATATYPES_INLINES_INCL
