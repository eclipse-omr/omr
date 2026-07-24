/*******************************************************************************
 * Copyright IBM Corp. and others 2026
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
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH
 * Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include "JitTest.hpp"
#include "default_compiler.hpp"

class LookupTest : public TRTest::JitTest {};

TEST_F(LookupTest, Lookup5Cases) {
  // Test function that returns different values based on input
  // Case 10 -> return 100
  // Case 20 -> return 200
  // Case 30 -> return 300
  // Case 40 -> return 400
  // Case 50 -> return 500
  // Default -> return -1

  auto inputTrees = "(method return=Int32 args=[Int32]"
                    "  (block name=\"entry\""
                    "    (lookup"
                    "      (iload parm=0)"
                    "      (case target=\"default\")"
                    "      (case value=10 target=\"case10\")"
                    "      (case value=20 target=\"case20\")"
                    "      (case value=30 target=\"case30\")"
                    "      (case value=40 target=\"case40\")"
                    "      (case value=50 target=\"case50\")))"
                    "  (block name=\"case10\""
                    "    (ireturn (iconst 100)))"
                    "  (block name=\"case20\""
                    "    (ireturn (iconst 200)))"
                    "  (block name=\"case30\""
                    "    (ireturn (iconst 300)))"
                    "  (block name=\"case40\""
                    "    (ireturn (iconst 400)))"
                    "  (block name=\"case50\""
                    "    (ireturn (iconst 500)))"
                    "  (block name=\"default\""
                    "    (ireturn (iconst -1))))";

  auto trees = parseString(inputTrees);
  ASSERT_NOTNULL(trees);

  Tril::DefaultCompiler compiler(trees);
  ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n"
                                   << "Input trees: " << inputTrees;

  auto entry_point = compiler.getEntryPoint<int32_t (*)(int32_t)>();

  // Test each case
  EXPECT_EQ(100, entry_point(10));
  EXPECT_EQ(200, entry_point(20));
  EXPECT_EQ(300, entry_point(30));
  EXPECT_EQ(400, entry_point(40));
  EXPECT_EQ(500, entry_point(50));

  // Test default case
  EXPECT_EQ(-1, entry_point(0));
  EXPECT_EQ(-1, entry_point(15));
  EXPECT_EQ(-1, entry_point(100));
  EXPECT_EQ(-1, entry_point(-5));
}

TEST_F(LookupTest, LookupWithNegativeValues) {
  // Test with negative case values
  auto inputTrees = "(method return=Int32 args=[Int32]"
                    "  (block name=\"entry\""
                    "    (lookup"
                    "      (iload parm=0)"
                    "      (case target=\"default\")"
                    "      (case value=-10 target=\"caseNeg10\")"
                    "      (case value=0 target=\"case0\")"
                    "      (case value=10 target=\"case10\")))"
                    "  (block name=\"caseNeg10\""
                    "    (ireturn (iconst -100)))"
                    "  (block name=\"case0\""
                    "    (ireturn (iconst 0)))"
                    "  (block name=\"case10\""
                    "    (ireturn (iconst 100)))"
                    "  (block name=\"default\""
                    "    (ireturn (iconst -1))))";

  auto trees = parseString(inputTrees);
  ASSERT_NOTNULL(trees);

  Tril::DefaultCompiler compiler(trees);
  ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n"
                                   << "Input trees: " << inputTrees;

  auto entry_point = compiler.getEntryPoint<int32_t (*)(int32_t)>();

  EXPECT_EQ(-100, entry_point(-10));
  EXPECT_EQ(0, entry_point(0));
  EXPECT_EQ(100, entry_point(10));
  EXPECT_EQ(-1, entry_point(5));
  EXPECT_EQ(-1, entry_point(-5));
}

TEST_F(LookupTest, LookupSparseValues) {
  // Test with sparse case values (good for binary search)
  auto inputTrees = "(method return=Int32 args=[Int32]"
                    "  (block name=\"entry\""
                    "    (lookup"
                    "      (iload parm=0)"
                    "      (case target=\"default\")"
                    "      (case value=5 target=\"case5\")"
                    "      (case value=100 target=\"case100\")"
                    "      (case value=500 target=\"case500\")"
                    "      (case value=1000 target=\"case1000\")"
                    "      (case value=5000 target=\"case5000\")))"
                    "  (block name=\"case5\""
                    "    (ireturn (iconst 1)))"
                    "  (block name=\"case100\""
                    "    (ireturn (iconst 2)))"
                    "  (block name=\"case500\""
                    "    (ireturn (iconst 3)))"
                    "  (block name=\"case1000\""
                    "    (ireturn (iconst 4)))"
                    "  (block name=\"case5000\""
                    "    (ireturn (iconst 5)))"
                    "  (block name=\"default\""
                    "    (ireturn (iconst 0))))";

  auto trees = parseString(inputTrees);
  ASSERT_NOTNULL(trees);

  Tril::DefaultCompiler compiler(trees);
  ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n"
                                   << "Input trees: " << inputTrees;

  auto entry_point = compiler.getEntryPoint<int32_t (*)(int32_t)>();

  EXPECT_EQ(1, entry_point(5));
  EXPECT_EQ(2, entry_point(100));
  EXPECT_EQ(3, entry_point(500));
  EXPECT_EQ(4, entry_point(1000));
  EXPECT_EQ(5, entry_point(5000));
  EXPECT_EQ(0, entry_point(50));
  EXPECT_EQ(0, entry_point(2000));
}

TEST_F(LookupTest, LookupSingleCase) {
  // Test with just one case (edge case)
  auto inputTrees = "(method return=Int32 args=[Int32]"
                    "  (block name=\"entry\""
                    "    (lookup"
                    "      (iload parm=0)"
                    "      (case target=\"default\")"
                    "      (case value=42 target=\"case42\")))"
                    "  (block name=\"case42\""
                    "    (ireturn (iconst 1)))"
                    "  (block name=\"default\""
                    "    (ireturn (iconst 0))))";

  auto trees = parseString(inputTrees);
  ASSERT_NOTNULL(trees);

  Tril::DefaultCompiler compiler(trees);
  ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n"
                                   << "Input trees: " << inputTrees;

  auto entry_point = compiler.getEntryPoint<int32_t (*)(int32_t)>();

  EXPECT_EQ(1, entry_point(42));
  EXPECT_EQ(0, entry_point(0));
  EXPECT_EQ(0, entry_point(41));
  EXPECT_EQ(0, entry_point(43));
}

TEST_F(LookupTest, LookupWithComputation) {
  // Test lookup with a computed selector value
  auto inputTrees = "(method return=Int32 args=[Int32]"
                    "  (block name=\"entry\""
                    "    (lookup"
                    "      (imul"
                    "        (iload parm=0)"
                    "        (iconst 10))"
                    "      (case target=\"default\")"
                    "      (case value=10 target=\"case10\")"
                    "      (case value=20 target=\"case20\")"
                    "      (case value=30 target=\"case30\")))"
                    "  (block name=\"case10\""
                    "    (ireturn (iconst 100)))"
                    "  (block name=\"case20\""
                    "    (ireturn (iconst 200)))"
                    "  (block name=\"case30\""
                    "    (ireturn (iconst 300)))"
                    "  (block name=\"default\""
                    "    (ireturn (iconst -1))))";

  auto trees = parseString(inputTrees);
  ASSERT_NOTNULL(trees);

  Tril::DefaultCompiler compiler(trees);
  ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n"
                                   << "Input trees: " << inputTrees;

  auto entry_point = compiler.getEntryPoint<int32_t (*)(int32_t)>();

  // Input 1 * 10 = 10 -> case10
  EXPECT_EQ(100, entry_point(1));
  // Input 2 * 10 = 20 -> case20
  EXPECT_EQ(200, entry_point(2));
  // Input 3 * 10 = 30 -> case30
  EXPECT_EQ(300, entry_point(3));
  // Input 4 * 10 = 40 -> default
  EXPECT_EQ(-1, entry_point(4));
}
// Assisted-by: IBM Bob
