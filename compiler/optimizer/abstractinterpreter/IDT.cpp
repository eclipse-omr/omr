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

#include "optimizer/abstractinterpreter/IDT.hpp"
#include "infra/String.hpp"

TR::IDT::IDT(TR::Region &region, TR_CallTarget *callTarget, TR::ResolvedMethodSymbol *symbol, uint32_t budget,
    TR::Compilation *comp)
    : _region(region)
    , _nextIdx(-1)
    , _comp(comp)
    , _root(new(_region) IDTNode(getNextGlobalIDTNodeIndex(), callTarget, symbol, -1, 1, NULL, budget))
    , _indices(NULL)
    , _totalCost(0)
{
    increaseGlobalIDTNodeIndex();
}

void TR::IDT::print()
{
    bool verboseInlining = comp()->getOptions()->getVerboseOption(TR_VerboseInlining);
    bool traceBIIDTGen = comp()->getOption(TR_TraceBIIDTGen);

    if (!verboseInlining && !traceBIIDTGen)
        return;
    const uint32_t candidates = getNumNodes() - 1;
    // print header line
    TR::StringBuf line(comp()->trMemory()->currentStackRegion());
    line.appendf("#IDT: %d candidate methods inlinable into %s with a budget %d", candidates,
        getRoot()->getName(comp()->trMemory()), getRoot()->getBudget());

    TR_VerboseLog::CriticalSection vlogLock(verboseInlining);
    if (verboseInlining) {
        TR_VerboseLog::writeLine(TR_Vlog_BI, "%s", line.text());
    }
    if (traceBIIDTGen)
        traceMsg(comp(), "%s\n", line.text());

    if (candidates <= 0)
        return;

    // print the IDT nodes in BFS
    TR::deque<TR::IDTNode *, TR::Region &> idtNodeQueue(comp()->trMemory()->currentStackRegion());

    idtNodeQueue.push_back(getRoot());
    while (!idtNodeQueue.empty()) {
        TR::IDTNode *currentNode = idtNodeQueue.front();
        idtNodeQueue.pop_front();

        int32_t index = currentNode->getGlobalIndex();

        // skip root node
        if (index != -1) {
            line.clear();
            line.appendf("#IDT: #%d: #%d inlinable @%d -> bcsz=%d %s target %s, static benefit = %d, benefit = %f, "
                         "cost = %d, budget = %d, callratio = %f, rootcallratio = %f",
                index, currentNode->getParentGlobalIndex(), currentNode->getByteCodeIndex(),
                currentNode->getByteCodeSize(), currentNode->getResolvedMethodSymbol()->signature(comp()->trMemory()),
                currentNode->getName(comp()->trMemory()), currentNode->getStaticBenefit(), currentNode->getBenefit(),
                currentNode->getCost(), currentNode->getBudget(), currentNode->getCallRatio(),
                currentNode->getRootCallRatio());
            if (verboseInlining)
                TR_VerboseLog::writeLine(TR_Vlog_BI, "%s", line.text());

            if (traceBIIDTGen)
                traceMsg(comp(), "%s\n", line.text());
        }

        // process children
        for (uint32_t i = 0; i < currentNode->getNumChildren(); i++)
            idtNodeQueue.push_back(currentNode->getChild(i));
    }
}

void TR::IDT::flattenIDT()
{
    if (_indices != NULL)
        return;

    // initialize nodes index array
    uint32_t numNodes = getNumNodes();
    _indices = new (_region) TR::IDTNode *[numNodes]();

    // add all the descendents of the root node to the indices array
    TR::deque<TR::IDTNode *, TR::Region &> idtNodeQueue(comp()->trMemory()->currentStackRegion());
    idtNodeQueue.push_back(getRoot());

    while (!idtNodeQueue.empty()) {
        TR::IDTNode *currentNode = idtNodeQueue.front();
        idtNodeQueue.pop_front();

        const int32_t index = currentNode->getGlobalIndex();
        TR_ASSERT_FATAL(_indices[index + 1] == 0, "Callee index not unique!\n");

        _indices[index + 1] = currentNode;

        for (uint32_t i = 0; i < currentNode->getNumChildren(); i++) {
            idtNodeQueue.push_back(currentNode->getChild(i));
        }
    }
}

TR::IDTNode *TR::IDT::getNodeByGlobalIndex(int32_t index)
{
    TR_ASSERT_FATAL(_indices, "Call flattenIDT() first");
    TR_ASSERT_FATAL(index < getNextGlobalIDTNodeIndex(), "Index out of range!");
    TR_ASSERT_FATAL(index >= -1, "Index too low!");
    return _indices[index + 1];
}

TR::IDTPriorityQueue::IDTPriorityQueue(TR::IDT *idt, TR::Region &region)
    : _entries(region)
    , _idt(idt)
    , _pQueue(IDTNodeCompare(), IDTNodeVector(region))
{
    _pQueue.push(idt->getRoot());
}

TR::IDTNode *TR::IDTPriorityQueue::get(uint32_t index)
{
    const size_t entriesSize = _entries.size();

    const uint32_t idtSize = size();
    TR_ASSERT_FATAL(index < idtSize, "IDTPriorityQueue::get index out of bound!");
    // already in entries
    if (entriesSize > index)
        return _entries.at(index);

    // not in entries yet. Update entries.
    while (_entries.size() <= index) {
        TR::IDTNode *newEntry = _pQueue.top();
        _pQueue.pop();

        _entries.push_back(newEntry);
        for (uint32_t j = 0; j < newEntry->getNumChildren(); j++) {
            _pQueue.push(newEntry->getChild(j));
        }
    }

    return _entries.at(index);
}
