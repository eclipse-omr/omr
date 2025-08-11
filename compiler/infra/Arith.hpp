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

#ifndef ARITH_INCL
#define ARITH_INCL

#include <stdint.h>

namespace TR {

template<typename T> struct IsSignedIntegral {
    static const bool value = false;
};

template<> struct IsSignedIntegral<int8_t> {
    static const bool value = true;
};

template<> struct IsSignedIntegral<int16_t> {
    static const bool value = true;
};

template<> struct IsSignedIntegral<int32_t> {
    static const bool value = true;
};

template<> struct IsSignedIntegral<int64_t> {
    static const bool value = true;
};

// The following {add,sub}WithOverflow require that the build (C++) compiler
// use two's complement (virtually guaranteed), and that type_t be a signed
// integer type.

template<typename T> T addWithOverflow(T a, T b, bool &overflow)
{
    static_assert(IsSignedIntegral<T>::value, "expected a signed integral type");

    // Cast to uintmax_t to avoid undefined behaviour on signed overflow.
    //
    // The uintmax_t-typed operands are sign-extended from a and b. They
    // won't undergo integer promotions, because they're already as large as
    // possible. And they're already at a common type, so they won't undergo
    // the "usual arithmetic conversions" either. Therefore this will do
    // unsigned (modular) arithmitic. Converting back to type_t produces the
    // expected two's complement result.
    T sum = uintmax_t(a) + uintmax_t(b);

    // The overflow flag is set when the arithmetic used to compute the sum overflows. This happens exactly
    // when both operand have the same sign and the resulting value has the opposite sign to this.
    // A non-negative result from xoring two numbers indicates that their sign bits are either both
    // set, or both unset, meaning they have the same sign, while a negative result from xoring two
    // numbers means that exactly one of them has its sign bit set, meaning the signs of the two numbers
    // differ.
    overflow = (a ^ b) >= 0 && (a ^ sum) < 0;

    return sum;
}

template<typename T> T subWithOverflow(T a, T b, bool &overflow)
{
    static_assert(IsSignedIntegral<T>::value, "expected a signed integral type");

    // About uintmax_t, see addWithOverflow above.
    T diff = uintmax_t(a) - uintmax_t(b);

    // The overflow flag is set when the arithmetic used to compute the difference overflows. This happens exactly
    // when both operand bounds have a differing sign and the resulting value has the same sign as the first operand.
    // A non-negative result from xoring two numbers indicates that their sign bits are either both
    // set, or both unset, meaning they have the same sign, while a negative result from xoring two
    // numbers means that exactly one of them has its sign bit set, meaning the signs of the two numbers
    // differ.
    overflow = (a ^ b) < 0 && (a ^ diff) < 0;

    return diff;
}

template<typename T> void incWithOverflow(T &lhs, T rhs, bool &overflow) { lhs = addWithOverflow(lhs, rhs, overflow); }

template<typename T> void decWithOverflow(T &lhs, T rhs, bool &overflow) { lhs = subWithOverflow(lhs, rhs, overflow); }

// This makes it convenient to do multiple operations in a row and check only
// at the end whether any have overflowed.
struct StickyOverflowFlag {
    StickyOverflowFlag()
        : occurred(false)
    {}

    bool occurred;
};

template<typename T> T addWithOverflow(T a, T b, StickyOverflowFlag &overflow)
{
    bool sumOverflow = false;
    T result = addWithOverflow(a, b, sumOverflow);
    overflow.occurred |= sumOverflow;
    return result;
}

template<typename T> T subWithOverflow(T a, T b, StickyOverflowFlag &overflow)
{
    bool diffOverflow = false;
    T result = subWithOverflow(a, b, diffOverflow);
    overflow.occurred |= diffOverflow;
    return result;
}

template<typename T> void incWithOverflow(T &lhs, T rhs, StickyOverflowFlag &overflow)
{
    lhs = addWithOverflow(lhs, rhs, overflow);
}

template<typename T> void decWithOverflow(T &lhs, T rhs, StickyOverflowFlag &overflow)
{
    lhs = subWithOverflow(lhs, rhs, overflow);
}

} // namespace TR

#endif
