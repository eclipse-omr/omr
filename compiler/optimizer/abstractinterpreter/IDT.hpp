/*******************************************************************************
 * Copyright IBM Corp. and others 2020
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

#ifndef IDT_INCL
#define IDT_INCL

#include "compile/Compilation.hpp"
#include "optimizer/CallInfo.hpp"
#include "optimizer/abstractinterpreter/IDTNode.hpp"
#include "env/Region.hpp"
#include "env/VerboseLog.hpp"
#include "il/ResolvedMethodSymbol.hpp"
#include <queue>

namespace TR {

/**
 * IDT stands for Inlining Dependency Tree
 * It is a structure that holds all candidate methods to be inlined.
 *
 * The parent-child relationship in the IDT corresponds to the caller-callee relationship.
 */
class IDT {
public:
    IDT(TR::Region &region, TR_CallTarget *, TR::ResolvedMethodSymbol *symbol, uint32_t budget, TR::Compilation *comp);

    TR::IDTNode *getRoot() { return _root; }

    TR::Region &getRegion() { return _region; }

    void addCost(uint32_t cost) { _totalCost += cost; }

    uint32_t getTotalCost() { return _totalCost; }

    /**
     * @brief Get the total number of nodes in this IDT.
     *
     * @return the total number of node
     */
    uint32_t getNumNodes() { return _nextIdx + 1; }

    /**
     * @brief Get the next avaible IDTNode index.
     *
     * @return the next index
     */
    int32_t getNextGlobalIDTNodeIndex() { return _nextIdx; }

    /**
     * @brief Increase the next available IDTNode index by 1.
     * This should only be called when successfully adding an IDTNode to the IDT
     */
    void increaseGlobalIDTNodeIndex() { _nextIdx++; }

    /**
     * @brief Get the IDTNode using index.
     * Before using this method for accessing IDTNode, flattenIDT() must be called.
     *
     * @return the IDT node
     */
    TR::IDTNode *getNodeByGlobalIndex(int32_t index);

    /**
     * @brief Flatten all the IDTNodes into a list.
     */
    void flattenIDT();

    void print();

private:
    TR::Compilation *comp() { return _comp; }

    TR::Compilation *_comp;
    TR::Region &_region;
    int32_t _nextIdx;
    uint32_t _totalCost;
    TR::IDTNode *_root;
    TR::IDTNode **_indices;
};

/**
 * A topological sort of the IDT in which the lowest-benefit node is listed first and lowest cost used to
 * break the tie whenever there is a choice.
 *
 * This topological sort is to prepare an ordered list of inlining options for the algorithm described
 * in following patent:
 * https://patents.google.com/patent/US10055210B2/en
 *
 * This patent describes the topological sort as the following quote:
 * "constraints on the order of node consideration, where the constraints serve to ensure every node is
 * considered only after all of the node's ancestors are considered, and nodes are included in order of
 * increasing cumulative benefit with lowest cumulative cost used to break ties to allow the generation
 * of all intermediate solutions at a given cost budget to simplify backtracking in subsequent iterations
 * of the algorithm".
 */
class IDTPriorityQueue {
public:
    IDTPriorityQueue(TR::IDT *idt, TR::Region &region);

    uint32_t size() { return _idt->getNumNodes(); }

    TR::IDTNode *get(uint32_t index);

private:
    struct IDTNodeCompare {
        bool operator()(TR::IDTNode *left, TR::IDTNode *right)
        {
            TR_ASSERT_FATAL(left && right, "Comparing against null");
            if (left->getBenefit() == right->getBenefit())
                return left->getCost() > right->getCost();
            else
                return left->getBenefit() > right->getBenefit();
        }
    };

    typedef TR::vector<IDTNode *, TR::Region &> IDTNodeVector;
    typedef std::priority_queue<IDTNode *, IDTNodeVector, IDTNodeCompare> IDTNodePriorityQueue;

    TR::IDT *_idt;
    IDTNodePriorityQueue _pQueue;
    IDTNodeVector _entries;
};
} // namespace TR

#endif
