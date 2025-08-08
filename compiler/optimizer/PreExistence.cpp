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

#include "optimizer/PreExistence.hpp"

#include "env/CompilerEnv.hpp"
#include "env/KnownObjectTable.hpp"
#include "compile/Compilation.hpp"
#ifdef J9_PROJECT_SPECIFIC
#include "env/VMAccessCriticalSection.hpp"
#endif
#include "optimizer/Inliner.hpp"

PrexKnowledgeLevel TR_PrexArgument::knowledgeLevel(TR_PrexArgument *pa)
{
    if (!pa)
        return NONE;
    else if (pa->hasKnownObjectIndex())
        return KNOWN_OBJECT;
    else if (pa->classIsFixed())
        return FIXED_CLASS;
    else if (pa->classIsPreexistent())
        return PREEXISTENT;
    else
        return NONE;
}

TR_PrexArgument::TR_PrexArgument(TR::KnownObjectTable::Index knownObjectIndex, TR::Compilation *comp)
    : _classKind(ClassIsUnknown)
    , _class(0)
    , _profiledClazz(0)
    , _isTypeInfoForInlinedBody(false)
    , _knownObjectIndex(knownObjectIndex)
{
#ifdef J9_PROJECT_SPECIFIC
    auto knot = comp->getKnownObjectTable();
    if (knot && !knot->isNull(knownObjectIndex)) {
        TR::VMAccessCriticalSection prexArgumentCriticalSection(comp,
            TR::VMAccessCriticalSection::tryToAcquireVMAccess);

        if (prexArgumentCriticalSection.hasVMAccess()) {
            _class = TR::Compiler->cls.objectClass(comp, comp->getKnownObjectTable()->getPointer(knownObjectIndex));
            _classKind = ClassIsFixed;
        }
    }
#endif
}

void TR_PrexArgInfo::dumpTrace()
{
    TR::Compilation *comp = TR::comp();
    traceMsg(comp, "<argInfo address = %p numArgs = %d>\n", this, getNumArgs());
    for (int i = 0; i < getNumArgs(); i++) {
        TR_PrexArgument *arg = get(i);
        if (arg && arg->getClass()) {
            char *className = TR::Compiler->cls.classSignature(comp, arg->getClass(), comp->trMemory());
            traceMsg(comp,
                "<Argument no=%d address=%p classIsFixed=%d classIsPreexistent=%d argIsKnownObject=%d koi=%d class=%p "
                "className= %s/>\n",
                i, arg, arg->classIsFixed(), arg->classIsPreexistent(), arg->hasKnownObjectIndex(),
                arg->getKnownObjectIndex(), arg->getClass(), className);
        } else {
            traceMsg(comp, "<Argument no=%d address=%p classIsFixed=%d classIsPreexistent=%d/>\n", i, arg,
                arg ? arg->classIsFixed() : 0, arg ? arg->classIsPreexistent() : 0);
        }
    }
    traceMsg(comp, "</argInfo>\n");
}

/**
 * \brief
 *    Given two `TR_PrexArgument`, return the one with more concrete argument info
 */
static TR_PrexArgument *strongerArgumentInfo(TR_PrexArgument *left, TR_PrexArgument *right, TR::Compilation *comp)
{
    if (TR_PrexArgument::knowledgeLevel(left) > TR_PrexArgument::knowledgeLevel(right))
        return left;
    else if (TR_PrexArgument::knowledgeLevel(right) > TR_PrexArgument::knowledgeLevel(left))
        return right;
    else if (left && right) {
        if (left->getClass() && right->getClass()) {
            if (comp->fe()->isInstanceOf(left->getClass(), right->getClass(), true, true, false))
                return left;
            else if (comp->fe()->isInstanceOf(right->getClass(), left->getClass(), true, true, false))
                return right;
        } else if (left->getClass())
            return left;
        else if (right->getClass())
            return right;

        return NULL;
    } else
        return left ? left : right; // Return non-null prex argument when possible
}

TR_PrexArgInfo *TR_PrexArgInfo::enhance(TR_PrexArgInfo *dest, TR_PrexArgInfo *source, TR::Compilation *comp)
{
    // If dest is NULL, we can't simply return source, as TR_PrexArgInfo is mutable, any
    // future change to dest will change source. Thus return a copy of source
    //
    if (!dest && source)
        return new (comp->trHeapMemory()) TR_PrexArgInfo(source, comp->trMemory());
    else if (!source)
        return dest;

    TR_ASSERT(dest->getNumArgs() == source->getNumArgs(),
        "Number of arguments don't match: dest %p %d arguments and source %p %d arguments", dest, dest->getNumArgs(),
        source, source->getNumArgs());

    auto numArgsToEnhance = dest->getNumArgs();
    for (int32_t i = 0; i < numArgsToEnhance; i++) {
        TR_PrexArgument *result = strongerArgumentInfo(dest->get(i), source->get(i), comp);
        if (result)
            dest->set(i, result);
    }

    return dest;
}

