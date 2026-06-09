/*******************************************************************************
 * Copyright IBM Corp. and others 2024
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

#include "JitTest.hpp"
#include "default_compiler.hpp"
#include "compilerunittest/CompilerUnitTest.hpp"
#include "VectorTestUtils.hpp"

class VnotTest : public TRTest::JitTest {};

/**
 * @brief Test vnot with Int8 elements - all zeros
 */
TEST_F(VnotTest, VnotInt8AllZeros) {
    TR::VectorLength vl = TR::VectorLength128;
    TR::DataTypes elementType = TR::Int8;
    TR::DataType vt = TR::DataType::createVectorType(elementType, vl);
    
    TR::ILOpCode vnotOp = TR::ILOpCode::createVectorOpCode(TR::vnot, vt);
    TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
    
    SKIP_IF(!TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, vnotOp), MissingImplementation);
    
    char inputTrees[1024];
    std::snprintf(inputTrees, sizeof(inputTrees),
                  "(method return=NoType args=[Address,Address]"
                  "  (block"
                  "    (vstoreiVector128Int8 offset=0 (aload parm=0)"
                  "      (vnotVector128Int8 (vloadiVector128Int8 (aload parm=1))))"
                  "    (return)))");
    
    auto trees = parseString(inputTrees);
    ASSERT_NOTNULL(trees);
    
    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile());
    
    uint8_t input[16] = {0};
    uint8_t expected[16];
    uint8_t result[16] = {0};
    
    for (int i = 0; i < 16; i++) expected[i] = 0xFF;
    
    auto entry_point = compiler.getEntryPoint<void(*)(void*, void*)>();
    entry_point(result, input);
    
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

/**
 * @brief Test vnot with Int8 elements - all ones
 */
TEST_F(VnotTest, VnotInt8AllOnes) {
    TR::VectorLength vl = TR::VectorLength128;
    TR::DataTypes elementType = TR::Int8;
    TR::DataType vt = TR::DataType::createVectorType(elementType, vl);
    
    TR::ILOpCode vnotOp = TR::ILOpCode::createVectorOpCode(TR::vnot, vt);
    TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
    
    SKIP_IF(!TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, vnotOp), MissingImplementation);
    
    char inputTrees[1024];
    std::snprintf(inputTrees, sizeof(inputTrees),
                  "(method return=NoType args=[Address,Address]"
                  "  (block"
                  "    (vstoreiVector128Int8 offset=0 (aload parm=0)"
                  "      (vnotVector128Int8 (vloadiVector128Int8 (aload parm=1))))"
                  "    (return)))");
    
    auto trees = parseString(inputTrees);
    ASSERT_NOTNULL(trees);
    
    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile());
    
    uint8_t input[16];
    uint8_t expected[16] = {0};
    uint8_t result[16] = {0};
    
    for (int i = 0; i < 16; i++) input[i] = 0xFF;
    
    auto entry_point = compiler.getEntryPoint<void(*)(void*, void*)>();
    entry_point(result, input);
    
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

/**
 * @brief Test vnot with Int8 elements - alternating pattern
 */
TEST_F(VnotTest, VnotInt8Alternating) {
    TR::VectorLength vl = TR::VectorLength128;
    TR::DataTypes elementType = TR::Int8;
    TR::DataType vt = TR::DataType::createVectorType(elementType, vl);
    
    TR::ILOpCode vnotOp = TR::ILOpCode::createVectorOpCode(TR::vnot, vt);
    TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
    
    SKIP_IF(!TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, vnotOp), MissingImplementation);
    
    char inputTrees[1024];
    std::snprintf(inputTrees, sizeof(inputTrees),
                  "(method return=NoType args=[Address,Address]"
                  "  (block"
                  "    (vstoreiVector128Int8 offset=0 (aload parm=0)"
                  "      (vnotVector128Int8 (vloadiVector128Int8 (aload parm=1))))"
                  "    (return)))");
    
    auto trees = parseString(inputTrees);
    ASSERT_NOTNULL(trees);
    
    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile());
    
    uint8_t input[16] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
                         0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
    uint8_t expected[16] = {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
                            0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA};
    uint8_t result[16] = {0};
    
    auto entry_point = compiler.getEntryPoint<void(*)(void*, void*)>();
    entry_point(result, input);
    
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

/**
 * @brief Test vnot with Int16 elements - all zeros
 */
TEST_F(VnotTest, VnotInt16AllZeros) {
    TR::VectorLength vl = TR::VectorLength128;
    TR::DataTypes elementType = TR::Int16;
    TR::DataType vt = TR::DataType::createVectorType(elementType, vl);
    
    TR::ILOpCode vnotOp = TR::ILOpCode::createVectorOpCode(TR::vnot, vt);
    TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
    
    SKIP_IF(!TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, vnotOp), MissingImplementation);
    
    char inputTrees[1024];
    std::snprintf(inputTrees, sizeof(inputTrees),
                  "(method return=NoType args=[Address,Address]"
                  "  (block"
                  "    (vstoreiVector128Int16 offset=0 (aload parm=0)"
                  "      (vnotVector128Int16 (vloadiVector128Int16 (aload parm=1))))"
                  "    (return)))");
    
    auto trees = parseString(inputTrees);
    ASSERT_NOTNULL(trees);
    
    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile());
    
    uint16_t input[8] = {0};
    uint16_t expected[8] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    uint16_t result[8] = {0};
    
    auto entry_point = compiler.getEntryPoint<void(*)(void*, void*)>();
    entry_point(result, input);
    
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

/**
 * @brief Test vnot with Int16 elements - alternating pattern
 */
TEST_F(VnotTest, VnotInt16Alternating) {
    TR::VectorLength vl = TR::VectorLength128;
    TR::DataTypes elementType = TR::Int16;
    TR::DataType vt = TR::DataType::createVectorType(elementType, vl);
    
    TR::ILOpCode vnotOp = TR::ILOpCode::createVectorOpCode(TR::vnot, vt);
    TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
    
    SKIP_IF(!TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, vnotOp), MissingImplementation);
    
    char inputTrees[1024];
    std::snprintf(inputTrees, sizeof(inputTrees),
                  "(method return=NoType args=[Address,Address]"
                  "  (block"
                  "    (vstoreiVector128Int16 offset=0 (aload parm=0)"
                  "      (vnotVector128Int16 (vloadiVector128Int16 (aload parm=1))))"
                  "    (return)))");
    
    auto trees = parseString(inputTrees);
    ASSERT_NOTNULL(trees);
    
    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile());
    
    uint16_t input[8] = {0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555};
    uint16_t expected[8] = {0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA};
    uint16_t result[8] = {0};
    
    auto entry_point = compiler.getEntryPoint<void(*)(void*, void*)>();
    entry_point(result, input);
    
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

/**
 * @brief Test vnot with Int32 elements - all zeros
 */
TEST_F(VnotTest, VnotInt32AllZeros) {
    TR::VectorLength vl = TR::VectorLength128;
    TR::DataTypes elementType = TR::Int32;
    TR::DataType vt = TR::DataType::createVectorType(elementType, vl);
    
    TR::ILOpCode vnotOp = TR::ILOpCode::createVectorOpCode(TR::vnot, vt);
    TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
    
    SKIP_IF(!TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, vnotOp), MissingImplementation);
    
    char inputTrees[1024];
    std::snprintf(inputTrees, sizeof(inputTrees),
                  "(method return=NoType args=[Address,Address]"
                  "  (block"
                  "    (vstoreiVector128Int32 offset=0 (aload parm=0)"
                  "      (vnotVector128Int32 (vloadiVector128Int32 (aload parm=1))))"
                  "    (return)))");
    
    auto trees = parseString(inputTrees);
    ASSERT_NOTNULL(trees);
    
    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile());
    
    uint32_t input[4] = {0};
    uint32_t expected[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    uint32_t result[4] = {0};
    
    auto entry_point = compiler.getEntryPoint<void(*)(void*, void*)>();
    entry_point(result, input);
    
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

/**
 * @brief Test vnot with Int32 elements - all ones
 */
TEST_F(VnotTest, VnotInt32AllOnes) {
    TR::VectorLength vl = TR::VectorLength128;
    TR::DataTypes elementType = TR::Int32;
    TR::DataType vt = TR::DataType::createVectorType(elementType, vl);
    
    TR::ILOpCode vnotOp = TR::ILOpCode::createVectorOpCode(TR::vnot, vt);
    TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
    
    SKIP_IF(!TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, vnotOp), MissingImplementation);
    
    char inputTrees[1024];
    std::snprintf(inputTrees, sizeof(inputTrees),
                  "(method return=NoType args=[Address,Address]"
                  "  (block"
                  "    (vstoreiVector128Int32 offset=0 (aload parm=0)"
                  "      (vnotVector128Int32 (vloadiVector128Int32 (aload parm=1))))"
                  "    (return)))");
    
    auto trees = parseString(inputTrees);
    ASSERT_NOTNULL(trees);
    
    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile());
    
    uint32_t input[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    uint32_t expected[4] = {0};
    uint32_t result[4] = {0};
    
    auto entry_point = compiler.getEntryPoint<void(*)(void*, void*)>();
    entry_point(result, input);
    
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

/**
 * @brief Test vnot with Int32 elements - alternating pattern
 */
TEST_F(VnotTest, VnotInt32Alternating) {
    TR::VectorLength vl = TR::VectorLength128;
    TR::DataTypes elementType = TR::Int32;
    TR::DataType vt = TR::DataType::createVectorType(elementType, vl);
    
    TR::ILOpCode vnotOp = TR::ILOpCode::createVectorOpCode(TR::vnot, vt);
    TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
    
    SKIP_IF(!TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, vnotOp), MissingImplementation);
    
    char inputTrees[1024];
    std::snprintf(inputTrees, sizeof(inputTrees),
                  "(method return=NoType args=[Address,Address]"
                  "  (block"
                  "    (vstoreiVector128Int32 offset=0 (aload parm=0)"
                  "      (vnotVector128Int32 (vloadiVector128Int32 (aload parm=1))))"
                  "    (return)))");
    
    auto trees = parseString(inputTrees);
    ASSERT_NOTNULL(trees);
    
    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile());
    
    uint32_t input[4] = {0xAAAAAAAA, 0x55555555, 0xAAAAAAAA, 0x55555555};
    uint32_t expected[4] = {0x55555555, 0xAAAAAAAA, 0x55555555, 0xAAAAAAAA};
    uint32_t result[4] = {0};
    
    auto entry_point = compiler.getEntryPoint<void(*)(void*, void*)>();
    entry_point(result, input);
    
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

/**
 * @brief Test vnot with Int32 elements - mixed pattern
 */
TEST_F(VnotTest, VnotInt32Mixed) {
    TR::VectorLength vl = TR::VectorLength128;
    TR::DataTypes elementType = TR::Int32;
    TR::DataType vt = TR::DataType::createVectorType(elementType, vl);
    
    TR::ILOpCode vnotOp = TR::ILOpCode::createVectorOpCode(TR::vnot, vt);
    TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
    
    SKIP_IF(!TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, vnotOp), MissingImplementation);
    
    char inputTrees[1024];
    std::snprintf(inputTrees, sizeof(inputTrees),
                  "(method return=NoType args=[Address,Address]"
                  "  (block"
                  "    (vstoreiVector128Int32 offset=0 (aload parm=0)"
                  "      (vnotVector128Int32 (vloadiVector128Int32 (aload parm=1))))"
                  "    (return)))");
    
    auto trees = parseString(inputTrees);
    ASSERT_NOTNULL(trees);
    
    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile());
    
    uint32_t input[4] = {0x12345678, 0x9ABCDEF0, 0xFEDCBA98, 0x76543210};
    uint32_t expected[4] = {0xEDCBA987, 0x6543210F, 0x01234567, 0x89ABCDEF};
    uint32_t result[4] = {0};
    
    auto entry_point = compiler.getEntryPoint<void(*)(void*, void*)>();
    entry_point(result, input);
    
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

/**
 * @brief Test vnot with Int64 elements - all zeros
 */
TEST_F(VnotTest, VnotInt64AllZeros) {
    TR::VectorLength vl = TR::VectorLength128;
    TR::DataTypes elementType = TR::Int64;
    TR::DataType vt = TR::DataType::createVectorType(elementType, vl);
    
    TR::ILOpCode vnotOp = TR::ILOpCode::createVectorOpCode(TR::vnot, vt);
    TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
    
    SKIP_IF(!TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, vnotOp), MissingImplementation);
    
    char inputTrees[1024];
    std::snprintf(inputTrees, sizeof(inputTrees),
                  "(method return=NoType args=[Address,Address]"
                  "  (block"
                  "    (vstoreiVector128Int64 offset=0 (aload parm=0)"
                  "      (vnotVector128Int64 (vloadiVector128Int64 (aload parm=1))))"
                  "    (return)))");
    
    auto trees = parseString(inputTrees);
    ASSERT_NOTNULL(trees);
    
    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile());
    
    uint64_t input[2] = {0};
    uint64_t expected[2] = {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF};
    uint64_t result[2] = {0};
    
    auto entry_point = compiler.getEntryPoint<void(*)(void*, void*)>();
    entry_point(result, input);
    
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

/**
 * @brief Test vnot with Int64 elements - all ones
 */
TEST_F(VnotTest, VnotInt64AllOnes) {
    TR::VectorLength vl = TR::VectorLength128;
    TR::DataTypes elementType = TR::Int64;
    TR::DataType vt = TR::DataType::createVectorType(elementType, vl);
    
    TR::ILOpCode vnotOp = TR::ILOpCode::createVectorOpCode(TR::vnot, vt);
    TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
    
    SKIP_IF(!TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, vnotOp), MissingImplementation);
    
    char inputTrees[1024];
    std::snprintf(inputTrees, sizeof(inputTrees),
                  "(method return=NoType args=[Address,Address]"
                  "  (block"
                  "    (vstoreiVector128Int64 offset=0 (aload parm=0)"
                  "      (vnotVector128Int64 (vloadiVector128Int64 (aload parm=1))))"
                  "    (return)))");
    
    auto trees = parseString(inputTrees);
    ASSERT_NOTNULL(trees);
    
    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile());
    
    uint64_t input[2] = {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF};
    uint64_t expected[2] = {0};
    uint64_t result[2] = {0};
    
    auto entry_point = compiler.getEntryPoint<void(*)(void*, void*)>();
    entry_point(result, input);
    
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

/**
 * @brief Test vnot with Int64 elements - alternating pattern
 */
TEST_F(VnotTest, VnotInt64Alternating) {
    TR::VectorLength vl = TR::VectorLength128;
    TR::DataTypes elementType = TR::Int64;
    TR::DataType vt = TR::DataType::createVectorType(elementType, vl);
    
    TR::ILOpCode vnotOp = TR::ILOpCode::createVectorOpCode(TR::vnot, vt);
    TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
    
    SKIP_IF(!TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, vnotOp), MissingImplementation);
    
    char inputTrees[1024];
    std::snprintf(inputTrees, sizeof(inputTrees),
                  "(method return=NoType args=[Address,Address]"
                  "  (block"
                  "    (vstoreiVector128Int64 offset=0 (aload parm=0)"
                  "      (vnotVector128Int64 (vloadiVector128Int64 (aload parm=1))))"
                  "    (return)))");
    
    auto trees = parseString(inputTrees);
    ASSERT_NOTNULL(trees);
    
    Tril::DefaultCompiler compiler(trees);
    ASSERT_EQ(0, compiler.compile());
    
    uint64_t input[2] = {0xAAAAAAAAAAAAAAAA, 0x5555555555555555};
    uint64_t expected[2] = {0x5555555555555555, 0xAAAAAAAAAAAAAAAA};
    uint64_t result[2] = {0};
    
    auto entry_point = compiler.getEntryPoint<void(*)(void*, void*)>();
    entry_point(result, input);
    
    for (int i = 0; i < 2; i++) {
        EXPECT_EQ(expected[i], result[i]);
    }
}

// Made with Bob
