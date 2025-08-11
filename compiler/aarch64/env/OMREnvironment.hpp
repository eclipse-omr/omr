/*******************************************************************************
 * Copyright IBM Corp. and others 2018
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

#ifndef OMR_ARM64_ENVIRONMENT_INCL
#define OMR_ARM64_ENVIRONMENT_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_ENVIRONMENT_CONNECTOR
#define OMR_ENVIRONMENT_CONNECTOR
namespace OMR {
namespace ARM64 { class Environment; }
typedef OMR::ARM64::Environment EnvironmentConnector;
}
#else
#error OMR::ARM64::Environment expected to be a primary connector, but an OMR connector is already defined
#endif

#include "compiler/env/OMREnvironment.hpp"


namespace OMR
{

namespace ARM64
{

class Environment : public OMR::Environment
   {
public:

   Environment() :
      OMR::Environment()
      {}

   Environment(TR::MajorOperatingSystem o, TR::Bitness b) :
      OMR::Environment(o, b)
      {}
      
   };

}

}
#endif //OMR_ARM64_ENVIRONMENT_INCL
