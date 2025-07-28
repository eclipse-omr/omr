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

#ifndef OMR_CHECKLIST_INCL
#define OMR_CHECKLIST_INCL

#ifdef OMRZTPF
#define __TPF_DO_NOT_MAP_ATOE_REMOVE
#endif

#include "infra/BitVector.hpp"

class TR_BitVector;

namespace TR {

class Block;
class Compilation;
class Node;

class Checklist {
protected:
    TR::Compilation * const _comp;
    TR_BitVector * const _v;

    // These will make the right thing happen in the implicitly-declared copy
    // constructors and copy assignment operators of subtypes. Keeping them
    // protected guards against object slicing.
    Checklist(const Checklist &other);
    Checklist &operator=(const Checklist &cl);

private:
    TR_BitVector *allocBV();

public:
    explicit Checklist(TR::Compilation *c);
    ~Checklist();

    bool isEmpty() const { return _v->isEmpty(); }

    void clear() { _v->empty(); }

    void print() const { _v->print(_comp); }
};

class NodeChecklist : public Checklist {
public:
    explicit NodeChecklist(TR::Compilation *c);
    bool contains(TR::Node *n) const;
    void add(TR::Node *n);
    void remove(TR::Node *n);
    bool contains(const NodeChecklist &other) const;
    void add(const NodeChecklist &other);
    void remove(const NodeChecklist &other);
    bool operator==(const NodeChecklist &other) const;

    bool operator!=(const NodeChecklist &other) const { return !operator==(other); }
};

class BlockChecklist : public Checklist {
public:
    explicit BlockChecklist(TR::Compilation *c);
    bool contains(TR::Block *b) const;
    void add(TR::Block *b);
    void remove(TR::Block *b);
    bool contains(const BlockChecklist &other) const;
    void add(const BlockChecklist &other);
    void remove(const BlockChecklist &other);
    bool operator==(const BlockChecklist &other) const;

    bool operator!=(const BlockChecklist &other) const { return !operator==(other); }
};

} // namespace TR

#endif
