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

#ifndef OMR_LOGGER_INCL
#define OMR_LOGGER_INCL

#include <stdint.h>
#include "env/FilePointerDecl.hpp"

namespace OMR {

/**
 * @class Logger
 *
 * @brief
 *
 * A \b Logger is a simple structure for performing diagnostic I/O behind a
 * lightweight API. A \c OMR::Logger is an abstract base class that declares a
 * logging API from which families of useful Loggers can be implemented.
 *
 * A complete description of Loggers and their usage can be found in
 * \ref ../../doc/compiler/debug/Loggers.md "Loggers.md"
 *
 * The Logger API is documented herein.
 */

class Logger {
public:
    Logger();

    /**
     * @brief
     *     Send a `\0`-terminated string with format specifiers and arguments
     *     to the Logger. The format specifier follows C/C++ conventions.
     *
     * @return number of chars successfully sent to the Logger, or a negative
     *     number on any error
     */
    virtual int32_t printf(const char *format, ...) = 0;

    /**
     * @brief
     *     Send a `\0`-terminated string with format specifiers and arguments
     *     in a `va_list` to the Logger. The format specifier follows C/C++
     *     conventions.
     *
     * @return number of chars successfully sent to the Logger, or a negative
     *     number on any error
     */
    virtual int32_t vprintf(const char *format, va_list args) = 0;

    /**
     * @brief Send a raw `\0`-terminated string to the Logger.
     *
     * @return a nonnegative number on success, or a negative value on any error
     */
    virtual int32_t prints(const char *string) = 0;

    /**
     * @brief Send a single `char` to the Logger.
     *
     * @return a nonnegative number on success, or a negative value on any error
     */
    virtual int32_t printc(char c) = 0;

    /**
     * @brief
     *     Send a single newline to the Logger.
     *     This is equivalent to `printc('\n')`.
     *
     * @return a nonnegative number on success, or a negative value on any error
     */
    virtual int32_t println() = 0;

    /**
     * @brief Returns the current zero-based output position indicator for this Logger.
     *
     * @return a nonnegative number on success, or a negative value on any error
     */
    virtual int64_t tell() = 0;

    /**
     * @brief
     *     Resets the output position indicator for this Logger to its
     *     earliest possible position (effectively rewinding it to the beginning).
     *     Subsequent content will overwrite previously logged content. This is
     *     useful to implement a circular Logger. A Logger must indicate
     *     support for this feature via the `supportsRewinding()` API.
     */
    virtual void rewind() = 0;

    /**
     * @brief
     *     Forces the Logger to flush any accumulated content to its underlying
     *     media (for example, a file on disk, or a network socket).
     *
     * @return 0 on success; a negative value on any error
     */
    virtual int32_t flush() = 0;

    /**
     * @brief
     *     Closes the Logger from accepting content. The Logger is also
     *     flushed and disabled. Supplying content to a closed Logger is
     *     undefined. A Logger cannot be re-opened once closed.
     *
     * @return 0 on success; a negative value on any error
     */
    virtual int32_t close() = 0;

    /**
     * @brief
     *     Answers whether this Logger supports the ability to have its
     *     output position indicator changed to an earlier point. This is
     *     useful for implementing circular Loggers.
     */
    virtual bool supportsRewinding() = 0;

    /**
     * @brief
     *     Answers whether the Logger is enabled to accept logging output.
     *     API functions within an implementing Logger are not required to check
     *     whether logging is enabled when called (for efficiency reasons).
     *     Logging to a disabled Logger is therefore undefined.
     *
     * @details
     *     Logging enablement is a compatibility feature that is deprecated. It
     *     is a cleaner alternative to the current logging anti-pattern of
     *     checking for the mere presence of a log as the criteria to enable
     *     updates to the log. Logging should be guarded with some kind of trace
     *     enablement option so the user has complete control over what goes into
     *     a log. While most writes to a log are guarded in the current code
     *     base, for some updates that check for the mere presence of a log it is
     *     not clear what the appropriate tracing option should be. These
     *     anti-patterns are refactored to check log enablement via the
     *     `isEnabled_DEPRECATED()` API, but should be refactored to eliminate
     *     this practice entirely in favour of a tracing option.
     */
    bool isEnabled_DEPRECATED() { return _enabled; }

    void setEnabled_DEPRECATED(bool e) { _enabled = e; }

    /**
     * @brief
     *     Answers `true` when the underlying flushes that a Logger `flush()`
     *     function might do should be skipped. Answers `false` otherwise.
     *
     * @details
     *     There may be a performance advantage to avoiding the actual flush in
     *     some scenarios, providing the consequences of not performing an actual
     *     flush are understood.
     *
     *     This function would typically be called by `flush()` functions within
     *     Logger implementations.
     */
    bool getSkipFlush() { return _skipFlush; }

    void setSkipFlush(bool s) { _skipFlush = s; }

    /**
     * @brief
     *     Answers whether this Logger has been closed via a call to the `close`
     *     function. Directing output to a closed Logger is undefined. A closed
     *     Logger cannot be re-opened.
     */
    bool getLoggerClosed() { return _loggerClosed; }

    void setLoggerClosed(bool b) { _loggerClosed = b; }

private:
    bool _enabled;
    bool _skipFlush;
    bool _loggerClosed;
};

/**
 * A Logger class that simply consumes its input without outputting anything.
 * This is useful as the default Logger that prevents unguarded outputs to
 * the log.
 */
class NullLogger : public Logger {
public:
    static NullLogger *create();

    virtual int32_t printf(const char *format, ...) { return 0; }

    virtual int32_t prints(const char *string) { return 0; }

    virtual int32_t printc(char c) { return 0; }

    virtual int32_t println() { return 0; }

    virtual int32_t vprintf(const char *format, va_list args) { return 0; }

    virtual int64_t tell() { return 0; }

    virtual void rewind() {}

    virtual int32_t flush() { return 0; }

    virtual int32_t close();

    virtual bool supportsRewinding() { return false; }

private:
    NullLogger()
        : Logger()
    {}
};

/**
 * A Logger class that fatally asserts if any of the logging functions
 * is called. This is useful for test environments to detect unguarded
 * calls to Logger functions.
 */
class AssertingLogger : public Logger {
public:
    static AssertingLogger *create();

    virtual int32_t printf(const char *format, ...);

    virtual int32_t prints(const char *string);

    virtual int32_t printc(char c);

    virtual int32_t println();

    virtual int32_t vprintf(const char *format, va_list args);

    virtual int64_t tell();

    virtual void rewind();

    virtual int32_t flush();

    virtual int32_t close();

    virtual bool supportsRewinding() { return false; }

private:
    AssertingLogger()
        : Logger()
    {}
};

/**
 * A Logger class that implements logging using C standard IO functions.
 */
class CStdIOStreamLogger : public Logger {
public:
    static CStdIOStreamLogger *create(::FILE *stream);

    /**
     * @brief
     *     A convenience function to create a Logger by directing output to
     *     the given filename. The file will be created first if necessary or
     *     overwritten if it already exists. The file will be opened for
     *     writing.
     *
     * @param[in] filename : name of file to direct logging output to
     *
     * @return A CStdIOStreamLogger object if successful; NULL on any error
     */
    static CStdIOStreamLogger *create(const char *filename);

    ~CStdIOStreamLogger();

    virtual int32_t printf(const char *format, ...);

    virtual int32_t prints(const char *string);

    virtual int32_t printc(char c);

    virtual int32_t println();

    virtual int32_t vprintf(const char *format, va_list args);

    virtual int64_t tell();

    virtual void rewind();

    virtual int32_t flush();

    virtual int32_t close();

    virtual bool supportsRewinding() { return true; }

    /**
     * @brief
     *     Answers whether the underlying stream should be closed when the
     *     Logger is closed. Generally, this should only return true if
     *     the stream was first opened by this Logger.
     */
    bool getRequiresStreamClose() { return _requiresStreamClose; }

    void setRequiresStreamClose(bool b) { _requiresStreamClose = b; }

    ::FILE *getStream() { return _stream; }

    void setStream(::FILE *s) { _stream = s; }

    static CStdIOStreamLogger *Stderr;

    static CStdIOStreamLogger *Stdout;

private:
    CStdIOStreamLogger(::FILE *stream, bool requiresStreamClose = false);

    ::FILE *_stream;
    bool _requiresStreamClose;
};

/**
 * A Logger class that implements logging using TR IO functions.
 */
class TRIOStreamLogger : public Logger {
public:
    static TRIOStreamLogger *create(TR::FILE *stream);

    virtual int32_t printf(const char *format, ...);

    virtual int32_t prints(const char *string);

    virtual int32_t printc(char c);

    virtual int32_t println();

    virtual int32_t vprintf(const char *format, va_list args);

    virtual int64_t tell();

    virtual void rewind();

    virtual int32_t flush();

    virtual int32_t close();

    virtual bool supportsRewinding() { return true; }

    TR::FILE *getStream() { return _stream; }

    void setStream(TR::FILE *s) { _stream = s; }

private:
    TRIOStreamLogger(TR::FILE *stream);

    TR::FILE *_stream;
};

/**
 * A Logger class that implements circular logging functionality by rewinding
 * after a certain threshold of chars have been logged. Logging output is not
 * strictly bound to be within the size threshold but will be enforced on a
 * best-effort basis.
 */
class CircularLogger : public Logger {
public:
    /**
     * @brief The \c CircularLogger factory function
     *
     * @param[in] innerLogger : the underlying Logger managed with circular
     *     logging functionality. The inner Logger must already be created.
     *
     * @param[in] rewindThresholdInChars : the threshold number of chars after
     *     which the Logger will rewind and resuming logging. This is not a
     *     guaranteed threshold, but an aspirational one. Some chars may be written
     *     to the Logger past the threshold.
     *
     * @return A new \c CircularLogger object
     */
    static CircularLogger *create(OMR::Logger *innerLogger, int64_t rewindThresholdInChars);

    virtual int32_t printf(const char *format, ...);

    virtual int32_t prints(const char *string);

    virtual int32_t printc(char c);

    virtual int32_t println();

    virtual int32_t vprintf(const char *format, va_list args);

    virtual int64_t tell();

    virtual void rewind();

    virtual int32_t flush();

    virtual int32_t close();

    virtual bool supportsRewinding() { return true; }

    /**
     * @return The underlying Logger used within this CircularLogger framework
     */
    OMR::Logger *getInnerLogger() { return _innerLogger; }

    void setInnerLogger(OMR::Logger *il) { _innerLogger = il; }

    /**
     * @return The threshold number of chars after which the Logger will rewind
     *     and resume logging. This is not a guaranteed threshold, but an
     *     aspirational one. Some chars may be written to the Logger past the
     *     threshold.
     */
    int64_t getRewindThresholdInChars() { return _rewindThresholdInChars; }

    void setRewindThresholdInChars(int64_t r) { _rewindThresholdInChars = r; }

private:
    CircularLogger(OMR::Logger *innerLogger, int64_t rewindThresholdInChars);

    OMR::Logger *_innerLogger;
    int64_t _rewindThresholdInChars;
};

/**
 * A Logger class that performs logging to a memory buffer. The maximum size of the
 * buffer is fixed on Logger creation. The memory buffer is protected against buffer
 * overflows.
 *
 * All content logged is terminated with a '\0'-termination character.
 *
 * The underlying char buffer must be allocated before the Logger is created.
 */
class MemoryBufferLogger : public Logger {
public:
    /**
     * @brief The \c MemoryBufferLogger factory function
     *
     * @param[in] buf : a non-NULL pointer to an already allocated memory buffer
     * @param[in] maxBufLen : the maximum capacity of the buffer in chars. Note
     *     that the size of the buffer must include space for one '\0'-termination
     *     character.
     *
     * @return An allocated \c MemoryBufferLogger object
     */
    static MemoryBufferLogger *create(char *buf, size_t maxBufLen);

    /**
     * @anchor membuf_printf
     *
     * @brief
     *     Send a `\0`-terminated string with format specifiers and arguments
     *     to the Logger. The format specifier follows C/C++ conventions.
     *
     * @details
     *     If there is insufficient space in the buffer for the string to log
     *     then it may be partially written until the buffer is exhausted. The
     *     negative of the number of chars that would have been written had
     *     there been sufficent space is returned.
     *
     * @return
     *     If non-negative, the number of chars successfully sent to the Logger
     *
     *     If negative, there is insufficient space in the memory buffer for the
     *     string to log. The return value is the negative of the number of chars
     *     that would have been sent if the logging had been successful.
     *
     *     MIN_INT on any other error
     */
    virtual int32_t printf(const char *format, ...);

    /**
     * @brief Send a raw `\0`-terminated string to the Logger.
     *
     * @see membuf_printf for details and return value
     */
    virtual int32_t prints(const char *string);

    /**
     * @brief Send a single `char` to the Logger.
     *
     * @see membuf_printf for details and return value
     */
    virtual int32_t printc(char c);

    /**
     * @brief
     *     Send a single newline to the Logger.
     *     This is equivalent to `printc('\n')`.
     *
     * @see membuf_printf for details and return value
     */
    virtual int32_t println();

    /**
     * @brief
     *     Send a `\0`-terminated string with format specifiers and arguments
     *     in a `va_list` to the Logger. The format specifier follows C/C++
     *     conventions.
     *
     * @see membuf_printf for details and return value
     */
    virtual int32_t vprintf(const char *format, va_list args);

    virtual int64_t tell();

    virtual void rewind();

    virtual int32_t flush();

    virtual int32_t close();

    virtual bool supportsRewinding() { return true; }

private:
    MemoryBufferLogger(char *buf, size_t maxBufLen);

    char *_buf;
    char *_bufCursor;
    size_t _maxBufLen;
    size_t _maxRemainingChars;
};

} // namespace OMR

#endif
