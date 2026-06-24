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
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include "JitTest.hpp"
#include "default_compiler.hpp"
#include "compilerunittest/CompilerUnitTest.hpp"
#include "VectorTestUtils.hpp"
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>


/**
 * @brief Calculate the bitwise complement of a value for integral types.
 *
 * @tparam T The type of the value (must be integral)
 * @param v The value to complement
 * @return T The bitwise complement of v
 */
template<typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
complement(T v) {
    return ~v;
}

/**
 * @brief Overload for non-integral types (returns zero).
 *
 * @tparam T The type of the value (must be non-integral)
 * @return T Zero value
 */
template<typename T>
typename std::enable_if<!std::is_integral<T>::value, T>::type
complement(T) {
    return T(0);
}

/**
 * @brief Calculate the number of bits set to 1 (population count) for integral types.
 *
 * @tparam T The type of the value (must be integral)
 * @param v The value to count bits in
 * @return T The number of bits set to 1
 */
template<typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
populationCount(T v) {
    typedef typename std::make_unsigned<T>::type U;
    U x = static_cast<U>(v);
    T count = 0;
    // Use Brian Kernighan's algorithm: each iteration clears the lowest set bit
    while (x) {
        x &= (x - 1);  // Clear the lowest set bit
        ++count;
    }
    return count;
}

/**
 * @brief Overload for non-integral types (returns zero).
 *
 * @tparam T The type of the value (must be non-integral)
 * @return T Zero value
 */
template<typename T>
typename std::enable_if<!std::is_integral<T>::value, T>::type
populationCount(T) {
    return T(0);
}

/**
 * @brief Calculate the number of leading zero bits for integral types.
 *
 * @tparam T The type of the value (must be integral)
 * @param v The value to count leading zeros in
 * @return T The number of leading zero bits
 */
template<typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
leadingZeroes(T v) {
    typedef typename std::make_unsigned<T>::type U;
    U x = static_cast<U>(v);

    // Special case: if value is zero, all bits are leading zeros
    if (x == 0)
        return sizeof(T) * 8;

    T lz = 0;
    // Start with a mask at the most significant bit position
    U mask = U(1) << (sizeof(T) * 8 - 1);

    // Count zeros from the most significant bit until we find a set bit
    while ((x & mask) == 0) {
        lz++;
        mask >>= 1;
    }
    return lz;
}

/**
 * @brief Overload for non-integral types (returns zero).
 *
 * @tparam T The type of the value (must be non-integral)
 * @return T Zero value
 */
template<typename T>
typename std::enable_if<!std::is_integral<T>::value, T>::type
leadingZeroes(T) {
    return T(0);
}

/**
 * @brief Calculate the number of trailing zero bits for integral types.
 *
 * @tparam T The type of the value (must be integral)
 * @param v The value to count trailing zeros in
 * @return T The number of trailing zero bits
 */
template<typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
trailingZeroes(T v) {
    typedef typename std::make_unsigned<T>::type U;
    U x = static_cast<U>(v);

    // Special case: if value is zero, all bits are trailing zeros
    if (x == 0)
        return sizeof(T) * 8;

    T tz = 0;
    // Count zeros from the least significant bit until we find a set bit
    while ((x & 1) == 0) {
        tz++;
        x >>= 1;
    }
    return tz;
}

/**
 * @brief Overload for non-integral types (returns zero).
 *
 * @tparam T The type of the value (must be non-integral)
 * @return T Zero value
 */
template<typename T>
typename std::enable_if<!std::is_integral<T>::value, T>::type
trailingZeroes(T) {
    return T(0);
}

/**
 * @brief Create a delegate function for calculating expected results for a vector operation.
 *
 * @tparam T The element type
 * @param op The vector operation to create a delegate for
 * @return std::function<T(T)> A function that calculates the expected result for the operation
 */
template<typename T>
std::function<T(T)> makeDelegate(TR::VectorOperation op) {
    switch (op) {

    case TR::vabs:
        return [](T v) -> T { return v < 0 ? -v : v; };

    case TR::vpopcnt:
        return [](T v) -> T { return populationCount(v); };

    case TR::vnolz:
        return [](T v) -> T { return leadingZeroes(v); };

    case TR::vnotz:
        return [](T v) -> T { return trailingZeroes(v); };

    case TR::vneg:
        return [](T v) -> T { return -v; };

    case TR::vnot:
        return [](T v) -> T { return complement(v); };
    }
    return nullptr;
}
/**
 * @brief Generate the IL tree and compile it for a vector unary operation test.
 *
 * The source vector is loaded indirectly from an address and the result is stored
 * indirectly to an address.
 *
 * @param opcode The IL opcode for the vector operation
 * @return Tril::DefaultCompiler The compiled test tree
 */
Tril::DefaultCompiler generateTestTree(TR::ILOpCode opcode) {
    TR::DataType vectorType = opcode.getType();
    char inputTrees[1024];
    
    // Build IL tree: method(Address dest, Address src) { *dest = op(*src); return; }
    char *formatStr = "(method return= NoType args=[Address,Address] "
                        "(block "
                            "(vstorei%s offset=0 "
                                "(aload parm=0) "
                                "(%s%s "
                                    "(vloadi%s (aload parm=1)))) "
                            "(return))) ";

    // Format the IL tree string with the vector type and operation name
    sprintf(inputTrees, formatStr,
        vectorType.toString(),
        opcode.getName(),
        vectorType.toString(),
        vectorType.toString());

    // Parse the IL tree string into an AST
    auto trees = parseString(inputTrees);
    EXPECT_NE(nullptr, trees);
    Tril::DefaultCompiler compiler(trees);

    // Compile the IL tree and verify compilation succeeds
    EXPECT_EQ(0, compiler.compile()) << "Compilation failed unexpectedly\n" << "Input trees: " << inputTrees;

    return compiler;
}

/**
 * @brief Generate test source data with a variety of edge case values.
 *
 * This function returns a source data vector with a good variation of different values
 * including edge cases like min, max, zero, infinity, and NaN values.
 *
 * @tparam T The element type
 * @param vectorElements The number of elements in the vector to ensure proper alignment
 * @return std::vector<T> A vector containing test data
 */
template <typename T>
std::vector<T> getTestSourceData(int vectorElements) {
    // Initialize test data with edge cases: zero, one, negative one, max, min values,
    // boundary values (min+1, max/2, min/2), and floating-point special values
    // (epsilon, round_error, infinity, NaN, denorm_min)
    std::vector<T> data = std::vector<T>{0, 1, -1, std::numeric_limits<T>::max(), std::numeric_limits<T>::min(),
        std::numeric_limits<T>::min() + 1, std::numeric_limits<T>::max()/2, std::numeric_limits<T>::min()/2,
        std::numeric_limits<T>::epsilon(), std::numeric_limits<T>::round_error(), std::numeric_limits<T>::infinity(),
        std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::signaling_NaN(), std::numeric_limits<T>::denorm_min()};

    // Round up the data size to the nearest multiple of vectorElements by padding with zeros
    // Formula: ((size + elements - 1) / elements) * elements, implemented with bit operations
    data.resize((data.size() + vectorElements - 1) & ~(vectorElements - 1));
    return data;
}

/**
 * @brief Parameterized test class for vector unary operations.
 *
 * @tparam T Configuration type containing the element type and operation
 */
template <typename T>
class ParameterizedUnaryVectorTest : public TRTest::JitTest {
    protected:
    /**
     * @brief Run a test for a specific vector length.
     *
     * @param vectorLength The vector length to test
     */
    void runTest(TR::VectorLength vectorLength) {
        // Extract the element type and operation from the test configuration template parameter
        using TestType = typename T::Type;
        TR::VectorOperation testOpcode = T::operation;

        // Create a vector data type with the specified element type and vector length
        TR::DataType vectorType = TR::DataType::createVectorType(TR::DataType::mapPrimitiveType<TestType>(), vectorLength);
        TR::ILOpCode opcode = TR::ILOpCode::createVectorOpCode(T::operation, vectorType);
        
        // Detect the current CPU and check if it supports this vector operation
        TR::CPU cpu = TR::CPU::detect(privateOmrPortLibrary);
        bool isSupported = TR::CodeGenerator::getSupportsOpCodeForAutoSIMD(&cpu, opcode);
        SKIP_IF(!isSupported, MissingImplementation) << "Opcode not supported";

        // Generate and compile the IL tree for this vector operation
        Tril::DefaultCompiler compiler = generateTestTree(opcode);
        auto entry_point = compiler.getEntryPoint<void (*)(TestType*,TestType*)>();
        
        // Calculate how many elements fit in one vector
        int vectorElements = vectorType.getVectorSize() / sizeof(TestType);

        // Generate test input data with various edge cases
        std::vector<TestType> input = getTestSourceData<TestType>(vectorElements);

        // Allocate output buffer with the same size as input
        std::vector<TestType> output(input.size());

        // Execute the compiled code on each vector-sized chunk of input data
        for(int i = 0; i < input.size(); i += vectorElements) {
            entry_point(&output[i], &input[i]);
        }

        // Create a reference implementation function for this operation
        auto fn = makeDelegate<TestType>(testOpcode);
        
        // Verify each output element matches the expected result from the reference implementation
        for (int i = 0; i < output.size(); i++) {
            EXPECT_EQ(output[i], fn(input[i]));
        }
    }
};

TYPED_TEST_CASE_P(ParameterizedUnaryVectorTest);

// Define test cases for different vector sizes (128-bit and 256-bit).
// The SKIP_IF checks prevent running tests on platforms that don't support these vector lengths,
// but we still conditionally compile them based on the target architecture to avoid unnecessary code generation.
TYPED_TEST_P(ParameterizedUnaryVectorTest, Vector128) {
    SKIP_IF(TR::VectorLength128 > TR::NumVectorLengths, MissingImplementation) << "Vector length is not supported by the target platform";
    this->runTest(TR::VectorLength128);
}
#if defined(TR_TARGET_X86)
TYPED_TEST_P(ParameterizedUnaryVectorTest, Vector256) {
    SKIP_IF(TR::VectorLength256 > TR::NumVectorLengths, MissingImplementation) << "Vector length is not supported by the target platform";
    this->runTest(TR::VectorLength256);
}
REGISTER_TYPED_TEST_CASE_P(ParameterizedUnaryVectorTest, Vector128, Vector256);
#else
REGISTER_TYPED_TEST_CASE_P(ParameterizedUnaryVectorTest, Vector128);
#endif

/**
 * @brief Configuration structure for parameterized tests.
 *
 * The configuration contains the element type and the operation for tests.
 *
 * @tparam T The element type
 * @tparam O The vector operation
 */
template <typename T, TR::VectorOperation O>
struct Config {
  using Type = T;
  static constexpr TR::VectorOperation operation = O;
};
// Helper macros to generate type lists for test instantiation.
// GET_INTEGRAL_CONFIGS: Creates configurations for integral types (int8_t, int16_t, int32_t, int64_t)
// GET_ALL_CONFIGS: Creates configurations for all types including floating-point (float, double)
#define GET_INTEGRAL_CONFIGS(opcode) ::testing::Types<Config<int8_t, opcode>, Config<int16_t, opcode>, Config<int32_t, opcode>, Config<int64_t, opcode>>
#define GET_ALL_CONFIGS(opcode) ::testing::Types<Config<int8_t, opcode>, Config<int16_t, opcode>, Config<int32_t, opcode>, Config<int64_t, opcode>, Config<float, opcode>, Config<double, opcode>>

// Instantiate parameterized tests for each vector operation with appropriate type configurations.
// Each operation is tested with all compatible element types across supported vector lengths.
typedef GET_ALL_CONFIGS(TR::vabs) vabsConfig;
INSTANTIATE_TYPED_TEST_CASE_P(VAbs, ParameterizedUnaryVectorTest, vabsConfig);

typedef GET_INTEGRAL_CONFIGS(TR::vnot) vnotConfig;
INSTANTIATE_TYPED_TEST_CASE_P(VNot, ParameterizedUnaryVectorTest, vnotConfig);

typedef GET_ALL_CONFIGS(TR::vneg) vnegConfig;
INSTANTIATE_TYPED_TEST_CASE_P(VNeg, ParameterizedUnaryVectorTest, vnegConfig);

typedef GET_INTEGRAL_CONFIGS(TR::vpopcnt) vpopcntConfig;
INSTANTIATE_TYPED_TEST_CASE_P(VPopulationCount, ParameterizedUnaryVectorTest, vpopcntConfig);

typedef GET_INTEGRAL_CONFIGS(TR::vnotz) vnotzConfig;
INSTANTIATE_TYPED_TEST_CASE_P(VTrailingZeroes, ParameterizedUnaryVectorTest, vnotzConfig);

typedef GET_INTEGRAL_CONFIGS(TR::vnolz) vnolzConfig;
INSTANTIATE_TYPED_TEST_CASE_P(VLeadingZeroes, ParameterizedUnaryVectorTest, vnolzConfig);