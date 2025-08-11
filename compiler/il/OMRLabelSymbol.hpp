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

#ifndef OMR_LABELSYMBOL_INCL
#define OMR_LABELSYMBOL_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_LABELSYMBOL_CONNECTOR
#define OMR_LABELSYMBOL_CONNECTOR

namespace OMR {
class LabelSymbol;
typedef OMR::LabelSymbol LabelSymbolConnector;
} // namespace OMR
#endif

#include "il/Symbol.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "infra/Assert.hpp"

class TR_Debug;

namespace TR {
class Block;
class CodeGenerator;
class Compilation;
class Instruction;
class LabelSymbol;
class ParameterSymbol;
class Snippet;
class StaticSymbol;
class SymbolReference;
} // namespace TR
template<class T> class List;

namespace OMR {

/**
 * A symbol representing a label.
 *
 * A label has an instruction, a code location...
 */
class OMR_EXTENSIBLE LabelSymbol : public TR::Symbol {
public:
    TR::LabelSymbol *self();

    template<typename AllocatorType> static TR::LabelSymbol *create(AllocatorType, TR::CodeGenerator *);

    template<typename AllocatorType> static TR::LabelSymbol *create(AllocatorType, TR::CodeGenerator *, TR::Block *);

protected:
    LabelSymbol(TR::CodeGenerator *cg);
    LabelSymbol(TR::CodeGenerator *cg, TR::Block *labb);

public:
    // Using declaration is required here or else the
    // debug getName will hide the parent class getName
    using TR::Symbol::getName;
    const char *getName(TR_Debug *debug);

    TR::Instruction *getInstruction() { return _instruction; }

    TR::Instruction *setInstruction(TR::Instruction *p) { return (_instruction = p); }

    uint8_t *getCodeLocation() { return _codeLocation; }

    void setCodeLocation(uint8_t *p) { _codeLocation = p; }

    int32_t getEstimatedCodeLocation() { return _estimatedCodeLocation; }

    int32_t setEstimatedCodeLocation(int32_t p) { return (_estimatedCodeLocation = p); }

    TR::Snippet *getSnippet() { return _snippet; }

    TR::Snippet *setSnippet(TR::Snippet *s) { return (_snippet = s); }

    void setDirectlyTargeted() { _directlyTargeted = true; }

    TR_YesNoMaybe isTargeted(TR::CodeGenerator *cg);

private:
    TR::Instruction *_instruction;

    uint8_t *_codeLocation;

    int32_t _estimatedCodeLocation;

    TR::Snippet *_snippet;

    bool _directlyTargeted;

public:
    /*------------- TR_RelativeLabelSymbol -----------------*/
    template<typename AllocatorType>
    static TR::LabelSymbol *createRelativeLabel(AllocatorType m, TR::CodeGenerator *cg, intptr_t offset);

    /**
     * Mark a LabelSymbol as a RelativeLabelSymbol, inializing members as
     * appropriate.
     *
     * A relative label provides an offset/distance (and changes the name of a
     * symbol to the offset).
     *
     * @todo This leaks memory right now.
     */
    void makeRelativeLabelSymbol(intptr_t offset);

    intptr_t getDistance();

private:
    intptr_t _offset;
};

} // namespace OMR

/**
 * Static creation function.
 */
TR::LabelSymbol *generateLabelSymbol(TR::CodeGenerator *cg);

#endif
