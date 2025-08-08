/*******************************************************************************
 * Copyright IBM Corp. and others 2000
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#ifndef ILOPCODES_INCL
#define ILOPCODES_INCL

#include "il/OMRDataTypes.hpp"

namespace TR {

enum VectorOperation {
#define VECTOR_OPERATION_MACRO(operation, name, prop1, prop2, prop3, prop4, dataType, typeProps, childProps, \
    swapChildrenOperation, reverseBranchOperation, boolCompareOpcode, ifCompareOpcode, ...)                  \
    operation,

    vBadOperation = 0,

#include "il/VectorOperations.enum"
#undef VECTOR_OPERATION_MACRO

    NumVectorOperations,

    firstTwoTypeVectorOperation = vcast
};

enum ILOpCodes {
#include "il/ILOpCodesEnum.hpp"
    NumScalarIlOps,

    NumOneVectorTypeOperations = TR::firstTwoTypeVectorOperation,
    NumTwoVectorTypeOperations = TR::NumVectorOperations - TR::firstTwoTypeVectorOperation,

    NumOneVectorTypeOps = NumOneVectorTypeOperations * TR::NumVectorTypes,
    NumTwoVectorTypeOps = NumTwoVectorTypeOperations * TR::NumVectorTypes * TR::NumVectorTypes,
    NumAllIlOps = TR::NumScalarIlOps + NumOneVectorTypeOps + NumTwoVectorTypeOps
};
} // namespace TR

#endif
