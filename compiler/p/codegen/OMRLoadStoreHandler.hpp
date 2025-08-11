/*******************************************************************************
 * Copyright IBM Corp. and others 2020
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

#ifndef OMR_POWER_LOADSTOREHANDLER_INCL
#define OMR_POWER_LOADSTOREHANDLER_INCL

#include "codegen/InstOpCode.hpp"
#include "codegen/MemoryReference.hpp"
#include "codegen/Register.hpp"
#include "il/Node.hpp"
#include "infra/Annotations.hpp"

namespace OMR { namespace Power {

/**
 * \brief A memory reference alongside information about operations to perform when its lifetime
 *        is complete.
 */
class NodeMemoryReference {
    TR::MemoryReference *_memRef;

public:
    NodeMemoryReference()
        : _memRef(NULL)
    {}

    explicit NodeMemoryReference(TR::MemoryReference *memRef)
        : _memRef(memRef)
    {}

    TR::MemoryReference *getMemoryReference() { return _memRef; }

    void decReferenceCounts(TR::CodeGenerator *cg) { return _memRef->decNodeReferenceCounts(cg); }
};

/**
 * \brief An extensible class containing static methods for evaluating loads and stores.
 *
 * This class contains static methods for assisting evaluators in generating load and store sequences. These methods
 * are meant to be used from evaluators and are capable of some limited customization of how a load or store is
 * performed to allow for codegen optimizations.
 *
 * This class is meant to be extended by downstream projects that require special handling for emitting load and store
 * sequences for certain types of symbol references, e.g. unresolved symbols in OpenJ9.
 *
 * \warning Unless otherwise mentioned, methods on this class are free to emit arbitrary instructions at the append
 *          cursor, and so should never be called during internal control flow.
 */
class OMR_EXTENSIBLE LoadStoreHandler {
public:
    /**
     * \brief Generate a sequence of instructions to compute the effective address of a load, store, or loadaddr.
     *
     * This method generates a sequence of instructions at the append cursor that compute the effective address of a
     * load, store, or loadaddr node. This is useful for evaluators that need to check or change the address of a
     * load/store before it is performed, such as some forms of GC write barrier.
     *
     * For load nodes, the address computed by this method can be used with generateLoadAddressSequence to actually
     * perform the load. For stores, the address computed by this method can be used with generateStoreAddressSequence
     * to actually perform the store.
     *
     * \warning While it is possible to emit a load or store directly using the computed address, this should be done
     *          with extreme caution and only in the most derived project. Downstream projects are free to customize
     *          how load/store sequences are emitted, so this should never be done in projects where overriding
     *          implementations could exist.
     *
     * \param cg The code generator
     * \param addrReg The register into which the effective address should be computed
     * \param node The load, store, or loadaddr node whose effective address should be computed
     * \param extraOffset An extra offset in bytes to add to the effective address
     */
    static void generateComputeAddressSequence(TR::CodeGenerator *cg, TR::Register *addrReg, TR::Node *node,
        int64_t extraOffset = 0);

    /**
     * \brief Generate a sequence of instructions to execute a load whose effective address has already been computed.
     *
     * This method generates a sequence of instructions at the append cursor that perform a load given a load node and
     * the effective address of the load, as computed by generateComputeAddressSequence.
     *
     * \param cg The code generator
     * \param trgReg The register into which the value should be loaded
     * \param node The load node to emit a load for
     * \param addrRef The register containing the effective address of the load
     * \param loadOp The opcode to use to perform the load
     * \param length The length of the data being loaded in bytes
     * \param requireIndexForm true if an indexed-form instruction must be used; false otherwise
     */
    static void generateLoadAddressSequence(TR::CodeGenerator *cg, TR::Register *trgReg, TR::Node *node,
        TR::Register *addrReg, TR::InstOpCode::Mnemonic loadOp, uint32_t length, bool requireIndexForm = false);

    /**
     * \brief Generate a sequence of instructions to execute a paired load whose effective address has already been
     *        computed.
     *
     * This method generates a sequence of instructions at the append cursor that perform a 64-bit load using 32-bit
     * instructions into a register pair given a load node and the effective address of the load, as computed by
     * generateComputeAddressSequence.
     *
     * \param cg The code generator
     * \param trgReg The register pair into which the value should be loaded
     * \param node The load node to emit a load for
     * \param addrReg The register containing the effective address of the load
     */
    static void generatePairedLoadAddressSequence(TR::CodeGenerator *cg, TR::Register *trgReg, TR::Node *node,
        TR::Register *addrReg);

    /**
     * \brief Generate a sequence of instructions to execute a store whose effective address has already been computed.
     *
     * This method generates a sequence of instructions at the append cursor that perform a store given a store node
     * and the effective address of the store, as computed by generateComputeAddressSequence.
     *
     * \param cg The code generator
     * \param srcReg The register whose value should be stored
     * \param node The store node to emit a store for
     * \param addrReg The register containing the effective address of the store
     * \param storeOp The opcode to use to perform the store
     * \param length The length of the data being stored in bytes
     * \param requireIndexForm true if an indexed-form instruction must be used; false otherwise
     */
    static void generateStoreAddressSequence(TR::CodeGenerator *cg, TR::Register *srcReg, TR::Node *node,
        TR::Register *addrReg, TR::InstOpCode::Mnemonic storeOp, uint32_t length, bool requireIndexForm = false);

    /**
     * \brief Generate a sequence of instructions to execute a paired store whose effective address has already been
     *        computed.
     *
     * This method generates a sequence of instructions at the append cursor that perform a 64-bit store using 32-bit
     * instructions from a register pair given a store node and the effective address of the store, as computed
     * by generateComputeAddressSequence.
     *
     * \param cg The code generator
     * \param srcReg The register pair whose value should be stored
     * \param node The store node to emit a store for
     * \param addrReg The register containing the effective address of the store
     */
    static void generatePairedStoreAddressSequence(TR::CodeGenerator *cg, TR::Register *srcReg, TR::Node *node,
        TR::Register *addrReg);

    /**
     * \brief Generate a sequence of instructions to execute a load for a given node.
     *
     * This method generates a sequence of instructions at the append cursor that perform a load at the location
     * specified by the given load node.
     *
     * \param cg The code generator
     * \param trgReg The register into which the value should be loaded
     * \param node The load node to emit a load for
     * \param loadOp The opcode to use to perform the load
     * \param length The length of the data being loaded in bytes
     * \param requireIndexForm true if an indexed-form instruction must be used; false otherwise
     * \param extraOffset An extra offset in bytes to add to the effective address of the load
     */
    static void generateLoadNodeSequence(TR::CodeGenerator *cg, TR::Register *trgReg, TR::Node *node,
        TR::InstOpCode::Mnemonic loadOp, uint32_t length, bool requireIndexForm = false, int64_t extraOffset = 0);

    /**
     * \brief Generate a sequence of instructions to execute a paired load for a given node.
     *
     * This method generates a sequence of instructions at the append cursor that perform a 64-bit load using 32-bit
     * instructions into a register pair at the location specified by the given load node.
     *
     * \param cg The code generator
     * \param trgReg The register pair into which the value should be loaded
     * \param node The load node to emit a load for
     */
    static void generatePairedLoadNodeSequence(TR::CodeGenerator *cg, TR::Register *trgReg, TR::Node *node);

    /**
     * \brief Generate a sequence of instructions to execute a store for a given node.
     *
     * This method generates a sequence of instructions at the append cursor that perform a store at the location
     * specified by the given store node.
     *
     * \param cg The code generator
     * \param srcReg The register whose value should be stored
     * \param node The store node to emit a store for
     * \param storeOp The opcode to use to perform the store
     * \param length The length of the data being stored in bytes
     * \param requireIndexForm true if an indexed-form instruction must be used; false otherwise
     * \param extraOffset An extra offset in bytes to add to the effective address of the store
     */
    static void generateStoreNodeSequence(TR::CodeGenerator *cg, TR::Register *srcReg, TR::Node *node,
        TR::InstOpCode::Mnemonic storeOp, uint32_t length, bool requireIndexForm = false, int64_t extraOffset = 0);

    /**
     * \brief Generate a sequence of instructions to execute a paired store for a given node.
     *
     * This method generates a sequence of instructions at the append cursor that perform a 64-bit store using 32-bit
     * instructions from a register pair at the location specified by the given store node.
     *
     * \param cg The code generator
     * \param srcReg The register pair whose value should be stored
     * \param node The store node to emit a store for
     */
    static void generatePairedStoreNodeSequence(TR::CodeGenerator *cg, TR::Register *srcReg, TR::Node *node);

    /**
     * \brief Determines whether the provided node represents a "simple" load.
     *
     * This method determines whether the provided node represents a "simple" load. Simple loads are those for which it
     * is correct to emit a single load instruction without any other surrounding instructions or other setup. Such
     * loads can, as an optimization, be performed within internal control flow with minimal restrictions.
     *
     * This method is guaranteed not to emit any instructions or change any global state.
     *
     * \param cg The code generator
     * \param node The node to check
     *
     * \return true if the provided node represents a "simple" load; false otherwise
     */
    static bool isSimpleLoad(TR::CodeGenerator *cg, TR::Node *node);

    /**
     * \brief Evaluate the effective address of a "simple" load node into a memory reference
     *
     * This method generates a memory reference for a load which was determined to be simple according to isSimpleLoad.
     * Managing the lifetime of the returned memory reference's registers is left to the caller.
     *
     * \warning While the returned memory reference can be used within internal control flow, calling this method may
     *          emit arbitrary instructions at the append instruction that may not be safe to perform during internal
     *          control flow. Hence, this method should always be called before beginning internal control flow.
     *
     * \param cg The code generator
     * \param node The node representing a "simple" load which should be evaluated to a memory reference
     * \param length The length of data represented by the returned memory reference in bytes
     * \param requireIndexForm true if the returned memory reference must be indexed-form; false otherwise
     * \param extraOffset An extra offset in bytes to add to the effective address of the store
     */
    static NodeMemoryReference generateSimpleLoadMemoryReference(TR::CodeGenerator *cg, TR::Node *node, uint32_t length,
        bool requireIndexForm = false, int64_t extraOffset = 0);
};

/**
 * \brief An extensible class containing implementation methods for evaluating loads and stores.
 *
 * This class is meant to be used as an extension point to customize the default implementations of the methods on
 * LoadStoreHandler provided by OMR without needing to duplicate code between them.
 *
 * \warning This class's methods should only be used within the LoadStoreHandler and LoadStoreHandlerImpl classes.
 *          Downstream projects are free to add extra functionality to LoadStoreHandler that may not work correctly if
 *          LoadStoreHandlerImpl is used directly.
 */
class OMR_EXTENSIBLE LoadStoreHandlerImpl {
public:
    /**
     * \brief Generate a memory reference corresponding to the effective address of a given load, store, or loadaddr
     *        node.
     *
     * \param cg The code generator
     * \param node The load, store, or loadaddr node whose effective address should be computed
     * \param length The length of data represented by the returned memory reference
     * \param requireIndexForm true if the returned memory reference must be indexed-form; false otherwise
     * \param extraOffset An extra offset in bytes to add to the effective address
     */
    static NodeMemoryReference generateMemoryReference(TR::CodeGenerator *cg, TR::Node *node, uint32_t length,
        bool requireIndexForm, int64_t extraOffset);

    /**
     * \brief Generate a sequence of instructions for performing a load given a load node and memory reference.
     *
     * \param cg The code generator
     * \param trgReg The register into which the value should be loaded
     * \param node The load node to emit a load for
     * \param memRef The memory reference for the effective address of the load
     * \param loadOp The opcode to use to perform the load
     */
    static void generateLoadSequence(TR::CodeGenerator *cg, TR::Register *trgReg, TR::Node *node,
        TR::MemoryReference *memRef, TR::InstOpCode::Mnemonic loadOp);

    /**
     * \brief Generate a sequence of instructions for performing a paired load given a load node and memory reference.
     *
     * \param cg The code generator
     * \param trgReg The register pair into which the value should be loaded
     * \param node The load node to emit a load for
     * \param memRef The memory reference for the effective address of the load
     */
    static void generatePairedLoadSequence(TR::CodeGenerator *cg, TR::Register *trgReg, TR::Node *node,
        TR::MemoryReference *memRef);

    /**
     * \brief Generate a sequence of instructions for performing a store given a store node and memory reference.
     *
     * \param cg The code generator
     * \param srcReg The register whose value should be stored
     * \param node The store node to emit a store for
     * \param memRef The memory reference for the effective address of the store
     * \param storeOp The opcode to use to perform the store
     */
    static void generateStoreSequence(TR::CodeGenerator *cg, TR::Register *srcReg, TR::Node *node,
        TR::MemoryReference *memRef, TR::InstOpCode::Mnemonic storeOp);

    /**
     * \brief Generate a sequence of instructions for performing a paired store given a store node and memory
     *        reference.
     *
     * \param cg The code generator
     * \param srcReg The register pair whose value should be stored
     * \param node The store node to emit a store for
     * \param memRef The memory reference for the effective address of the store
     */
    static void generatePairedStoreSequence(TR::CodeGenerator *cg, TR::Register *srcReg, TR::Node *node,
        TR::MemoryReference *memRef);
};
}} // namespace OMR::Power

#endif
