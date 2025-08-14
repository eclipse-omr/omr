<!--
Copyright IBM Corp. and others 2025

This program and the accompanying materials are made available under
the terms of the Eclipse Public License 2.0 which accompanies this
distribution and is available at https://www.eclipse.org/legal/epl-2.0/
or the Apache License, Version 2.0 which accompanies this distribution and
is available at https://www.apache.org/licenses/LICENSE-2.0.

This Source Code may also be made available under the following
Secondary Licenses when the conditions for such availability set
forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
General Public License, version 2 with the GNU Classpath
Exception [1] and GNU General Public License, version 2 with the
OpenJDK Assembly Exception [2].

[1] https://www.gnu.org/software/classpath/license.html
[2] https://openjdk.org/legal/assembly-exception.html

SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
-->

# Loggers

A *Logger* is a simple structure for performing diagnostic I/O behind a
lightweight API. There are several advantages to using a logging mechanism:

- a Logger provides a consistent API to be used for all diagnostic output.
  Historical Testarossa implementations employed multiple solutions
  simultaneously with no standardization or consistency.

- it will be easier to make broad changes in the future affecting all
  diagnostic I/O if a single API is used. For example, introducing log levels
  controlling diagnostic behaviour commonly used in logging infrastructure
  found in other projects.

- Loggers enable creative debugging practices. For example, a Logger need not
  write to a file but be streamed out a socket instead, and it is
  straightforward to capture diagnostic output at different parts of the
  compilation process to different Loggers.

- a Logger hides its implementation details behind a consistent, abstract API.
  For example, a Logger could be implemented to use C stdio or Testarossa's
  `TR::IO` for I/O, but the API for the developer remains the same (only the
  installed Logger implementation is different).

A `OMR::Logger` is an abstract base class that declares a logging API. A family
of useful Logger implementations is provided in the OMR compiler, but it is
straightfoward for a developer to extend or create custom Loggers.

## Logger API Functions

The Logger API has a family of output functions available for use depending on
the circumstance. For instance, not all output requires format specifiers, and
in those circumstances API functions are available to output a constant string
or even a single character. Such variants are more efficient because they are
not scanned for format specifiers prior to output. Developers should choose the
most appropriate and efficient function for their output needs.

## Logger Implementations

### NullLogger

This is the simplest Logger that simply eats anything passed into it without
outputting it anywhere. This is useful to install as a default Logger to catch
any unguarded Logger output in a production setting. Knowing that some Logger
will always be available means that code does not always need to check for the
presence of a Logger before outputting.

### CStdIOStreamLogger

A Logger that outputs directly to a file using C stdio output functions.

### TRIOStreamLogger

A Logger that outputs directly to a file using Testarossa `TR::IO` output
functions.

### CircularLogger

A CircularLogger implements circular logging functionality by rewinding its
output after a certain threshold of chars have been logged. It is essentially a
wrapper around another rewindable Logger supplied when the CircularLogger is
created that constrains its output.

This is not a perfect circular Logger where the maximum output size of the
underlying Logger will never be exceeded. Instead, it makes a reasonable effort
to keep the logging output within a certain character threshold.

### MemoryBufferLogger

A MemoryBufferLogger logs its output to a provided fixed-size memory buffer.
Logging output is always guaranteed to fall within the bounds of the buffer,
with truncation occurring as necessary. The memory buffer will be
'\0'-terminated and the memory buffer must be sized to accommodate the
termination character.

### AssertingLogger

An AssertingLogger is a Logger where each API output function fatal asserts if
it is called. It is useful in test modes to catch instances of unguarded Logger
output.

## Using Loggers

### Logger Availability

To ensure versatility, all diagnostic functions should accept a Logger
parameter for the logging destination. By convention, this should be passed as
the first parameter. There may be circumstances where a Logger is not passed,
but only in situations where the logging destination can be obviously derived
from the context.

Stated differently, diagnostic functions should not make an assumption that
their output will go to the Logger found on the `TR::Compilation` object. This
makes it difficult to support creative debugging techniques (such as dumping a
data structure to an alternate file or simply stdio) because the destination is
fixed.

Nevertheless, a valid Logger will always be available on the `TR::Compilation`
object to catch diagnostic output. The default Logger is the `OMR::NullLogger`
which simply eats anything passed to it. This differs from the historical
Testarossa design where a log file is only created when specified by the user
on the command-line, leading to defensive code that repeatedly confirmed the
existence of a log file throughout the code. Such checks no longer apply,
however, all diagnastic output should be guarded with a trace option of some
sort. The default Logger is replaced with a more appropriate Logger depending
on the diagnostic circumstances. Downstream projects may use a
`OMR::AssertingLogger` as part of their testing to catch unguarded use of
logging functions.

### Guarding Diagnostic Logging

All diagnostic output should be guarded with a trace option that controls its
output. Controlling diagnostic output by checking for the presence of a log
file is a deprecated anti-pattern.

A common pattern for guarding a single output function is the following:
```
   if (myTraceOption)
      {
      log->printf("My output: %d\n", myInt);
      }
```
To improve code readability and density, convenience macros have been introduced
for the "print" family of functions that should be used in lieu of the pattern
above. For example, the code above is equivalent to:
```
   logprintf(myTraceOption, log, "My output: %d\n", myInt);
```
If multiple output functions share a single guard then the macros should not be
used.

### Logger Enablement

A Logger has two deprecated functions that control whether a Logger is prepared to
accept logging output:
```
bool isEnabled_DEPRECATED();
void setEnabled_DEPRECATED(bool e);
```
Logging enablement is a compatibility feature that provides a cleaner
alternative to the historical logging anti-pattern of checking for the mere
presence of a log file as the criteria to enable updates to the log. The new
requirement to guard all diagnostic output with a trace option that can be
enabled/disabled was challenging to implement in some areas of the OMR code and
known downstream projects without additional thought and re-design. Rather than
delay the rollout of the Logger feature while these corner cases were studied
and refactored, the enablement API was introduced and applied sparingly as a
bridge.

Because these APIs are deprecated, additional uses of them in the code are not
permitted. They will be removed when proper refactoring of the relevant parts
of code is completed.
