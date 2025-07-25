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

#ifndef THREADLOCAL_INCL
#define THREADLOCAL_INCL

#include <errno.h>
#include "infra/Assert.hpp"

#if defined(OMRZTPF) && !defined(SUPPORTS_THREAD_LOCAL)
/*
 * For this include file, ThreadLocal.hpp, and the z/TPF OS platform,
 * say that tls is supported. The z/TPF platform should use
 * pthread _key_t variables and pthread functions for
 * the tls* functions that are defined here.
 */
#define SUPPORTS_THREAD_LOCAL
#endif

#if defined(SUPPORTS_THREAD_LOCAL)
#if defined(OMR_OS_WINDOWS)
#include "windows.h"

#define TR_TLS_DECLARE(type, variable) extern DWORD variable

#define TR_TLS_DEFINE(type, variable) DWORD variable = TLS_OUT_OF_INDEXES

#define TR_TLS_ALLOC(variable)                                                                                       \
    {                                                                                                                \
        (variable) = TlsAlloc();                                                                                     \
        TR_ASSERT_FATAL((variable) != TLS_OUT_OF_INDEXES, "TlsAlloc failed with GetLastError = %d", GetLastError()); \
    }

#define TR_TLS_FREE(variable)                                                              \
    {                                                                                      \
        DWORD rc = TlsFree(variable);                                                      \
        TR_ASSERT_FATAL(rc != 0, "TlsFree failed with GetLastError = %d", GetLastError()); \
    }

#define TR_TLS_SET(variable, value)                                                            \
    {                                                                                          \
        BOOL rc = TlsSetValue((variable), value);                                              \
        TR_ASSERT_FATAL(rc != 0, "TlsSetValue failed with GetLastError = %d", GetLastError()); \
    }

#define TR_TLS_GET(variable, type) ((type)TlsGetValue(variable))
#elif (defined(LINUX) && !defined(OMRZTPF)) || defined(OSX) || defined(AIXPPC)
#define TR_TLS_DECLARE(type, variable) extern __thread type variable

#define TR_TLS_DEFINE(type, variable) __thread type variable = NULL

#define TR_TLS_ALLOC(variable)
#define TR_TLS_FREE(variable)

#define TR_TLS_SET(variable, value) variable = value

#define TR_TLS_GET(variable, type) (variable)
#elif defined(OMRZTPF) || defined(J9ZOS390)
#include <pthread.h>

#define TR_TLS_DECLARE(type, variable) extern pthread_key_t variable

#define TR_TLS_DEFINE(type, variable) pthread_key_t variable

#define TR_TLS_ALLOC(variable)                                                                                    \
    {                                                                                                             \
        int rc = pthread_key_create(&variable, NULL);                                                             \
        TR_ASSERT_FATAL(rc == 0, "pthread_key_create failed with rc = %d, errorno = %d, message = %s", rc, errno, \
            strerror(errno));                                                                                     \
    }

#define TR_TLS_FREE(variable)                                                                                     \
    {                                                                                                             \
        int rc = pthread_key_delete(variable);                                                                    \
        TR_ASSERT_FATAL(rc == 0, "pthread_key_delete failed with rc = %d, errorno = %d, message = %s", rc, errno, \
            strerror(errno));                                                                                     \
    }

#define TR_TLS_SET(variable, value)                                                                                \
    {                                                                                                              \
        int rc = pthread_setspecific(variable, value);                                                             \
        TR_ASSERT_FATAL(rc == 0, "pthread_setspecific failed with rc = %d, errorno = %d, message = %s", rc, errno, \
            strerror(errno));                                                                                      \
    }

#if defined(J9ZOS390)
#define TR_TLS_GET(variable, type) ((type)pthread_getspecific_d8_np(variable))
#else
#define TR_TLS_GET(variable, type) ((type)pthread_getspecific(variable))
#endif
#endif
#else /* !defined(SUPPORTS_THREAD_LOCAL) */
#define TR_TLS_DECLARE(type, variable) extern type variable

#define TR_TLS_DEFINE(type, variable) type variable = NULL

#define TR_TLS_ALLOC(variable)
#define TR_TLS_FREE(variable)

#define TR_TLS_SET(variable, value) variable = value

#define TR_TLS_GET(variable, type) (variable)
#endif
#endif /* THREADLOCAL_INCL */
