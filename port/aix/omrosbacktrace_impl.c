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
 * Assisted-by: IBM Bob
 *******************************************************************************/

/**
 * @file
 * @ingroup Port
 * @brief Stack backtracing support
 */

#include <string.h>
#include <sys/ldr.h>

#include "omrport.h"
#include "omrportpriv.h"
#include "omrintrospect.h"
#include "omrsignal_context.h"

/* Backstop against corrupt stacks: the strictly-increasing link check should already prevent
 * cycles, but cap the number of frames walked so a damaged back chain can never make us
 * allocate without bound.
 */
#define AIX_MAX_BACKTRACE_DEPTH 4096

/* AIX stack frames are 16-byte aligned in both 32-bit and 64-bit mode. */
#define AIX_STACK_FRAME_ALIGNMENT 16

/**
 * Check that a back chain link is plausible before it is followed. The AIX stack grows
 * downward and link points from a callee to its caller, so a valid back chain ends in NULL
 * or moves strictly toward higher, 16-byte aligned addresses. The link is never dereferenced.
 * @param[in] frame the current stack frame
 * @param[in] next the link read from the current stack frame
 * @return TRUE if next is NULL or a plausible caller frame, FALSE otherwise
 */
static BOOLEAN
isValidBackChainLink(const AIXStackFrame *frame, const AIXStackFrame *next)
{
	if (NULL == next) {
		return TRUE;
	}
	if ((uintptr_t)frame >= (uintptr_t)next) {
		return FALSE;
	}
	return (0 == ((uintptr_t)next & (AIX_STACK_FRAME_ALIGNMENT - 1))) ? TRUE : FALSE;
}

char *
tbtable_name(struct tbtable *table, short *length)
{
	int offset_to_name = 0;
	char *cursor = (char *)&table->tb_ext;

	/* see sys/debug.h for details of structure layout and permutations */
	if (!table->tb.name_present) {
		return NULL;
	}

	if (table->tb.fixedparms || table->tb.floatparms) {
		offset_to_name += sizeof(table->tb_ext.parminfo);
	}
	if (table->tb.has_tboff) {
		offset_to_name += sizeof(table->tb_ext.tb_offset);
	}
	if (table->tb.int_hndl) {
		offset_to_name += sizeof(table->tb_ext.hand_mask);
	}
	if (table->tb.has_ctl) {
		offset_to_name += sizeof(table->tb_ext.ctl_info);
		offset_to_name += sizeof(table->tb_ext.ctl_info_disp);
	}

	cursor += offset_to_name;
	*length = *(short *)cursor;

	cursor += sizeof(short);

	return cursor;
}

void *
tbtable_symbol_start(struct tbtable *table)
{
	/* if the offset isn't available, bail out */
	if (!table->tb.has_tboff) {
		return NULL;
	}

	/* get the offset */
	/* TODO: do we need to delete the word of NULLs? */
	return (void *)((char *)table - table->tb_ext.tb_offset);
}

/* This function constructs a backtrace from a CPU context. Generally there are only one or two
 * values in the context that are actually used to construct the stack but these vary by platform
 * so aren't detailed here. If no heap is specified then this function will use malloc to allocate
 * the memory necessary for the stack frame structures which must be freed by the caller.
 *
 * @param portLbirary a pointer to an initialized port library
 * @param threadInfo the thread structure we want to attach the backtrace to. Must not be NULL.
 * @param heap a heap from which to allocate any necessary memory. If NULL malloc is used instead.
 * @param signalInfo a platform signal context. If not null the context held in threadInfo is replaced
 *        with the signal context before the backtrace is generated.
 * @return the number of frames in the backtrace.
 */
uintptr_t
omrintrospect_backtrace_thread_raw(struct OMRPortLibrary *portLibrary, J9PlatformThread *threadInfo, J9Heap *heap, void *signalInfo)
{
	uintptr_t num_entries = 0;
	AIXStackFrame *frame = NULL;
	J9PlatformStackFrame **nextFrame = NULL;
	OMRUnixSignalInfo *sigInfo = (OMRUnixSignalInfo *)signalInfo;
	const char *regName = "";
	void **faultingAddress = NULL;

	if (threadInfo == NULL || (threadInfo->context == NULL && sigInfo == NULL)) {
		return 0;
	}

	/* if we've been passed a port library wrapped signal, then extract info from there */
	if (sigInfo != NULL) {
		threadInfo->context = sigInfo->platformSignalInfo.context;

		/* get the faulting address so we can discard frames that are part of the signal handling */
		infoForControl(portLibrary, sigInfo, 0, &regName, (void **)&faultingAddress);
	}

	frame = (AIXStackFrame *)threadInfo->context->uc_mcontext.jmp_context.gpr[1];

	if (NULL != frame) {
		/* the top frame is always incomplete */
		AIXStackFrame *next = frame->link;

		if (isValidBackChainLink(frame, next)) {
			frame = next;
		} else {
			frame = NULL;
			if (!threadInfo->error) {
				threadInfo->error = CORRUPT_STACK;
			}
		}
	}

	nextFrame = &threadInfo->callstack;

	for (num_entries = 0;
		(num_entries < AIX_MAX_BACKTRACE_DEPTH) && (NULL != frame) && (NULL != frame->iar);
		num_entries++
	) {
		AIXStackFrame *next = NULL;

		if (heap == NULL) {
			*nextFrame = portLibrary->mem_allocate_memory(portLibrary, sizeof(J9PlatformStackFrame), OMR_GET_CALLSITE(), OMRMEM_CATEGORY_PORT_LIBRARY);
		} else {
			*nextFrame = portLibrary->heap_allocate(portLibrary, heap, sizeof(J9PlatformStackFrame));
		}

		if (NULL == *nextFrame) {
			if (!threadInfo->error) {
				threadInfo->error = ALLOCATION_FAILURE;
			}

			break;
		}

		(*nextFrame)->parent_frame = NULL;
		(*nextFrame)->symbol = NULL;
		(*nextFrame)->instruction_pointer = (uintptr_t)frame->iar;
		(*nextFrame)->stack_pointer = (uintptr_t)frame;
		(*nextFrame)->base_pointer = (uintptr_t)frame;

		nextFrame = &(*nextFrame)->parent_frame;

		next = frame->link;
		if (isValidBackChainLink(frame, next)) {
			frame = next;
		} else {
			frame = NULL;
			if (!threadInfo->error) {
				threadInfo->error = CORRUPT_STACK;
			}
		}
	}

	/* Stopped by the depth cap with a frame still pending. Don't read that frame: it is
	 * unverified and the stack may be corrupt, so conservatively treat it as truncation.
	 */
	if ((num_entries >= AIX_MAX_BACKTRACE_DEPTH) && (NULL != frame)) {
		if (!threadInfo->error) {
			threadInfo->error = CORRUPT_STACK;
		}
	}

	return num_entries;
}

/* This function takes a thread structure already populated with a backtrace by omrintrospect_backtrace_thread
 * and looks up the symbols for the frames. The format of the string generated is:
 *     symbol_name (statement_id instruction_pointer [module+offset])
 * If it isn't possible to determine any of the items in the string then they are omitted. If no heap is specified
 * then this function will use malloc to allocate the memory necessary for the symbols which must be freed by the caller.
 *
 * @param portLbirary a pointer to an initialized port library
 * @param threadInfo a thread structure populated with a backtrace
 * @param heap a heap from which to allocate any necessary memory. If NULL malloc is used instead.
 * @param options controls how much effort is expended trying to resolve symbols
 * @return the number of frames for which a symbol was constructed.
 */
uintptr_t
omrintrospect_backtrace_symbols_raw(struct OMRPortLibrary *portLibrary, J9PlatformThread *threadInfo, J9Heap *heap, uint32_t options)
{
	/* 32-bit AIX needs 130 slots available for the base VM as of Java6sr7.
	 * This allows a little overhead for 3rd party jni libraries.
	 */
	struct ld_info buffer[150];
	int loadqueryResult = 0;
	int i = 0;
	J9PlatformStackFrame *frame = NULL;

	if (threadInfo == NULL || threadInfo->callstack == NULL) {
		return 0;
	}

	loadqueryResult = loadquery(L_GETINFO, buffer, sizeof(buffer));

	for (i = 0, frame = threadInfo->callstack; frame != NULL; frame = frame->parent_frame, i++) {
		char output_buf[512];
		char *cursor = output_buf;
		struct ld_info *ldInfo = NULL;
		uintptr_t length = 0;
		uintptr_t iar = frame->instruction_pointer;
		char *symbol_name = "";
		char *module_name = "";
		short symbol_length = 0;
		int symbol_offset = -1;
		int module_offset = 0;

		/* try to find the module in the list */
		if (loadqueryResult != -1) {
			struct ld_info *next = buffer;
			struct ld_info *cursor = (struct ld_info *)-1;

			while (cursor != next && cursor != NULL) {
				cursor = next;
				module_offset = iar - (uintptr_t)cursor->ldinfo_textorg;
				if (module_offset >= 0 && module_offset < (uintptr_t)cursor->ldinfo_textsize) {
					ldInfo = cursor;

					/* strip off the full path if possible */
					module_name = strrchr(ldInfo->ldinfo_filename, '/');
					if (module_name != NULL) {
						module_name++;
					} else {
						module_name = ldInfo->ldinfo_filename;
					}

					break;
				}

				module_offset = 0;
				next = (struct ld_info *)((char *)cursor + cursor->ldinfo_next);
			}
		}

		if (ldInfo != NULL) {
			void *moduleEnd = (void *)(ldInfo->ldinfo_textorg + ldInfo->ldinfo_textsize - sizeof(struct AIXFunctionEpilogue));
			/* align our search for the word aligned null word that terminates the procedure */
			struct tbtable *epilogue = (struct tbtable *)(iar & ((~(uintptr_t)0) << 5));

			/* now we've bounded the search to the module look for the function name */
			while (*(uint32_t *)epilogue != 0 && (void *)epilogue < moduleEnd) {
				epilogue = (struct tbtable *)(((uint32_t *)epilogue) + 1);
			}

			if (*(uint32_t *)epilogue == 0) {
				uintptr_t symbol_start;

				/* step past the marking NULL */
				epilogue = (struct tbtable *)(((uint32_t *)epilogue) + 1);

				symbol_name = tbtable_name(epilogue, &symbol_length);
				symbol_start = (uintptr_t)tbtable_symbol_start(epilogue);

				/* sanity check, and calculate offset if we're sane */
				if (symbol_start >= (uintptr_t)ldInfo->ldinfo_textorg) {
					symbol_offset = iar - (uintptr_t)symbol_start;
				}
			}
		}

		/* symbol_name+offset (id, instruction_pointer [module+offset]) */
		if (symbol_name[0] != '\0') {
			cursor += omrstr_printf(portLibrary, cursor, sizeof(output_buf) - (cursor - output_buf), "%.*s", symbol_length, symbol_name);
			if (symbol_offset >= 0) {
				cursor += omrstr_printf(portLibrary, cursor, sizeof(output_buf) - (cursor - output_buf), "+0x%x", symbol_offset);
			}
			/* add the space for padding */
			*(cursor++) = ' ';
		}
		cursor += omrstr_printf(portLibrary, cursor, sizeof(output_buf) - (cursor - output_buf), "(0x%p", frame->instruction_pointer);
		if (module_name[0] != '\0') {
			cursor += omrstr_printf(portLibrary, cursor, sizeof(output_buf) - (cursor - output_buf), " [%s+0x%x]", module_name, module_offset);
		}
		*(cursor++) = ')';
		*cursor = 0;

		length = (cursor - output_buf) + 1;
		if (heap == NULL) {
			frame->symbol = portLibrary->mem_allocate_memory(portLibrary, length, OMR_GET_CALLSITE(), OMRMEM_CATEGORY_PORT_LIBRARY);
		} else {
			frame->symbol = portLibrary->heap_allocate(portLibrary, heap, length);
		}

		if (frame->symbol != NULL) {
			strncpy(frame->symbol, output_buf, length);
		} else {
			frame->symbol = NULL;
			if (!threadInfo->error) {
				threadInfo->error = ALLOCATION_FAILURE;
			}
			i--;
		}
	}

	return i;
}
