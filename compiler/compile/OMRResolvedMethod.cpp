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

#include "compile/Compilation.hpp"
#include "compile/ResolvedMethod.hpp"
#include "il/SymbolReference.hpp"
#include "ilgen/IlInjector.hpp"
#include "ilgen/IlBuilder.hpp"
#include "ilgen/IlType.hpp"
#include "ilgen/MethodBuilder.hpp"
#include "ilgen/IlGeneratorMethodDetails_inlines.hpp"

// needs major overhaul
OMR::ResolvedMethod::ResolvedMethod(TR_OpaqueMethodBlock *method)
{
    _ilInjector = reinterpret_cast<TR::IlInjector *>(method);

    TR::ResolvedMethod *resolvedMethod = (TR::ResolvedMethod *)_ilInjector->methodSymbol()->getResolvedMethod();
    _fileName = resolvedMethod->classNameChars();
    _name = resolvedMethod->nameChars();
    _numParms = resolvedMethod->getNumArgs();
    _parmTypes = resolvedMethod->_parmTypes;
    _lineNumber = resolvedMethod->getLineNumber();
    _returnType = resolvedMethod->returnIlType();
    _signature = resolvedMethod->getSignature();
    _externalName = 0;
    _entryPoint = resolvedMethod->getEntryPoint();
    strncpy(_signatureChars, resolvedMethod->signatureChars(),
        MAX_SIGNATURE_LENGTH); // TODO: introduce concept of robustness
}

const char *OMR::ResolvedMethod::signature(TR_Memory *trMemory, TR_AllocationKind allocKind)
{
    if (!_signature) {
        size_t len = strlen(_fileName) + 1 + strlen(_lineNumber) + 1 + strlen(_name) + 1;
        char *s = (char *)trMemory->allocateMemory(len, allocKind);
        snprintf(s, len, "%s:%s:%s", _fileName, _lineNumber, _name);

        if (allocKind == heapAlloc)
            _signature = s;

        return s;
    } else
        return _signature;
}

const char *OMR::ResolvedMethod::externalName(TR_Memory *trMemory, TR_AllocationKind allocKind)
{
    if (!_externalName) {
        // For C++, need to mangle name
        // char * s = (char *)trMemory->allocateMemory(1 + strlen(_name) + 1, allocKind);
        // sprintf(s, "_Z%d%si", (int32_t)strlen(_name), _name);

        // functions must be defined as extern "C"
        _externalName = _name;

        // if ( allocKind == heapAlloc)
        //   _externalName = s;
    }

    return _externalName;
}

TR::DataType OMR::ResolvedMethod::parmType(uint32_t slot)
{
    TR_ASSERT((slot < unsigned(_numParms)), "Invalid slot provided for Parameter Type");
    return _parmTypes[slot]->getPrimitiveType();
}

void OMR::ResolvedMethod::computeSignatureChars()
{
    char *name = NULL;
    uint32_t len = 3;
    for (int32_t p = 0; p < _numParms; p++) {
        TR::IlType *type = _parmTypes[p];
        len += static_cast<uint32_t>(strlen(type->getSignatureName()));
    }
    len += static_cast<uint32_t>(strlen(_returnType->getSignatureName()));
    TR_ASSERT(len < MAX_SIGNATURE_LENGTH, "signature array may not be large enough"); // TODO: robustness

    int32_t s = 0;
    _signatureChars[s++] = '(';
    for (int32_t p = 0; p < _numParms; p++) {
        name = _parmTypes[p]->getSignatureName();
        len = static_cast<uint32_t>(strlen(name));
        strncpy(_signatureChars + s, name, MAX_SIGNATURE_LENGTH - s);
        s += len;
    }
    _signatureChars[s++] = ')';
    name = _returnType->getSignatureName();
    len = static_cast<uint32_t>(strlen(name));
    strncpy(_signatureChars + s, name, MAX_SIGNATURE_LENGTH - s);
    s += len;
    _signatureChars[s++] = 0;
}

char *OMR::ResolvedMethod::localName(uint32_t slot, uint32_t bcIndex, int32_t &nameLength, TR_Memory *trMemory)
{
    char *name = NULL;
    if (_ilInjector != NULL && _ilInjector->isMethodBuilder()) {
        TR::MethodBuilder *bldr = _ilInjector->asMethodBuilder();
        name = (char *)bldr->getSymbolName(slot);
        if (name == NULL) {
            name = "";
        }
    } else {
        size_t len = 8 * sizeof(char);
        name = (char *)trMemory->allocateHeapMemory(len);
        snprintf(name, len, "Parm %2d", slot);
    }

    nameLength = static_cast<int32_t>(strlen(name));
    return name;
}

TR::IlInjector *OMR::ResolvedMethod::getInjector(TR::IlGeneratorMethodDetails *details,
    TR::ResolvedMethodSymbol *methodSymbol, TR::FrontEnd *fe, TR::SymbolReferenceTable *symRefTab)
{
    _ilInjector->initialize(details, methodSymbol, fe, symRefTab);
    return _ilInjector;
}

TR::DataType OMR::ResolvedMethod::returnType() { return _returnType->getPrimitiveType(); }

char *OMR::ResolvedMethod::getParameterTypeSignature(int32_t parmIndex)
{
    return _parmTypes[parmIndex]->getSignatureName();
}
