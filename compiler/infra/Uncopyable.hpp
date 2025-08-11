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

#ifndef TR_UNCOPYABLE_INCL
#define TR_UNCOPYABLE_INCL

namespace TR {

/**
 * A utility class that prevents its subtypes from implicitly defining a
 * default copy constructor and copy assignment operator.
 *
 * To prevent a type from being copied, simply make it inherit from
 * TR::Uncopyable. Private inheritance is recommended because the subtyping
 * relationship is irrelevant to consumers of the subtype. Example:
 *
 * \code
 * class Foo : private TR::Uncopyable { ... };
 * ...
 * Foo x(otherFoo); // compile error
 * existingFoo = otherFoo; // compile error
 * \endcode
 *
 * Note that such inheritance does not prevent subtypes from defining their own
 * copy constructors or copy assignment operators explicitly, though to do so
 * would be confusing.
 */
class Uncopyable {
protected:
    Uncopyable() {}

    ~Uncopyable() {}

    // The default copy constructor and copy assignment operator implementations
    // for any subtype require access to the corresponding two methods below.
    // Because they're private, those default definitions are disallowed, so the
    // methods are deleted in the subtype instead.
    //
    // Because explicitly deleted methods are not supported on all build
    // compilers, these methods are deliberately left undefined to ensure that
    // there are no uses of them. If somehow such a use is accidentally
    // introduced (within this class), it will result in an undefined symbol
    // error at link time.
    //
private:
    Uncopyable(const Uncopyable &); // = delete;
    Uncopyable &operator=(const Uncopyable &); // = delete;
};

} // namespace TR

#endif
