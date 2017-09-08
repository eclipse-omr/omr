/*******************************************************************************
 * Copyright (c) 1991, 2015 IBM Corp. and others
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
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *******************************************************************************/

#define _GNU_SOURCE

#include <sys/ucontext.h>
#include <unistd.h>

#include "omrport.h"
#include "omrsignal_context.h"


void
fillInUnixSignalInfo(struct OMRPortLibrary *portLibrary, void *contextInfo, struct J9UnixSignalInfo *j9Info)
{
	j9Info->platformSignalInfo.context = (ucontext_t *)contextInfo;
	/* module info is filled on demand */
}

uint32_t
infoForSignal(struct OMRPortLibrary *portLibrary, struct J9UnixSignalInfo *info, int32_t index, const char **name, void **value)
{
	*name = "";

	switch (index) {

	case OMRPORT_SIG_SIGNAL_TYPE:
	case 0:
		*name = "J9Generic_Signal_Number";
		*value = &info->portLibrarySignalType;
		return OMRPORT_SIG_VALUE_32;

	case OMRPORT_SIG_SIGNAL_PLATFORM_SIGNAL_TYPE:
	case 1:
		*name = "Signal_Number";
		*value = &info->sigInfo->si_signo;
		return OMRPORT_SIG_VALUE_32;

	case OMRPORT_SIG_SIGNAL_ERROR_VALUE:
	case 2:
		*name = "Error_Value";
		*value = &info->sigInfo->si_errno;
		return OMRPORT_SIG_VALUE_32;

	case OMRPORT_SIG_SIGNAL_CODE:
	case 3:
		*name = "Signal_Code";
		*value = &info->sigInfo->si_code;
		return OMRPORT_SIG_VALUE_32;

	case OMRPORT_SIG_SIGNAL_HANDLER:
	case 4:
		*name = "Handler1";
		*value = &info->handlerAddress;
		return OMRPORT_SIG_VALUE_ADDRESS;

	case 5:
		*name = "Handler2";
		*value = &info->handlerAddress2;
		return OMRPORT_SIG_VALUE_ADDRESS;

	case OMRPORT_SIG_SIGNAL_INACCESSIBLE_ADDRESS:
	case 6:
		/* si_code > 0 indicates that the signal was generated by the kernel */
		if (info->sigInfo->si_code > 0) {
			if ((info->sigInfo->si_signo == SIGBUS) || (info->sigInfo->si_signo == SIGSEGV)) {
				*name = "InaccessibleAddress";
				*value = &info->sigInfo->si_addr;
				return OMRPORT_SIG_VALUE_ADDRESS;
			}
		}
		return OMRPORT_SIG_VALUE_UNDEFINED;

	default:
		return OMRPORT_SIG_VALUE_UNDEFINED;
	}
}

uint32_t
infoForFPR(struct OMRPortLibrary *portLibrary, struct J9UnixSignalInfo *info, int32_t index, const char **name, void **value)
{
	switch (index) {
	default:
		return OMRPORT_SIG_VALUE_UNDEFINED;
	}
}

uint32_t
infoForGPR(struct OMRPortLibrary *portLibrary, struct J9UnixSignalInfo *info, int32_t index, const char **name, void **value)
{
	struct sigcontext *context = (struct sigcontext *)&info->platformSignalInfo.context->uc_mcontext;
	*name = "";

	switch (index) {
	case OMRPORT_SIG_GPR_X86_EDI:
	case 0:
		*name = "EDI";
		*value = &context->edi;
		return OMRPORT_SIG_VALUE_ADDRESS;
	case OMRPORT_SIG_GPR_X86_ESI:
	case 1:
		*name = "ESI";
		*value = &context->esi;
		return OMRPORT_SIG_VALUE_ADDRESS;
	case OMRPORT_SIG_GPR_X86_EAX:
	case 2:
		*name = "EAX";
		*value = &context->eax;
		return OMRPORT_SIG_VALUE_ADDRESS;
	case OMRPORT_SIG_GPR_X86_EBX:
	case 3:
		*name = "EBX";
		*value = &context->ebx;
		return OMRPORT_SIG_VALUE_ADDRESS;
	case OMRPORT_SIG_GPR_X86_ECX:
	case 4:
		*name = "ECX";
		*value = &context->ecx;
		return OMRPORT_SIG_VALUE_ADDRESS;
	case OMRPORT_SIG_GPR_X86_EDX:
	case 5:
		*name = "EDX";
		*value = &context->edx;
		return OMRPORT_SIG_VALUE_ADDRESS;
	default:
		return OMRPORT_SIG_VALUE_UNDEFINED;
	}

}

uint32_t
infoForControl(struct OMRPortLibrary *portLibrary, struct J9UnixSignalInfo *info, int32_t index, const char **name, void **value)
{
	struct sigcontext *context = (struct sigcontext *)&info->platformSignalInfo.context->uc_mcontext;
	*name = "";

	switch (index) {
	case OMRPORT_SIG_CONTROL_PC:
	case 0:
		*name = "EIP";
		*value = (void *)&(context->eip);
		return OMRPORT_SIG_VALUE_ADDRESS;
	case 1:
		*name = "ES";
		*value = (void *)&(context->es);
		return OMRPORT_SIG_VALUE_16;
	case 2:
		*name = "DS";
		*value = (void *)&(context->ds);
		return OMRPORT_SIG_VALUE_16;
	case OMRPORT_SIG_CONTROL_SP:
	case 3:
		*name = "ESP";
		*value = (void *)&(context->esp);
		return OMRPORT_SIG_VALUE_ADDRESS;
	case OMRPORT_SIG_CONTROL_X86_EFLAGS:
	case 4:
		*name = "EFlags";
		*value = (void *)&(context->eflags);
		return OMRPORT_SIG_VALUE_ADDRESS;
	case 5:
		*name = "CS";
		*value = (void *)&(context->cs);
		return OMRPORT_SIG_VALUE_16;
	case 6:
		*name = "SS";
		*value = (void *)&(context->ss);
		return OMRPORT_SIG_VALUE_16;
	case OMRPORT_SIG_CONTROL_BP:
	case 7:
		*name = "EBP";
		*value = (void *)&(context->ebp);
		return OMRPORT_SIG_VALUE_ADDRESS;
	default:
		return OMRPORT_SIG_VALUE_UNDEFINED;
	}
}


uint32_t
infoForModule(struct OMRPortLibrary *portLibrary, struct J9UnixSignalInfo *info, int32_t index, const char **name, void **value)
{
	void *address;
	Dl_info *dl_info = &(info->platformSignalInfo.dl_info);
	struct sigcontext *context = (struct sigcontext *)&info->platformSignalInfo.context->uc_mcontext;
	*name = "";

	address = (void *)context->eip;
	int dl_result = dladdr(address, dl_info);

	switch (index) {
	case OMRPORT_SIG_MODULE_NAME:
	case 0:
		*name = "Module";
		if (dl_result) {
			*value = (void *)(dl_info->dli_fname);
			return OMRPORT_SIG_VALUE_STRING;
		}
		return OMRPORT_SIG_VALUE_UNDEFINED;
	case 1:
		*name = "Module_base_address";
		if (dl_result) {
			*value = (void *)&(dl_info->dli_fbase);
			return OMRPORT_SIG_VALUE_ADDRESS;
		}
		return OMRPORT_SIG_VALUE_UNDEFINED;
	case 2:
		*name = "Symbol";
		if (dl_result) {
			if (dl_info->dli_sname != NULL) {
				*value = (void *)(dl_info->dli_sname);
				return OMRPORT_SIG_VALUE_STRING;
			}
		}
		return OMRPORT_SIG_VALUE_UNDEFINED;
	case 3:
		*name = "Symbol_address";
		if (dl_result) {
			*value = (void *)&(dl_info->dli_saddr);
			return OMRPORT_SIG_VALUE_ADDRESS;
		}
		return OMRPORT_SIG_VALUE_UNDEFINED;
	default:
		return OMRPORT_SIG_VALUE_UNDEFINED;
	}
}
