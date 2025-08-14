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

OMR::Logger::Logger()
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

OMR::NullLogger *OMR::NullLogger::create() { return new OMR::NullLogger(); }

int32_t OMR::NullLogger::close()
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

OMR::AssertingLogger *OMR::AssertingLogger::create() { return new OMR::AssertingLogger(); }

int32_t OMR::AssertingLogger::printf(const char *format, ...)
{
    TR_ASSERT_FATAL(false, "Unexpected Logger printf with format string: %s", format);
    return -1;
}

int32_t OMR::AssertingLogger::prints(const char *string)
{
    TR_ASSERT_FATAL(false, "Unexpected Logger prints with string: %s", string);
    return -1;
}

int32_t OMR::AssertingLogger::printc(char c)
{
    TR_ASSERT_FATAL(false, "Unexpected Logger printc with char: %c (%02x)", c, c);
    return -1;
}

int32_t OMR::AssertingLogger::println()
{
    TR_ASSERT_FATAL(false, "Unexpected Logger println");
    return -1;
}

int32_t OMR::AssertingLogger::vprintf(const char *format, va_list args)
{
    TR_ASSERT_FATAL(false, "Unexpected Logger vprintf with format string: %s", format);
    return -1;
}

int64_t OMR::AssertingLogger::tell()
{
    TR_ASSERT_FATAL(false, "Unexpected Logger tell");
    return -1;
}

void OMR::AssertingLogger::rewind() { TR_ASSERT_FATAL(false, "Unexpected Logger rewind"); }

int32_t OMR::AssertingLogger::flush()
{
    TR_ASSERT_FATAL(false, "Unexpected Logger flush");
    return -1;
}

int32_t OMR::AssertingLogger::close()
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
OMR::CStdIOStreamLogger::CStdIOStreamLogger(::FILE *stream, bool requiresStreamClose)
    : _stream(stream)
    , _requiresStreamClose(requiresStreamClose)
{}

OMR::CStdIOStreamLogger *OMR::CStdIOStreamLogger::create(::FILE *stream) { return new OMR::CStdIOStreamLogger(stream); }

OMR::CStdIOStreamLogger *OMR::CStdIOStreamLogger::create(const char *filename)
{
    ::FILE *fd = fopen(filename, "w");
    if (!fd) {
        // Error opening/creating the Logger file
        return NULL;
    }

    return new OMR::CStdIOStreamLogger(fd, true);
}

OMR::CStdIOStreamLogger::~CStdIOStreamLogger()
{
    if (!getLoggerClosed()) {
        this->close();
    }
}

int32_t OMR::CStdIOStreamLogger::printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int32_t length = ::vfprintf(getStream(), format, args);
    va_end(args);
    return length;
}

int32_t OMR::CStdIOStreamLogger::prints(const char *str) { return ::fputs(str, getStream()); }

int32_t OMR::CStdIOStreamLogger::printc(char c) { return ::fputc(c, getStream()); }

int32_t OMR::CStdIOStreamLogger::println() { return ::fputc('\n', getStream()); }

int32_t OMR::CStdIOStreamLogger::vprintf(const char *format, va_list args)
{
    return ::vfprintf(getStream(), format, args);
}

int64_t OMR::CStdIOStreamLogger::tell() { return ::ftell(getStream()); }

int32_t OMR::CStdIOStreamLogger::flush()
{
    if (!getSkipFlush())
        return ::fflush(getStream());

    return 0;
}

void OMR::CStdIOStreamLogger::rewind() { ::fseek(getStream(), 0, SEEK_SET); }

int32_t OMR::CStdIOStreamLogger::close()
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

OMR::CStdIOStreamLogger *OMR::CStdIOStreamLogger::Stderr = OMR::CStdIOStreamLogger::create(stderr);

OMR::CStdIOStreamLogger *OMR::CStdIOStreamLogger::Stdout = OMR::CStdIOStreamLogger::create(stdout);

/*
 * -----------------------------------------------------------------------------
 * TRIOStreamLogger
 * -----------------------------------------------------------------------------
 */
OMR::TRIOStreamLogger::TRIOStreamLogger(TR::FILE *stream)
    : _stream(stream)
{}

OMR::TRIOStreamLogger *OMR::TRIOStreamLogger::create(TR::FILE *stream) { return new OMR::TRIOStreamLogger(stream); }

int32_t OMR::TRIOStreamLogger::printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int32_t length = TR::IO::vfprintf(getStream(), format, args);
    va_end(args);
    return length;
}

int32_t OMR::TRIOStreamLogger::prints(const char *str) { return TR::IO::fprintf(getStream(), "%s", str); }

int32_t OMR::TRIOStreamLogger::printc(char c) { return TR::IO::fprintf(getStream(), "%c", c); }

int32_t OMR::TRIOStreamLogger::println() { return TR::IO::fprintf(getStream(), "\n"); }

int32_t OMR::TRIOStreamLogger::vprintf(const char *format, va_list args)
{
    return TR::IO::vfprintf(getStream(), format, args);
}

int64_t OMR::TRIOStreamLogger::tell() { return TR::IO::ftell(getStream()); }

int32_t OMR::TRIOStreamLogger::flush()
{
    if (!getSkipFlush()) {
        // TR::IO::fflush does not have error checking
        TR::IO::fflush(getStream());
    }

    return 0;
}

void OMR::TRIOStreamLogger::rewind() { TR::IO::fseek(getStream(), 0, SEEK_SET); }

int32_t OMR::TRIOStreamLogger::close()
{
    setLoggerClosed(true);

    // Do not close the stream as the Logger is simply a wrapper around it.
    // Just disable the Logger and flush it.
    //
    setEnabled_DEPRECATED(false);
    return this->flush();
}

