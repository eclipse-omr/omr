/*******************************************************************************
 * Copyright IBM Corp. and others 2026
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
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH
 * Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include "CaseNodeConverter.hpp"
#include "ilgen.hpp"
#include "ras/Logger.hpp"

namespace Tril {

TR::Node *CaseNodeConverter::impl(const ASTNode *tree, IlGenState *state) {
  // Check if this is a case node
  if (strcmp("case", tree->getName()) != 0) {
    return NULL;
  }

  TraceIL("Creating case node from ASTNode %p\n", tree);

  // Get the target block
  const auto targetArg = tree->getArgByName("target");
  if (targetArg == NULL) {
    TraceIL("  case node missing required 'target' argument\n");
    throw CaseNodeGenError("case node missing required 'target' argument");
  }

  const auto targetName = targetArg->getValue()->getString();
  auto targetId = state->findBlockByName(targetName);
  auto targetEntry = state->blocks()[targetId]->getEntry();
  TraceIL("  case target block %d (\"%s\", entry = %p)\n", targetId, targetName,
          targetEntry);

  // Check if this is a case with a value (not default)
  const auto valueArg = tree->getArgByName("value");
  if (valueArg != NULL) {
    // This is a regular case with a constant value
    int32_t caseValue = valueArg->getValue()->get<int32_t>();
    TR::Node *node = TR::Node::createCase(0, targetEntry, caseValue);
    TraceIL("  created case node with value %d\n", caseValue);
    return node;
  } else {
    // This is the default case - case constant of 0 for default
    TR::Node *node = TR::Node::createCase(0, targetEntry, 0);
    TraceIL("  created default case node\n", "");
    return node;
  }

} // namespace Tril

// Made with Bob
