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

#ifndef OMR_SNIPPETGCMAP_INCL
#define OMR_SNIPPETGCMAP_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_SNIPPETGCMAP_CONNECTOR
#define OMR_SNIPPETGCMAP_CONNECTOR

namespace OMR {
class SnippetGCMap;
typedef OMR::SnippetGCMap SnippetGCMapConnector;
} // namespace OMR
#endif

#include "infra/Flags.hpp"

namespace TR {
class Instruction;
class CodeGenerator;
} // namespace TR
class TR_GCStackMap;

namespace OMR {

class SnippetGCMap {
public:
    SnippetGCMap()
        : _flags(0)
        , _GCRegisterMask(0)
        , _stackMap(0)
    {}

    TR_GCStackMap *getStackMap() { return _stackMap; }

    void setStackMap(TR_GCStackMap *m) { _stackMap = m; }

    uint32_t getGCRegisterMask() { return _GCRegisterMask; }

    void setGCRegisterMask(uint32_t regMask) { _GCRegisterMask = regMask; }

    bool isGCSafePoint() { return _flags.testAll(TO_MASK8(GCSafePoint)); }

    void setGCSafePoint() { _flags.set(TO_MASK8(GCSafePoint)); }

    void resetGCSafePoint() { _flags.reset(TO_MASK8(GCSafePoint)); }

    void registerStackMap(TR::Instruction *instruction, TR::CodeGenerator *cg);
    void registerStackMap(uint8_t *callSiteAddress, TR::CodeGenerator *cg);

    enum {
        GCSafePoint = 0
    };

protected:
    flags8_t _flags;
    uint32_t _GCRegisterMask;
    TR_GCStackMap *_stackMap;
};

} // namespace OMR

#endif
