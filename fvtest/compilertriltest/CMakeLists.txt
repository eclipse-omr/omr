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
###############################################################################

project(comptest LANGUAGES C CXX)

set(OMR_WARNINGS_AS_ERRORS OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add bigobj compiler options for TypeConversionTest.cpp under Windows
if(OMR_OS_WINDOWS)
	add_compile_options(-bigobj)
endif()

omr_add_executable(comptest NOWARNINGS
	main.cpp
	JitTest.cpp
	JitTestUtilitiesTest.cpp
	ILValidatorTest.cpp
	ArithmeticTest.cpp
	ShiftAndRotateTest.cpp
	SimplifierFoldAbsNegTest.cpp
	SimplifierFoldAndTest.cpp
	IfxcmpgeReductionTest.cpp
	VectorTest.cpp
	VectorMaskTest.cpp
	VectorTestUtils.cpp
	CallTest.cpp
	LongAndAsRotateTest.cpp
	MockStrategyTest.cpp
	LogicalTest.cpp
	LinkageTest.cpp
	BitPermuteTest.cpp
	MaxMinTest.cpp
	CompareTest.cpp
	TypeConversionTest.cpp
	SelectTest.cpp
	MinimalTest.cpp
	ArrayTest.cpp
)

target_include_directories(comptest PUBLIC
	${omr_SOURCE_DIR}/fvtest
)

target_link_libraries(comptest
	omrGtestGlue
	omrport
	tril
)

set_property(TARGET comptest PROPERTY FOLDER fvtest)

omr_add_test(
	NAME comptest
	COMMAND $<TARGET_FILE:comptest> --gtest_color=yes --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/comptest-results.xml
)
