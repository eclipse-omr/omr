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

/*
 * This file is used to generate the HLASM corresponding to the C calls
 * that use the IARV64 macro in omrvmem.c
 *
 * This file is compiled manually using the METAL-C compiler that was
 * introduced in z/OS V1R9. The generated output (omriarv64.s) is then
 * inserted into omrvmem_support_above_bar.s which is compiled by our makefiles.
 *
 * omrvmem_support_above_bar.s indicates where to put the contents of omriarv64.s.
 * Search for:
 * 		Insert contents of omriarv64.s below
 *
 * *******
 * NOTE!!!!! You must strip the line numbers from any pragma statements!
 * *******
 *
 * It should be obvious, however, just to be clear be sure to omit the
 * first two lines from omriarv64.s which will look something like:
 *
 *          TITLE '5694A01 V1.9 z/OS XL C
 *                     ./omriarv64.c'
 *
 * To compile:
 *  xlc -S -qmetal -Wc,lp64 -qlongname omriarv64.c
 *
 * z/OS V1R9 z/OS V1R9.0 Metal C Programming Guide and Reference:
 * 		http://publibz.boulder.ibm.com/epubs/pdf/ccrug100.pdf
 */

#include "omriarv64.h"

#pragma prolog(omrallocate_1M_fixed_pages,"MYPROLOG")
#pragma epilog(omrallocate_1M_fixed_pages,"MYEPILOG")

__asm(" IARV64 PLISTVER=MAX,MF=(L,LGETSTOR)":"DS"(lgetstor));

/*
 * Allocate 1MB fixed pages using IARV64 system macro.
 * Memory allocated is freed using omrfree_memory_above_bar().
 *
 * @params[in] numMBSegments Number of 1MB segments to be allocated
 * @params[in] userExtendedPrivateAreaMemoryType capability of OS: 0 - general, 1 - 2G-32G range, 2 - 2G-64G range
 *
 * @return pointer to memory allocated, NULL on failure.
 */
void * omrallocate_1M_fixed_pages(int *numMBSegments, int *userExtendedPrivateAreaMemoryType, const char * ttkn) {
	long segments = 0;
	long origin = 0;
	long useMemoryType = *userExtendedPrivateAreaMemoryType;
	int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;
	__asm(" IARV64 PLISTVER=MAX,MF=(L,LGETSTOR)":"DS"(wgetstor));

	segments = *numMBSegments;
	wgetstor = lgetstor;

	switch (useMemoryType) {
	case ZOS64_VMEM_ABOVE_BAR_GENERAL:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,PAGEFRAMESIZE=1MEG,"\
				"SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	case ZOS64_VMEM_2_TO_32G:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO32G=YES,"\
				"CONTROL=UNAUTH,PAGEFRAMESIZE=1MEG,"\
				"SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	case ZOS64_VMEM_2_TO_64G:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO64G=YES,"\
				"CONTROL=UNAUTH,PAGEFRAMESIZE=1MEG,"\
				"SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	}

	if (OMRIARV64_SUCCESS != iarv64_rc) {
		return (void *)0;
	}
	return (void *)origin;
}

#pragma prolog(omrallocate_1M_pageable_pages_above_bar,"MYPROLOG")
#pragma epilog(omrallocate_1M_pageable_pages_above_bar,"MYEPILOG")

__asm(" IARV64 PLISTVER=MAX,MF=(L,NGETSTOR)":"DS"(ngetstor));

/*
 * Allocate 1MB pageable pages above 2GB bar using IARV64 system macro.
 * Memory allocated is freed using omrfree_memory_above_bar().
 *
 * @params[in] numMBSegments Number of 1MB segments to be allocated
 * @params[in] userExtendedPrivateAreaMemoryType capability of OS: 0 - general, 1 - 2G-32G range, 2 - 2G-64G range
 *
 * @return pointer to memory allocated, NULL on failure.
 */
void * omrallocate_1M_pageable_pages_above_bar(int *numMBSegments, int *userExtendedPrivateAreaMemoryType, const char * ttkn) {
	long segments = 0;
	long origin = 0;
	long useMemoryType = *userExtendedPrivateAreaMemoryType;
	int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;

	__asm(" IARV64 PLISTVER=MAX,MF=(L,NGETSTOR)":"DS"(wgetstor));

	segments = *numMBSegments;
	wgetstor = ngetstor;

	switch (useMemoryType) {
	case ZOS64_VMEM_ABOVE_BAR_GENERAL:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,"\
				"PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\
				"ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	case ZOS64_VMEM_2_TO_32G:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTO32G=YES,"\
				"PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\
				"ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	case ZOS64_VMEM_2_TO_64G:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTO64G=YES,"\
				"PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\
				"ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	}

	if (OMRIARV64_SUCCESS != iarv64_rc) {
		return (void *)0;
	}
	return (void *)origin;
}

#pragma prolog(omrallocate_1M_pageable_pages_guarded_above_bar,"MYPROLOG")
#pragma epilog(omrallocate_1M_pageable_pages_guarded_above_bar,"MYEPILOG")

__asm(" IARV64 PLISTVER=MAX,MF=(L,SGETSTOR)":"DS"(sgetstor));

/*
 * Allocate 1MB pageable guarded pages above 2GB bar using IARV64 system macro.
 * Memory allocated is freed using omrfree_memory_above_bar().
 *
 * Note: To stay below MEMLIMIT, GUARDSIZE64 is set equal to the number of
 * segments, which guards the entire allocated memory region.
 *
 * @params[in] numMBSegments number of 1MB segments to be allocated
 * @params[in] userExtendedPrivateAreaMemoryType capability of OS: 0 - general, 1 - 2G-32G range, 2 - 2G-64G range
 *
 * @return pointer to memory allocated, NULL on failure.
 */
void *omrallocate_1M_pageable_pages_guarded_above_bar(int *numMBSegments, int *userExtendedPrivateAreaMemoryType, const char *ttkn) {
	long segments = 0;
	long origin = 0;
	long useMemoryType = *userExtendedPrivateAreaMemoryType;
	int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;

	__asm(" IARV64 PLISTVER=MAX,MF=(L,SGETSTOR)":"DS"(wgetstor));

	segments = *numMBSegments;
	wgetstor = sgetstor;

	switch (useMemoryType) {
	case ZOS64_VMEM_ABOVE_BAR_GENERAL:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,"\
				"GUARDSIZE64=(%2),"\
				"PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\
				"ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	case ZOS64_VMEM_2_TO_32G:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTO32G=YES,"\
				"GUARDSIZE64=(%2),"\
				"PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\
				"ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	case ZOS64_VMEM_2_TO_64G:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTO64G=YES,"\
				"GUARDSIZE64=(%2),"\
				"PAGEFRAMESIZE=PAGEABLE1MEG,TYPE=PAGEABLE,SEGMENTS=(%2),"\
				"ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	}

	if (OMRIARV64_SUCCESS != iarv64_rc) {
		return (void *)0;
	}
	return (void *)origin;
}

#pragma prolog(omrallocate_2G_pages,"MYPROLOG")
#pragma epilog(omrallocate_2G_pages,"MYEPILOG")

__asm(" IARV64 PLISTVER=MAX,MF=(L,OGETSTOR)":"DS"(ogetstor));

/*
 * Allocate 2GB fixed pages using IARV64 system macro.
 * Memory allocated is freed using omrfree_memory_above_bar().
 *
 * @params[in] num2GBUnits Number of 2GB units to be allocated
 * @params[in] userExtendedPrivateAreaMemoryType capability of OS: 0 - general, 1 - 2G-32G range, 2 - 2G-64G range
 *
 * @return pointer to memory allocated, NULL on failure.
 */
void * omrallocate_2G_pages(int *num2GBUnits, int *userExtendedPrivateAreaMemoryType, const char * ttkn) {
	long units = 0;
	long origin = 0;
	long useMemoryType = *userExtendedPrivateAreaMemoryType;
	int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;

	__asm(" IARV64 PLISTVER=MAX,MF=(L,OGETSTOR)":"DS"(wgetstor));

	units = *num2GBUnits;
	wgetstor = ogetstor;

	switch (useMemoryType) {
	case ZOS64_VMEM_ABOVE_BAR_GENERAL:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,"\
				"PAGEFRAMESIZE=2G,TYPE=FIXED,UNITSIZE=2G,UNITS=(%2),"\
				"ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&units),"r"(&wgetstor),"r"(ttkn));
		break;
	case ZOS64_VMEM_2_TO_32G:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTO32G=YES,"\
				"PAGEFRAMESIZE=2G,TYPE=FIXED,UNITSIZE=2G,UNITS=(%2),"\
				"ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&units),"r"(&wgetstor),"r"(ttkn));
		break;
	case ZOS64_VMEM_2_TO_64G:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTO64G=YES,"\
				"PAGEFRAMESIZE=2G,TYPE=FIXED,UNITSIZE=2G,UNITS=(%2),"\
				"ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&units),"r"(&wgetstor),"r"(ttkn));
		break;
	}

	if (OMRIARV64_SUCCESS != iarv64_rc) {
		return (void *)0;
	}
	return (void *)origin;
}

#pragma prolog(omrallocate_4K_pages_in_userExtendedPrivateArea,"MYPROLOG")
#pragma epilog(omrallocate_4K_pages_in_userExtendedPrivateArea,"MYEPILOG")

__asm(" IARV64 PLISTVER=MAX,MF=(L,MGETSTOR)":"DS"(mgetstor));

/*
 * Allocate 4KB pages above 2GB bar using IARV64 system macro.
 * Memory allocated is freed using omrfree_memory_above_bar().
 *
 * @params[in] numMBSegments Number of 1MB segments to be allocated
 * @params[in] userExtendedPrivateAreaMemoryType capability of OS: 0 - general, 1 - 2G-32G range, 2 - 2G-64G range
 *
 * @return pointer to memory allocated, NULL on failure.
 */
void * omrallocate_4K_pages_in_userExtendedPrivateArea(int *numMBSegments, int *userExtendedPrivateAreaMemoryType, const char * ttkn) {
	long segments = 0;
	long origin = 0;
	long useMemoryType = *userExtendedPrivateAreaMemoryType;
	int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;

	__asm(" IARV64 PLISTVER=MAX,MF=(L,MGETSTOR)":"DS"(wgetstor));

	segments = *numMBSegments;
	wgetstor = mgetstor;

	switch (useMemoryType) {
	case ZOS64_VMEM_ABOVE_BAR_GENERAL:
		break;
	case ZOS64_VMEM_2_TO_32G:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO32G=YES,"\
				"CONTROL=UNAUTH,PAGEFRAMESIZE=4K,"\
				"SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	case ZOS64_VMEM_2_TO_64G:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,USE2GTO64G=YES,"\
				"CONTROL=UNAUTH,PAGEFRAMESIZE=4K,"\
				"SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	}

	if (OMRIARV64_SUCCESS != iarv64_rc) {
		return (void *)0;
	}
	return (void *)origin;
}

#pragma prolog(omrallocate_4K_pages_guarded_in_userExtendedPrivateArea,"MYPROLOG")
#pragma epilog(omrallocate_4K_pages_guarded_in_userExtendedPrivateArea,"MYEPILOG")

__asm(" IARV64 PLISTVER=MAX,MF=(L,TGETSTOR)":"DS"(tgetstor));

/*
 * Allocate 4KB pages guarded above 2G bar using IARV64 system macro.
 * Memory allocated is freed using omrfree_memory_above_bar().
 *
 * @params[in] numMBSegments number of 1MB segments to be allocated
 * @params[in] userExtendedPrivateAreaMemoryType capability of OS: 0 - general, 1 - 2G-32G range, 2 - 2G-64G range
 *
 * @return pointer to memory allocated, NULL on failure.
 */
void *omrallocate_4K_pages_guarded_in_userExtendedPrivateArea(int *numMBSegments, int *userExtendedPrivateAreaMemoryType, const char *ttkn) {
	long segments = 0;
	long origin = 0;
	long useMemoryType = *userExtendedPrivateAreaMemoryType;
	int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;

	__asm(" IARV64 PLISTVER=MAX,MF=(L,TGETSTOR)":"DS"(wgetstor));

	segments = *numMBSegments;
	wgetstor = tgetstor;

	switch (useMemoryType) {
	case ZOS64_VMEM_ABOVE_BAR_GENERAL:
		break;
	case ZOS64_VMEM_2_TO_32G:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTO32G=YES,"\
				"GUARDSIZE64=(%2),PAGEFRAMESIZE=4K,SEGMENTS=(%2),"\
				"ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	case ZOS64_VMEM_2_TO_64G:
		__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,CONTROL=UNAUTH,USE2GTO64G=YES,"\
				"GUARDSIZE64=(%2),PAGEFRAMESIZE=4K,SEGMENTS=(%2),"\
				"ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
				::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));
		break;
	}

	if (OMRIARV64_SUCCESS != iarv64_rc) {
		return (void *)0;
	}
	return (void *)origin;
}

#pragma prolog(omrallocate_4K_pages_above_bar,"MYPROLOG")
#pragma epilog(omrallocate_4K_pages_above_bar,"MYEPILOG")

__asm(" IARV64 PLISTVER=MAX,MF=(L,RGETSTOR)":"DS"(rgetstor));

/*
 * Allocate 4KB pages using IARV64 system macro.
 * Memory allocated is freed using omrfree_memory_above_bar().
 *
 * @params[in] numMBSegments Number of 1MB segments to be allocated
 *
 * @return pointer to memory allocated, NULL on failure.
 */
void * omrallocate_4K_pages_above_bar(int *numMBSegments, const char * ttkn) {
	long segments = 0;
	long origin = 0;
	int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;

	__asm(" IARV64 PLISTVER=MAX,MF=(L,RGETSTOR)":"DS"(wgetstor));

	segments = *numMBSegments;
	wgetstor = rgetstor;

	__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,"\
			"CONTROL=UNAUTH,PAGEFRAMESIZE=4K,"\
			"SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
			::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));

	if (0 != iarv64_rc) {
		return (void *)0;
	}
	return (void *)origin;
}

#pragma prolog(omrallocate_4K_pages_guarded_above_bar,"MYPROLOG")
#pragma epilog(omrallocate_4K_pages_guarded_above_bar,"MYEPILOG")

__asm(" IARV64 PLISTVER=MAX,MF=(L,EGETSTOR)":"DS"(egetstor));

/*
 * Allocate 4KB pages guarded using IARV64 system macro.
 * Memory allocated is freed using omrfree_memory_above_bar().
 *
 * Note: To stay below MEMLIMIT, GUARDSIZE64 is set equal to the number of
 * segments, which guards the entire allocated memory region.
 *
 * @params[in] numMBSegments number of 1MB segments to be allocated
 *
 * @return pointer to memory allocated, NULL on failure.
 */
void *omrallocate_4K_pages_guarded_above_bar(int *numMBSegments, const char *ttkn) {
	long segments = 0;
	long origin = 0;
	int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;

	__asm(" IARV64 PLISTVER=MAX,MF=(L,EGETSTOR)":"DS"(wgetstor));

	segments = *numMBSegments;
	wgetstor = egetstor;

	__asm(" IARV64 REQUEST=GETSTOR,COND=YES,SADMP=NO,"\
			"GUARDSIZE64=(%2),"\
			"CONTROL=UNAUTH,PAGEFRAMESIZE=4K,"\
			"SEGMENTS=(%2),ORIGIN=(%1),TTOKEN=(%4),RETCODE=%0,MF=(E,(%3))"\
			::"m"(iarv64_rc),"r"(&origin),"r"(&segments),"r"(&wgetstor),"r"(ttkn));

	if (OMRIARV64_SUCCESS != iarv64_rc) {
		return (void *)0;
	}
	return (void *)origin;
}

#pragma prolog(omrfree_memory_above_bar,"MYPROLOG")
#pragma epilog(omrfree_memory_above_bar,"MYEPILOG")

__asm(" IARV64 PLISTVER=MAX,MF=(L,PGETSTOR)":"DS"(pgetstor));

/*
 * Free memory allocated using IARV64 system macro.
 *
 * @params[in] address pointer to memory region to be freed
 *
 * @return non-zero if memory is not freed successfully, 0 otherwise.
 */
int omrfree_memory_above_bar(void *address, const char * ttkn){
	void *xmemobjstart = 0;
	int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;

	__asm(" IARV64 PLISTVER=MAX,MF=(L,PGETSTOR)":"DS"(wgetstor));

	xmemobjstart = address;
	wgetstor = pgetstor;

	__asm(" IARV64 REQUEST=DETACH,COND=YES,MEMOBJSTART=(%2),TTOKEN=(%3),RETCODE=%0,MF=(E,(%1))"\
			::"m"(iarv64_rc),"r"(&wgetstor),"r"(&xmemobjstart),"r"(ttkn));
	return iarv64_rc;
}

#pragma prolog(omrremove_guard,"MYPROLOG")
#pragma epilog(omrremove_guard,"MYEPILOG")

__asm(" IARV64 PLISTVER=MAX,MF=(L,FGETSTOR)":"DS"(fgetstor));

/*
 * Remove guard for memory allocated using IARV64 system macro.
 *
 * @params[in] address pointer to memory region to be freed
 * @params[in] numMBSegments number of 1MB segments to be allocated
 *
 * @return non-zero if memory is not freed successfully, 0 otherwise.
 *         04 - Operation completed successfully but with exceptions.
 *              One or more segments in the memory object are already
 *              in the requested state.
 *         08 - Request was rejected because there was insufficient
 *              storage to satisfy the request.
 */
int omrremove_guard(void *address, int *numMBSegments){
	void *memObjConvertStart = 0;
	int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;
	long segments = 0;

	__asm(" IARV64 PLISTVER=MAX,MF=(L,FGETSTOR)":"DS"(wgetstor));

	memObjConvertStart = address;
	wgetstor = fgetstor;
	segments = *numMBSegments;

	__asm(" IARV64 REQUEST=CHANGEGUARD,CONVERT=FROMGUARD,COND=YES,"\
			"CONVERTSTART=(%2),CONVERTSIZE64=(%3),"\
			"RETCODE=%0,MF=(E,(%1))"\
			::"m"(iarv64_rc),"r"(&wgetstor),"r"(&memObjConvertStart),"r"(&segments));
	return iarv64_rc;
}

#pragma prolog(omradd_guard,"MYPROLOG")
#pragma epilog(omradd_guard,"MYEPILOG")

__asm(" IARV64 PLISTVER=MAX,MF=(L,DGETSTOR)":"DS"(dgetstor));

/*
 * Add guard to memory allocated using IARV64 system macro.
 *
 * @params[in] address pointer to memory region to be freed
 * @params[in] numMBSegments number of 1MB segments to be allocated
 *
 * @return non-zero if memory is not freed successfully, 0 otherwise.
 *         04 - Operation completed successfully but with exceptions.
 *              One or more segments in the memory object are already
 *              in the requested state.
 *         08 - Request was rejected because there was insufficient
 *              storage to satisfy the request.
 */
int omradd_guard(void *address, int *numMBSegments) {
	void *memObjConvertStart = 0;
	int  iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;
	long segments = 0;

	__asm(" IARV64 PLISTVER=MAX,MF=(L,DGETSTOR)":"DS"(wgetstor));

	memObjConvertStart = address;
	wgetstor = dgetstor;
	segments = *numMBSegments;

	__asm(" IARV64 REQUEST=CHANGEGUARD,CONVERT=TOGUARD,COND=YES,"\
			"CONVERTSTART=(%2),CONVERTSIZE64=(%3),"\
			"RETCODE=%0,MF=(E,(%1))"\
			::"m"(iarv64_rc),"r"(&wgetstor),"r"(&memObjConvertStart),"r"(&segments));
	return iarv64_rc;
}

#pragma prolog(omrdiscard_data,"MYPROLOG")
#pragma epilog(omrdiscard_data,"MYEPILOG")

__asm(" IARV64 PLISTVER=MAX,MF=(L,QGETSTOR)":"DS"(qgetstor));

/* Used to pass parameters to IARV64 DISCARDDATA in omrdiscard_data(). */
struct rangeList {
	void *startAddr;
	long pageCount;
};

/*
 * Discard memory allocated using IARV64 system macro.
 *
 * @params[in] address pointer to memory region to be discarded
 * @params[in] numFrames number of frames to be discarded. Frame size is 4KB.
 *
 * @return non-zero if memory is not discarded successfully, 0 otherwise.
 */
int omrdiscard_data(void *address, int *numFrames) {
	struct rangeList range = {0};
	void *rangePtr = 0;
	int iarv64_rc = OMRIARV64_ERROR_NONSYSTEM_FAILURE;

	__asm(" IARV64 PLISTVER=MAX,MF=(L,QGETSTOR)":"DS"(wgetstor));

	range.startAddr = address;
	range.pageCount = *numFrames;
	rangePtr = (void *)&range;
	wgetstor = qgetstor;

	__asm(" IARV64 REQUEST=DISCARDDATA,KEEPREAL=NO,"\
			"RANGLIST=(%1),RETCODE=%0,MF=(E,(%2))"\
			::"m"(iarv64_rc),"r"(&rangePtr),"r"(&wgetstor));

	return iarv64_rc;
}
