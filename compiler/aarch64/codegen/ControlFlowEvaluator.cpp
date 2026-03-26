/*******************************************************************************
 * ControlFlowEvaluator.cpp
 *
 * Author: [Your Name]
 *
 * (C) Copyright IBM Corp. 2023
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
  *******************************************************************************/
 
#include "codegen/ARM64ConditionCode.hpp"
#include "codegen/ARM64HelperCallSnippet.hpp"
#include "codegen/ARM64Instruction.hpp"
#include "codegen/CodeGenerator.hpp"
 
static TR::Instruction *ificmpHelper(TR::Node *node, TR::ARM64ConditionCode cc, TR::CodeGenerator *cg, TR::Node *firstChild, TR::Node *secondChild)
{
    TR::LabelSymbol *dstLabel;
    TR::Instruction *result;
    TR::RegisterDependencyConditions *deps;
    bool secondChildNeedsRelocation = OMR::relocationNeedsProfiledPointerCheck(cg, secondChild);
    TR_ResolvedMethod *method = cg->comp()->getCurrentMethod();
    bool secondChildNeedsPicSite = (secondChild->getOpCodeValue() == TR::aconst)
        && ((secondChild->isClassPointerConstant()
                && cg->fe()->isUnloadAssumptionRequired(
                    reinterpret_cast<TR_OpaqueClassBlock *>(secondChild->getAddress()), method))
            || (secondChild->isMethodPointerConstant()
                && cg->fe()->isUnloadAssumptionRequired(
                    cg->fe()
                        ->createResolvedMethod(cg->trMemory(),
                            reinterpret_cast<TR_OpaqueMethodBlock *>(secondChild->getAddress()), method)
                        ->classOfMethod(),
                    method)));
    return NULL;
}