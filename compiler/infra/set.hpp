/*******************************************************************************
 * Copyright IBM Corp. and others 2017
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
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
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef TR_SET_HPP
#define TR_SET_HPP

#pragma once

#include <set>
#include "env/Region.hpp"
#include "env/TRMemory.hpp"
#include "env/TypedAllocator.hpp"

namespace TR {

template<typename T, typename Cmp = std::less<T>, typename Alloc = TR::Region &>
class set : public std::set<T, Cmp, TR::typed_allocator<T, Alloc> > {
private:
    typedef std::set<T, Cmp, TR::typed_allocator<T, Alloc> > Base;

public:
    typedef typename Base::value_compare value_compare;
    typedef typename Base::allocator_type allocator_type;

    set(const value_compare &cmp, const allocator_type &alloc)
        : Base(cmp, alloc)
    {}

    // This constructor is available when the comparison is default-constructible.
    set(const allocator_type &alloc)
        : Base(value_compare(), alloc)
    {}
};

} // namespace TR

#endif // TR_SET_HPP
