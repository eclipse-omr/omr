/*******************************************************************************
 * Copyright IBM Corp. and others 2014
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

#ifndef TR_RESOLVEDMETHOD_INCL
#define TR_RESOLVEDMETHOD_INCL

#include <string.h>

#include "compile/Method.hpp"
#include "compile/OMRResolvedMethod.hpp"

namespace TR {
class IlGeneratorMethodDetails;
}

namespace TR {
class IlType;
}

namespace TR {
class TypeDictionary;
}

namespace TR {
class IlInjector;
}

namespace TR {
class MethodBuilder;
}

namespace TR {
class FrontEnd;
}

namespace TR {

class ResolvedMethod : public OMR::ResolvedMethodConnector {
public:
    ResolvedMethod(TR_OpaqueMethodBlock *method)
        : OMR::ResolvedMethodConnector(method)
    {}

    ResolvedMethod(char *fileName, char *lineNumber, char *name, int32_t numArgs, TR::IlType **parmTypes,
        TR::IlType *returnType, void *entryPoint, TR::IlInjector *ilInjector)
        : OMR::ResolvedMethodConnector(fileName, lineNumber, name, numArgs, parmTypes, returnType, entryPoint,
              ilInjector)
    {}
};

} // namespace TR

#endif // !defined(TR_RESOLVEDMETHOD_INCL)
