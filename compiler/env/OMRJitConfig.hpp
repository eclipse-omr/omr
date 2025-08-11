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

#ifndef OMR_JITCONFIG_INCL
#define OMR_JITCONFIG_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_JITCONFIG_CONNECTOR
#define OMR_JITCONFIG_CONNECTOR
namespace OMR {
class JitConfig;
typedef OMR::JitConfig JitConfigConnector;
}
#endif

namespace TR {
class JitConfig;
}

#include <stddef.h>
#include <stdint.h>
#include "infra/Annotations.hpp"
#include "env/IO.hpp"

namespace OMR
{

class OMR_EXTENSIBLE JitConfig
   {
protected:

   JitConfig();

public:

   static TR::JitConfig *instance();

   TR::JitConfig *self();

   // possibly temporary place for options to be stored?
   struct
      {
      int32_t codeCacheKB;
      char *vLogFileName;
      TR::FILE *vLogFile;
      uint64_t verboseFlags;
      } options;

   void setInterpreterTOC(size_t interpreterTOC) { _interpreterTOC = interpreterTOC; }
   size_t getInterpreterTOC()                    { return _interpreterTOC; }

   void *getPseudoTOC()               { return _pseudoTOC; }
   void setPseudoTOC(void *pseudoTOC) { _pseudoTOC = pseudoTOC; }

private:

   char _eyecatcher[8];

   void *_processorInfo;

   size_t _interpreterTOC;

   void *_pseudoTOC; // only used on POWER, otherwise should be NULL
   };

}

#endif
