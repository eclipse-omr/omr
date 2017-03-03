/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 2000, 2016
 *
 *  This program and the accompanying materials are made available
 *  under the terms of the Eclipse Public License v1.0 and
 *  Apache License v2.0 which accompanies this distribution.
 *
 *      The Eclipse Public License is available at
 *      http://www.eclipse.org/legal/epl-v10.html
 *
 *      The Apache License v2.0 is available at
 *      http://www.opensource.org/licenses/apache2.0.php
 *
 * Contributors:
 *    Multiple authors (IBM Corp.) - initial implementation and documentation
 *******************************************************************************/

#ifndef GENERALLOOPUNROLLER_INCL
#define GENERALLOOPUNROLLER_INCL

#include <stddef.h>                              // for NULL
#include <stdint.h>                              // for int32_t, int64_t
#include "codegen/LinkageConventionsEnum.hpp"
#include "compile/Compilation.hpp"               // for Compilation
#include "cs2/arrayof.h"                         // for ArrayOf
#include "env/TRMemory.hpp"                      // for TR_Memory, etc
#include "env/jittypes.h"                        // for intptrj_t
#include "il/Block.hpp"                          // for Block
#include "il/DataTypes.hpp"                      // for DataTypes, TR::DataType
#include "il/ILOpCodes.hpp"                      // for ILOpCodes
#include "il/ILOps.hpp"                          // for TR::ILOpCode
#include "il/Node.hpp"                           // for Node, vcount_t
#include "il/Node_inlines.hpp"                   // for Node::getFirstChild, etc
#include "il/Symbol.hpp"                         // for Symbol
#include "il/SymbolReference.hpp"
#include "il/TreeTop.hpp"                        // for TreeTop
#include "il/TreeTop_inlines.hpp"                // for TreeTop::getNode
#include "infra/Array.hpp"                       // for TR_Array
#include "infra/Assert.hpp"                      // for TR_ASSERT
#include "infra/List.hpp"
#include "optimizer/OptimizationManager.hpp"     // for OptimizationManager
#include "optimizer/Optimizations.hpp"
#include "optimizer/InductionVariable.hpp"
#include "optimizer/LoopCanonicalizer.hpp"  // for TR_LoopTransformer

class TR_BitVector;
class TR_BlockStructure;
class TR_Debug;
class TR_HashTab;
class TR_RegionStructure;
class TR_Structure;
class TR_StructureSubGraphNode;
class TR_UseDefInfo;
namespace TR { class AutomaticSymbol; }
namespace TR { class CFG; }
namespace TR { class CFGEdge; }
namespace TR { class Optimization; }
namespace TR { class Optimizer; }
namespace TR { class ParameterSymbol; }

class TR_LoopUnroller
   {
   public:
   enum UnrollKind
      {
      NoUnroll,                 //
      CompleteUnroll,           // No loop
      ExactUnroll,              // Without residue
      PeelOnceUnrollOnce,       //
      UnrollWithResidue,        //
      GeneralUnroll,            // Retains backedges
      SPMDKernel,               // Vector
      };

   // unroll a counted loop - returns true on success
   //
   static bool unroll(TR::Compilation *comp, TR_RegionStructure *loop,
                      TR_PrimaryInductionVariable *piv, UnrollKind kind,
                      int32_t unrollCount, int32_t peelCount, TR::Optimizer *optimizer=NULL);

   // unroll a non-counted loop - returns true on success
   //
   static bool unroll(TR::Compilation *comp, TR_RegionStructure *loop,
                      int32_t unrollCount, int32_t peelCount, TR::Optimizer *optimizer=NULL);
   static TR_StructureSubGraphNode *findNodeInHierarchy(TR_RegionStructure*, int32_t);

   protected:

   TR_LoopUnroller(TR::Compilation *comp, TR::Optimizer *optimizer, TR_RegionStructure *loop,
                   TR_StructureSubGraphNode *branchNode, int32_t unrollCount,
                   int32_t peelCount, TR::Block *invariantBlock, UnrollKind unrollKind, int32_t vectorSize = 1);

   TR_LoopUnroller(TR::Compilation *comp, TR::Optimizer *optimizer, TR_RegionStructure *loop,
                   TR_PrimaryInductionVariable *piv, UnrollKind unrollKind,
                   int32_t unrollCount, int32_t peelCount, TR::Block *invariantBlock, int32_t vectorSize = 1);

   TR::Compilation *comp()  { return _comp; }
   TR::Optimizer *optimizer()  { return _optimizer; }
   bool            trace() { return comp()->trace(OMR::generalLoopUnroller) || comp()->trace(OMR::SPMDKernelParallelization); }
   TR_Debug *  getDebug() { return comp()->getDebug(); }

   TR_Memory *               trMemory()                    { return _trMemory; }
   TR_StackMemory            trStackMemory()               { return _trMemory; }
   TR_HeapMemory             trHeapMemory()                { return _trMemory; }
   TR_PersistentMemory *     trPersistentMemory()          { return _trMemory->trPersistentMemory(); }

   bool            isCountedLoop()     { return (_piv != 0); }
   bool            isIncreasingLoop()  { TR_ASSERT(isCountedLoop(), "assertion failure"); return (_piv->getDeltaOnBackEdge() > 0); }
   int32_t getLoopStride()
      { TR_ASSERT(isCountedLoop(), "assertion failure"); return _piv->getDeltaOnBackEdge(); }




   TR::Node *getTestNode()         { TR_ASSERT(_piv, "assertion failure"); return _piv->getBranchBlock()->getLastRealTreeTop()->getNode(); }
   TR::DataType getIndexType()     { TR_ASSERT(_piv, "assertion failure"); return _piv->getSymRef()->getSymbol()->getType(); }
   TR::ILOpCode getTestOpCode2()    { return getTestNode()->getOpCode(); }
   TR::DataType getTestChildType() { return getTestNode()->getFirstChild()->getType(); }
   bool    getTestIsUnsigned()     { return getTestOpCode2().isUnsignedCompare(); }

   static bool isWellFormedLoop(TR_RegionStructure *loop, TR::Compilation *, TR::Block *&loopInvariantBlock);
   static bool isTransactionStartLoop(TR_RegionStructure *loop, TR::Compilation *);


   enum EdgeContext {InvalidContext = 0, BackEdgeFromPrevGeneration, BackEdgeToEntry,
		     ExitEdgeFromBranchNode, BackEdgeFromLastGenerationCompleteUnroll};

   void prepareForArrayShadowRenaming(TR_RegionStructure *loop);
   void getLoopPreheaders(TR_RegionStructure *loop, TR_ScratchList<TR::Block> *preheaders);
   void collectInternalPointers();
   void collectArrayAccesses();
   void examineNode(TR::Node *node, intptrj_t visitCount);
   struct IntrnPtr;
   IntrnPtr *findIntrnPtr(int32_t symRefNum);
   bool haveIdenticalOffsets(IntrnPtr *intrnPtr1, IntrnPtr *intrnPtr2);
   void examineArrayAccesses();
   void refineArrayAliasing();

   int32_t unroll(TR_RegionStructure *loop, TR_StructureSubGraphNode *branchNode);
   void unrollLoopOnce(TR_RegionStructure *loop, TR_StructureSubGraphNode *branchNode, bool finalUnroll);
   bool shouldConnectToNextIteration(TR_StructureSubGraphNode *subNode, TR_RegionStructure *loop);

   void generateSpillLoop(TR_RegionStructure*, TR_StructureSubGraphNode*);
   void modifyOriginalLoop(TR_RegionStructure*, TR_StructureSubGraphNode*);
   void modifyBranchTree(TR_RegionStructure *loop,
			 TR_StructureSubGraphNode *loopNode,
			 TR_StructureSubGraphNode *branchNode);
   int32_t numExitEdgesTo(TR_RegionStructure *from, int32_t to);
   bool edgeAlreadyExists(TR_StructureSubGraphNode *from,
			  TR_StructureSubGraphNode *to);
   bool edgeAlreadyExists(TR_StructureSubGraphNode *from,
			  int32_t toNum);
   bool cfgEdgeAlreadyExists(TR::Block *from, TR::Block *to, EdgeContext edgeContext = InvalidContext);

   TR_StructureSubGraphNode *getEntryBlockNode(TR_StructureSubGraphNode *node);
   void cloneBlocksInRegion(TR_RegionStructure *, bool isSpillLoop = false);
   inline TR_Structure *cloneStructure(TR_Structure *);
   TR_Structure *cloneBlockStructure(TR_BlockStructure *);
   TR_Structure *cloneRegionStructure(TR_RegionStructure *);
   void removeExternalEdge(TR_RegionStructure *parent,
			   TR_StructureSubGraphNode *from, int32_t to);
   void redirectBackEdgeToExitDestination(TR_RegionStructure *loop,
					  TR_StructureSubGraphNode *branchNode,
					  TR_StructureSubGraphNode *fromNode,
                                          bool notLoopBranchNode);

   void addEdgeForSpillLoop(TR_RegionStructure *region,
			    TR::CFGEdge *originalEdge,
			    TR_StructureSubGraphNode *newFromNode,
			    TR_StructureSubGraphNode *newToNode,
			    bool removeOriginal = false,
			    EdgeContext context = InvalidContext,
                            bool notLoopBranchNode = false);
   void addExitEdgeAndFixEverything(TR_RegionStructure *region,
				    TR::CFGEdge *originalEdge,
				    TR_StructureSubGraphNode *newFromNode,
				    TR_StructureSubGraphNode *exitNode = NULL,
				    TR::Block *newExitBlock = NULL,
				    EdgeContext context = InvalidContext);
   void addEdgeAndFixEverything(TR_RegionStructure *region, TR::CFGEdge *originalEdge,
				TR_StructureSubGraphNode *fromNode = NULL,
				TR_StructureSubGraphNode *newToNode = NULL,
				bool redirectOriginal = false,
				bool removeOriginalEdges = false,
				bool edgeToEntry = false,
				EdgeContext context = InvalidContext);

   //void fixExitEdges(TR_Structure *s, TR_Structure *clone);
   void fixExitEdges(TR_Structure *s, TR_Structure *clone,
		     TR_StructureSubGraphNode *branchNode = 0);

   void addEdgesInHierarchy(TR_RegionStructure *region,
			    TR_StructureSubGraphNode *from, TR_StructureSubGraphNode *to);
   TR::Symbol *findSymbolInTree(TR::Node *node);
   static bool nodeRefersToSymbol(TR::Node*, TR::Symbol*);
   bool isSuccessor(TR::Block *A, TR::Block *B);

   TR::CFGEdge *findEdge(TR_StructureSubGraphNode *from, TR_StructureSubGraphNode *to);

   TR::Node *createIfNodeForSpillLoop(TR::Node *ifNode);

   void swingBlocks(TR::Block *from, TR::Block *to);  //Adds an edge to the queue of to-be-swung blocks
   void processSwingBlocks(TR::Block *from, TR::Block *to);
   void processSwingQueue();

   bool isInternalPointerLimitExceeded();

   void prepareLoopStructure(TR_RegionStructure *);

   struct SwingPair
      {
      TR::Block *from;
      TR::Block *to;
      SwingPair(TR::Block *f, TR::Block *t) : from(f), to(t) {}
      };

   TR::Compilation            *_comp;
   TR_Memory *                _trMemory;
   TR::Optimizer             *_optimizer;
   TR_RegionStructure        *_loop;
   TR_StructureSubGraphNode  *_branchNode;
   int32_t                    _unrollCount;
   int32_t                    _peelCount;
   UnrollKind                 _unrollKind;
   int32_t                    _vectorSize;

   TR_RegionStructure        *_rootStructure;
   TR::CFG                    *_cfg;

   int32_t                    _iteration;
   TR::Block                 **_blockMapper[2];
   TR_StructureSubGraphNode **_nodeMapper[2];
   List<SwingPair>            _swingQueue;
   int32_t                    _numNodes; // number of blocks at the start of unrolling
   TR_StructureSubGraphNode  *_firstEntryNode;

   // Counted Loops only
   TR_PrimaryInductionVariable *_piv;
   TR_StructureSubGraphNode    *_spillNode;
   TR::Block                    *_spillBranchBlock;
   bool                         _spillLoopRequired;
   TR::Block                    *_overflowTestBlock; // FIXME: init
   TR::Block                    *_loopIterTestBlock; // FIXME: init
   TR::Block                    *_loopInvariantBlock;
   bool                         _loopInvariantBlockAtEnd;
   bool                         _branchToExit;
   bool                         _wasEQorNELoop;
   TR::ILOpCodes                 _origLoopCondition;
   TR::TreeTop                  *_startPosOfUnrolledBodies;
   TR::TreeTop                  *_endPosOfUnrolledBodies;

   TR_ScratchList<TR::SymbolReference> _newSymRefs;
   struct IntrnPtr
      {
      int32_t symRefNum;
      TR_BasicInductionVariable *biv;
      int32_t bivNum;
      TR::Node *offsetNode;
      bool isOffsetConst;
      int64_t offsetValue;
      };
   TR_ScratchList<IntrnPtr> _listOfInternalPointers;
   struct ArrayAccess
      {
      TR::Node *aaNode;
      TR::Node *intrnPtrNode;
      };
   struct ListsOfArrayAccesses
      {
      int32_t symRefNum;
      TR_ScratchList<ArrayAccess> *list;
      };
   TR_ScratchList<ListsOfArrayAccesses> _listOfListsOfArrayAccesses;
   friend class TR_SPMDKernelParallelizer;

private:
   void manuallyRemoveEdgeFromCFG(TR::CFGEdge &edge);
   };


/**
 * Class TR_GeneralLoopUnroller
 * ============================
 *
 * The general loop unroller optimization can unroll or peel a majority of 
 * loops with or without the loop driving tests done after each iteration 
 * (loop) body. Usually a peel is inserted before the unrolled loop, and 
 * the residual iterations to be done (at most u - 1 residual iterations 
 * where u is the unroll factor) are also done using the peeled code. 
 * Peeling aids partial redundancy elimination (PRE) as code does not have 
 * to be moved; instead dominated expressions can be commoned (which is far 
 * easier than code motion due to problems introduced by exception 
 * checks in Java).
 *
 * Async checks (yield points) are eliminated from u - 1 unrolled bodies 
 * and only one remains.
 * 
 * Unroll factors and code growth thresholds are arrived at based on 
 * profiling information when available. This analysis uses induction 
 * variable information found by value propagation.
 */

class LoopWeightProbe;
class TR_GeneralLoopUnroller : public TR_LoopTransformer
   {
   public:

   TR_GeneralLoopUnroller(TR::OptimizationManager *manager);
   static TR::Optimization *create(TR::OptimizationManager *manager)
      {
      return new (manager->allocator()) TR_GeneralLoopUnroller(manager);
      }

   virtual int32_t perform();

   private:
   void collectNonColdInnerLoops(TR_RegionStructure *region, List<TR_RegionStructure> &innerLoops);

   // candidates for being made static
   void gatherStatistics(TR_Structure *str, int32_t &numNodes, int32_t &numBlocks,
                         int32_t &numBranches, int32_t &numSubscripts, LoopWeightProbe &lwp);
   int32_t weighNaturalLoop(TR_RegionStructure *loop,
                            TR_LoopUnroller::UnrollKind &, int32_t &unrollCount,
                            int32_t &peelCount, int32_t &cost);
   bool canUnrollUnCountedLoop(TR_RegionStructure *loop,
                               int32_t numBlocks, int32_t numNodes,
                               int32_t entryBlockFrequency);

   bool branchContainsInductionVariable(TR_RegionStructure *loop, TR::Node *branchNode, bool checkOnlyPiv = true);
   bool branchContainsInductionVariable(TR::Node *branchNode, TR::SymbolReference *ivSymRef);

   void    countNodesAndSubscripts(TR::Node *node, int32_t &numNodes, int32_t &numSubscripts,
         LoopWeightProbe &lwp);
   void profileNonCountedLoops(List<TR_RegionStructure> &loops);

   bool  _haveProfilingInfo;

   struct UnrollInfo
      {
      TR_ALLOC(TR_Memory::InductionVariableAnalysis);
      UnrollInfo(TR_RegionStructure *loop,
                 int32_t w, int32_t c,
                 TR_LoopUnroller::UnrollKind unrollKind,
                 int32_t unrollCount, int32_t peelCount)
         : _loop(loop), _weight(w), _cost(c), _unrollKind(unrollKind),
           _unrollCount(unrollCount), _peelCount(peelCount) {}

      TR_RegionStructure *_loop;
      TR_LoopUnroller::UnrollKind _unrollKind;
      int32_t _weight;
      int32_t _cost;
      int32_t    _unrollCount;
      int32_t    _peelCount;
      };

   int32_t _basicSizeThreshold;
   };

#endif
