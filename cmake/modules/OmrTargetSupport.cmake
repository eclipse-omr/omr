###############################################################################
# Copyright IBM Corp. and others 2020
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

if(OMR_TARGET_SUPPORT)
	return()
endif()
set(OMR_TARGET_SUPPORT 1)

include(OmrAssert)
include(OmrUtility)

# omr_add_library(name <sources> ...)
#   STATIC | SHARED | OBJECT  - same meanings as for standard add_library
#   OUTPUT_NAME <name>  - Set the OUTPUT_NAME property of the target to <name>
# At present, a thin wrapper around add_library, but it ensures that exports
# and split debug info are handled
function(omr_add_library name)
	set(options SHARED STATIC OBJECT INTERFACE NOWARNINGS SKIP_STRIP)
	set(oneValueArgs OUTPUT_NAME)
	set(multiValueArgs)

	foreach(var IN LISTS options oneValueArgs multiValueArgs)
		set(opt_${var} "")
	endforeach()
	cmake_parse_arguments(opt "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	omr_count_true(num_types VARIABLES opt_SHARED opt_STATIC opt_OBJECT opt_INTERFACE)
	omr_assert(TEST num_types LESS 2 MESSAGE "Only one of STATIC | SHARED | OBJECT | INTERFACE may be given")

	# if no target type given, fall back to STATIC|SHARED depending on BUILD_SHARED_LIBS
	# (same behavior as CMake)
	if(num_types EQUAL 0)
		if(BUILD_SHARED_LIBS)
			set(opt_SHARED True)
		else()
			set(opt_STATIC True)
		endif()
	endif()

	if(opt_SHARED)
		set(lib_type "SHARED")
	elseif(opt_STATIC)
		set(lib_type "STATIC")
	elseif(opt_OBJECT)
		set(lib_type "OBJECT")
	elseif(opt_INTERFACE)
		set(lib_type "INTERFACE")
	else()
		omr_assert(TEST "False" MESSAGE "Unreachable")
	endif()

	add_library(${name} ${lib_type} ${opt_UNPARSED_ARGUMENTS})

	if(opt_OUTPUT_NAME)
		set_target_properties(${name} PROPERTIES OUTPUT_NAME "${opt_OUTPUT_NAME}")
	endif()

	if(opt_SKIP_STRIP)
		set_target_properties(${name} PROPERTIES OMR_SKIP_STRIP TRUE)
	endif()

	if(NOT opt_NOWARNINGS AND NOT lib_type STREQUAL "INTERFACE")
		if(OMR_WARNINGS_AS_ERRORS)
			target_compile_options(${name} PRIVATE
				$<$<COMPILE_LANGUAGE:C>:${OMR_C_WARNINGS_AS_ERROR_FLAG}>
				$<$<COMPILE_LANGUAGE:CXX>:${OMR_CXX_WARNINGS_AS_ERROR_FLAG}>)

			if(DEFINED CMAKE_ASM_MASM AND DEFINED OMR_MASM_WARNINGS_AS_ERROR_FLAG)
				target_compile_options(${name} PRIVATE
					$<$<COMPILE_LANGUAGE:MASM>:${OMR_MASM_WARNINGS_AS_ERROR_FLAG}>)
			endif()

			if(DEFINED CMAKE_ASM_NASM AND DEFINED OMR_NASM_WARNINGS_AS_ERROR_FLAG)
				target_compile_options(${name} PRIVATE
					$<$<COMPILE_LANGUAGE:NASM>:${OMR_NASM_WARNINGS_AS_ERROR_FLAG}>)
			endif()
		endif()

		if(OMR_ENHANCED_WARNINGS)
			target_compile_options(${name} PRIVATE
				$<$<COMPILE_LANGUAGE:C>:${OMR_C_ENHANCED_WARNINGS_FLAG}>
				$<$<COMPILE_LANGUAGE:CXX>:${OMR_CXX_ENHANCED_WARNINGS_FLAG}>)

			if(DEFINED CMAKE_ASM_MASM AND DEFINED OMR_MASM_ENHANCED_WARNINGS_FLAG)
				target_compile_options(${name} PRIVATE
					$<$<COMPILE_LANGUAGE:MASM>:${OMR_MASM_ENHANCED_WARNINGS_FLAG}>)
			endif()

			if(DEFINED CMAKE_ASM_NASM AND DEFINED OMR_NASM_ENHANCED_WARNINGS_FLAG)
				target_compile_options(${name} PRIVATE
					$<$<COMPILE_LANGUAGE:NASM>:${OMR_NASM_ENHANCED_WARNINGS_FLAG}>)
			endif()
		endif()
	endif()

	if(opt_SHARED)
		# Split debug info if applicable. Note: omr_process_split_debug is responsible for
		# checking OMR_SEPARATE_DEBUG_INFO, and _omr_toolchain_separate_debug_symbols is
		# responsible for checking OMR_SKIP_STRIP.
		omr_process_split_debug(${name})
	endif()
endfunction()

# omr_add_executable(name <sources> ...)
#   OUTPUT_NAME <name>  - Set the OUTPUT_NAME property of the target to <name>
# At present, a thin wrapper around add_executable, but it ensures that
# split debug info is handled
function(omr_add_executable name)
	set(options NOWARNINGS)
	set(oneValueArgs OUTPUT_NAME)
	set(multiValueArgs)

	foreach(var IN LISTS options oneValueArgs multiValueArgs)
		set(opt_${var} "")
	endforeach()
	cmake_parse_arguments(opt "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	add_executable(${name} ${opt_UNPARSED_ARGUMENTS})

	if(NOT opt_NOWARNINGS)
		if(OMR_WARNINGS_AS_ERRORS)
			target_compile_options(${name} PRIVATE
				$<$<COMPILE_LANGUAGE:C>:${OMR_C_WARNINGS_AS_ERROR_FLAG}>
				$<$<COMPILE_LANGUAGE:CXX>:${OMR_CXX_WARNINGS_AS_ERROR_FLAG}>)

			if(DEFINED CMAKE_ASM_MASM AND DEFINED OMR_MASM_WARNINGS_AS_ERROR_FLAG)
				target_compile_options(${name} PRIVATE
					$<$<COMPILE_LANGUAGE:MASM>:${OMR_MASM_WARNINGS_AS_ERROR_FLAG}>)
			endif()

			if(DEFINED CMAKE_ASM_NASM AND DEFINED OMR_NASM_WARNINGS_AS_ERROR_FLAG)
				target_compile_options(${name} PRIVATE
					$<$<COMPILE_LANGUAGE:NASM>:${OMR_NASM_WARNINGS_AS_ERROR_FLAG}>)
			endif()
		endif()

		if(OMR_ENHANCED_WARNINGS)
			target_compile_options(${name} PRIVATE
				$<$<COMPILE_LANGUAGE:C>:${OMR_C_ENHANCED_WARNINGS_FLAG}>
				$<$<COMPILE_LANGUAGE:CXX>:${OMR_CXX_ENHANCED_WARNINGS_FLAG}>)

			if(DEFINED CMAKE_ASM_MASM AND DEFINED OMR_MASM_ENHANCED_WARNINGS_FLAG)
				target_compile_options(${name} PRIVATE
					$<$<COMPILE_LANGUAGE:MASM>:${OMR_MASM_ENHANCED_WARNINGS_FLAG}>)
			endif()

			if(DEFINED CMAKE_ASM_NASM AND DEFINED OMR_NASM_ENHANCED_WARNINGS_FLAG)
				target_compile_options(${name} PRIVATE
					$<$<COMPILE_LANGUAGE:NASM>:${OMR_NASM_ENHANCED_WARNINGS_FLAG}>)
			endif()
		endif()
	endif()

	if(opt_OUTPUT_NAME)
		set_target_properties(${name} PROPERTIES OUTPUT_NAME "${opt_OUTPUT_NAME}")
	endif()
	omr_process_split_debug(${name})
endfunction()

# omr_process_exports(<target> [symbol]...)
# Performs appropriate processing to export symbols from a target.
# Note: function is internaly guarded, so its safe to call multiple times.
function(omr_process_exports target)
	omr_assert(FATAL_ERROR TEST TARGET ${target} MESSAGE "omr_process_exports called on invalid target '${target}'")

	# Check if we have already processed the exports.
	get_target_property(has_processed ${target} OMR_EXPORTS_PROCESSED)

	if((NOT has_processed) AND (COMMAND _omr_toolchain_process_exports))
		_omr_toolchain_process_exports(${target})
	endif()

	set_target_properties(${target} PROPERTIES OMR_EXPORTS_PROCESSED TRUE)
endfunction()

# omr_add_exports(<target>)
# Exports given symbols from a given target, and calls omr_process_exports.
function(omr_add_exports target)
	omr_assert(FATAL_ERROR TEST TARGET ${target} MESSAGE "omr_add_exports called on invalid target '${target}'")
	set_property(TARGET ${target} APPEND PROPERTY EXPORTED_SYMBOLS ${ARGN})
	omr_process_exports(${target})
endfunction()

# omr_process_split_debug(<target>)
#   Process a target to generate split debug info if requested/required
function(omr_process_split_debug target)
	omr_assert(FATAL_ERROR TEST TARGET ${target} MESSAGE "omr_process_split_debug called on invalid target '${target}'")

	# if we have already processed this target, skip it
	get_target_property(is_split ${target} OMR_SPLIT_DEBUG_PROCESSED)
	if(is_split)
		return()
	endif()

	set_target_properties(${target} PROPERTIES OMR_SPLIT_DEBUG_PROCESSED TRUE)

	get_target_property(target_type "${target}" TYPE)

	# Only try making split debug info for exes/shared libs, and only if we have support from the toolchain
	if((target_type MATCHES "EXECUTABLE|SHARED_LIBRARY") AND (COMMAND _omr_toolchain_separate_debug_symbols))
		# Default to using config option.
		set(use_split_debug ${OMR_SEPARATE_DEBUG_INFO})

		# OMR_SEPARATE_DEBUG_INFO has no impact on Windows since it already
		# uses separate .pdb files.
		if(OMR_OS_WINDOWS)
			set(use_split_debug FALSE)
		endif()

		# DDR requires separate debug info on OSX.
		if(OMR_OS_OSX)
			set(use_split_debug TRUE)
		endif()

		if(use_split_debug)
			_omr_toolchain_separate_debug_symbols("${target}")
		endif()
	endif()
endfunction()
