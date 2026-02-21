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

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "codegen/CodeGenerator.hpp"
#include "env/FrontEnd.hpp"
#include "env/KnownObjectTable.hpp"
#include "codegen/RegisterConstants.hpp"
#include "compile/Compilation.hpp"
#include "compile/Method.hpp"
#include "compile/ResolvedMethod.hpp"
#include "compile/SymbolReferenceTable.hpp"
#include "control/Options.hpp"
#include "control/Options_inlines.hpp"
#include "env/CompilerEnv.hpp"
#include "env/IO.hpp"
#include "env/TRMemory.hpp"
#include "env/jittypes.h"
#include "il/Block.hpp"
#include "il/DataTypes.hpp"
#include "il/ILOpCodes.hpp"
#include "il/ILOps.hpp"
#include "il/MethodSymbol.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "il/RegisterMappedSymbol.hpp"
#include "il/ResolvedMethodSymbol.hpp"
#include "il/Symbol.hpp"
#include "il/SymbolReference.hpp"
#include "il/TreeTop.hpp"
#include "il/TreeTop_inlines.hpp"
#include "infra/Assert.hpp"
#include "infra/BitVector.hpp"
#include "infra/Cfg.hpp"
#include "infra/Flags.hpp"
#include "infra/List.hpp"
#include "infra/SimpleRegex.hpp"
#include "infra/CfgEdge.hpp"
#include "infra/CfgNode.hpp"
#include "optimizer/Optimizations.hpp"
#include "optimizer/Optimizer.hpp"
#include "optimizer/Structure.hpp"
#include "optimizer/StructuralAnalysis.hpp"
#include "optimizer/ValueNumberInfo.hpp"
#include "optimizer/VPConstraint.hpp"
#include "ras/Debug.hpp"
#include "ras/Logger.hpp"

#define DEFAULT_TREETOP_INDENT 2
#define DEFAULT_INDENT_INCREMENT 2

#define VSNPRINTF_BUFFER_SIZE 512

extern int32_t addressWidth;

void TR_Debug::printTopLegend(OMR::Logger *log)
{
    log->prints("\n----------------------------------------------------------------------------------------------------"
                "--------------------------------------------------------------------\n");
    log->flush();
}

void TR_Debug::printBottomLegend(OMR::Logger *log)
{
    log->prints("\n"
                "index:       node global index\n");
    char const *lineOrStatement = "line-number";
    char const *bciOrLoc = "bci";
    char const *bciOrLocVerbose = "bytecode-index";

    log->printf("%s=[x,y,z]: byte-code-info [callee-index, %s, %s]\n", bciOrLoc, bciOrLocVerbose, lineOrStatement);

    log->prints("rc:          reference count\n"
                "vc:          visit count\n"
                "vn:          value number\n"
                "li:          local index\n"
                "udi:         use/def index\n"
                "nc:          number of children\n"
                "addr:        address size in bytes\n"
                "flg:         node flags\n");

    log->flush();
}

// Prints the symbol reference table showing only symbols that were added during the last optimization and also sets
// TR::Compilationl::_prevSymRefTabSize to the current size of symbol reference table as it is the end of this
// optimization.
//
void TR_Debug::printSymRefTable(OMR::Logger *log, bool printFullTable)
{
    TR_PrettyPrinterString output(this);
    TR::SymbolReferenceTable *symRefTab = _comp->getSymRefTab();
    TR::SymbolReference *symRefIterator;
    int32_t currSymRefTabSize = symRefTab->baseArray.size();

    if (printFullTable)
        _comp->setPrevSymRefTabSize(0);

    if (currSymRefTabSize > 0
        && _comp->getPrevSymRefTabSize() < currSymRefTabSize) // entry(ies) have been added to symbol refetence table
                                                              // since last optimization was performed
    {
        if (printFullTable)
            log->prints("\nSymbol References:\n------------------\n");
        else
            log->prints("\nSymbol References (incremental):\n--------------------------------\n");

        for (int i = _comp->getPrevSymRefTabSize(); i < currSymRefTabSize; i++) {
            if ((symRefIterator = symRefTab->getSymRef(i))) {
                output.reset();
                print(symRefIterator, output, false /*hideHelperMethodInfo*/, true /*verbose*/);
                log->printf("%s\n", output.getStr());
            }
        }
        log->flush();
    }

    _comp->setPrevSymRefTabSize(_comp->getSymRefTab()->baseArray.size());
}

void TR_Debug::printOptimizationHeader(OMR::Logger *log, const char *funcName, const char *optName, int32_t optIndex,
    bool mustBeDone)
{
    log->printf("<optimization id=%d name=%s method=%s>\n", optIndex, optName ? optName : "???", funcName);
    log->printf("Performing %d: %s%s\n", optIndex, optName ? optName : "???", mustBeDone ? " mustBeDone" : "");
}

static bool valueIsProbablyHex(TR::Node *node)
{
    switch (node->getDataType()) {
        case TR::Int16:
            if (node->getShortInt() > 16384 || node->getShortInt() < -16384)
                return true;
            return false;
        case TR::Int32:
            if (node->getInt() > 16384 || node->getInt() < -16384)
                return true;
            return false;
        case TR::Int64:
            if (node->getLongInt() > 16384 || node->getLongInt() < -16384)
                return true;
            return false;
        default:
            return false;
    }
    return false;
}

void TR_Debug::printLoadConst(OMR::Logger *log, TR::Node *node)
{
    TR_PrettyPrinterString output(this);
    printLoadConst(node, output);
    log->prints(output.getStr());
    _comp->incrNodeOpCodeLength(output.getLength());
}

void TR_Debug::printLoadConst(TR::Node *node, TR_PrettyPrinterString &output)
{
    char const *fmtStr;
    bool isUnsigned = node->getOpCode().isUnsigned();
    switch (node->getDataType()) {
        case TR::Int8:
            if (isUnsigned)
                output.appendf(" %3u", node->getUnsignedByte());
            else
                output.appendf(" %3d", node->getByte());
            break;
        case TR::Int16:
            fmtStr = valueIsProbablyHex(node) ? " 0x%4x" : " '%5d' ";
            output.appendf(fmtStr, node->getConst<uint16_t>());
            break;
        case TR::Int32:
            if (isUnsigned) {
                fmtStr = valueIsProbablyHex(node) ? " 0x%x" : " %u";
                output.appendf(fmtStr, node->getUnsignedInt());
            } else {
                fmtStr = valueIsProbablyHex(node) ? " 0x%x" : " %d";
                output.appendf(fmtStr, node->getInt());
            }
            break;
        case TR::Int64:
            if (isUnsigned) {
                fmtStr = valueIsProbablyHex(node) ? " " UINT64_PRINTF_FORMAT_HEX : " " UINT64_PRINTF_FORMAT;
                output.appendf(fmtStr, node->getUnsignedLongInt());
            } else {
                fmtStr = valueIsProbablyHex(node) ? " " INT64_PRINTF_FORMAT_HEX : " " INT64_PRINTF_FORMAT;
                output.appendf(fmtStr, node->getLongInt());
            }
            break;
        case TR::Float: {
            output.appendf(" %g [0x%08x]", node->getFloat(), node->getFloatBits());
        } break;
        case TR::Double: {
            output.appendf(" %g [" UINT64_PRINTF_FORMAT_HEX "]", node->getDouble(), node->getDoubleBits());
        } break;
        case TR::Address:
            if (node->getAddress() == 0)
                output.appends(" NULL");
            else
                output.appendf(" " UINT64_PRINTF_FORMAT_HEX, node->getAddress());
            if (node->isClassPointerConstant()) {
                TR_OpaqueClassBlock *clazz = (TR_OpaqueClassBlock *)node->getAddress();
                int32_t len;
                const char *sig = TR::Compiler->cls.classNameChars(_comp, clazz, len);
                if (clazz) {
                    if (TR::Compiler->cls.isInterfaceClass(_comp, clazz))
                        output.appends(" Interface");
                    else if (TR::Compiler->cls.isAbstractClass(_comp, clazz))
                        output.appends(" Abstract");
                }
                output.appendf(" (%.*s.class)", len, sig);
            }
            break;

        default:
            output.appendf(" Bad Type %s", node->getDataType().toString());
            break;
    }
}

void TR_Debug::print(OMR::Logger *log, TR::CFG *cfg)
{
    int32_t numNodes = 0;
    int32_t index;
    TR::CFGNode *node;

    // If the depth-first numbering for the blocks has already been done, use this
    // numbering to order the blocks. Otherwise use a reasonable ordering.
    //
    for (node = cfg->getFirstNode(); node; node = node->getNext()) {
        index = node->getNumber();
        if (index < 0)
            numNodes++;
        else if (index >= numNodes)
            numNodes = index + 1;
    }

    void *stackMark = 0;
    TR::CFGNode **array;

    // From here, down, stack allocations will expire when the function returns
    TR::StackMemoryRegion stackMemoryRegion(*cfg->comp()->trMemory());

    array = (TR::CFGNode **)cfg->comp()->trMemory()->allocateStackMemory(numNodes * sizeof(TR::CFGNode *));

    memset(array, 0, numNodes * sizeof(TR::CFGNode *));
    index = numNodes;

    for (node = cfg->getFirstNode(); node; node = node->getNext()) {
        int32_t nodeNum = node->getNumber();
        array[nodeNum >= 0 ? nodeNum : --index] = node;
    }

    log->prints("\n<cfg>\n");

    for (index = 0; index < numNodes; index++)
        if (array[index] != NULL)
            print(log, array[index], 6);

    if (cfg->getStructure()) {
        log->prints("<structure>\n");
        print(log, cfg->getStructure(), 6);
        log->prints("</structure>");
    }

    log->prints("\n</cfg>\n");
}

void TR_Debug::print(OMR::Logger *log, TR_RegionAnalysis *regionAnalysis, uint32_t indentation)
{
    for (int32_t index = 0; index < regionAnalysis->_totalNumberOfNodes; index++) {
        TR_RegionAnalysis::StructInfo &node = regionAnalysis->getInfo(index);
        if (node._structure == NULL)
            continue;

        printBaseInfo(log, node._structure, indentation);

        // Dump successors
        log->printf("%*sout       = [", indentation + 11, " ");
        int num = 0;
        TR_RegionAnalysis::StructureBitVector::Cursor cursor(node._succ);
        for (cursor.SetToFirstOne(); cursor.Valid(); cursor.SetToNextOne()) {
            TR_RegionAnalysis::StructInfo &succ = regionAnalysis->getInfo(cursor);

            log->printf("%d ", succ.getNumber());

            if (num > 20) {
                log->println();
                num = 0;
            }
            num++;
        }
        log->prints("]\n");

        // Dump exception successors
        log->printf("%*sexceptions= [", indentation + 11, " ");
        num = 0;
        TR_RegionAnalysis::StructureBitVector::Cursor eCursor(node._exceptionSucc);
        for (eCursor.SetToFirstOne(); eCursor.Valid(); eCursor.SetToNextOne()) {
            TR_RegionAnalysis::StructInfo &succ = regionAnalysis->getInfo(eCursor);
            log->printf("%d ", succ.getNumber());

            if (num > 20) {
                log->println();
                num = 0;
            }
            num++;
        }
        log->prints("]\n");
    }
}

void TR_Debug::print(OMR::Logger *log, TR_Structure *structure, uint32_t indentation)
{
    if (structure->asBlock())
        print(log, structure->asBlock(), indentation);
    else
        print(log, structure->asRegion(), indentation);
}

void TR_Debug::print(OMR::Logger *log, TR_RegionStructure *regionStructure, uint32_t indentation)
{
    TR_RegionStructure *versionedLoop = NULL;
    if (debug("fullStructure"))
        printBaseInfo(log, regionStructure, indentation);
    else {
        // Do indentation and dump the address of this block

        const char *type;
        if (regionStructure->containsInternalCycles())
            type = "Improper region";
        else if (regionStructure->isNaturalLoop()) {
            versionedLoop = regionStructure->getVersionedLoop();
            if (versionedLoop) {
                TR::Block *entryBlock = regionStructure->getEntryBlock();
                if (entryBlock->isCold())
                    type = "Natural loop is the slow version of the fast versioned Natural loop ";
                else
                    type = "Natural loop is the fast version of the slow Natural loop ";
            } else
                type = "Natural loop";
        } else
            type = "Acyclic region";

        if (versionedLoop)
            log->printf("%*s%d [%s] %s %d\n", indentation, " ", regionStructure->getNumber(), getName(regionStructure),
                type, versionedLoop->getNumber());
        else
            log->printf("%*s%d [%s] %s\n", indentation, " ", regionStructure->getNumber(), getName(regionStructure),
                type);
    }

    // Dump induction variables
    //
    for (TR_InductionVariable *v = regionStructure->getFirstInductionVariable(); v; v = v->getNext()) {
        print(log, v, indentation + 3);
    }

    // Dump members
    printSubGraph(log, regionStructure, indentation + 3);
}

void TR_Debug::printPreds(OMR::Logger *log, TR::CFGNode *node)
{
    log->prints("in={");
    int num = 0;
    for (auto edge = node->getPredecessors().begin(); edge != node->getPredecessors().end(); ++edge) {
        log->printf("%d ", (*edge)->getFrom()->getNumber());

        if (num > 20) {
            log->println();
            num = 0;
        }
        num++;
    }

    num = 0;
    log->prints("} exc-in={");
    for (auto edge = node->getExceptionPredecessors().begin(); edge != node->getExceptionPredecessors().end(); ++edge) {
        log->printf("%d ", (*edge)->getFrom()->getNumber());

        if (num > 20) {
            log->println();
            num = 0;
        }
        num++;
    }
    log->printc('}');
}

void TR_Debug::printSubGraph(OMR::Logger *log, TR_RegionStructure *regionStructure, uint32_t indentation)
{
    int offset = 3;
    int num = 0;

    TR_StructureSubGraphNode *node, *next;

    log->printf("%*sSubgraph: (* = exit edge)\n", indentation, " ");

    TR_RegionStructure::Cursor si(*regionStructure);
    for (node = si.getCurrent(); node != NULL; node = si.getNext()) {
        // Dump successors
        if (node->getNumber() != node->getStructure()->getNumber()) {
            // This is an error situation, but print it to aid in debugging
            //
            log->printf("%*s%d(%d) -->", indentation + offset * 2, " ", node->getNumber(),
                node->getStructure()->getNumber());
        } else {
            log->printf("%*s(%s:%s)%d -->", indentation + offset * 2, " ", getName(node), getName(node->getStructure()),
                node->getNumber());
        }

        num = 0;
        for (auto edge = node->getSuccessors().begin(); edge != node->getSuccessors().end(); ++edge) {
            next = toStructureSubGraphNode((*edge)->getTo());
            log->printf(" %d(%s)", next->getNumber(), getName(next));
            if (regionStructure->isExitEdge(*edge))
                log->printc('*');

            if (num > 10) {
                log->println();
                num = 0;
            }
            num++;
        }
        log->println();

        // Dump exception successors
        if (!node->getExceptionSuccessors().empty()) {
            log->printf("%*s(%s:%s)%d >>>", indentation + offset * 2, " ", getName(node), getName(node->getStructure()),
                node->getNumber());
            num = 0;
            for (auto edge = node->getExceptionSuccessors().begin(); edge != node->getExceptionSuccessors().end();
                 ++edge) {
                next = toStructureSubGraphNode((*edge)->getTo());
                log->printf(" %d(%s)", next->getNumber(), getName(next));
                if (regionStructure->isExitEdge(*edge))
                    log->printc('*');

                if (num > 10) {
                    log->println();
                    num = 0;
                }
                num++;
            }
            log->println();
        }
        if (node->getStructure()->getParent() != regionStructure)
            log->printf("******* Structure %d does not refer back to its parent structure\n",
                node->getStructure()->getNumber());
    }

    ListElement<TR::CFGEdge> *firstExitEdge = regionStructure->getExitEdges().getListHead();

    if (firstExitEdge)
        log->printf("%*s%s", indentation + offset, " ", "Exit edges:\n");

    num = 0;
    ListElement<TR::CFGEdge> *exitEdge;
    for (exitEdge = firstExitEdge; exitEdge != NULL; exitEdge = exitEdge->getNextElement()) {
        log->printf("%*s(%s)%d -->%d\n", indentation + offset * 2, " ", getName(exitEdge->getData()->getFrom()),
            exitEdge->getData()->getFrom()->getNumber(), exitEdge->getData()->getTo()->getNumber());

        if (num > 10) {
            log->println();
            num = 0;
        }
        num++;
    }

    static char *verbose = ::feGetEnv("TR_VerboseStructures");
    if (verbose) {
        log->printf("%*sPred list:\n", indentation, " ");
        si.reset();
        for (node = si.getCurrent(); node != NULL; node = si.getNext()) {
            log->printf("%*s%d:", indentation + offset * 2, " ", node->getNumber());
            printPreds(log, node);
            log->println();
        }
        for (exitEdge = firstExitEdge; exitEdge != NULL; exitEdge = exitEdge->getNextElement()) {
            log->printf("%*s*%d:", indentation + offset * 2, " ", exitEdge->getData()->getTo()->getNumber());
            printPreds(log, exitEdge->getData()->getTo());
            log->println();
        }
    }

    si.reset();
    for (node = si.getCurrent(); node != NULL; node = si.getNext()) {
        print(log, node->getStructure(), indentation);
    }
}

void TR_Debug::print(OMR::Logger *log, TR_InductionVariable *inductionVariable, uint32_t indentation)
{
    int offset = 3;

    log->printf("%*sInduction variable [%s]\n", indentation, " ", getName(inductionVariable->getLocal()));
    log->printf("%*sEntry value: ", indentation + offset, " ");
    print(log, inductionVariable->getEntry());
    log->printf("\n%*sExit value:  ", indentation + offset, " ");
    print(log, inductionVariable->getExit());
    log->printf("\n%*sIncrement:   ", indentation + offset, " ");
    print(log, inductionVariable->getIncr());
    log->println();
}

static const char *structNames[] = { "Blank", "Block", "Region" };

void TR_Debug::printBaseInfo(OMR::Logger *log, TR_Structure *structure, uint32_t indentation)
{
    // Do indentation and dump the address of this block
    log->printf("%*s%d [%s] %s", indentation, " ", structure->getNumber(), getName(structure),
        structNames[structure->getKind()]);

    log->println();
}

void TR_Debug::print(OMR::Logger *log, TR_BlockStructure *blockStructure, uint32_t indentation)
{
    printBaseInfo(log, blockStructure, indentation);
    if (blockStructure->getBlock()->getStructureOf() != blockStructure)
        log->printf("******* Block %d does not refer back to block structure\n",
            blockStructure->getBlock()->getNumber());
}

void TR_Debug::print(OMR::Logger *log, TR::CFGNode *cfgNode, uint32_t indentation)
{
    if (cfgNode->asBlock())
        print(log, toBlock(cfgNode), indentation);
    else
        print(log, toStructureSubGraphNode(cfgNode), indentation);
}

void TR_Debug::print(OMR::Logger *log, TR_StructureSubGraphNode *node, uint32_t indentation)
{
    print(log, node->getStructure(), indentation);
}

void TR_Debug::printNodesInEdgeListIterator(OMR::Logger *log, TR::CFGEdgeList &li, bool fromNode)
{
    TR::Block *b;
    int num = 0;
    for (auto edge = li.begin(); edge != li.end(); ++edge) {
        b = fromNode ? toBlock((*edge)->getFrom()) : toBlock((*edge)->getTo());
        if ((*edge)->getFrequency() >= 0)
            log->printf("%d(%d) ", b->getNumber(), (*edge)->getFrequency());
        else
            log->printf("%d ", b->getNumber());

        if (num > 20) {
            log->println();
            num = 0;
        }
        num++;
    }
}

void TR_Debug::print(OMR::Logger *log, TR::Block *block, uint32_t indentation)
{
    // Do indentation and dump the address of this block
    log->printf("%*s", indentation, " ");
    if (block->getNumber() >= 0)
        log->printf("%4d ", block->getNumber());
    log->printf("[%s] ", getName(block));

    // If there are no nodes associated with this block, it must be the entry or
    // exit node.
    if (!block->getEntry()) {
        TR_ASSERT(!block->getExit(), "both entry and exit must be specified for TR_Block");
        if (block->getPredecessors().empty())
            log->prints("entry\n");
        else
            log->prints("exit\n");
    } else {
        // Dump information about regular block
        log->printf("BBStart at %s", getName(block->getEntry()->getNode()));
        if (block->getFrequency() >= 0)
            log->printf(", frequency = %d", block->getFrequency());

        static const bool enableTracePartialInlining = feGetEnv("TR_EnableTracePartialInlining") != NULL;
        if (enableTracePartialInlining) {
            log->prints(", partialFlags = ");
            if (block->isUnsanitizeable())
                log->prints("U, ");
            if (block->containsCall())
                log->prints("C, ");
            if (block->isRestartBlock())
                log->prints("R, ");
            if (block->isPartialInlineBlock())
                log->prints("P, ");
        }

        log->println();
    }

    bool fromNode;

    // Dump predecessors
    log->printf("%*sin        = [", indentation + 11, " ");
    printNodesInEdgeListIterator(log, block->getPredecessors(), fromNode = true);
    log->prints("]\n");

    // Dump successors
    log->printf("%*sout       = [", indentation + 11, " ");
    printNodesInEdgeListIterator(log, block->getSuccessors(), fromNode = false);
    log->prints("]\n");

    // Dump exception predecessors
    log->printf("%*sexception in  = [", indentation + 11, " ");
    printNodesInEdgeListIterator(log, block->getExceptionPredecessors(), fromNode = true);
    log->prints("]\n");

    // Dump exception successors
    log->printf("%*sexception out = [", indentation + 11, " ");
    printNodesInEdgeListIterator(log, block->getExceptionSuccessors(), fromNode = false);
    log->prints("]\n");
}

static void printInlinePath(OMR::Logger *log, TR_InlinedCallSite *site, TR_Debug *debug)
{
    int32_t callerIndex = site->_byteCodeInfo.getCallerIndex();
    if (callerIndex == -1) {
        log->printf("%02d", site->_byteCodeInfo.getByteCodeIndex());
    } else {
        TR::Compilation *comp = debug->comp();
        printInlinePath(log, &comp->getInlinedCallSite(callerIndex), debug);
        log->printf(".%02d", site->_byteCodeInfo.getByteCodeIndex());
    }
}

void TR_Debug::printIRTrees(OMR::Logger *log, const char *title, TR::ResolvedMethodSymbol *methodSymbol)
{
    if (!methodSymbol)
        methodSymbol = _comp->getMethodSymbol();

    const char *hotnessString = _comp->getHotnessName(_comp->getMethodHotness());

    const char *sig = signature(methodSymbol);

    log->printf("<trees\n"
                "\ttitle=\"%s\"\n"
                "\tmethod=\"%s\"\n"
                "\thotness=\"%s\">\n",
        title, sig, hotnessString);

    // we dump the name again?
    log->printf("\n%s: for %s\n", title, sig);

    if (_comp->getMethodSymbol() == methodSymbol && _comp->getNumInlinedCallSites() > 0) {
        log->prints("\nCall Stack Info\n");
        log->prints("CalleeIndex CallerIndex ByteCodeIndex CalleeMethod\n");

        for (auto i = 0U; i < _comp->getNumInlinedCallSites(); ++i) {
            TR_InlinedCallSite &ics = _comp->getInlinedCallSite(i);
            TR_ResolvedMethod *meth = _comp->getInlinedResolvedMethod(i);
            log->printf("    %4d       %4d       %5d       ", i, ics._byteCodeInfo.getCallerIndex(),
                ics._byteCodeInfo.getByteCodeIndex());

            TR::KnownObjectTable *knot = _comp->getKnownObjectTable();
            TR::KnownObjectTable::Index targetMethodHandleIndex = TR::KnownObjectTable::UNKNOWN;
            if (knot && meth && meth->convertToMethod()->isArchetypeSpecimen() && meth->getMethodHandleLocation())
                targetMethodHandleIndex = knot->getExistingIndexAt(meth->getMethodHandleLocation());
            if (targetMethodHandleIndex != TR::KnownObjectTable::UNKNOWN)
                log->printf("obj%d.", targetMethodHandleIndex);

            log->printf("%s\n", fe()->sampleSignature(ics._methodInfo, 0, 0, _comp->trMemory()));

            if (debug("printInlinePath")) {
                log->prints("     = ");
                printInlinePath(log, &ics, this);
                log->println();
            }
        }
    }

    _nodeChecklist.empty();
    int32_t nodeCount = 0;

    printTopLegend(log);

    TR::TreeTop *tt = methodSymbol->getFirstTreeTop();
    for (; tt; tt = tt->getNextTreeTop()) {
        nodeCount += print(log, tt);
        TR::Node *ttNode = tt->getNode();
        if (_comp->getOption(TR_TraceLiveness) && methodSymbol->getAutoSymRefs()
            && ttNode->getOpCodeValue() == TR::BBStart && ttNode->getBlock()->getLiveLocals()) {
            log->printf("%*s// Live locals:", addressWidth + 48, "");
            TR_BitVector *liveLocals = ttNode->getBlock()->getLiveLocals();
            // ouch, this loop is painstaking
            for (int32_t i = _comp->getSymRefTab()->getIndexOfFirstSymRef(); i < _comp->getSymRefTab()->getNumSymRefs();
                 i++) {
                TR::SymbolReference *symRef = _comp->getSymRefTab()->getSymRef(i);
                if (symRef && symRef->getSymbol()->isAutoOrParm()
                    && liveLocals->isSet(symRef->getSymbol()->castToRegisterMappedSymbol()->getLiveLocalIndex()))
                    log->printf(" #%d", symRef->getReferenceNumber());
            }
            log->println();
        }
    }

    printBottomLegend(log);
    printSymRefTable(log);

    log->printf("\nNumber of nodes = %d, symRefCount = %d\n", nodeCount, _comp->getSymRefTab()->getNumSymRefs());

    log->prints("</trees>\n");
    log->flush();
}

void TR_Debug::printBlockOrders(OMR::Logger *log, const char *title, TR::ResolvedMethodSymbol *methodSymbol)
{
    TR::TreeTop *tt = methodSymbol->getFirstTreeTop();

    log->printf("%s block ordering:\n", title);
    unsigned numberOfColdBlocks = 0;
    while (tt != NULL) {
        TR::Node *node = tt->getNode();
        if (node && node->getOpCodeValue() == TR::BBStart) {
            TR::Block *block = node->getBlock();
            log->printf("block_%-4d\t[ " POINTER_PRINTF_FORMAT "]\tfrequency %4d", block->getNumber(), block,
                block->getFrequency());
            if (block->isSuperCold()) {
                numberOfColdBlocks++;
                log->prints("\t(super cold)\n");
            } else if (block->isCold())
                log->prints("\t(cold)\n");
            else
                log->println();

            TR::CFGEdgeList &successors = block->getSuccessors();
            for (auto succEdge = successors.begin(); succEdge != successors.end(); ++succEdge)
                log->printf("\t -> block_%-4d\tfrequency %4d\n", (*succEdge)->getTo()->getNumber(),
                    (*succEdge)->getFrequency());
        }
        tt = tt->getNextTreeTop();
    }
}

TR_PrettyPrinterString::TR_PrettyPrinterString(TR_Debug *debug)
{
    len = 0;
    buffer[0] = '\0';
    _comp = debug->comp();
    _debug = debug;
}

char *TR_Debug::formattedString(char *buf, uint32_t bufLen, const char *format, va_list args,
    TR_AllocationKind allocationKind)
{
    va_list args_copy;
    va_copy(args_copy, args);

#if defined(J9ZOS390)
    // vsnprintf returns -1 on zos when buffer is too small
    char s[VSNPRINTF_BUFFER_SIZE];
    int resultLen = vsnprintf(s, VSNPRINTF_BUFFER_SIZE, format, args_copy);
    if (resultLen < 0) {
        OMR::Logger *log = _comp->log();
        static char *disableReportFormattedStringError = feGetEnv("TR_DisableReportFormattedStringError");
        if (!disableReportFormattedStringError && log->isEnabled_DEPRECATED())
            log->printf("Failed to get length of string with format %s\n", format);
    }
#else
    int resultLen = vsnprintf(NULL, 0, format, args_copy);
#endif

    va_copy_end(args_copy);

    if (unsigned(resultLen + 1) > bufLen) {
        bufLen = resultLen + 1;
        buf = (char *)comp()->trMemory()->allocateMemory(bufLen, allocationKind);
    }

    vsnprintf(buf, bufLen, format, args);

    return buf;
}

void TR_PrettyPrinterString::appendf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    len += vsnprintf(buffer + len, maxBufferLength - len, format, args);
    va_end(args);
}

void TR_PrettyPrinterString::appends(char const *str)
{
    size_t const strLen0 = strlen(str) + 1; // include terminating '\0'
    size_t const bufLenBytes = maxBufferLength - len;

    char *buf = buffer + len;

    if (strLen0 < bufLenBytes) {
        memcpy(buf, str, strLen0);
        len += static_cast<int32_t>(strLen0 - 1);
    } else if (bufLenBytes != 0) {
        memcpy(buf, str, bufLenBytes - 1);
        buf[bufLenBytes - 1] = '\0';
        len += static_cast<int32_t>(bufLenBytes - 1);
    }
}

int32_t TR_Debug::print(OMR::Logger *log, TR::TreeTop *tt)
{
    return print(log, tt->getNode(), DEFAULT_TREETOP_INDENT, true);
}

// Dump this node and its children with a standard prefix.
// Return the number of nodes encountered.
int32_t TR_Debug::print(OMR::Logger *log, TR::Node *node, uint32_t indentation, bool printChildren)
{
    TR_ASSERT(node != NULL, "node is NULL\n");

    // If this node has already been printed, just print a reference to it.
    //
    if (_nodeChecklist.isSet(node->getGlobalIndex())) {
        printBasicPreNodeInfoAndIndent(log, node, indentation);
        log->printf("==>%s", getName(node->getOpCode()));
        if (node->getOpCode().isLoadConst())
            printLoadConst(log, node);
#ifdef J9_PROJECT_SPECIFIC
        printBCDNodeInfo(log, node);
#endif
        log->println();
        log->flush();
        _comp->setNodeOpCodeLength(0); // probably redundant - just to be safe
        return 0;
    }

    _nodeChecklist.set(node->getGlobalIndex());
    printBasicPreNodeInfoAndIndent(log, node, indentation);

    int32_t nodeCount = 1; // Count this node
    int32_t i;

    printNodeInfo(log, node);
    if (!debug("disableDumpNodeFlags"))
        printNodeFlags(log, node);
    printBasicPostNodeInfo(log, node, indentation);

    log->println();

    TR_PrettyPrinterString output(this);

    if (printChildren) {
        indentation += DEFAULT_INDENT_INCREMENT;
        if (node->getOpCode().isSwitch()) {
            nodeCount += print(log, node->getFirstChild(), indentation, true);

            printBasicPreNodeInfoAndIndent(log, node->getSecondChild(), indentation);
            nodeCount++;

            output.appends("default ");
            _comp->incrNodeOpCodeLength(output.getLength());
            log->prints(output.getStr());
            output.reset();

            printDestination(log, node->getSecondChild()->getBranchDestination());
            printBasicPostNodeInfo(log, node->getSecondChild(), indentation);
            log->println();
            TR::Node *oldParent = getCurrentParent();
            int32_t oldChildIndex = getCurrentChildIndex();
            if (node->getSecondChild()->getNumChildren() == 1) // a GlRegDep
            {
                setCurrentParentAndChildIndex(node->getSecondChild(), 1);
                nodeCount += print(log, node->getSecondChild()->getFirstChild(), indentation + DEFAULT_INDENT_INCREMENT,
                    true);
            }
            setCurrentParentAndChildIndex(oldParent, oldChildIndex);

            uint16_t upperBound = node->getCaseIndexUpperBound();

            if (node->getOpCodeValue() == TR::lookup) {
                bool unsigned_case = node->getFirstChild()->getOpCode().isUnsigned();

                TR::Node *oldParent = getCurrentParent();
                int32_t oldChildIndex = getCurrentChildIndex();
                for (i = 2; i < upperBound; i++) {
                    printBasicPreNodeInfoAndIndent(log, node->getChild(i), indentation);
                    nodeCount++;

                    char const *fmtStr;
                    if (sizeof(CASECONST_TYPE) == 8) {
                        fmtStr = unsigned_case ? INT64_PRINTF_FORMAT_HEX ":\t" : INT64_PRINTF_FORMAT ":\t";
                    } else {
                        fmtStr = unsigned_case ? "%u:\t" : "%d:\t";
                    }
                    output.appendf(fmtStr, node->getChild(i)->getCaseConstant());

                    _comp->incrNodeOpCodeLength(output.getLength());
                    log->prints(output.getStr());
                    output.reset();

                    printDestination(log, node->getChild(i)->getBranchDestination());

                    printBasicPostNodeInfo(log, node->getChild(i), indentation);
                    log->println();
                    if (node->getChild(i)->getNumChildren() == 1) // a GlRegDep
                    {
                        setCurrentParentAndChildIndex(node->getChild(i), i);
                        nodeCount += print(log, node->getChild(i)->getFirstChild(),
                            indentation + DEFAULT_INDENT_INCREMENT, true);
                    }
                }
                setCurrentParentAndChildIndex(oldParent, oldChildIndex);
            } else {
                TR::Node *oldParent = getCurrentParent();
                int32_t oldChildIndex = getCurrentChildIndex();
                for (i = 2; i < upperBound; i++) {
                    printBasicPreNodeInfoAndIndent(log, node->getChild(i), indentation);
                    nodeCount++;
                    output.appendf("%d", i - 2);
                    _comp->incrNodeOpCodeLength(output.getLength());
                    log->prints(output.getStr());
                    output.reset();

                    printDestination(log, node->getChild(i)->getBranchDestination());
                    printBasicPostNodeInfo(log, node->getChild(i), indentation);
                    log->println();
                    if (node->getChild(i)->getNumChildren() == 1) // a GlRegDep
                    {
                        setCurrentParentAndChildIndex(node->getChild(i), i);
                        nodeCount += print(log, node->getChild(i)->getFirstChild(),
                            indentation + DEFAULT_INDENT_INCREMENT, true);
                    }
                }
                setCurrentParentAndChildIndex(oldParent, oldChildIndex);
                // Check for branch table address
                if (upperBound < node->getNumChildren()
                    && node->getChild(upperBound)->getOpCodeValue() != TR::GlRegDeps) {
                    nodeCount += print(log, node->getChild(upperBound), indentation, true);
                }
            }
        } else {
            TR::Node *oldParent = getCurrentParent();
            int32_t oldChildIndex = getCurrentChildIndex();
            for (i = 0; i < node->getNumChildren(); i++) {
                setCurrentParentAndChildIndex(node, i);
                nodeCount += print(log, node->getChild(i), indentation, true);
            }
            setCurrentParentAndChildIndex(oldParent, oldChildIndex);
        }
    }

    log->flush();
    return nodeCount;
}

uint32_t TR_Debug::getIntLength(uint32_t num) const
{
    // The fastest means is a binary search over the range of possible
    // uint32_t values.  The maximum number of digits for a 32-bit unsigned
    // integer is 10.
    //
    if (num < 100000) {
        if (num < 100) {
            if (num < 10)
                return 1; // 0-9
            else
                return 2; // 10-99
        } else {
            if (num < 10000) {
                if (num < 1000)
                    return 3; // 100-999
                else
                    return 4; // 1000-9999
            } else
                return 5; // 10000-99999
        }
    } else {
        if (num < 10000000) {
            if (num < 1000000)
                return 6; // 100_000-999_999
            else
                return 7; // 1_000_000-9_999_999
        } else {
            if (num < 100000000)
                return 8; // 10_000_000-99_999_999
            else {
                if (num < 1000000000)
                    return 9; // 100_000_000-999_999_999
                else
                    return 10; // 1_000_000_000-UINT_MAX
            }
        }
    }
}

uint32_t TR_Debug::getNumSpacesAfterIndex(uint32_t index, uint32_t maxIndexLength) const
{
    uint32_t indexLength = getIntLength(index);

    return maxIndexLength > indexLength ? (maxIndexLength - indexLength) : 0;
}

// Dump this node and its children with a fixed prefix.
// Return the number of nodes encountered.
//
int32_t TR_Debug::printWithFixedPrefix(OMR::Logger *log, TR::Node *node, uint32_t indentation, bool printChildren,
    bool printRefCounts, const char *prefix)
{
    uint32_t numSpaces, globalIndex;

    TR_PrettyPrinterString output(this);

    _comp->setNodeOpCodeLength(0);

    char const * const globalIndexPrefix = "n";

    globalIndex = node->getGlobalIndex();
    numSpaces = getNumSpacesAfterIndex(globalIndex, MAX_GLOBAL_INDEX_LENGTH);

    // If this node has already been printed, just print a reference to it.
    //
    if (_nodeChecklist.isSet(node->getGlobalIndex())) {
        if (printRefCounts) {
            log->printf("%s%s%dn%*s  (%3d) %*s==>%s", prefix, globalIndexPrefix, globalIndex, numSpaces, "",
                node->getReferenceCount(), indentation, " ", getName(node->getOpCode()));
        } else {
            log->printf("%s%s%dn%*s  %*s==>%s", prefix, globalIndexPrefix, globalIndex, numSpaces, "", indentation, " ",
                getName(node->getOpCode()));
        }

        if (node->getOpCode().isLoadConst())
            printLoadConst(log, node);
#ifdef J9_PROJECT_SPECIFIC
        printBCDNodeInfo(log, node);
#endif

        if (_comp->cg()->getAppendInstruction() != NULL && node->getDataType() != TR::NoType
            && node->getRegister() != NULL) {
            output.appendf(" (in %s)", getName(node->getRegister()));
            _comp->incrNodeOpCodeLength(output.getLength());
            log->prints(output.getStr());
            output.reset();
        }
        if (!debug("disableDumpNodeFlags"))
            printNodeFlags(log, node);

        log->flush();
        return 0;
    }

    _nodeChecklist.set(node->getGlobalIndex());

    if (printRefCounts)
        log->printf("%s%s%dn%*s  (%3d) %*s", prefix, globalIndexPrefix, globalIndex, numSpaces, "",
            node->getReferenceCount(), indentation, " ");
    else
        log->printf("%s%s%dn%*s  %*s", prefix, globalIndexPrefix, globalIndex, numSpaces, "", indentation, " ");

    int32_t nodeCount = 1; // Count this node
    int32_t i;

    printNodeInfo(log, node);
    if (_comp->cg()->getAppendInstruction() != NULL && node->getDataType() != TR::NoType
        && node->getRegister() != NULL) {
        output.appendf(" (in %s)", getName(node->getRegister()));
        _comp->incrNodeOpCodeLength(output.getLength());
        log->prints(output.getStr());
        output.reset();
    }
    if (!debug("disableDumpNodeFlags"))
        printNodeFlags(log, node);
    printBasicPostNodeInfo(log, node, indentation);

    if (printChildren) {
        indentation += DEFAULT_INDENT_INCREMENT;
        if (node->getOpCode().isSwitch()) {
            log->println();
            nodeCount += printWithFixedPrefix(log, node->getFirstChild(), indentation, true, printRefCounts, prefix);

            globalIndex = node->getSecondChild()->getGlobalIndex();
            numSpaces = getNumSpacesAfterIndex(globalIndex, MAX_GLOBAL_INDEX_LENGTH);

            log->printf("\n%s%s%dn%*s  %*s", prefix, globalIndexPrefix, globalIndex, numSpaces, "", indentation, " ");
            nodeCount++;
            output.appends("default ");
            _comp->incrNodeOpCodeLength(output.getLength());
            log->prints(output.getStr());
            output.reset();

            printDestination(log, node->getSecondChild()->getBranchDestination());
            printBasicPostNodeInfo(log, node->getSecondChild(), indentation);
            if (node->getSecondChild()->getNumChildren() == 1) // a GlRegDep
                nodeCount += printWithFixedPrefix(log, node->getSecondChild()->getFirstChild(),
                    indentation + DEFAULT_INDENT_INCREMENT, true, printRefCounts, prefix);

            uint16_t upperBound = node->getCaseIndexUpperBound();

            if (node->getOpCodeValue() == TR::lookup) {
                for (i = 2; i < upperBound; i++) {
                    globalIndex = node->getChild(i)->getGlobalIndex();
                    numSpaces = getNumSpacesAfterIndex(globalIndex, MAX_GLOBAL_INDEX_LENGTH);

                    log->printf("\n%s%s%dn%*s  %*s", prefix, globalIndexPrefix, globalIndex, numSpaces, "", indentation,
                        " ");
                    nodeCount++;

                    char const *fmtStr;
                    if (sizeof(CASECONST_TYPE) == 8) {
                        fmtStr = (node->getFirstChild()->getOpCode().isUnsigned()) ? INT64_PRINTF_FORMAT_HEX ":\t"
                                                                                   : INT64_PRINTF_FORMAT ":\t";
                    } else {
                        fmtStr = (node->getFirstChild()->getOpCode().isUnsigned()) ? "%u:\t" : "%d:\t";
                    }
                    output.appendf(fmtStr, node->getChild(i)->getCaseConstant());
                    _comp->incrNodeOpCodeLength(output.getLength());
                    log->prints(output.getStr());
                    output.reset();

                    printDestination(log, node->getChild(i)->getBranchDestination());

                    printBasicPostNodeInfo(log, node->getChild(i), indentation);

                    if (node->getChild(i)->getNumChildren() == 1) // a GlRegDep
                        nodeCount += printWithFixedPrefix(log, node->getChild(i)->getFirstChild(),
                            indentation + DEFAULT_INDENT_INCREMENT, true, printRefCounts, prefix);
                }
            } else {
                for (i = 2; i < upperBound; i++) {
                    globalIndex = node->getChild(i)->getGlobalIndex();
                    numSpaces = getNumSpacesAfterIndex(globalIndex, MAX_GLOBAL_INDEX_LENGTH);

                    log->printf("\n%s%s%dn%*s  %*s", prefix, globalIndexPrefix, globalIndex, numSpaces, "", indentation,
                        " ");
                    nodeCount++;
                    output.appendf("%d", i - 2);
                    _comp->incrNodeOpCodeLength(output.getLength());
                    log->prints(output.getStr());
                    output.reset();

                    printDestination(log, node->getChild(i)->getBranchDestination());
                    if (node->getChild(i)->getNumChildren() == 1) // a GlRegDep
                        nodeCount += printWithFixedPrefix(log, node->getChild(i)->getFirstChild(),
                            indentation + DEFAULT_INDENT_INCREMENT, true, printRefCounts, prefix);
                }
                // Check for branch table address
                if (upperBound < node->getNumChildren()
                    && node->getChild(upperBound)->getOpCodeValue() != TR::GlRegDeps) {
                    nodeCount += printWithFixedPrefix(log, node->getChild(upperBound), indentation, true,
                        printRefCounts, prefix);
                }
            }
        } else {
            TR::Node *oldParent = getCurrentParent();
            int32_t oldChildIndex = getCurrentChildIndex();
            setCurrentParent(node);
            for (i = 0; i < node->getNumChildren(); i++) {
                log->println();
                setCurrentChildIndex(i);
                nodeCount += printWithFixedPrefix(log, node->getChild(i), indentation, true, printRefCounts, prefix);
            }
            setCurrentParentAndChildIndex(oldParent, oldChildIndex);
        }
    }

    log->flush();
    return nodeCount;
}

void TR_Debug::printDestination(OMR::Logger *log, TR::TreeTop *treeTop)
{
    TR_PrettyPrinterString output(this);
    printDestination(treeTop, output);
    log->prints(output.getStr());
    _comp->incrNodeOpCodeLength(output.getLength());
}

void TR_Debug::printDestination(TR::TreeTop *treeTop, TR_PrettyPrinterString &output)
{
    if (treeTop == NULL)
        return;

    TR::Node *node = treeTop->getNode();
    TR::Block *block = node->getBlock();
    output.appends(" --> ");
    if (block->getNumber() >= 0)
        output.appendf("block_%d", block->getNumber());
    output.appendf(" BBStart at n%dn", node->getGlobalIndex());
}

void TR_Debug::printBasicPreNodeInfoAndIndent(OMR::Logger *log, TR::Node *node, uint32_t indentation)
{
    uint32_t numSpaces;

    if (node->getOpCodeValue() == TR::BBStart && !node->getBlock()->isExtensionOfPreviousBlock()
        && node->getBlock()
               ->getPrevBlock()) // a block that is not an extension of previous block and is not the first block
    {
        log->println();
    }

    char const * const globalIndexPrefix = "n";

    numSpaces = getNumSpacesAfterIndex(node->getGlobalIndex(), MAX_GLOBAL_INDEX_LENGTH);
    log->printf("%s%dn%*s %*s", globalIndexPrefix, node->getGlobalIndex(), numSpaces, "", indentation, " ");

    _comp->setNodeOpCodeLength(0);
}

void TR_Debug::printBasicPostNodeInfo(OMR::Logger *log, TR::Node *node, uint32_t indentation)
{
    TR_PrettyPrinterString output(this);

    output.appends("  ");
    if ((_comp->getNodeOpCodeLength() + indentation) < DEFAULT_NODE_LENGTH + DEFAULT_INDENT_INCREMENT)
        output.appendf("%*s",
            DEFAULT_NODE_LENGTH + DEFAULT_INDENT_INCREMENT - (_comp->getNodeOpCodeLength() + indentation), "");

    int32_t lineNumber = _comp->getLineNumber(node);

    output.appendf("[%s] ", getName(node));

    char const *bciOrLoc = "bci";

    if (lineNumber < 0) {
        output.appendf("%s=[%d,%d,-] rc=", bciOrLoc, node->getByteCodeInfo().getCallerIndex(),
            node->getByteCodeInfo().getByteCodeIndex());
    } else {
        output.appendf("%s=[%d,%d,%d] rc=", bciOrLoc, node->getByteCodeInfo().getCallerIndex(),
            node->getByteCodeInfo().getByteCodeIndex(), lineNumber);
    }

    output.appendf("%d vc=%d", node->getReferenceCount(), node->getVisitCount());

    if (_comp->getOptimizer() && _comp->getOptimizer()->getValueNumberInfo())
        output.appendf(" vn=%d", _comp->getOptimizer()->getValueNumberInfo()->getValueNumber(node));
    else
        output.appends(" vn=-");

    if (node->getLocalIndex())
        output.appendf(" li=%d", node->getLocalIndex());
    else
        output.appends(" li=-");

    if (node->getUseDefIndex())
        output.appendf(" udi=%d", node->getUseDefIndex());
    else
        output.appends(" udi=-");

    output.appendf(" nc=%d", node->getNumChildren());

    if (node->getFlags().getValue())
        output.appendf(" flg=0x%x", node->getFlags().getValue());

    log->prints(output.getStr());

    _comp->setNodeOpCodeLength(0); // redundant - just to be safe
}

void TR_Debug::printNodeInfo(OMR::Logger *log, TR::Node *node)
{
    TR_PrettyPrinterString output(this);
    printNodeInfo(node, output, false);
    log->prints(output.getStr());
    _comp->incrNodeOpCodeLength(output.getLength());
}

// Dump the node information for this node
void TR_Debug::printNodeInfo(TR::Node *node, TR_PrettyPrinterString &output, bool prettyPrint)
{
    char const * const globalIndexPrefix = "n";

    if (!prettyPrint || (node->getOpCodeValue() != TR::BBStart && node->getOpCodeValue() != TR::BBEnd)) {
        output.appendf("%s", getName(node->getOpCode()));
    }

    // print vector type immediately after opcode name
    if (node->getOpCode().isVectorOpCode()) {
        TR::ILOpCode opcode = node->getOpCode();

        if (opcode.isTwoTypeVectorOpCode()) {
            // example: vconvVector128Int32_Vector128Float
            output.appendf("%s_%s", getName(opcode.getVectorSourceDataType()),
                getName(opcode.getVectorResultDataType()));
        } else {
            // example: vaddVector128Int32
            output.appendf("%s", getName(opcode.getVectorResultDataType()));
        }
    }

#ifdef TR_ALLOW_NON_CONST_KNOWN_OBJECTS
    if (node->hasKnownObjectIndex()) {
        output.appendf(" (node obj%d)", node->getKnownObjectIndex());
    }
#endif

    if (node->getOpCode().isNullCheck()) {
        if (node->getNullCheckReference())
            output.appendf(" on %s%dn", globalIndexPrefix, node->getNullCheckReference()->getGlobalIndex());
        else
            output.appends(" on null NullCheckReference ----- INVALID tree!!");
    } else if (node->getOpCodeValue() == TR::allocationFence) {
        if (node->getAllocation())
            output.appendf(" on %s%dn", globalIndexPrefix, node->getAllocation()->getGlobalIndex());
        else
            output.appends(" on ALL");
    }

    if (node->getOpCode().hasSymbolReference() && node->getSymbolReference()) {
        bool hideHelperMethodInfo = false;
        if (node->getOpCode().isCheck()) // Omit helper method info for OpCodes that are checked
            hideHelperMethodInfo = true;

        print(node->getSymbolReference(), output, hideHelperMethodInfo);
    } else if (node->getOpCode().isBranch()) {
        printDestination(node->getBranchDestination(), output);
    } else if (node->getOpCodeValue() == TR::exceptionRangeFence) {
        if (node->getNumRelocations() > 0) {
            if (node->getRelocationType() == TR_AbsoluteAddress)
                output.appends(" Absolute [");
            else if (node->getRelocationType() == TR_ExternalAbsoluteAddress)
                output.appends(" External Absolute [");
            else
                output.appends(" Relative [");

            for (auto i = 0U; i < node->getNumRelocations(); ++i)
                output.appendf(" " POINTER_PRINTF_FORMAT, node->getRelocationDestination(i));

            output.appends(" ]");
        }
    } else if (node->getOpCodeValue() == TR::BBStart) {
        TR::Block *block = node->getBlock();
        if (block->getNumber() >= 0) {
            output.appendf(" <block_%d>", block->getNumber());
        }

        if (block->getFrequency() >= 0)
            output.appendf(" (freq %d)", block->getFrequency());
        if (block->isExtensionOfPreviousBlock())
            output.appends(" (extension of previous block)");
        if (block->isCatchBlock()) {
            int32_t length;
            const char *classNameChars = block->getExceptionClassNameChars();
            if (classNameChars) {
                length = block->getExceptionClassNameLength();
                output.appendf(" (catches %.*s)", length, getName(classNameChars, length));
            } else {
                classNameChars = "...";
                length = 3;
                output.appendf(" (catches %.*s)", length,
                    classNameChars); // passing literal string shouldn't use getName()
            }

            if (block->isOSRCatchBlock()) {
                output.appends(" (OSR handler)");
            }
        }
        if (block->isSuperCold())
            output.appends(" (super cold)");
        else if (block->isCold())
            output.appends(" (cold)");

        if (block->isLoopInvariantBlock())
            output.appends(" (loop pre-header)");
        TR_BlockStructure *blockStructure = block->getStructureOf();
        if (blockStructure) {
            if (_comp->getFlowGraph()->getStructure()) {
                TR_Structure *parent = blockStructure->getParent();
                while (parent) {
                    TR_RegionStructure *region = parent->asRegion();
                    if (region->isNaturalLoop() || region->containsInternalCycles()) {
                        output.appendf(" (in loop %d)", region->getNumber());
                        break;
                    }
                    parent = parent->getParent();
                }
                TR_BlockStructure *dupBlock = blockStructure->getDuplicatedBlock();
                if (dupBlock)
                    output.appendf(" (dup of block_%d)", dupBlock->getNumber());
            }
        }
    } else if (node->getOpCodeValue() == TR::BBEnd) {
        TR::Block *block = node->getBlock(), *nextBlock;
        if (block->getNumber() >= 0) {
            output.appendf(" </block_%d>", block->getNumber());
            if (block->isSuperCold())
                output.appends(" (super cold)");
            else if (block->isCold())
                output.appends(" (cold)");
        }

        if ((nextBlock = block->getNextBlock())
            && !nextBlock->isExtensionOfPreviousBlock()) // end of a block that is not the last block and the next block
                                                         // is not an extension of it
            output.appends(" =====");
    } else if (node->getOpCode().isArrayLength()) {
        int32_t stride = node->getArrayStride();

        if (stride > 0)
            output.appendf(" (stride %d)", stride);
    } else if (node->getOpCode().isLoadReg() || node->getOpCode().isStoreReg()) {
        if ((node->getType().isInt64() && _comp->target().is32Bit() && !_comp->cg()->use64BitRegsOn32Bit()))
            output.appendf(" %s:%s ", getGlobalRegisterName(node->getHighGlobalRegisterNumber()),
                getGlobalRegisterName(node->getLowGlobalRegisterNumber()));
        else
            output.appendf(" %s ", getGlobalRegisterName(node->getGlobalRegisterNumber()));

        if (node->getOpCode().isLoadReg())
            print(node->getRegLoadStoreSymbolReference(), output);
    } else if (node->getOpCodeValue() == TR::PassThrough) {
        // print only if under a GlRegDep
        bool isParentGlRegDep = getCurrentParent() ? (getCurrentParent()->getOpCodeValue() == TR::GlRegDeps) : false;
        if (isParentGlRegDep) {
            TR::DataType t = node->getDataType();
            // This is a half-hearted attempt at getting better register sizes, I know
            TR_RegisterSizes size;
            if (t == TR::Int8)
                size = TR_ByteReg;
            else if (t == TR::Int16)
                size = TR_HalfWordReg;
            else if (t == TR::Int32)
                size = TR_WordReg;
            else
                size = TR_DoubleWordReg;
            if ((node->getFirstChild()->getType().isInt64() && _comp->target().is32Bit()))
                output.appendf(" %s:%s ", getGlobalRegisterName(node->getHighGlobalRegisterNumber(), size),
                    getGlobalRegisterName(node->getLowGlobalRegisterNumber(), size));
            else
                output.appendf(" %s ", getGlobalRegisterName(node->getGlobalRegisterNumber(), size));
        }
    } else if (node->getOpCode().hasNoDataType()) {
        output.appendf(" (%s)", getName(node->getDataType()));
    }

    if (node->getOpCode().isLoadConst()) {
        printLoadConst(node, output);

        if (getCurrentParent() && getCurrentParent()->getOpCodeValue() == TR::newarray
            && getCurrentParent()->getSecondChild() == node) {
            output.appends("   ; array type is ");

            const char *typeStr;
            switch (node->getInt()) {
                case 4:
                    typeStr = "boolean";
                    break;
                case 5:
                    typeStr = "char";
                    break;
                case 6:
                    typeStr = "float";
                    break;
                case 7:
                    typeStr = "double";
                    break;
                case 8:
                    typeStr = "byte";
                    break;
                case 9:
                    typeStr = "short";
                    break;
                case 10:
                    typeStr = "int";
                    break;
                case 11:
                    typeStr = "long";
                    break;
                default:
                    TR_ASSERT_FATAL(0, "Unexpected array type");
                    typeStr = "unknown";
            }

            output.appends(typeStr);
        }
    }

#ifdef J9_PROJECT_SPECIFIC
    printBCDNodeInfo(node, output);
#endif
}

// Dump the node flags for this node
// All flag printers should be called from here
void TR_Debug::printNodeFlags(OMR::Logger *log, TR::Node *node)
{
    TR_PrettyPrinterString output(this);

    if (node->getFlags().getValue()) {
        output.appends(" (");
        nodePrintAllFlags(node, output);
        output.appends(")");
    }

    log->prints(output.getStr());

    _comp->incrNodeOpCodeLength(output.getLength());
}

#ifdef J9_PROJECT_SPECIFIC
void TR_Debug::printBCDNodeInfo(OMR::Logger *log, TR::Node *node)
{
    TR_PrettyPrinterString output(this);
    printBCDNodeInfo(node, output);
    log->prints(output.getStr());
    _comp->incrNodeOpCodeLength(output.getLength());
}

void TR_Debug::printBCDNodeInfo(TR::Node *node, TR_PrettyPrinterString &output)
{
    if (node->getType().isBCD()) {
        if (node->getOpCode().isStore() || node->getOpCode().isCall() || node->getOpCode().isLoadConst()
            || (node->getOpCode().isConversion() && !node->getOpCode().isConversionWithFraction())) {
            if (node->hasSourcePrecision()) {
                output.appendf(" <prec=%d (len=%d) srcprec=%d> ", node->getDecimalPrecision(), node->getSize(),
                    node->getSourcePrecision());
            } else {
                output.appendf(" <prec=%d (len=%d)> ", node->getDecimalPrecision(), node->getSize());
            }
        } else if (node->getOpCode().isLoad()) {
            output.appendf(" <prec=%d (len=%d) adj=%d> ", node->getDecimalPrecision(), node->getSize(),
                node->getDecimalAdjust());
        } else if (node->canHaveSourcePrecision()) {
            output.appendf(" <prec=%d (len=%d) srcprec=%d %s=%d round=%d> ", node->getDecimalPrecision(),
                node->getSize(), node->getSourcePrecision(),
                node->getOpCode().isConversionWithFraction() ? "frac" : "adj",
                node->getOpCode().isConversionWithFraction() ? node->getDecimalFraction() : node->getDecimalAdjust(),
                node->getDecimalRound());
        } else {
            output.appendf(" <prec=%d (len=%d) %s=%d round=%d> ", node->getDecimalPrecision(), node->getSize(),
                node->getOpCode().isConversionWithFraction() ? "frac" : "adj",
                node->getOpCode().isConversionWithFraction() ? node->getDecimalFraction() : node->getDecimalAdjust(),
                node->getDecimalRound());
        }
        if (!node->getOpCode().isStore()) {
            output.appends("sign=");
            if (node->hasKnownOrAssumedCleanSign() || node->hasKnownOrAssumedPreferredSign()
                || node->hasKnownOrAssumedSignCode()) {
                if (node->signStateIsKnown())
                    output.appends("known(");
                else
                    output.appends("assumed(");
                if (node->hasKnownOrAssumedCleanSign())
                    output.appends("clean");
                if (node->hasKnownOrAssumedPreferredSign())
                    output.appendf("%spreferred", node->hasKnownOrAssumedCleanSign() ? "/" : "");
                if (node->hasKnownOrAssumedSignCode())
                    output.appendf("%s%s",
                        node->hasKnownOrAssumedCleanSign() || node->hasKnownOrAssumedPreferredSign() ? "/" : "",
                        getName(node->hasKnownSignCode() ? node->getKnownSignCode() : node->getAssumedSignCode()));
                output.appends(") ");
            } else if (node->getOpCode().isLoad()) {
                output.appendf("%s ", node->hasSignStateOnLoad() ? "hasState" : "noState");
            } else {
                output.appends("? ");
            }
        }
        if (node->isSetSignValueOnNode()) {
            output.appendf("setSign=%s ", getName(node->getSetSign()));
        }
    } else if (node->getOpCode().isConversionWithFraction()) {
        output.appendf(" <frac=%d> ", node->getDecimalFraction());
    } else if ((node->getType()).isAggregate()) {
        output.appendf(" <size=%lld bytes>", (int64_t)0);
    }
    if (node->castedToBCD()) {
        output.appends(" <castedToBCD=true> ");
    }
}
#endif

// Prints out a specification of the control flow graph in VCG format.
//
void TR_Debug::printVCG(OMR::Logger *log, TR::CFG *cfg, const char *sig)
{
    _nodeChecklist.empty();
    _structureChecklist.empty();
    if (debug("nestedVCG")) {
        log->prints("graph: {\n");
        log->prints("title: \"Nested Flow Graph\"\n");
        log->prints("splines: yes\n");
        log->prints("portsharing: no\n");
        log->prints("manhatten_edges: no\n");
        log->prints("layoutalgorithm: dfs\n");
        log->prints("finetuning: no\n");
        log->prints("xspace: 60\n");
        log->prints("yspace: 50\n\n");
        log->prints("node.borderwidth: 2\n");
        log->prints("node.color: white\n");
        log->prints("node.textcolor: black\n");
        log->prints("edge.color: black\n");
        log->printf("node: {title: \"Top1\" label: \"%s\" vertical_order: 0 horizontal_order: 0 textcolor: blue "
                    "borderwidth: 1}\n",
            sig);

        if (cfg->getStructure())
            printVCG(log, cfg->getStructure());

        log->prints("\n}\n");
    } else if (debug("bseqVCG")) {
        log->prints("graph: {\n");
        log->prints("title: \"Nested Flow Graph\"\n");
        log->prints("splines: yes\n");
        log->prints("portsharing: yes\n");
        log->prints("manhatten_edges: no\n");
        log->prints("layoutalgorithm: dfs\n");
        log->prints("finetuning: no\n");
        log->prints("xspace: 60\n");
        log->prints("yspace: 50\n\n");
        log->prints("node.borderwidth: 2\n");
        log->prints("node.color: white\n");
        log->prints("node.textcolor: black\n");
        log->prints("edge.color: black\n");
        log->printf("node: {title: \"Top1\" label: \"%s\" vertical_order: 0 horizontal_order: 0 textcolor: blue "
                    "borderwidth: 1}\n",
            sig);

        int32_t order;
        TR::TreeTop *exitTree, *treeTop;
        for (order = 1, treeTop = _comp->getStartTree(); treeTop; treeTop = exitTree->getNextTreeTop(), ++order) {
            TR::Block *block = treeTop->getNode()->getBlock();
            exitTree = block->getExit();
            printVCG(log, block, order, 1);
        }
        printVCG(log, toBlock(cfg->getStart()), 0, 1);
        printVCG(log, toBlock(cfg->getEnd()), order, 1);

        log->prints("\n}\n");
    } else {
        log->prints("graph: {\n");
        log->prints("title: \"Linear Flow Graph\"\n");
        log->prints("splines: no\n");
        log->prints("portsharing: no\n");
        log->prints("manhatten_edges: no\n");
        log->prints("layoutalgorithm: dfs\n");
        log->prints("finetuning: no\n");
        log->prints("xspace: 60\n");
        log->prints("yspace: 50\n\n");
        log->prints("node.borderwidth: 2\n");
        log->prints("node.color: white\n");
        log->prints("node.textcolor: black\n");
        log->prints("edge.color: black\n");
        log->printf("node: {title: \"Top1\" label: \"%s\" vertical_order: 0 textcolor: blue borderwidth: 1}\n", sig);

        TR::CFGNode *node;
        for (node = cfg->getFirstNode(); node; node = node->getNext())
            printVCG(log, toBlock(node));

        log->prints("\n}\n");
    }
}

void TR_Debug::printVCG(OMR::Logger *log, TR_Structure *structure)
{
    if (structure->asRegion())
        printVCG(log, structure->asRegion());
}

void TR_Debug::printVCG(OMR::Logger *log, TR_RegionStructure *regionStructure)
{
    log->prints("graph: {\n");
    log->printf("title: \"%s\"\n", getName(regionStructure));

    printVCG(log, regionStructure->getEntry(), true);
    TR_RegionStructure::Cursor it(*regionStructure);
    TR_StructureSubGraphNode *node;
    for (node = it.getFirst(); node != NULL; node = it.getNext()) {
        printVCG(log, node, false);
    }
    it.reset();
    for (node = it.getFirst(); node != NULL; node = it.getNext()) {
        printVCGEdges(log, node);
    }

    log->prints("}\n");
}

void TR_Debug::printVCG(OMR::Logger *log, TR_StructureSubGraphNode *node, bool isEntry)
{
    if (_structureChecklist.isSet(node->getNumber()))
        return;
    _structureChecklist.set(node->getNumber());

    log->printf("node: {title: \"%s\" ", getName(node));
    log->printf("label: \"%d\" ", node->getNumber());
    if (isEntry)
        log->prints("vertical_order: 1 ");
    if (node->getStructure() == NULL) // exit destination
        log->prints("color: red}\n");
    else {
        if (node->getStructure()->asRegion())
            log->prints("color: lightcyan ");
        log->prints("}\n");
        printVCG(log, node->getStructure());
    }
}

void TR_Debug::printVCGEdges(OMR::Logger *log, TR_StructureSubGraphNode *node)
{
    for (auto edge = node->getSuccessors().begin(); edge != node->getSuccessors().end(); ++edge) {
        TR_StructureSubGraphNode *to = toStructureSubGraphNode((*edge)->getTo());
        printVCG(log, to, false); // print it out if it is an exit destination
        log->printf("edge: { sourcename: \"%s\" targetname: \"%s\" }\n", getName(node), getName(to));
    }

    for (auto edge = node->getExceptionSuccessors().begin(); edge != node->getExceptionSuccessors().end(); ++edge) {
        TR_StructureSubGraphNode *to = toStructureSubGraphNode((*edge)->getTo());
        printVCG(log, to, false); // print it out if it is an exit destination
        log->printf("edge: { sourcename: \"%s\" targetname: \"%s\" color: pink}\n", getName(node), getName(to));
    }
}

//      no-opt   cold    warm         hot            veryhot scorching unknown
static const char *blockColours[numHotnessLevels]
    = { "white", "blue", "lightblue", "lightyellow", "gold", "red", "white" };
static const char *edgeColours[numHotnessLevels]
    = { "black", "blue", "lightblue", "lightyellow", "gold", "red", "black" };

void TR_Debug::printVCG(OMR::Logger *log, TR::Block *block, int32_t vorder, int32_t horder)
{
    TR::CFG *cfg = _comp->getFlowGraph();

    log->printf("node: {title: \"%d\" ", block->getNumber());
    if (!block->getEntry()) {
        TR_ASSERT(!block->getExit(), "both entry and exit must be specified for TR_Block");
        if (block->getPredecessors().empty())
            log->prints("vertical_order: 0 label: \"Entry\" shape: ellipse color: lightgreen ");
        else
            log->prints("label: \"Exit\" shape: ellipse color: lightyellow ");
    } else {
        log->printf("label: \"%d", block->getNumber());
        log->prints("\" ");

        log->printf("color: %s ", blockColours[unknownHotness]);
        if (vorder != -1)
            log->printf("vertical_order: %d ", vorder);
        if (horder != -1)
            log->printf("horizontal_order: %d ", horder);
    }
    log->prints("}\n");

    TR::Block *b;
    for (auto edge = block->getSuccessors().begin(); edge != block->getSuccessors().end(); ++edge) {
        b = toBlock((*edge)->getTo());
        if (b->getNumber() >= 0) {
            log->printf("edge: { sourcename: \"%d\" targetname: \"%d\" color: %s}\n", block->getNumber(),
                b->getNumber(), edgeColours[unknownHotness]);
        }
    }

    for (auto edge = block->getExceptionSuccessors().begin(); edge != block->getExceptionSuccessors().end(); ++edge) {
        b = toBlock((*edge)->getTo());
        if (b->getNumber() >= 0) {
            log->printf(
                "edge: { sourcename: \"%d\" targetname: \"%d\" linestyle: dotted label: \"exception\" color: %s }\n",
                block->getNumber(), b->getNumber(), edgeColours[unknownHotness]);
        }
    }
}

// Output a representation of this node in the VCG output
//
void TR_Debug::printVCG(OMR::Logger *log, TR::Node *node, uint32_t indentation)
{
    int32_t i;

    if (_nodeChecklist.isSet(node->getGlobalIndex())) {
        log->printf("%*s==>%s at %s\\n", 12 + indentation, " ", getName(node->getOpCode()), getName(node));
        return;
    }

    _nodeChecklist.set(node->getGlobalIndex());
    log->printf("%s  ", getName(node));
    log->printf("%*s", indentation, " ");
    printNodeInfo(log, node);
    log->prints("\\n");

    indentation += 5;

    if (node->getOpCode().isSwitch()) {
        log->printf("%*s ***can't print switches yet***\\n", indentation + 10, " ");
    } else {
        for (i = 0; i < node->getNumChildren(); i++)
            printVCG(log, node->getChild(i), indentation);
    }
}

void TR_Debug::print(OMR::Logger *log, TR::VPConstraint *info)
{
    if (!info) {
        log->prints("none");
        return;
    }

    if (info->asIntConst()) {
        log->printf("%dI", info->getLowInt());
        return;
    }

    if (info->asIntRange()) {
        if (info->getLowInt() == TR::getMinSigned<TR::Int32>())
            log->prints("(TR::getMinSigned<TR::Int32>() ");
        else
            log->printf("(%d ", info->getLowInt());
        if (info->getHighInt() == TR::getMaxSigned<TR::Int32>())
            log->prints("to TR::getMaxSigned<TR::Int32>())");
        else
            log->printf("to %d)", info->getHighInt());
        log->printc('I');
        return;
    }

    if (info->asLongConst()) {
        log->printf(INT64_PRINTF_FORMAT "L", info->getLowLong());
        return;
    }

    if (info->asLongRange()) {
        if (info->getLowLong() == TR::getMinSigned<TR::Int64>())
            log->prints("(TR::getMinSigned<TR::Int64>() ");
        else
            log->printf("(" INT64_PRINTF_FORMAT " ", info->getLowLong());
        if (info->getHighLong() == TR::getMaxSigned<TR::Int64>())
            log->printf("to TR::getMaxSigned<TR::Int64>())");
        else
            log->printf("to " INT64_PRINTF_FORMAT ")", info->getHighLong());
        log->printc('L');
        return;
    }

    log->prints("unprintable constraint");
}

const char *TR_Debug::getName(TR::DataType type) { return TR::DataType::getName(type); }

// this should be the same as in OMRDataTypes.cpp
static const char *pRawSignCodeNames[TR_NUM_RAW_SIGN_CODES] = {
    "bcd_sign_unknown",
    "0xc",
    "0xd",
    "0xf",
};

const char *TR_Debug::getName(TR_RawBCDSignCode type)
{
    if (type < TR_NUM_RAW_SIGN_CODES)
        return pRawSignCodeNames[type];
    else
        return "unknown sign";
}

const char *TR_Debug::getName(TR::ILOpCodes opCode) { return TR::ILOpCode(opCode).getName(); }

const char *TR_Debug::getName(TR::ILOpCode opCode) { return opCode.getName(); }

void TR_Debug::verifyGlobalIndices(TR::Node *node, TR::Node **nodesByGlobalIndex)
{
    TR::Node *nodeFromArray = nodesByGlobalIndex[node->getGlobalIndex()];
    if (nodeFromArray == node)
        return;
    TR_ASSERT(nodeFromArray == NULL, "Found two nodes with the same global index: %p and %p", nodeFromArray, node);
    nodesByGlobalIndex[node->getGlobalIndex()] = node;

    for (int32_t i = node->getNumChildren() - 1; i >= 0; --i)
        verifyGlobalIndices(node->getChild(i), nodesByGlobalIndex);
}

void TR_Debug::verifyTrees(TR::ResolvedMethodSymbol *methodSymbol)
{
#ifndef ASSUMES
    if (!_comp->getOption(TR_TraceTreeVerification)) {
        return;
    }
#endif

    // Pre-allocate the bitvector to the correct size.
    // This prevents un-necessary growing and memory waste
    _nodeChecklist.set(comp()->getNodeCount() + 1);

    _nodeChecklist.empty();
    TR::TreeTop *tt, *firstTree = methodSymbol->getFirstTreeTop();
    for (tt = firstTree; tt; tt = tt->getNextTreeTop()) {
        TR::Node *node = tt->getNode();
        node->setLocalIndex(0);
        verifyTreesPass1(node);
    }

    _nodeChecklist.empty();
    for (tt = firstTree; tt; tt = tt->getNextTreeTop())
        verifyTreesPass2(tt->getNode(), true);

    // Disable verifyGlobalIndices() by default because the allocation below of
    // nodesByGlobalIndex exposes a leak in TR::Region / TR::SegmentProvider
    // when there are many nodes. This check has been effectively doing nothing
    // in most builds anyway because it just traverses the trees doing
    // TR_ASSERT().
    static const bool enableVerifyGlobalIndices = feGetEnv("TR_enableVerifyGlobalIndices") != NULL;

    if (enableVerifyGlobalIndices) {
        size_t size = _comp->getNodeCount() * sizeof(TR::Node *);
        TR::Node **nodesByGlobalIndex = (TR::Node **)_comp->trMemory()->allocateStackMemory(size);
        memset(nodesByGlobalIndex, 0, size);
        for (tt = firstTree; tt; tt = tt->getNextTreeTop())
            verifyGlobalIndices(tt->getNode(), nodesByGlobalIndex);
    }
}

// Node verification. This is done in 2 passes. In pass 1 the reference count is
// accumulated in the node's localIndex. In pass 2 the reference count is checked.
//
void TR_Debug::verifyTreesPass1(TR::Node *node)
{
    // If this is the first time through this node, verify the children
    //
    if (!_nodeChecklist.isSet(node->getGlobalIndex())) {
        _nodeChecklist.set(node->getGlobalIndex());

        for (int32_t i = node->getNumChildren() - 1; i >= 0; --i) {
            TR::Node *child = node->getChild(i);
            if (_nodeChecklist.isSet(child->getGlobalIndex())) {
                // Just inc simulated ref count
                child->incLocalIndex();
            } else {
                // Initialize simulated ref count and visit it
                child->setLocalIndex(1);
                verifyTreesPass1(child);
            }

            // Check the type of the child against the expected type
            //
            auto expectedType = node->getOpCode().expectedChildType(i);
            if (node->getOpCodeValue() == TR::multianewarray && i == node->getNumChildren() - 1) {
                expectedType = TR::Address;
            }

            if (debug("checkTypes") && expectedType != TR::NoType && expectedType < TR::NumAllTypes) {
                // See if the child's type is compatible with this node's type
                // Temporarily allow known cases to succeed
                //
                TR::ILOpCodes conversionOp = TR::BadILOp;
                TR::DataType childType = child->getDataType();
                if (childType != expectedType && childType != TR::NoType
                    && !((node->getOpCodeValue() == TR::imul || node->getOpCodeValue() == TR::ishl)
                        && child->getOpCodeValue() == TR::loadaddr)) {
                    logprintf(_comp->getOption(TR_TraceTreeVerification), getLogger(),
                        "TREE VERIFICATION ERROR -- node [%s] has wrong type for child [%s] (%s), expected %s\n",
                        getName(node), getName(child), getName(childType), getName(expectedType));

                    TR_ASSERT(debug("fixTrees"), "Tree verification error");
                }
            }
        }
    }
}

void TR_Debug::verifyTreesPass2(TR::Node *node, bool isTreeTop)
{
    OMR::Logger *log = getLogger();
    bool trace = _comp->getOption(TR_TraceTreeVerification);

    // Verify the reference count. Pass 1 should have set the localIndex to the
    // reference count.
    //
    if (!_nodeChecklist.isSet(node->getGlobalIndex())) {
        _nodeChecklist.set(node->getGlobalIndex());

        for (int32_t i = node->getNumChildren() - 1; i >= 0; --i)
            verifyTreesPass2(node->getChild(i), false);

        if (isTreeTop) {
            if (node->getReferenceCount() != 0) {
                logprintf(trace, log, "TREE VERIFICATION ERROR -- treetop node [%s] with ref count %d\n", getName(node),
                    node->getReferenceCount());

                TR_ASSERT(debug("fixTrees"), "Tree verification error");
                node->setReferenceCount(0);
            }
        }

        if (node->getReferenceCount() > 1
            && (node->getOpCodeValue() == TR::call || node->getOpCodeValue() == TR::calli)) {
            logprintf(trace, log, "TREE VERIFICATION ERROR -- void call node [%s] with ref count %d\n", getName(node),
                node->getReferenceCount());

            TR_ASSERT(debug("fixTrees"), "Tree verification error");
        }

        if (node->getReferenceCount() != node->getLocalIndex()) {
            logprintf(trace, log, "TREE VERIFICATION ERROR -- node [%s] ref count is %d and should be %d\n",
                getName(node), node->getReferenceCount(), node->getLocalIndex());

            TR_ASSERT(debug("fixTrees"), "Tree verification error");

            // if there is logging, don't fix the ref count!
            if (!trace) {
                node->setReferenceCount(node->getLocalIndex());
            }
        }
    }
}

TR::Node *TR_Debug::verifyFinalNodeReferenceCounts(TR::ResolvedMethodSymbol *methodSymbol)
{
    _nodeChecklist.empty();

    TR::Node *firstBadNode = NULL;
    for (TR::TreeTop *tt = methodSymbol->getFirstTreeTop(); tt; tt = tt->getNextTreeTop()) {
        TR::Node *badNode = verifyFinalNodeReferenceCounts(tt->getNode());
        if (!firstBadNode)
            firstBadNode = badNode;
    }

    if (_comp->getOption(TR_TraceNodeRefCountVerification)) {
        getLogger()->flush();
    }

    if (debug("enforceFinalNodeReferenceCounts")) {
        TR_ASSERT(!firstBadNode, "Node [%s] final ref count is %d and should be zero\n", getName(firstBadNode),
            firstBadNode->getReferenceCount());
    }

    return firstBadNode;
}

TR::Node *TR_Debug::verifyFinalNodeReferenceCounts(TR::Node *node)
{
    TR::Node *badNode = NULL;
    if (!_nodeChecklist.isSet(node->getGlobalIndex())) {
        _nodeChecklist.set(node->getGlobalIndex());

        // Check this node
        //
        if (node->getReferenceCount() != 0) {
            badNode = node;

            logprintf(_comp->getOption(TR_TraceNodeRefCountVerification), getLogger(),
                "WARNING -- node [%s] has final ref count %d and should be zero\n", getName(badNode),
                badNode->getReferenceCount());
        }

        // Recursively check its children
        //
        for (int32_t i = 0; i < node->getNumChildren(); i++) {
            TR::Node *badChild = verifyFinalNodeReferenceCounts(node->getChild(i));
            if (!badNode)
                badNode = badChild;
        }
    }

    return badNode;
}

// Verifies the number of times a node is referenced within a block
//
void TR_Debug::verifyBlocks(TR::ResolvedMethodSymbol *methodSymbol)
{
#ifndef ASSUMES
    if (!_comp->getOption(TR_TraceBlockVerification)) {
        return;
    }
#endif

    TR::TreeTop *tt, *exitTreeTop;
    for (tt = methodSymbol->getFirstTreeTop(); tt; tt = exitTreeTop->getNextTreeTop()) {
        TR::TreeTop *firstTreeTop = tt;
        exitTreeTop = tt->getExtendedBlockExitTreeTop();

        _nodeChecklist.empty();
        for (; tt != exitTreeTop->getNextTreeTop(); tt = tt->getNextTreeTop()) {
            TR_ASSERT(tt, "TreeTop problem in verifyBlocks");
            TR::Node *node = tt->getNode();
            node->setLocalIndex(node->getReferenceCount());
            verifyBlocksPass1(node);
        }

        _nodeChecklist.empty();

        // go back to the start of the block, and check the localIndex to make sure it is 0
        // do not walk the tree backwards as this causes huge stack usage in verifyBlocksPass2
        _nodeChecklist.empty();
        for (tt = firstTreeTop; tt != exitTreeTop->getNextTreeTop(); tt = tt->getNextTreeTop())
            verifyBlocksPass2(tt->getNode());
    }
}

// Child access within a Block verification. In pass 1, the verifyRefCount for each child is
// decremented for each visit.  The second pass is to make sure that the verifyRefCount is zero
// by the end of the block.
//
void TR_Debug::verifyBlocksPass1(TR::Node *node)
{
    // If this is the first time through this node, verify the children
    //
    if (!_nodeChecklist.isSet(node->getGlobalIndex())) {
        _nodeChecklist.set(node->getGlobalIndex());
        for (int32_t i = node->getNumChildren() - 1; i >= 0; --i) {
            TR::Node *child = node->getChild(i);
            if (_nodeChecklist.isSet(child->getGlobalIndex())) {
                // If the child has already been visited, decrement its verifyRefCount.
                child->decLocalIndex();
            } else {
                // If the child has not yet been visited, set its localIndex and visit it
                child->setLocalIndex(child->getReferenceCount() - 1);
                verifyBlocksPass1(child);
            }
        }
    }
}

void TR_Debug::verifyBlocksPass2(TR::Node *node)
{
    // Pass through and make sure that the localIndex == 0 for each child
    //
    if (!_nodeChecklist.isSet(node->getGlobalIndex())) {
        _nodeChecklist.set(node->getGlobalIndex());
        for (int32_t i = node->getNumChildren() - 1; i >= 0; --i)
            verifyBlocksPass2(node->getChild(i));

        if (node->getLocalIndex() != 0) {
            const size_t bufferSize = 150;
            char buffer[bufferSize];
            snprintf(buffer, bufferSize,
                "BLOCK VERIFICATION ERROR -- node [%s] accessed outside of its (extended) basic block: %d time(s)\n",
                getName(node), node->getLocalIndex());

            logprints(_comp->getOption(TR_TraceBlockVerification), getLogger(), buffer);

            TR_ASSERT(debug("fixTrees"), buffer);
        }
    }
}
