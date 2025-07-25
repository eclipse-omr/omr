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

#ifndef OMR_REGION_HPP
#define OMR_REGION_HPP

#pragma once

#include <stddef.h>
#include <deque>
#include <stack>
#include "infra/ReferenceWrapper.hpp"
#include "env/TypedAllocator.hpp"
#include "env/MemorySegment.hpp"
#include "env/RawAllocator.hpp"

namespace TR {

class SegmentProvider;
class RegionProfiler;

class Region {
    /** \brief A list entry that knows an object to destroy, and how to destroy it. */
    class Destroyer {
    public:
        Destroyer(Destroyer *prev)
            : _prev(prev)
        {}

        Destroyer *prev() { return _prev; }

        virtual void destroy() = 0;

    private:
        Destroyer * const _prev;
    };

    /**
     * \brief A Destroyer that destroys an object of type \p T.
     */
    template<typename T> class TypedDestroyer : public Destroyer {
    public:
        TypedDestroyer(Destroyer *prev, T *obj)
            : Destroyer(prev)
            , _obj(obj)
        {}

        virtual void destroy() { _obj->~T(); }

    private:
        T * const _obj;
    };

public:
    Region(TR::SegmentProvider &segmentProvider, TR::RawAllocator rawAllocator);
    Region(const Region &prototype);
    virtual ~Region() throw();
    void *allocate(const size_t bytes, void *hint = 0);

    /**
     * \brief Register an object to be explicitly destroyed along with this Region.
     *
     * <em>Because Region frees memory in bulk, such registration is
     * unnecessary for almost all objects.</em>
     *
     * All registered objects will be destroyed in LIFO order when this
     * Region is destroyed, just prior to freeing the backing memory. As such:
     *
     * (a) \p obj must have been allocated by this Region, and
     * (b) objects should generally be registered in construction order.
     *
     * To ensure that these hold, register immediately after new like so:
     *
       \verbatim
          TheOneRing *ring = new (region) TheOneRing(...); // must be destroyed
          region.registerDestructor(ring);
       \endverbatim
     *
     * A previous version of this API attempted to guarantee (a) and (b) by
     * fusing this registration with the allocation and construction of the
     * object to be registered, but in so doing, it required the object's type
     * to be copy-constructible. It's possible to fuse without that requirement
     * (and without requiring it to be default-constructible), but only with
     * variadic macros, which are not necessarily supported on all build
     * compilers.
     *
     * <em>Pointer type:</em> The type \p T at the call site is the type whose
     * destructor will be called to destroy \p obj. Therefore, \p T must be
     * either the exact concrete type of \p obj or a base type with a virtual
     * destructor. If \p T is a base type without a virtual destructor, then
     * \p obj will not be properly destroyed.
     *
     * <em>Possibly unintuitive relative lifetime of registered objects:</em>
     * If a Region R0 is used to create and register another Region R1 (or an
     * object embedding another Region R1 by value), then any objects created
     * and registered in R1 will outlive objects registered in R0 after the
     * registration of R1. This may be surprising because all \em unregistered
     * objects in R0 outlive all \em unregistered objects in R1. The following
     * example shows the order of destruction of the (non-Region) objects:
     *
       \verbatim
          Foo *foo = new (R0) Foo(...);
          R0.registerDestructor(foo);      // third destroyed

          TR::Region *R1 = new (R0) TR::Region(...);
          R0.registerDestructor(R1);

          Bar *bar = new (R0) Bar(...);
          R0.registerDestructor(bar);      // first destroyed

          Baz *baz = new (*R1) Baz(...);
          R1->registerDestructor(baz);      // second destroyed (while destroying R1)
       \endverbatim
     *
     * \param[in] obj The object that should eventually be destroyed. It must
     *                have been allocated by this Region.
     *
     */
    template<typename T> void registerDestructor(T *obj)
    {
        _lastDestroyer = new (*this) TypedDestroyer<T>(_lastDestroyer, obj);
    }

    void deallocate(void *allocation, size_t = 0) throw();

    static void reset(TR::Region &targetRegion, TR::Region &prototypeRegion)
    {
        targetRegion.~Region();
        new (&targetRegion) TR::Region(prototypeRegion);
    }

    friend bool operator==(const TR::Region &lhs, const TR::Region &rhs) { return &lhs == &rhs; }

    friend bool operator!=(const TR::Region &lhs, const TR::Region &rhs) { return !operator==(lhs, rhs); }

    // Enable automatic conversion into a form compatible with C++ standard library containers
    template<typename T> operator TR::typed_allocator<T, Region &>() { return TR::typed_allocator<T, Region &>(*this); }

    size_t bytesAllocated() { return _bytesAllocated; }

    static size_t initialSize() { return INITIAL_SEGMENT_SIZE; }

private:
    friend class TR::RegionProfiler;

    size_t _bytesAllocated;
    TR::SegmentProvider &_segmentProvider;
    TR::RawAllocator _rawAllocator;
    TR::MemorySegment _initialSegment;
    TR::reference_wrapper<TR::MemorySegment> _currentSegment;

    Destroyer *_lastDestroyer;

    static const size_t INITIAL_SEGMENT_SIZE = 4096;

    union {
        char data[INITIAL_SEGMENT_SIZE];
        unsigned long long alignment;
    } _initialSegmentArea;
};

} // namespace TR

inline void *operator new(size_t size, TR::Region &region) { return region.allocate(size); }

inline void operator delete(void *p, TR::Region &region) { region.deallocate(p); }

inline void *operator new[](size_t size, TR::Region &region) { return region.allocate(size); }

inline void operator delete[](void *p, TR::Region &region) { region.deallocate(p); }

#endif // OMR_REGION_HPP
