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
 ******************************************************************************/

#include "il/symbol/OMRSymbol.hpp"

#include <stddef.h>                            // for size_t
#include <stdint.h>                            // for uint32_t, int32_t, etc
#include <string.h>                            // for memcmp, strncmp
#include "codegen/FrontEnd.hpp"                // for TR_FrontEnd
#include "compile/Compilation.hpp"             // for Compilation
#include "compile/ResolvedMethod.hpp"          // for TR_ResolvedMethod
#include "compile/SymbolReferenceTable.hpp"    // for SymbolReferenceTable
#include "env/KnownObjectTable.hpp"            // for KnownObjectTable, etc
#include "env/ObjectModel.hpp"                 // for ObjectModel
#include "env/TRMemory.hpp"
#include "il/DataTypes.hpp"                    // for DataType
#include "il/Symbol.hpp"                       // for Symbol
#include "il/SymbolReference.hpp"              // for SymbolReference
#include "il/symbol/AutomaticSymbol.hpp"       // for AutomaticSymbol
#include "il/symbol/RegisterMappedSymbol.hpp"  // for RegisterMappedSymbol
#include "il/symbol/StaticSymbol.hpp"          // for StaticSymbol
#include "infra/Assert.hpp"                    // for TR_ASSERT
#include "infra/Flags.hpp"                     // for flags32_t
#include "ras/Debug.hpp"                       // for TR_DebugBase

template <typename AllocatorType>
TR::Symbol * OMR::Symbol::create(AllocatorType m)
   {
   return new (m) TR::Symbol();
   }

template <typename AllocatorType>
TR::Symbol * OMR::Symbol::create(AllocatorType m, TR::DataType d)
   {
   return new (m) TR::Symbol(d);
   }

template <typename AllocatorType>
TR::Symbol * OMR::Symbol::create(AllocatorType m, TR::DataType d, uint32_t s)
   {
   return new (m) TR::Symbol(d,s);
   }

OMR::Symbol::Symbol(TR::DataType d) :
   _size(0),
   _name(0),
   _flags(0),
   _flags2(0),
   _localIndex(0),
   _restrictedRegisterNumber(-1)
   {
   self()->setDataType(d);
   }

OMR::Symbol::Symbol(TR::DataType d, uint32_t size) :
   _name(0),
   _flags(0),
   _flags2(0),
   _localIndex(0),
   _restrictedRegisterNumber(-1)
   {
   self()->setDataType(d);
   _size = size;
   }

bool
OMR::Symbol::isReferenced()
   {
   return self()->isVariableSizeSymbol() && self()->castToVariableSizeSymbol()->isReferenced();
   }

bool
OMR::Symbol::dontEliminateStores(TR::Compilation *comp, bool isForLocalDeadStore)
   {
   return (self()->isAuto() && _flags.testAny(PinningArrayPointer)) ||
          (self()->isParm() && _flags.testAny(ReinstatedReceiver)) ||
          _flags.testAny(HoldsMonitoredObject) ||
          (comp->getSymRefTab()->findThisRangeExtensionSymRef() && (self() == comp->getSymRefTab()->findThisRangeExtensionSymRef()->getSymbol()));
   }

uint32_t
OMR::Symbol::getNumberOfSlots()
   {
   uint32_t numSlots = self()->getRoundedSize()/self()->convertTypeToSize(TR::Address);

   // We should always give at least 1 slot.
   //  This is specifically for the case of an int type on 64bit pltfrms
   return (numSlots? numSlots : 1);
   }

TR::DataType
OMR::Symbol::convertSigCharToType(char sigChar)
   {
   switch (sigChar)
      {
      case 'Z': return TR::Int8;
      case 'B': return TR::Int8;
      case 'C': return TR::Int16;
      case 'S': return TR::Int16;
      case 'I': return TR::Int32;
      case 'J': return TR::Int64;
      case 'F': return TR::Float;
      case 'D': return TR::Double;
      case 'L':
      case '[': return TR::Address;
      }
   TR_ASSERT(0, "unknown signature character");
   return TR::Int32;
   }

/**
 * Sets the data type of a symbol, and the size, if the size can be inferred
 * from the data type.
 */
void
OMR::Symbol::setDataType(TR::DataType dt)
   {
   uint32_t inferredSize = TR::DataType::getSize(dt);
   if (inferredSize)
      {
      _size = inferredSize;
      }
   _flags.setValue(DataTypeMask, dt);
   }

uint32_t
OMR::Symbol::getRoundedSize()
   {
   int32_t roundedSize = (int32_t)((self()->getSize()+3)&(~3)); // cast explicitly
   return roundedSize ? roundedSize : 4;
   }

int32_t
OMR::Symbol::getOffset()
   {
   TR::RegisterMappedSymbol * r = self()->getRegisterMappedSymbol();
   return r ? r->getOffset() : 0;
   }

/*
 * Static factory methods
 */
template <typename AllocatorType>
TR::Symbol * OMR::Symbol::createShadow(AllocatorType m)
   {
   TR::Symbol * sym = new (m) TR::Symbol();
   sym->_flags.setValue(KindMask, IsShadow);
   return sym;
   }

template <typename AllocatorType>
TR::Symbol * OMR::Symbol::createShadow(AllocatorType m, TR::DataType d)
   {
   TR::Symbol * sym = new (m) TR::Symbol(d);
   sym->_flags.setValue(KindMask, IsShadow);
   return sym;
   }

template <typename AllocatorType>
TR::Symbol * OMR::Symbol::createShadow(AllocatorType m, TR::DataType d, uint32_t s)
   {
   TR::Symbol * sym = new (m) TR::Symbol(d,s);
   sym->_flags.setValue(KindMask, IsShadow);
   return sym;
   }

template <typename AllocatorType>
TR::Symbol * OMR::Symbol::createNamedShadow(AllocatorType m, TR::DataType d, uint32_t s, char *name)
   {
   auto * sym = createShadow(m,d,s);
   sym->_name = name;
   sym->_flags2.set(NamedShadow);
   return sym;
   }

/*
 * Explicit instantiation of factories for each TR_Memory type.
 */

template TR::Symbol * OMR::Symbol::create(TR_StackMemory);
template TR::Symbol * OMR::Symbol::create(TR_HeapMemory);
template TR::Symbol * OMR::Symbol::create(PERSISTENT_NEW_DECLARE);

template TR::Symbol * OMR::Symbol::create(TR_StackMemory, TR::DataType);
template TR::Symbol * OMR::Symbol::create(TR_HeapMemory, TR::DataType);
template TR::Symbol * OMR::Symbol::create(PERSISTENT_NEW_DECLARE, TR::DataType);

template TR::Symbol * OMR::Symbol::create(TR_StackMemory, TR::DataType, uint32_t);
template TR::Symbol * OMR::Symbol::create(TR_HeapMemory, TR::DataType, uint32_t);
template TR::Symbol * OMR::Symbol::create(PERSISTENT_NEW_DECLARE, TR::DataType, uint32_t);

template TR::Symbol * OMR::Symbol::createShadow(TR_StackMemory);
template TR::Symbol * OMR::Symbol::createShadow(TR_HeapMemory);
template TR::Symbol * OMR::Symbol::createShadow(PERSISTENT_NEW_DECLARE);

template TR::Symbol * OMR::Symbol::createShadow(TR_StackMemory,         TR::DataType);
template TR::Symbol * OMR::Symbol::createShadow(TR_HeapMemory,          TR::DataType);
template TR::Symbol * OMR::Symbol::createShadow(PERSISTENT_NEW_DECLARE, TR::DataType);

template TR::Symbol * OMR::Symbol::createShadow(TR_StackMemory,         TR::DataType, uint32_t);
template TR::Symbol * OMR::Symbol::createShadow(TR_HeapMemory,          TR::DataType, uint32_t);
template TR::Symbol * OMR::Symbol::createShadow(PERSISTENT_NEW_DECLARE, TR::DataType, uint32_t);

template TR::Symbol * OMR::Symbol::createNamedShadow(TR_StackMemory,         TR::DataType, uint32_t, char *);
template TR::Symbol * OMR::Symbol::createNamedShadow(TR_HeapMemory,          TR::DataType, uint32_t, char *);
template TR::Symbol * OMR::Symbol::createNamedShadow(PERSISTENT_NEW_DECLARE, TR::DataType, uint32_t, char *);
