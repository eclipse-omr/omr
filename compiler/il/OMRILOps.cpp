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

#include "il/ILOps.hpp"

#include <stdint.h>
#include <stddef.h>
#include "env/CompilerEnv.hpp"
#include "il/DataTypes.hpp"
#include "il/ILOpCodes.hpp"
#include "il/ILOps.hpp"
#include "il/ILProps.hpp"
#include "infra/Assert.hpp"
#include "infra/Flags.hpp"

/**
 * Table of opcode properties.
 *
 * \note A note on the syntax of the table below. The commented field names,
 * or rather, the designated initializer syntax would actually work in
 * C99, however we can't use that feature because microsoft compiler
 * still doesn't support C99. If microsoft ever supports this we can
 * make the table even more robust and type safe by uncommenting the
 * field initializers.
 */
OMR::OpCodeProperties OMR::ILOpCode::_opCodeProperties[] = {
#include "il/ILOpCodeProperties.hpp"
};

void OMR::ILOpCode::checkILOpArrayLengths()
{
    for (int i = TR::FirstOMROp; i < TR::NumScalarIlOps; i++) {
        TR::ILOpCodes opCode = (TR::ILOpCodes)i;
        TR::ILOpCode op(opCode);
        OMR::OpCodeProperties &props = TR::ILOpCode::_opCodeProperties[opCode];

        TR_ASSERT(props.opcode == opCode, "_opCodeProperties table out of sync at index %d, has %s\n", i, op.getName());
    }
}

// FIXME: We should put the smarts in the getSize() routine in TR::DataType
// instead of modifying this large table that should in fact be read-only
//
void OMR::ILOpCode::setTarget()
{
    if (TR::Compiler->target.is64Bit()) {
        for (int32_t i = 0; i < opCodePropertiesSize; ++i) {
            flags32_t *tp = (flags32_t *)(&_opCodeProperties[i].typeProperties); // so ugly
            if (tp->getValue() == ILTypeProp::Reference) {
                tp->reset(ILTypeProp::Size_Mask);
                tp->set(ILTypeProp::Size_8);
            }
        }
        TR::DataType::setSize(TR::Address, 8);
    } else {
        for (int32_t i = 0; i < opCodePropertiesSize; ++i) {
            flags32_t *tp = (flags32_t *)(&_opCodeProperties[i].typeProperties); // so ugly
            if (tp->getValue() == ILTypeProp::Reference) {
                tp->reset(ILTypeProp::Size_Mask);
                tp->set(ILTypeProp::Size_4);
            }
        }
        TR::DataType::setSize(TR::Address, 4);
    }
}

TR::ILOpCodes OMR::ILOpCode::compareOpCode(TR::DataType dt, enum TR_ComparisonTypes ct, bool unsignedCompare)
{
    if (unsignedCompare) {
        switch (dt) {
            case TR::Int8: {
                switch (ct) {
                    case TR_cmpLT:
                        return TR::bucmplt;
                    case TR_cmpLE:
                        return TR::bucmple;
                    case TR_cmpGT:
                        return TR::bucmpgt;
                    case TR_cmpGE:
                        return TR::bucmpge;
                    default:
                        return TR::BadILOp;
                }
                break;
            }
            case TR::Int16: {
                switch (ct) {
                    case TR_cmpLT:
                        return TR::sucmplt;
                    case TR_cmpLE:
                        return TR::sucmple;
                    case TR_cmpGT:
                        return TR::sucmpgt;
                    case TR_cmpGE:
                        return TR::sucmpge;
                    default:
                        return TR::BadILOp;
                }
                break;
            }
            case TR::Int32: {
                switch (ct) {
                    case TR_cmpLT:
                        return TR::iucmplt;
                    case TR_cmpLE:
                        return TR::iucmple;
                    case TR_cmpGT:
                        return TR::iucmpgt;
                    case TR_cmpGE:
                        return TR::iucmpge;
                    default:
                        return TR::BadILOp;
                }
                break;
            }
            case TR::Int64: {
                switch (ct) {
                    case TR_cmpLT:
                        return TR::lucmplt;
                    case TR_cmpLE:
                        return TR::lucmple;
                    case TR_cmpGT:
                        return TR::lucmpgt;
                    case TR_cmpGE:
                        return TR::lucmpge;
                    default:
                        return TR::BadILOp;
                }
                break;
            }
            case TR::Address: {
                switch (ct) {
                    case TR_cmpEQ:
                        return TR::acmpeq;
                    case TR_cmpNE:
                        return TR::acmpne;
                    case TR_cmpLT:
                        return TR::acmplt;
                    case TR_cmpLE:
                        return TR::acmple;
                    case TR_cmpGT:
                        return TR::acmpgt;
                    case TR_cmpGE:
                        return TR::acmpge;
                    default:
                        return TR::BadILOp;
                }
                break;
            }
            default:
                return TR::BadILOp;
        }
    } else {
        switch (dt) {
            case TR::Int8: {
                switch (ct) {
                    case TR_cmpEQ:
                        return TR::bcmpeq;
                    case TR_cmpNE:
                        return TR::bcmpne;
                    case TR_cmpLT:
                        return TR::bcmplt;
                    case TR_cmpLE:
                        return TR::bcmple;
                    case TR_cmpGT:
                        return TR::bcmpgt;
                    case TR_cmpGE:
                        return TR::bcmpge;
                    default:
                        return TR::BadILOp;
                }
                break;
            }
            case TR::Int16: {
                switch (ct) {
                    case TR_cmpEQ:
                        return TR::scmpeq;
                    case TR_cmpNE:
                        return TR::scmpne;
                    case TR_cmpLT:
                        return TR::scmplt;
                    case TR_cmpLE:
                        return TR::scmple;
                    case TR_cmpGT:
                        return TR::scmpgt;
                    case TR_cmpGE:
                        return TR::scmpge;
                    default:
                        return TR::BadILOp;
                }
                break;
            }
            case TR::Int32: {
                switch (ct) {
                    case TR_cmpEQ:
                        return TR::icmpeq;
                    case TR_cmpNE:
                        return TR::icmpne;
                    case TR_cmpLT:
                        return TR::icmplt;
                    case TR_cmpLE:
                        return TR::icmple;
                    case TR_cmpGT:
                        return TR::icmpgt;
                    case TR_cmpGE:
                        return TR::icmpge;
                    default:
                        return TR::BadILOp;
                }
                break;
            }
            case TR::Int64: {
                switch (ct) {
                    case TR_cmpEQ:
                        return TR::lcmpeq;
                    case TR_cmpNE:
                        return TR::lcmpne;
                    case TR_cmpLT:
                        return TR::lcmplt;
                    case TR_cmpLE:
                        return TR::lcmple;
                    case TR_cmpGT:
                        return TR::lcmpgt;
                    case TR_cmpGE:
                        return TR::lcmpge;
                    default:
                        return TR::BadILOp;
                }
                break;
            }
            case TR::Float: {
                switch (ct) {
                    case TR_cmpEQ:
                        return TR::fcmpeq;
                    case TR_cmpNE:
                        return TR::fcmpneu;
                    case TR_cmpLT:
                        return TR::fcmplt;
                    case TR_cmpLE:
                        return TR::fcmple;
                    case TR_cmpGT:
                        return TR::fcmpgt;
                    case TR_cmpGE:
                        return TR::fcmpge;
                    default:
                        return TR::BadILOp;
                }
                break;
            }
            case TR::Double: {
                switch (ct) {
                    case TR_cmpEQ:
                        return TR::dcmpeq;
                    case TR_cmpNE:
                        return TR::dcmpneu;
                    case TR_cmpLT:
                        return TR::dcmplt;
                    case TR_cmpLE:
                        return TR::dcmple;
                    case TR_cmpGT:
                        return TR::dcmpgt;
                    case TR_cmpGE:
                        return TR::dcmpge;
                    default:
                        return TR::BadILOp;
                }
                break;
            }
            case TR::Address: {
                switch (ct) {
                    case TR_cmpEQ:
                        return TR::acmpeq;
                    case TR_cmpNE:
                        return TR::acmpne;
                    case TR_cmpLT:
                        return TR::acmplt;
                    case TR_cmpLE:
                        return TR::acmple;
                    case TR_cmpGT:
                        return TR::acmpgt;
                    case TR_cmpGE:
                        return TR::acmpge;
                    default:
                        return TR::BadILOp;
                }
                break;
            }
            default:
                return TR::BadILOp;
        }
    }
    return TR::BadILOp;
}

/**
 * \brief
 *    Return a comparison type given the compare opcode.
 *
 * \parm op
 *    The compare opcode.
 *
 * \return
 *    The compareison type.
 */
TR_ComparisonTypes OMR::ILOpCode::getCompareType(TR::ILOpCodes op)
{
    if (isStrictlyLessThanCmp(op))
        return TR_cmpLT;
    else if (isStrictlyGreaterThanCmp(op))
        return TR_cmpGT;
    else if (isLessCmp(op))
        return TR_cmpLE;
    else if (isGreaterCmp(op))
        return TR_cmpGE;
    else if (isEqualCmp(op))
        return TR_cmpEQ;
    else
        return TR_cmpNE;
}

namespace OMR {

#define TR_Bad TR::BadILOp

static TR::ILOpCodes conversionMap[TR::NumOMRTypes][TR::NumOMRTypes] =
    //                       No      Int8     Int16    Int32    Int64    Float    Double   Addr     Aggregate
    {
        /* NoType */ { TR_Bad,  TR_Bad,  TR_Bad,  TR_Bad,  TR_Bad,  TR_Bad,  TR_Bad,  TR_Bad, TR_Bad }, // NoType
        /* Int8 */
        { TR_Bad,  TR_Bad, TR::b2s, TR::b2i, TR::b2l, TR::b2f, TR::b2d, TR::b2a, TR_Bad }, // Int8
        /* Int16 */
        { TR_Bad, TR::s2b,  TR_Bad, TR::s2i, TR::s2l, TR::s2f, TR::s2d, TR::s2a, TR_Bad }, // Int16
        /* Int32 */
        { TR_Bad, TR::i2b, TR::i2s,  TR_Bad, TR::i2l, TR::i2f, TR::i2d, TR::i2a, TR_Bad }, // Int32
        /* Int64 */
        { TR_Bad, TR::l2b, TR::l2s, TR::l2i,  TR_Bad, TR::l2f, TR::l2d, TR::l2a, TR_Bad }, // Int64
        /* Float */
        { TR_Bad, TR::f2b, TR::f2s, TR::f2i, TR::f2l,  TR_Bad, TR::f2d,  TR_Bad, TR_Bad }, // Float
        /* Double */
        { TR_Bad, TR::d2b, TR::d2s, TR::d2i, TR::d2l, TR::d2f,  TR_Bad,  TR_Bad, TR_Bad }, // Double
        /* Address */
        { TR_Bad, TR::a2b, TR::a2s, TR::a2i, TR::a2l,  TR_Bad,  TR_Bad,  TR_Bad, TR_Bad }, // Address
        /* Aggregate */
        { TR_Bad,  TR_Bad,  TR_Bad,  TR_Bad,  TR_Bad,  TR_Bad,  TR_Bad,  TR_Bad, TR_Bad }, // Aggregate
};

#undef TR_Bad

} // namespace OMR

TR::ILOpCodes OMR::ILOpCode::getDataTypeConversion(TR::DataType t1, TR::DataType t2)
{
    if (t1.isVector() && t2.isVector())
        return TR::ILOpCode::createVectorOpCode(TR::vconv, t1, t2);

    if (t1.isVector() || t2.isVector())
        return TR::BadILOp;

    if (t1.isMask() || t2.isMask())
        return TR::BadILOp;

    TR_ASSERT(t1 < TR::NumOMRTypes, "conversion opcode from unexpected datatype %s requested", t1.toString());
    TR_ASSERT(t2 < TR::NumOMRTypes, "conversion opcode to unexpected datatype %s requested", t2.toString());
    return OMR::conversionMap[t1][t2];
}

TR::ILOpCodes OMR::ILOpCode::getDataTypeBitConversion(TR::DataType t1, TR::DataType t2)
{
    if (t1.isVector() || t2.isVector())
        return TR::BadILOp;

    if (t1.isMask() || t2.isMask())
        return TR::BadILOp;

    TR_ASSERT(t1 < TR::NumOMRTypes, "conversion opcode from unexpected datatype %s requested", t1.toString());
    TR_ASSERT(t2 < TR::NumOMRTypes, "conversion opcode to unexpected datatype %s requested", t2.toString());
    if (t1 == TR::Int32 && t2 == TR::Float)
        return TR::ibits2f;
    else if (t1 == TR::Float && t2 == TR::Int32)
        return TR::fbits2i;
    else if (t1 == TR::Int64 && t2 == TR::Double)
        return TR::lbits2d;
    else if (t1 == TR::Double && t2 == TR::Int64)
        return TR::dbits2l;
    else
        return TR::BadILOp;
}
