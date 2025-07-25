/*******************************************************************************
 * Copyright IBM Corp. and others 2017
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

#ifndef STATICRELOCATION_HPP
#define STATICRELOCATION_HPP

#pragma once

#include <stdint.h>

class TR_ResolvedMethod;

namespace TR {

enum StaticRelocationType {
    Absolute,
    Relative,
};

enum StaticRelocationSize {
    word8,
    word16,
    word32,
    word64,
};

/**
 * @brief The StaticRelocation class contains the information required to create a static relocation in code generated
 * for use in a statically compiled context.
 */
class StaticRelocation {
public:
    /**
     * @brief StaticRelocation Initializes the object.
     * @param location The address requiring relocation.
     * @param symbol The name of the symbol to be targetted by the relocation.
     * @param size The size of the relocation.
     * @param relocationType The type of the relocation, namely whether that relocation is absolute or relative.
     */
    StaticRelocation(uint8_t *location, const char *symbol, StaticRelocationSize size,
        StaticRelocationType relocationType)
        : _location(location)
        , _symbol(symbol)
        , _size(size)
        , _type(relocationType)
    {}

    /**
     * @brief location Returns the address requiring relocation.
     * @return The address requiring relocation.
     */
    uint8_t *location() const { return _location; }

    /**
     * @brief symbol Returns the name of the symbol to be targetted by the relocation.
     * @return The name of the symbol to be targetted by the relocation.
     */
    const char *symbol() const { return _symbol; }

    /**
     * @brief size Returns the size of the relocation.
     * @return The size of the relocation.
     */
    StaticRelocationSize size() const { return _size; }

    /**
     * @brief type Returns the type of the relocation, namely whether that relocation is absolute or relative.
     * @return The type of the relocation.
     */
    StaticRelocationType type() const { return _type; }

private:
    uint8_t * const _location;
    const char *_symbol;
    const StaticRelocationSize _size;
    const StaticRelocationType _type;
};

} // namespace TR

#endif // STATICRELOCATION_HPP
