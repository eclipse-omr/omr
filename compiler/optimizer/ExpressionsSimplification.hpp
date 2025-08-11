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

#ifndef EXPSIMP_INCL
#define EXPSIMP_INCL

#include <stdint.h>
#include "env/TRMemory.hpp"
#include "il/Node.hpp"
#include "optimizer/Optimization.hpp"
#include "optimizer/OptimizationManager.hpp"

class TR_BitVector;
class TR_RegionStructure;
class TR_Structure;
namespace TR {
class Block;
class CFGNode;
class TreeTop;
}
template <class T> class List;
template <class T> class ListIterator;

/*
 * Class TR_ExpressionsSimplification
 * ==================================
 *
 * Expression simplification is a loop optimization that aims to optimize 
 * local variable updates (inside the loop) that can be predicted and done 
 * in the loop pre-header (outside the loop). The optimization only runs 
 * at higher optimization levels like scorching. Here is an example:
 * 
 * i=0;
 * total = 0;
 * while (i < N)
 *    {
 *    total = total + M;
 *    ...other code..
 *    i = i + 1;
 *    }
 * 
 * would be converted by expression simplification to:
 * 
 * i=0;
 * total = 0;
 * total = total + M*N;
 * while (i < N)
 *    {
 *    ...other code..
 *    i = i + 1;
 *    }
 * 
 * Thereby avoiding the update to total every time through the loop. This 
 * optimization is currently done for updates that are additions, subtractions, 
 * xors and negations inside the loop, i.e. the optimization knows the 
 * equivalent expression to emit in the loop pre-header for these kinds of 
 * updates inside the loop. Note that there are simplifying assumptions 
 * around the complexity of expressions allowed as M and N in the above 
 * example to keep the analysis relatively cheap.
 * 
 * While the optimization does win big when it succeeds in microbenchmarks 
 * it rarely makes a significant difference in real world programs (that 
 * simply do not contain enough such opportunities typically) meaning it 
 * has not received too much development focus over and above what was needed 
 * to catch some important cases that were observed in relevant benchmarks. 
 * Since this only runs at higher opt levels and is not particularly expensive, 
 * it does not interfere too much in real world Java programs either 
 * in terms of compile time or throughput.
 */
class TR_ExpressionsSimplification : public TR::Optimization
   {
   public:
   // Performs isolated store elimination
   //
   TR_ExpressionsSimplification(TR::OptimizationManager *manager);
   static TR::Optimization *create(TR::OptimizationManager *manager)
      {
      return new (manager->allocator()) TR_ExpressionsSimplification(manager);
      }

   class LoopInfo
      {
      public:
      TR_ALLOC(TR_Memory::ExpressionsSimplification)
      LoopInfo(TR::Node* boundry, int32_t lowerBound, int32_t upperBound, int32_t increment, bool equals)
         : _boundry(boundry), _lowerBound(lowerBound), _upperBound(upperBound), _increment(increment), _equals(equals)
            { }

      int32_t getLowerBound() { return _lowerBound; }
      int32_t getUpperBound() { return _upperBound; }
      int32_t getIncrement() { return _increment; }
      TR::Node* getBoundaryNode() { return _boundry; }
      bool isEquals() {return _equals;}

      int32_t getNumIterations()
         {
         if (_increment == 0)
            return 0;

         if ((_increment > 0 && _lowerBound > _upperBound) || (_increment < 0 && _lowerBound < _upperBound))
            return 0;

         // In extreme cases, the number of iterations might be greater than or equal to 2^31.
         // Calculate the number of iterations using int64_t calculations, and return zero
         // (i.e., unknown) as the number of iterations if the value exceeds the maximum value
         // of type int32_t
         int64_t lb64 = _lowerBound;
         int64_t ub64 = _upperBound;
         int64_t inc64 = _increment;
         int64_t numIters;

         if (isEquals())
            {
            numIters = (ub64 - lb64 + inc64)/inc64;
            }
         else if (_increment > 0)
            {
            numIters = (ub64 - lb64 + inc64 - 1)/inc64;
            }
         else
            {
            numIters = (ub64 - lb64 + inc64 + 1)/inc64;
            }

         return (numIters <= std::numeric_limits<int32_t>::max()) ? (int32_t) numIters : 0;
         }

      private:
      TR::Node* _boundry;
      int32_t  _lowerBound;
      int32_t  _upperBound;
      int32_t  _increment;
      bool     _equals;
      };

   virtual int32_t perform();
   virtual const char * optDetailString() const throw();

   protected:
   vcount_t _visitCount;


   private:
   void findAndSimplifyInvariantLoopExpressions(TR_RegionStructure *);
   void invalidateCandidates();
   void removeUncertainBlocks(TR_RegionStructure*, List<TR::Block> *blocksInLoop);
   LoopInfo* findLoopInfo(TR_RegionStructure*);
   TR::Block * findPredecessorBlock(TR::CFGNode *);
   void transformNode(TR::Node *, TR::Block *);
   bool findUseInRegion(TR_RegionStructure *, uint16_t, uint16_t);
   bool findUseInRegion(TR::Node*, uint16_t, uint16_t);
   void removeCandidate(TR_RegionStructure *);
   void removeCandidate(TR::Node *, TR::TreeTop *);
   int32_t perform(TR_Structure *);
   void simplifyInvariantLoopExpressions(ListIterator<TR::Block> &blocks);
   void setSummationReductionCandidates(TR::Node *currentNode, TR::TreeTop *tt);
   void setStoreMotionCandidates(TR::Node *currentNode, TR::TreeTop *tt);
   bool tranformSummationReductionCandidate(TR::TreeTop *treeTop, LoopInfo *loopInfo, bool *isPreheaderBlockInvalid);
   void tranformStoreMotionCandidate(TR::TreeTop *treeTop, bool *isPreheaderBlockInvalid);

   TR::Node* iaddisubSimplifier(TR::Node *, LoopInfo*);
   TR::Node* ixorinegSimplifier(TR::Node *, LoopInfo*, bool *);
   bool checkForLoad(TR::Node *node, TR::Node *load);
   void removeUnsupportedCandidates();
   bool isSupportedNodeForExpressionSimplification(TR::Node *node);

   /**
    * Holds information from analysis of a candidate for expression simplification.
    * Used to determine the kind of transformation that should be attempted.
    */
   class SimplificationCandidateTuple
      {
      public:

      /**
       * Public constructor for a candidate for expression simplification
       * \param tt    The \ref TR::TreeTop whose tree is a candidate for expression
       *              simplification
       * \param flags Flags describing the kind of expression simplification that
       *              should be attempted on this candidate
       */
      SimplificationCandidateTuple(TR::TreeTop *tt, flags32_t flags) : _treeTop(tt), _flags(flags) {}

      enum
         {
         /**
          * Flag that indicates this is a candidate for a loop invariant transformation
          */
         InvariantExpressionCandidate = 0x01,

         /**
          * Flag that indicates this is a candidate for a summation reduction transformation
          */
         SummationReductionCandidate = 0x02,
         };

      /**
       * Query to check whether this was identified to be a candidate for a loop
       * invariant transformation
       * \return \c true if and only if this is a candidate for a loop invariant
       *         transformation
       */
      bool isInvariantExpressionCandidate()
         {
         return _flags.testAny(InvariantExpressionCandidate);
         }

      /**
       * Query to check whether this was identified to be a candidate for a summation
       * reduction transformation
       * \return \c true if and only if this is a candidate for a summation
       *         reduction transformation
       */
      bool isSummationReductionCandidate()
         {
         return _flags.testAny(SummationReductionCandidate);
         }

      /**
       * Query to retrieve the \ref TR::TreeTop whose tree is the candidate for an
       * expression simplification transformation
       * \return This candidate's \c TR::TreeTop
       */
      TR::TreeTop *getTreeTop()
         {
         return _treeTop;
         }

      /**
       * Prints trace output describing this candidate for expression simplification
       */
      void print(TR::Compilation *comp);

      private:

      /**
       * The \ref TR::TreeTop whose tree is the candidate for an expression
       * simplification transformation
       */
      TR::TreeTop *_treeTop;

      /**
       * Flags describing the kind of expression simplification that should be
       * attempted on this candidate
       */
      flags32_t _flags;
      };

   TR_RegionStructure* _currentRegion;
   List<SimplificationCandidateTuple> *_candidates;

   TR_BitVector *_supportedExpressions;
   };

#endif
