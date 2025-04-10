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


#include "OpCodeTest.hpp"
#include "default_compiler.hpp"
#include "omrformatconsts.h"

template <typename uint_t>
uint_t bitOR(uint_t l, uint_t r) {
    return l | r;
}

template <typename uint_t>
uint_t bitAND(uint_t l, uint_t r) {
    return l & r;
}

template <typename uint_t>
uint_t bitXOR(uint_t l, uint_t r) {
    return l ^ r;
}

int16_t sbyteswap(int16_t l) {
    return ((l << 8) & 0xff00)
         | ((l >> 8) & 0x00ff);
}

int32_t ineg(int32_t l) {
    return -l;
}

int32_t iabs(int32_t l) {
   if (l >= 0)
      return l;
   else
      return -1 * l;
}

int32_t ibyteswap(int32_t l) {
    return ((l << 24) & 0xff000000)
         | ((l <<  8) & 0x00ff0000)
         | ((l >>  8) & 0x0000ff00)
         | ((l >> 24) & 0x000000ff);
}

int64_t lbyteswap(int64_t l) {
    return ((l << 56) & 0xff00000000000000)
         | ((l << 40) & 0x00ff000000000000)
         | ((l << 24) & 0x0000ff0000000000)
         | ((l <<  8) & 0x000000ff00000000)
         | ((l >>  8) & 0x00000000ff000000)
         | ((l >> 24) & 0x0000000000ff0000)
         | ((l >> 40) & 0x000000000000ff00)
         | ((l >> 56) & 0x00000000000000ff);
}

int64_t lneg(int64_t l) {
    return l == INT64_MIN ? INT64_MIN : -l;
}

class Int16LogicalUnary : public TRTest::UnaryOpTest<int16_t> {};

TEST_P(Int16LogicalUnary, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    if (param.opcode == "sbyteswap") {
        SKIP_ON_ARM(MissingImplementation);
        SKIP_ON_RISCV(MissingImplementation);
    }

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120,
        "(method return=Int16"
        "  (block"
        "    (ireturn"
        "      (s2i"
        "        (%s"
        "          (sconst %" OMR_PRId16 ") )"
        "))))",
        param.opcode.c_str(),
        param.value);

    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<int16_t (*)()>();
    volatile auto exp = param.oracle(param.value);
    volatile auto act = entry_point();
    ASSERT_EQ(exp, act);
}

TEST_P(Int16LogicalUnary, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    if (param.opcode == "sbyteswap") {
        SKIP_ON_ARM(MissingImplementation);
        SKIP_ON_RISCV(MissingImplementation);
    }

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120,
        "(method return=Int16 args=[Int16]"
        "  (block"
        "    (ireturn"
        "      (s2i"
        "         (%s"
        "           (sload parm=0) )"
        "))))",
        param.opcode.c_str());

    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<int16_t (*)(int16_t)>();
    ASSERT_EQ(param.oracle(param.value), entry_point(param.value));
}

INSTANTIATE_TEST_CASE_P(LogicalTest, Int16LogicalUnary, ::testing::Combine(
    ::testing::ValuesIn(TRTest::const_values<int16_t>()),
    ::testing::Values(
        std::tuple<const char*, int16_t(*)(int16_t)>("sbyteswap", sbyteswap)
        )));

class Int32LogicalUnary : public TRTest::UnaryOpTest<int32_t> {};

TEST_P(Int32LogicalUnary, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    if (param.opcode == "ibyteswap") {
        SKIP_ON_ARM(MissingImplementation);
        SKIP_ON_RISCV(MissingImplementation);
    }

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120, "(method return=Int32 (block (ireturn (%s (iconst %d) ) )))", param.opcode.c_str(), param.value);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<int32_t (*)()>();
    volatile auto exp = param.oracle(param.value);
    volatile auto act = entry_point();
    ASSERT_EQ(exp, act);
}

TEST_P(Int32LogicalUnary, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    if (param.opcode == "ibyteswap") {
        SKIP_ON_ARM(MissingImplementation);
        SKIP_ON_RISCV(MissingImplementation);
    }

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120, "(method return=Int32 args=[Int32] (block (ireturn (%s (iload parm=0) ) )))", param.opcode.c_str());

    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<int32_t (*)(int32_t)>();
    ASSERT_EQ(param.oracle(param.value), entry_point(param.value));
}

INSTANTIATE_TEST_CASE_P(LogicalTest, Int32LogicalUnary, ::testing::Combine(
    ::testing::ValuesIn(TRTest::const_values<int32_t>()),
    ::testing::Values(
        std::tuple<const char*, int32_t(*)(int32_t)>("ibyteswap", ibyteswap),
        std::tuple<const char*, int32_t(*)(int32_t)>("ineg", ineg),
        std::tuple<const char*, int32_t(*)(int32_t)>("iabs", iabs)
    )));

class Int64LogicalUnary : public TRTest::UnaryOpTest<int64_t> {};

TEST_P(Int64LogicalUnary, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    if (param.opcode == "lbyteswap") {
        SKIP_ON_ARM(MissingImplementation);
        SKIP_ON_RISCV(MissingImplementation);
    }

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120,
        "(method return=Int64"
        "  (block"
        "    (lreturn"
        "      (%s"
        "        (lconst %" OMR_PRId64 ") )"
        ")))",
        param.opcode.c_str(),
        param.value);

    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<int64_t (*)()>();
    volatile auto exp = param.oracle(param.value);
    volatile auto act = entry_point();
    ASSERT_EQ(exp, act);
}

TEST_P(Int64LogicalUnary, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    if (param.opcode == "lbyteswap") {
        SKIP_ON_ARM(MissingImplementation);
        SKIP_ON_RISCV(MissingImplementation);
    }

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120,
        "(method return=Int64 args=[Int64]"
        "  (block"
        "    (lreturn"
        "      (%s"
        "        (lload parm=0) )"
        ")))",
        param.opcode.c_str());

    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<int64_t (*)(int64_t)>();
    ASSERT_EQ(param.oracle(param.value), entry_point(param.value));
}

INSTANTIATE_TEST_CASE_P(LogicalTest, Int64LogicalUnary, ::testing::Combine(
    ::testing::ValuesIn(TRTest::const_values<int64_t>()),
    ::testing::Values(
        std::tuple<const char*, int64_t(*)(int64_t)>("lbyteswap", lbyteswap),
        std::tuple<const char*, int64_t(*)(int64_t)>("lneg", lneg)
        )));


class Int8LogicalBinary : public TRTest::BinaryOpTest<uint8_t> {};

TEST_P(Int8LogicalBinary, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120, "(method return=Int8 (block (ireturn (%s (bconst %d) (bconst %d)))))", param.opcode.c_str(), param.lhs, param.rhs);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<uint8_t (*)(void)>();
    volatile auto exp = param.oracle(param.lhs, param.rhs);
    volatile auto act = entry_point();
    ASSERT_EQ(exp, act);
}

TEST_P(Int8LogicalBinary, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120, "(method return=Int8 args=[Int8, Int8] (block (ireturn (%s (bload parm=0) (bload parm=1)))))", param.opcode.c_str());

    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<uint8_t (*)(uint8_t, uint8_t)>();
    ASSERT_EQ(param.oracle(param.lhs, param.rhs), entry_point(param.lhs, param.rhs));
}

INSTANTIATE_TEST_CASE_P(LogicalTest, Int8LogicalBinary, ::testing::Combine(
    ::testing::ValuesIn(TRTest::const_value_pairs<uint8_t,uint8_t>()),
    ::testing::Values(
        std::tuple<const char*, uint8_t(*)(uint8_t, uint8_t)>("bor", bitOR),
        std::tuple<const char*, uint8_t(*)(uint8_t, uint8_t)>("band", bitAND),
        std::tuple<const char*, uint8_t(*)(uint8_t, uint8_t)>("bxor", bitXOR)
    )));

class Int16LogicalBinary : public TRTest::BinaryOpTest<uint16_t> {};

TEST_P(Int16LogicalBinary, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120, "(method return=Int16 (block (ireturn (%s (sconst %d) (sconst %d)))))", param.opcode.c_str(), param.lhs, param.rhs);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<uint16_t (*)(void)>();
    volatile auto exp = param.oracle(param.lhs, param.rhs);
    volatile auto act = entry_point();
    ASSERT_EQ(exp, act);
}

TEST_P(Int16LogicalBinary, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120, "(method return=Int16 args=[Int16, Int16] (block (ireturn (%s (sload parm=0) (sload parm=1)))))", param.opcode.c_str());

    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<uint16_t (*)(uint16_t, uint16_t)>();
    ASSERT_EQ(param.oracle(param.lhs, param.rhs), entry_point(param.lhs, param.rhs));
}

INSTANTIATE_TEST_CASE_P(LogicalTest, Int16LogicalBinary, ::testing::Combine(
    ::testing::ValuesIn(TRTest::const_value_pairs<uint16_t,uint16_t>()),
    ::testing::Values(
        std::tuple<const char*, uint16_t(*)(uint16_t, uint16_t)>("sor", bitOR),
        std::tuple<const char*, uint16_t(*)(uint16_t, uint16_t)>("sand", bitAND),
        std::tuple<const char*, uint16_t(*)(uint16_t, uint16_t)>("sxor", bitXOR)
    )));

class Int32LogicalBinary : public TRTest::BinaryOpTest<uint32_t> {};

TEST_P(Int32LogicalBinary, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120, "(method return=Int32 (block (ireturn (%s (iconst %d) (iconst %d)) )))", param.opcode.c_str(), param.lhs, param.rhs);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<uint32_t (*)(void)>();
    volatile auto exp = param.oracle(param.lhs, param.rhs);
    volatile auto act = entry_point();
    ASSERT_EQ(exp, act);
}

TEST_P(Int32LogicalBinary, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[120] = {0};
    std::snprintf(inputTrees, 120, "(method return=Int32 args=[Int32, Int32] (block (ireturn (%s (iload parm=0) (iload parm=1)) )))", param.opcode.c_str());

    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<uint32_t (*)(uint32_t, uint32_t)>();
    ASSERT_EQ(param.oracle(param.lhs, param.rhs), entry_point(param.lhs, param.rhs));
}

INSTANTIATE_TEST_CASE_P(LogicalTest, Int32LogicalBinary, ::testing::Combine(
    ::testing::ValuesIn(TRTest::const_value_pairs<uint32_t,uint32_t>()),
    ::testing::Values(
        std::tuple<const char*, uint32_t(*)(uint32_t, uint32_t)>("ior", bitOR),
        std::tuple<const char*, uint32_t(*)(uint32_t, uint32_t)>("iand", bitAND),
        std::tuple<const char*, uint32_t(*)(uint32_t, uint32_t)>("ixor", bitXOR)
    )));

class Int64LogicalBinary : public TRTest::BinaryOpTest<uint64_t> {};

TEST_P(Int64LogicalBinary, UsingConst) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[160] = {0};
    std::snprintf(inputTrees, 160,
        "(method return=Int64"
        "  (block"
        "    (lreturn"
        "      (%s"
        "        (lconst %" OMR_PRId64 ")"
        "        (lconst %" OMR_PRId64 ") ) ) ) )",
        param.opcode.c_str(), param.lhs, param.rhs);
    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<uint64_t (*)(void)>();
    volatile auto exp = param.oracle(param.lhs, param.rhs);
    volatile auto act = entry_point();
    ASSERT_EQ(exp, act);
}

TEST_P(Int64LogicalBinary, UsingLoadParam) {
    auto param = TRTest::to_struct(GetParam());

    char inputTrees[160] = {0};
    std::snprintf(inputTrees, 160,
        "(method return=Int64 args=[Int64, Int64]"
        "  (block"
        "    (lreturn"
        "      (%s"
        "        (lload parm=0)"
        "        (lload parm=1) ) ) ) )",
        param.opcode.c_str());

    auto trees = parseString(inputTrees);

    ASSERT_NOTNULL(trees);

    Tril::DefaultCompiler compiler(trees);

    ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    auto entry_point = compiler.getEntryPoint<uint64_t (*)(uint64_t, uint64_t)>();
    ASSERT_EQ(param.oracle(param.lhs, param.rhs), entry_point(param.lhs, param.rhs));
}

INSTANTIATE_TEST_CASE_P(LogicalTest, Int64LogicalBinary, ::testing::Combine(
	::testing::ValuesIn(TRTest::const_value_pairs<uint64_t,uint64_t>()),
	::testing::Values(
		std::tuple<const char*, uint64_t(*)(uint64_t, uint64_t)>("lor", bitOR),
		std::tuple<const char*, uint64_t(*)(uint64_t, uint64_t)>("land", bitAND),
		std::tuple<const char*, uint64_t(*)(uint64_t, uint64_t)>("lxor", bitXOR)
   )));
