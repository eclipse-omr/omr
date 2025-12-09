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

#ifndef OMR_COMPILER_ENV_INCL
#define OMR_COMPILER_ENV_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_COMPILER_ENV_CONNECTOR
#define OMR_COMPILER_ENV_CONNECTOR

namespace OMR {
class CompilerEnv;
typedef OMR::CompilerEnv CompilerEnvConnector;
} // namespace OMR
#endif

#include "infra/Annotations.hpp"
#include "env/RawAllocator.hpp"
#include "env/Environment.hpp"
#include "env/DebugEnv.hpp"
#include "env/PersistentAllocator.hpp"
#include "env/ClassEnv.hpp"
#include "env/ObjectModel.hpp"
#include "env/ArithEnv.hpp"
#include "env/VMEnv.hpp"
#include "env/VMMethodEnv.hpp"
#include "env/TRMemory.hpp"
#include "ras/Logger.hpp"

namespace TR {
class CompilerEnv;
}

/*
 * Message logger convenience macros
 */

/**
 * @brief Convenience macro to output a NUL-terminated string with format specifiers to
 *     the message Logger on the CompilerEnv object
 *
 * @param[in] fmt : NUL-terminated string to print with format specifiers
 * @param[in] ... : variable arguments
 */
#define mesg_printf(fmt, ...)                           \
    do {                                                \
        TR::Compiler->mesg->printf(fmt, ##__VA_ARGS__); \
    } while (0)

/**
 * @brief Convenience macro to output a NUL-terminated string to the message Logger on the
 *     CompilerEnv object
 *
 * @param[in] str : NUL-terminated string to print
 */
#define mesg_prints(str)                 \
    do {                                 \
        TR::Compiler->mesg->prints(str); \
    } while (0)

namespace OMR {

class OMR_EXTENSIBLE CompilerEnv {
public:
    CompilerEnv(TR::RawAllocator raw, const TR::PersistentAllocatorKit &persistentAllocatorKit,
        OMRPortLibrary * const omrPortLib = NULL);

    TR::CompilerEnv *self();

    /// Primordial raw allocator.  This is guaranteed to be thread safe.
    ///
    TR::RawAllocator rawAllocator;

    // Compilation host environment
    //
    TR::Environment host;

    // Compilation target environment
    //
    TR::Environment target;

    // Compilation relocatable target environment
    //
    TR::Environment relocatableTarget;

    // Class information in this compilation environment.
    //
    TR::ClassEnv cls;

    // Information about the VM environment.
    //
    TR::VMEnv vm;

    // Information about methods in this compilation environment
    //
    TR::VMMethodEnv mtd;

    // Object model in this compilation environment
    //
    TR::ObjectModel om;

    // Arithmetic semantics in this compilation environment
    //
    TR::ArithEnv arith;

    // Debug environment for the compiler.  This is not thread safe.
    //
    TR::DebugEnv debug;

    bool isInitialized() { return _initialized; }

    // --------------------------------------------------------------------------

    // Initialize a CompilerEnv.  This should only be executed after a CompilerEnv
    // object has been allocated and its constructor run.  Its intent is to
    // execute initialization logic that may require a completely initialized
    // object beforehand.
    //
    void initialize();

    TR::PersistentAllocator &persistentAllocator() { return _persistentAllocator; }

    TR_PersistentMemory *persistentMemory() { return ::trPersistentMemory; }

    OMRPortLibrary * const omrPortLib;

    /**
     * Logger for compiler messages. This Logger should be used instead of writing informational
     * messages about exceptional situations directly to stderr. The message Logger can be changed
     * from the default if a different destination for compiler messages is desired.
     *
     * Convenience macros `mesg_printf` and `mesg_prints` have been provided to simplify
     * access to the message Logger.
     */
    OMR::Logger *mesg;

protected:
    // Initialize message Logger for this compiler
    //
    void initializeMessageLogger();

    // Initialize 'target' environment for this compiler
    //
    void initializeTargetEnvironment();

    // Initialize 'relocatableTarget' environment for this compiler
    //
    void initializeRelocatableTargetEnvironment();

    // Initialize 'host' environment for this compiler
    //
    void initializeHostEnvironment();

private:
    bool _initialized;

    TR::PersistentAllocator _persistentAllocator;

public:
    /// Compiler-lifetime region allocator.
    /// NOTE: its a raw allocator until the region allocation work is completed.
    ///
    TR::PersistentAllocator &regionAllocator;
};

} // namespace OMR

#endif
