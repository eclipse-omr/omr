/*******************************************************************************
 * Copyright IBM Corp. and others 1991
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
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

/**
 * @file sysinfo_helpers.c
 * @ingroup Port
 * @brief Contains implementation of methods to retrieve memory and processor info details.
 */

#include <errno.h>
#include <sys/__wlm.h>

#if defined(J9ZOS390) && !defined(OMR_EBCDIC)
#include "atoe.h"
#endif
#include "omrport.h"
#include "omrportpriv.h"
#include "omrsimap.h"
#include "omrzfs.h"
#include "omrsysinfo_helpers.h"
#include "ut_omrport.h"


/* Reduce general CP capacity to reflect Java’s preference for zIIP execution.
 * Java workloads are zIIP-eligible and will consume zIIP capacity ahead of
 * general CP capacity when available. Since zIIP usage is typically preferred
 * for Java workloads, we scale down the effective GCP capacity accordingly.
 */
#define GCP_SCALING_FACTOR 0.1

/* Apply an SMT-2 gain factor when estimating effective CPU capacity.
 * Because sibling threads share execution resources, effective capacity gains
 * are workload-dependent. A gain factor of 1.3 is used as a reasonable average.
 */
#define SMT_2_GAIN_FACTOR 1.3

#define IIP_HONOR_PRIORITY_ENABLED_MASK 0x2
#define MIN_CPU_UTILIZATION_SAMPLING_INTERVAL_NS 10000000LL

/* Function retrieves and populates memory usage statistics on a z/OS platform. */
int32_t
retrieveZOSMemoryStats(struct OMRPortLibrary *portLibrary, struct J9MemoryInfo *memInfo)
{
	uint64_t userCacheUsed = 0;
	uint64_t bufferCacheSize = 0;
	int32_t rc = 0;
	/* CVT address should be obtained in the following way to avoid Zero-Address Detection (ZAD)
	 * event (see JTCJAZZ: 60569)
	 */
	J9CVT * __ptr32 cvtp = ((J9PSA *)0)->flccvt;
	J9RCE * __ptr32 rcep = cvtp->cvtrcep;
	J9ASMVT * __ptr32 asmvtp = cvtp->cvtasmvt;

	Trc_PRT_retrieveZOSMemoryStats_Entered();
	Assert_PRT_true(NULL != memInfo);
	memInfo->totalPhysical = ((uint64_t)rcep->rcepool * J9BYTES_PER_PAGE);
	memInfo->availPhysical = ((uint64_t)rcep->rceafc * J9BYTES_PER_PAGE);
	memInfo->totalSwap = ((uint64_t)asmvtp->asmslots * J9BYTES_PER_PAGE);
	memInfo->availSwap = ((uint64_t)(asmvtp->asmslots - (asmvtp->asmvsc + asmvtp->asmnvsc
									 + asmvtp->asmerrs)) * J9BYTES_PER_PAGE);
	rc = getZFSUserCacheUsed(&userCacheUsed);
	if (0 == rc) {
		memInfo->cached = userCacheUsed;
	}

	rc = getZFSMetaCacheSize(&bufferCacheSize);
	if (0 == rc) {
		memInfo->buffered = bufferCacheSize;
	}

	memInfo->hostAvailPhysical = memInfo->availPhysical;
	memInfo->hostCached = memInfo->cached;
	memInfo->hostBuffered = memInfo->buffered;

	Trc_PRT_retrieveZOSMemoryStats_memUsageStats_v1(
		memInfo->totalPhysical,
		memInfo->availPhysical,
		memInfo->totalSwap,
		memInfo->availSwap,
		memInfo->cached,
		memInfo->buffered);

	Trc_PRT_retrieveZOSMemoryStats_Exit(0);
	return 0;
}

/* Function retrieves and populates processor usage statistics on a z/OS platform. */
int32_t
retrieveZOSProcessorStats(struct OMRPortLibrary *portLibrary, struct J9ProcessorInfos *procInfo)
{
	/* CVT address should be obtained in the following way to avoid Zero-Address Detection (ZAD)
	 * event (see JTCJAZZ: 60569)
	 */
	J9CVT * __ptr32 cvtp = ((J9PSA *)0)->flccvt;
	J9RMCT * __ptr32 rmctp = cvtp->cvtopctp;
	J9CCT * __ptr32 cctp = rmctp->rmctcct;

	Trc_PRT_retrieveZOSProcessorStats_Entered();
	Assert_PRT_true(NULL != procInfo);
	/* The only valid processor times that z/OS exposes are the total Busy and Idle times. */
	procInfo->procInfoArray[0].idleTime = cctp->ccvrbswt;
	procInfo->procInfoArray[0].busyTime = (cctp->ccvrbstd * cctp->ccvcpuct) - cctp->ccvrbswt;

	/* Get zIIP utilization. */
	procInfo->procInfoArray[0].avgZiipUtilization = cctp->ccvutils;
	procInfo->procInfoArray[0].combinedAvgCpuUtilization = cctp->ccvutila;

	Trc_PRT_retrieveZOSProcessorStats_cpuUsageStats_v1(procInfo->procInfoArray[0].idleTime, procInfo->procInfoArray[0].busyTime);

	Trc_PRT_retrieveZOSProcessorStats_Exit(0);
	return 0;
}

double
retrieveZosCpuLoad(struct OMRPortLibrary *portLibrary)
{
	/* Read ccvutils. */
	J9CVT * __ptr32 cvtp = ((J9PSA *)0)->flccvt;
	J9RMCT * __ptr32 rcmtp = cvtp->cvtopctp;
	J9CCT * __ptr32 cctp = rcmtp->rmctcct;

	int32_t ziipCount = retrieveZosOnlineZiipCount();
	double cpuLoad = 0.0;
	if (ziipCount > 0) {
		/* Read zIIP load. */
		cpuLoad = cctp->ccvutils / 100.0;
	} else {
		/* Read GCP load. */
		cpuLoad = cctp->ccvutilp / 100.0;
	}
	return cpuLoad;
}

double
calculateCombinedZosCpuLoad(struct OMRPortLibrary *portLibrary)
{
	/* Read ccvutils. */
	J9CVT * __ptr32 cvtp = ((J9PSA *)0)->flccvt;
	J9RMCT * __ptr32 rcmtp = cvtp->cvtopctp;
	J9CCT * __ptr32 cctp = rcmtp->rmctcct;

	double ziipCapacity = retrieveZosZiipCapacity();
	double gcpCapacity = retrieveZosOnlineGcpCount();
	double ziipLoad = 0.0;
	double gcpLoad = 0.0;

	if (gcpCapacity <= 0.0) {
		/* Something went wrong, we must have at least 1 GCP. */
		return -1.0;
	} else if (ziipCapacity > 0.0) {
		/* We only need to account for GCPs if IIPHONORPRIORITY is set. */
		if (isIipHonorPrioritySet(portLibrary)) {
			gcpCapacity *= GCP_SCALING_FACTOR;
			gcpLoad = cctp->ccvutilp;
		} else {
			/* Java workload won't be dispatched to GCPs. So we don't need to
			 * include GCPs in CPU load calculation.
			 */
			gcpCapacity = 0.0;
		}
		ziipLoad = cctp->ccvutils;
	} else {
		/* We don't have any zIIPs, only GCPs. */
		gcpLoad = cctp->ccvutilp;
	}
	return ((ziipLoad * ziipCapacity) + (gcpLoad * gcpCapacity))
			/ (100.00 * (ziipCapacity + gcpCapacity));
}

BOOLEAN
isIipHonorPrioritySet(struct OMRPortLibrary *portLibrary)
{
	J9CVT * __ptr32 cvtp = ((J9PSA *)0)->flccvt;
	char disableCheck[2] = {0};
	if ((0 == omrsysinfo_get_env(portLibrary, "DISABLE_IIPHONORPRIORITY_CHECK", disableCheck, 2))
		&& ('1' == disableCheck[0])
	) {
		return FALSE;
	}
	/* Check if IIPHONORPRIORITY flag is set. */
	return OMR_ARE_ANY_BITS_SET(cvtp->cvtsvt->svtIFAFlags, IIP_HONOR_PRIORITY_ENABLED_MASK);
}

BOOLEAN
isSMTEnabled(void)
{
	J9CVT * __ptr32 cvtp = ((J9PSA *)0)->flccvt;
	return (0 != cvtp->cvtopctp->rmctirarmctz->rmctz_mt_ziip) ? TRUE : FALSE;
}

double
retrieveZosZiipCapacity(void)
{
	double ziipCount = retrieveZosOnlineZiipCount();
	if (isSMTEnabled()) {
		ziipCount *= SMT_2_GAIN_FACTOR;
	}
	return ziipCount;
}

int32_t
retrieveZosOnlineZiipCount(void)
{
	J9CVT * __ptr32 cvtp = ((J9PSA *)0)->flccvt;
	J9CSD * __ptr32 cvtcsd = cvtp->cvtcsd;

	int32_t ziipCount = cvtcsd->online_zIIP_count;
	/* Adjust for SMT-2 if enabled. If SMT-2 is enabled,
	 * online_zIIP_count already accounts for SMT-2 by returning
	 * double the actual number of physical zIIP engines.
	 */
	if (isSMTEnabled()) {
		ziipCount = (ziipCount + 1) / 2;
	}
	return ziipCount;
}

int32_t
retrieveZosOnlineGcpCount(void)
{
	J9CVT * __ptr32 cvtp = ((J9PSA *)0)->flccvt;
	J9CSD * __ptr32 cvtcsd = cvtp->cvtcsd;
	return cvtcsd->online_gcp_count;
}

int32_t
retrieveZosOnlineCpuCount(void)
{
	J9CVT * __ptr32 cvtp = ((J9PSA *)0)->flccvt;
	J9CSD * __ptr32 cvtcsd = cvtp->cvtcsd;
	return cvtcsd->online_combined_CPUs;
}

double
retrieveZosCpuCapacity(struct OMRPortLibrary *portLibrary)
{
	double ziipCapacity = retrieveZosZiipCapacity();
	double gcpCount = retrieveZosOnlineGcpCount();
	double totalCpuCapacity = 0.0;

	if (gcpCount <= 0.0) {
		/* Something went wrong, we must have at least 1 GCP. */
		return -1.0;
	} else if (ziipCapacity > 0.0) {
		/* We only need to account for GCPs if IIPHONORPRIORITY is set. */
		if (isIipHonorPrioritySet(portLibrary)) {
			totalCpuCapacity = gcpCount * GCP_SCALING_FACTOR;
		}
		totalCpuCapacity += ziipCapacity;
	} else {
		/* We don't have any zIIPs, return online GCP count. */
		totalCpuCapacity = gcpCount;
	}
	return totalCpuCapacity;
}

double
retrieveSvtZiipNormalizationFactor(void)
{
	J9CVT * __ptr32 cvtp = ((J9PSA *)0)->flccvt;
	return 256.0 / cvtp->cvtsvt->svtSupNormalization;
}

int32_t
calculateZosProcessCpuUtilization(struct OMRPortLibrary *portLibrary, struct CpuUsageStats *usageStats)
{
	/* Logic from https://github.com/eclipse-omr/omr/blob/fe5de1e9f3f14d76d758873a4e17e311d77ea99a/port/unix/omrsysinfo.c#L4727-L4764
	 * has been modified for z/OS.
	 */
	double numberOfCpus = 0.0;
	J9SysinfoCPUTime currentCpuTime = {0};
	J9SysinfoCPUTime *oldestCpuTime = NULL;
	J9SysinfoCPUTime *latestCpuTime = NULL;
	omrthread_process_time_t processTime = {INT64_MAX, INT64_MAX};

	if ((NULL == portLibrary) || (NULL == usageStats)) {
		return OMRPORT_ERROR_INVALID_ARGUMENTS;
	}
	if (omrthread_get_process_times(&processTime) < 0) {
		return OMRPORT_ERROR_OPFAILED;
	}

	/* J9SysinfoCPUTime expects an integer CPU count, but on z/OS the scaled
	 * value is not guaranteed to be a whole number. Which is why we query
	 * it every time instead of caching it in the J9SysinfoCPUTime.
	 */
	numberOfCpus = retrieveZosCpuCapacity(portLibrary);
	if (numberOfCpus <= 0.0) {
		return OMRPORT_ERROR_SYSINFO_ERROR_GETTING_CPU_CAPACITY;
	}

	oldestCpuTime = &(portLibrary->portGlobals->oldestCPUTime);
	latestCpuTime = &(portLibrary->portGlobals->latestCPUTime);
	currentCpuTime.cpuTime = processTime._systemTime + processTime._userTime;
	currentCpuTime.userTime = processTime._userTime;
	currentCpuTime.systemTime = processTime._systemTime;
	currentCpuTime.timestamp = portLibrary->time_nano_time(portLibrary);

	if (0 == oldestCpuTime->timestamp) {
		*oldestCpuTime = currentCpuTime;
		*latestCpuTime = currentCpuTime;
		return OMRPORT_ERROR_INSUFFICIENT_DATA;
	}

	/* Calculate using the most recent value in the history. */
	if ((currentCpuTime.timestamp - latestCpuTime->timestamp) >= MIN_CPU_UTILIZATION_SAMPLING_INTERVAL_NS) {
		usageStats->perProcessUtilization = OMR_MIN(
				(currentCpuTime.cpuTime - latestCpuTime->cpuTime)
						/ (numberOfCpus * (currentCpuTime.timestamp - latestCpuTime->timestamp)),
				1.0);
		if (usageStats->perProcessUtilization >= 0.0) {
			*oldestCpuTime = *latestCpuTime;
			*latestCpuTime = currentCpuTime;

			/* Write old and latest CPU time to return struct. */
			usageStats->oldestCpuTime = *oldestCpuTime;
			usageStats->latestCpuTime = currentCpuTime;
			return 0;
		} else {
			/* Either the latest or the current time are bogus, so discard the latest value and try with the oldest value. */
			*latestCpuTime = currentCpuTime;
		}
	}

	if ((currentCpuTime.timestamp - oldestCpuTime->timestamp) >= MIN_CPU_UTILIZATION_SAMPLING_INTERVAL_NS) {
		usageStats->perProcessUtilization = OMR_MIN(
				(currentCpuTime.cpuTime - oldestCpuTime->cpuTime)
						/ (numberOfCpus * (currentCpuTime.timestamp - oldestCpuTime->timestamp)),
				1.0);
		if (usageStats->perProcessUtilization >= 0.0) {
			/* Write old and latest CPU time to return struct. */
			usageStats->oldestCpuTime = *oldestCpuTime;
			usageStats->latestCpuTime = currentCpuTime;
			return 0;
		} else {
			*oldestCpuTime = currentCpuTime;
		}
	}
	return OMRPORT_ERROR_OPFAILED;
}

int32_t
retrieveZosCpuUsageStats(struct OMRPortLibrary *portLibrary, struct CpuUsageStats *usageStats)
{
	/* Read ccvutils. */
	J9CVT * __ptr32 cvtp = ((J9PSA *)0)->flccvt;
	J9RMCT * __ptr32 rcmtp = cvtp->cvtopctp;
	J9CCT * __ptr32 cctp = rcmtp->rmctcct;

	if ((NULL == portLibrary) || (NULL == usageStats)) {
		return OMRPORT_ERROR_INVALID_ARGUMENTS;
	}

	usageStats->onlineZiipCount = retrieveZosOnlineZiipCount();
	usageStats->ziipCapacity = retrieveZosZiipCapacity();
	usageStats->onlineGcpCount = retrieveZosOnlineGcpCount();

	usageStats->onlineCpuCount = retrieveZosOnlineCpuCount();
	usageStats->cpuCapacity = retrieveZosCpuCapacity(portLibrary);
	usageStats->threadsPerZiipCore = rcmtp->rmctirarmctz->rmctz_mt_ziip;

	/* Normalization factor to be used to convert time spent on zIIP to equivalent time on a CP. */
	usageStats->svtNormalizationFactor = retrieveSvtZiipNormalizationFactor();

	/* Check if IIPHONORPRIORITY is enabled for zIIPs. */
	usageStats->isIipHonorPrioritySet = isIipHonorPrioritySet(portLibrary);
	usageStats->svtIFAFlags = cvtp->cvtsvt->svtIFAFlags;

	usageStats->gcpLoad = cctp->ccvutilp / 100.0;
	usageStats->ziipLoad = cctp->ccvutils / 100.0;
	usageStats->combinedLoad = cctp->ccvutila / 100.0;

	/* Combined overall CPU load. */
	usageStats->calculatedCombinedCpuLoad = calculateCombinedZosCpuLoad(portLibrary);

	/* Get CPU usage for current process. Also writes oldest and latest CPU time
	 * to usageStats struct for manual calculation if needed.
	 */
	int32_t result = calculateZosProcessCpuUtilization(portLibrary, usageStats);
	if (result != 0) {
		usageStats->perProcessUtilization = result;
	}

	return 0;
}
