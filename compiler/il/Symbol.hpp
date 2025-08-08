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

/**
 * \page Symbols Symbols
 *
 * A Symbol is a name for a program object.
 *
 * In OMR, symbols have three main attributes:
 *
 * - A datatype
 * - A size
 * - A name
 *
 * Symbols are further divided into four subclasses:
 *
 * -# \a RegisterMapped symbols are those that can be mapped to registers.
 * -# \a Static symbols are those that can have an address computed at compile
 *       time.
 * -# \a Method symbols name executable code pieces.
 * -# \a Label symbols give names to locations in code.
 *
 * Symbol creation is done through factory methods.
 *
 * See \ref symbolcreation for details.
 *
 */

#ifndef TR_SYMBOL_INCL
#define TR_SYMBOL_INCL

#include "il/OMRSymbol.hpp"

#include <stdint.h>
#include "il/DataTypes.hpp"

namespace TR {

class OMR_EXTENSIBLE Symbol : public OMR::SymbolConnector {
public:
    Symbol()
        : OMR::SymbolConnector()
    {}

    Symbol(TR::DataType d)
        : OMR::SymbolConnector(d)
    {}

    Symbol(TR::DataType d, uint32_t s)
        : OMR::SymbolConnector(d, s)
    {}
};

} // namespace TR

#include "il/Symbol_inlines.hpp"

#endif
