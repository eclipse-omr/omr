###############################################################################
# Copyright IBM Corp. and others 2024
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
###############################################################################

if(OMR_OS_ZOS)
	set(OMR_ZOS_COMPILE_ARCHITECTURE "arch10" CACHE STRING "z/OS compile machine architecture" FORCE)
	set(OMR_ZOS_COMPILE_TARGET "ZOSV2R5" CACHE STRING "z/OS compile target operating system" FORCE)
	set(OMR_ZOS_COMPILE_TUNE "12" CACHE STRING "z/OS compile machine architecture tuning" FORCE)
	set(OMR_ZOS_LINK_COMPAT "ZOSV2R5" CACHE STRING "z/OS link compatible operating system" FORCE)
	set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "--shared")
	set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "--shared")

	set(CMAKE_ASM_FLAGS "-fno-integrated-as")
	string(APPEND CMAKE_ASM_FLAGS " \"-Wa,-mgoff\"")
	if(OMR_ENV_DATA64)
		string(APPEND CMAKE_ASM_FLAGS " \"-Wa,-mSYSPARM(BIT64)\"")
	endif()

	list(APPEND OMR_PLATFORM_COMPILE_OPTIONS
		"-fstrict-aliasing"
		"-mzos-target=${OMR_ZOS_COMPILE_TARGET}"
		"-m64"
	)

	list(APPEND OMR_PLATFORM_C_COMPILE_OPTIONS
		-march=${OMR_ZOS_COMPILE_ARCHITECTURE}
	)

	# -std=gnu++14 is used over -std=cpp++14, as the gnu* options are closest to
	# -qlanglvl=extended (enabling most extensions by default). This is preferred
	# in order to avoid defining _EXT repeatedly in most places.
	list(APPEND OMR_PLATFORM_CXX_COMPILE_OPTIONS
		-march=${OMR_ZOS_COMPILE_ARCHITECTURE}
		"-std=gnu++14"
		-fasm
	)

	list(APPEND OMR_PLATFORM_SHARED_COMPILE_OPTIONS
		-fvisibility=default
	)

	if(OMR_ENV_DATA64)
		list(APPEND OMR_PLATFORM_DEFINITIONS
			-DJ9ZOS39064
		)
	else()
		list(APPEND OMR_PLATFORM_DEFINITIONS
			-D_LARGE_FILES
		)
	endif()

	# Testarossa build variables. Longer term the distinction between TR and the rest
	# of the OMR code should be heavily reduced. In the mean time, we keep
	# the distinction.

	# TR_COMPILE_OPTIONS are variables appended to CMAKE_{C,CXX}_FLAGS, and so
	# apply to both C and C++ compilations.
	list(APPEND TR_COMPILE_OPTIONS
		-DYYLMAX=1000
	)

	list(APPEND TR_CXX_COMPILE_OPTIONS
		-Wuninitialized
		-mnocsect
	)

	# Configure the platform-dependent library for multithreading.
	set(OMR_PLATFORM_THREAD_LIBRARY "")
endif()

set(SPP_CMD ${CMAKE_C_COMPILER})
set(SPP_FLAGS -E -P)

if(OMR_OS_ZOS)
	function(_omr_toolchain_process_exports TARGET_NAME)
		# Any type of target which says it has exports should get the DLL, and EXPORTALL
		# compile flags.
		# Open XL equivalent has been added below.
		target_compile_options(${TARGET_NAME}
			PRIVATE
				-fvisibility=default
		)

		get_target_property(target_type ${TARGET_NAME} TYPE)
		if(NOT target_type STREQUAL "SHARED_LIBRARY")
			return()
		endif()
	endfunction()
else()
	function(_omr_toolchain_process_exports TARGET_NAME)
		get_target_property(target_type ${TARGET_NAME} TYPE)
		if(NOT target_type STREQUAL "SHARED_LIBRARY")
			return()
		endif()

		set(exp_file "$<TARGET_PROPERTY:${TARGET_NAME},BINARY_DIR>/${TARGET_NAME}.exp")
		omr_process_template(
			"${omr_SOURCE_DIR}/cmake/modules/platform/toolcfg/xlc_exports.exp.in"
			"${exp_file}"
		)
		set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-bE:${TARGET_NAME}.exp")
	endfunction()

	function(_omr_toolchain_separate_debug_symbols tgt)
		set(exe_file "$<TARGET_FILE:${tgt}>")
		omr_get_target_output_genex(${tgt} output_name)
		set(dbg_file "${output_name}${OMR_DEBUG_INFO_OUTPUT_EXTENSION}")
		add_custom_command(
			TARGET "${tgt}"
			POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy ${exe_file} ${dbg_file}
		)
		# Check if the given target shouldn't be stripped.
		get_target_property(skip_strip ${tgt} OMR_SKIP_STRIP)
		if (NOT ${skip_strip})
			add_custom_command(
				TARGET "${tgt}"
				POST_BUILD
				COMMAND "${CMAKE_STRIP}" -X32_64 -t ${exe_file}
			)
		endif()
		set_target_properties(${tgt} PROPERTIES OMR_DEBUG_FILE "${dbg_file}")
	endfunction()
endif()
