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
 * @file
 * @ingroup Port
 * @brief file
 */


#include <windows.h>
#include "omrport.h"
#include "omrutil.h"

char *
omrfile_read_text(struct OMRPortLibrary *portLibrary, intptr_t fd, char *buf, intptr_t nbytes)
{
	char *cursor = buf;

	if (nbytes <= 0) {
		return 0;
	}

	/* discount 1 for the trailing NUL */
	nbytes -= 1;

	while (nbytes > 0) {
		char temp[64];
		intptr_t size = sizeof(temp) > nbytes ? nbytes : sizeof(temp);
		intptr_t count = portLibrary->file_read(portLibrary, fd, temp, size);
		intptr_t i = 0;

		/* ignore translation for now */
		if (count < 0) {
			if (cursor == buf) {
				return NULL;
			} else {
				break;
			}
		}

		for (i = 0; i < count; i++) {
			char c = temp[i];

			if ('\r' == c) { /* EOL */
				/* is this a bare CR, or part of a CRLF pair? */
				portLibrary->file_seek(portLibrary, fd, i - count + 1, EsSeekCur);
				count = portLibrary->file_read(portLibrary, fd, temp, 1);
				if ((count > 0) && ('\n' == temp[0])) {
					/* matched CRLF pair */
					c = '\n';
				} else {
					/* this was a bare CR -- back up */
					portLibrary->file_seek(portLibrary, fd, -1, EsSeekCur);
				}
				*cursor++ = c;
				goto done;
			} else if ('\n' == c) { /* this can only be a bare LF */
				portLibrary->file_seek(portLibrary, fd, i - count + 1, EsSeekCur);
				*cursor++ = '\n';
				goto done;
			} else {
				*cursor++ = c;
			}
		}
		nbytes -= count;
	}

done:
	*cursor = '\0';
	return buf;
}


intptr_t
omrfile_write_text(struct OMRPortLibrary *portLibrary, intptr_t fd, const char *buf, intptr_t nbytes)
{
	intptr_t result = 0;
	intptr_t i = 0;
	int newlines = 0;
	int highchars = 0;
	char stackBuf[512];
	char *newBuf = stackBuf;
	intptr_t newLen = 0;

	/* scan the buffer for any characters which need to be converted */
	for (i = 0; i < nbytes; i++) {
		if ('\n' == buf[i]) {
			newlines += 1;
		} else if (0 != ((uint8_t)buf[i] & 0x80)) {
			highchars += 1;
		}
	}

	/* if there are any non-ASCII chars, convert to Unicode and then to the local code page */
	if (0 != highchars) {
		uint16_t wStackBuf[512];
		uint16_t *wBuf = wStackBuf;
		newLen = (nbytes + newlines) * 2;
		if (newLen > sizeof(wStackBuf)) {
			wBuf = portLibrary->mem_allocate_memory(portLibrary, newLen, OMR_GET_CALLSITE(), OMRMEM_CATEGORY_PORT_LIBRARY);
		}
		if (NULL != wBuf) {
			const uint8_t *in = (const uint8_t *)buf;
			const uint8_t *end = in + nbytes;
			uint16_t *out = wBuf;

			while (in < end) {
				if ('\n' == *in) {
					*out++ = (uint16_t)'\r';
					*out++ = (uint16_t)'\n';
					in += 1;
				} else {
					uint32_t numberU8Consumed = decodeUTF8CharN(in, out++, end - in);
					if (0 == numberU8Consumed) {
						break;
					}
					in += numberU8Consumed;
				}
			}
			/* in will be NULL if an error occurred */
			if (NULL != in) {
				UINT codePage = GetConsoleOutputCP();
				intptr_t wLen = out - wBuf;
				intptr_t mbLen = WideCharToMultiByte(codePage, 0, wBuf, (int)wLen, NULL, 0, NULL, NULL);
				if (mbLen > 0) {
					if (mbLen > sizeof(stackBuf)) {
						newBuf = portLibrary->mem_allocate_memory(portLibrary, mbLen, OMR_GET_CALLSITE(), OMRMEM_CATEGORY_PORT_LIBRARY);
						/* if we couldn't allocate the buffer, just output the data the way it was */
					}
					if (NULL != newBuf) {
						WideCharToMultiByte(codePage, 0, wBuf, (int)wLen, newBuf, (int)mbLen, NULL, NULL);
						buf = newBuf;
						nbytes = mbLen;
					}
				}
			}
			if (wStackBuf != wBuf) {
				portLibrary->mem_free_memory(portLibrary, wBuf);
			}
		}
	} else if (0 != newlines) {
		/* change any LFs to CRLFs */
		newLen = nbytes + newlines;
		if (newLen > sizeof(stackBuf)) {
			newBuf = portLibrary->mem_allocate_memory(portLibrary, newLen, OMR_GET_CALLSITE(), OMRMEM_CATEGORY_PORT_LIBRARY);
			/* if we couldn't allocate the buffer, just output the data the way it was */
		}
		if (NULL != newBuf) {
			char *cursor = newBuf;
			for (i = 0; i < nbytes; i++) {
				if ('\n' == buf[i]) {
					*cursor++ = '\r';
				}
				*cursor++ = buf[i];
			}
			buf = newBuf;
			nbytes = newLen;
		}
	}

	result = portLibrary->file_write(portLibrary, fd, (void *)buf, nbytes);

	if ((stackBuf != newBuf)&& (NULL != newBuf)) {
		portLibrary->mem_free_memory(portLibrary, newBuf);
	}

	return (result == nbytes) ? 0 : result;
}

int32_t
omrfile_get_text_encoding(struct OMRPortLibrary *portLibrary, char *charsetName, uintptr_t nbytes)
{
	CPINFOEX cpix;

	if (NULL == charsetName) {
		return -1;
	} else if (FALSE == GetCPInfoEx(GetConsoleOutputCP(), 0, &cpix)) {
		return -2;
	} else {
		uintptr_t length = strlen(cpix.CodePageName);

		/* In case of very detailed text from OS, truncate at the first space. */
		char *c_ptr = strchr(cpix.CodePageName, ' ');
		if (NULL != c_ptr) {
			length = c_ptr - cpix.CodePageName;
		}

		if (nbytes <= length) {
			return (int32_t)(intptr_t)(length + 1);
		}

		memcpy(charsetName, cpix.CodePageName, length);
		charsetName[length] = '\0';
	}
	return 0;
}
