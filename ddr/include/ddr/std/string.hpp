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

#ifndef STRING_HPP
#define STRING_HPP

#include "ddr/config.hpp"

#if defined(J9ZOS390)

#include <ctype.h>
#if !defined(__open_xl__)
#undef toupper
#undef tolower
#endif /* !defined(__open_xl__) */
#include <string>

#if !defined(__open_xl__)
#define toupper(c) (islower(c) ? (c & _XUPPER_ASCII) : c)
#define tolower(c) (isupper(c) ? (c | _XLOWER_ASCII) : c)
#endif /* defined(__open_xl__) */

#else /* defined(J9ZOS390) */
#include <string>
#endif /* defined(J9ZOS390) */

#endif /* STRING_HPP */
