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
    _ilgen = reinterpret_cast<TR_IlGenerator *>(method);

    TR::ResolvedMethod *resolvedMethod = (TR::ResolvedMethod *)_ilgen->methodSymbol()->getResolvedMethod();
    _fileName = resolvedMethod->classNameChars();
    _name = resolvedMethod->nameChars();
    _numParms = resolvedMethod->getNumArgs();
    _parmTypes = resolvedMethod->_parmTypes;
    _parmNames = resolvedMethod->_parmNames;
    _lineNumber = resolvedMethod->getLineNumber();
    _returnType = resolvedMethod->returnType();
    _signature = resolvedMethod->getSignature();
    _externalName = 0;
    _entryPoint = resolvedMethod->getEntryPoint();
    strncpy(_signatureChars, resolvedMethod->signatureChars(),
        MAX_SIGNATURE_LENGTH); // TODO: introduce concept of robustness
}

OMR::ResolvedMethod::ResolvedMethod(const char *fileName, const char *lineNumber, const char *name, int32_t numParms,
    const char **parmNames, TR::DataType *parmTypes, TR::DataType returnType, void *entryPoint, TR_IlGenerator *ilgen)
    : _fileName(fileName)
    , _lineNumber(lineNumber)
    , _name(name)
    , _signature(0)
    , _externalName(0)
    , _numParms(numParms)
    , _parmNames(parmNames)
    , _parmTypes(parmTypes)
    , _returnType(returnType)
    , _entryPoint(entryPoint)
    , _ilgen(ilgen)
{
    computeSignatureChars();
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
        // For C++, need to mangle name but needs to be implemented
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
    return _parmTypes[slot];
}

void OMR::ResolvedMethod::computeSignatureChars()
{
    const char *name = NULL;
    size_t len = 3; // two parentheses plus trailing \0
    for (int32_t p = 0; p < _numParms; p++)
        len += strlen(signatureNameForType[_parmTypes[p].getDataType()]);
    len += strlen(signatureNameForType[_returnType.getDataType()]);
    TR_ASSERT(len < MAX_SIGNATURE_LENGTH, "signature array may not be large enough"); // TODO: robustness

    int32_t s = 0;
    _signatureChars[s++] = '(';
    for (int32_t p = 0; p < _numParms; p++) {
        name = getParameterTypeSignature(p);
        strncpy(_signatureChars + s, name, MAX_SIGNATURE_LENGTH - s);
        s += strlen(name);
    }
    _signatureChars[s++] = ')';
    name = signatureNameForType[_returnType.getDataType()];
    strncpy(_signatureChars + s, name, MAX_SIGNATURE_LENGTH - s);
    s += strlen(name);
    _signatureChars[s++] = 0;
}

char *OMR::ResolvedMethod::localName(uint32_t slot, uint32_t bcIndex, int32_t &nameLength, TR_Memory *trMemory)
{
    // should really just return _parmNames[slot], but since it doesn't return const char *, make a copy and return that
    char *name = (char *)trMemory->allocateHeapMemory(strlen(_parmNames[slot]) + 1);
    strcpy(name, _parmNames[slot]);
    nameLength = static_cast<int32_t>(strlen(name));
    return name;
}

TR_IlGenerator *OMR::ResolvedMethod::getIlGenerator(TR::IlGeneratorMethodDetails *details,
    TR::ResolvedMethodSymbol *methodSymbol, TR::FrontEnd *fe, TR::SymbolReferenceTable *symRefTab)
{
    _ilgen->initialize(details, methodSymbol, fe, symRefTab);
    return _ilgen;
}

TR::DataType OMR::ResolvedMethod::returnType() { return _returnType; }

char *OMR::ResolvedMethod::getParameterTypeSignature(int32_t parmIndex)
{
    TR_ASSERT((parmIndex < _numParms), "Invalid slot provided for getParameterTypeSignature");

    // this const-ness should really be fixed, but because this function is virtual and is probably overridden by all
    // OMR projects, so needs coordination
    return const_cast<char *>(signatureNameForType[_parmTypes[parmIndex].getDataType()]);
}

const char *OMR::ResolvedMethod::signatureNameForType[TR::NumOMRTypes] = {
    "V", // NoType
    "B", // Int8
    "C", // Int16
    "I", // Int32
    "J", // Int64
    "F", // Float
    "D", // Double
    "L", // Address
    "A" // Aggregate
};

const char *OMR::ResolvedMethod::signatureNameForVectorType[TR::NumVectorElementTypes] = {
    "V1", // VectorInt8
    "V2", // VectorInt16
    "V4", // VectorInt32
    "V8", // VectorInt64
    "VF", // VectorFloat
    "VD", // VectorDouble
};

const char *OMR::ResolvedMethod::signatureNameForMaskType[TR::NumVectorElementTypes] = {
    "M1", // MaskInt8
    "M2", // MaskInt16
    "M4", // MaskInt32
    "M8", // MaskInt64
    "MF", // MaskFloat
    "MD", // MaskDouble
};
