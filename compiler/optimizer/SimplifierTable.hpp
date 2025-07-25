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

#ifndef SIMPLIFIERTABLE_INCL
#define SIMPLIFIERTABLE_INCL

namespace TR {
class Block;
class Node;
class Simplifier;
}

#include "optimizer/SimplifierHandlers.hpp"

const SimplifierPointerTable simplifierOpts;

const SimplifierPtr SimplifierPointerTable::table[] =
   {
   #include "optimizer/SimplifierTable.enum"
#define OPCODE_MACRO(\
   opcode, \
   name, \
   prop1, \
   prop2, \
   prop3, \
   prop4, \
   dataType, \
   typeProps, \
   childProps, \
   swapChildrenOpcode, \
   reverseBranchOpcode, \
   boolCompareOpcode, \
   ifCompareOpcode, \
   ...) opcode ## SimplifierHandler,

   BadILOpSimplifierHandler,

   #include "il/Opcodes.enum"

#define VECTOR_OPERATION_MACRO(\
   operation, \
   name, \
   prop1, \
   prop2, \
   prop3, \
   prop4, \
   dataType, \
   typeProps, \
   childProps, \
   swapChildrenOpcode, \
   reverseBranchOpcode, \
   boolCompareOpcode, \
   ifCompareOpcode, \
   ...) operation ## SimplifierHandler,

   BadILOpSimplifierHandler,

   #include "il/VectorOperations.enum"

#undef OPCODE_MACRO
#undef VECTOR_OPERATION_MACRO
   };

#endif
