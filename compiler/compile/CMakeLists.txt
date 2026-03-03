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

compiler_library(compile

	${CMAKE_CURRENT_LIST_DIR}/OSRData.cpp
	${CMAKE_CURRENT_LIST_DIR}/OMRMethod.cpp
	${CMAKE_CURRENT_LIST_DIR}/OMRResolvedMethod.cpp
	${CMAKE_CURRENT_LIST_DIR}/VirtualGuard.cpp
	${CMAKE_CURRENT_LIST_DIR}/OMRSymbolReferenceTable.cpp
	${CMAKE_CURRENT_LIST_DIR}/OMRAliasBuilder.cpp
	${CMAKE_CURRENT_LIST_DIR}/OMRCompilation.cpp
	${CMAKE_CURRENT_LIST_DIR}/TLSCompilationManager.cpp
	${CMAKE_CURRENT_LIST_DIR}/TRResolvedMethod.cpp

)
