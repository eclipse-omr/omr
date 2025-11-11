###############################################################################
# Copyright IBM Corp. and others 2017
#
# This program and the accompanying materials are made available under
# the terms of the Eclipse Public License 2.0 which accompanies this
# distribution and is available at https://www.eclipse.org/legal/epl-2.0/
# or the Apache License, Version 2.0 which accompanies this distribution
# and is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# This Source Code may also be made available under the following Secondary
# Licenses when the conditions for such availability set forth in the
# Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
# version 2 with the GNU Classpath Exception [1] and GNU General Public
# License, version 2 with the OpenJDK Assembly Exception [2].
#
# [1] https://www.gnu.org/software/classpath/license.html
# [2] https://openjdk.org/legal/assembly-exception.html
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
#############################################################################

# Find libelf
# Optional dependencies:
#  LibZ
# Will set:
#  LIBELF_FOUND
#  LIBELF_INCLUDE_DIRS
#  LIBELF_LIBRARIES
#  LIBELF_DEFINITIONS

if(NOT OMR_OS_ZOS)
	# On z/OS we don't want libz.
	find_package(LibZ)
else()
	# For technical reasons this is hard to autodetect on zos.
	# Note: the value can still be overridden on the command line.
	if(OMR_ENV_DATA64)
		set(LIBELF_LIBRARY "/usr/lpp/cbclib/lib/libelfdwarf64.x" CACHE FILEPATH "")
	else()
		set(LIBELF_LIBRARY "/usr/lpp/cbclib/lib/libelfdwarf32.x" CACHE FILEPATH "")
	endif()
endif()

# First we try getting the info we need from pkg-config
find_package(PkgConfig QUIET)
pkg_check_modules(PC_LIBELF QUIET libelf)

find_path(ELF_H_INCLUDE_DIR NAMES elf.h
	HINTS
	${PC_LIBELF_INCLUDEDIR}
	${PC_LIBELF_INCLUDE_DIRS}
)

find_path(LIBELF_H_INCLUDE_DIR NAMES libelf.h
	HINTS
	${PC_LIBELF_INCLUDEDIR}
	${PC_LIBELF_INCLUDE_DIRS}
)

find_library(LIBELF_LIBRARY NAMES elf
	HINTS
	${PC_LIBELF_LIBRARY_DIRS}
	${PC_LIBELF_LIBDIR}
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibElf
	DEFAULT_MSG
	LIBELF_LIBRARY
	ELF_H_INCLUDE_DIR
	LIBELF_H_INCLUDE_DIR
)

if(NOT LIBELF_FOUND)
	set(LIBELF_INCLUDE_DIRS NOTFOUND)
	set(LIBELF_LIBRARIES NOTFOUND)
	set(LIBELF_DEFINITIONS NOTFOUND)
	return()
endif()

# Everything below is only set if the library is found

set(LIBELF_INCLUDE_DIRS ${ELF_H_INCLUDE_DIR} ${LIBELF_H_INCLUDE_DIR})
set(LIBELF_LIBRARIES ${LIBELF_LIBRARY})
set(LIBELF_DEFINITIONS "")

if(LibZ_FOUND)
	list(APPEND LIBELF_INCLUDE_DIRS ${LIBZ_INCLUDE_DIRS})
	list(APPEND LIBELF_LIBRARIES ${LIBZ_LIBRARIES})
	list(APPEND LIBELF_DEFINITIONS ${LIBZ_DEFINITIONS})
endif(LibZ_FOUND)

if(NOT TARGET LibElf::elf)
	add_library(LibElf::elf UNKNOWN IMPORTED)

	set_target_properties(LibElf::elf
		PROPERTIES
			IMPORTED_LOCATION "${LIBELF_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${ELF_H_INCLUDE_DIR};${LIBELF_H_INCLUDE_DIR}"
			INTERFACE_COMPILE_DEFINITIONS ""
	)

	if(LibZ_FOUND)
		set_property(
			TARGET LibElf::elf APPEND
			PROPERTY INTERFACE_LINK_LIBRARIES LibZ::z
		)
	endif(LibZ_FOUND)
endif()
