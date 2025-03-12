/*******************************************************************************
 * Copyright IBM Corp. and others 2024
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

#include "env/OMRRetainedMethodSet.hpp"

#include "compile/Compilation.hpp"
#include "compile/ResolvedMethod.hpp"
#include "infra/Assert.hpp"

OMR::RetainedMethodSet::RetainedMethodSet(
   TR::Compilation *comp,
   TR_ResolvedMethod *method,
   OMR::RetainedMethodSet *parent)
   : _comp(comp)
   , _method(method)
   , _parent(parent)
   , _keepaliveMethods(comp->trMemory()->heapMemoryRegion())
   , _bondMethods(comp->trMemory()->heapMemoryRegion())
   {
   // empty
   }

void
OMR::RetainedMethodSet::keepalive()
   {
   flatten(FlattenMode_Keepalive);
   }

void
OMR::RetainedMethodSet::bond()
   {
   flatten(FlattenMode_Bond);
   }

void
OMR::RetainedMethodSet::flatten(FlattenMode mode)
   {
   TR_ASSERT_FATAL(_parent != NULL, "cannot flatten the root set");

   if (_method == NULL)
      {
      return; // already done
      }

   const char *flattenKindName = NULL; // only for tracing
   if (_comp->getOption(TR_TraceRetainedMethods))
      {
      flattenKindName = mode == FlattenMode_Keepalive ? "keepalive" : "bond";
      traceMsg(
         _comp,
         "RetainedMethodSet %p: %s %p %.*s.%.*s%.*s\n",
         this,
         flattenKindName,
         _method->getNonPersistentIdentifier(),
         _method->classNameLength(),
         _method->classNameChars(),
         _method->nameLength(),
         _method->nameChars(),
         _method->signatureLength(),
         _method->signatureChars());
      }

   // If keepalive has already happened for the parent, then anything
   // propagated to the parent now would fail to be further propagated up
   // toward the root set.
   TR_ASSERT_FATAL(_parent->_method != NULL, "must keepalive in bottom up order");

   if (!_parent->willRemainLoaded(_method))
      {
      void *key = unloadingKey(_method);
      if (mode == FlattenMode_Keepalive)
         {
         _parent->_keepaliveMethods.insert(std::make_pair(key, _method));
         }
      else
         {
         _parent->_bondMethods.insert(std::make_pair(key, _method));
         }

      if (_comp->getOption(TR_TraceRetainedMethods))
         {
         traceMsg(
            _comp,
            "RetainedMethodSet %p: added %s method %p (key %p)\n",
            _parent,
            flattenKindName,
            _method->getNonPersistentIdentifier(),
            key);
         }
      }

   _parent->propagateMethods(_parent->_keepaliveMethods, _keepaliveMethods);
   _parent->propagateMethods(_parent->_bondMethods, _bondMethods);

   // If unloadable methods are allowed to be inlined without a keepalive, then
   // they will be inlined with a bond. Keepalives only matter if all bonds are
   // satisfied, so it's important to ignore keepalives when determining what
   // will remain loaded without a bond (for the purpose of determining whether
   // to create a bond).
   //
   // NOTE: When TR_DontInlineUnloadableMethods is set, we still have to skip
   // this for keepalives, even though there won't be any bonds. Otherwise,
   // assignKeepaliveConstRefLabels() will think all keepalives are redundant.
   //
   if (mode == FlattenMode_Bond)
      {
      // Merge even if the parent already tells us that _method is guaranteed to
      // remain loaded. There may have been code attested to remain loaded in this
      // set but not yet in the parent.
      mergeIntoParent();
      TR_ASSERT_FATAL(
         _parent->willRemainLoaded(_method),
         "method should now be guaranteed to remain loaded");
      }

   _method = NULL;
   }

OMR::RetainedMethodSet *
OMR::RetainedMethodSet::createChild(TR_ResolvedMethod *method)
   {
   TR_ASSERT_FATAL(false, "unimplemented: OMR::RetainedMethodSet::createChild");
   }

OMR::RetainedMethodSet *
OMR::RetainedMethodSet::withKeepalivesAttested()
   {
   return this;
   }

void
OMR::RetainedMethodSet::mergeIntoParent()
   {
   TR_ASSERT_FATAL(false, "unimplemented: OMR::RetainedMethodSet::mergeIntoParent");
   }

void *
OMR::RetainedMethodSet::unloadingKey(TR_ResolvedMethod *method)
   {
   return method->getNonPersistentIdentifier();
   }

void
OMR::RetainedMethodSet::propagateMethods(MethodMap &dest, const MethodMap &src)
   {
   for (auto it = src.begin(), end = src.end(); it != end; it++)
      {
      void *key = it->first;
      TR_ResolvedMethod *method = it->second;
      if (!willRemainLoaded(method))
         {
         dest.insert(std::make_pair(key, method));
         }
      }
   }
