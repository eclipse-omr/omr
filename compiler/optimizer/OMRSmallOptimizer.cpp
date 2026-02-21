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

#include "optimizer/SmallOptimizer.hpp"
#include "optimizer/Optimizer.hpp"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "codegen/CodeGenerator.hpp"
#include "env/FrontEnd.hpp"
#include "compile/Compilation.hpp"
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
#include "optimizer/Optimization.hpp"
#include "optimizer/OptimizationManager.hpp"
#include "optimizer/OptimizationStrategies.hpp"
#include "optimizer/Optimizations.hpp"
#include "optimizer/Structure.hpp"
#include "optimizer/StructuralAnalysis.hpp"
#include "optimizer/UseDefInfo.hpp"
#include "optimizer/ValueNumberInfo.hpp"
#include "optimizer/DeadStoreElimination.hpp"
#include "optimizer/DeadTreesElimination.hpp"
#include "optimizer/CopyPropagation.hpp"
#include "optimizer/GeneralLoopUnroller.hpp"
#include "optimizer/LocalCSE.hpp"
#include "optimizer/LocalDeadStoreElimination.hpp"
#include "optimizer/LocalOpts.hpp"
#include "optimizer/LoopCanonicalizer.hpp"
#include "optimizer/OrderBlocks.hpp"
#include "optimizer/Simplifier.hpp"
#include "optimizer/Inliner.hpp"
#include "ras/Debug.hpp"
#include "optimizer/InductionVariable.hpp"
#include "optimizer/GlobalValuePropagation.hpp"
#include "optimizer/LocalValuePropagation.hpp"
#include "optimizer/RegDepCopyRemoval.hpp"
#include "optimizer/GlobalRegisterAllocator.hpp"
#include "optimizer/SwitchAnalyzer.hpp"
#include "env/RegionProfiler.hpp"

namespace TR {
class AutomaticSymbol;
}

using namespace OMR; // Note: used here only to avoid having to prepend all opts in strategies with OMR::

#define MAX_LOCAL_OPTS_ITERS 5

#ifdef OPT_TIMING // provide statistics on time taken by individual optimizations
TR_Stats statOptTiming[OMR::numOpts];
TR_Stats statStructuralAnalysisTiming("Structural Analysis");
TR_Stats statUseDefsTiming("Use Defs");
TR_Stats statGlobalValNumTiming("Global Value Numbering");
#endif // OPT_TIMING

static const OptimizationStrategy cheapTacticalGlobalRegisterAllocatorOpts[] = {
    { OMR::redundantGotoElimination, OMR::IfNotProfiling }, // need to be run before global register allocator
    { OMR::tacticalGlobalRegisterAllocator, OMR::IfEnabled },
    { OMR::endGroup }
};

static const OptimizationStrategy globalDeadStoreOpts[] = {
    { OMR::globalDeadStoreElimination, OMR::IfMoreThanOneBlock },
    { OMR::localDeadStoreElimination, OMR::IfOneBlock },
    { OMR::deadTreesElimination },
    { OMR::endGroup }
};

static const OptimizationStrategy JBnoOptStrategyOpts[] = { { OMR::endOpts } };

static const OptimizationStrategy JBcoldStrategyOpts[]
    = { { OMR::deadTreesElimination }, { OMR::treeSimplification }, { OMR::localCSE }, { OMR::basicBlockExtension },
          { OMR::cheapTacticalGlobalRegisterAllocatorGroup }, { OMR::endOpts } };

static const OptimizationStrategy JBwarmStrategyOpts[] = {
    { OMR::deadTreesElimination },
    { OMR::inlining },
    { OMR::treeSimplification },
    { OMR::localCSE },
    { OMR::basicBlockOrdering }, // straighten goto's
    { OMR::globalCopyPropagation },
    { OMR::globalDeadStoreElimination, OMR::IfMoreThanOneBlock },
    { OMR::deadTreesElimination },
    { OMR::treeSimplification },
    { OMR::basicBlockHoisting },
    { OMR::treeSimplification },
    { OMR::globalValuePropagation, OMR::IfMoreThanOneBlock },
    { OMR::localValuePropagation, OMR::IfOneBlock },
    {
     OMR::switchAnalyzer,
     },
    { OMR::localCSE },
    { OMR::treeSimplification },
    { OMR::trivialDeadTreeRemoval, OMR::IfEnabled },
    { OMR::basicBlockOrdering, OMR::IfLoops }, // clean up block order for loop canonicalization, if it will run
    { OMR::loopCanonicalization,
     OMR::IfLoops }, // canonicalization must run before inductionVariableAnalysis else indvar data gets messed up
    { OMR::inductionVariableAnalysis, OMR::IfLoops }, // needed for loop unroller
    { OMR::generalLoopUnroller, OMR::IfLoops },
    { OMR::basicBlockExtension, OMR::MarkLastRun }, // clean up order and extend blocks now
    { OMR::treeSimplification },
    { OMR::localCSE },
    { OMR::treeSimplification, OMR::IfEnabled },
    { OMR::trivialDeadTreeRemoval, OMR::IfEnabled },
    { OMR::cheapTacticalGlobalRegisterAllocatorGroup },
    {
     OMR::globalDeadStoreGroup,
     },
    { OMR::redundantGotoElimination, OMR::IfEnabled }, // if global register allocator created new block
    { OMR::rematerialization },
    { OMR::deadTreesElimination, OMR::IfEnabled }, // remove dead anchors created by check/store removal
    { OMR::deadTreesElimination, OMR::IfEnabled }, // remove dead RegStores produced by previous deadTrees pass
    { OMR::regDepCopyRemoval },
    { OMR::endOpts },
};

static const OptimizationStrategy *omrCompilationStrategies[] = {
    JBnoOptStrategyOpts,
    JBcoldStrategyOpts,
    JBwarmStrategyOpts,
    JBwarmStrategyOpts,
};

TR::Optimizer *OMR::SmallOptimizer::createOptimizer(TR::Compilation *comp, TR::ResolvedMethodSymbol *methodSymbol,
    bool isIlGen)
{
    // returns IL optimizer, performs tree-to-tree optimizing transformations.
    if (isIlGen) {
        return new (comp->trHeapMemory()) TR::Optimizer(comp, methodSymbol, isIlGen, JBnoOptStrategyOpts);
    }

    if (comp->getOptions()->getCustomStrategy()) {
        if (comp->getOption(TR_TraceOptDetails))
            comp->log()->printf("Using custom optimization strategy\n");

        // Reformat custom strategy as array of Optimization rather than array of int32_t
        //
        int32_t *srcStrategy = comp->getOptions()->getCustomStrategy();
        int32_t size = comp->getOptions()->getCustomStrategySize();
        OptimizationStrategy *customStrategy
            = (OptimizationStrategy *)comp->trMemory()->allocateHeapMemory(size * sizeof(customStrategy[0]));
        for (int32_t i = 0; i < size; i++) {
            OptimizationStrategy o = { (OMR::Optimizations)(srcStrategy[i] & TR::Options::OptNumMask) };
            if (srcStrategy[i] & TR::Options::MustBeDone)
                o._options = MustBeDone;
            customStrategy[i] = o;
        }

        return new (comp->trHeapMemory()) TR::Optimizer(comp, methodSymbol, isIlGen, customStrategy);
    }

    TR::Optimizer *optimizer = new (comp->trHeapMemory()) TR::Optimizer(comp, methodSymbol, isIlGen,
        TR::Optimizer::optimizationStrategy(comp), TR::Optimizer::valueNumberInfoBuildType());

    return optimizer;
}

// ************************************************************************
//
// Implementation of TR::Optimizer
//
// ************************************************************************

OMR::SmallOptimizer::SmallOptimizer(TR::Compilation *comp, TR::ResolvedMethodSymbol *methodSymbol, bool isIlGen,
    const OptimizationStrategy *strategy, uint16_t VNType)
    : _compilation(comp)
    , _cg(comp->cg())
    , _trMemory(comp->trMemory())
    , _methodSymbol(methodSymbol)
    , _isIlGen(isIlGen)
    , _strategy(strategy)
    , _vnInfoType(VNType)
    , _symReferencesTable(NULL)
    , _useDefInfo(NULL)
    , _valueNumberInfo(NULL)
    , _aliasSetsAreValid(false)
    , _cantBuildGlobalsUseDefInfo(false)
    , _cantBuildLocalsUseDefInfo(false)
    , _cantBuildGlobalsValueNumberInfo(false)
    , _cantBuildLocalsValueNumberInfo(false)
    , _canRunBlockByBlockOptimizations(true)
    , _cachedExtendedBBInfoValid(false)
    , _inlineSynchronized(true)
    , _enclosingFinallyBlock(NULL)
    , _eliminatedCheckcastNodes(comp->trMemory())
    , _classPointerNodes(comp->trMemory())
    , _optMessageIndex(0)
    , _seenBlocksGRA(NULL)
    , _resetExitsGRA(NULL)
    , _successorBitsGRA(NULL)
    , _stackedOptimizer(false)
    , _firstTimeStructureIsBuilt(true)
    , _disableLoopOptsThatCanCreateLoops(false)
{
    // zero opts table
    memset(_opts, 0, sizeof(_opts));

    _opts[OMR::basicBlockExtension] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_ExtendBasicBlocks::create, OMR::basicBlockExtension);
    _opts[OMR::basicBlockHoisting]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_HoistBlocks::create, OMR::basicBlockHoisting);
    _opts[OMR::basicBlockOrdering]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_OrderBlocks::create, OMR::basicBlockOrdering);
    _opts[OMR::compactNullChecks]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_CompactNullChecks::create, OMR::compactNullChecks);
    _opts[OMR::deadTreesElimination] = new (comp->allocator())
        TR::OptimizationManager(self(), TR::DeadTreesElimination::create, OMR::deadTreesElimination);
    _opts[OMR::generalLoopUnroller] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_GeneralLoopUnroller::create, OMR::generalLoopUnroller);
    _opts[OMR::globalCopyPropagation] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_CopyPropagation::create, OMR::globalCopyPropagation);
    _opts[OMR::globalDeadStoreElimination] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_DeadStoreElimination::create, OMR::globalDeadStoreElimination);
    _opts[OMR::inlining]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_TrivialInliner::create, OMR::inlining);
    _opts[OMR::localCSE] = new (comp->allocator()) TR::OptimizationManager(self(), TR::LocalCSE::create, OMR::localCSE);
    _opts[OMR::localDeadStoreElimination] = new (comp->allocator())
        TR::OptimizationManager(self(), TR::LocalDeadStoreElimination::create, OMR::localDeadStoreElimination);
    _opts[OMR::loopCanonicalization] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_LoopCanonicalizer::create, OMR::loopCanonicalization);
    _opts[OMR::redundantGotoElimination] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_EliminateRedundantGotos::create, OMR::redundantGotoElimination);
    _opts[OMR::rematerialization]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR_Rematerialization::create, OMR::rematerialization);
    _opts[OMR::treeSimplification]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR::Simplifier::create, OMR::treeSimplification);
    _opts[OMR::trivialDeadTreeRemoval] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_TrivialDeadTreeRemoval::create, OMR::trivialDeadTreeRemoval);
    _opts[OMR::globalValuePropagation] = new (comp->allocator())
        TR::OptimizationManager(self(), TR::GlobalValuePropagation::create, OMR::globalValuePropagation);
    _opts[OMR::localValuePropagation] = new (comp->allocator())
        TR::OptimizationManager(self(), TR::LocalValuePropagation::create, OMR::localValuePropagation);
    _opts[OMR::inductionVariableAnalysis] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_InductionVariableAnalysis::create, OMR::inductionVariableAnalysis);
    _opts[OMR::regDepCopyRemoval] = new (comp->allocator())
        TR::OptimizationManager(self(), TR::RegDepCopyRemoval::create, OMR::regDepCopyRemoval);
    _opts[OMR::tacticalGlobalRegisterAllocator] = new (comp->allocator())
        TR::OptimizationManager(self(), TR_GlobalRegisterAllocator::create, OMR::tacticalGlobalRegisterAllocator);
    _opts[OMR::switchAnalyzer]
        = new (comp->allocator()) TR::OptimizationManager(self(), TR::SwitchAnalyzer::create, OMR::switchAnalyzer);
    // NOTE: Please add new OMR optimizations here!

    // initialize OMR optimization groups

    _opts[OMR::globalDeadStoreGroup]
        = new (comp->allocator()) TR::OptimizationManager(self(), NULL, OMR::globalDeadStoreGroup, globalDeadStoreOpts);
    // NOTE: Please add new OMR optimization groups here!

    // From JBOptimizer, should be folded into above
    // Initialize optimization groups
    _opts[OMR::cheapTacticalGlobalRegisterAllocatorGroup] = new (comp->allocator()) TR::OptimizationManager(self(),
        NULL, OMR::cheapTacticalGlobalRegisterAllocatorGroup, cheapTacticalGlobalRegisterAllocatorOpts);
    _opts[OMR::globalDeadStoreGroup]
        = new (comp->allocator()) TR::OptimizationManager(self(), NULL, OMR::globalDeadStoreGroup, globalDeadStoreOpts);

// turn requested on for optimizations/groups
#if 0
   // these don't seem to work for the compiler unit tests
   self()->setRequestOptimization(OMR::cheapTacticalGlobalRegisterAllocatorGroup, true);
   self()->setRequestOptimization(OMR::tacticalGlobalRegisterAllocatorGroup, true);
   self()->setRequestOptimization(OMR::tacticalGlobalRegisterAllocator, true);
#endif
}

// Note: optimizer_name array needs to match Optimizations enum defined
// in compiler/optimizer/Optimizations.hpp
static const char *optimizer_name[] = {
#define OPTIMIZATION(name) #name,
#define OPTIMIZATION_ENUM_ONLY(entry) "****",
#include "optimizer/Optimizations.enum"
    OPTIMIZATION_ENUM_ONLY(numOpts)
#include "optimizer/OptimizationGroups.enum"
        OPTIMIZATION_ENUM_ONLY(numGroups)
#undef OPTIMIZATION
#undef OPTIMIZATION_ENUM_ONLY
};

const char *OMR::SmallOptimizer::getOptimizationName(OMR::Optimizations opt) { return ::optimizer_name[opt]; }

bool OMR::SmallOptimizer::isEnabled(OMR::Optimizations i)
{
    if (_opts[i] != NULL)
        return _opts[i]->enabled();
    return false;
}

TR_Debug *OMR::SmallOptimizer::getDebug() { return _compilation->getDebug(); }

void OMR::SmallOptimizer::setCachedExtendedBBInfoValid(bool b)
{
    TR_ASSERT(!comp()->isPeekingMethod(), "ERROR: Should not modify _cachedExtendedBBInfoValid while peeking");
    _cachedExtendedBBInfoValid = b;
}

TR_UseDefInfo *OMR::SmallOptimizer::setUseDefInfo(TR_UseDefInfo *u)
{
    if (_useDefInfo != NULL) {
        dumpOptDetails(comp(), "     (Invalidating use/def info)\n");
        delete _useDefInfo;
    }
    return (_useDefInfo = u);
}

TR_ValueNumberInfo *OMR::SmallOptimizer::setValueNumberInfo(TR_ValueNumberInfo *v)
{
    if (_valueNumberInfo && !v)
        dumpOptDetails(comp(), "     (Invalidating value number info)\n");

    if (_valueNumberInfo)
        delete _valueNumberInfo;
    return (_valueNumberInfo = v);
}

TR_UseDefInfo *OMR::SmallOptimizer::createUseDefInfo(TR::Compilation *comp, bool requiresGlobals, bool prefersGlobals,
    bool loadsShouldBeDefs, bool cannotOmitTrivialDefs, bool conversionRegsOnly, bool doCompletion)
{
    return new (comp->allocator()) TR_UseDefInfo(comp, comp->getFlowGraph(), self(), requiresGlobals, prefersGlobals,
        loadsShouldBeDefs, cannotOmitTrivialDefs, conversionRegsOnly, doCompletion, getCallsAsUses());
}

TR_ValueNumberInfo *OMR::SmallOptimizer::createValueNumberInfo(bool requiresGlobals, bool preferGlobals,
    bool noUseDefInfo)
{
    LexicalTimer t("global value numbering (for globals definitely)", comp()->phaseTimer());
    TR::LexicalMemProfiler mp("global value numbering (for globals definitely)", comp()->phaseMemProfiler());

    TR_ValueNumberInfo *valueNumberInfo = NULL;
    switch (_vnInfoType) {
        case PrePartitionVN:
            valueNumberInfo = new (comp()->allocator())
                TR_ValueNumberInfo(comp(), self(), requiresGlobals, preferGlobals, noUseDefInfo);
            break;
        case HashVN:
            valueNumberInfo = new (comp()->allocator())
                TR_HashValueNumberInfo(comp(), self(), requiresGlobals, preferGlobals, noUseDefInfo);
            break;
        default:
            valueNumberInfo = new (comp()->allocator())
                TR_ValueNumberInfo(comp(), self(), requiresGlobals, preferGlobals, noUseDefInfo);
            break;
    };

    TR_ASSERT(valueNumberInfo != NULL, "Failed to create ValueNumber Information");
    return valueNumberInfo;
}

void OMR::SmallOptimizer::optimize()
{
    TR::Compilation::CompilationPhaseScope mainCompilationPhaseScope(comp());
    OMR::Logger *log = comp()->log();

    if (isIlGenOpt()) {
        const OptimizationStrategy *opt = _strategy;
        while (opt->_num != endOpts) {
            TR::OptimizationManager *manager = getOptimization(opt->_num);
            TR_ASSERT(manager->getSupportsIlGenOptLevel(), "Optimization %s should support IlGen opt level",
                manager->name());
            opt++;
        }

        if (comp()->getOption(TR_TraceTrees)
            && (comp()->isOutermostMethod() || comp()->trace(inlining) || comp()->getOption(TR_DebugInliner)))
            comp()->dumpMethodTrees(log, "Pre IlGenOpt Trees", getMethodSymbol());
    }

    LexicalTimer t("optimize", comp()->signature(), comp()->phaseTimer());
    TR::LexicalMemProfiler mp("optimize", comp()->signature(), comp()->phaseMemProfiler());
    TR::StackMemoryRegion stackMemoryRegion(*trMemory());

    // Sometimes the Compilation object needs to host more than one Optimizer
    // (over time).  This is because Symbol::genIL can be called, for example,
    // (indirectly) by addVeryRefinedCallAliasSets.  Under some circumstances,
    // genIL will instantiate a new Optimizer which must use the caller's
    // Compilation.  So, we need to push and pop the appropriate Optimizer.
    TR::Optimizer *stackedOptimizer = comp()->getOptimizer();
    _stackedOptimizer = (self() != stackedOptimizer);
    comp()->setOptimizer(self());

    if (comp()->getOption(TR_TraceOptDetails)) {
        if (comp()->isOutermostMethod()) {
            const char *hotnessString = comp()->getHotnessName(comp()->getMethodHotness());
            TR_ASSERT(hotnessString, "expected to have a hotness string");
            log->printf("<optimize\n"
                        "\tmethod=\"%s\"\n"
                        "\thotness=\"%s\">\n",
                comp()->signature(), hotnessString);
        }
    }

    if (comp()->getOption(TR_TraceOpts)) {
        if (comp()->isOutermostMethod()) {
            const char *hotnessString = comp()->getHotnessName(comp()->getMethodHotness());
            TR_ASSERT(hotnessString, "expected to have a hotness string");
            log->printf("<strategy hotness=\"%s\">\n", hotnessString);
        }
    }

    int32_t firstOptIndex = comp()->getOptions()->getFirstOptIndex();
    int32_t lastOptIndex = comp()->getOptions()->getLastOptIndex();

    _firstDumpOptPhaseTrees = INT_MAX;
    _lastDumpOptPhaseTrees = INT_MAX;

    if (comp()->getOption(TR_TraceOptDetails))
        _firstDumpOptPhaseTrees = 0;

#ifdef DEBUG
    char *p;
    p = debug("dumpOptPhaseTrees");
    if (p) {
        _firstDumpOptPhaseTrees = 0;
        if (*p) {
            while (*p >= '0' && *p <= '9')
                _firstDumpOptPhaseTrees = _firstDumpOptPhaseTrees * 10 + *(p++) - '0';
            if (*(p++) == '-') {
                _lastDumpOptPhaseTrees = 0;
                while (*p >= '0' && *p <= '9')
                    _lastDumpOptPhaseTrees = _lastDumpOptPhaseTrees * 10 + *(p++) - '0';
            } else
                _lastDumpOptPhaseTrees = _firstDumpOptPhaseTrees;
        }
    }

    static char *c3 = feGetEnv("TR_dumpGraphs");
    if (c3) {
        if (!debug("dumpGraphs"))
            addDebug("dumpGraphs");
        // Check if it is a number
        //
        if (*c3 >= '0' && *c3 <= '9')
            _dumpGraphsIndex = atoi(c3);
        else
            _dumpGraphsIndex = -1;
    }
#endif

    TR_SingleTimer myTimer;
    TR_FrontEnd *fe = comp()->fe();
    bool doTiming = comp()->getOption(TR_Timing);
    if (doTiming && comp()->getOutFile() != NULL) {
        myTimer.initialize("all optimizations", trMemory());
    }

    if (comp()->getOption(TR_Profile) && !comp()->isProfilingCompilation()) {
        // These numbers are chosen to try to maximize the odds of finding bugs.
        // freq=2 means we'll switch to and from the profiling body often,
        // thus testing those transitions.
        // The low count value means we will try to recompile the method
        // fairly early, thus testing recomp.
        //
        self()->switchToProfiling(2, 30);
    }

    const OptimizationStrategy *opt = _strategy;
    while (opt->_num != endOpts) {
        int32_t actualCost = performOptimization(opt, firstOptIndex, lastOptIndex, doTiming);
        opt++;
        if (!isIlGenOpt() && comp()->getNodePool().removeDeadNodes()) {
            setValueNumberInfo(NULL);
        }
    }

    if (comp()->getOption(TR_EnableDeterministicOrientedCompilation) && comp()->isOutermostMethod()
        && (comp()->getMethodHotness() > cold) && (comp()->getMethodHotness() < scorching)) {
        TR_Hotness nextHotness = checkMaxHotnessOfInlinedMethods(comp());
        if (nextHotness > comp()->getMethodHotness()) {
            comp()->setNextOptLevel(nextHotness);
            comp()->failCompilation<TR::InsufficientlyAggressiveCompilation>(
                "Method needs to be compiled at higher level");
        }
    }

    dumpPostOptTrees();

    if (comp()->getOption(TR_TraceOpts)) {
        if (comp()->isOutermostMethod())
            log->printf("</strategy>\n");
    }

    if (comp()->getOption(TR_TraceOptDetails)) {
        if (comp()->isOutermostMethod())
            log->printf("</optimize>\n");
    }

    comp()->setOptimizer(stackedOptimizer);
    _stackedOptimizer = false;
}

void OMR::SmallOptimizer::dumpPostOptTrees()
{
    // do nothing for IlGen optimizer
    if (isIlGenOpt())
        return;

    TR::Method *method = comp()->getMethodSymbol()->getMethod();
    if ((debug("dumpPostLocalOptTrees") || comp()->getOption(TR_TraceTrees)))
        comp()->dumpMethodTrees(comp()->log(), "Post Optimization Trees");
}

void dumpName(TR::Optimizer *op, TR_FrontEnd *fe, TR::Compilation *comp, OMR::Optimizations optNum)
{
    static int level = 1;
    TR::OptimizationManager *manager = op->getOptimization(optNum);

    if (level > 6)
        return;

    if (optNum > endGroup && optNum < OMR::numGroups) {
        trfprintf(comp->getOutFile(), "%*s<%s>\n", level * 6, " ", manager->name());

        level++;

        const OptimizationStrategy *subGroup = ((TR::OptimizationManager *)manager)->groupOfOpts();

        while (subGroup->_num != endOpts && subGroup->_num != endGroup) {
            dumpName(op, fe, comp, subGroup->_num);
            subGroup++;
        }

        level--;

        trfprintf(comp->getOutFile(), "%*s</%s>", level * 6, " ", manager->name());
    } else if (optNum > endOpts && optNum < OMR::numOpts)
        trfprintf(comp->getOutFile(), "%*s%s", level * 6, " ", manager->name());
    else
        trfprintf(comp->getOutFile(), "%*s<%d>", level * 6, " ", optNum);

    trfprintf(comp->getOutFile(), "\n");
}

void OMR::SmallOptimizer::dumpStrategy(const OptimizationStrategy *opt)
{
    TR_FrontEnd *fe = comp()->fe();

    trfprintf(comp()->getOutFile(), "endOpts:%d OMR::numOpts:%d endGroup:%d numGroups:%d\n", endOpts, OMR::numOpts,
        endGroup, OMR::numGroups);

    while (opt->_num != endOpts) {
        dumpName(self(), fe, comp(), opt->_num);
        opt++;
    }

    trfprintf(comp()->getOutFile(), "\n");
}

static bool hasMoreThanOneBlock(TR::Compilation *comp)
{
    return (comp->getStartBlock() && comp->getStartBlock()->getNextBlock());
}

static void breakForTesting(int index)
{
    static char *optimizerBreakLocationStr = feGetEnv("TR_optimizerBreakLocation");
    if (optimizerBreakLocationStr) {
        static int optimizerBreakLocation = atoi(optimizerBreakLocationStr);
        static char *optimizerBreakSkipCountStr = feGetEnv("TR_optimizerBreakSkipCount");
        static int optimizerBreakSkipCount = optimizerBreakSkipCountStr ? atoi(optimizerBreakSkipCountStr) : 0;
        if (index == optimizerBreakLocation) {
            if (optimizerBreakSkipCount == 0)
                TR::Compiler->debug.breakPoint();
            else
                --optimizerBreakSkipCount;
        }
    }
}

int32_t OMR::SmallOptimizer::performOptimization(const OptimizationStrategy *optimization, int32_t firstOptIndex,
    int32_t lastOptIndex, int32_t doTiming)
{
    OMR::Logger *log = comp()->log();
    OMR::Optimizations optNum = optimization->_num;
    TR::OptimizationManager *manager = getOptimization(optNum);
    TR_ASSERT(manager != NULL, "Optimization manager should have been initialized for %s.",
        getOptimizationName(optNum));

    comp()->reportAnalysisPhase(BEFORE_OPTIMIZATION);
    breakForTesting(1010);

    int32_t optIndex = comp()->getOptIndex()
        + 1; // +1 because we haven't incremented yet at this point, becuase we're not sure we should
    // Determine whether or not to do this optimization
    //
    bool doThisOptimization = false;
    bool doThisOptimizationIfEnabled = false;
    bool mustBeDone = false;
    bool justSetLastRun = false;
    switch (optimization->_options) {
        case Always:
            doThisOptimization = true;
            break;

        case IfLoops:
            if (comp()->mayHaveLoops())
                doThisOptimization = true;
            break;

        case IfMoreThanOneBlock:
            if (hasMoreThanOneBlock(comp()))
                doThisOptimization = true;
            break;

        case IfOneBlock:
            if (!hasMoreThanOneBlock(comp()))
                doThisOptimization = true;
            break;

        case IfLoopsMarkLastRun:
            if (comp()->mayHaveLoops())
                doThisOptimization = true;
            TR_ASSERT(optNum < OMR::numOpts, "No current support for marking groups as last (optNum=%d,numOpt=%d\n",
                optNum, OMR::numOpts); // make sure we didn't mark groups
            manager->setLastRun(true);
            justSetLastRun = true;
            break;

        case IfNoLoops:
            if (!comp()->mayHaveLoops())
                doThisOptimization = true;
            break;

        case IfProfiling:
            if (comp()->isProfilingCompilation())
                doThisOptimization = true;
            break;

        case IfNotProfiling:
            if (!comp()->isProfilingCompilation() || debug("ignoreIfNotProfiling"))
                doThisOptimization = true;
            break;

        case IfJitProfiling:
            if (comp()->getProfilingMode() == JitProfiling)
                doThisOptimization = true;
            break;

        case IfNotJitProfiling:
            if (comp()->getProfilingMode() != JitProfiling)
                doThisOptimization = true;
            break;

        case IfNews:
            if (comp()->hasNews())
                doThisOptimization = true;
            break;

        case IfOptServer:
            if (comp()->isOptServer())
                doThisOptimization = true;
            break;

        case IfMonitors:
            if (comp()->getMethodSymbol()->mayContainMonitors())
                doThisOptimization = true;
            break;

        case IfEnabledAndMonitors:
            if (manager->requested() && comp()->getMethodSymbol()->mayContainMonitors())
                doThisOptimization = true;
            break;

        case IfEnabledAndOptServer:
            if (manager->requested() && comp()->isOptServer()) {
                doThisOptimizationIfEnabled = true;
                doThisOptimization = true;
            }
            break;

#ifdef J9_PROJECT_SPECIFIC
        case IfNotClassLoadPhase:
            if (!comp()->getPersistentInfo()->isClassLoadingPhase() || comp()->getOption(TR_DontDowngradeToCold))
                doThisOptimization = true;
            break;

        case IfNotClassLoadPhaseAndNotProfiling:
            if ((!comp()->getPersistentInfo()->isClassLoadingPhase() || comp()->getOption(TR_DontDowngradeToCold))
                && (!comp()->isProfilingCompilation() || debug("ignoreIfNotProfiling")))
                doThisOptimization = true;
            break;
#endif

        case IfEnabledAndLoops:
            if (comp()->mayHaveLoops() && manager->requested()) {
                doThisOptimizationIfEnabled = true;
                doThisOptimization = true;
            }
            break;

        case IfEnabledAndMoreThanOneBlock:
            if (hasMoreThanOneBlock(comp()) && manager->requested()) {
                doThisOptimizationIfEnabled = true;
                doThisOptimization = true;
            }
            break;

        case IfEnabledAndMoreThanOneBlockMarkLastRun:
            if (hasMoreThanOneBlock(comp()) && manager->requested()) {
                doThisOptimizationIfEnabled = true;
                doThisOptimization = true;
            }
            TR_ASSERT(optNum < OMR::numOpts, "No current support for marking groups as last (optNum=%d,numOpt=%d\n",
                optNum, OMR::numOpts); // make sure we didn't mark groups
            manager->setLastRun(true);
            justSetLastRun = true;
            break;

        case IfEnabledAndNoLoops:
            if (!comp()->mayHaveLoops() && manager->requested()) {
                doThisOptimizationIfEnabled = true;
                doThisOptimization = true;
            }
            break;

        case IfNoLoopsOREnabledAndLoops:
            if (!comp()->mayHaveLoops() || manager->requested()) {
                if (comp()->mayHaveLoops())
                    doThisOptimizationIfEnabled = true;
                doThisOptimization = true;
            }
            break;

        case IfEnabledAndProfiling:
            if (comp()->isProfilingCompilation() && manager->requested()) {
                doThisOptimizationIfEnabled = true;
                doThisOptimization = true;
            }
            break;

        case IfEnabledAndNotProfiling:
            if (!comp()->isProfilingCompilation() && manager->requested()) {
                doThisOptimizationIfEnabled = true;
                doThisOptimization = true;
            }
            break;

        case IfEnabledAndNotJitProfiling:
            if (comp()->getProfilingMode() != JitProfiling && manager->requested()) {
                doThisOptimizationIfEnabled = true;
                doThisOptimization = true;
            }
            break;

        case IfLoopsAndNotProfiling:
            if (comp()->mayHaveLoops() && !comp()->isProfilingCompilation())
                doThisOptimization = true;
            break;

        case MustBeDone:
            mustBeDone = doThisOptimization = true;
            break;

        case IfFullInliningUnderOSRDebug:
            if (comp()->getOption(TR_FullSpeedDebug) && comp()->getOption(TR_EnableOSR)
                && comp()->getOption(TR_FullInlineUnderOSRDebug))
                doThisOptimization = true;
            break;

        case IfNotFullInliningUnderOSRDebug:
            if (comp()->getOption(TR_FullSpeedDebug)
                && (!comp()->getOption(TR_EnableOSR) || !comp()->getOption(TR_FullInlineUnderOSRDebug)))
                doThisOptimization = true;
            break;

        case IfOSR:
            if (comp()->getOption(TR_EnableOSR))
                doThisOptimization = true;
            break;

        case IfVoluntaryOSR:
            if (comp()->getOption(TR_EnableOSR) && comp()->getOSRMode() == TR::voluntaryOSR)
                doThisOptimization = true;
            break;

        case IfInvoluntaryOSR:
            if (comp()->getOption(TR_EnableOSR) && comp()->getOSRMode() == TR::involuntaryOSR)
                doThisOptimization = true;
            break;

        case IfEnabled:
            if (manager->requested()) {
                doThisOptimizationIfEnabled = true;
                doThisOptimization = true;
            }
            break;

        case IfEnabledMarkLastRun:
            if (manager->requested()) {
                doThisOptimizationIfEnabled = true;
                doThisOptimization = true;
            }
            TR_ASSERT(optNum < OMR::numOpts, "No current support for marking groups as last (optNum=%d,numOpt=%d\n",
                optNum, OMR::numOpts); // make sure we didn't mark groups
            manager->setLastRun(true);
            justSetLastRun = true;
            break;

        case IfAOTAndEnabled: {
            bool enableColdCheapTacticalGRA = comp()->getOption(TR_EnableColdCheapTacticalGRA);
            bool disableAOTColdCheapTacticalGRA = comp()->getOption(TR_DisableAOTColdCheapTacticalGRA);

            if ((comp()->compileRelocatableCode() || enableColdCheapTacticalGRA) && manager->requested()
                && !disableAOTColdCheapTacticalGRA) {
                doThisOptimizationIfEnabled = true;
                doThisOptimization = true;
            }
        } break;

        case IfMethodHandleInvokes: {
            if (comp()->getMethodSymbol()->hasMethodHandleInvokes()
                && !comp()->getOption(TR_DisableMethodHandleInvokeOpts))
                doThisOptimization = true;
        } break;

        case IfNotQuickStart: {
            if (!comp()->getOptions()->isQuickstartDetected()) {
                doThisOptimization = true;
            }
            break;
        }

        case IfEAOpportunitiesMarkLastRun:
            getOptimization(optNum)->setLastRun(true);
            justSetLastRun = true;
            // fall through
        case IfEAOpportunities:
        case IfEAOpportunitiesAndNotOptServer: {
            if (comp()->getMethodSymbol()->hasEscapeAnalysisOpportunities()) {
                if ((optimization->_options == IfEAOpportunitiesAndNotOptServer) && comp()->isOptServer()) {
                    // don't enable
                } else {
                    doThisOptimization = true;
                }
            }
            break;
        }
        case IfAggressiveLiveness: {
            if (comp()->getOption(TR_EnableAggressiveLiveness)) {
                doThisOptimization = true;
            }
            break;
        }
        case IfVectorAPI: {
            if (comp()->getMethodSymbol()->hasVectorAPI() && !comp()->getOption(TR_DisableVectorAPIExpansion))
                doThisOptimization = true;
        } break;
        case IfExceptionHandlers: {
            if (comp()->hasExceptionHandlers())
                doThisOptimization = true;
        } break;
        case IfLoopsAndNotCompileTimeSensitive: {
            if (comp()->mayHaveLoops() && comp()->getOption(TR_NotCompileTimeSensitive))
                doThisOptimization = true;
        } break;
        case MarkLastRun:
            doThisOptimization = true;
            TR_ASSERT(optNum < OMR::numOpts, "No current support for marking groups as last (optNum=%d,numOpt=%d\n",
                optNum, OMR::numOpts); // make sure we didn't mark groups
            manager->setLastRun(true);
            justSetLastRun = true;
            break;
        default:
            TR_ASSERT(0, "unexpection optimization flags");
    }

    if (doThisOptimizationIfEnabled && manager->getRequestedBlocks()->isEmpty())
        doThisOptimization = false;

    int32_t actualCost = 0;
    static int32_t optDepth = 1;

    TR_FrontEnd *fe = comp()->fe();

    // If this is the start of an optimization subGroup, perform the
    // optimizations in the subgroup.
    //
    if (optNum > OMR::numOpts && doThisOptimization) {
        if (comp()->getOption(TR_TraceOptDetails) || comp()->getOption(TR_TraceOpts)) {
            if (comp()->isOutermostMethod())
                log->printf("%*s<optgroup name=%s>\n", optDepth * 3, " ", manager->name());
        }

        optDepth++;

        // Find the subgroup. It is either referenced directly from this
        // optimization or picked up from the table of groups using the
        // optimization number.
        //
        manager->setRequested(false);

        if (optNum == loopVersionerGroup && getOptimization(lastLoopVersionerGroup) != NULL)
            getOptimization(lastLoopVersionerGroup)->setRequested(false);

        const OptimizationStrategy *subGroup = ((TR::OptimizationManager *)manager)->groupOfOpts();
        const OptimizationStrategy *origSubGroup = subGroup;
        int32_t numIters = 0;

        while (1) {
            // Perform the optimizations in the subgroup
            //
            while (subGroup->_num != endGroup && subGroup->_num != endOpts) {
                actualCost += performOptimization(subGroup, firstOptIndex, lastOptIndex, doTiming);
                subGroup++;
            }

            numIters++;

            if (optNum == eachLocalAnalysisPassGroup) {
                const OptimizationStrategy *currSubGroup = subGroup;
                subGroup = origSubGroup;
                bool blocksArePending = false;
                while (subGroup->_num != endGroup && subGroup->_num != endOpts) {
                    OMR::Optimizations optNum = subGroup->_num;
                    if (!manager->getRequestedBlocks()->isEmpty()) {
                        blocksArePending = true;
                        break;
                    }
                    subGroup++;
                }

                subGroup = currSubGroup;
                if (!blocksArePending || (numIters >= MAX_LOCAL_OPTS_ITERS)) {
                    break;
                } else
                    subGroup = origSubGroup;
            } else
                break;
        }

        optDepth--;

        if (comp()->getOption(TR_TraceOptDetails) || comp()->getOption(TR_TraceOpts)) {
            if (comp()->isOutermostMethod())
                log->printf("%*s</optgroup>\n", optDepth * 3, " ");
        }

        return actualCost;
    }

    //
    // This is a real optimization.
    //
    TR::RegionProfiler rp(comp()->trMemory()->heapMemoryRegion(), *comp(), "opt/%s/%s",
        comp()->getHotnessName(comp()->getMethodHotness()), getOptimizationName(optNum));

    if (comp()->isOutermostMethod())
        comp()->incOptIndex(); // Note that we count the opt even if we're not doing it, to keep the opt indexes more
                               // stable

    if (!doThisOptimization) {
        if (!manager->requested() && !manager->getRequestedBlocks()->isEmpty()) {
            TR_ASSERT(0, "Opt is disabled but blocks are still present\n");
        }
        return 0;
    }

    if (mustBeDone || (optIndex >= firstOptIndex && optIndex <= lastOptIndex)) {
        bool needTreeDump = false;
        bool needStructureDump = false;

        if (!isEnabled(optNum))
            return 0;

        TR::SimpleRegex *regex = comp()->getOptions()->getDisabledOpts();
        if (regex && TR::SimpleRegex::match(regex, optIndex))
            return 0;

        if (regex && TR::SimpleRegex::match(regex, manager->name()))
            return 0;

        // actually doing optimization
        regex = comp()->getOptions()->getBreakOnOpts();
        if (regex && TR::SimpleRegex::match(regex, optIndex))
            TR::Compiler->debug.breakPoint();

        TR::Optimization *opt = manager->factory()(manager);

        // Do any opt specific checks before analysis/opt is run
        if (!opt->shouldPerform()) {
            delete opt;
            return 0;
        }

        if (comp()->getOption(TR_TraceOptDetails)) {
            if (comp()->isOutermostMethod())
                getDebug()->printOptimizationHeader(log, comp()->signature(), manager->name(), optIndex,
                    optimization->_options == MustBeDone);
        }

        if (comp()->getOption(TR_TraceOpts)) {
            if (comp()->isOutermostMethod())
                log->printf("%*s%s\n", optDepth * 3, " ", manager->name());
        }

        if (!_aliasSetsAreValid && !manager->getDoesNotRequireAliasSets()) {
            TR::Compilation::CompilationPhaseScope buildingAliases(comp());
            comp()->reportAnalysisPhase(BUILDING_ALIASES);
            breakForTesting(1020);
            dumpOptDetails(comp(), "   (Building alias info)\n");
            comp()->getSymRefTab()->aliasBuilder.createAliasInfo();
            _aliasSetsAreValid = true;
            ++actualCost;
        }
        breakForTesting(1021);

        if (manager->getRequiresUseDefInfo() || manager->getRequiresValueNumbering())
            manager->setRequiresStructure(true);

        if (manager->getRequiresStructure() && !comp()->getFlowGraph()->getStructure()) {
            TR::Compilation::CompilationPhaseScope buildingStructure(comp());
            comp()->reportAnalysisPhase(BUILDING_STRUCTURE);
            breakForTesting(1030);
            dumpOptDetails(comp(), "   (Doing structural analysis)\n");

#ifdef OPT_TIMING
            TR_SingleTimer myTimer;
            if (doTiming) {
                myTimer.initialize("structural analysis", trMemory());
                myTimer.startTiming(comp());
            }
#endif

            actualCost += doStructuralAnalysis();

            if (_firstTimeStructureIsBuilt && comp()->getFlowGraph()->getStructure()) {
                _firstTimeStructureIsBuilt = false;
                _numLoopsInMethod = 0;
                countNumberOfLoops(comp()->getFlowGraph()->getStructure());
                // dumpOptDetails(comp(), "Number of loops in the cfg = %d\n", _numLoopsInMethod);

                if (!comp()->getOption(TR_ProcessHugeMethods) && (_numLoopsInMethod >= (HIGH_LOOP_COUNT - 25)))
                    _disableLoopOptsThatCanCreateLoops = true;
                _numLoopsInMethod = 0;
            }

            needStructureDump = true;

#ifdef OPT_TIMING
            if (doTiming) {
                myTimer.stopTiming(comp());
                statStructuralAnalysisTiming.update(
                    (double)myTimer.timeTaken() * 1000.0 / TR::Compiler->vm.getHighResClockResolution());
            }
#endif
        }
        breakForTesting(1031);

        if (manager->getStronglyPrefersGlobalsValueNumbering() && getUseDefInfo()
            && !getUseDefInfo()->hasGlobalsUseDefs() && !cantBuildGlobalsUseDefInfo()) {
            // We would strongly prefer global usedef info, but we only have
            // local usedef info. We can build global usedef info so force a
            // rebuild.
            //
            setUseDefInfo(NULL);
        }

        if (manager->getDoesNotRequireLoadsAsDefsInUseDefs() && getUseDefInfo() && getUseDefInfo()->hasLoadsAsDefs()) {
            setUseDefInfo(NULL);
        }

        if (!manager->getDoesNotRequireLoadsAsDefsInUseDefs() && getUseDefInfo()
            && !getUseDefInfo()->hasLoadsAsDefs()) {
            setUseDefInfo(NULL);
        }

        TR_UseDefInfo *useDefInfo;
        if (manager->getRequiresGlobalsUseDefInfo() || manager->getRequiresGlobalsValueNumbering()) {
            // We need global usedef info. If it doesn't exist but can be built,
            // build it.
            //
            if (!cantBuildGlobalsUseDefInfo() && (!getUseDefInfo() || !getUseDefInfo()->hasGlobalsUseDefs())) {
                TR::Compilation::CompilationPhaseScope buildingUseDefs(comp());
                comp()->reportAnalysisPhase(BUILDING_USE_DEFS);
                breakForTesting(1040);
#ifdef OPT_TIMING
                TR_SingleTimer myTimer;
                if (doTiming) {
                    myTimer.initialize("use defs (for globals definitely)", trMemory());
                    myTimer.startTiming(comp());
                }
#endif

                LexicalTimer t("use defs (for globals definitely)", comp()->phaseTimer());
                TR::LexicalMemProfiler mp("use defs (for globals definitely)", comp()->phaseMemProfiler());
                useDefInfo = createUseDefInfo(comp(),
                    true, // requiresGlobals
                    false, // prefersGlobals
                    !manager->getDoesNotRequireLoadsAsDefsInUseDefs(), manager->getCannotOmitTrivialDefs(),
                    false, // conversionRegsOnly
                    true); // doCompletion

#ifdef OPT_TIMING
                if (doTiming) {
                    myTimer.stopTiming(comp());
                    statUseDefsTiming.update(
                        (double)myTimer.timeTaken() * 1000.0 / TR::Compiler->vm.getHighResClockResolution());
                }
#endif

                if (useDefInfo->infoIsValid()) {
                    setUseDefInfo(useDefInfo);
                } else
                    // release storage for failed _useDefInfo
                    delete useDefInfo;

                actualCost += 10;
                needTreeDump = true;
            }
        }

        else if (manager->getRequiresUseDefInfo() || manager->getRequiresValueNumbering()) {
            if (!cantBuildLocalsUseDefInfo() && !getUseDefInfo()) {
                TR::Compilation::CompilationPhaseScope buildingUseDefs(comp());
                comp()->reportAnalysisPhase(BUILDING_USE_DEFS);
                breakForTesting(1050);
#ifdef OPT_TIMING
                TR_SingleTimer myTimer;
                if (doTiming) {
                    myTimer.initialize("use defs (for globals possibly)", trMemory());
                    myTimer.startTiming(comp());
                }
#endif
                LexicalTimer t("use defs (for globals possibly)", comp()->phaseTimer());
                TR::LexicalMemProfiler mp("use defs (for globals possibly)", comp()->phaseMemProfiler());
                useDefInfo = createUseDefInfo(comp(),
                    false, // requiresGlobals
                    manager->getPrefersGlobalsUseDefInfo() || manager->getPrefersGlobalsValueNumbering(),
                    !manager->getDoesNotRequireLoadsAsDefsInUseDefs(), manager->getCannotOmitTrivialDefs(),
                    false, // conversionRegsOnly
                    true); // doCompletion

#ifdef OPT_TIMING
                if (doTiming) {
                    myTimer.stopTiming(comp());
                    statUseDefsTiming.update(
                        (double)myTimer.timeTaken() * 1000.0 / TR::Compiler->vm.getHighResClockResolution());
                }
#endif

                if (useDefInfo->infoIsValid()) {
                    setUseDefInfo(useDefInfo);
                } else
                    // release storage for failed _useDefInfo
                    delete useDefInfo;

                actualCost += 10;
                needTreeDump = true;
            }
        }

        TR_ValueNumberInfo *valueNumberInfo;
        if (manager->getRequiresGlobalsValueNumbering()) {
            // We need global value number info.
            // If it doesn't exist but can be built, build it.
            //
            if (!cantBuildGlobalsValueNumberInfo()
                && (!getValueNumberInfo() || !getValueNumberInfo()->hasGlobalsValueNumbers())) {
                TR::Compilation::CompilationPhaseScope buildingValueNumbers(comp());
                comp()->reportAnalysisPhase(BUILDING_VALUE_NUMBERS);
                breakForTesting(1060);
#ifdef OPT_TIMING
                TR_SingleTimer myTimer;
                if (doTiming) {
                    myTimer.initialize("global value numbering (for globals definitely)", trMemory());
                    myTimer.startTiming(comp());
                }
#endif

                valueNumberInfo = createValueNumberInfo(true, false);

#ifdef OPT_TIMING
                if (doTiming) {
                    myTimer.stopTiming(comp());
                    statGlobalValNumTiming.update(
                        (double)myTimer.timeTaken() * 1000.0 / TR::Compiler->vm.getHighResClockResolution());
                }
#endif

                if (valueNumberInfo->infoIsValid())
                    setValueNumberInfo(valueNumberInfo);
                actualCost += 10;
                needTreeDump = true;
            }
        }

        else if (manager->getRequiresValueNumbering()) {
            if (!cantBuildLocalsValueNumberInfo() && !getValueNumberInfo()) {
                TR::Compilation::CompilationPhaseScope buildingValueNumbers(comp());
                comp()->reportAnalysisPhase(BUILDING_VALUE_NUMBERS);
                breakForTesting(1070);
#ifdef OPT_TIMING
                TR_SingleTimer myTimer;
                if (doTiming) {
                    myTimer.initialize("global value numbering (for globals possibly)", trMemory());
                    myTimer.startTiming(comp());
                }
#endif

                valueNumberInfo = createValueNumberInfo(false, manager->getPrefersGlobalsValueNumbering());

#ifdef OPT_TIMING
                if (doTiming) {
                    myTimer.stopTiming(comp());
                    statGlobalValNumTiming.update(
                        (double)myTimer.timeTaken() * 1000.0 / TR::Compiler->vm.getHighResClockResolution());
                }
#endif
                if (valueNumberInfo->infoIsValid())
                    setValueNumberInfo(valueNumberInfo);
                actualCost += 10;
                needTreeDump = true;
            }
        }

        if (manager->getRequiresAccurateNodeCount()) {
            TR::Compilation::CompilationPhaseScope buildingAccurateNodeCount(comp());
            comp()->reportAnalysisPhase(BUILDING_ACCURATE_NODE_COUNT);
            breakForTesting(1080);
            comp()->generateAccurateNodeCount();
        }

        // dumpOptDetails(comp(), "\n");

#ifdef OPT_TIMING
        if (*(statOptTiming[optNum].getName()) == 0) // has no name yet
            statOptTiming[optNum].setName(manager->name());
#endif

#ifdef OPT_TIMING
        TR_SingleTimer myTimer;
        if (doTiming) {
            myTimer.initialize(manager->name(), trMemory());
            myTimer.startTiming(comp());
        }
#endif
        LexicalTimer t(manager->name(), comp()->phaseTimer());
        TR::LexicalMemProfiler mp(manager->name(), comp()->phaseMemProfiler());

        int32_t origSymRefCount = comp()->getSymRefCount();
        int32_t origNodeCount = comp()->getNodeCount();
        int32_t origCfgNodeCount = comp()->getFlowGraph()->getNextNodeNumber();
        int32_t origOptMsgIndex = self()->getOptMessageIndex();

        if (comp()->isOutermostMethod() && (comp()->getFlowGraph()->getMaxFrequency() < 0)
            && !manager->getDoNotSetFrequencies()) {
            TR::Compilation::CompilationPhaseScope buildingFrequencies(comp());
            comp()->reportAnalysisPhase(BUILDING_FREQUENCIES);
            breakForTesting(1100);
            comp()->getFlowGraph()->setFrequencies();
        }

        bool origTraceSetting = manager->trace();

        regex = comp()->getOptions()->getOptsToTrace();
        if (regex && TR::SimpleRegex::match(regex, optIndex))
            manager->setTrace(true);

        if (doThisOptimizationIfEnabled)
            manager->setPerformOnlyOnEnabledBlocks(true);

        bool methodTreeDumpPermitted = (optIndex >= _firstDumpOptPhaseTrees && optIndex <= _lastDumpOptPhaseTrees);

        // Check whether this optimization's index or name matches the
        // filter specified to request dumps of method trees
        //
        regex = comp()->getOptions()->getOptsToDumpTrees();
        if (regex && (TR::SimpleRegex::match(regex, optIndex) || TR::SimpleRegex::match(regex, manager->name()))) {
            needTreeDump = true;
            methodTreeDumpPermitted = true;
        }

        // check if method exceeds loop or basic block threshold
        if (manager->getRequiresStructure() && comp()->getFlowGraph()->getStructure()) {
            if (checkNumberOfLoopsAndBasicBlocks(comp(), comp()->getFlowGraph()->getStructure())) {
                if (comp()->getOption(TR_ProcessHugeMethods)) {
                    dumpOptDetails(comp(),
                        "Method is normally too large (%d blocks and %d loops) but limits overridden\n",
                        _numBasicBlocksInMethod, _numLoopsInMethod);
                } else {
                    if (comp()->getOption(TR_MimicInterpreterFrameShape)) {
                        comp()->failCompilation<TR::ExcessiveComplexity>(
                            "complex method under MimicInterpreterFrameShape");
                    } else {
                        comp()->failCompilation<TR::ExcessiveComplexity>("Method is too large");
                    }
                }
            }
        }

        comp()->reportOptimizationPhase(optNum);
        breakForTesting(optNum);
        if (!doThisOptimizationIfEnabled
            || manager->getRequestedBlocks()->find(toBlock(comp()->getFlowGraph()->getStart()))
            || manager->getRequestedBlocks()->find(toBlock(comp()->getFlowGraph()->getEnd()))) {
            TR_ASSERT((justSetLastRun || !manager->getLastRun()), "%s shouldn't be run after LastRun was set\n",
                manager->name());

            manager->setRequested(false);

            comp()->recordBegunOpt();
            if (comp()->getOption(TR_TraceLastOpt)
                && comp()->getOptIndex() == comp()->getOptions()->getLastOptIndex()) {
                comp()->getOptions()->enableTracing(optNum);
                manager->setTrace(true);
            }

            comp()->reportAnalysisPhase(PERFORMING_OPTIMIZATION);

            {
                TR::StackMemoryRegion stackMemoryRegion(*trMemory());
                opt->prePerform();
                actualCost += opt->perform();
                opt->postPerform();
            }

            comp()->reportAnalysisPhase(AFTER_OPTIMIZATION);
        } else if (canRunBlockByBlockOptimizations()) {
            TR::StackMemoryRegion stackMemoryRegion(*trMemory());

            opt->prePerformOnBlocks();
            ListIterator<TR::Block> blockIt(manager->getRequestedBlocks());
            manager->setRequested(false);
            manager->setPerformOnlyOnEnabledBlocks(false);
            for (TR::Block *block = blockIt.getFirst(); block != NULL; block = blockIt.getNext()) {
                // if (!comp()->getFlowGraph()->getRemovedNodes().find(block))
                if (!block->nodeIsRemoved()) {
                    block = block->startOfExtendedBlock();
                    TR_ASSERT((justSetLastRun || !manager->getLastRun()),
                        "opt %d shouldn't be run after LastRun was set for this optimization\n", optNum);
                    actualCost += opt->performOnBlock(block);
                }
            }
            opt->postPerformOnBlocks();
        }

        delete opt;
        // we cannot easily invalidate during IL gen since we could be peeking and we cannot destroy our
        // caller's alias sets
        if (!isIlGenOpt())
            comp()->invalidateAliasRegion();
        breakForTesting(-optNum);

        if (comp()->compilationShouldBeInterrupted((TR_CallingContext)optNum)) {
            comp()->failCompilation<TR::CompilationInterrupted>("interrupted between optimizations");
        }

        manager->setTrace(origTraceSetting);

        int32_t finalOptMsgIndex = self()->getOptMessageIndex();
        if ((finalOptMsgIndex != origOptMsgIndex) && !manager->getDoesNotRequireTreeDumps())
            comp()->reportOptimizationPhaseForSnap(optNum);

        if (comp()->getNodeCount() > unsigned(origNodeCount)) {
            // If nodes were added, invalidate
            //
            setValueNumberInfo(NULL);
            if (!manager->getMaintainsUseDefInfo())
                setUseDefInfo(NULL);
        }

        if ((comp()->getSymRefCount() != origSymRefCount) /* || manager->getCanAddSymbolReference()*/) {
            setSymReferencesTable(NULL);
            // invalidate any alias sets so that they are rebuilt
            // by the next optimization that needs them
            //
            setAliasSetsAreValid(false);
        }

        if (comp()->getVisitCount() > HIGH_VISIT_COUNT) {
            comp()->resetVisitCounts(1);
            dumpOptDetails(comp(), "\nResetting visit counts for this method after %s\n", manager->name());
        }

        if (comp()->getFlowGraph()->getMightHaveUnreachableBlocks())
            comp()->getFlowGraph()->removeUnreachableBlocks();

#ifdef OPT_TIMING
        if (doTiming) {
            myTimer.stopTiming(comp());
            statOptTiming[optNum].update(
                (double)myTimer.timeTaken() * 1000.0 / TR::Compiler->vm.getHighResClockResolution());
        }
#endif

#ifdef DEBUG
        if (manager->getDumpStructure() && debug("dumpStructure")) {
            log->printf("\nStructures:\n");
            getDebug()->print(comp()->log(), comp()->getFlowGraph()->getStructure(), 6);
        }

#endif

        if (methodTreeDumpPermitted && comp()->isOutermostMethod()) {
            if (manager->getDoesNotRequireTreeDumps()) {
                dumpOptDetails(comp(), "Trivial opt -- omitting listings\n");
            } else if (needTreeDump || (finalOptMsgIndex != origOptMsgIndex))
                comp()->dumpMethodTrees(log, "Trees after ", manager->name(), getMethodSymbol());
            else if (finalOptMsgIndex == origOptMsgIndex) {
                dumpOptDetails(comp(), "No transformations done by this pass -- omitting listings\n");
                if (needStructureDump && comp()->getDebug() && comp()->getFlowGraph()->getStructure()) {
                    comp()->getDebug()->print(comp()->log(), comp()->getFlowGraph()->getStructure(), 6);
                }
            }
        }

#ifdef DEBUG
        if (debug("dumpGraphs") && (_dumpGraphsIndex == -1 || _dumpGraphsIndex == optIndex))
            comp()->dumpMethodGraph(optIndex);
#endif

        manager->performChecks();

        static const bool enableCountTemps = feGetEnv("TR_EnableCountTemps") != NULL;
        if (enableCountTemps) {
            int32_t tempCount = 0;

            log->printf("Temps seen (if any): ");

            for (TR::TreeTop *tt = getMethodSymbol()->getFirstTreeTop(); tt; tt = tt->getNextTreeTop()) {
                TR::Node *ttNode = tt->getNode();

                if (ttNode->getOpCodeValue() == TR::treetop) {
                    ttNode = ttNode->getFirstChild();
                }

                if (ttNode->getOpCode().isStore() && ttNode->getOpCode().hasSymbolReference()) {
                    TR::SymbolReference *symRef = ttNode->getSymbolReference();

                    if ((symRef->getSymbol()->getKind() == TR::Symbol::IsAutomatic) && symRef->isTemporary(comp())) {
                        ++tempCount;
                        log->printf("%s ", comp()->getDebug()->getName(ttNode->getSymbolReference()));
                    }
                }
            }

            log->printf("\nNumber of temps seen = %d\n", tempCount);
        }

        if (comp()->getOption(TR_TraceOptDetails)) {
            if (comp()->isOutermostMethod())
                log->printf("</optimization>\n\n");
        }
    }

    return actualCost;
}

void OMR::SmallOptimizer::enableAllLocalOpts()
{
    setRequestOptimization(localCSE, true);
    setRequestOptimization(treeSimplification, true);
    setRequestOptimization(localDeadStoreElimination, true);
    setRequestOptimization(deadTreesElimination, true);
    setRequestOptimization(compactNullChecks, true);
    setRequestOptimization(redundantGotoElimination, true);
}

int32_t OMR::SmallOptimizer::doStructuralAnalysis()
{
    // Only perform structural analysis if there may be loops in the method
    //
    // TEMPORARY HACK - always do structural analysis
    //
    TR_Structure *rootStructure = NULL;
    /////if (comp()->mayHaveLoops())
    {
        LexicalTimer t("StructuralAnalysis", comp()->phaseTimer());
        rootStructure = TR_RegionAnalysis::getRegions(comp());
        comp()->getFlowGraph()->setStructure(rootStructure);

        if (debug("dumpStructure")) {
            comp()->log()->printf("\nStructures:\n");
            getDebug()->print(comp()->log(), rootStructure, 6);
        }
    }

    return 10;
}

int32_t OMR::SmallOptimizer::changeContinueLoopsToNestedLoops()
{
    TR_RegionStructure *rootStructure = comp()->getFlowGraph()->getStructure()->asRegion();
    if (rootStructure && rootStructure->changeContinueLoopsToNestedLoops(rootStructure)) {
        comp()->getFlowGraph()->setStructure(NULL);
        doStructuralAnalysis();
    }

    return 10;
}

bool OMR::SmallOptimizer::prepareForNodeRemoval(TR::Node *node, bool deferInvalidatingUseDefInfo)
{
    int32_t index;

    TR_UseDefInfo *udInfo = getUseDefInfo();
    bool useDefInfoAreInvalid = false;
    if (udInfo) {
        index = node->getUseDefIndex();
        if (udInfo->isUseIndex(index)) {
            // udInfo->setUseDefInfoToNull(index);
            udInfo->resetDefUseInfo();

            // If the node is both a use and a def we can't repair the info, since
            // it is a def to other uses that we don't know about (it's an unresolved
            // load, which acts like a call def node).
            //
            if (udInfo->isDefIndex(index)) {
                if (!deferInvalidatingUseDefInfo)
                    setUseDefInfo(NULL);
                useDefInfoAreInvalid = true;
            }
        }
        node->setUseDefIndex(0);
    }

    TR_ValueNumberInfo *vnInfo = getValueNumberInfo();
    if (vnInfo) {
        vnInfo->removeNodeInfo(node);
    }

    for (int32_t i = node->getNumChildren() - 1; i >= 0; i--) {
        TR::Node *child = node->getChild(i);
        if (child != NULL && child->getReferenceCount() == 1)
            if (prepareForNodeRemoval(child))
                useDefInfoAreInvalid = true;
    }
    return useDefInfoAreInvalid;
}

void OMR::SmallOptimizer::getStaticFrequency(TR::Block *block, int32_t *currentWeight)
{
    if (comp()->getUsesBlockFrequencyInGRA())
        *currentWeight = block->getFrequency();
    else
        block->getStructureOf()->calculateFrequencyOfExecution(currentWeight);
}

TR_Hotness OMR::SmallOptimizer::checkMaxHotnessOfInlinedMethods(TR::Compilation *comp)
{
    TR_Hotness strategy = comp->getMethodHotness();
#ifdef J9_PROJECT_SPECIFIC
    if (comp->getNumInlinedCallSites() > 0) {
        for (uint32_t i = 0; i < comp->getNumInlinedCallSites(); ++i) {
            TR_InlinedCallSite &ics = comp->getInlinedCallSite(i);
            TR_OpaqueMethodBlock *method = comp->fe()->getInlinedCallSiteMethod(&ics);
            if (TR::Compiler->mtd.isCompiledMethod(method)) {
                TR_PersistentJittedBodyInfo *bodyInfo
                    = TR::Recompilation::getJittedBodyInfoFromPC((void *)TR::Compiler->mtd.startPC(method));
                if (bodyInfo && bodyInfo->getHotness() > strategy) {
                    strategy = bodyInfo->getHotness();
                } else if (!bodyInfo
                    && TR::Options::getCmdLineOptions()->allowRecompilation()) // don't do it for fixed level
                {
                    strategy = scorching;
                    break;
                }
            }
        }
    }
#endif
    return strategy;
}

bool OMR::SmallOptimizer::checkNumberOfLoopsAndBasicBlocks(TR::Compilation *comp, TR_Structure *rootStructure)
{
    TR::CFGNode *node;
    _numBasicBlocksInMethod = 0;
    for (node = comp->getFlowGraph()->getFirstNode(); node; node = node->getNext()) {
        _numBasicBlocksInMethod++;
    }

    // dumpOptDetails(comp(), "Number of nodes in the cfg = %d\n", _numBasicBlocksInMethod);

    _numLoopsInMethod = 0;
    countNumberOfLoops(rootStructure);
    // dumpOptDetails(comp(), "Number of loops in the cfg = %d\n", _numLoopsInMethod);

    int32_t highBasicBlockCount = HIGH_BASIC_BLOCK_COUNT;
    int32_t highLoopCount = HIGH_LOOP_COUNT;
    // set loop count thershold to a higher value for now
    // TODO: find a better way to fix this by creating a check
    // about _disableLoopOptsThatCanCreateLoops
    if (comp->getMethodHotness() >= veryHot)
        highLoopCount = VERY_HOT_HIGH_LOOP_COUNT;
    if (comp->isOptServer()) {
        highBasicBlockCount = highBasicBlockCount * 2;
        highLoopCount = highLoopCount * 2;
    }

    if ((_numBasicBlocksInMethod >= highBasicBlockCount) || (_numLoopsInMethod >= highLoopCount)) {
        return true;
    }
    return false;
}

void OMR::SmallOptimizer::countNumberOfLoops(TR_Structure *rootStructure)
{
    TR_RegionStructure *regionStructure = rootStructure->asRegion();
    if (regionStructure) {
        if (regionStructure->isNaturalLoop())
            _numLoopsInMethod++;
        TR_StructureSubGraphNode *node;
        TR_RegionStructure::Cursor si(*regionStructure);
        for (node = si.getFirst(); node; node = si.getNext())
            countNumberOfLoops(node->getStructure());
    }
}

bool OMR::SmallOptimizer::areNodesEquivalent(TR::Node *node1, TR::Node *node2, TR::Compilation *_comp,
    bool allowBCDSignPromotion)
{
    // WCodeLinkageFixup runs a version of LocalCSE that is not owned by
    // an optimizer, so it has to pass in a TR_Compilation

    if (node1 == node2)
        return true;

    if (!(node1->getOpCodeValue() == node2->getOpCodeValue()))
        return false;

    TR::ILOpCode &opCode1 = node1->getOpCode();
    if (opCode1.isSwitch() == 0) {
        if (opCode1.hasSymbolReference()) {
            if (node1->getSymbolReference()->getReferenceNumber()
                != node2->getSymbolReference()->getReferenceNumber()) {
                return false;
            } else if ((opCode1.isCall() && !node1->isPureCall()) || opCode1.isStore()
                || opCode1.getOpCodeValue() == TR::New || opCode1.getOpCodeValue() == TR::newarray
                || opCode1.getOpCodeValue() == TR::anewarray || opCode1.getOpCodeValue() == TR::multianewarray
                || opCode1.getOpCodeValue() == TR::monent || opCode1.getOpCodeValue() == TR::monexit) {
                if (!(node1 == node2))
                    return false;
            }
        } else if (opCode1.isBranch()) {
            if (!(node1->getBranchDestination()->getNode() == node2->getBranchDestination()->getNode()))
                return false;
        }

#ifdef J9_PROJECT_SPECIFIC
        if (node1->getOpCode().isSetSignOnNode() && node1->getSetSign() != node2->getSetSign())
            return false;
#endif

        if (opCode1.isLoadConst()) {
            switch (node1->getDataType()) {
                case TR::Int8:
                    if (node1->getByte() != node2->getByte())
                        return false;
                    break;
                case TR::Int16:
                    if (node1->getShortInt() != node2->getShortInt())
                        return false;
                    break;
                case TR::Int32:
                    if (node1->getInt() != node2->getInt())
                        return false;
                    break;
                case TR::Int64:
                    if (node1->getLongInt() != node2->getLongInt())
                        return false;
                    break;
                case TR::Float:
                    if (node1->getFloatBits() != node2->getFloatBits())
                        return false;
                    break;
                case TR::Double:
                    if (node1->getDoubleBits() != node2->getDoubleBits())
                        return false;
                    break;
                case TR::Address:
                    if (node1->getAddress() != node2->getAddress())
                        return false;
                    break;
#ifdef J9_PROJECT_SPECIFIC
                case TR::Aggregate:
                    if (!areBCDAggrConstantNodesEquivalent(node1, node2, _comp)) {
                        return false;
                    }
#endif
                    break;
                default: {
                    TR_ASSERT_FATAL(!node1->getDataType().isMask(), "OMR does not support mask constants\n");

                    if (node1->getDataType().isVector()) {
                        if (node1->getLiteralPoolOffset() != node2->getLiteralPoolOffset())
                            return false;
                        break;
                    }
#ifdef J9_PROJECT_SPECIFIC
                    if (node1->getDataType().isBCD()) {
                        if (!areBCDAggrConstantNodesEquivalent(node1, node2, _comp))
                            return false;
                    }
#endif
                }
            }
        } else if (opCode1.isArrayLength()) {
            if (node1->getArrayStride() != node2->getArrayStride())
                return false;
        }
#ifdef J9_PROJECT_SPECIFIC
        else if (node1->getType().isBCD()) {
            if (node1->isDecimalSizeAndShapeEquivalent(node2)) {
                // LocalAnalysis temporarily changes store opcodes to load opcodes to enable matching up loads/stores
                // However since sign state is not tracked (and is not relevant) for stores this causes the equivalence
                // test to unnecessarily fail. The isBCDStoreTemporarilyALoad flag allow skipping of the sign state
                // compare for these cases.
                if (!(node1->getOpCode().isLoadVar() && node1->isBCDStoreTemporarilyALoad())
                    && !(node2->getOpCode().isLoadVar() && node2->isBCDStoreTemporarilyALoad())
                    && !node1->isSignStateEquivalent(node2)) {
                    if (allowBCDSignPromotion && node1->isSignStateAnImprovementOver(node2)) {
                        if (_comp->cg()->traceBCDCodeGen())
                            _comp->log()->printf(
                                "y^y : found sign state mismatch node1 %s (%p), node2 %s (%p) but node1 improves sign "
                                "state over node2\n",
                                node1->getOpCode().getName(), node1, node2->getOpCode().getName(), node2);
                        return true;
                    } else {
                        if (_comp->cg()->traceBCDCodeGen())
                            _comp->log()->printf("x^x : found sign state mismatch node1 %s (%p), node2 %s (%p)\n",
                                node1->getOpCode().getName(), node1, node2->getOpCode().getName(), node2);
                        return false;
                    }
                }
            } else {
                return false;
            }
        } else if (opCode1.isConversionWithFraction() && node1->getDecimalFraction() != node2->getDecimalFraction()) {
            return false;
        } else if (node1->chkOpsCastedToBCD() && node1->castedToBCD() != node2->castedToBCD()) {
            return false;
        } else if (opCode1.getOpCodeValue() == TR::loadaddr
            && (node1->getSymbolReference()->isTempVariableSizeSymRef()
                && node2->getSymbolReference()->isTempVariableSizeSymRef())
            && (node1->getDecimalPrecision() != node2->getDecimalPrecision())) {
            return false;
        }
#endif
        else if (opCode1.isArrayRef()) {
            // for some reason this tests hasPinningArrayPointer only when the node also is true on
            // _flags.testAny(internalPointer)
            bool haveIPs = node1->isInternalPointer() && node2->isInternalPointer();
            bool haveNoIPs = !node1->isInternalPointer() && !node2->isInternalPointer();
            TR::AutomaticSymbol *pinning1 = node1->hasPinningArrayPointer() ? node1->getPinningArrayPointer() : NULL;
            TR::AutomaticSymbol *pinning2 = node2->hasPinningArrayPointer() ? node2->getPinningArrayPointer() : NULL;
            if ((haveIPs && (pinning1 == pinning2)) || haveNoIPs)
                return true;
            else
                return false;
        } else if (opCode1.getOpCodeValue() == TR::PassThrough) {
            return false;
        } else if (opCode1.isLoadReg()) {
            if (!node2->getOpCode().isLoadReg()) {
                return false;
            }

            if (node1->getGlobalRegisterNumber() != node2->getGlobalRegisterNumber()) {
                return false;
            }
        } // IvanB
    } else {
        if (!(areNodesEquivalent(node1->getFirstChild(), node2->getFirstChild(), _comp)))
            return false;

        if (!(node1->getSecondChild()->getBranchDestination()->getNode()
                == node2->getSecondChild()->getBranchDestination()->getNode()))
            return false;

        if (opCode1.getOpCodeValue() == TR::lookup) {
            for (int i = node1->getCaseIndexUpperBound() - 1; i > 1; i--) {
                if (!(node1->getChild(i)->getBranchDestination()->getNode()
                        == node2->getChild(i)->getBranchDestination()->getNode()))
                    return false;
            }
        } else if (opCode1.getOpCodeValue() == TR::table) {
            for (int i = node1->getCaseIndexUpperBound() - 1; i > 1; i--) {
                if (!(node1->getChild(i)->getBranchDestination()->getNode()
                        == node2->getChild(i)->getBranchDestination()->getNode()))
                    return false;
            }
        }
    }

    return true;
}

#ifdef J9_PROJECT_SPECIFIC
bool OMR::SmallOptimizer::areBCDAggrConstantNodesEquivalent(TR::Node *node1, TR::Node *node2, TR::Compilation *_comp)
{
    size_t size1 = (node1->getDataType().isBCD()) ? node1->getDecimalPrecision() : 0;
    size_t size2 = (node2->getDataType().isBCD()) ? node2->getDecimalPrecision() : 0;

    if (size1 != size2) {
        return false;
    }
    if (node1->getNumChildren() == 1 && node2->getNumChildren() == 1 && // if neither is a delayed literal,
        node1->getLiteralPoolOffset() != node2->getLiteralPoolOffset()) // compare their offsets in the literal pool.
    {
        return false;
    }
    return true;
}
#endif

bool OMR::SmallOptimizer::areSyntacticallyEquivalent(TR::Node *node1, TR::Node *node2, vcount_t visitCount)
{
    if (node1->getVisitCount() == visitCount) {
        if (node2->getVisitCount() == visitCount)
            return true;
        else
            return false;
    }

    if (node2->getVisitCount() == visitCount) {
        if (node1->getVisitCount() == visitCount)
            return true;
        else
            return false;
    }

    bool equivalent = true;
    if (!areNodesEquivalent(node1, node2))
        equivalent = false;

    if (node1->getNumChildren() != node2->getNumChildren())
        equivalent = false;

    if (equivalent) {
        int32_t numChildren = node1->getNumChildren();
        int32_t i;
        for (i = numChildren - 1; i >= 0; i--) {
            TR::Node *child1 = node1->getChild(i);
            TR::Node *child2 = node2->getChild(i);

            if (!areSyntacticallyEquivalent(child1, child2, visitCount)) {
                equivalent = false;
                break;
            }
        }
    }

    return equivalent;
}

/**
 * Build the table of corresponding symbol references for use by optimizations.
 * This table allows a fast determination of whether two symbol references
 * represent the same symbol.
 */
int32_t *OMR::SmallOptimizer::getSymReferencesTable()
{
    if (_symReferencesTable == NULL) {
        int32_t symRefCount = comp()->getSymRefCount();
        _symReferencesTable = (int32_t *)trMemory()->allocateStackMemory(symRefCount * sizeof(int32_t));
        memset(_symReferencesTable, 0, symRefCount * sizeof(int32_t));
        TR::SymbolReferenceTable *symRefTab = comp()->getSymRefTab();
        for (int32_t symRefNumber = 0; symRefNumber < symRefCount; symRefNumber++) {
            bool newSymbol = true;
            if (symRefNumber >= comp()->getSymRefTab()->getIndexOfFirstSymRef()) {
                TR::SymbolReference *symRef = symRefTab->getSymRef(symRefNumber);
                TR::Symbol *symbol = symRef ? symRef->getSymbol() : 0;
                if (symbol) {
                    for (int32_t i = comp()->getSymRefTab()->getIndexOfFirstSymRef(); i < symRefNumber; ++i) {
                        if (_symReferencesTable[i] == i) {
                            TR::SymbolReference *otherSymRef = symRefTab->getSymRef(i);
                            TR::Symbol *otherSymbol = otherSymRef ? otherSymRef->getSymbol() : 0;
                            if (otherSymbol && symbol == otherSymbol
                                && symRef->getOffset() == otherSymRef->getOffset()) {
                                newSymbol = false;
                                _symReferencesTable[symRefNumber] = i;
                                break;
                            }
                        }
                    }
                }
            }

            if (newSymbol)
                _symReferencesTable[symRefNumber] = symRefNumber;
        }
    }
    return _symReferencesTable;
}

#ifdef DEBUG
void OMR::SmallOptimizer::doStructureChecks()
{
    TR::CFG *cfg = getMethodSymbol()->getFlowGraph();
    if (cfg) {
        TR_Structure *rootStructure = cfg->getStructure();
        if (rootStructure) {
            TR::StackMemoryRegion stackMemoryRegion(*trMemory());

            // Allocate bit vector of block numbers that have been seen
            //
            TR_BitVector blockNumbers(cfg->getNextNodeNumber(), comp()->trMemory(), stackAlloc);
            rootStructure->checkStructure(&blockNumbers);
        }
    }
}
#endif

bool OMR::SmallOptimizer::getLastRun(OMR::Optimizations opt)
{
    if (!_opts[opt])
        return false;
    return _opts[opt]->getLastRun();
}

void OMR::SmallOptimizer::setRequestOptimization(OMR::Optimizations opt, bool value, TR::Block *block)
{
    if (_opts[opt])
        _opts[opt]->setRequested(value, block);
}

void OMR::SmallOptimizer::setAliasSetsAreValid(bool b, bool setForWCode)
{
    if (_aliasSetsAreValid && !b)
        dumpOptDetails(comp(), "     (Invalidating alias info)\n");

    _aliasSetsAreValid = b;
}

const OptimizationStrategy *OMR::SmallOptimizer::_mockStrategy = NULL;

const OptimizationStrategy *OMR::SmallOptimizer::optimizationStrategy(TR::Compilation *c)
{
    // Mock strategies are used for testing, and override
    // the compilation strategy.
    if (NULL != OMR::SmallOptimizer::_mockStrategy) {
        OMR::Logger *log = c->log();
        if (log->isEnabled_DEPRECATED())
            log->printf("Using mock optimization strategy %p\n", OMR::SmallOptimizer::_mockStrategy);

        return OMR::SmallOptimizer::_mockStrategy;
    }

    TR_Hotness strategy = c->getMethodHotness();
    TR_ASSERT(strategy <= lastOMRStrategy, "Invalid optimization strategy");

    // Downgrade strategy rather than crashing in prod.
    if (strategy > lastOMRStrategy)
        strategy = lastOMRStrategy;

    return omrCompilationStrategies[strategy];
}

ValueNumberInfoBuildType OMR::SmallOptimizer::valueNumberInfoBuildType() { return PrePartitionVN; }

TR::Optimizer *OMR::SmallOptimizer::self() { return (static_cast<TR::Optimizer *>(this)); }

const char *OMR::SmallOptimizer::getAnalysisPhaseName(OMR::SmallOptimizer::AnalysisPhases phaseId)
{
#define OPTIMIZER_ANALYSIS_PHASES_MACRO(analysis_phase, name, id, ...) \
    case id:                                                           \
        return name;

    switch (phaseId) {
#include "optimizer/OptimizerAnalysisPhases.enum"
        default:
            break;
    }

    return "Unknown analysis phase";
}

OMR_InlinerPolicy *OMR::SmallOptimizer::getInlinerPolicy()
{
    return new (comp()->allocator()) OMR_InlinerPolicy(comp());
}

OMR_InlinerUtil *OMR::SmallOptimizer::getInlinerUtil() { return new (comp()->allocator()) OMR_InlinerUtil(comp()); }
