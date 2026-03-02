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

#include "VectorTestUtils.hpp"

int32_t TRTest::vectorSize(TR::VectorLength vl) {
    switch (vl) {
        case TR::VectorLength64:
            return 8;
        case TR::VectorLength128:
            return 16;
        case TR::VectorLength256:
            return 32;
        case TR::VectorLength512:
            return 64;
        default:
            TR_ASSERT_FATAL(0, "Illegal vector length");
            return 0;
    }
}

int32_t TRTest::typeSize(TR::DataTypes dt) {
    switch (dt) {
        case TR::Int8:
            return 1;
        case TR::Int16:
            return 2;
        case TR::Int32:
            return 4;
        case TR::Int64:
            return 8;
        case TR::Float:
            return 4;
        case TR::Double:
            return 8;
        default:
            TR_ASSERT_FATAL(0, "Illegal data type");
            return 0;
    }
}

void TRTest::compareResults(void *expected, void *actual, TR::DataTypes dt, TR::VectorLength vl, bool isReduction) {
    int elementBytes = typeSize(dt);
    int lengthBytes = isReduction ? elementBytes : vectorSize(vl);

    for (int i = 0; i < lengthBytes; i += elementBytes) {
        switch (dt) {
            case TR::Int8:
                EXPECT_EQ(*((int8_t *) expected), *((int8_t *) actual));
                break;
            case TR::Int16:
                EXPECT_EQ(*((int16_t *) expected), *((int16_t *) actual));
                break;
            case TR::Int32:
                EXPECT_EQ(*((int32_t *) expected), *((int32_t *) actual));
                break;
            case TR::Int64:
                EXPECT_EQ(*((int64_t *) expected), *((int64_t *) actual));
                break;
            case TR::Float:
                if (std::isnan(*((float *) expected)))
                    EXPECT_TRUE(std::isnan(*((float *) actual))) << "Expected NaN but got " << *((float *) actual);
                else
                    EXPECT_FLOAT_EQ(*((float *) expected), *((float *) actual));
                break;
            case TR::Double:
                if (std::isnan(*((double *) expected)))
                    EXPECT_TRUE(std::isnan(*((double *) actual))) << "Expected NaN but got " << *((double *) actual);
                else
                    EXPECT_DOUBLE_EQ(*((double *) expected), *((double *) actual));
                break;
            default:
                TR_ASSERT_FATAL(0, "Illegal type to compare");
                break;
        }
        expected = static_cast<char *>(expected) + elementBytes;
        actual = static_cast<char *>(actual) + elementBytes;
    }
}

void TRTest::generateByType(void *output, TR::DataType dt, bool nonZero) {
    switch (dt) {
        case TR::Int8:
            *((int8_t *) output) = -128 + static_cast<int8_t>(rand() % 255);
            if (nonZero && *((int8_t *) output) == 0) *((int8_t *) output) = 1;
            break;
        case TR::Int16:
            *((int16_t *) output) = -200 + static_cast<int16_t>(rand() % 400);
            if (nonZero && *((int16_t *) output) == 0) *((int16_t *) output) = 1;
            break;
        case TR::Int32:
            *((int32_t *) output) = -1000 + static_cast<int32_t>(rand() % 2000);
            if (nonZero && *((int32_t *) output) == 0) *((int32_t *) output) = 1;
            break;
        case TR::Int64:
            *((int64_t *) output) = -1000 + static_cast<int64_t>(rand() % 2000);
            if (nonZero && *((int64_t *) output) == 0) *((int64_t *) output) = 1;
            break;
        case TR::Float:
            *((float *) output) = static_cast<float>(rand() / 1000.0);
            break;
        case TR::Double:
            *((double *) output) = static_cast<double>(rand() / 1000.0);
            break;
    }
}
