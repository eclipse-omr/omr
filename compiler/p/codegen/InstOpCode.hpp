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

#ifndef TR_POWER_INSTOPCODE_INCL
#define TR_POWER_INSTOPCODE_INCL

#include "codegen/OMRInstOpCode.hpp"

#ifndef OPS_MAX
#define OPS_MAX TR::InstOpCode::NumOpCodes
#endif

namespace TR {

class InstOpCode : public OMR::InstOpCodeConnector {
public:
    InstOpCode()
        : OMR::InstOpCodeConnector(bad)
    {}

    InstOpCode(TR::InstOpCode::Mnemonic m)
        : OMR::InstOpCodeConnector(m)
    {}

    TR::InstOpCode::Mnemonic getOpCodeValue() { return _mnemonic; }

    TR::InstOpCode::Mnemonic setOpCodeValue(TR::InstOpCode::Mnemonic op) { return (_mnemonic = op); }

    TR::InstOpCode::Mnemonic getRecordFormOpCodeValue() { return (TR::InstOpCode::Mnemonic)(_mnemonic + 1); }
};
} // namespace TR
#endif
