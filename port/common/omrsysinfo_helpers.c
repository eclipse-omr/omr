/*******************************************************************************
 * Copyright IBM Corp. and others 2019
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
 * @file
 * @ingroup Port
 * @brief System information
 */

#include "omrsysinfo_helpers.h"

#include "omrport.h"
#include "omrporterror.h"
#include "portnls.h"

#include <string.h>

#if defined(OMR_OS_WINDOWS) || defined(J9X86) || defined(J9HAMMER)
#if defined(OMR_OS_WINDOWS)
#include <intrin.h>
#endif /* defined(OMR_OS_WINDOWS) */

/* defines for the CPUID instruction */
#define CPUID_EAX                                         0
#define CPUID_EBX                                         1
#define CPUID_ECX                                         2
#define CPUID_EDX                                         3

#define CPUID_VENDOR_INFO                                 0
#define CPUID_FAMILY_INFO                                 1
#define CPUID_STRUCTURED_EXTENDED_FEATURE_INFO            7
#define CPUID_AVX10_ENUMERATION_FEATURE_INFO              0x24

#define CPUID_VENDOR_INTEL                                "GenuineIntel"
#define CPUID_VENDOR_AMD                                  "AuthenticAMD"
#define CPUID_VENDOR_LENGTH                               12

#define CPUID_SIGNATURE_STEPPING                          0x0000000F
#define CPUID_SIGNATURE_MODEL                             0x000000F0
#define CPUID_SIGNATURE_FAMILY                            0x00000F00
#define CPUID_SIGNATURE_PROCESSOR                         0x00003000
#define CPUID_SIGNATURE_EXTENDEDMODEL                     0x000F0000
#define CPUID_SIGNATURE_EXTENDEDFAMILY                    0x0FF00000

#define CPUID_SIGNATURE_STEPPING_SHIFT                    0
#define CPUID_SIGNATURE_MODEL_SHIFT                       4
#define CPUID_SIGNATURE_FAMILY_SHIFT                      8
#define CPUID_SIGNATURE_PROCESSOR_SHIFT                   12
#define CPUID_SIGNATURE_EXTENDEDMODEL_SHIFT               16
#define CPUID_SIGNATURE_EXTENDEDFAMILY_SHIFT              20

#define CPUID_FAMILYCODE_INTEL_PENTIUM                    0x05
#define CPUID_FAMILYCODE_INTEL_CORE                       0x06
#define CPUID_FAMILYCODE_INTEL_PENTIUM4                   0x0F

#define INDEX_TO_MASK(idx) (1u << ((idx) % 32))

/**
 * Intel model code suffix naming convention (post-Broadwell)
 *
 * _X : server part (Xeon)
 * _D : micro server
 * _L : mobile
 *    : client
 */
#define CPUID_MODELCODE_INTEL_EMERALDRAPIDS_X             0xCF

#define CPUID_MODELCODE_INTEL_SAPPHIRERAPIDS_X            0x8F

#define CPUID_MODELCODE_INTEL_ICELAKE_X                   0x6A
#define CPUID_MODELCODE_INTEL_ICELAKE_D                   0x6C
#define CPUID_MODELCODE_INTEL_ICELAKE                     0x7D
#define CPUID_MODELCODE_INTEL_ICELAKE_L                   0x7E

#define CPUID_MODELCODE_INTEL_SKYLAKE_L                   0x4E
#define CPUID_MODELCODE_INTEL_SKYLAKE                     0x5E
#define CPUID_MODELCODE_INTEL_SKYLAKE_X                   0x55

#define CPUID_MODELCODE_INTEL_BROADWELL                   0x4F
#define CPUID_MODELCODE_INTEL_HASWELL_1                   0x3F
#define CPUID_MODELCODE_INTEL_HASWELL_2                   0x3C
#define CPUID_MODELCODE_INTEL_IVYBRIDGE_1                 0x3E
#define CPUID_MODELCODE_INTEL_IVYBRIDGE_2                 0x3A
#define CPUID_MODELCODE_INTEL_SANDYBRIDGE                 0x2A
#define CPUID_MODELCODE_INTEL_SANDYBRIDGE_EP              0x2D
#define CPUID_MODELCODE_INTEL_WESTMERE_EP                 0x2C
#define CPUID_MODELCODE_INTEL_WESTMERE_EX                 0x2F
#define CPUID_MODELCODE_INTEL_NEHALEM                     0x1A
#define CPUID_MODELCODE_INTEL_CORE2_HARPERTOWN            0x17
#define CPUID_MODELCODE_INTEL_CORE2_WOODCREST_CLOVERTOWN  0x0F

#define CPUID_STEPPING_INTEL_CASCADELAKE_MIN              0x05
#define CPUID_STEPPING_INTEL_CASCADELAKE_MAX              0x07
#define CPUID_STEPPING_INTEL_COOPERLAKE_MIN               0x0a
#define CPUID_STEPPING_INTEL_COOPERLAKE_MAX               0x0b

#define CPUID_FAMILYCODE_AMD_KSERIES                      0x05
#define CPUID_FAMILYCODE_AMD_ATHLON                       0x06
#define CPUID_FAMILYCODE_AMD_OPTERON                      0x0F

#define CPUID_MODELCODE_AMD_K5                            0x04

#define CPUID_EXTENDEDFAMILYCODE_AMD_OPTERON              0x06

static const char* const OMR_FEATURE_X86_NAME[] = {
	"fpu",              /* 0 * 32 + 0 */
	"vme",              /* 0 * 32 + 1 */
	"de",               /* 0 * 32 + 2 */
	"pse",              /* 0 * 32 + 3 */
	"tsc",              /* 0 * 32 + 4 */
	"msr",              /* 0 * 32 + 5 */
	"pae",              /* 0 * 32 + 6 */
	"mce",              /* 0 * 32 + 7 */
	"cx8",              /* 0 * 32 + 8 */
	"apic",             /* 0 * 32 + 9 */
	"null",             /* 0 * 32 + 10 */
	"sep",              /* 0 * 32 + 11 */
	"mtrr",             /* 0 * 32 + 12 */
	"pge",              /* 0 * 32 + 13 */
	"mca",              /* 0 * 32 + 14 */
	"cmov",             /* 0 * 32 + 15 */
	"pat",              /* 0 * 32 + 16 */
	"pse36",            /* 0 * 32 + 17 */
	"psn",              /* 0 * 32 + 18 */
	"clfsh",            /* 0 * 32 + 19 */
	"null",             /* 0 * 32 + 20 */
	"ds",               /* 0 * 32 + 21 */
	"acpi",             /* 0 * 32 + 22 */
	"mmx",              /* 0 * 32 + 23 */
	"fxsr",             /* 0 * 32 + 24 */
	"sse",              /* 0 * 32 + 25 */
	"sse2",             /* 0 * 32 + 26 */
	"ss",               /* 0 * 32 + 27 */
	"htt",              /* 0 * 32 + 28 */
	"tm",               /* 0 * 32 + 29 */
	"null",             /* 0 * 32 + 30 */
	"pbe",              /* 0 * 32 + 31 */
	"sse3",             /* 1 * 32 + 0 */
	"pclmulqdq",        /* 1 * 32 + 1 */
	"dtes64",           /* 1 * 32 + 2 */
	"monitor",          /* 1 * 32 + 3 */
	"ds_cpl",           /* 1 * 32 + 4 */
	"vmx",              /* 1 * 32 + 5 */
	"smx",              /* 1 * 32 + 6 */
	"eist",             /* 1 * 32 + 7 */
	"tm2",              /* 1 * 32 + 8 */
	"ssse3",            /* 1 * 32 + 9 */
	"cnxt_id",          /* 1 * 32 + 10 */
	"sdbg",             /* 1 * 32 + 11 */
	"fma",              /* 1 * 32 + 12 */
	"cmpxchg16b",       /* 1 * 32 + 13 */
	"xtpr",             /* 1 * 32 + 14 */
	"pdcm",             /* 1 * 32 + 15 */
	"null",             /* 1 * 32 + 16 */
	"pcid",             /* 1 * 32 + 17 */
	"dca",              /* 1 * 32 + 18 */
	"sse4_1",           /* 1 * 32 + 19 */
	"sse4_2",           /* 1 * 32 + 20 */
	"x2apic",           /* 1 * 32 + 21 */
	"movbe",            /* 1 * 32 + 22 */
	"popcnt",           /* 1 * 32 + 23 */
	"tsc_deadline",     /* 1 * 32 + 24 */
	"aesni",            /* 1 * 32 + 25 */
	"xsave",            /* 1 * 32 + 26 */
	"osxsave",          /* 1 * 32 + 27 */
	"avx",              /* 1 * 32 + 28 */
	"f16c",             /* 1 * 32 + 29 */
	"rdrand",           /* 1 * 32 + 30 */
	"null",             /* 1 * 32 + 31 */
	"null",             /* 2 * 32 + 0 */
	"null",             /* 2 * 32 + 1 */
	"null",             /* 2 * 32 + 2 */
	"null",             /* 2 * 32 + 3 */
	"avx10.1",          /* 2 * 32 + 4 */
	"avx10.2",          /* 2 * 32 + 5 */
	"null",             /* 2 * 32 + 6 */
	"null",             /* 2 * 32 + 7 */
	"null",             /* 2 * 32 + 8 */
	"null",             /* 2 * 32 + 9 */
	"null",             /* 2 * 32 + 10 */
	"null",             /* 2 * 32 + 11 */
	"null",             /* 2 * 32 + 12 */
	"null",             /* 2 * 32 + 13 */
	"null",             /* 2 * 32 + 14 */
	"null",             /* 2 * 32 + 15 */
	"avx10/128",        /* 2 * 32 + 16 */
	"avx10/256",        /* 2 * 32 + 17 */
	"avx10/512",        /* 2 * 32 + 18 */
	"null",             /* 2 * 32 + 19 */
	"null",             /* 2 * 32 + 20 */
	"null",             /* 2 * 32 + 21 */
	"null",             /* 2 * 32 + 22 */
	"null",             /* 2 * 32 + 23 */
	"null",             /* 2 * 32 + 24 */
	"null",             /* 2 * 32 + 25 */
	"null",             /* 2 * 32 + 26 */
	"null",             /* 2 * 32 + 27 */
	"null",             /* 2 * 32 + 28 */
	"null",             /* 2 * 32 + 29 */
	"null",             /* 2 * 32 + 30 */
	"null",             /* 2 * 32 + 31 */
	"fsgsbase",         /* 3 * 32 + 0 */
	"ia32_tsc_adjust",  /* 3 * 32 + 1 */
	"sgx",              /* 3 * 32 + 2 */
	"bmi1",             /* 3 * 32 + 3 */
	"hle",              /* 3 * 32 + 4 */
	"avx2",             /* 3 * 32 + 5 */
	"fdp_excptn_only",  /* 3 * 32 + 6 */
	"smep",             /* 3 * 32 + 7 */
	"bmi2",             /* 3 * 32 + 8 */
	"ermsb",            /* 3 * 32 + 9 */
	"invpcid",          /* 3 * 32 + 10 */
	"rtm",              /* 3 * 32 + 11 */
	"rdt_m",            /* 3 * 32 + 12 */
	"deprecate_fpucs",  /* 3 * 32 + 13 */
	"mpx",              /* 3 * 32 + 14 */
	"rdt_a",            /* 3 * 32 + 15 */
	"avx512f",          /* 3 * 32 + 16 */
	"avx512dq",         /* 3 * 32 + 17 */
	"rdseed",           /* 3 * 32 + 18 */
	"adx",              /* 3 * 32 + 19 */
	"smap",             /* 3 * 32 + 20 */
	"avx512_ifma",      /* 3 * 32 + 21 */
	"null",             /* 3 * 32 + 22 */
	"clflushopt",       /* 3 * 32 + 23 */
	"clwb",             /* 3 * 32 + 24 */
	"ipt",              /* 3 * 32 + 25 */
	"avx512pf",         /* 3 * 32 + 26 */
	"avx512er",         /* 3 * 32 + 27 */
	"avx512cd",         /* 3 * 32 + 28 */
	"sha",              /* 3 * 32 + 29 */
	"avx512bw",         /* 3 * 32 + 30 */
	"avx512vl",         /* 3 * 32 + 31 */
	"prefetchwt1",      /* 4 * 32 + 0 */
	"avx512_vbmi",      /* 4 * 32 + 1 */
	"umip",             /* 4 * 32 + 2 */
	"pku",              /* 4 * 32 + 3 */
	"ospke",            /* 4 * 32 + 4 */
	"waitpkg",          /* 4 * 32 + 5 */
	"avx512_vbmi2",     /* 4 * 32 + 6 */
	"cet_ss",           /* 4 * 32 + 7 */
	"gfni",             /* 4 * 32 + 8 */
	"vaes",             /* 4 * 32 + 9 */
	"vpclmulqdq",       /* 4 * 32 + 10 */
	"avx512_vnni",      /* 4 * 32 + 11 */
	"avx512_bitalg",    /* 4 * 32 + 12 */
	"tme_en",           /* 4 * 32 + 13 */
	"avx512_vpopcntdq", /* 4 * 32 + 14 */
	"null",             /* 4 * 32 + 15 */
	"la57",             /* 4 * 32 + 16 */
	"mawau_0",          /* 4 * 32 + 17 */
	"mawau_1",          /* 4 * 32 + 18 */
	"mawau_2",          /* 4 * 32 + 19 */
	"mawau_3",          /* 4 * 32 + 20 */
	"mawau_4",          /* 4 * 32 + 21 */
	"rdpid",            /* 4 * 32 + 22 */
	"kl",               /* 4 * 32 + 23 */
	"bus_lock_detect",  /* 4 * 32 + 24 */
	"cldemote",         /* 4 * 32 + 25 */
	"null",             /* 4 * 32 + 26 */
	"movdiri",          /* 4 * 32 + 27 */
	"movdir64b",        /* 4 * 32 + 28 */
	"enqcmd",           /* 4 * 32 + 29 */
	"sgx_lc",           /* 4 * 32 + 30 */
	"pks",              /* 4 * 32 + 31 */
	"sha512",           /* 5 * 32 + 0 */
	"sm3",              /* 5 * 32 + 1 */
	"sm4",              /* 5 * 32 + 2 */
	"null",             /* 5 * 32 + 3 */
	"avx_vnni",         /* 5 * 32 + 4 */
	"avx512_bf16",      /* 5 * 32 + 5 */
	"lass",             /* 5 * 32 + 6 */
	"cmpccxadd",        /* 5 * 32 + 7 */
	"archperfmonext",   /* 5 * 32 + 8 */
	"null",             /* 5 * 32 + 9 */
	"fastrep_movsb_zero",        /* 5 * 32 + 10 */
	"fastrep_stosb_short",       /* 5 * 32 + 11 */
	"fastrep_cmpsb_scasb_short", /* 5 * 32 + 12 */
	"null",             /* 5 * 32 + 13 */
	"null",             /* 5 * 32 + 14 */
	"null",             /* 5 * 32 + 15 */
	"null",             /* 5 * 32 + 16 */
	"null",             /* 5 * 32 + 17 */
	"null",             /* 5 * 32 + 18 */
	"wrmsrns",          /* 5 * 32 + 19 */
	"null",             /* 5 * 32 + 20 */
	"amx_fp16",         /* 5 * 32 + 21 */
	"hreset",           /* 5 * 32 + 22 */
	"avx_ifma",         /* 5 * 32 + 23 */
	"null",             /* 5 * 32 + 24 */
	"null",             /* 5 * 32 + 25 */
	"lam",              /* 5 * 32 + 26 */
	"msrlist",          /* 5 * 32 + 27 */
	"null",             /* 5 * 32 + 28 */
	"null",             /* 5 * 32 + 29 */
	"invd_disable_post_bios_done", /* 5 * 32 + 30 */
	"movrs",            /* 5 * 32 + 31 */
	"null",             /* 6 * 32 + 0  */
	"null",             /* 6 * 32 + 1  */
	"null",             /* 6 * 32 + 2  */
	"null",             /* 6 * 32 + 3  */
	"avx_vnni_int8",    /* 6 * 32 + 4  */
	"avx_ne_convert",   /* 6 * 32 + 5  */
	"null",             /* 6 * 32 + 6  */
	"null",             /* 6 * 32 + 7  */
	"amx_complex",      /* 6 * 32 + 8  */
	"null",             /* 6 * 32 + 9  */
	"avx_vnni_int16",   /* 6 * 32 + 10 */
	"null",             /* 6 * 32 + 11 */
	"null",             /* 6 * 32 + 12 */
	"null",             /* 6 * 32 + 13 */
	"prefetchi",        /* 6 * 32 + 14 */
	"null",             /* 6 * 32 + 15 */
	"null",             /* 6 * 32 + 16 */
	"uiretuif",         /* 6 * 32 + 17 */
	"cet_sss",          /* 6 * 32 + 18 */
	"avx10",            /* 6 * 32 + 19 */
	"null",             /* 6 * 32 + 20 */
	"apx",              /* 6 * 32 + 21 */
	"null",             /* 6 * 32 + 22 */
	"null",             /* 6 * 32 + 23 */
	"null",             /* 6 * 32 + 24 */
	"null",             /* 6 * 32 + 25 */
	"null",             /* 6 * 32 + 26 */
	"null",             /* 6 * 32 + 27 */
	"null",             /* 6 * 32 + 28 */
	"null",             /* 6 * 32 + 29 */
	"null",             /* 6 * 32 + 30 */
	"null"              /* 6 * 32 + 31 */
};

const char *
omrsysinfo_get_x86_processor_feature_name(uint32_t feature)
{
	if (feature >= sizeof(OMR_FEATURE_X86_NAME)/sizeof(const char *)) {
		return "null";
	}
	return OMR_FEATURE_X86_NAME[feature];
}

/**
 * @internal
 * Populates OMRProcessorDesc *desc on Windows and Linux (x86)
 *
 * @param[in] desc pointer to the struct that will contain the CPU type and features.
 *
 * @return 0 on success, -1 on failure
 */
intptr_t
omrsysinfo_get_x86_description(struct OMRPortLibrary *portLibrary, OMRProcessorDesc *desc)
{
	uint32_t CPUInfo[4] = {0};
	char vendor[12];
	uint32_t familyCode = 0;
	uint32_t processorSignature = 0;
	uint32_t processorStepping = 0;

	desc->processor = OMR_PROCESSOR_X86_UNKNOWN;

	/* vendor */
	omrsysinfo_get_x86_cpuid(CPUID_VENDOR_INFO, CPUInfo);
	memcpy(vendor + 0, &CPUInfo[CPUID_EBX], sizeof(uint32_t));
	memcpy(vendor + 4, &CPUInfo[CPUID_EDX], sizeof(uint32_t));
	memcpy(vendor + 8, &CPUInfo[CPUID_ECX], sizeof(uint32_t));

	/* family and model */
	omrsysinfo_get_x86_cpuid(CPUID_FAMILY_INFO, CPUInfo);
	processorSignature = CPUInfo[CPUID_EAX];
	familyCode = (processorSignature & CPUID_SIGNATURE_FAMILY) >> CPUID_SIGNATURE_FAMILY_SHIFT;
	processorStepping = (processorSignature & CPUID_SIGNATURE_STEPPING) >> CPUID_SIGNATURE_STEPPING_SHIFT;
	if (0 == strncmp(vendor, CPUID_VENDOR_INTEL, CPUID_VENDOR_LENGTH)) {
		switch (familyCode) {
		case CPUID_FAMILYCODE_INTEL_PENTIUM:
			desc->processor = OMR_PROCESSOR_X86_INTEL_PENTIUM;
			break;
		case CPUID_FAMILYCODE_INTEL_CORE:
		{
			uint32_t modelCode  = omrsysinfo_get_cpu_model(processorSignature);
			uint32_t extendedModelCode = omrsysinfo_get_cpu_extended_model(processorSignature);
			uint32_t totalModelCode = modelCode + (extendedModelCode << 4);

			switch (totalModelCode) {
			case CPUID_MODELCODE_INTEL_EMERALDRAPIDS_X:
				desc->processor = OMR_PROCESSOR_X86_INTEL_EMERALDRAPIDS;
				break;
			case CPUID_MODELCODE_INTEL_SAPPHIRERAPIDS_X:
				desc->processor = OMR_PROCESSOR_X86_INTEL_SAPPHIRERAPIDS;
				break;
			case CPUID_MODELCODE_INTEL_ICELAKE_X:
			case CPUID_MODELCODE_INTEL_ICELAKE_D:
			case CPUID_MODELCODE_INTEL_ICELAKE_L:
			case CPUID_MODELCODE_INTEL_ICELAKE:
				desc->processor = OMR_PROCESSOR_X86_INTEL_ICELAKE;
				break;
			case CPUID_MODELCODE_INTEL_SKYLAKE_X:
			case CPUID_MODELCODE_INTEL_SKYLAKE_L:
			case CPUID_MODELCODE_INTEL_SKYLAKE:
				if ((CPUID_STEPPING_INTEL_CASCADELAKE_MIN <= processorStepping) &&
				    (CPUID_STEPPING_INTEL_CASCADELAKE_MAX >= processorStepping)) {
					desc->processor = OMR_PROCESSOR_X86_INTEL_CASCADELAKE;
				}
				else if ((CPUID_STEPPING_INTEL_COOPERLAKE_MIN <= processorStepping) &&
				         (CPUID_STEPPING_INTEL_COOPERLAKE_MAX >= processorStepping)) {
					desc->processor = OMR_PROCESSOR_X86_INTEL_COOPERLAKE;
				}
				else {
					desc->processor = OMR_PROCESSOR_X86_INTEL_SKYLAKE;
				}
				break;
			case CPUID_MODELCODE_INTEL_BROADWELL:
				desc->processor = OMR_PROCESSOR_X86_INTEL_BROADWELL;
				break;
			case CPUID_MODELCODE_INTEL_HASWELL_1:
			case CPUID_MODELCODE_INTEL_HASWELL_2:
				desc->processor = OMR_PROCESSOR_X86_INTEL_HASWELL;
				break;
			case CPUID_MODELCODE_INTEL_IVYBRIDGE_1:
			case CPUID_MODELCODE_INTEL_IVYBRIDGE_2:
				desc->processor = OMR_PROCESSOR_X86_INTEL_IVYBRIDGE;
				break;
			case CPUID_MODELCODE_INTEL_SANDYBRIDGE:
			case CPUID_MODELCODE_INTEL_SANDYBRIDGE_EP:
				desc->processor = OMR_PROCESSOR_X86_INTEL_SANDYBRIDGE;
				break;
			case CPUID_MODELCODE_INTEL_WESTMERE_EP:
			case CPUID_MODELCODE_INTEL_WESTMERE_EX:
				desc->processor = OMR_PROCESSOR_X86_INTEL_WESTMERE;
				break;
			case CPUID_MODELCODE_INTEL_NEHALEM:
				desc->processor = OMR_PROCESSOR_X86_INTEL_NEHALEM;
				break;
			case CPUID_MODELCODE_INTEL_CORE2_HARPERTOWN:
			case CPUID_MODELCODE_INTEL_CORE2_WOODCREST_CLOVERTOWN:
				desc->processor = OMR_PROCESSOR_X86_INTEL_CORE2;
				break;
			default:
				desc->processor = OMR_PROCESSOR_X86_INTEL_P6;
				break;
			}
			break;
		}
		case CPUID_FAMILYCODE_INTEL_PENTIUM4:
			desc->processor = OMR_PROCESSOR_X86_INTEL_PENTIUM4;
			break;
		}
	} else if (0 == strncmp(vendor, CPUID_VENDOR_AMD, CPUID_VENDOR_LENGTH)) {
		switch (familyCode) {
		case CPUID_FAMILYCODE_AMD_KSERIES:
		{
			uint32_t modelCode  = omrsysinfo_get_cpu_model(processorSignature);
			if (modelCode < CPUID_MODELCODE_AMD_K5) {
				desc->processor = OMR_PROCESSOR_X86_AMD_K5;
			}
			else {
				desc->processor = OMR_PROCESSOR_X86_AMD_K6;
			}
			break;
		}
		case CPUID_FAMILYCODE_AMD_ATHLON:
			desc->processor = OMR_PROCESSOR_X86_AMD_ATHLONDURON;
			break;
		case CPUID_FAMILYCODE_AMD_OPTERON:
		{
			uint32_t extendedFamilyCode  = omrsysinfo_get_cpu_extended_family(processorSignature);
			if (extendedFamilyCode < CPUID_EXTENDEDFAMILYCODE_AMD_OPTERON) {
				desc->processor = OMR_PROCESSOR_X86_AMD_OPTERON;
			}
			else {
				desc->processor = OMR_PROCESSOR_X86_AMD_FAMILY15H;
			}
			break;
		}
		}
	}

	desc->physicalProcessor = desc->processor;

	/* features */
	desc->features[0] = CPUInfo[CPUID_EDX];
	desc->features[1] = CPUInfo[CPUID_ECX];
	desc->features[2] = 0;

	/* If OSXSAVE is supported, populate the XSAVE state in desc->features[2], bits 0-3 */
	if (OMR_ARE_ANY_BITS_SET(desc->features[(OMR_FEATURE_X86_OSXSAVE) / 32], INDEX_TO_MASK(OMR_FEATURE_X86_OSXSAVE))) {
		desc->features[2] |= omrsysinfo_get_x86_xsave_state();
	}

	/* extended features */
	omrsysinfo_get_x86_cpuid_ext(CPUID_STRUCTURED_EXTENDED_FEATURE_INFO, 0, CPUInfo); /* CPUID.(0x7.0x0) */
	desc->features[3] = CPUInfo[CPUID_EBX]; /* Structured Extended Feature Flags in EBX */
	desc->features[4] = CPUInfo[CPUID_ECX]; /* Structured Extended Feature Flags in ECX */

	omrsysinfo_get_x86_cpuid_ext(CPUID_STRUCTURED_EXTENDED_FEATURE_INFO, 1, CPUInfo); /* CPUID.(0x7.0x1). */
	desc->features[5] = CPUInfo[CPUID_EAX]; /* Structured Extended Feature Flags in EAX */
	desc->features[6] = CPUInfo[CPUID_EDX]; /* Structured Extended Feature Flags in EDX */

	/* If AVX 10 is supported, populate the AVX 10.X sub version flags in desc->features[2], bits 4-6, 16-18
	 * Unused bits (7-15, 19-31) are reserved for future expansion.
	 */
	if (OMR_ARE_ANY_BITS_SET(desc->features[(OMR_FEATURE_X86_AVX10) / 32], INDEX_TO_MASK(OMR_FEATURE_X86_AVX10))) {
		desc->features[2] |= omrsysinfo_get_x86_avx10_subversion();
	}

	return 0;
}

/**
 * @internal
 * @brief Retrieves a bitmap representing the extended processor state components
 *        enabled by the operating system via the XCR0 register (XSAVE).
 *
 * This function uses the `xgetbv` instruction to query the XCR0 register, which
 * indicates which portions of the extended processor state are enabled and managed
 * by the OS. This function requires that the `OSXSAVE` CPU feature is present.
 *
 * The returned bitmap contains flags representing supported XSAVE state components,
 * such as SSE, AVX, AVX-512, and APX, based on what is enabled in XCR0.
 *
 * @return A bitmap of enabled XSAVE state components.
 */
uint32_t
omrsysinfo_get_x86_xsave_state()
{
	uint32_t state = 0;
	/* Record extended processor state features from XCR0. */
	unsigned long long xcr0 = 0;
	unsigned int leaf = 0;
#if defined(OMR_OS_WINDOWS)
	xcr0 = _xgetbv(leaf);
#else /* defined(OMR_OS_WINDOWS) */
	unsigned int eax = 0;
	unsigned int edx = 0;
	__asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(leaf));
	xcr0 = ((unsigned long long) edx << 32) | eax;
#endif /* defined(OMR_OS_WINDOWS) */

	if (OMR_ARE_ANY_BITS_SET(xcr0, OMR_X86_XCR0_MASK_XMM)) {
		state |= INDEX_TO_MASK(OMR_FEATURE_X86_XSAVE_SSE);
	}

	if (OMR_ARE_ALL_BITS_SET(xcr0, OMR_X86_XCR0_MASK_XMM | OMR_X86_XCR0_MASK_YMM)) {
		state |= INDEX_TO_MASK(OMR_FEATURE_X86_XSAVE_AVX);
	}

	if (OMR_ARE_ALL_BITS_SET(xcr0, OMR_X86_XCR0_MASK_XMM | OMR_X86_XCR0_MASK_YMM | OMR_X86_XCR0_MASK_AVX512)) {
		state |= INDEX_TO_MASK(OMR_FEATURE_X86_XSAVE_AVX512);
	}

	if (OMR_ARE_ANY_BITS_SET(xcr0, OMR_X86_XCR0_MASK_APX_EGPR)) {
		state |= INDEX_TO_MASK(OMR_FEATURE_X86_XSAVE_APX);
	}

	return state;
}

/**
 * @brief Queries the AVX10 sub-version and supported vector lengths on x86 CPUs.
 *
 * This function retrieves information about the AVX10 instruction set extensions
 * supported by the processor using the CPUID leaf eax=0x24, sub-leaf ecx=0. The AVX10 sub-version
 * is stored in EBX[7:0] and is used to determine which AVX10.X features are available.
 * Additionally, vector length support (128-bit, 256-bit, 512-bit) is also reported via flags.
 *
 * The returned value is a bitmask composed of `OMR_FEATURE_X86_AVX10_*` flags indicating:
 * - AVX10 sub-version support (AVX10.1, AVX10.2, AVX10.3)
 * - Vector length support (128/256/512-bit)
 *
 * @return A bitmask indicating the AVX10 features and vector lengths supported by the CPU.
 */
uint32_t omrsysinfo_get_x86_avx10_subversion() {
	/* AVX 10.X support is stored in CPUID(0x24,0).EBX[7:0] as an enumeration*/
	uint32_t state = 0;
	uint32_t subVersion = 0;
	uint32_t CPUInfo[4] = {0};

	omrsysinfo_get_x86_cpuid_ext(CPUID_AVX10_ENUMERATION_FEATURE_INFO, 0, CPUInfo); /* CPUID.(0x24.0x0). */
	subVersion = CPUInfo[CPUID_EBX] & 0xff;

	if (subVersion >= 1) {
		state |= INDEX_TO_MASK(OMR_FEATURE_X86_AVX10_1);
	}

	if (subVersion >= 2) {
		state |= INDEX_TO_MASK(OMR_FEATURE_X86_AVX10_2);
	}

	/* Set more flags as new sub versions arrive */

	/* Check vector length support */
	state |= CPUInfo[CPUID_EBX] & INDEX_TO_MASK(OMR_FEATURE_X86_AVX10_128);
	state |= CPUInfo[CPUID_EBX] & INDEX_TO_MASK(OMR_FEATURE_X86_AVX10_256);
	state |= CPUInfo[CPUID_EBX] & INDEX_TO_MASK(OMR_FEATURE_X86_AVX10_512);

	return state;
}

/**
 * Assembly code to get the register data from CPUID instruction
 * This function executes the CPUID instruction based on which we can detect
 * if the environment is virtualized or not, and also get the Hypervisor Vendor
 * Name based on the same instruction. The leaf value specifies what information
 * to return.
 *
 * @param[in]   leaf        The leaf value to the CPUID instruction and the value
 *                          in EAX when CPUID is called. A value of 0x1 returns basic
 *                          information including feature support.
 * @param[out]  cpuInfo     Reference to the an integer array which holds the data
 *                          of EAX,EBX,ECX and EDX registers.
 *              cpuInfo[0]  To hold the EAX register data, value in this register at
 *                          the time of CPUID tells what information to return
 *                          EAX=0x1,returns the processor Info and feature bits
 *                          in EBX,ECX,EDX registers.
 *                          EAX=0x40000000 returns the Hypervisor Vendor Names
 *                          in the EBX,ECX,EDX registers.
 *              cpuInfo[1]  For EAX = 0x40000000 hold first 4 characters of the
 *                          Hypervisor Vendor String
 *              cpuInfo[2]  For EAX = 0x1, the 31st bit of ECX tells if its
 *                          running on Hypervisor or not,For EAX = 0x40000000 holds the second
 *                          4 characters of the the Hypervisor Vendor String
 *              cpuInfo[3]  For EAX = 0x40000000 hold the last 4 characters of the
 *                          Hypervisor Vendor String
 *
 */
void
omrsysinfo_get_x86_cpuid(uint32_t leaf, uint32_t *cpuInfo)
{
	cpuInfo[0] = leaf;

#if defined(OMR_OS_WINDOWS)
	/* Specific CPUID instruction available in Windows */
	__cpuid(cpuInfo, cpuInfo[0]);

#elif defined(LINUX) || defined(OSX)
#if defined(J9X86)
	__asm volatile(
		"mov %%ebx, %%edi;"
		"cpuid;"
		"mov %%ebx, %%esi;"
		"mov %%edi, %%ebx;"
		:"+a" (cpuInfo[0]), "=S" (cpuInfo[1]), "=c" (cpuInfo[2]), "=d" (cpuInfo[3])
		: /* None */
		:"edi");

#elif defined(J9HAMMER)
	__asm volatile(
		"cpuid;"
		:"+a" (cpuInfo[0]), "=b" (cpuInfo[1]), "=c" (cpuInfo[2]), "=d" (cpuInfo[3])
		);
#endif /* defined(LINUX) || defined(OSX) */
#endif /* defined(OMR_OS_WINDOWS) */
}

/**
 * Similar to omrsysinfo_get_x86_cpuid() above, but with a second subleaf parameter for the
 * leaves returned by the 'cpuid' instruction which are further divided into
 * subleaves.
 *
 * @param[in]   leaf        Value in EAX when cpuid is called to determine what info
 *                          is returned.
 *              subleaf     Value in ECX when cpuid is called which is needed by some
 *                          leafs returned in order to further specify what is returned
 * @param[out]  cpuInfo     Reference to the an integer array which holds the data
 *                          of EAX,EBX,ECX and EDX registers returned by cpuid.
 *                          cpuInfo[0] holds EAX and cpuInfo[4] holds EDX.
 */
void
omrsysinfo_get_x86_cpuid_ext(uint32_t leaf, uint32_t subleaf, uint32_t *cpuInfo)
{
	cpuInfo[0] = leaf;
	cpuInfo[2] = subleaf;

#if defined(OMR_OS_WINDOWS)
	/* Specific CPUID instruction available in Windows */
	__cpuidex(cpuInfo, cpuInfo[0], cpuInfo[2]);

#elif defined(LINUX) || defined(OSX)
#if defined(J9X86)
	__asm volatile(
		"mov %%ebx, %%edi;"
		"cpuid;"
		"mov %%ebx, %%esi;"
		"mov %%edi, %%ebx;"
		:"+a" (cpuInfo[0]), "=S" (cpuInfo[1]), "+c" (cpuInfo[2]), "=d" (cpuInfo[3])
		: /* None */
		:"edi");

#elif defined(J9HAMMER)
	__asm volatile(
		"cpuid;"
		:"+a" (cpuInfo[0]), "=b" (cpuInfo[1]), "+c" (cpuInfo[2]), "=d" (cpuInfo[3])
		);
#endif /* defined(LINUX) || defined(OSX) */
#endif /* defined(OMR_OS_WINDOWS) */
}

uint32_t
omrsysinfo_get_cpu_family(uint32_t processorSignature)
{
	return (processorSignature & CPUID_SIGNATURE_FAMILY) >> CPUID_SIGNATURE_FAMILY_SHIFT;
}

uint32_t
omrsysinfo_get_cpu_extended_family(uint32_t processorSignature)
{
	return (processorSignature & CPUID_SIGNATURE_EXTENDEDFAMILY) >> CPUID_SIGNATURE_EXTENDEDFAMILY_SHIFT;
}

uint32_t
omrsysinfo_get_cpu_model(uint32_t processorSignature)
{
	return (processorSignature & CPUID_SIGNATURE_MODEL) >> CPUID_SIGNATURE_MODEL_SHIFT;
}

uint32_t
omrsysinfo_get_cpu_extended_model(uint32_t processorSignature)
{
	return (processorSignature & CPUID_SIGNATURE_EXTENDEDMODEL) >> CPUID_SIGNATURE_EXTENDEDMODEL_SHIFT;
}

#endif /* defined(OMR_OS_WINDOWS) || defined(J9X86) || defined(J9HAMMER) */
