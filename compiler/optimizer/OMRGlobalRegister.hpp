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

#ifndef OMR_GLOBALREGISTER_INCL
#define OMR_GLOBALREGISTER_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_GLOBALREGISTER_CONNECTOR
#define OMR_GLOBALREGISTER_CONNECTOR

namespace OMR {
class GlobalRegister;
typedef OMR::GlobalRegister GlobalRegisterConnector;
} // namespace OMR
#endif

#include "il/Node.hpp"

namespace TR {
class RegisterCandidate;
class TreeTop;
class Block;
class Compilation;
class GlobalRegister;
} // namespace TR

class TR_NodeMappings;
class TR_GlobalRegisterAllocator;

namespace OMR {

class OMR_EXTENSIBLE GlobalRegister {
public:
    TR_ALLOC(TR_Memory::GlobalRegister)

    // no ctor so that it can be zero initialized by the TR_Array template

    inline TR::GlobalRegister *self();

    TR::RegisterCandidate *getRegisterCandidateOnEntry() { return _rcOnEntry; }

    TR::RegisterCandidate *getRegisterCandidateOnExit() { return _rcOnExit; }

    TR::RegisterCandidate *getCurrentRegisterCandidate() { return _rcCurrent; }

    TR::TreeTop *getLastRefTreeTop() { return _lastRef; }

    bool getAutoContainsRegisterValue();

    TR::Node *getValue() { return _value; }

    TR_NodeMappings &getMappings() { return _mappings; }

    TR::TreeTop *optimalPlacementForStore(TR::Block *, TR::Compilation *);

    void setRegisterCandidateOnEntry(TR::RegisterCandidate *rc) { _rcOnEntry = rc; }

    void setRegisterCandidateOnExit(TR::RegisterCandidate *rc) { _rcOnExit = rc; }

    void setCurrentRegisterCandidate(TR::RegisterCandidate *rc, vcount_t, TR::Block *, int32_t, TR::Compilation *,
        bool resetOtherHalfOfLong = true);
    // void setCurrentLongRegisterCandidate(TR::RegisterCandidate * rc, TR::GlobalRegister *otherReg);
    void copyCurrentRegisterCandidate(TR::GlobalRegister *);

    void setValue(TR::Node *n) { _value = n; }

    void setAutoContainsRegisterValue(bool b) { _autoContainsRegisterValue = b; }

    void setLastRefTreeTop(TR::TreeTop *tt) { _lastRef = tt; }

    void setReloadRegisterCandidateOnEntry(TR::RegisterCandidate *rc, bool flag = true)
    {
        if (rc == _rcOnEntry)
            _reloadOnEntry = flag;
    }

    bool getReloadRegisterCandidateOnEntry() { return _reloadOnEntry; }

    void setUnavailable(bool flag = true) { _unavailable = flag; }

    void setUnavailableResolved(bool flag = true) { _unavailableResolved = flag; }

    bool isUnavailable() { return _unavailable; }

    bool isUnavailableResolved() { return _unavailableResolved; }

    TR::Node *createStoreFromRegister(vcount_t, TR::TreeTop *, int32_t, TR::Compilation *,
        bool storeUnconditionally = false);
    TR::Node *createStoreToRegister(TR::TreeTop *, TR::Node *, vcount_t, TR::Compilation *,
        TR_GlobalRegisterAllocator *);
    TR::Node *createLoadFromRegister(TR::Node *, TR::Compilation *);

protected:
    TR::RegisterCandidate *_rcOnEntry;
    TR::RegisterCandidate *_rcOnExit;
    TR::RegisterCandidate *_rcCurrent;
    TR::Node *_value;
    TR::TreeTop *_lastRef;
    bool _autoContainsRegisterValue;
    bool _reloadOnEntry; // reload the register as this is first block with stack available to reload killed reg
    bool _unavailable; // Live register killed but stack unavailable making it impossible to reload
    bool _unavailableResolved; // register is unavailable but a reload was found in all successors
    TR_NodeMappings _mappings;
};

} // namespace OMR

#endif
