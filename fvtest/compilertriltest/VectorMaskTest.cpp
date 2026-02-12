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

#include "JitTest.hpp"
#include "VectorTestUtils.hpp"
#include "default_compiler.hpp"
#include "compilerunittest/CompilerUnitTest.hpp"

#define MAX_NUM_LANES 64

class VectorTest : public TRTest::JitTest {};

class ParameterizedMaskTest : public VectorTest, public ::testing::WithParamInterface<std::tuple<TR::VectorLength, TR::DataTypes>> {};

class ParameterizedUnaryMaskTest : public VectorTest, public ::testing::WithParamInterface<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>> {};

class ParameterizedBinaryMaskTest : public VectorTest, public ::testing::WithParamInterface<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, uint64_t (*) (uint64_t, uint64_t, int32_t)>> {};

class ParameterizedVBlendTest : public VectorTest, public ::testing::WithParamInterface<std::tuple<TR::VectorLength, TR::DataTypes>> {};

enum MaskConvType {
   maskArrayLoad,
   maskArrayStore,
   maskToArray,
   arrayToMask,
   maskToLongBits,
   longBitsToMask
};

TR::ILOpCode getMaskConvOpcode(MaskConvType convType, TR::DataType maskType) {
   TR_ASSERT_FATAL(maskType.isMask(), "Expected mask type");
   size_t numLanes = maskType.getVectorNumLanes();
   TR::DataType loadType;
   TR::VectorLength vl;
   TR::ILOpCode loadOpcode;
   TR::ILOpCode storeOpcode;
   TR::ILOpCode m2ArrayOpcode;
   TR::ILOpCode array2mOpcode;

   switch (numLanes) {
      case 1:
         loadType = TR::Int8;
         m2ArrayOpcode = TR::ILOpCode::createVectorOpCode(TR::m2b, maskType);
         array2mOpcode = TR::ILOpCode::createVectorOpCode(TR::b2m, maskType);
         break;
      case 2:
         loadType = TR::Int16;
         m2ArrayOpcode = TR::ILOpCode::createVectorOpCode(TR::m2s, maskType);
         array2mOpcode = TR::ILOpCode::createVectorOpCode(TR::s2m, maskType);
         break;
      case 4:
         loadType = TR::Int32;
         m2ArrayOpcode = TR::ILOpCode::createVectorOpCode(TR::m2i, maskType);
         array2mOpcode = TR::ILOpCode::createVectorOpCode(TR::i2m, maskType);
         break;
      case 8:
         loadType = TR::Int64;
         m2ArrayOpcode = TR::ILOpCode::createVectorOpCode(TR::m2l, maskType);
         array2mOpcode = TR::ILOpCode::createVectorOpCode(TR::l2m, maskType);
         break;
      case 16:
         vl = TR::VectorLength128; break;
      case 32:
         vl = TR::VectorLength256; break;
      case 64:
         vl = TR::VectorLength512; break;
      default:
         TR_ASSERT_FATAL(false, "Unexpected number of lanes");
   }

   TR::DataType vectorType = TR::DataType::createVectorType(TR::Int8, maskType.getVectorLength());

   if (numLanes <= 8) {
      loadOpcode = TR::ILOpCode::indirectLoadOpCode(loadType);
      storeOpcode = TR::ILOpCode::indirectStoreOpCode(loadType);
   } else {
      loadType = TR::DataType::createVectorType(TR::Int8, vl);
      loadOpcode = TR::ILOpCode::createVectorOpCode(TR::vloadi, loadType);
      storeOpcode = TR::ILOpCode::createVectorOpCode(TR::vstorei, loadType);
      m2ArrayOpcode = TR::ILOpCode::createVectorOpCode(TR::m2v, maskType, vectorType);
      array2mOpcode = TR::ILOpCode::createVectorOpCode(TR::v2m, vectorType, maskType);
   }

   switch (convType) {
      case maskArrayLoad:
         return loadOpcode;
      case maskArrayStore:
         return storeOpcode;
      case maskToArray:
         return m2ArrayOpcode;
      case arrayToMask:
         return array2mOpcode;
      case maskToLongBits:
         return TR::ILOpCode::createVectorOpCode(TR::mToLongBits, vectorType);
      case longBitsToMask:
         return TR::ILOpCode::createVectorOpCode(TR::mLongBitsToMask, maskType);
      default:
         TR_ASSERT_FATAL(false, "Unexpected mask operation.");
         return TR::ILOpCode();
   }
}

void longBitsToBoolArray(uint64_t bits, bool *arr) {
   for (int64_t i = 0; i < 63; i++) {
      arr[i] = (bits & (1ull << i)) ? true : false;
   }
}

int64_t anyTrue(bool *arr, int32_t numLanes) {
   for (int32_t i = 0; i < numLanes; i++) {
      if (arr[i]) {
         return true;
      }
   }

   return false;
}

int64_t allTrue(bool *arr, int32_t numLanes) {
   for (int32_t i = 0; i < numLanes; i++) {
      if (!arr[i]) {
         return false;
      }
   }

   return true;
}

int64_t trueCount(bool *arr, int32_t numLanes) {
   int64_t trueCount = 0;

   for (int32_t i = 0; i < numLanes; i++) {
      if (arr[i]) {
         trueCount++;
      }
   }

   return trueCount;
}

int64_t firstTrue(bool *arr, int32_t numLanes) {
   for (int32_t i = 0; i < numLanes; i++) {
      if (arr[i]) {
         return i;
      }
   }

   return numLanes;
}

int64_t lastTrue(bool *arr, int32_t numLanes) {
   int64_t lastTrue = -1;

   for (int32_t i = 0; i < numLanes; i++) {
      if (arr[i]) {
         lastTrue = i;
      }
   }

   return lastTrue;
}

uint64_t maskAnd(uint64_t maskA, uint64_t maskB, int32_t numLanes) {
   return (maskA & maskB) & ((1ULL << numLanes) - 1);
}

uint64_t maskOr(uint64_t maskA, uint64_t maskB, int32_t numLanes) {
  return (maskA | maskB) & ((1ULL << numLanes) - 1);
}

uint64_t maskXor(uint64_t maskA, uint64_t maskB, int32_t numLanes) {
  return (maskA ^ maskB) & ((1ULL << numLanes) - 1);
}

void getOpcodeName(TR::ILOpCode opcode, char buf[], const size_t bufSize) {
   if (opcode.isVectorOpCode() && opcode.isTwoTypeVectorOpCode()) {
      TR::DataType srcType = opcode.getVectorSourceDataType();
      TR::DataType resultType = opcode.getVectorResultDataType();
      snprintf(buf, bufSize, "%s%s_%s", opcode.getName(), srcType.toString(), resultType.toString());
   } else if (opcode.isVectorOpCode()) {
      TR::DataType opcodeType = opcode.getVectorDataType();
      snprintf(buf, bufSize, "%s%s", opcode.getName(), opcodeType.toString());
   } else {
      snprintf(buf, bufSize, "%s", opcode.getName());
   }
}

bool isSupported(TR::CPU *cpu, TR::ILOpCode op) {
   if (!op.isVectorOpCode())
      return true;
   return TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(cpu, op);
}

template<typename... Args>
bool isSupported(TR::CPU *cpu, TR::ILOpCode first, Args... args) {
   return isSupported(cpu, first) && isSupported(cpu, args...);
}

TEST_P(ParameterizedMaskTest, mLoadStore) {
   TR::VectorLength vl = std::get<0>(GetParam());
   TR::DataTypes et = std::get<1>(GetParam());

   SKIP_IF(vl > TR::NumVectorLengths, MissingImplementation) << "Vector length is not supported by the target platform";
   SKIP_ON_S390(KnownBug) << "This test is currently disabled on Z platforms because not all Z platforms have vector support (issue #1843)";
   SKIP_ON_S390X(KnownBug) << "This test is currently disabled on Z platforms because not all Z platforms have vector support (issue #1843)";

   TR::DataType vt = TR::DataType::createMaskType(et, vl);
   TR::ILOpCode loadOp = TR::ILOpCode::createVectorOpCode(TR::mloadi, vt);
   TR::ILOpCode storeOp = TR::ILOpCode::createVectorOpCode(TR::mstorei, vt);
   TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);

   bool platformSupport = TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, loadOp) && TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, storeOp);
   SKIP_IF(!platformSupport, MissingImplementation) << "Opcode is not supported by the target platform";

   const size_t inputTreesSize = 1024;
   char inputTrees[inputTreesSize];
   char *formatStr = "(method return= NoType args=[Address,Address]   "
                     "  (block                                        "
                     "     (mstorei%s  offset=0                       "
                     "         (aload parm=0)                         "
                     "         (mloadi%s (aload parm=1)))             "
                     "     (return)))                                 ";

   snprintf(inputTrees, inputTreesSize, formatStr, vt.toString(), vt.toString());
   auto trees = parseString(inputTrees);
   ASSERT_NOTNULL(trees);

   Tril::DefaultCompiler compiler(trees);
   ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

   auto entry_point = compiler.getEntryPoint<void (*)(void *,void *)>();

   const uint8_t maxVectorLength = MAX_NUM_LANES;
   char output[maxVectorLength] = {0};
   char input[maxVectorLength] = {0};
   char zero[maxVectorLength] = {0};

   bool useBitMask = cpu.supportsFeature(OMR_FEATURE_X86_AVX512F);
   size_t maskSize = TR::DataType::getSize(vt) / (useBitMask ? 8 : 1);

   for (int i = 0; i < maskSize; i++) {
      input[i] = i % 2 ? 0xff : 0;
   }

   entry_point(output, input);

   EXPECT_EQ(0, memcmp(input, output, maskSize));
   EXPECT_EQ(0, memcmp(output + maskSize, zero, maxVectorLength - maskSize));
}

TEST_P(ParameterizedBinaryMaskTest, bitwiseMaskTests) {
   TR::VectorLength vl = std::get<0>(GetParam());
   TR::DataTypes et = std::get<1>(GetParam());
   TR::VectorOperation maskOperation = std::get<2>(GetParam());
   uint64_t (*refFunc)(uint64_t, uint64_t, int32_t) = std::get<3>(GetParam());

   SKIP_IF(vl > TR::NumVectorLengths, MissingImplementation) << "Vector length is not supported by the target platform";
   SKIP_ON_S390(KnownBug) << "This test is currently disabled on Z platforms because not all Z platforms have vector support (issue #1843)";
   SKIP_ON_S390X(KnownBug) << "This test is currently disabled on Z platforms because not all Z platforms have vector support (issue #1843)";

   TR::DataType vt = TR::DataType::createVectorType(et, vl);
   TR::DataType mt = TR::DataType::createMaskType(et, vl);
   TR::ILOpCode loadOp = getMaskConvOpcode(maskArrayLoad, mt);
   TR::ILOpCode maskOpcode = TR::ILOpCode::createVectorOpCode(maskOperation, mt);
   TR::ILOpCode m2LongOpcode = TR::ILOpCode::createVectorOpCode(TR::mToLongBits, mt);

   TR::ILOpCode a2m = getMaskConvOpcode(arrayToMask, mt);
   TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
   bool platformSupport = isSupported(&cpu, loadOp, maskOpcode, a2m, m2LongOpcode);
   SKIP_IF(!platformSupport, MissingImplementation) << "Opcode is not supported by the target platform";

   const size_t inputTreesSize = 1024;
   char inputTrees[1024];
   const size_t bits2mSize = 64;
   char bits2m[bits2mSize];
   const size_t binaryMaskOpSize = 64;
   char binaryMaskOp[binaryMaskOpSize];

   char *formatStr = "(method return= NoType args=[Address,Address,Address]"
                     "  (block                                        "
                     "     (lstorei offset=0                          "
                     "        (aload parm=0)                          "
                     "        (mToLongBits%s  offset=0                "
                     "           (%s                                  "
                     "              (%s (%s%s (aload parm=1)))        "
                     "              (%s (%s%s (aload parm=2))))))     "
                     "     (return)))                                 ";

   getOpcodeName(a2m, bits2m, bits2mSize);
   getOpcodeName(maskOpcode, binaryMaskOp, binaryMaskOpSize);

   const char *arraySizeType = vt.getVectorNumLanes() > 8 ? vt.toString() : "";
   snprintf(inputTrees, inputTreesSize, formatStr, mt.toString(),
           binaryMaskOp,
           bits2m,
           loadOp.getName(), arraySizeType,
           bits2m,
           loadOp.getName(), arraySizeType);
   auto trees = parseString(inputTrees);

   ASSERT_NOTNULL(trees);

   Tril::DefaultCompiler compiler(trees);
   ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

   auto entry_point = compiler.getEntryPoint<void (*)(void *,void *,void *)>();
   int32_t numLanes = mt.getVectorNumLanes();

   uint64_t bitMasks[] = {
       0,
       1,
       static_cast<uint64_t>(-1),
       1ull << (numLanes - 1),
       1 | (1ull << (numLanes - 1)),
       0xC, 0x3, 0x7, 0xF,
       0x9, 0xF, 0xFF, 0xF00F,
       0xF0, 0x2, 0x4, 0x8,
       0xF0F0F0F0F0F0F0F0ull
   };

   int numMasks = sizeof(bitMasks) / sizeof(uint64_t);

   for (int i = 0; i < numMasks; i++) {
      bool inputMaskA[MAX_NUM_LANES] = {};
      longBitsToBoolArray(bitMasks[i], inputMaskA);

      for (int j = 0; j < numMasks; j++) {
         bool inputMaskB[MAX_NUM_LANES] = {};
         longBitsToBoolArray(bitMasks[j], inputMaskB);
         uint64_t refResult = refFunc(bitMasks[i], bitMasks[j], numLanes);
         uint64_t result = 0;
         entry_point(&result, inputMaskA, inputMaskB);

         EXPECT_EQ(refResult, result) << binaryMaskOp << "test failed with input: 0x" << std::hex << std::setfill('0') << bitMasks[i] << " & 0x" << bitMasks[j] << " " << mt.toString();
      }
   }
}

TEST_P(ParameterizedMaskTest, boolToMaskToBool) {
   TR::VectorLength vl = std::get<0>(GetParam());
   TR::DataTypes et = std::get<1>(GetParam());

   SKIP_IF(vl > TR::NumVectorLengths, MissingImplementation) << "Vector length is not supported by the target platform";
   SKIP_ON_S390(KnownBug) << "This test is currently disabled on Z platforms because not all Z platforms have vector support (issue #1843)";
   SKIP_ON_S390X(KnownBug) << "This test is currently disabled on Z platforms because not all Z platforms have vector support (issue #1843)";

   TR::DataType vt = TR::DataType::createVectorType(et, vl);
   TR::DataType mt = TR::DataType::createMaskType(et, vl);
   TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);

   TR::ILOpCode loadOpcode = getMaskConvOpcode(maskArrayLoad, mt);
   TR::ILOpCode storeOpcode = getMaskConvOpcode(maskArrayStore, mt);

   TR::ILOpCode a2m = getMaskConvOpcode(arrayToMask, mt);
   TR::ILOpCode m2a = getMaskConvOpcode(maskToArray, mt);

   bool support = isSupported(&cpu, loadOpcode, storeOpcode, a2m, m2a);
   SKIP_IF(!support, MissingImplementation) << "Opcode is not supported by the target platform";
   const char *arraySizeType = vt.getVectorNumLanes() > 8 ? vt.toString() : "";

   const size_t inputTreesSize = 1024;
   char inputTrees[inputTreesSize];
   const size_t m2bitsSize = 64;
   char m2bits[m2bitsSize];
   const size_t bits2mSize = 64;
   char bits2m[bits2mSize];

   char *formatStr = "(method return= NoType args=[Address,Address]  \n"
                     "  (block                                       \n"
                     "     (%s%s offset=0                            \n"
                     "         (aload parm=0)                        \n"
                     "         (%s                                   \n"
                     "            (%s                                \n"
                     "               (%s%s (aload parm=1)))))        \n"
                     "     (return)))                                \n";

   getOpcodeName(m2a, m2bits, m2bitsSize);
   getOpcodeName(a2m, bits2m, bits2mSize);
   snprintf(inputTrees, inputTreesSize, formatStr,
         storeOpcode.getName(), arraySizeType,
         m2bits,
         bits2m,
         loadOpcode.getName(), arraySizeType);

   auto trees = parseString(inputTrees);
   ASSERT_NOTNULL(trees);

   Tril::DefaultCompiler compiler(trees);
   ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

   auto entry_point = compiler.getEntryPoint<void (*)(void *,void *)>();

   const uint8_t maxVectorLength = MAX_NUM_LANES;
   char output[maxVectorLength] = {0};
   char input[maxVectorLength] = {0};
   inputTrees[0] = 1;

   for (int i = 0; i < mt.getVectorNumLanes(); i++) {
      input[i] = i % 2 ? 1 : 0;
   }

   entry_point(output, input);

   EXPECT_EQ(0, memcmp(input, output, mt.getVectorNumLanes()));
}

INSTANTIATE_TEST_CASE_P(MaskTypeParameters, ParameterizedMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes>>(
      std::make_tuple(TR::VectorLength128, TR::Int8),
      std::make_tuple(TR::VectorLength128, TR::Int16),
      std::make_tuple(TR::VectorLength128, TR::Int32),
      std::make_tuple(TR::VectorLength128, TR::Int64),
      std::make_tuple(TR::VectorLength128, TR::Float),
      std::make_tuple(TR::VectorLength128, TR::Double),
      std::make_tuple(TR::VectorLength256, TR::Int8),
      std::make_tuple(TR::VectorLength256, TR::Int16),
      std::make_tuple(TR::VectorLength256, TR::Int32),
      std::make_tuple(TR::VectorLength256, TR::Int64),
      std::make_tuple(TR::VectorLength256, TR::Float),
      std::make_tuple(TR::VectorLength256, TR::Double),
      std::make_tuple(TR::VectorLength512, TR::Int8),
      std::make_tuple(TR::VectorLength512, TR::Int16),
      std::make_tuple(TR::VectorLength512, TR::Int32),
      std::make_tuple(TR::VectorLength512, TR::Int64),
      std::make_tuple(TR::VectorLength512, TR::Float),
      std::make_tuple(TR::VectorLength512, TR::Double)
)));

TEST_P(ParameterizedUnaryMaskTest, unaryTest) {
   TR::VectorLength vl = std::get<0>(GetParam());
   TR::DataTypes et = std::get<1>(GetParam());
   TR::VectorOperation maskOperation = std::get<2>(GetParam());
   auto refFunc = std::get<3>(GetParam());

   SKIP_IF(vl > TR::NumVectorLengths, MissingImplementation) << "Vector length is not supported by the target platform";
   SKIP_ON_S390(KnownBug) << "This test is currently disabled on Z platforms because not all Z platforms have vector support (issue #1843)";
   SKIP_ON_S390X(KnownBug) << "This test is currently disabled on Z platforms because not all Z platforms have vector support (issue #1843)";

   TR::DataType vt = TR::DataType::createVectorType(et, vl);
   TR::DataType mt = TR::DataType::createMaskType(et, vl);
   TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);

   TR::ILOpCode loadOpcode = getMaskConvOpcode(maskArrayLoad, mt);
   TR::ILOpCode storeOpcode = getMaskConvOpcode(maskArrayStore, mt);

   TR::ILOpCode a2m = getMaskConvOpcode(arrayToMask, mt);
   TR::ILOpCode maskOpcode = TR::ILOpCode::createVectorOpCode(maskOperation, mt);

   bool support = isSupported(&cpu, loadOpcode, storeOpcode, a2m, maskOpcode);
   SKIP_IF(!support, MissingImplementation) << "Opcode is not supported by the target platform";
   const char *arraySizeType = vt.getVectorNumLanes() > 8 ? vt.toString() : "";

   const size_t inputTreesSize = 1024;
   char inputTrees[inputTreesSize];
   const size_t bits2mSize = 64;
   char bits2m[bits2mSize];
   const size_t unaryMaskOpSize = 64;
   char unaryMaskOp[unaryMaskOpSize];

   char *formatStr = "(method return= NoType args=[Address,Address]  \n"
                     "  (block                                       \n"
                     "     (%s offset=0                              \n"
                     "         (aload parm=0)                        \n"
                     "         (%s                                   \n"
                     "            (%s                                \n"
                     "               (%s%s (aload parm=1)))))        \n"
                     "     (return)))                                \n";

   getOpcodeName(a2m, bits2m, bits2mSize);
   getOpcodeName(maskOpcode, unaryMaskOp, unaryMaskOpSize);
   snprintf(inputTrees, inputTreesSize, formatStr,
           maskOpcode.is8Byte() ? "lstorei" : "istorei",
           unaryMaskOp,
           bits2m,
           loadOpcode.getName(), arraySizeType);

   auto trees = parseString(inputTrees);
   ASSERT_NOTNULL(trees);

   Tril::DefaultCompiler compiler(trees);
   ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

   auto entry_point = compiler.getEntryPoint<void (*)(void *,void *)>();
   int32_t numLanes = mt.getVectorNumLanes();

   uint64_t bitMasks[] = {
      0, 1,
      static_cast<uint64_t>(-1),
      1ull << (numLanes - 1),
      1 | (1ull << (numLanes - 1)),
      0xC, 0x3, 0x7, 0xF,
      0x9, 0xF, 0xFF, 0xF00F,
      0xF0, 0x2, 0x4, 0x8,
      0xF0F0F0F0F0F0F0F0ull
   };

   for (int i = 0; i < sizeof(bitMasks); i++) {
      bool inputMask[MAX_NUM_LANES] = {};
      longBitsToBoolArray(bitMasks[i], inputMask);
      int64_t refResult = refFunc(inputMask, numLanes);
      int64_t result = 0;

      if (maskOpcode.is8Byte()) {
         entry_point(&result, inputMask);
      } else {
         int32_t result32 = 0;
         entry_point(&result32, inputMask);
         result = static_cast<int64_t>(result32);
      }

      EXPECT_EQ(refResult, result) << unaryMaskOp << " test failed with Input Mask: 0x" << std::hex << std::setw(8) << std::setfill('0') << bitMasks[i];
   }
}

TEST_P(ParameterizedVBlendTest, vblend) {
   TR::VectorLength vl = std::get<0>(GetParam());
   TR::DataTypes et = std::get<1>(GetParam());

   SKIP_IF(vl > TR::NumVectorLengths, MissingImplementation) << "Vector length is not supported by the target platform";
   SKIP_ON_S390(KnownBug) << "This test is currently disabled on Z platforms because not all Z platforms have vector support (issue #1843)";
   SKIP_ON_S390X(KnownBug) << "This test is currently disabled on Z platforms because not all Z platforms have vector support (issue #1843)";

   TR::DataType vt = TR::DataType::createVectorType(et, vl);
   TR::DataType mt = TR::DataType::createMaskType(et, vl);
   TR::ILOpCode loadOp = getMaskConvOpcode(maskArrayLoad, mt);
   TR::ILOpCode blend = TR::ILOpCode::createVectorOpCode(TR::vblend, mt);

   TR::ILOpCode a2m = getMaskConvOpcode(arrayToMask, mt);
   TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
   bool platformSupport = isSupported(&cpu, loadOp, a2m, blend);
   SKIP_IF(!platformSupport, MissingImplementation) << "Opcode is not supported by the target platform";

   const size_t inputTreesSize = 1024;
   char inputTrees[inputTreesSize];
   const size_t bits2mSize = 64;
   char bits2m[bits2mSize];

   char *formatStr = "(method return= NoType args=[Address,Address,Address,Address]"
                     "  (block                                        "
                     "     (vstorei%s offset=0                        "
                     "        (aload parm=0)                          "
                     "        (vblend%s                               "
                     "              (vloadi%s (aload parm=1))         "
                     "              (vloadi%s (aload parm=2))         "
                     "              (%s (%s%s (aload parm=3)))))      "
                     "     (return)))                                 ";

   getOpcodeName(a2m, bits2m, bits2mSize);

   const char *arraySizeType = vt.getVectorNumLanes() > 8 ? vt.toString() : "";
   snprintf(inputTrees, inputTreesSize, formatStr,
           vt.toString(), vt.toString(), vt.toString(), vt.toString(),
           bits2m,
           loadOp.getName(), arraySizeType);

   auto trees = parseString(inputTrees);
   ASSERT_NOTNULL(trees);

   Tril::DefaultCompiler compiler(trees);
   ASSERT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

   auto entry_point = compiler.getEntryPoint<void (*)(void *,void *,void *,void *)>();
   int32_t numLanes = mt.getVectorNumLanes();
   uint32_t elementSizeBytes = TR::DataType::getSize(et);

   bool inputMask[MAX_NUM_LANES] = {};
   uint8_t inputA[MAX_NUM_LANES];
   uint8_t inputB[MAX_NUM_LANES];
   uint8_t expected[MAX_NUM_LANES];

   void *aOff = inputA;
   void *bOff = inputB;
   void *expectedOff = expected;

   for (int i = 0; i < numLanes; i++) {
      TRTest::generateByType(aOff, et, false);
      TRTest::generateByType(bOff, et, false);
      inputMask[i] = (rand() < RAND_MAX / 2) ? true : false;

      // If the lane is set, then the expected result is the second input
      if (inputMask[i]) {
         memcpy(expectedOff, bOff, elementSizeBytes);
      } else {
         memcpy(expectedOff, aOff, elementSizeBytes);
      }

      aOff = static_cast<char *>(aOff) + elementSizeBytes;
      bOff = static_cast<char *>(bOff) + elementSizeBytes;
      expectedOff = static_cast<char *>(expectedOff) + elementSizeBytes;
   }

   uint8_t result[MAX_NUM_LANES] = {}; // allocate proper array
   entry_point(result, inputA, inputB, inputMask); // pass array

   TRTest::compareResults(expected, result, et, vl);
}

INSTANTIATE_TEST_CASE_P(vblend128Test, ParameterizedVBlendTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes>>(
   std::make_tuple(TR::VectorLength128, TR::Int8),
   std::make_tuple(TR::VectorLength128, TR::Int16),
   std::make_tuple(TR::VectorLength128, TR::Int32),
   std::make_tuple(TR::VectorLength128, TR::Int64),
   std::make_tuple(TR::VectorLength128, TR::Float),
   std::make_tuple(TR::VectorLength128, TR::Double)
)));

INSTANTIATE_TEST_CASE_P(vblend256Test, ParameterizedVBlendTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes>>(
   std::make_tuple(TR::VectorLength256, TR::Int8),
   std::make_tuple(TR::VectorLength256, TR::Int16),
   std::make_tuple(TR::VectorLength256, TR::Int32),
   std::make_tuple(TR::VectorLength256, TR::Int64),
   std::make_tuple(TR::VectorLength256, TR::Float),
   std::make_tuple(TR::VectorLength256, TR::Double)
)));

INSTANTIATE_TEST_CASE_P(vblend512Test, ParameterizedVBlendTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes>>(
   std::make_tuple(TR::VectorLength512, TR::Int8),
   std::make_tuple(TR::VectorLength512, TR::Int16),
   std::make_tuple(TR::VectorLength512, TR::Int32),
   std::make_tuple(TR::VectorLength512, TR::Int64),
   std::make_tuple(TR::VectorLength512, TR::Float),
   std::make_tuple(TR::VectorLength512, TR::Double)
)));

INSTANTIATE_TEST_CASE_P(mTrueCount128ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength128, TR::Int8,   TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength128, TR::Int16,  TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength128, TR::Int32,  TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength128, TR::Int64,  TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength128, TR::Float,  TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength128, TR::Double, TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount))
)));

INSTANTIATE_TEST_CASE_P(mTrueCount256ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength256, TR::Int8,   TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength256, TR::Int16,  TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength256, TR::Int32,  TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength256, TR::Int64,  TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength256, TR::Float,  TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength256, TR::Double, TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount))
)));

INSTANTIATE_TEST_CASE_P(mTrueCount512ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength512, TR::Int8,   TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength512, TR::Int16,  TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength512, TR::Int32,  TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength512, TR::Int64,  TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength512, TR::Float,  TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount)),
   std::make_tuple(TR::VectorLength512, TR::Double, TR::mTrueCount, static_cast<int64_t (*)(bool*, int32_t)>(::trueCount))
)));

INSTANTIATE_TEST_CASE_P(mFirstTrue128ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength128, TR::Int8,   TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength128, TR::Int16,  TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength128, TR::Int32,  TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength128, TR::Int64,  TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength128, TR::Float,  TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength128, TR::Double, TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue))
)));

INSTANTIATE_TEST_CASE_P(mFirstTrue256ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength256, TR::Int8,   TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength256, TR::Int16,  TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength256, TR::Int32,  TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength256, TR::Int64,  TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength256, TR::Float,  TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength256, TR::Double, TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue))
)));

INSTANTIATE_TEST_CASE_P(mFirstTrue512ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength512, TR::Int8,   TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength512, TR::Int16,  TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength512, TR::Int32,  TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength512, TR::Int64,  TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength512, TR::Float,  TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue)),
   std::make_tuple(TR::VectorLength512, TR::Double, TR::mFirstTrue, static_cast<int64_t (*)(bool*, int32_t)>(::firstTrue))
)));

INSTANTIATE_TEST_CASE_P(mLastTrue128ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength128, TR::Int8,   TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength128, TR::Int16,  TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength128, TR::Int32,  TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength128, TR::Int64,  TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength128, TR::Float,  TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength128, TR::Double, TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue))
)));

INSTANTIATE_TEST_CASE_P(mLastTrue256ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength256, TR::Int8,   TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength256, TR::Int16,  TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength256, TR::Int32,  TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength256, TR::Int64,  TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength256, TR::Float,  TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength256, TR::Double, TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue))
)));

INSTANTIATE_TEST_CASE_P(mLastTrue512ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength512, TR::Int8,   TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength512, TR::Int16,  TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength512, TR::Int32,  TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength512, TR::Int64,  TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength512, TR::Float,  TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue)),
   std::make_tuple(TR::VectorLength512, TR::Double, TR::mLastTrue, static_cast<int64_t (*)(bool*, int32_t)>(::lastTrue))
)));

INSTANTIATE_TEST_CASE_P(mAnyTrue128ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength128, TR::Int8,   TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength128, TR::Int16,  TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength128, TR::Int32,  TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength128, TR::Int64,  TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength128, TR::Float,  TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength128, TR::Double, TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue))
)));

INSTANTIATE_TEST_CASE_P(mAnyTrue256ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength256, TR::Int8,   TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength256, TR::Int16,  TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength256, TR::Int32,  TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength256, TR::Int64,  TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength256, TR::Float,  TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength256, TR::Double, TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue))
)));

INSTANTIATE_TEST_CASE_P(mAnyTrue512ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength512, TR::Int8,   TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength512, TR::Int16,  TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength512, TR::Int32,  TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength512, TR::Int64,  TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength512, TR::Float,  TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue)),
   std::make_tuple(TR::VectorLength512, TR::Double, TR::mAnyTrue, static_cast<int64_t (*)(bool*, int32_t)>(::anyTrue))
)));

INSTANTIATE_TEST_CASE_P(mAllTrue128ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength128, TR::Int8,   TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength128, TR::Int16,  TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength128, TR::Int32,  TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength128, TR::Int64,  TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength128, TR::Float,  TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength128, TR::Double, TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue))
)));

INSTANTIATE_TEST_CASE_P(mAllTrue256ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength256, TR::Int8,   TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength256, TR::Int16,  TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength256, TR::Int32,  TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength256, TR::Int64,  TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength256, TR::Float,  TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength256, TR::Double, TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue))
)));

INSTANTIATE_TEST_CASE_P(mAllTrue512ParametersTest, ParameterizedUnaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, int64_t (*) (bool *arr, int32_t)>>(
   std::make_tuple(TR::VectorLength512, TR::Int8,   TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength512, TR::Int16,  TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength512, TR::Int32,  TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength512, TR::Int64,  TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength512, TR::Float,  TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue)),
   std::make_tuple(TR::VectorLength512, TR::Double, TR::mAllTrue, static_cast<int64_t (*)(bool*, int32_t)>(::allTrue))
)));

INSTANTIATE_TEST_CASE_P(mandTest128ParametersTest, ParameterizedBinaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, uint64_t (*) (uint64_t, uint64_t, int32_t)>>(
   std::make_tuple(TR::VectorLength128, TR::Int8,   TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength128, TR::Int16,  TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength128, TR::Int32,  TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength128, TR::Int64,  TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength128, TR::Float,  TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength128, TR::Double, TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd))
)));

INSTANTIATE_TEST_CASE_P(mandTest256ParametersTest, ParameterizedBinaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, uint64_t (*) (uint64_t, uint64_t, int32_t)>>(
   std::make_tuple(TR::VectorLength256, TR::Int8,   TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength256, TR::Int16,  TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength256, TR::Int32,  TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength256, TR::Int64,  TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength256, TR::Float,  TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength256, TR::Double, TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd))
)));

INSTANTIATE_TEST_CASE_P(mandTest512ParametersTest, ParameterizedBinaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, uint64_t (*) (uint64_t, uint64_t, int32_t)>>(
   std::make_tuple(TR::VectorLength512, TR::Int8,   TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength512, TR::Int16,  TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength512, TR::Int32,  TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength512, TR::Int64,  TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength512, TR::Float,  TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd)),
   std::make_tuple(TR::VectorLength512, TR::Double, TR::mand, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskAnd))
)));

INSTANTIATE_TEST_CASE_P(morTest128ParametersTest, ParameterizedBinaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, uint64_t (*) (uint64_t, uint64_t, int32_t)>>(
   std::make_tuple(TR::VectorLength128, TR::Int8,   TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength128, TR::Int16,  TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength128, TR::Int32,  TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength128, TR::Int64,  TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength128, TR::Float,  TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength128, TR::Double, TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr))
)));

INSTANTIATE_TEST_CASE_P(morTest256ParametersTest, ParameterizedBinaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, uint64_t (*) (uint64_t, uint64_t, int32_t)>>(
   std::make_tuple(TR::VectorLength256, TR::Int8,   TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength256, TR::Int16,  TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength256, TR::Int32,  TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength256, TR::Int64,  TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength256, TR::Float,  TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength256, TR::Double, TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr))
)));

INSTANTIATE_TEST_CASE_P(morTest512ParametersTest, ParameterizedBinaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, uint64_t (*) (uint64_t, uint64_t, int32_t)>>(
   std::make_tuple(TR::VectorLength512, TR::Int8,   TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength512, TR::Int16,  TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength512, TR::Int32,  TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength512, TR::Int64,  TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength512, TR::Float,  TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr)),
   std::make_tuple(TR::VectorLength512, TR::Double, TR::mor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskOr))
)));

INSTANTIATE_TEST_CASE_P(mxorTest128ParametersTest, ParameterizedBinaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, uint64_t (*) (uint64_t, uint64_t, int32_t)>>(
   std::make_tuple(TR::VectorLength128, TR::Int8,   TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength128, TR::Int16,  TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength128, TR::Int32,  TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength128, TR::Int64,  TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength128, TR::Float,  TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength128, TR::Double, TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor))
)));

INSTANTIATE_TEST_CASE_P(mxorTest256ParametersTest, ParameterizedBinaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, uint64_t (*) (uint64_t, uint64_t, int32_t)>>(
   std::make_tuple(TR::VectorLength256, TR::Int8,   TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength256, TR::Int16,  TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength256, TR::Int32,  TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength256, TR::Int64,  TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength256, TR::Float,  TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength256, TR::Double, TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor))
)));

INSTANTIATE_TEST_CASE_P(mxorTest512ParametersTest, ParameterizedBinaryMaskTest, ::testing::ValuesIn(*TRTest::MakeVector<std::tuple<TR::VectorLength, TR::DataTypes, TR::VectorOperation, uint64_t (*) (uint64_t, uint64_t, int32_t)>>(
   std::make_tuple(TR::VectorLength512, TR::Int8,   TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength512, TR::Int16,  TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength512, TR::Int32,  TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength512, TR::Int64,  TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength512, TR::Float,  TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor)),
   std::make_tuple(TR::VectorLength512, TR::Double, TR::mxor, static_cast<uint64_t (*)(uint64_t, uint64_t, int32_t)>(::maskXor))
)));
