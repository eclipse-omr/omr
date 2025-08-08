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

#ifndef ANYCONST_INCL
#define ANYCONST_INCL

#include "env/KnownObjectTable.hpp"
#include "infra/Assert.hpp"
#include <stdint.h>

namespace TR {

/**
 * \brief A value of arbitrary type that could be the result of a node.
 *
 * The value is constant just in the sense that it is known at compile time.
 */
class AnyConst {
    enum Kind {
        KindInt8,
        KindInt16,
        KindInt32,
        KindInt64,
        KindFloat,
        KindDouble,
        KindAddress,
        KindKnownObject,
    };

    Kind _kind;

    union {
        float _float;
        double _double;
        uint64_t _integral; // for everything else
    };

    AnyConst(Kind kind, uint64_t x)
        : _kind(kind)
        , _integral(x)
    {}

    AnyConst(float x)
        : _kind(KindFloat)
        , _float(x)
    {}

    AnyConst(double x)
        : _kind(KindDouble)
        , _double(x)
    {}

public:
    static AnyConst makeInt8(uint8_t x) { return AnyConst(KindInt8, x); }

    static AnyConst makeInt16(uint16_t x) { return AnyConst(KindInt16, x); }

    static AnyConst makeInt32(uint32_t x) { return AnyConst(KindInt32, x); }

    static AnyConst makeInt64(uint64_t x) { return AnyConst(KindInt64, x); }

    static AnyConst makeFloat(float x) { return AnyConst(x); }

    static AnyConst makeDouble(double x) { return AnyConst(x); }

    // Address: mostly for null, but can be used for pointers to data structures
    // that don't move and for object references while VM access is held.
    static AnyConst makeAddress(uintptr_t x) { return AnyConst(KindAddress, x); }

    static AnyConst makeKnownObject(TR::KnownObjectTable::Index i)
    {
        TR_ASSERT_FATAL(i != TR::KnownObjectTable::UNKNOWN, "should be known");
        return AnyConst(KindKnownObject, i);
    }

    bool isInt8() const { return _kind == KindInt8; }

    bool isInt16() const { return _kind == KindInt16; }

    bool isInt32() const { return _kind == KindInt32; }

    bool isInt64() const { return _kind == KindInt64; }

    bool isFloat() const { return _kind == KindFloat; }

    bool isDouble() const { return _kind == KindDouble; }

    bool isAddress() const { return _kind == KindAddress; }

    bool isKnownObject() const { return _kind == KindKnownObject; }

    uint8_t getInt8() const
    {
        kindAssert(KindInt8);
        return (uint8_t)_integral;
    }

    uint16_t getInt16() const
    {
        kindAssert(KindInt16);
        return (uint16_t)_integral;
    }

    uint32_t getInt32() const
    {
        kindAssert(KindInt32);
        return (uint32_t)_integral;
    }

    uint64_t getInt64() const
    {
        kindAssert(KindInt64);
        return (uint64_t)_integral;
    }

    float getFloat() const
    {
        kindAssert(KindFloat);
        return _float;
    }

    double getDouble() const
    {
        kindAssert(KindDouble);
        return _double;
    }

    uintptr_t getAddress() const
    {
        kindAssert(KindAddress);
        return (uintptr_t)_integral;
    }

    TR::KnownObjectTable::Index getKnownObjectIndex() const
    {
        kindAssert(KindKnownObject);
        return (TR::KnownObjectTable::Index)_integral;
    }

private:
    void kindAssert(Kind expected) const { TR_ASSERT_FATAL(_kind == expected, "type mismatch"); }
};

} // namespace TR

#endif // ANYCONST_INCL
