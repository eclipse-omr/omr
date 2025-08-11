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

#ifndef TR_MAP_HPP
#define TR_MAP_HPP

#pragma once

#include <map>
#include "env/Region.hpp"
#include "env/TRMemory.hpp"
#include "env/TypedAllocator.hpp"

namespace TR {

template<typename K, typename V, typename Cmp = std::less<K>, typename Alloc = TR::Region &>
class map : public std::map<K, V, Cmp, TR::typed_allocator<std::pair<const K, V>, Alloc> > {
private:
    typedef std::map<K, V, Cmp, TR::typed_allocator<std::pair<const K, V>, Alloc> > Base;

public:
    typedef typename Base::key_compare key_compare;
    typedef typename Base::allocator_type allocator_type;

    map(const key_compare &cmp, const allocator_type &alloc)
        : Base(cmp, alloc)
    {}

    // This constructor is available when the comparison is default-constructible.
    map(const allocator_type &alloc)
        : Base(key_compare(), alloc)
    {}
};

} // namespace TR

#endif // TR_MAP_HPP
