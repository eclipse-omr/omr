/*******************************************************************************
 * Copyright IBM Corp. and others 2021
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

#include "infra/String.hpp"
#include "infra/Assert.hpp"
#include "env/FrontEnd.hpp" // defines va_copy for old MSVC and z/OS
#include <limits.h>

#if defined(_MSC_VER) && (_MSC_VER < 1900)
#define stdlib_vsnprintf _vsnprintf
#else
#define stdlib_vsnprintf vsnprintf
#endif

namespace TR {

// Returns the length of the formatted string without writing it to memory.
int vprintfLen(const char *fmt, va_list args)
{
    // This works with both standard vsnprintf() and _vsnprintf().
    return stdlib_vsnprintf(NULL, 0, fmt, args);
}

// Returns the length of the formatted string without writing it to memory.
int printfLen(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = TR::vprintfLen(fmt, args);
    va_end(args);
    return len;
}

// Returns true if the formatted string was truncated. The length of the
// formatted string (excluding the NUL terminator) is stored into len. That
// length is (size - 1) if the output was truncated, because when using
// _vsnprintf() the would-be length is unknown.
bool vsnprintfTrunc(char *buf, size_t size, int *len, const char *fmt, va_list args)
{
    // This assertion guarantees that (size - 1) will not overflow in the
    // truncation case below, and that there is actually a location in which to
    // store the terminating NUL. The only reason to pass size=0 would be to
    // compute the formatted length, which should be done using
    // TR::[v]printfLen() instead.
    TR_ASSERT_FATAL(size > 0, "vsnprintfTrunc: no buffer space provided");

    // This assertion guarantees that it's safe to truncate (size - 1) to int in
    // the string truncation case below.
    TR_ASSERT_FATAL(size - 1 <= (size_t)INT_MAX, "vsnprintfTrunc: buffer too large");

    int n = stdlib_vsnprintf(buf, size, fmt, args);

    // The formatted length does not include the NUL, so if (n >= size), the
    // result has been truncated. If we are using _vsnprintf(), it's possible to
    // get (n == size), which indicates that the entire formatted string has
    // been stored *without* a trailing NUL, but not (n > size). In the case
    // where standard vsnprintf() produces (n > size), _vsnprintf() returns a
    // negative value instead. So a negative result also indicates truncation.
    bool truncated = n < 0 || n >= size;
    if (truncated) {
        buf[size - 1] = '\0'; // Ensure NUL-termination in the _vsnprintf() case.
        *len = (int)(size - 1);
    } else {
        *len = n;
    }

    return truncated;
}

// See vsnprintfTrunc() above.
bool snprintfTrunc(char *buf, size_t size, int *len, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool truncated = TR::vsnprintfTrunc(buf, size, len, fmt, args);
    va_end(args);
    return truncated;
}

// Variant of vsnprintfTrunc() without the len pointer, for cases where the
// caller is uninterested in the length.
bool vsnprintfTrunc(char *buf, size_t size, const char *fmt, va_list args)
{
    int dummy;
    return vsnprintfTrunc(buf, size, &dummy, fmt, args);
}

// Variant of snprintfTrunc() without the len pointer, for cases where the
// caller is uninterested in the length.
bool snprintfTrunc(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool truncated = TR::vsnprintfTrunc(buf, size, fmt, args);
    va_end(args);
    return truncated;
}

// Asserts that the formatted string was not truncated and returns its length
// (excluding the NUL terminator).
int vsnprintfNoTrunc(char *buf, size_t size, const char *fmt, va_list args)
{
    int len;
    bool truncated = TR::vsnprintfTrunc(buf, size, &len, fmt, args);
    TR_ASSERT_FATAL(!truncated, "vsnprintfNoTrunc: truncation occurred");
    return len;
}

// Asserts that the formatted string was not truncated and returns its length
// (excluding the NUL terminator).
int snprintfNoTrunc(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = TR::vsnprintfNoTrunc(buf, size, fmt, args);
    va_end(args);
    return len;
}

void TR::StringBuf::vappendf(const char *fmt, va_list args)
{
    va_list argsCopy;

    va_copy(argsCopy, args);
    int appendLen = TR::vprintfLen(fmt, argsCopy);
    va_copy_end(argsCopy);

    TR_ASSERT_FATAL(appendLen >= 0, "error in format string");

    size_t newLen = _len + appendLen;
    ensureCapacity(newLen);

    TR_ASSERT_FATAL(appendLen + 1 <= _cap - _len, "insufficient buffer capacity");
    int realAppendLen = stdlib_vsnprintf(&_text[_len], appendLen + 1, fmt, args);
    TR_ASSERT_FATAL(realAppendLen == appendLen, "incorrect predicted snprintf length");
    TR_ASSERT_FATAL(_text[newLen] == '\0', "missing NUL terminator");

    _len = newLen;
}

void TR::StringBuf::appendf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vappendf(fmt, args);
    va_end(args);
}

void TR::StringBuf::ensureCapacity(size_t newLen)
{
    // Note that the capacity must always be strictly greater than the length so
    // that there is room for the NUL terminator.
    if (newLen < _cap)
        return; // nothing to do

    size_t newCap = newLen + 1; // + 1 for NUL terminator

    // Grow the buffer geometrically each time it is reallocated to ensure that
    // appends are amortized O(1) per character
    if (newCap < 2 * _cap)
        newCap = 2 * _cap;

    char *newText = (char *)_region.allocate(newCap);
    memcpy(newText, _text, _len + 1); // + 1 for NUL terminator
    _text = newText;
    _cap = newCap;
}

} // namespace TR
