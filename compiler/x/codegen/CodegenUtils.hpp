/*******************************************************************************
 * Copyright IBM Corp. and others 2025
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

#ifndef OMR_CODEGENUTILS_INCL
#define OMR_CODEGENUTILS_INCL

#include "codegen/Machine.hpp"
#include "codegen/CodeGenerator.hpp"
#include "x/codegen/OMRX86Instruction.hpp"
#include "x/codegen/RegisterRematerialization.hpp"

namespace TR { class Node; }

#define SETUP_CUSTOM_REGISTER(type, cg, deps, varName)                  \
        varName = cg->allocateRegister(type);                           \
        deps->addPostCondition(varName, TR::RealRegister::NoReg, cg);

#define SETUP_CUSTOM_REGISTERS_4(type, cg, deps, varA) SETUP_CUSTOM_REGISTER(type, cg, deps, varA)
#define SETUP_CUSTOM_REGISTERS_5(type, cg, deps, varA, varB) SETUP_CUSTOM_REGISTER(type, cg, deps, varA)       \
                                                             SETUP_CUSTOM_REGISTER(type, cg, deps, varB)
#define SETUP_CUSTOM_REGISTERS_6(type, cg, deps, varA, varB, varC) SETUP_CUSTOM_REGISTERS_5(type, cg, deps, varA, varB) \
                                                             SETUP_CUSTOM_REGISTER(type, cg, deps, varC)
#define SETUP_CUSTOM_REGISTERS_7(type, cg, deps, varA, varB, varC, varD) SETUP_CUSTOM_REGISTERS_6(type, cg, deps, varA, varB, varC) \
                                                             SETUP_CUSTOM_REGISTER(type, cg, deps, varD)
#define SETUP_CUSTOM_REGISTERS_8(type, cg, deps, varA, varB, varC, varD, varE) SETUP_CUSTOM_REGISTERS_7(type, cg, deps, varA, varB, varC, varD) \
                                                             SETUP_CUSTOM_REGISTER(type, cg, deps, varE)

#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, NAME,...) NAME
#define SETUP_CUSTOM_REGISTERS(...) GET_MACRO(__VA_ARGS__, SETUP_CUSTOM_REGISTERS_8, SETUP_CUSTOM_REGISTERS_7, SETUP_CUSTOM_REGISTERS_6, SETUP_CUSTOM_REGISTERS_5, SETUP_CUSTOM_REGISTERS_4)(__VA_ARGS__)


#define STOP_USING_REGISTERS(cg, ...)                                        \
    {                                                                        \
    TR::Register* registers[] = { __VA_ARGS__ };                             \
    for (int i = 0; i < sizeof(registers) / sizeof(TR::Register *); ++i) {   \
        cg->stopUsingRegister(registers[i]);                                 \
    }                                                                        \
    }

#define SETUP_GPR_REGISTERS(cg, deps, ...) SETUP_CUSTOM_REGISTERS(TR_GPR, cg, deps, __VA_ARGS__)
#define SETUP_VRF_REGISTERS(cg, deps, ...) SETUP_CUSTOM_REGISTERS(TR_VRF, cg, deps, __VA_ARGS__)
#define SETUP_FPR_REGISTERS(cg, deps, ...) SETUP_CUSTOM_REGISTERS(TR_FPR, cg, deps, __VA_ARGS__)

namespace OMR
{

namespace X86
{

/**
 * @brief Inserts an instruction to load a constant value into a specified register.
 *
 * This function generates an instruction to load a constant value into the target register.
 * The generated instruction can be appended to a specified instruction chain.
 *
 * @param node The IL node that generated the load constant
 * @param target The register into which the constant value should be loaded
 * @param value The constant value to be loaded into the target register
 * @param type The rematerializable type of the constant
 * @param cg The code generator responsible for generating the instructions
 * @param currentInstruction The instruction to which the new instruction will be appended (optional)
 *
 * @return A pointer to the generated instruction.
 */
TR::Instruction *insertLoadConstant(TR::Node *node,
                                    TR::Register *target,
                                    intptr_t value,
                                    TR_RematerializableTypes type,
                                    TR::CodeGenerator *cg,
                                    TR::Instruction *currentInstruction = NULL);

/**
 * @brief Loads a constant value into a register and returns the register.
 *
 * This function either loads the specified constant value into the provided target register,
 * or allocates a new register to hold the constant if no target is provided.
 *
 * @param node The IL node associated with this operation
 * @param value The constant value to load into the registe.
 * @param type The rematerializable type of the constant
 * @param cg The code generator managing the instruction generation process
 * @param targetRegister The target register to load the value into (optional)
 *
 * @return A pointer to the register containing the loaded constant value.
 */
TR::Register *loadConstant(TR::Node *node,
                           intptr_t value,
                           TR_RematerializableTypes type,
                           TR::CodeGenerator *cg,
                           TR::Register *targetRegister = NULL);

/**
 * @brief Generates loop control flow with start and end bounds
 *
 * This function generates assembly code for a loop that iterates from a specified
 * starting value to an ending value. The loop body is implemented by a user-provided function.
 *
 * @param begin The starting value of the loop index
 * @param end The ending value of the loop index
 * @param node The node associated with the loop
 * @param cg The code generator used to emit instructions
 * @param genBodyFunction A callback function that generates the body of the loop
 */
void generateLoop(int32_t begin,
                  int32_t end,
                  TR::Node *node,
                  TR::CodeGenerator *cg,
                  std::function<void(int32_t)> genBodyFunction);

/**
 * @brief Generates a loop with parameterized unrolling and optional residue element processing.
 *
 * This function generates an optimized loop with a specified unrolling factor and the ability to process
 * multiple elements per iteration. If the total number of iterations is not divisible by the number of
 * elements processed in each unrolled iteration, a secondary residue loop can be generated to handle
 * the leftover elements.
 *
 * Constraints:
 * - `unrollFactor` must be a power of 2.
 * - `elementsPerIteration` must be a power of 2.
 * - `residueGenBodyFunction` If specified, must only process one iteration of loop index.
 *
 * @param unrollFactor Number of iterations to unroll in the main loop.
 * @param elementsPerIteration Number of elements processed per loop body iteration.
 * @param indexReg The register used to store the loop index.
 * @param maxIndexReg The register containing the loop bound.
 * @param node IL node associated with the loop generation.
 * @param cg Pointer to the code generator responsible for emitting the loop code.
 * @param loopInitializerFunction Function to generate initialization code prior to the first iteration, if applicable.
 * @param genBodyFunction Function to generate the body of the loop for each unrolled iteration.
 * @param residueGenBodyFunction Function to generate the body for the residue loop, if applicable.
 */
void generateLoop(int32_t unrollFactor,
                  int32_t elementsPerIteration,
                  TR::Register *indexReg,
                  TR::Register *maxIndexReg,
                  TR::Node *node,
                  TR::CodeGenerator *cg,
                  std::function<void()> loopInitializerFunction,
                  std::function<void(int32_t)> genBodyFunction,
                  std::function<void(int32_t)> residueGenBodyFunction = NULL);

/**
 * @brief Generates a loop with parameterized unrolling and optional residue element processing.
 *
 * This function generates an optimized loop with a specified unrolling factor and the ability to process
 * multiple elements per iteration. If the total number of iterations is not divisible by the number of
 * elements processed in each unrolled iteration, a secondary residue loop can be generated to handle
 * the leftover elements.
 *
 * Constraints:
 * - `unrollFactor` must be a power of 2.
 * - `elementsPerIteration` must be a power of 2.
 * - `residueGenBodyFunction` If specified, must only process one iteration of loop index.
 *
 * @param unrollFactor Number of iterations to unroll in the main loop.
 * @param elementsPerIteration Number of elements processed per loop body iteration.
 * @param indexReg The register used to store the loop index.
 * @param maxIndexReg The register containing the loop bound.
 * @param node IL node associated with the loop generation.
 * @param cg Pointer to the code generator responsible for emitting the loop code.
 * @param genBodyFunction Function to generate the body of the loop for each unrolled iteration.
 * @param residueGenBodyFunction Function to generate the body for the residue loop, if applicable.
 */
inline void generateLoop(int32_t unrollFactor,
                         int32_t elementsPerIteration,
                         TR::Register *indexReg,
                         TR::Register *maxIndexReg,
                         TR::Node *node,
                         TR::CodeGenerator *cg,
                         std::function<void(int32_t)> genBodyFunction,
                         std::function<void(int32_t)> residueGenBodyFunction = NULL)
   {
   generateLoop(unrollFactor, elementsPerIteration, indexReg, maxIndexReg, node, cg, NULL, genBodyFunction, residueGenBodyFunction);
   }

/**
 * @brief Generates a loop with parameterized unrolling and residue processing.
 *
 * This function generates an unrolled loop where each unrolled iteration
 * processes only one element. A second loop is automatically generated to
 * process leftover (residue) elements, ensuring all elements are handled.
 *
 * Constraints:
 * - `unrollFactor` must be a power of 2.
 * - `genBodyFunction` must only process one iteration of loop index.
 *
 * @param unrollFactor Number of iterations to unroll in the main loop.
 * @param indexReg The register used to store the loop index.
 * @param maxIndexReg The register containing the loop bound.
 * @param node IL node associated with the loop generation.
 * @param cg Pointer to the code generator responsible for emitting the loop code.
 * @param genBodyFunction Function to generate the body of the loop for each unrolled iteration.
 */
inline void generateUnrolledLoopWithResidue(int32_t unrollFactor,
                                            TR::Register *indexReg,
                                            TR::Register *maxIndexReg,
                                            TR::Node *node,
                                            TR::CodeGenerator *cg,
                                            std::function<void(int32_t)> genBodyFunction)
   {
   generateLoop(unrollFactor, 1, indexReg, maxIndexReg, node, cg, NULL, genBodyFunction, genBodyFunction);
   }

/**
 * @brief Generates a simple loop without unrolling or residue processing.
 *
 * This function generates a loop where each iteration processes a single element.
 * It is suitable for scenarios where the loop body function processes exactly one
 * loop index per iteration. No additional residue handling is required.
 *
 * Constraints:
 * - `genBodyFunction` must only process one iteration of loop index.
 *
 * @param indexReg The register used to store the loop index.
 * @param loopBoundReg The register containing the loop bound.
 * @param node IL node associated with the loop generation.
 * @param cg Pointer to the code generator responsible for emitting the loop code.
 * @param genBodyFunction Function to generate the body of the loop for each iteration.
 */
inline void generateLoop(TR::Register *indexReg,
                         TR::Register *maxIndexReg,
                         TR::Node *node,
                         TR::CodeGenerator *cg,
                         std::function<void(int32_t)> genBodyFunction)
   {
   generateLoop(1, 1, indexReg, maxIndexReg, node, cg, NULL, genBodyFunction, NULL);
   }


}

}

#endif
