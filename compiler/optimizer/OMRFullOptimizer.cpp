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

#include "optimizer/FullOptimizer.hpp"
#include "optimizer/Optimizer.hpp"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "codegen/CodeGenerator.hpp"
#include "env/FrontEnd.hpp"
#include "compile/CompilationTypes.hpp"
#include "compile/Method.hpp"
#include "compile/SymbolReferenceTable.hpp"
#include "control/Options.hpp"
#include "control/Options_inlines.hpp"
#include "control/Recompilation.hpp"
#ifdef J9_PROJECT_SPECIFIC
#include "control/RecompilationInfo.hpp"
#endif
#include "env/CompilerEnv.hpp"
#include "env/IO.hpp"
#include "env/PersistentInfo.hpp"
#include "env/StackMemoryRegion.hpp"
#include "env/TRMemory.hpp"
#include "env/jittypes.h"
#include "il/Block.hpp"
#include "il/DataTypes.hpp"
#include "il/ILOpCodes.hpp"
#include "il/ILOps.hpp"
#include "il/Node.hpp"
#include "il/NodePool.hpp"
#include "il/Node_inlines.hpp"
#include "il/ResolvedMethodSymbol.hpp"
#include "il/Symbol.hpp"
#include "il/SymbolReference.hpp"
#include "il/TreeTop.hpp"
#include "il/TreeTop_inlines.hpp"
#include "infra/Assert.hpp"
#include "infra/BitVector.hpp"
#include "infra/Cfg.hpp"
#include "infra/List.hpp"
#include "infra/SimpleRegex.hpp"
#include "infra/CfgNode.hpp"
#include "infra/Timer.hpp"
#include "optimizer/LoadExtensions.hpp"
#include "optimizer/Optimization.hpp"
#include "optimizer/OptimizationManager.hpp"
#include "optimizer/OptimizationStrategies.hpp"
#include "optimizer/Optimizations.hpp"
#include "optimizer/Structure.hpp"
#include "optimizer/StructuralAnalysis.hpp"
#include "optimizer/UseDefInfo.hpp"
#include "optimizer/ValueNumberInfo.hpp"
#include "optimizer/AsyncCheckInsertion.hpp"
#include "optimizer/DeadStoreElimination.hpp"
#include "optimizer/DeadTreesElimination.hpp"
#include "optimizer/CatchBlockRemover.hpp"
#include "optimizer/CFGSimplifier.hpp"
#include "optimizer/CompactLocals.hpp"
#include "optimizer/CopyPropagation.hpp"
#include "optimizer/ExpressionsSimplification.hpp"
#include "optimizer/GeneralLoopUnroller.hpp"
#include "optimizer/LocalCSE.hpp"
#include "optimizer/LocalDeadStoreElimination.hpp"
#include "optimizer/LocalLiveRangeReducer.hpp"
#include "optimizer/LocalOpts.hpp"
#include "optimizer/LocalReordering.hpp"
#include "optimizer/LoopCanonicalizer.hpp"
#include "optimizer/LoopReducer.hpp"
#include "optimizer/LoopReplicator.hpp"
#include "optimizer/LoopVersioner.hpp"
#include "optimizer/OrderBlocks.hpp"
#include "optimizer/RedundantAsyncCheckRemoval.hpp"
#include "optimizer/Simplifier.hpp"
#include "optimizer/VirtualGuardCoalescer.hpp"
#include "optimizer/VirtualGuardHeadMerger.hpp"
#include "optimizer/Inliner.hpp"
#include "ras/Debug.hpp"
#include "optimizer/InductionVariable.hpp"
#include "optimizer/GlobalValuePropagation.hpp"
#include "optimizer/LocalValuePropagation.hpp"
#include "optimizer/RegDepCopyRemoval.hpp"
#include "optimizer/SinkStores.hpp"
#include "optimizer/PartialRedundancy.hpp"
#include "optimizer/OSRDefAnalysis.hpp"
#include "optimizer/StripMiner.hpp"
#include "optimizer/FieldPrivatizer.hpp"
#include "optimizer/ReorderIndexExpr.hpp"
#include "optimizer/GlobalRegisterAllocator.hpp"
#include "optimizer/RecognizedCallTransformer.hpp"
#include "optimizer/SwitchAnalyzer.hpp"
#include "env/RegionProfiler.hpp"

static const OptimizationStrategy localValuePropagationOpts[] = {
    { OMR::localCSE },
    { OMR::localValuePropagation },
    { OMR::localCSE, OMR::IfEnabled },
    { OMR::localValuePropagation, OMR::IfEnabled },
    { OMR::endGroup }
};

static const OptimizationStrategy arrayPrivatizationOpts[] = {
    { OMR::globalValuePropagation, OMR::IfMoreThanOneBlock }, // reduce # of null/bounds checks and setup iv info
    { OMR::veryCheapGlobalValuePropagationGroup, OMR::IfEnabled }, // enabled by blockVersioner
    { OMR::inductionVariableAnalysis, OMR::IfLoops },
    { OMR::loopCanonicalization, OMR::IfLoops }, // setup for any unrolling in arrayPrivatization
    { OMR::treeSimplification }, // get rid of null/bnd checks if possible
    { OMR::deadTreesElimination },
    { OMR::basicBlockOrdering, OMR::IfLoops }, // required for loop reduction
    { OMR::treesCleansing, OMR::IfLoops },
    { OMR::inductionVariableAnalysis, OMR::IfLoops }, // required for array Privatization
    { OMR::basicBlockOrdering, OMR::IfEnabled }, // cleanup if unrolling happened
    { OMR::globalValuePropagation, OMR::IfEnabledAndMoreThanOneBlock }, // ditto
    { OMR::endGroup }
};

// To be run just before PRE
static const OptimizationStrategy reorderArrayIndexOpts[] = {
    { OMR::inductionVariableAnalysis, OMR::IfLoops }, // need to id the primary IVs
    { OMR::reorderArrayIndexExpr, OMR::IfLoops }, // try to maximize loop invarient
                                                  // expressions in index calculations and be hoisted
    { OMR::endGroup }
};

/*temp for openj9 static*/ const OptimizationStrategy cheapObjectAllocationOpts[] = {
    { OMR::explicitNewInitialization, OMR::IfNews }, // do before local dead store
    { OMR::endGroup }
};

/*temp for openj9 static*/ const OptimizationStrategy expensiveObjectAllocationOpts[] = {
    { OMR::eachEscapeAnalysisPassGroup, OMR::IfEAOpportunities },
    { OMR::explicitNewInitialization, OMR::IfNews }, // do before local dead store
    { OMR::endGroup }
};

/*temp for openj9 static*/ const OptimizationStrategy eachEscapeAnalysisPassOpts[] = {
    { OMR::preEscapeAnalysis, OMR::IfOSR },
    { OMR::escapeAnalysis },
    { OMR::postEscapeAnalysis, OMR::IfOSR },
    { OMR::eachEscapeAnalysisPassGroup, OMR::IfEnabled }, // if another pass requested
    { OMR::endGroup }
};

static const OptimizationStrategy veryCheapGlobalValuePropagationOpts[] = {
    { OMR::globalValuePropagation, OMR::IfMoreThanOneBlock },
    { OMR::endGroup }
};

/*temp for openj9 static*/ const OptimizationStrategy cheapGlobalValuePropagationOpts[] = {
    { OMR::CFGSimplification, OMR::IfOptServer }, // for WAS trace folding
    { OMR::treeSimplification, OMR::IfOptServer }, // for WAS trace folding
    { OMR::localCSE, OMR::IfEnabledAndOptServer }, // for WAS trace folding
    { OMR::treeSimplification, OMR::IfEnabledAndOptServer }, // for WAS trace folding
    { OMR::globalValuePropagation, OMR::IfLoopsMarkLastRun },
    { OMR::treeSimplification, OMR::IfEnabled },
    { OMR::cheapObjectAllocationGroup },
    { OMR::treeSimplification, OMR::IfEnabled },
    { OMR::catchBlockRemoval, OMR::IfEnabled }, // if checks were removed
    { OMR::osrExceptionEdgeRemoval }, // most inlining is done by now
    { OMR::globalValuePropagation, OMR::IfEnabledAndMoreThanOneBlockMarkLastRun }, // mark monitors requiring sync
    { OMR::virtualGuardTailSplitter, OMR::IfEnabled }, // merge virtual guards
    { OMR::CFGSimplification },
    { OMR::endGroup }
};

/*temp for openj9 static*/ const OptimizationStrategy expensiveGlobalValuePropagationOpts[] = {
    { OMR::CFGSimplification, OMR::IfOptServer }, // for WAS trace folding
    { OMR::treeSimplification, OMR::IfOptServer }, // for WAS trace folding
    { OMR::localCSE, OMR::IfEnabledAndOptServer }, // for WAS trace folding
    { OMR::treeSimplification, OMR::IfEnabled }, // may be enabled by inner prex
    { OMR::globalValuePropagation, OMR::IfMoreThanOneBlock },
    { OMR::treeSimplification, OMR::IfEnabled },
    { OMR::deadTreesElimination }, // clean up left-over accesses before escape analysis
#ifdef J9_PROJECT_SPECIFIC
    { OMR::expensiveObjectAllocationGroup },
#endif
    { OMR::globalValuePropagation, OMR::IfEnabledAndMoreThanOneBlock }, // if inlined a call or an object
    { OMR::treeSimplification, OMR::IfEnabled },
    { OMR::catchBlockRemoval, OMR::IfEnabled }, // if checks were removed
    { OMR::osrExceptionEdgeRemoval }, // most inlining is done by now
#ifdef J9_PROJECT_SPECIFIC
    { OMR::redundantMonitorElimination, OMR::IfEnabled }, // performed if method has monitors
    { OMR::redundantMonitorElimination, OMR::IfEnabled }, // performed if method has monitors
#endif
    { OMR::globalValuePropagation, OMR::IfEnabledAndMoreThanOneBlock }, // mark monitors requiring sync
    { OMR::virtualGuardTailSplitter, OMR::IfEnabled }, // merge virtual guards
    { OMR::CFGSimplification },
    { OMR::endGroup }
};

static const OptimizationStrategy eachExpensiveGlobalValuePropagationOpts[] = {
    { OMR::globalValuePropagation, OMR::IfMoreThanOneBlock },
    { OMR::treeSimplification, OMR::IfEnabled },
    { OMR::veryCheapGlobalValuePropagationGroup, OMR::IfEnabled }, // enabled by blockversioner
    { OMR::deadTreesElimination }, // clean up left-over accesses before escape analysis
#ifdef J9_PROJECT_SPECIFIC
    { OMR::expensiveObjectAllocationGroup },
#endif
    { OMR::eachExpensiveGlobalValuePropagationGroup, OMR::IfEnabled }, // if inlining was done
    { OMR::endGroup }
};

static const OptimizationStrategy veryExpensiveGlobalValuePropagationOpts[] = {
    { OMR::eachExpensiveGlobalValuePropagationGroup },
    { OMR::localDeadStoreElimination }, // remove local/parm/some field stores
    { OMR::treeSimplification, OMR::IfEnabled },
    { OMR::catchBlockRemoval, OMR::IfEnabled }, // if checks were removed
    { OMR::osrExceptionEdgeRemoval }, // most inlining is done by now
#ifdef J9_PROJECT_SPECIFIC
    { OMR::redundantMonitorElimination, OMR::IfEnabled }, // performed if method has monitors
    { OMR::redundantMonitorElimination, OMR::IfEnabled }, // performed if method has monitors
#endif
    { OMR::globalValuePropagation, OMR::IfEnabledAndMoreThanOneBlock }, // mark monitors requiring syncs
    { OMR::virtualGuardTailSplitter, OMR::IfEnabled }, // merge virtual guards
    { OMR::CFGSimplification },
    { OMR::endGroup }
};

static const OptimizationStrategy partialRedundancyEliminationOpts[] = {
    { OMR::globalValuePropagation, OMR::IfMoreThanOneBlock }, // GVP (before PRE)
    { OMR::deadTreesElimination },
    { OMR::treeSimplification, OMR::IfEnabled },
    { OMR::treeSimplification }, // might fold expressions created by versioning/induction variables
    { OMR::treeSimplification, OMR::IfEnabled }, // Array length simplification shd be followed by reassoc before PRE
    { OMR::reorderArrayExprGroup, OMR::IfEnabled }, // maximize opportunities hoisting of index array expressions
    { OMR::partialRedundancyElimination, OMR::IfMoreThanOneBlock },
    {
     OMR::localCSE,
     }, // common up expression which can benefit EA
    { OMR::catchBlockRemoval, OMR::IfEnabled }, // if checks were removed
    { OMR::deadTreesElimination, OMR::IfEnabled }, // if checks were removed
    { OMR::compactNullChecks, OMR::IfEnabled }, // PRE creates explicit null checks in large numbers
    { OMR::localReordering, OMR::IfEnabled }, // PRE may create temp stores that can be moved closer to uses
    { OMR::globalValuePropagation, OMR::IfEnabledAndMoreThanOneBlockMarkLastRun }, // GVP (after PRE)
#ifdef J9_PROJECT_SPECIFIC
    { OMR::preEscapeAnalysis, OMR::IfOSR },
    { OMR::escapeAnalysis, OMR::IfEAOpportunitiesMarkLastRun }, // to stack-allocate after loopversioner and localCSE
    { OMR::postEscapeAnalysis, OMR::IfOSR },
#endif
    { OMR::basicBlockOrdering, OMR::IfLoops }, // early ordering with no extension
    { OMR::globalCopyPropagation, OMR::IfLoops }, // for Loop Versioner

    { OMR::loopVersionerGroup, OMR::IfEnabledAndLoops },
    { OMR::treeSimplification, OMR::IfEnabled }, // loop reduction block should be after PRE so that privatization
    { OMR::treesCleansing }, // clean up gotos in code and convert to fall-throughs for loop reducer
    { OMR::redundantGotoElimination,
     OMR::IfNotJitProfiling }, // clean up for loop reducer.  Note: NEVER run this before PRE
    { OMR::loopReduction, OMR::IfLoops }, // will have happened and it needs to be before loopStrider
    { OMR::localCSE, OMR::IfEnabled }, // so that it will not get confused with internal pointers.
    { OMR::globalDeadStoreElimination,
     OMR::IfEnabledAndMoreThanOneBlock }, // It may need to be run twice if deadstore elimination is required,
    {
     OMR::deadTreesElimination,
     }, // but this only happens for unsafe access (arraytranslate.twoToOne)
    {
     OMR::loopReduction,
     }, // and so is conditional
#ifdef J9_PROJECT_SPECIFIC
    { OMR::idiomRecognition, OMR::IfLoopsAndNotProfiling }, // after loopReduction!!
#endif
    { OMR::lastLoopVersionerGroup, OMR::IfLoops },
    {
     OMR::treeSimplification,
     }, // cleanup before AutoVectorization
    {
     OMR::deadTreesElimination,
     }, // cleanup before AutoVectorization
    { OMR::inductionVariableAnalysis, OMR::IfLoopsAndNotProfiling },
#ifdef J9_PROJECT_SPECIFIC
    { OMR::SPMDKernelParallelization, OMR::IfLoops },
#endif
    { OMR::loopStrider, OMR::IfLoops },
    { OMR::treeSimplification, OMR::IfEnabled },
    { OMR::lastLoopVersionerGroup, OMR::IfEnabledAndLoops },
    {
     OMR::treeSimplification,
     }, // cleanup before strider
    {
     OMR::localCSE,
     }, // cleanup before strider so it will not be confused by commoned nodes (mandatory to run local CSE before
    // strider)
    {
     OMR::deadTreesElimination,
     }, // cleanup before strider so that dead stores can be eliminated more effcientlly (i.e. false uses are not seen)
    { OMR::loopStrider, OMR::IfLoops },

    { OMR::treeSimplification, OMR::IfEnabled }, // cleanup after strider
    { OMR::loopInversion, OMR::IfLoops },
    { OMR::endGroup }
};

static const OptimizationStrategy methodHandleInvokeInliningOpts[] = {
    {
     OMR::treeSimplification,
     }, // Supply some known-object info, and help CSE
    {
     OMR::localCSE,
     }, // Especially copy propagation to replace temps with more descriptive trees
    { OMR::localValuePropagation }, // Propagate known-object info and derive more specific archetype specimen symbols
// for inlining
#ifdef J9_PROJECT_SPECIFIC
    {
     OMR::targetedInlining,
     },
#endif
    { OMR::deadTreesElimination },
    { OMR::methodHandleInvokeInliningGroup,
     OMR::IfEnabled }, // Repeat as required to inline all the MethodHandle.invoke calls we can afford
    { OMR::endGroup },
};

static const OptimizationStrategy earlyGlobalOpts[] = {
    { OMR::methodHandleInvokeInliningGroup, OMR::IfMethodHandleInvokes },
#ifdef J9_PROJECT_SPECIFIC
    { OMR::inlining },
#endif
    { OMR::osrExceptionEdgeRemoval }, // most inlining is done by now
    { OMR::treeSimplification, OMR::IfEnabled },
    { OMR::compactNullChecks }, // cleans up after inlining; MUST be done before PRE
#ifdef J9_PROJECT_SPECIFIC
    { OMR::virtualGuardTailSplitter }, // merge virtual guards
    { OMR::treeSimplification },
    { OMR::CFGSimplification },
#endif
    { OMR::endGroup }
};

static const OptimizationStrategy earlyLocalOpts[] = {
    { OMR::localValuePropagation },
    { OMR::localReordering },
    {
     OMR::switchAnalyzer,
     },
    { OMR::treeSimplification, OMR::IfEnabled }, // simplify any exprs created by LCP/LCSE
#ifdef J9_PROJECT_SPECIFIC
    { OMR::catchBlockRemoval }, // if all possible exceptions in a try were removed by inlining/LCP/LCSE
#endif
    { OMR::deadTreesElimination }, // remove any anchored dead loads
    { OMR::profiledNodeVersioning },
    { OMR::endGroup }
};

/*temp for openj9 static*/ const OptimizationStrategy isolatedStoreOpts[]
    = { { OMR::isolatedStoreElimination }, { OMR::deadTreesElimination }, { OMR::endGroup } };

static const OptimizationStrategy globalDeadStoreOpts[] = {
    { OMR::globalDeadStoreElimination, OMR::IfMoreThanOneBlock },
    { OMR::localDeadStoreElimination, OMR::IfOneBlock },
    { OMR::deadTreesElimination },
    { OMR::endGroup }
};

/*temp for openj9 static*/ const OptimizationStrategy loopAliasRefinerOpts[] = {
    { OMR::inductionVariableAnalysis, OMR::IfLoops },
    { OMR::loopCanonicalization },
    { OMR::globalValuePropagation, OMR::IfMoreThanOneBlock }, // create ivs
    { OMR::loopAliasRefiner },
    { OMR::endGroup }
};

/*temp for openj9 static*/ const OptimizationStrategy loopSpecializerOpts[] = {
    { OMR::inductionVariableAnalysis, OMR::IfLoops },
    { OMR::loopCanonicalization },
    { OMR::loopSpecializer },
    { OMR::endGroup }
};

static const OptimizationStrategy loopVersionerOpts[] = {
    { OMR::basicBlockOrdering },
    { OMR::inductionVariableAnalysis, OMR::IfLoops },
    { OMR::loopCanonicalization },
    { OMR::loopVersioner },
    { OMR::endGroup }
};

static const OptimizationStrategy lastLoopVersionerOpts[] = {
    { OMR::inductionVariableAnalysis, OMR::IfLoops },
    { OMR::loopCanonicalization },
    { OMR::loopVersioner, OMR::MarkLastRun },
    { OMR::endGroup }
};

static const OptimizationStrategy loopCanonicalizationOpts[] = {
    { OMR::globalCopyPropagation, OMR::IfLoops }, // propagate copies to allow better invariance detection
    { OMR::loopVersionerGroup },
    { OMR::deadTreesElimination }, // remove dead anchors created by check removal (versioning)
    { OMR::treeSimplification }, // remove unreachable blocks (with nullchecks etc.) left by LoopVersioner
    { OMR::fieldPrivatization }, // use canonicalized loop to privatize fields
    { OMR::treeSimplification }, // might fold expressions created by versioning/induction variables
    { OMR::loopSpecializerGroup, OMR::IfEnabledAndLoops }, // specialize the versioned loop if possible
    { OMR::deadTreesElimination, OMR::IfEnabledAndLoops }, // remove dead anchors created by specialization
    { OMR::treeSimplification, OMR::IfEnabledAndLoops }, // might fold expressions created by specialization
    { OMR::endGroup }
};

static const OptimizationStrategy stripMiningOpts[] = {
    { OMR::inductionVariableAnalysis, OMR::IfLoops },
    { OMR::loopCanonicalization },
    { OMR::inductionVariableAnalysis },
    { OMR::stripMining },
    { OMR::endGroup }
};

static const OptimizationStrategy blockManipulationOpts[] = {
    { OMR::coldBlockOutlining },
    { OMR::CFGSimplification, OMR::IfNotJitProfiling },
    { OMR::basicBlockHoisting, OMR::IfNotJitProfiling },
    { OMR::treeSimplification },
    { OMR::redundantGotoElimination, OMR::IfNotJitProfiling }, // redundant gotos gone
    { OMR::treesCleansing }, // maximize fall throughs
    { OMR::virtualGuardHeadMerger },
    { OMR::basicBlockExtension, OMR::MarkLastRun }, // extend blocks; move trees around if reqd
    { OMR::treeSimplification }, // revisit; not really required ?
    { OMR::basicBlockPeepHole, OMR::IfEnabled },
    { OMR::endGroup }
};

static const OptimizationStrategy eachLocalAnalysisPassOpts[] = {
    { OMR::localValuePropagationGroup, OMR::IfEnabled },
#ifdef J9_PROJECT_SPECIFIC
    { OMR::arraycopyTransformation },
#endif
    { OMR::treeSimplification, OMR::IfEnabled },
    { OMR::localCSE, OMR::IfEnabled },
    { OMR::localDeadStoreElimination, OMR::IfEnabled }, // after local copy/value propagation
    { OMR::rematerialization, OMR::IfEnabled },
    { OMR::compactNullChecks, OMR::IfEnabled },
    { OMR::deadTreesElimination, OMR::IfEnabled }, // remove dead anchors created by check/store removal
    { OMR::endGroup }
};

static const OptimizationStrategy lateLocalOpts[] = {
    { OMR::eachLocalAnalysisPassGroup },
    { OMR::andSimplification }, // needs commoning across blocks to work well; must be done after versioning
    { OMR::treesCleansing }, // maximize fall throughs after LCP has converted some conditions to gotos
    { OMR::eachLocalAnalysisPassGroup },
    { OMR::localDeadStoreElimination }, // after latest copy propagation
    { OMR::deadTreesElimination }, // remove dead anchors created by check/store removal
    {
     OMR::globalDeadStoreGroup,
     },
    { OMR::eachLocalAnalysisPassGroup },
    { OMR::treeSimplification },
    { OMR::endGroup }
};

static const OptimizationStrategy tacticalGlobalRegisterAllocatorOpts[] = {
    { OMR::inductionVariableAnalysis, OMR::IfLoops },
    { OMR::loopCanonicalization, OMR::IfLoops },
    { OMR::liveRangeSplitter, OMR::IfLoops },
    { OMR::redundantGotoElimination, OMR::IfNotJitProfiling }, // need to be run before global register allocator
    { OMR::treeSimplification, OMR::MarkLastRun }, // Cleanup the trees after redundantGotoElimination
    { OMR::tacticalGlobalRegisterAllocator, OMR::IfEnabled },
    { OMR::localCSE },
    { OMR::globalCopyPropagation, OMR::IfEnabledAndMoreThanOneBlock }, // if live range splitting created copies
    { OMR::localCSE }, // localCSE after post-PRE + post-GRA globalCopyPropagation to clean up whole expression remat
    // (rtc 64659)
    { OMR::globalDeadStoreGroup, OMR::IfEnabled },
    { OMR::redundantGotoElimination,
     OMR::IfEnabledAndNotJitProfiling }, // if global register allocator created new block
    { OMR::deadTreesElimination }, // remove dangling GlRegDeps
    { OMR::deadTreesElimination, OMR::IfEnabled }, // remove dead RegStores produced by previous deadTrees pass
    { OMR::deadTreesElimination, OMR::IfEnabled }, // remove dead RegStores produced by previous deadTrees pass
    { OMR::endGroup }
};

static const OptimizationStrategy finalGlobalOpts[] = {
    { OMR::rematerialization },
    { OMR::compactNullChecks, OMR::IfEnabled },
    { OMR::deadTreesElimination },
    { OMR::localLiveRangeReduction },
    { OMR::compactLocals, OMR::IfNotJitProfiling }, // analysis results are invalidated by jitProfilingGroup
#ifdef J9_PROJECT_SPECIFIC
    { OMR::globalLiveVariablesForGC },
#endif
    { OMR::endGroup }
};

// **************************************************************************
//
// Strategy that is run for each non-peeking IlGeneration - this allows early
// optimizations to be run even before the IL is available to Inliner
//
// **************************************************************************
static const OptimizationStrategy ilgenStrategyOpts[] = {
#ifdef J9_PROJECT_SPECIFIC
    { OMR::osrLiveRangeAnalysis, OMR::IfOSR },
    { OMR::osrDefAnalysis, OMR::IfInvoluntaryOSR },
    {
                              OMR::methodHandleTransformer,
                              },
    { OMR::varHandleTransformer, OMR::MustBeDone },
    { OMR::handleRecompilationOps, OMR::MustBeDone },
    { OMR::unsafeFastPath },
    { OMR::recognizedCallTransformer },
    { OMR::coldBlockMarker },
    { OMR::CFGSimplification },
    { OMR::allocationSinking, OMR::IfNews },
    { OMR::invariantArgumentPreexistence,
                              OMR::IfNotClassLoadPhaseAndNotProfiling }, // Should not run if a recompilation is possible
#endif
    { OMR::endOpts },
};

// **********************************************************
//
// OMR Strategies
//
// **********************************************************

static const OptimizationStrategy omrNoOptStrategyOpts[] = {
    { OMR::endOpts },
};

static const OptimizationStrategy omrColdStrategyOpts[] = {
    { OMR::basicBlockExtension },
    { OMR::localCSE },
    { OMR::treeSimplification },
    { OMR::localCSE },
    { OMR::endOpts },
};

static const OptimizationStrategy omrWarmStrategyOpts[] = {
    { OMR::basicBlockExtension },
    { OMR::localCSE },
    { OMR::treeSimplification },
    { OMR::localCSE },
    { OMR::localDeadStoreElimination },
    { OMR::globalDeadStoreGroup },
    { OMR::endOpts },
};

static const OptimizationStrategy omrHotStrategyOpts[] = {
    { OMR::coldBlockOutlining },
    { OMR::earlyGlobalGroup },
    { OMR::earlyLocalGroup },
    { OMR::andSimplification }, // needs commoning across blocks to work well; must be done after versioning
    {
     OMR::stripMiningGroup,
     }, // strip mining in loops
    {
     OMR::loopReplicator,
     }, // tail-duplication in loops
    {
     OMR::blockSplitter,
     }, // treeSimplification + blockSplitter + VP => opportunity for EA
    {
     OMR::arrayPrivatizationGroup,
     }, // must preceed escape analysis
    { OMR::veryExpensiveGlobalValuePropagationGroup },
    {
     OMR::globalDeadStoreGroup,
     },
    {
     OMR::globalCopyPropagation,
     },
    {
     OMR::loopCanonicalizationGroup,
     }, // canonicalize loops (improve fall throughs)
    {
     OMR::expressionsSimplification,
     },
    { OMR::partialRedundancyEliminationGroup },
    {
     OMR::globalDeadStoreElimination,
     },
    {
     OMR::inductionVariableAnalysis,
     },
    {
     OMR::loopSpecializerGroup,
     },
    {
     OMR::inductionVariableAnalysis,
     },
    {
     OMR::generalLoopUnroller,
     }, // unroll Loops
    { OMR::blockSplitter, OMR::MarkLastRun },
    { OMR::blockManipulationGroup },
    { OMR::lateLocalGroup },
    { OMR::redundantAsyncCheckRemoval }, // optimize async check placement
#ifdef J9_PROJECT_SPECIFIC
    {
     OMR::recompilationModifier,
     }, // do before GRA to avoid commoning of longs afterwards
#endif
    {
     OMR::globalCopyPropagation,
     }, // Can produce opportunities for store sinking
    { OMR::generalStoreSinking },
    {
     OMR::localCSE,
     }, //  common up lit pool refs in the same block
    {
     OMR::treeSimplification,
     }, // cleanup the trees after sunk store and localCSE
    { OMR::trivialBlockExtension },
    {
     OMR::localDeadStoreElimination,
     }, //  remove the astore if no literal pool is required
    {
     OMR::localCSE,
     }, //  common up lit pool refs in the same block
    { OMR::arraysetStoreElimination },
    { OMR::localValuePropagation, OMR::MarkLastRun },
    { OMR::checkcastAndProfiledGuardCoalescer },
    { OMR::osrExceptionEdgeRemoval, OMR::MarkLastRun },
    {
     OMR::tacticalGlobalRegisterAllocatorGroup,
     },
    {
     OMR::globalDeadStoreElimination,
     }, // global dead store removal
    { OMR::deadTreesElimination }, // cleanup after dead store removal
    { OMR::compactNullChecks }, // cleanup at the end
    { OMR::finalGlobalGroup }, // done just before codegen
    { OMR::regDepCopyRemoval },
    { OMR::endOpts },
};

// The following arrays of Optimization pointers are externally declared in OptimizerStrategies.hpp
// This allows frontends to assist in selection of optimizer strategies.
// (They cannot be made 'static const')

const OptimizationStrategy *omrCompilationStrategies[] = {
    omrNoOptStrategyOpts, // empty strategy
    omrColdStrategyOpts, // <<  specialized
    omrWarmStrategyOpts, // <<  specialized
    omrHotStrategyOpts, // currently used to test available omr optimizations
};

void OMR::FullOptimizer::enableAllLocalOpts()
{
    this->TR::SmallOptimizer::enableAllLocalOpts();
    setRequestOptimization(andSimplification, true);
    setRequestOptimization(catchBlockRemoval, true);
    setRequestOptimization(lateLocalGroup, true);
    setRequestOptimization(localReordering, true);
    setRequestOptimization(localValuePropagationGroup, true);
}

OMR::FullOptimizer::FullOptimizer(TR::Compilation *comp, TR::ResolvedMethodSymbol *methodSymbol, bool isIlGen,
    const OptimizationStrategy *strategy, uint16_t VNType)
    : TR::SmallOptimizer(comp, methodSymbol, isIlGen, strategy, VNType)
{
    if (isIlGen) {
        // override what SmallOptimizer puts here
        setStrategy(ilgenStrategyOpts);
    }

    _opts[OMR::andSimplification]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_SimplifyAnds::create, OMR::andSimplification);
    _opts[OMR::arraysetStoreElimination] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_ArraysetStoreElimination::create, OMR::arraysetStoreElimination);
    _opts[OMR::asyncCheckInsertion] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_AsyncCheckInsertion::create, OMR::asyncCheckInsertion);
    _opts[OMR::basicBlockPeepHole] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_PeepHoleBasicBlocks::create, OMR::basicBlockPeepHole);
    _opts[OMR::blockShuffling]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_BlockShuffling::create, OMR::blockShuffling);
    _opts[OMR::blockSplitter]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_BlockSplitter::create, OMR::blockSplitter);
    _opts[OMR::catchBlockRemoval]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_CatchBlockRemover::create, OMR::catchBlockRemoval);
    _opts[OMR::CFGSimplification]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR::CFGSimplifier::create, OMR::CFGSimplification);
    _opts[OMR::checkcastAndProfiledGuardCoalescer] = new (comp->allocator()) TR::OptimizationManager(self(),
        TR_CheckcastAndProfiledGuardCoalescer::create, OMR::checkcastAndProfiledGuardCoalescer);
    _opts[OMR::coldBlockMarker]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_ColdBlockMarker::create, OMR::coldBlockMarker);
    _opts[OMR::coldBlockOutlining] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_ColdBlockOutlining::create, OMR::coldBlockOutlining);
    _opts[OMR::compactLocals]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_CompactLocals::create, OMR::compactLocals);
    _opts[OMR::expressionsSimplification] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_ExpressionsSimplification::create, OMR::expressionsSimplification);
    _opts[OMR::innerPreexistence]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_InnerPreexistence::create, OMR::innerPreexistence);
    _opts[OMR::invariantArgumentPreexistence] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_InvariantArgumentPreexistence::create, OMR::invariantArgumentPreexistence);
    _opts[OMR::loadExtensions]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_LoadExtensions::create, OMR::loadExtensions);
    _opts[OMR::localLiveRangeReduction] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_LocalLiveRangeReduction::create, OMR::localLiveRangeReduction);
    _opts[OMR::localReordering]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_LocalReordering::create, OMR::localReordering);
    _opts[OMR::loopVersioner]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_LoopVersioner::create, OMR::loopVersioner);
    _opts[OMR::loopReduction]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_LoopReducer::create, OMR::loopReduction);
    _opts[OMR::loopReplicator]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_LoopReplicator::create, OMR::loopReplicator);
    _opts[OMR::profiledNodeVersioning] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_ProfiledNodeVersioning::create, OMR::profiledNodeVersioning);
    _opts[OMR::redundantAsyncCheckRemoval] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_RedundantAsyncCheckRemoval::create, OMR::redundantAsyncCheckRemoval);
    _opts[OMR::treesCleansing]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_CleanseTrees::create, OMR::treesCleansing);
    _opts[OMR::trivialBlockExtension] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_TrivialBlockExtension::create, OMR::trivialBlockExtension);
    _opts[OMR::virtualGuardHeadMerger] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_VirtualGuardHeadMerger::create, OMR::virtualGuardHeadMerger);
    _opts[OMR::virtualGuardTailSplitter] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_VirtualGuardTailSplitter::create, OMR::virtualGuardTailSplitter);
    _opts[OMR::generalStoreSinking] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_GeneralSinkStores::create, OMR::generalStoreSinking);
    _opts[OMR::redundantInductionVarElimination] = new (comp->allocator()) TR::OptimizationManager(self(),
        TR_RedundantInductionVarElimination::create, OMR::redundantInductionVarElimination);
    _opts[OMR::partialRedundancyElimination] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_PartialRedundancy::create, OMR::partialRedundancyElimination);
    _opts[OMR::loopInversion]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_LoopInverter::create, OMR::loopInversion);
    _opts[OMR::osrExceptionEdgeRemoval] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_OSRExceptionEdgeRemoval::create, OMR::osrExceptionEdgeRemoval);
    _opts[OMR::stripMining]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_StripMiner::create, OMR::stripMining);
    _opts[OMR::fieldPrivatization]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_FieldPrivatizer::create, OMR::fieldPrivatization);
    _opts[OMR::reorderArrayIndexExpr] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_IndexExprManipulator::create, OMR::reorderArrayIndexExpr);
    _opts[OMR::loopStrider]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_LoopStrider::create, OMR::loopStrider);
    _opts[OMR::osrDefAnalysis]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_OSRDefAnalysis::create, OMR::osrDefAnalysis);
    _opts[OMR::osrLiveRangeAnalysis] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_OSRLiveRangeAnalysis::create, OMR::osrLiveRangeAnalysis);
    _opts[OMR::liveRangeSplitter]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_LiveRangeSplitter::create, OMR::liveRangeSplitter);
    _opts[OMR::loopSpecializer]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_LoopSpecializer::create, OMR::loopSpecializer);
    _opts[OMR::recognizedCallTransformer] = new (comp->allocator())
        TR::OptimizationManager(self(), TR::RecognizedCallTransformer::create, OMR::recognizedCallTransformer);
    // NOTE: Please add new OMR optimizations here!

    // initialize OMR optimization groups
    _opts[OMR::loopCanonicalizationGroup] = new (comp->allocator())
        TR::OptimizationManager(self(), NULL, OMR::loopCanonicalizationGroup, loopCanonicalizationOpts);
    _opts[OMR::loopVersionerGroup]
        = new (comp->allocator()) TR::OptimizationManager(self(), NULL, OMR::loopVersionerGroup, loopVersionerOpts);
    _opts[OMR::lastLoopVersionerGroup] = new (comp->allocator())
        TR::OptimizationManager(self(), NULL, OMR::lastLoopVersionerGroup, lastLoopVersionerOpts);
    _opts[OMR::methodHandleInvokeInliningGroup] = new (comp->allocator())
        TR::OptimizationManager(self(), NULL, OMR::methodHandleInvokeInliningGroup, methodHandleInvokeInliningOpts);
    _opts[OMR::earlyGlobalGroup]
        = new (comp->allocator()) TR::OptimizationManager(self(), NULL, OMR::earlyGlobalGroup, earlyGlobalOpts);
    _opts[OMR::isolatedStoreGroup]
        = new (comp->allocator()) TR::OptimizationManager(self(), NULL, OMR::earlyLocalGroup, isolatedStoreOpts);
    _opts[OMR::earlyLocalGroup]
        = new (comp->allocator()) TR::OptimizationManager(self(), NULL, OMR::earlyLocalGroup, earlyLocalOpts);
    _opts[OMR::stripMiningGroup]
        = new (comp->allocator()) TR::OptimizationManager(self(), NULL, OMR::stripMiningGroup, stripMiningOpts);
    _opts[OMR::arrayPrivatizationGroup] = new (comp->allocator())
        TR::OptimizationManager(self(), NULL, OMR::arrayPrivatizationGroup, arrayPrivatizationOpts);
    _opts[OMR::veryCheapGlobalValuePropagationGroup] = new (comp->allocator()) TR::OptimizationManager(self(), NULL,
        OMR::veryCheapGlobalValuePropagationGroup, veryCheapGlobalValuePropagationOpts);
    _opts[OMR::eachExpensiveGlobalValuePropagationGroup] = new (comp->allocator()) TR::OptimizationManager(self(), NULL,
        OMR::eachExpensiveGlobalValuePropagationGroup, eachExpensiveGlobalValuePropagationOpts);
    _opts[OMR::veryExpensiveGlobalValuePropagationGroup] = new (comp->allocator()) TR::OptimizationManager(self(), NULL,
        OMR::veryExpensiveGlobalValuePropagationGroup, veryExpensiveGlobalValuePropagationOpts);
    _opts[OMR::loopSpecializerGroup]
        = new (comp->allocator()) TR::OptimizationManager(self(), NULL, OMR::loopSpecializerGroup, loopSpecializerOpts);
    _opts[OMR::lateLocalGroup]
        = new (comp->allocator()) TR::OptimizationManager(self(), NULL, OMR::lateLocalGroup, lateLocalOpts);
    _opts[OMR::eachLocalAnalysisPassGroup] = new (comp->allocator())
        TR::OptimizationManager(self(), NULL, OMR::eachLocalAnalysisPassGroup, eachLocalAnalysisPassOpts);
    _opts[OMR::tacticalGlobalRegisterAllocatorGroup] = new (comp->allocator()) TR::OptimizationManager(self(), NULL,
        OMR::tacticalGlobalRegisterAllocatorGroup, tacticalGlobalRegisterAllocatorOpts);
    _opts[OMR::partialRedundancyEliminationGroup] = new (comp->allocator())
        TR::OptimizationManager(self(), NULL, OMR::partialRedundancyEliminationGroup, partialRedundancyEliminationOpts);
    _opts[OMR::reorderArrayExprGroup] = new (comp->allocator())
        TR::OptimizationManager(self(), NULL, OMR::reorderArrayExprGroup, reorderArrayIndexOpts);
    _opts[OMR::blockManipulationGroup] = new (comp->allocator())
        TR::OptimizationManager(self(), NULL, OMR::blockManipulationGroup, blockManipulationOpts);
    _opts[OMR::localValuePropagationGroup] = new (comp->allocator())
        TR::OptimizationManager(self(), NULL, OMR::localValuePropagationGroup, localValuePropagationOpts);
    _opts[OMR::finalGlobalGroup]
        = new (comp->allocator()) TR::OptimizationManager(self(), NULL, OMR::finalGlobalGroup, finalGlobalOpts);
    // NOTE: Please add new OMR optimization groups here!
}
