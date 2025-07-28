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

#include <stddef.h>
#include <stdint.h>
#include "codegen/CodeGenerator.hpp"
#include "env/FrontEnd.hpp"
#include "codegen/InstOpCode.hpp"
#include "codegen/Machine.hpp"
#include "codegen/MemoryReference.hpp"
#include "codegen/Register.hpp"
#include "codegen/RegisterConstants.hpp"
#include "codegen/RegisterPair.hpp"
#include "codegen/TreeEvaluator.hpp"
#include "compile/Compilation.hpp"
#include "compile/CompilationTypes.hpp"
#include "compile/ResolvedMethod.hpp"
#include "env/CompilerEnv.hpp"
#include "env/Processors.hpp"
#include "env/TRMemory.hpp"
#include "env/jittypes.h"
#include "il/DataTypes.hpp"
#include "il/ILOpCodes.hpp"
#include "il/ILOps.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "infra/Assert.hpp"
#include "p/codegen/GenerateInstructions.hpp"
#include "p/codegen/LoadStoreHandler.hpp"
#include "p/codegen/PPCOpsDefines.hpp"

class TR_OpaqueClassBlock;
class TR_OpaqueMethodBlock;

TR::Register *OMR::Power::TreeEvaluator::iconstEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Register *tempReg = node->setRegister(cg->allocateRegister());
    loadConstant(cg, node, (int32_t)node->get64bitIntegralValue(), tempReg);
    return tempReg;
}

TR::Register *OMR::Power::TreeEvaluator::aconstEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Compilation *comp = cg->comp();
    bool isClass = node->isClassPointerConstant();
    TR_ResolvedMethod *method = comp->getCurrentMethod();

    bool isPicSite = node->isClassPointerConstant()
        && cg->fe()->isUnloadAssumptionRequired((TR_OpaqueClassBlock *)node->getAddress(), method);
    if (!isPicSite)
        isPicSite = node->isMethodPointerConstant()
            && cg->fe()->isUnloadAssumptionRequired(
                cg->fe()
                    ->createResolvedMethod(cg->trMemory(), (TR_OpaqueMethodBlock *)node->getAddress(), method)
                    ->classOfMethod(),
                method);

    bool isProfiledPointerConstant = node->isClassPointerConstant() || node->isMethodPointerConstant();

    // use data snippet only on class pointers when HCR is enabled
    intptr_t address = cg->comp()->target().is64Bit() ? node->getLongInt() : node->getInt();
    if (isProfiledPointerConstant && cg->profiledPointersRequireRelocation()) {
        TR::Register *trgReg = cg->allocateRegister();
        loadAddressConstantInSnippet(cg, node, address, trgReg, NULL, TR::InstOpCode::Op_load, isPicSite, NULL);
        node->setRegister(trgReg);
        return trgReg;
    } else {
        TR::Register *tempReg = node->setRegister(cg->allocateRegister());
        loadActualConstant(cg, node, address, tempReg, NULL, isPicSite);
        return tempReg;
    }
}

TR::Register *OMR::Power::TreeEvaluator::lconstEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit()) {
        TR::Register *tempReg = node->setRegister(cg->allocateRegister());
        loadConstant(cg, node, node->getLongInt(), tempReg);
        return tempReg;
    }
    TR::Register *lowReg = cg->allocateRegister();
    TR::Register *highReg = cg->allocateRegister();
    TR::RegisterPair *trgReg = cg->allocateRegisterPair(lowReg, highReg);
    node->setRegister(trgReg);
    int32_t lowValue = node->getLongIntLow();
    int32_t highValue = node->getLongIntHigh();
    int32_t difference;

    difference = highValue - lowValue;
    loadConstant(cg, node, lowValue, lowReg);
    if ((highValue >= LOWER_IMMED && highValue <= UPPER_IMMED) || difference < LOWER_IMMED
        || difference > UPPER_IMMED) {
        loadConstant(cg, node, highValue, highReg);
    } else {
        generateTrg1Src1ImmInstruction(cg, TR::InstOpCode::addi2, node, highReg, lowReg, difference);
    }

    return trgReg;
}

TR::Register *OMR::Power::TreeEvaluator::inegEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Register *trgReg;
    trgReg = cg->allocateRegister(TR_GPR);
    TR::Node *firstChild = node->getFirstChild();
    TR::Register *sourceRegister = cg->evaluate(firstChild);
    generateTrg1Src1Instruction(cg, TR::InstOpCode::neg, node, trgReg, sourceRegister);

    cg->decReferenceCount(firstChild);
    return node->setRegister(trgReg);
}

TR::Register *OMR::Power::TreeEvaluator::lnegEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit()) {
        return TR::TreeEvaluator::inegEvaluator(node, cg);
    } else {
        TR::Node *firstChild = node->getFirstChild();
        TR::Register *lowReg = cg->allocateRegister();
        TR::Register *highReg = cg->allocateRegister();
        TR::RegisterPair *trgReg = cg->allocateRegisterPair(lowReg, highReg);
        TR::Register *sourceRegister = cg->evaluate(firstChild);
        generateTrg1Src1ImmInstruction(cg, TR::InstOpCode::subfic, node, lowReg, sourceRegister->getLowOrder(), 0);
        generateTrg1Src1Instruction(cg, TR::InstOpCode::subfze, node, highReg, sourceRegister->getHighOrder());

        node->setRegister(trgReg);
        cg->decReferenceCount(firstChild);
        return trgReg;
    }
}

TR::Register *OMR::Power::TreeEvaluator::iabsEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Register *trgReg = cg->allocateRegister();
    TR::Register *tmpReg = cg->allocateRegister();
    TR::Node *firstChild = node->getFirstChild();
    TR::Register *sourceRegister = cg->evaluate(firstChild);

    generateTrg1Src1ImmInstruction(cg, TR::InstOpCode::srawi, node, trgReg, sourceRegister, 31);
    generateTrg1Src2Instruction(cg, TR::InstOpCode::XOR, node, tmpReg, sourceRegister, trgReg);
    generateTrg1Src2Instruction(cg, TR::InstOpCode::subfc, node, trgReg, trgReg, tmpReg);
    cg->stopUsingRegister(tmpReg);

    node->setRegister(trgReg);
    cg->decReferenceCount(firstChild);
    return trgReg;
}

TR::Register *OMR::Power::TreeEvaluator::labsEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit()) {
        TR::Register *trgReg = cg->allocateRegister();
        TR::Register *tmpReg = cg->allocateRegister();
        TR::Node *firstChild = node->getFirstChild();
        TR::Register *sourceRegister = cg->evaluate(firstChild);

        generateTrg1Src1ImmInstruction(cg, TR::InstOpCode::sradi, node, trgReg, sourceRegister, 63);
        generateTrg1Src2Instruction(cg, TR::InstOpCode::XOR, node, tmpReg, sourceRegister, trgReg);
        generateTrg1Src2Instruction(cg, TR::InstOpCode::subfc, node, trgReg, trgReg, tmpReg);
        cg->stopUsingRegister(tmpReg);

        node->setRegister(trgReg);
        cg->decReferenceCount(firstChild);
        return trgReg;
    } else {
        TR::Node *firstChild = node->getFirstChild();
        TR::Register *lowReg = cg->allocateRegister();
        TR::Register *highReg = cg->allocateRegister();
        TR::RegisterPair *trgReg = cg->allocateRegisterPair(lowReg, highReg);
        TR::Register *sourceRegister = cg->evaluate(firstChild);
        TR::Register *tempReg = cg->allocateRegister();

        generateTrg1Src1ImmInstruction(cg, TR::InstOpCode::srawi, node, tempReg, sourceRegister->getHighOrder(), 31);
        generateTrg1Src2Instruction(cg, TR::InstOpCode::XOR, node, highReg, sourceRegister->getHighOrder(), tempReg);
        generateTrg1Src2Instruction(cg, TR::InstOpCode::XOR, node, lowReg, sourceRegister->getLowOrder(), tempReg);
        generateTrg1Src2Instruction(cg, TR::InstOpCode::subfc, node, lowReg, tempReg, lowReg);
        generateTrg1Src2Instruction(cg, TR::InstOpCode::subfe, node, highReg, tempReg, highReg);
        cg->stopUsingRegister(tempReg);

        node->setRegister(trgReg);
        cg->decReferenceCount(firstChild);
        return trgReg;
    }
}

TR::Register *OMR::Power::TreeEvaluator::fabsEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Register *trgReg;
    trgReg = cg->allocateSinglePrecisionRegister();
    TR::Node *firstChild = node->getFirstChild();
    TR::Register *sourceRegister = cg->evaluate(firstChild);
    generateTrg1Src1Instruction(cg, TR::InstOpCode::fabs, node, trgReg, sourceRegister);
    cg->decReferenceCount(firstChild);
    return node->setRegister(trgReg);
}

TR::Register *OMR::Power::TreeEvaluator::dabsEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Register *trgReg;
    trgReg = cg->allocateRegister(TR_FPR);
    TR::Node *firstChild = node->getFirstChild();
    TR::Register *sourceRegister = cg->evaluate(firstChild);
    generateTrg1Src1Instruction(cg, TR::InstOpCode::fabs, node, trgReg, sourceRegister);
    cg->decReferenceCount(firstChild);
    return node->setRegister(trgReg);
}

// also handles b2s, bu2s
TR::Register *OMR::Power::TreeEvaluator::b2iEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Node *child = node->getFirstChild();
    TR::Register *trgReg;
    if (node->isUnneededConversion()
        || (child->getOpCodeValue() != TR::bload && child->getOpCodeValue() != TR::bloadi
            && child->getOpCodeValue() != TR::s2b && child->getOpCodeValue() != TR::f2b
            && child->getOpCodeValue() != TR::d2b && child->getOpCodeValue() != TR::i2b
            && child->getOpCodeValue() != TR::l2b && child->getOpCodeValue() != TR::iRegLoad
            && child->getOpCodeValue() != TR::bRegLoad)) {
        trgReg = cg->gprClobberEvaluate(child);
    } else {
        trgReg = cg->allocateRegister();
        generateTrg1Src1Instruction(cg, TR::InstOpCode::extsb, node, trgReg, cg->evaluate(child));
    }
    node->setRegister(trgReg);
    cg->decReferenceCount(child);
    return trgReg;
}

TR::Register *OMR::Power::TreeEvaluator::b2aEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit())
        return TR::TreeEvaluator::b2lEvaluator(node, cg);
    else
        return TR::TreeEvaluator::b2iEvaluator(node, cg);
}

TR::Register *OMR::Power::TreeEvaluator::s2iEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Node *child = node->getFirstChild();
    TR::Register *trgReg = cg->allocateRegister();

    if (child->getOpCode().isLoadVar() && !child->getRegister() && child->getReferenceCount() == 1) {
        TR::LoadStoreHandler::generateLoadNodeSequence(cg, trgReg, child, TR::InstOpCode::lha, 2);
    } else {
        generateTrg1Src1Instruction(cg, TR::InstOpCode::extsh, node, trgReg, cg->evaluate(child));
    }

    node->setRegister(trgReg);
    cg->decReferenceCount(child);
    return trgReg;
}

TR::Register *OMR::Power::TreeEvaluator::b2lEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Node *child = node->getFirstChild();
    if (cg->comp()->target().is64Bit()) {
        TR::Register *trgReg = cg->allocateRegister();
        generateTrg1Src1Instruction(cg, TR::InstOpCode::extsb, node, trgReg, cg->evaluate(child));
        node->setRegister(trgReg);
        cg->decReferenceCount(child);
        return trgReg;
    } else // 32 bit target
    {
        TR::Register *lowReg = cg->allocateRegister();
        TR::Register *highReg = cg->allocateRegister();
        TR::RegisterPair *trgReg = cg->allocateRegisterPair(lowReg, highReg);
        generateTrg1Src1Instruction(cg, TR::InstOpCode::extsb, node, lowReg, cg->evaluate(child));
        generateTrg1Src1ImmInstruction(cg, TR::InstOpCode::srawi, node, highReg, trgReg->getLowOrder(), 31);
        node->setRegister(trgReg);
        cg->decReferenceCount(child);
        return trgReg;
    }
}

TR::Register *s2l32Evaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Register *trgReg = cg->allocateRegisterPair(cg->allocateRegister(), cg->allocateRegister());
    TR::Register *srcReg = cg->evaluate(node->getFirstChild());

    generateTrg1Src1Instruction(cg, TR::InstOpCode::extsh, node, trgReg->getLowOrder(), srcReg);
    generateTrg1Src1ImmInstruction(cg, TR::InstOpCode::srawi, node, trgReg->getHighOrder(), trgReg->getLowOrder(), 31);

    node->setRegister(trgReg);
    cg->decReferenceCount(node->getFirstChild());
    return trgReg;
}

TR::Register *OMR::Power::TreeEvaluator::s2lEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (!cg->comp()->target().is64Bit())
        return s2l32Evaluator(node, cg);

    TR::Register *trgReg = cg->allocateRegister();
    TR::Register *srcReg = cg->evaluate(node->getFirstChild());

    generateTrg1Src1Instruction(cg, TR::InstOpCode::extsh, node, trgReg, srcReg);

    node->setRegister(trgReg);
    cg->decReferenceCount(node->getFirstChild());
    return trgReg;
}

TR::Register *i2l32Evaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Register *trgReg
        = cg->allocateRegisterPair(cg->gprClobberEvaluate(node->getFirstChild()), cg->allocateRegister());

    generateTrg1Src1ImmInstruction(cg, TR::InstOpCode::srawi, node, trgReg->getHighOrder(), trgReg->getLowOrder(), 31);

    node->setRegister(trgReg);
    cg->decReferenceCount(node->getFirstChild());
    return trgReg;
}

TR::Register *OMR::Power::TreeEvaluator::i2lEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (!cg->comp()->target().is64Bit())
        return i2l32Evaluator(node, cg);

    TR::Register *trgReg = cg->allocateRegister();
    TR::Register *srcReg = cg->evaluate(node->getFirstChild());

    generateTrg1Src1Instruction(cg, TR::InstOpCode::extsw, node, trgReg, srcReg);

    node->setRegister(trgReg);
    cg->decReferenceCount(node->getFirstChild());
    return trgReg;
}

// also handles su2l
TR::Register *OMR::Power::TreeEvaluator::iu2lEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Node *child = node->getFirstChild();
    if (cg->comp()->target().is64Bit()) {
        if (node->getOpCodeValue() == TR::iu2l && child && child->getReferenceCount() == 1 && !child->getRegister()
            && (child->getOpCodeValue() == TR::iloadi || child->getOpCodeValue() == TR::iload)) {
            TR::Register *returnRegister = cg->evaluate(child);
            node->setRegister(returnRegister);
            cg->decReferenceCount(child);
            return returnRegister;
        }

        if (!cg->useClobberEvaluate()
            && // Sharing registers between nodes only works when not using simple clobber evaluation
            node->getOpCodeValue() == TR::iu2l && child && child->getRegister()
            && (child->getOpCodeValue() == TR::iloadi || child->getOpCodeValue() == TR::iload)) {
            node->setRegister(child->getRegister());
            cg->decReferenceCount(child);
            return child->getRegister();
        }

        // TODO: check if first child is memory reference
        TR::Register *trgReg = cg->allocateRegister();
        generateTrg1Src1Imm2Instruction(cg, TR::InstOpCode::rldicl, node, trgReg, cg->evaluate(child), 0,
            CONSTANT64(0x00000000ffffffff));
        node->setRegister(trgReg);
        cg->decReferenceCount(child);
        return trgReg;
    } else // 32 bit target
    {
        TR::Register *sourceRegister = NULL;
        TR::Register *trgReg = cg->allocateRegisterPair(cg->gprClobberEvaluate(child), cg->allocateRegister());
        generateTrg1ImmInstruction(cg, TR::InstOpCode::li, node, trgReg->getHighOrder(), 0);
        node->setRegister(trgReg);
        cg->decReferenceCount(child);
        return trgReg;
    }
}

TR::Register *OMR::Power::TreeEvaluator::su2aEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit())
        return TR::TreeEvaluator::iu2lEvaluator(node, cg);
    else
        return TR::TreeEvaluator::s2iEvaluator(node, cg);
}

TR::Register *OMR::Power::TreeEvaluator::su2iEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Node *child = node->getFirstChild();
    TR::Register *sourceRegister = NULL;
    TR::Register *trgReg = cg->allocateRegister();
    TR::Register *temp;

    temp = child->getRegister();
    if (child->getReferenceCount() == 1 && child->getOpCode().isMemoryReference() && (temp == NULL)) {
        TR::LoadStoreHandler::generateLoadNodeSequence(cg, trgReg, child, TR::InstOpCode::lhz, 2);
    } else {
        generateTrg1Src1Imm2Instruction(cg, TR::InstOpCode::rlwinm, node, trgReg, cg->evaluate(child), 0, 0xffff);
        cg->decReferenceCount(child);
    }
    return node->setRegister(trgReg);
}

TR::Register *OMR::Power::TreeEvaluator::s2aEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit())
        return TR::TreeEvaluator::s2lEvaluator(node, cg);
    else
        return TR::TreeEvaluator::s2iEvaluator(node, cg);
}

// also handles s2b
TR::Register *OMR::Power::TreeEvaluator::i2bEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    return TR::TreeEvaluator::passThroughEvaluator(node, cg);
}

TR::Register *OMR::Power::TreeEvaluator::i2cEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    return TR::TreeEvaluator::passThroughEvaluator(node, cg);
}

TR::Register *OMR::Power::TreeEvaluator::i2sEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    return TR::TreeEvaluator::passThroughEvaluator(node, cg);
}

TR::Register *OMR::Power::TreeEvaluator::i2aEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit())
        return TR::TreeEvaluator::i2lEvaluator(node, cg);
    else
        return TR::TreeEvaluator::passThroughEvaluator(node, cg);
}

TR::Register *OMR::Power::TreeEvaluator::iu2aEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit())
        return TR::TreeEvaluator::iu2lEvaluator(node, cg);
    else
        return TR::TreeEvaluator::passThroughEvaluator(node, cg);
}

TR::Register *passThroughLongLowEvaluator(TR::Node *node, TR::CodeGenerator *cg, TR::InstOpCode::Mnemonic loadOp,
    int64_t srcLen)
{
    if (cg->comp()->target().is64Bit())
        return TR::TreeEvaluator::passThroughEvaluator(node, cg);

    TR::Node *valueNode = node->getFirstChild();
    TR::Register *trgReg;

    if (valueNode->getReferenceCount() == 1 && !valueNode->getRegister() && valueNode->getOpCode().isLoadVar()) {
        int64_t extraOffset = cg->comp()->target().cpu.isBigEndian() ? (8 - srcLen) : 0;

        trgReg = cg->allocateRegister();
        TR::LoadStoreHandler::generateLoadNodeSequence(cg, trgReg, valueNode, loadOp, srcLen, false, extraOffset);
    } else {
        TR::Register *valueReg = cg->evaluate(valueNode);

        if (cg->canClobberNodesRegister(valueNode)) {
            trgReg = valueReg->getLowOrder();
        } else {
            trgReg = cg->allocateRegister();
            generateTrg1Src1Instruction(cg, TR::InstOpCode::mr, node, trgReg, valueReg->getLowOrder());
        }
    }

    node->setRegister(trgReg);
    cg->decReferenceCount(valueNode);
    return trgReg;
}

TR::Register *OMR::Power::TreeEvaluator::l2bEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    return passThroughLongLowEvaluator(node, cg, TR::InstOpCode::lbz, 1);
}

TR::Register *OMR::Power::TreeEvaluator::l2sEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    return passThroughLongLowEvaluator(node, cg, TR::InstOpCode::lhz, 2);
}

TR::Register *OMR::Power::TreeEvaluator::l2iEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    return passThroughLongLowEvaluator(node, cg, TR::InstOpCode::lwz, 4);
}

TR::Register *OMR::Power::TreeEvaluator::l2aEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Compilation *comp = cg->comp();
    if (cg->comp()->target().is64Bit()) {
        // -J9JIT_COMPRESSED_POINTER-
        //
        if (comp->useCompressedPointers()) {
            // pattern match the sequence under the l2a
            //    aloadi f      l2a                       <- node
            //       aload O       ladd
            //                       lshl
            //                          i2l
            //                            iloadi f        <- load
            //                               aload O
            //                          iconst shftKonst
            //                       lconst HB
            //
            // -or- if the load is known to be null
            //  l2a
            //    i2l
            //      iloadi f
            //         aload O
            //
            TR::Node *firstChild = node->getFirstChild();
            bool hasCompPtrs = false;
            if (firstChild->getOpCode().isAdd() && firstChild->containsCompressionSequence())
                hasCompPtrs = true;
            else if (node->isNull())
                hasCompPtrs = true;

            // after remat changes, this assume is no longer valid
            //
            /// TR_ASSERT(hasCompPtrs, "no compression sequence found under l2a node [%p]\n", node);
            TR::Register *source = cg->evaluate(firstChild);

            if ((firstChild->containsCompressionSequence() || (TR::Compiler->om.compressedReferenceShift() == 0))
                && !node->isl2aForCompressedArrayletLeafLoad())
                source->setContainsCollectedReference();

            node->setRegister(source);
            cg->decReferenceCount(firstChild);
            cg->insertPrefetchIfNecessary(node, source);

            return source;
        } else
            return TR::TreeEvaluator::passThroughEvaluator(node, cg);
    } else
        return TR::TreeEvaluator::l2iEvaluator(node, cg);
}

TR::Register *OMR::Power::TreeEvaluator::su2lEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Node *child = node->getFirstChild();
    TR::Register *sourceRegister = NULL;
    if (cg->comp()->target().is64Bit()) {
        TR::Register *trgReg = cg->allocateRegister();
        TR::Register *temp;

        temp = child->getRegister();
        if (child->getReferenceCount() == 1 && child->getOpCode().isMemoryReference() && (temp == NULL)) {
            TR::LoadStoreHandler::generateLoadNodeSequence(cg, trgReg, child, TR::InstOpCode::lhz, 2);
        } else {
            generateTrg1Src1Imm2Instruction(cg, TR::InstOpCode::rldicl, node, trgReg, cg->evaluate(child), 0,
                CONSTANT64(0x000000000000ffff));
            cg->decReferenceCount(child);
        }

        return node->setRegister(trgReg);
    } else // 32 bit target
    {
        bool decSharedNode = false;
        TR::Register *trgReg;
        TR::Register *temp;
        temp = child->getRegister();
        if (child->getReferenceCount() == 1 && child->getOpCode().isMemoryReference() && (temp == NULL)) {
            trgReg = cg->allocateRegisterPair(cg->allocateRegister(), cg->allocateRegister());
            TR::LoadStoreHandler::generateLoadNodeSequence(cg, trgReg->getLowOrder(), child, TR::InstOpCode::lhz, 2);
        } else {
            trgReg = cg->allocateRegisterPair(cg->gprClobberEvaluate(child), cg->allocateRegister());
            generateTrg1Src1Imm2Instruction(cg, TR::InstOpCode::rlwinm, node, trgReg->getLowOrder(),
                cg->evaluate(child), 0, 0xffff);
            decSharedNode = true;
        }
        generateTrg1ImmInstruction(cg, TR::InstOpCode::li, node, trgReg->getHighOrder(), 0);

        node->setRegister(trgReg);

        if (decSharedNode)
            cg->decReferenceCount(child);

        return trgReg;
    }
}

TR::Register *OMR::Power::TreeEvaluator::bu2iEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Node *child = node->getFirstChild();
    TR::Register *trgReg;

    if (child->getReferenceCount() == 1 && child->getOpCode().isMemoryReference() && (child->getRegister() == NULL)) {
        trgReg = cg->gprClobberEvaluate(child);
    } else {
        trgReg = cg->allocateRegister();
        generateTrg1Src1Imm2Instruction(cg, TR::InstOpCode::rlwinm, node, trgReg, cg->evaluate(child), 0, 0xff);
    }

    node->setRegister(trgReg);
    cg->decReferenceCount(child);
    return trgReg;
}

TR::Register *OMR::Power::TreeEvaluator::bu2lEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Node *child = node->getFirstChild();
    TR::Register *trgReg;

    if (cg->comp()->target().is64Bit()) {
        if (child->getOpCode().isMemoryReference()) {
            trgReg = cg->gprClobberEvaluate(child);
        } else {
            trgReg = cg->allocateRegister();
            generateTrg1Src1Imm2Instruction(cg, TR::InstOpCode::rldicl, node, trgReg, cg->evaluate(child), 0,
                CONSTANT64(0x00000000000000ff));
        }
    } else // 32 bit target
    {
        TR::Register *lowReg;
        if (child->getOpCode().isMemoryReference()) {
            lowReg = cg->gprClobberEvaluate(child);
        } else {
            lowReg = cg->allocateRegister();
            generateTrg1Src1Imm2Instruction(cg, TR::InstOpCode::rlwinm, node, lowReg, cg->evaluate(child), 0, 0xff);
        }

        TR::Register *highReg = cg->allocateRegister();
        generateTrg1ImmInstruction(cg, TR::InstOpCode::li, node, highReg, 0);
        trgReg = cg->allocateRegisterPair(lowReg, highReg);
    }

    node->setRegister(trgReg);
    cg->decReferenceCount(child);
    return trgReg;
}

TR::Register *OMR::Power::TreeEvaluator::bu2aEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit())
        return TR::TreeEvaluator::bu2lEvaluator(node, cg);
    else
        return TR::TreeEvaluator::bu2iEvaluator(node, cg);
}

TR::Register *OMR::Power::TreeEvaluator::a2iEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit())
        return TR::TreeEvaluator::l2iEvaluator(node, cg);
    else
        return TR::TreeEvaluator::passThroughEvaluator(node, cg);
}

TR::Register *OMR::Power::TreeEvaluator::a2lEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit())
        return TR::TreeEvaluator::passThroughEvaluator(node, cg);
    else
        return TR::TreeEvaluator::iu2lEvaluator(node, cg);
}

TR::Register *OMR::Power::TreeEvaluator::a2bEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit())
        return TR::TreeEvaluator::l2bEvaluator(node, cg);
    else
        return TR::TreeEvaluator::i2bEvaluator(node, cg);
}

TR::Register *OMR::Power::TreeEvaluator::a2sEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    if (cg->comp()->target().is64Bit())
        return TR::TreeEvaluator::l2sEvaluator(node, cg);
    else
        return TR::TreeEvaluator::i2sEvaluator(node, cg);
}

TR::Register *OMR::Power::TreeEvaluator::strcmpEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Node::recreate(node, TR::icall);
    TR::Register *trgReg = TR::TreeEvaluator::directCallEvaluator(node, cg);
    return trgReg;
}

TR::Register *OMR::Power::TreeEvaluator::strcFuncEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Node::recreate(node, TR::acall);
    TR::Register *trgReg = TR::TreeEvaluator::directCallEvaluator(node, cg);
    return trgReg;
}

TR::Register *OMR::Power::TreeEvaluator::dfloorEvaluator(TR::Node *node, TR::CodeGenerator *cg)
{
    TR::Node *firstChild = node->getFirstChild();
    TR::Register *srcReg = cg->evaluate(firstChild);

    TR::Register *trgReg;
    TR::Register *tmp2Reg;
    TR::Register *tmp3Reg;
    TR::Register *tmp4Reg;

    TR::Register *tmp1Reg, *tmp5Reg;
    TR::Node *const_node, *const_node_max;
    if ((node->getOpCodeValue() == TR::dfloor) || (node->getOpCodeValue() == TR::dceil)) {
        trgReg = cg->allocateRegister(TR_FPR);
        tmp2Reg = cg->allocateRegister(TR_FPR);
        tmp3Reg = cg->allocateRegister(TR_FPR);
        tmp4Reg = cg->allocateRegister(TR_FPR);

        const_node = TR::Node::create(node, TR::dconst, 0);
        if (node->getOpCodeValue() == TR::dceil) {
            const_node->setDouble(1.0);
        } else
            const_node->setDouble(-1.0);
        tmp1Reg = TR::TreeEvaluator::dconstEvaluator(const_node, cg);

        const_node_max = TR::Node::create(node, TR::dconst, 0);
        const_node_max->setDouble(CONSTANT64(0x10000000000000)); // 2**52
        tmp5Reg = TR::TreeEvaluator::dconstEvaluator(const_node_max, cg);
    } else {
        trgReg = cg->allocateSinglePrecisionRegister();
        tmp2Reg = cg->allocateSinglePrecisionRegister();
        tmp3Reg = cg->allocateSinglePrecisionRegister();
        tmp4Reg = cg->allocateSinglePrecisionRegister();

        const_node = TR::Node::create(node, TR::fconst, 0);
        if (node->getOpCodeValue() == TR::fceil) {
            const_node->setFloat(1.0);
        } else
            const_node->setFloat(-1.0);
        tmp1Reg = TR::TreeEvaluator::fconstEvaluator(const_node, cg);

        const_node_max = TR::Node::create(node, TR::fconst, 0);
        const_node_max->setFloat(0x800000);
        tmp5Reg = TR::TreeEvaluator::fconstEvaluator(const_node_max, cg);
    }

    const_node->unsetRegister();
    const_node_max->unsetRegister();

    generateTrg1Src1Instruction(cg, TR::InstOpCode::fctidz, node, trgReg, srcReg);
    generateTrg1Src1Instruction(cg, TR::InstOpCode::fcfid, node, trgReg, trgReg); // trgReg = trunc(x)
    generateTrg1Src2Instruction(cg, TR::InstOpCode::fadd, node, tmp2Reg, trgReg,
        tmp1Reg); // tmp2  = trunc(x) -1  for floor
                  //       = trunc(x) +1  for ceil
    if ((node->getOpCodeValue() == TR::dfloor) || (node->getOpCodeValue() == TR::ffloor))
        generateTrg1Src2Instruction(cg, TR::InstOpCode::fsub, node, tmp3Reg, srcReg,
            trgReg); // tmp3  = (x-trunc(x)) for floor
    else
        generateTrg1Src2Instruction(cg, TR::InstOpCode::fsub, node, tmp3Reg, trgReg,
            srcReg); // tmp3  = (trunc(x)-x) for ceil

    generateTrg1Src3Instruction(cg, TR::InstOpCode::fsel, node, trgReg, tmp3Reg, trgReg, tmp2Reg);

    // if x is a NaN or out of range  (i.e. fabs(x)>2**52), then floor(x) = x, otherwise the value computed above

    generateTrg1Src1Instruction(cg, TR::InstOpCode::fabs, node, tmp4Reg, srcReg); // tmp4  = fabs(x)

    generateTrg1Src2Instruction(cg, TR::InstOpCode::fsub, node, tmp5Reg, tmp5Reg, tmp4Reg); // tmp5  = 2**52 - fabs(x)

    generateTrg1Src3Instruction(cg, TR::InstOpCode::fsel, node, trgReg, tmp5Reg, trgReg, srcReg);

    cg->stopUsingRegister(tmp1Reg);
    cg->stopUsingRegister(tmp2Reg);
    cg->stopUsingRegister(tmp3Reg);

    cg->stopUsingRegister(tmp4Reg);
    cg->stopUsingRegister(tmp5Reg);

    node->setRegister(trgReg);
    cg->decReferenceCount(firstChild);

    return trgReg;
}
