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

#ifndef TR_STRING_INCL
#define TR_STRING_INCL

#include <stdarg.h>
#include <stddef.h>
#include "env/defines.h"
#include "env/TRMemory.hpp"
#include "infra/Uncopyable.hpp"

#if HOST_COMPILER == COMPILER_GCC || HOST_COMPILER == COMPILER_CLANG
#define TR_PRINTF_FORMAT_ATTR(fmtIndex, argsIndex) __attribute__((format(printf, (fmtIndex), (argsIndex))))
#else
#define TR_PRINTF_FORMAT_ATTR(fmtIndex, argsIndex)
#endif

namespace TR {

int vprintfLen(const char *fmt, va_list args);

int printfLen(const char *fmt, ...) TR_PRINTF_FORMAT_ATTR(1, 2);

bool vsnprintfTrunc(char *buf, size_t size, int *len, const char *fmt, va_list args);

bool snprintfTrunc(char *buf, size_t size, int *len, const char *fmt, ...) TR_PRINTF_FORMAT_ATTR(4, 5);

bool vsnprintfTrunc(char *buf, size_t size, const char *fmt, va_list args);

bool snprintfTrunc(char *buf, size_t size, const char *fmt, ...) TR_PRINTF_FORMAT_ATTR(3, 4);

int vsnprintfNoTrunc(char *buf, size_t size, const char *fmt, va_list args);

int snprintfNoTrunc(char *buf, size_t size, const char *fmt, ...) TR_PRINTF_FORMAT_ATTR(3, 4);

class StringBuf : private TR::Uncopyable {
    TR::Region &_region;
    size_t _cap;
    size_t _len;
    char *_text;

    // can't use a delegating constructor
    void initEmptyBuffer()
    {
        TR_ASSERT_FATAL(_cap > 0, "StringBuf: no buffer space");
        TR_ASSERT_FATAL(_text != NULL, "StringBuf: buffer is null");
        _text[0] = '\0';
    }

public:
    // region, initialBuffer must both outlive this StringBuffer
    StringBuf(TR::Region &region, char *initialBuffer, size_t capacity)
        : _region(region)
        , _cap(capacity)
        , _len(0)
        , _text(initialBuffer)
    {
        initEmptyBuffer();
    }

    // region must outlive this StringBuffer
    StringBuf(TR::Region &region, size_t capacity = 32)
        : _region(region)
        , _cap(capacity)
        , _len(0)
        , _text((char *)region.allocate(capacity))
    {
        initEmptyBuffer();
    }

    size_t len() const { return _len; }

    const char *text() const { return _text; }

    void clear()
    {
        _len = 0;
        _text[0] = '\0';
    }

    void vappendf(const char *fmt, va_list args);

    void appendf(const char *fmt, ...) TR_PRINTF_FORMAT_ATTR(2, 3);

private:
    void ensureCapacity(size_t newLen);
};

} // namespace TR

#endif
