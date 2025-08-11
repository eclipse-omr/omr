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

#include "env/VerboseLog.hpp"
#include "infra/Assert.hpp"

void *TR_VerboseLog::_config = 0;

const char *TR_VerboseLog::_vlogTable[]
    = { 0, "#INFO:  ", "#SAMPLE:  ", "#IPROFILER:  ", "#CODECACHE:  ", "#JITSTATE:  ", "#PERF:  ", "#MEMORY:  ",
          "#FAILURE:  ", "#JITDUMP:  ", "#SCHINTS:  ", "#GCCYCLE:  ", "#J2I:  ", "#CR:  ", "#RA:  ", "#HK:  ", "#HD:  ",
          "#MH:   ", // extra space to line up with MHd
          "#MHd:  ", "#DLT:  ",
          "+ ", // TR_Vlog_COMP
          " ", // TR_Vlog_COMPSTART
          "! ", // TR_Vlog_COMPFAIL
          "-precompileMethod ",
          " ", // TR_Vlog_MMAP
          "#OSR:  ", // extra space to line up with OSRd
          "#OSRd: ", "#HWP: ", "#INL: ", "#GC: ",
          "[IBM GPU JIT]: ", // to be consistent with JCL GPU output
          "#PATCH : ", "#DISPATCH: ", "#RECLAMATION: ", "#PROFILING: ", "#JITServer: ", "#AOTCOMPRESSION: ",
          "#BenefitInliner: ", "#FSD: ", "#VECTOR API: ", "#CHECKPOINT RESTORE: ", "#METHOD STATS: " };

void TR_VerboseLog::writeLine(TR_VlogTag tag, const char *format, ...)
{
    TR_ASSERT(tag != TR_Vlog_null, "TR_Vlog_null is not a valid Vlog tag");
    va_list args;
    va_start(args, format);
    writeTimeStamp();
    write(_vlogTable[tag]);
    vwrite(format, args);
    write("\n");
    va_end(args);
}

void TR_VerboseLog::writeLine(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vwrite(format, args);
    write("\n");
    va_end(args);
}

void TR_VerboseLog::writeLineLocked(TR_VlogTag tag, const char *format, ...)
{
    TR_ASSERT(tag != TR_Vlog_null, "TR_Vlog_null is not a valid Vlog tag");
    CriticalSection lock;
    va_list args;
    va_start(args, format);
    writeTimeStamp();
    write(_vlogTable[tag]);
    vwrite(format, args);
    write("\n");
    va_end(args);
}

void TR_VerboseLog::write(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vwrite(format, args);
    va_end(args);
}

void TR_VerboseLog::write(TR_VlogTag tag, const char *format, ...)
{
    TR_ASSERT_FATAL(tag != TR_Vlog_null, "TR_Vlog_null is not a valid Vlog tag");
    va_list args;
    va_start(args, format);
    writeTimeStamp();
    write(_vlogTable[tag]);
    vwrite(format, args);
    va_end(args);
}

void TR_VerboseLog::initialize(void *config) { _config = config; }

// This is never called. It's just a place to put static_assert() where
// definitions in this file are available (for e.g. array initializers) and
// where private members of TR_VerboseLog are also visible.
void TR_VerboseLog::privateStaticAsserts()
{
    static_assert(sizeof(_vlogTable) / sizeof(_vlogTable[0]) == TR_Vlog_numTags,
        "TR_VlogTag and _vlogTable are out of sync");
}
