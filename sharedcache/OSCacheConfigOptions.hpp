/*******************************************************************************
 * Copyright (c) 2001, 2019 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/
#if !defined(OS_CACHE_CONFIG_OPTIONS_HPP_INCLUDED)
#define OS_CACHE_CONFIG_OPTIONS_HPP_INCLUDED

#include "omr.h"

class OSCacheConfigOptions
{
public:
    typedef enum StartupReason {
        StartupReason_Destroy,
        StartupReason_Expired,
        StartupReason_Open,
        StartupReason_Stat,
        StartupReason_EnsureWideOpen = 0x1000000
    } StartupReason;

    typedef enum StatReason {
        StatReason_Destroy,
        StatReason_Expired,
        StatReason_GatherStatistics,
        StatReason_Iterate,
        StatReason_List,
        StatReason_EnsureWideEnum = 0x1000000
    } StatReason;

protected:
    I_32 _openMode;
    StartupReason _startupReason;

public:
    OSCacheConfigOptions(I_32 openMode)
        : _openMode(openMode)
        , _startupReason(StartupReason::StartupReason_Open)
    {}

    virtual ~OSCacheConfigOptions() {}

    virtual bool useUserHomeDirectoryForCacheDir() = 0;
    virtual bool isUserSpecifiedCacheDir() = 0;
    virtual bool readOnlyOpenMode() = 0;

    virtual I_32 fileMode();
    virtual I_32 openMode() = 0;
    virtual UDATA groupPermissions() = 0;

    virtual IDATA cacheDirPermissions() = 0;
    virtual bool usingNetworkCache() = 0;

    /* TODO: the restore check only applies to the shared memory cache,
       so we should probably create an OSSharedMemoryCacheConfigOptions
       subclass, and put it there.. then OSSharedMemoryCache will own a
       reference to an OSSharedMemoryCacheConfigOptions object. */
    virtual bool restoreCheckEnabled() = 0;
    virtual bool openButDoNotCreate() = 0;

    virtual bool openToDestroyExistingCache() = 0;
    virtual bool openToDestroyExpiredCache() = 0;
    virtual bool openToStatExistingCache() = 0;

    /* reasons for stats. should have a StatReason enum. */
    virtual bool statToDestroy() = 0;
    virtual bool statExpired() = 0;
    virtual bool statIterate() = 0;
    virtual bool statList() = 0;

    virtual OSCacheConfigOptions& setAppendBaseDir(bool appendBaseDir) = 0;

    virtual bool semaphoreCheckEnabled() = 0;

    virtual OSCacheConfigOptions& setStatReason(StatReason reason) = 0;
    virtual OSCacheConfigOptions& setOpenReason(StartupReason reason) = 0;
    virtual OSCacheConfigOptions& setReadOnlyOpenMode() = 0;
    virtual OSCacheConfigOptions& setOpenMode(I_32 openMode) = 0;

    /* the block size of the cache. */
    virtual U_32 cacheSize() = 0;
    virtual OSCacheConfigOptions& setCacheSize(U_32 size) = 0;

    virtual U_32 maxCRCSamples() = 0;

    /* does the cache create a file? */
    virtual bool createFile() = 0;

    /* do we try to open the cache read-only if we failed to open the cache with write permissions? */
    virtual bool tryReadOnlyOnOpenFailure() = 0;

    /* when the cache is corrupt, do we dump its contents? */
    virtual bool disableCorruptCacheDumps() = 0;

    virtual bool verboseEnabled() = 0;
    virtual bool groupAccessEnabled() = 0; // true iff _groupPerm = 1 in the J9 cache.

    /* render the options to a bit vector understood by the functions of the OMR port library. */
    virtual U_32 renderToFlags() = 0;

    virtual UDATA renderCreateOptionsToFlags() = 0;
    virtual UDATA renderVerboseOptionsToFlags() = 0;
    virtual void resetVerboseOptionsFromFlags(UDATA flags) = 0;
};
#endif
