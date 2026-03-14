/*******************************************************************************
 * Copyright IBM Corp. and others 2024
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

#ifndef CONSTREFPRIVATIZATION_INCL
#define CONSTREFPRIVATIZATION_INCL

#include "env/KnownObjectTable.hpp"
#include "env/TRMemory.hpp"
#include "infra/Bit.hpp"
#include "infra/CfgNode.hpp"
#include "infra/vector.hpp"
#include "optimizer/Optimization.hpp"
#include "optimizer/OptimizationManager.hpp"

class TR_RegionStructure;
class TR_StructureSubGraphNode;

namespace TR {

class ConstRefPrivatization : public TR::Optimization {
public:
    ConstRefPrivatization(TR::OptimizationManager *manager);

    static TR::Optimization *create(TR::OptimizationManager *manager)
    {
        return new (manager->allocator()) ConstRefPrivatization(manager);
    }

    virtual int32_t perform();
    virtual const char *optDetailString() const throw();

private:
    struct BlockNodePair {
        TR::Block *_block;
        TR::Node *_node;
    };

    struct NodeTreePair {
        TR::Node *_node;
        TR::TreeTop *_tree;
    };

    struct BlockInfo {
        BlockInfo(TR::Region &region);
        TR::vector<NodeTreePair, TR::Region &> _constRefLoads;
        uint32_t _flowDataIndex;
        bool _seen;
        bool _ignored;
        bool _isExit;
    };

    struct RegionStackEntry {
        RegionStackEntry(TR_StructureSubGraphNode *node);
        TR_StructureSubGraphNode *_node;
        TR::CFGEdgeList::iterator _it;
        TR::CFGEdgeList::iterator _end;
    };

    template<typename T> class FlowVector {
    public:
        FlowVector(TR::ConstRefPrivatization *opt)
            : _opt(opt)
            , _vec(*opt->_memRegion)
            , _len(0)
            , _numNodes(0)
        {}

        void reset(uint32_t len, const T &value, uint32_t numNodes = 0)
        {
            if (numNodes == 0) {
                numNodes = (uint32_t)_opt->_regionNodes->size();
            }

            _len = len;
            _numNodes = numNodes;
            _vec.clear();
            TR_ASSERT_FATAL((size_t)_len <= SIZE_MAX / (size_t)_numNodes, "overflow");
            _vec.resize((size_t)_numNodes * (size_t)_len, value);
        }

        class NodeView {
        public:
            NodeView(FlowVector &fv, uint32_t fdi)
                : _data(initData(fv, fdi))
                , _len(fv._len)
            {}

            T &operator[](uint32_t i)
            {
                TR_ASSERT_FATAL(i < _len, "i=%u >= _len=%u", i, _len);
                return _data[i];
            }

            uint32_t len() { return _len; }

        private:
            static T *initData(FlowVector &fv, uint32_t fdi)
            {
                TR_ASSERT_FATAL(fdi < fv._numNodes, "fdi=%u >= _numNodes=%u", fdi, fv._numNodes);
                return &fv._vec[(size_t)fdi * (size_t)fv._len];
            }

            T *_data;
            uint32_t _len;
        };

        NodeView atNode(uint32_t fdi) { return NodeView(*this, fdi); }

        NodeView atNode(const BlockInfo &bi) { return atNode(bi._flowDataIndex); }

        uint32_t len() { return _len; }

        uint32_t numNodes() { return _numNodes; }

    private:
        TR::ConstRefPrivatization * const _opt;
        TR::vector<T, TR::Region &> _vec;
        size_t _len;
        size_t _numNodes;
    };

    class FlowBitSet {
    public:
        FlowBitSet(TR::ConstRefPrivatization *opt)
            : _fv(opt)
            , _numBits(0)
        {}

        void reset(uint32_t numBits, uint32_t numNodes = 0)
        {
            _numBits = numBits;
            uint64_t initialMask = (uint64_t)0;
            _fv.reset((numBits + 63) >> 6, initialMask, numNodes);
        }

        class NodeView {
        public:
            NodeView(FlowBitSet &set, uint32_t fdi)
                : _fvView(set._fv.atNode(fdi))
                , _numBits(set._numBits)
            {}

            bool isBitSet(uint32_t i) { return (word(i) & mask(i)) != 0; }

            void setBit(uint32_t i) { word(i) |= mask(i); }

            bool overlaps(NodeView other)
            {
                assertSameSize(other);
                for (uint32_t i = 0; i < len(); i++) {
                    if ((_fvView[i] & other._fvView[i]) != 0) {
                        return true;
                    }
                }

                return false;
            }

            bool contains(NodeView other)
            {
                assertSameSize(other);
                for (uint32_t i = 0; i < len(); i++) {
                    uint64_t rhsBits = other._fvView[i];
                    if ((_fvView[i] & rhsBits) != rhsBits) {
                        return false;
                    }
                }

                return true;
            }

            bool equals(NodeView other)
            {
                assertSameSize(other);
                for (uint32_t i = 0; i < len(); i++) {
                    if (_fvView[i] != other._fvView[i]) {
                        return false;
                    }
                }

                return true;
            }

            bool areAllBitsClear()
            {
                for (uint32_t i = 0; i < len(); i++) {
                    if (_fvView[i] != (uint64_t)0) {
                        return false;
                    }
                }

                return true;
            }

            void copyFrom(NodeView other)
            {
                assertSameSize(other);
                for (uint32_t i = 0; i < len(); i++) {
                    _fvView[i] = other._fvView[i];
                }
            }

            void unionWith(NodeView other)
            {
                assertSameSize(other);
                for (uint32_t i = 0; i < len(); i++) {
                    _fvView[i] |= other._fvView[i];
                }
            }

            void intersectWith(NodeView other)
            {
                assertSameSize(other);
                for (uint32_t i = 0; i < len(); i++) {
                    _fvView[i] &= other._fvView[i];
                }
            }

            void subtract(NodeView other)
            {
                assertSameSize(other);
                for (uint32_t i = 0; i < len(); i++) {
                    _fvView[i] &= ~other._fvView[i];
                }
            }

            void clearAllBits()
            {
                for (uint32_t i = 0; i < len(); i++) {
                    _fvView[i] = (uint64_t)0;
                }
            }

            void setAllBits()
            {
                if (len() == 0) {
                    return;
                }

                uint32_t len1 = len() - 1;
                for (uint32_t i = 0; i < len1; i++) {
                    _fvView[i] = ~(uint64_t)0;
                }

                uint32_t bits = _numBits;
                uint64_t lastMask = ~(uint64_t)0;
                if ((bits & 0x3f) != 0) {
                    lastMask = ((uint64_t)1 << (bits & 0x3f)) - 1;
                }

                _fvView[len1] = lastMask;
            }

            class Iterator {
            public:
                Iterator(NodeView &view)
                    : _setView(view)
                    , _i(0)
                    , _value(0)
                {
                    advance();
                }

                bool next(uint32_t *out)
                {
                    if (_value == 0) {
                        *out = 0;
                        return false;
                    }

                    *out = trailingZeroes(_value) + 64 * _i;
                    _value &= _value - 1;
                    if (_value == 0) {
                        _i++;
                        advance();
                    }

                    return true;
                }

            private:
                void advance()
                {
                    while (_i < _setView.len()) {
                        _value = _setView._fvView[_i];
                        if (_value != 0) {
                            break;
                        }

                        _i++;
                    }
                }

                NodeView &_setView;
                uint64_t _value;
                uint32_t _i;
            };

            Iterator iter() { return Iterator(*this); }

        private:
            uint32_t len() { return _fvView.len(); }

            uint64_t mask(uint32_t i) { return (uint64_t)1 << (i & 0x3f); }

            uint64_t &word(uint32_t i)
            {
                TR_ASSERT_FATAL(i < _numBits, "i=%u >= _numBits=%u", i, _numBits);
                return _fvView[i >> 6];
            }

            void assertSameSize(NodeView other)
            {
                TR_ASSERT_FATAL(_numBits == other._numBits, "bit set size mismatch: %u vs. %u", _numBits,
                    other._numBits);
            }

            FlowVector<uint64_t>::NodeView _fvView;
            uint32_t _numBits;
        };

        NodeView atNode(uint32_t fdi) { return NodeView(*this, fdi); }

        NodeView atNode(const BlockInfo &bi) { return atNode(bi._flowDataIndex); }

        uint32_t numBits() { return _numBits; }

        uint32_t numNodes() { return _fv.numNodes(); }

    private:
        FlowVector<uint64_t> _fv;
        uint32_t _numBits;
    };

    struct NewConstRefLoad {
        TR::Block *_block;
        NodeTreePair _load;
    };

    struct StackMemData {
        StackMemData(TR::ConstRefPrivatization *opt);

        TR::vector<BlockInfo, TR::Region &> _blockInfos; // indexed by block number
        TR::vector<NewConstRefLoad, TR::Region &> _newConstRefLoads;
        TR::vector<RegionStackEntry, TR::Region &> _regionNodeStack;
        TR::vector<TR::Block *, TR::Region &> _predBlocks;
        TR::vector<TR::Block *, TR::Region &> _criticalEdgeSplitPredBlocks;
        TR::vector<bool, TR::Region &> _koiIsUsed;
        TR::vector<TR::KnownObjectTable::Index, TR::Region &> _usedKois;
        TR::vector<uint32_t, TR::Region &> _koiToMui;
        TR::vector<TR::KnownObjectTable::Index, TR::Region &> _muiToKoi;
        TR::vector<uint32_t, TR::Region &> _muiToCi;
        TR::vector<uint32_t, TR::Region &> _ciToMui;
        TR::vector<BlockNodePair, TR::Region &> _curBlockCiToTempLoad;
        TR::vector<int32_t, TR::Region &> _loopLoadFrequencyByKoi;
        TR::vector<bool, TR::Region &> _koiIsHoisted;

        FlowBitSet _usedMuis; // muis used directly in a given block

        // muis that may have been loaded by an earlier unignored block in
        // directly in the current region before entering a given node.
        FlowBitSet _potentiallyAvailableMuis;

        // Probability of loading a candidate in an unignored block directly in
        // the current region once we exit a given node.
        FlowVector<float> _ciFutureLoadP;

        // Probability of loading a candidate in an unignored block directly in
        // the current region once we enter a given node. This is the same as
        // _ciFutureLoadP but it includes candidates used in the node itself.
        FlowVector<float> _ciLoadP;

        FlowBitSet _initHereCis;
        FlowBitSet _initIncomingCis;
        FlowBitSet _initCis; // current node or earlier, includes _initHereCis
        FlowBitSet _liveCis; // live on entry

        FlowBitSet _scratchCisBacking;
    };

    typedef TR::vector<TR_StructureSubGraphNode *, TR::Region &> SubNodeVector;

    bool collectConstRefLoads();
    void processRegion(TR_RegionStructure *region);
    void populateRegionNodes(TR_RegionStructure *region, SubNodeVector &regionNodes);
    void ignoreRegion(TR_RegionStructure *region);
    void ignoreRegionRec(TR_RegionStructure *region);
    void findMultiplyUsedConstRefs();
    void doAcyclicAnalysisAndTransform(TR_RegionStructure *region);
    void appendTempStores(TR::Block *predBlock, FlowBitSet::NodeView edgeWantCis);
    void appendSingleTempStore(TR::Block *storeBlock, TR::KnownObjectTable::Index koi);
    void flushNewConstRefLoads();
    void collectPredBlocks(TR_StructureSubGraphNode *node, TR_StructureSubGraphNode *pred);
    void collectPredBlocksRec(TR_StructureSubGraphNode *node, TR_StructureSubGraphNode *pred);
    void hoistConstRefLoads(TR_RegionStructure *region);

    TR::SymbolReference *knownObjTemp(TR::KnownObjectTable::Index koi);
    void changeStaticLoadToTempLoad(TR::Node *load, TR::KnownObjectTable::Index koi);
    void pruneTransformedConstRefLoads(BlockInfo &blockInfo);

    const char *regionKindStr(TR_RegionStructure *region);
    void traceBitSet(FlowBitSet::NodeView set);
    void traceCiLoadPs(FlowVector<float>::NodeView xs);

    float clamp(float lo, float x, float hi) { return x < lo ? lo : x > hi ? hi : x; }

    TR::KnownObjectTable * const _knot;
    const TR::KnownObjectTable::Index _knotSize;
    TR::Region *_memRegion; // memory region
    StackMemData *_sd;
    const char *_curRegionKindStr;
    SubNodeVector *_regionNodes;
};

} // namespace TR

#endif // CONSTREFPRIVATIZATION_INCL
