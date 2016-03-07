/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 1991, 2015
 *
 *  This program and the accompanying materials are made available
 *  under the terms of the Eclipse Public License v1.0 and
 *  Apache License v2.0 which accompanies this distribution.
 *
 *      The Eclipse Public License is available at
 *      http://www.eclipse.org/legal/epl-v10.html
 *
 *      The Apache License v2.0 is available at
 *      http://www.opensource.org/licenses/apache2.0.php
 *
 * Contributors:
 *    Multiple authors (IBM Corp.) - initial API and implementation and/or initial documentation
 *******************************************************************************/

/**
 * @file
 * @ingroup Port
 * @brief Dump formatting
 */

#include <errno.h>
#include <usersec.h>
#include <sys/sysconfig.h>
#include <stdlib.h>
#include <string.h>
#include "omrosdump_helpers.h"

#define CORE_PATH_ENV_STRING "CORE_PATH"

/**
 * Look in the specified directory for the core file that was generated by the OS
 *  and return the full path to it.
 *
 * @param[in] 	portLibrary pointer to the port library
 * @param[in] 	corepath the directory where the core file is expected to be found
 * @param[out]	corepath the full path of the core file that was generated
 * @param[in] 	pid the process' PID. Used to help find the core file
 *
 * @internal @note corepath should be a buffer of at least EsMaxPath
 */
void
appendCoreName(OMRPortLibrary *portLibrary, char *corepath, intptr_t pid)
{
	/* Name will be appended to the end of the path... */
	char *base = corepath + strlen(corepath);
	char corename[64] = "core";
	time_t lastModTime = 0;

	struct stat attrBuf;
	char pidFilter[24];
	uintptr_t pidLen;

	DIR *folder = opendir(corepath);

	struct dirent *entry = NULL;

	/* Failsafe name */
	strcpy(base, "core");

	if (!folder) {
		return;
	}

	pidLen = portLibrary->str_printf(portLibrary, pidFilter, sizeof(pidFilter), ".%d", pid);

	while ((entry = readdir(folder))) {
		char *s = entry->d_name;

		/* Files beginning with "core." must contain ".<pid>",
		 * unless we're on AIX, in which case if core compression is enabled, they can be of the form:
		 * 		1. start with "core.<pid>" or
		 * 		2. "core.Z"
		 * Otherwise, we're interested in only files named "core".
		 */

		if (strcmp(s, "core.Z") == 0) {
			/* don't continue */
		} else if (strstr(s, "core.") == s) {
			do {
				s = strstr(s + 1, pidFilter);
			} while ((s != NULL) && (s[pidLen] != '.') && (s[pidLen] != '\0'));

			/* No such match */
			if (s == NULL) {
				continue;
			}
		} else if (strcmp(s, "core") != 0) {
			/* Otherwise we're only interested in plain "core" files */
			continue;
		}

		/* Temporarily append current name */
		strcpy(base, entry->d_name);

		/* Valid file? - need to use full path */
		if (stat(corepath, &attrBuf) == 0) {
			if (S_ISREG(attrBuf.st_mode) && attrBuf.st_mtime >= lastModTime) {
				/* Keep track of most recent regular matching file */
				strncpy(corename, entry->d_name, 63);
				corename[63] = '\0';
				lastModTime = attrBuf.st_mtime;
			}
		}
	}
	closedir(folder);

	/* Append most likely name */
	strcpy(base, corename);
}

/*
 * Generate a system core file using the gencore() API.
 *
 * @param[in] portLibrary
 * @param[in] filename the requested filename. Provide NULL string if none specified
 * @param[out] the filename that was used.
 *
 * @return 0 on success, non-zero otherwise
 *
 */
intptr_t
genSystemCoreUsingGencore(struct OMRPortLibrary *portLibrary, char *filename)
{
	BOOLEAN filenameWasEmpty = FALSE;
	struct coredumpinfo coreDumpInfo;
	int rc = -1;

	if (*filename == '\0') {
		filenameWasEmpty = TRUE;
	}

	if (*filename != '/') {
		/* filename is not an absolute path, get the cwd to prepend the filename with */
		char cwd[EsMaxPath];

		if (NULL != getcwd(cwd, EsMaxPath)) {

			/* append cwd with '/' if none is present */
			if (cwd[strlen(cwd) - 1] != '/') {
				strncat(cwd, "/", EsMaxPath);
			}

			if (*filename == '\0') {
				/* empty filename was passed in, use the default */
				strncpy(filename, DEFAULT_CORE_FILE_NAME, EsMaxPath);
			}

			/*  append cwd with the filename, then copy the resulting absolute path back into filename */
			strncat(cwd, filename, EsMaxPath);
			strncpy(filename, cwd, EsMaxPath);
		}
	}

	/* we now have a filename that's an absolute path, generate the core file */
	coreDumpInfo.name = filename;
	coreDumpInfo.length = (unsigned int) strlen(filename);

#if defined(DUMP_DBG)
	portLibrary->tty_printf(portLibrary, "\tomrdump_create: attempting to generate corefile, filename: %s\n", coreDumpInfo.name);
	fflush(stdout);
#endif

	coreDumpInfo.pid = getpid();
	coreDumpInfo.flags = GENCORE_VERSION_1;
	rc = gencore(&coreDumpInfo);

#if defined(DUMP_DBG)
	portLibrary->tty_printf(portLibrary, "\tomrdump_create: gencore/coredump returned: %i\n", rc);
	if (rc == -1) {
		portLibrary->tty_printf(portLibrary, "\tomrdump_create: errno: %u %s\n", errno, portLibrary->error_last_error_message(portLibrary));
	}
	fflush(stdout);
#endif


	if (rc == 0) {
		/* check to see if core compression is enabled */
		char value[EsMaxPath];
		value[0] = '\0';

		getPEnvValue(portLibrary, "CORE_COMPRESS", value);
		if (0 == strncmp(value, "on", strlen("on"))) {
			/* the core file will have been compressed and the name will have been appended with ".Z" */
			strncat(filename, ".Z", EsMaxPath - strlen(filename));
		}

	} else {
		/* gencore failed, revert to old mechanism after resetting filename */
		if (filenameWasEmpty == TRUE) {
			*filename = '\0';
		}
	}

	return rc;
}


/*
 * Determines the OS's preferred directory for writing core files.
 *
 * @param[out] coreFilePath caller allocated buffer to put preferred destination for core file.
 *
 * @note buffer is not modified if a preferred desitination was not found
 * @note buffer length is platform dependent, assumed to be EsMaxPath/MAX_PATH
 */
void
findOSPreferredCoreDir(struct OMRPortLibrary *portLibrary, char *coreFilePath)
{
	int i, bufferLen;
	char buffer[EsMaxPath];
	char **penv;

	buffer[0] = '\0';

	getPEnvValue(portLibrary, CORE_PATH_ENV_STRING, buffer);

	/* path via the chcore command takes precedence, if it was not set, check the path set via the syscorepath command*/
	if (buffer[0] == '\0') {
		sysconfig(GET_COREPATH, buffer, MAXPATHLEN);
#if defined(DUMP_DBG)
		portLibrary->tty_printf(portLibrary, "\tsysconfig corepath: %s\n", buffer);
#endif
	}

	bufferLen = strlen(buffer);

	/* make sure there's a '/' at the end of any dir name */
	if ((buffer[0] != '\0') && (buffer[bufferLen - 1] != '/')) {
		buffer[bufferLen] = '/';
		buffer[bufferLen + 1] = '\0';
		bufferLen++;
	}

	/* only overwrite coreFilePath if we did find a preferred directory */
	if (buffer[0] != '\0') {
		strncpy(coreFilePath, buffer, EsMaxPath);
	}

#if defined(DUMP_DBG)
	portLibrary->tty_printf(portLibrary, "\tcorepath set to: %s\n", buffer);
#endif

	return;
}

/*
 * Get the value for the process env option

 * @param[in] portLibrary The port library.
 * @param[in] envVar user-allocated buffer containing environment variable to query.
 * @param[out] value user-allocated buffer for value string describing envVar.
 *
 * @return 0 if were able to find the environment variable envVar, -1 otherwise
 *
 * @note envVar and value are assumed to be EsMaxPath in length
 */
uintptr_t
getPEnvValue(struct OMRPortLibrary *portLibrary, char *envVar, char *value)
{
	char **penv;
	int i;
	char scanFor[EsMaxPath];

	portLibrary->str_printf(portLibrary, scanFor, EsMaxPath, "%s=", envVar);

	penv = getpenv(PENV_SYS);

	/* search the list of env vars returned by getpenv() */
	for (i = 0; penv[i] != NULL; i++) {
		if (!strncmp(penv[i], scanFor, strlen(scanFor))) {
#if defined(DUMP_DBG)
			portLibrary->tty_printf(portLibrary, "\tfound: %s\n", penv[i]);
#endif
			strncpy(value, penv[i] + strlen(scanFor), EsMaxPath);
			return 0;
		}
	}

	return -1;
}



