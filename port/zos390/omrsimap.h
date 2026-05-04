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
 * @brief Map z/OS Control Block Structures for retrieving system processor
 * and memory usage statistics.  These C structures provide access to the common
 * storage area of currently running address space.  This area which is below
 * 16M bytes contains system control blocks necessary for accessing in-memory
 * system data.
 * MVS Data Areas published as part of official z/OS documentation is the basis
 * for arriving at these structures.  In particular refer to Volumes 1 and 5 for
 * the following listed mappings.  These mappings are essentially cut down
 * structures containing details only relevant to the current implementation.
 * Description of each field in a structure begins with offset:length conforming
 * the data layout published for that structure in MVS Data Areas.
 * Note 1) Fields inside each structure are named after the corresponding data
 *         area element to ensure name consistency while referring to them.
 *      2) All the data area elements employed here are validated by the
 *         respective z/OS development team for correctness and general
 *         programmable usability.
 * See JTCJAZZ 60676.
 */
#ifndef omrsimap_h
#define omrsimap_h

#include "omrcomp.h"

#pragma pack(1)

/**
 * The RSM Internal Table (RIT) reached from the `ritp' address field in PVT. The
 * field of interest here is the total amount of online storage.
 */
typedef struct J9RIT {
	uint8_t ritFiller1[296];          /**< 0:296 Fields irrelevant to our current purpose. */
	uint64_t rittos;                  /**< 296:8 The total amount of online storage at IPL */
	/**< Ignore rest of the fields in RIT. */
} J9RIT;

/**
 * The Page Vector Table (PVT). Reached from the virtual address found at offset 356 of
 * CVT.
 */
typedef struct J9PVT {
	uint8_t pvtFiller1[4];             /**< 0:4 PVT Control Block Identifier 'PVT' */
	J9RIT *__ptr32 pvtritp;            /**< 4:4 Address of the start of RSM Internal Table (RIT) */
	/**< Ignore rest of the fields in PVT. */
} J9PVT;

/**
 * CPU Management Control Table (CCT)
 * It is a collection of CPU control constants, variables, and flags.
 * Fields of interest:
 *   CCVRBSWT - Recent base system wait time
 *   CCVRBSTD - Recent base time of day
 *   CCVCPUCT - No. of online CPUs (GCPs and zIIPs combined)
 *   CCVUTILP - Avg. CPU utilization % for GCPs
 *   CCVUTILA - Avg. CPU utilization % across GCPs and zIIPs (not marked a Programming Interface)
 *   CCVUTILS - Avg. CPU utilization % for the zIIPs (not marked a Programming Interface)
 */
typedef struct J9CCT {
	uint8_t cctFiller1[72];           /**< 0:72 Ignore fields not relevant to current implementation */
	uint32_t ccvrbswt;                /**< 72:4 Recent base system wait time */
	uint8_t cctFiller2[4];            /**< 76:4 Ignore fields not relevant to current implementation */
	uint32_t ccvrbstd;                /**< 80:4 Recent base time of day */
	uint8_t cctFiller3[18];           /**< 84:18 Ignore fields not relevant to current implementation */
	uint16_t ccvutilp;                /**< 102:2 System CPU utilization */
	uint8_t cctFiller4[6];            /**< 104:6 Ignore fields not relevant to current implementation */
	uint16_t ccvcpuct;                /**< 110:2 No of online CPUs */
	uint8_t cctFiller5[10];           /**< 112:10 Ignore fields not relevant to current implementation */
	uint16_t ccvutila;                /**< 122:2 Avg CPU utilization across GCP and zIIPs */
	uint8_t cctFiller6[22];           /**< 124:22 Ignore fields not relevant to current implementation */
	uint16_t ccvutils;                /**< 146:2 Avg CPU utilization for the zIIPs */
	/**< Ignore rest of the CCT */
} J9CCT;

/**
 * Resources Manager Control Table Extension 3 (RMCTX3). Also known as IRARMCTZ.
 *
 * Fields of interest:
 *    RMCTZ_MT_zIIP - bit indicating whether SMT-2 is enabled for zIIP processors
 */
typedef struct J9IRARMCTZ {
	uint8_t rmctzFiller[1270];        /**< 0:1270 Ignore fields not relevant to current implementation */
	uint8_t rmctz_mt_ziip;            /**< 1270:1 bit indicating whether SMT-2 is enabled for zIIP processors */
	/**< Ignore rest of the RMCT Extension 3 */
} J9IRARMCTZ;

/**
 * System Resources Manager Control Table (RMCT)
 * It serves as the origin to locate system resource manager tables and entry points.
 * Fields of interest:
 *   RMCTCCT - CPU Management Control Table
 *   RMCTIRARMCTZ - Address of RMCT Extension 3 Mapped by IRARMCTZ
 */
typedef struct J9RMCT {
	uint8_t rmctname[4];              /**< 0:4 Block Identification */
	J9CCT *__ptr32 rmctcct;           /**< 4:4 CPU Management Control Table */
	uint8_t rmctFiller[368];          /**< 8:368 Ignore fields not relevant to current implementation */
	J9IRARMCTZ *__ptr32 rmctirarmctz; /**< 376:4 Address of RMCT Extention 3 Mapped by IRARMCTZ */
	/**< Ignore rest of the RMCT */
} J9RMCT;

/**
 * Auxiliary Storage Manager (ASM) Vector Table (ASMVT)
 * It is a collection of general ASM information.
 * Fields of interest:
 *   ASMSLOTS - Count of total local slots
 *   ASMVSC - Count of VIO allocated slots
 *   ASMNVSC - Count of non-VIO allocated slots
 *   ASMERRS - Count of bad slots
 */
typedef struct J9ASMVT {
	uint8_t asmvtFiller1[112];        /**< 0:112 Ignore fields not relevant to current implementation */
	uint32_t asmslots;                /**< 112:4 Count of total local slots in all open local page data sets */
	uint32_t asmvsc;                  /**< 116:4 Count of total local slots allocated to VIO private area pages */
	uint32_t asmnvsc;                 /**< 120:4 Count of total local slots to non-VIO private area pages */
	uint32_t asmerrs;                 /**< 124:4 Count of bad slots found on local data sets during normal operation */
	/**< Ignore rest of the ASMVT */
} J9ASMVT;

/**
 * RSM Control & Enumeration Area (RCE)
 * It contains system wide counts and control information used by Real Storage Manager (RSM).
 * Fields of interest:
 *   RCEPOOL - No of frames currently available to system
 *   RCEAFC - Total no of frames currently on all available frame queues
 */
typedef struct J9RCE {
	uint8_t rceid[4];                 /**< 0:4 RCE control block Id */
	int32_t rcepool;                  /**< 4:4 No of frames currently available to system */
	uint8_t rceFiller1[128];          /**< 8:128 Ignore fields not relevant to current implementation */
	int32_t rceafc;                   /**< 136:4 Total no of frames currently on all available frame queues */
	/**< Ignore rest of the RCE */
} J9RCE;

/**
 * Supervisor Vector Table (SVT).
 * Contains service routine addresses and control blocks used by Supervisor Control.
 * Fields of interest:
 *     SVT_SUP_NORMALIZATION - Normalization factor for zIIPs. Multiply zIIP time
 *                             by this value and divide by
 *                             SVT_NORMALIZATION_DIVIDE 256 to get the
 *                             equivalent time on a CP.
 * Source: https://www.ibm.com/docs/en/zos/3.1.0?topic=xtl-svt-information
 */
typedef struct J9CVTSVT {
	uint8_t svtFiller1[416];          /**< 0:416 Ignore fields not relevant to current implementation */
	uint32_t svtSupNormalization;     /**< 416:4 SVT_SUP_NORMALIZATION */
	uint8_t svtFiller2[460];          /**< 420:460 Ignore fields not relevant to current implementation */
	uint8_t svtIFAFlags;              /**< 880:1 SVTIFAFLAGS Processor Flags - not just IFAs */
	/**< Ignore rest of the SVT */
} J9CVTSVT;

/* Common system data area (CSD). */
typedef struct J9CSD {
	uint8_t cvtFiller1[10];           /**< 0:10 Ignore fields not relevant to current implementation */
	int16_t online_combined_CP_count; /**< 10:2 CSDCPUOL */
	uint8_t cvtFiller2[200];          /**< 12:200 Ignore fields not relevant to current implementation */
	int32_t online_combined_CPUs;     /**< 212:4 32-bit CSD_NUMBER_ONLINE_CPUS */
	uint8_t cvtFiller3[48];           /**< 216:48 Ignore fields not relevant to current implementation */
	int32_t online_gcp_count;         /**< 264:4 CSD_NUMBER_ONLINE_STANDARD_CPS */
	int32_t online_zIIP_count;        /**< 268:4 CSD_NUMBER_ONLINE_ZIIPS */
	/**< Ignore rest of the CSD */
} J9CSD;

/**
 * Communications Vector Table (CVT)
 * It contains addresses of other control blocks and tables used by the control
 * program routines.
 * Fields of interest:
 *   CVTPVTP - Address of the page vector table (PVT)
 *   CVTOPCTP - Address of system resource manager (SRM) control table
 *   CVTCSD - Address of common system data area (CSD)
 *   CVTASMVT - Pointer to auxiliary storage management vector table (ASMVT)
 *   CVTRCEP - Address of the RSM Control & Enumeration Area
 */
typedef struct J9CVT {
	uint8_t cvtFiller1[356];          /**< 0:356 Ignore fields not relevant to current implementation */
	J9PVT *__ptr32 cvtpvtp;           /**< 356:4 Address of Page Vector Table (PVT). */
	uint8_t cvtFiller2[244];          /**< 360:244 Ignore fields not relevant to current implementation */
	J9RMCT *__ptr32 cvtopctp;         /**< 604:4 Address of system resources manager (SRM) table */
	uint8_t cvtFiller3[52];           /**< 608:52 Ignore fields not relevant to current implementation */
	J9CSD *__ptr32 cvtcsd;            /**< 660:4 Address of common system data area (CSD) */
	uint8_t cvtFiller4[40];           /**< 664:40 Ignore fields not relevant to current implementation */
	J9ASMVT *__ptr32 cvtasmvt;        /**< 704:4 Pointer to auxiliary storage management vector table (ASMVT) */
	uint8_t cvtFiller5[160];          /**< 708:160 Ignore fields not relevant to current implementation */
	J9CVTSVT *__ptr32 cvtsvt;         /**< 868:4 Address of Supervisor Vector Table (SVT) */
	uint8_t cvtFiller6[296];          /**< 872:296 Ignore fields not relevant to current implementation */
	J9RCE *__ptr32 cvtrcep;           /**< 1168:4 Address of the RSM Control & Enumeration Area */
	uint8_t cvtFiller7[92];           /**< 1172:92 Ignore fields not relevant to current implementation */
	uint8_t cvtoslvl[16];             /**< 1264:1279 OS level/feature information */
	/**< Ignore rest of the CVT */
} J9CVT;

/**
 * Prefixed Save Area (PSA)
 * It maps the storage that starts at location 0 for the related processor.
 * Fields of interest:
 *   FLCCVT - Address of CVT after IPL
 */
typedef struct J9PSA {
	uint8_t psaFiller1[16];           /**< 0:16 Ignore 16 bytes before CVT pointer */
	J9CVT *__ptr32 flccvt;            /**< 16:4 Address of CVT after IPL */
	/**< Ignore rest of the PSA */
} J9PSA;

#pragma pack(pop)

#endif /* omrsimap_h */
