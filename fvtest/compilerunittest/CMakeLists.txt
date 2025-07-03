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
###############################################################################

project(compunittest LANGUAGES C CXX)

set(OMR_WARNINGS_AS_ERRORS OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMPCGTEST_FILES
	main.cpp
	CodeGenTest.cpp
)

if(OMR_ARCH_POWER)
	list(APPEND COMPCGTEST_FILES
		p/BinaryEncoder.cpp
		p/MemoryReferenceExpansion.cpp
		p/Peephole.cpp
	)
endif()

if(OMR_ARCH_X86)
	list(APPEND COMPCGTEST_FILES
		x/BinaryEncoder.cpp
	)
endif()

if(OMR_ARCH_AARCH64)
	list(APPEND COMPCGTEST_FILES
		aarch64/BinaryEncoder.cpp
	)
endif()

list(APPEND COMPCGTEST_FILES
	abstractinterpreter/AbsInterpreterTest.cpp
)

# MSVC and XL C/C++ have trouble with this file
if (NOT OMR_TOOLCONFIG STREQUAL "msvc" AND NOT OMR_TOOLCONFIG STREQUAL "xlc")
	list(APPEND COMPCGTEST_FILES
		CCData.cpp
	)
endif()

omr_add_executable(compunittest NOWARNINGS ${COMPCGTEST_FILES})

# For the time being, link against Tril even though we don't really need Tril itself. This is done
# to avoid needing to duplicate the boilerplate in Tril's CMakeLists.txt.
target_link_libraries(compunittest
	omrGtestGlue
	tril
)

set_property(TARGET compunittest PROPERTY FOLDER fvtest)

omr_add_test(
	NAME compunittest
	COMMAND $<TARGET_FILE:compunittest> --gtest_color=yes --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/compunittest-results.xml
)
