/*******************************************************************************
 * Copyright (c) 2023, 2023 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
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
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef LINUX
#include <unistd.h>
#endif
#include "env/FrontEnd.hpp"
#include "env/IO.hpp"
#include "infra/Assert.hpp"
#include "ras/Logger.hpp"

TR::Logger::Logger()
{
    // The creator will decide when this Logger is ready to accept content
    //
    setEnabled_DEPRECATED(false);

    setLoggerClosed(false);

    static char *envSkipLoggerFlushes = feGetEnv("TR_skipLoggerFlushes");
    setSkipFlush(envSkipLoggerFlushes ? true : false);
}

/*
 * -----------------------------------------------------------------------------
 * NullLogger
 * -----------------------------------------------------------------------------
 */

TR::NullLogger *TR::NullLogger::create() { return new TR::NullLogger(); }

int32_t TR::NullLogger::close()
{
    setLoggerClosed(true);
    setEnabled_DEPRECATED(false);
    return this->flush();
}

/*
 * -----------------------------------------------------------------------------
 * AssertingLogger
 * -----------------------------------------------------------------------------
 */

TR::AssertingLogger *TR::AssertingLogger::create() { return new TR::AssertingLogger(); }

int32_t TR::AssertingLogger::printf(const char *format, ...)
{
    TR_ASSERT_FATAL(false, "Unexpected Logger printf");
    return -1;
}

int32_t TR::AssertingLogger::prints(const char *string)
{
    TR_ASSERT_FATAL(false, "Unexpected Logger prints");
    return -1;
}

int32_t TR::AssertingLogger::printc(char c)
{
    TR_ASSERT_FATAL(false, "Unexpected Logger printc");
    return -1;
}

int32_t TR::AssertingLogger::println()
{
    TR_ASSERT_FATAL(false, "Unexpected Logger println");
    return -1;
}

int32_t TR::AssertingLogger::vprintf(const char *format, va_list args)
{
    TR_ASSERT_FATAL(false, "Unexpected Logger vprintf");
    return -1;
}

int64_t TR::AssertingLogger::tell()
{
    TR_ASSERT_FATAL(false, "Unexpected Logger tell");
    return -1;
}

void TR::AssertingLogger::rewind() { TR_ASSERT_FATAL(false, "Unexpected Logger rewind"); }

int32_t TR::AssertingLogger::flush()
{
    TR_ASSERT_FATAL(false, "Unexpected Logger flush");
    return -1;
}

int32_t TR::AssertingLogger::close()
{
    TR_ASSERT_FATAL(false, "Unexpected Logger close");
    setLoggerClosed(true);
    setEnabled_DEPRECATED(false);
    return -1;
}

/*
 * -----------------------------------------------------------------------------
 * CStdIOStreamLogger
 * -----------------------------------------------------------------------------
 */
TR::CStdIOStreamLogger::CStdIOStreamLogger(::FILE *stream, bool requiresStreamClose)
    : _stream(stream)
    , _requiresStreamClose(requiresStreamClose)
{}

TR::CStdIOStreamLogger *TR::CStdIOStreamLogger::create(::FILE *stream) { return new TR::CStdIOStreamLogger(stream); }

TR::CStdIOStreamLogger *TR::CStdIOStreamLogger::create(const char *filename)
{
    ::FILE *fd = fopen(filename, "w");
    if (!fd) {
        // Error opening/creating the Logger file
        return NULL;
    }

    return new TR::CStdIOStreamLogger(fd, true);
}

TR::CStdIOStreamLogger::~CStdIOStreamLogger()
{
    if (!getLoggerClosed()) {
        this->close();
    }
}

int32_t TR::CStdIOStreamLogger::printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int32_t length = ::vfprintf(getStream(), format, args);
    va_end(args);
    return length;
}

int32_t TR::CStdIOStreamLogger::prints(const char *str) { return ::fputs(str, getStream()); }

int32_t TR::CStdIOStreamLogger::printc(char c) { return ::fputc(c, getStream()); }

int32_t TR::CStdIOStreamLogger::println() { return ::fputc('\n', getStream()); }

int32_t TR::CStdIOStreamLogger::vprintf(const char *format, va_list args)
{
    return ::vfprintf(getStream(), format, args);
}

int64_t TR::CStdIOStreamLogger::tell() { return ::ftell(getStream()); }

int32_t TR::CStdIOStreamLogger::flush()
{
    if (!getSkipFlush())
        return ::fflush(getStream());

    return 0;
}

void TR::CStdIOStreamLogger::rewind() { ::fseek(getStream(), 0, SEEK_SET); }

int32_t TR::CStdIOStreamLogger::close()
{
    setLoggerClosed(true);

    // Disable the Logger
    //
    setEnabled_DEPRECATED(false);

    int32_t result = this->flush();
    if (result != 0) {
        return result;
    }

    // result must be 0 at this point

    if (getRequiresStreamClose()) {
        // Indicate that the stream was closed (or at least attempted to close)
        // to prevent further attempts
        //
        setRequiresStreamClose(false);

        result = ::fclose(getStream());
    }

    return result;
}

TR::CStdIOStreamLogger *TR::CStdIOStreamLogger::Stderr = TR::CStdIOStreamLogger::create(stderr);

TR::CStdIOStreamLogger *TR::CStdIOStreamLogger::Stdout = TR::CStdIOStreamLogger::create(stdout);

/*
 * -----------------------------------------------------------------------------
 * TRIOStreamLogger
 * -----------------------------------------------------------------------------
 */
TR::TRIOStreamLogger::TRIOStreamLogger(TR::FILE *stream)
    : _stream(stream)
{}

TR::TRIOStreamLogger *TR::TRIOStreamLogger::create(TR::FILE *stream) { return new TR::TRIOStreamLogger(stream); }

int32_t TR::TRIOStreamLogger::printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int32_t length = TR::IO::vfprintf(getStream(), format, args);
    va_end(args);
    return length;
}

int32_t TR::TRIOStreamLogger::prints(const char *str) { return TR::IO::fprintf(getStream(), "%s", str); }

int32_t TR::TRIOStreamLogger::printc(char c) { return TR::IO::fprintf(getStream(), "%c", c); }

int32_t TR::TRIOStreamLogger::println() { return TR::IO::fprintf(getStream(), "\n"); }

int32_t TR::TRIOStreamLogger::vprintf(const char *format, va_list args)
{
    return TR::IO::vfprintf(getStream(), format, args);
}

int64_t TR::TRIOStreamLogger::tell() { return TR::IO::ftell(getStream()); }

int32_t TR::TRIOStreamLogger::flush()
{
    if (!getSkipFlush()) {
        // TR::IO::fflush does not have error checking
        TR::IO::fflush(getStream());
    }

    return 0;
}

void TR::TRIOStreamLogger::rewind() { TR::IO::fseek(getStream(), 0, SEEK_SET); }

int32_t TR::TRIOStreamLogger::close()
{
    setLoggerClosed(true);

    // Do not close the stream as the Logger is simply a wrapper around it.
    // Just disable the Logger and flush it.
    //
    setEnabled_DEPRECATED(false);
    return this->flush();
}

/*
 * -----------------------------------------------------------------------------
 * CircularLogger
 * -----------------------------------------------------------------------------
 */
TR::CircularLogger::CircularLogger(TR::Logger *innerLogger, int64_t rewindThresholdInChars)
    : _innerLogger(innerLogger)
    , _rewindThresholdInChars(rewindThresholdInChars)
{
    TR_ASSERT_FATAL(innerLogger->supportsRewinding(),
        "Inner logger must support rewinding for use in a circular logger");
    TR_ASSERT_FATAL(rewindThresholdInChars > 0, "Circular log threshold must be a non-zero, positive integer");
}

TR::CircularLogger *TR::CircularLogger::create(TR::Logger *innerLogger, int64_t rewindThresholdInChars)
{
    return new TR::CircularLogger(innerLogger, rewindThresholdInChars);
}

int32_t TR::CircularLogger::printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (getInnerLogger()->tell() > getRewindThresholdInChars()) {
        getInnerLogger()->rewind();
    }

    int32_t length = getInnerLogger()->vprintf(format, args);
    va_end(args);
    return length;
}

int32_t TR::CircularLogger::prints(const char *str)
{
    if (getInnerLogger()->tell() > getRewindThresholdInChars()) {
        getInnerLogger()->rewind();
    }

    return getInnerLogger()->prints(str);
}

int32_t TR::CircularLogger::printc(char c)
{
    if (getInnerLogger()->tell() > getRewindThresholdInChars()) {
        getInnerLogger()->rewind();
    }

    return getInnerLogger()->printc(c);
}

int32_t TR::CircularLogger::println()
{
    if (getInnerLogger()->tell() > getRewindThresholdInChars()) {
        getInnerLogger()->rewind();
    }

    return getInnerLogger()->println();
}

int32_t TR::CircularLogger::vprintf(const char *format, va_list args)
{
    if (getInnerLogger()->tell() > getRewindThresholdInChars()) {
        getInnerLogger()->rewind();
    }

    return getInnerLogger()->vprintf(format, args);
}

int64_t TR::CircularLogger::tell() { return getInnerLogger()->tell(); }

int32_t TR::CircularLogger::flush() { return getInnerLogger()->flush(); }

void TR::CircularLogger::rewind() { getInnerLogger()->rewind(); }

int32_t TR::CircularLogger::close()
{
    setLoggerClosed(true);
    setEnabled_DEPRECATED(false);
    return getInnerLogger()->close();
}

/*
 * -----------------------------------------------------------------------------
 * MemoryBufferLogger
 * -----------------------------------------------------------------------------
 */

TR::MemoryBufferLogger *TR::MemoryBufferLogger::create(char *buf, size_t maxBufLen)
{
    return new TR::MemoryBufferLogger(buf, maxBufLen);
}

TR::MemoryBufferLogger::MemoryBufferLogger(char *buf, size_t maxBufLen)
    : _buf(buf)
    , _bufCursor(buf)
    , _maxBufLen(maxBufLen)
    , _maxRemainingChars(maxBufLen)
{}

int32_t TR::MemoryBufferLogger::printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int32_t result = TR::MemoryBufferLogger::vprintf(format, args);
    va_end(args);

    return result;
}

int32_t TR::MemoryBufferLogger::prints(const char *string) { return TR::MemoryBufferLogger::printf("%s", string); }

int32_t TR::MemoryBufferLogger::printc(char c) { return TR::MemoryBufferLogger::printf("%c", c); }

int32_t TR::MemoryBufferLogger::println() { return TR::MemoryBufferLogger::printf("\n"); }

int32_t TR::MemoryBufferLogger::vprintf(const char *format, va_list args)
{
    int32_t charsWritten = ::vsnprintf(_bufCursor, _maxRemainingChars, format, args);

    if (charsWritten >= 0) {
        if (charsWritten < _maxRemainingChars) {
            _bufCursor += charsWritten;
            _maxRemainingChars -= charsWritten;
            return charsWritten;
        } else {
            // Buffer would overflow, but only max allowable chars were written
            //
            _bufCursor += _maxRemainingChars;
            _maxRemainingChars = 0;
            return -charsWritten;
        }
    } else {
        // Non-overflow error
        //
        return INT_MIN;
    }
}

int64_t TR::MemoryBufferLogger::tell() { return static_cast<int64_t>(_bufCursor - _buf); }

void TR::MemoryBufferLogger::rewind()
{
    _bufCursor = _buf;
    _maxRemainingChars = _maxBufLen;
}

int32_t TR::MemoryBufferLogger::flush() { return 0; }

int32_t TR::MemoryBufferLogger::close()
{
    setLoggerClosed(true);
    setEnabled_DEPRECATED(false);
    return this->flush();
}
