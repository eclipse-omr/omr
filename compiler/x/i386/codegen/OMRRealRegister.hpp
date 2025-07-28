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

#ifndef OMR_X86_I386_REAL_REGISTER_INCL
#define OMR_X86_I386_REAL_REGISTER_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef OMR_REAL_REGISTER_CONNECTOR
#define OMR_REAL_REGISTER_CONNECTOR
namespace OMR {
namespace X86 { namespace I386 { class RealRegister; } }
typedef OMR::X86::I386::RealRegister RealRegisterConnector;
}
#else
#error OMR::X86::I386::RealRegister expected to be a primary connector, but a OMR connector is already defined
#endif

#include "x/codegen/OMRRealRegister.hpp"

#include <stdint.h>
#include "codegen/RegisterConstants.hpp"

namespace TR {
class CodeGenerator;
}

namespace OMR
{

namespace X86
{

namespace I386
{

class OMR_EXTENSIBLE RealRegister : public OMR::X86::RealRegister
   {
   protected:

   RealRegister(TR::CodeGenerator *cg) : OMR::X86::RealRegister(cg) {}

   RealRegister(TR_RegisterKinds   rk,
                uint16_t           w,
                RegState           s,
                RegNum             ri,
                RegMask            m,
                TR::CodeGenerator *cg) :
      OMR::X86::RealRegister(rk, w, s, ri, m, cg) {}

   public:

   static RegNum xmmIndex(uint8_t r)
      {
      switch(r)
         {
         case 0:
            return OMR::RealRegister::xmm0;
         case 1:
            return OMR::RealRegister::xmm1;
         case 2:
            return OMR::RealRegister::xmm2;
         case 3:
            return OMR::RealRegister::xmm3;
         case 4:
            return OMR::RealRegister::xmm4;
         case 5:
            return OMR::RealRegister::xmm5;
         case 6:
            return OMR::RealRegister::xmm6;
         case 7:
            return OMR::RealRegister::xmm7;
         default:
            TR_ASSERT(false, "xmmIndex is only valid for registers xmm0 to xmm7");
            return OMR::RealRegister::NoReg;
         }
      }

   static RegMask gprMask(RegNum idx)
      {
      switch(idx)
         {
         case OMR::RealRegister::NoReg:
            return OMR::RealRegister::noRegMask;
         case OMR::RealRegister::eax:
            return OMR::RealRegister::eaxMask;
         case OMR::RealRegister::ebx:
            return OMR::RealRegister::ebxMask;
         case OMR::RealRegister::ecx:
            return OMR::RealRegister::ecxMask;
         case OMR::RealRegister::edx:
            return OMR::RealRegister::edxMask;
         case OMR::RealRegister::edi:
            return OMR::RealRegister::ediMask;
         case OMR::RealRegister::esi:
            return OMR::RealRegister::esiMask;
         case OMR::RealRegister::ebp:
            return OMR::RealRegister::ebpMask;
         case OMR::RealRegister::esp:
            return OMR::RealRegister::espMask;
         default:
            TR_ASSERT(false, "gprMask is only valid for registers eax to esp");
            return OMR::RealRegister::noRegMask;
         }
      }

   static RegMask fprMask(RegNum idx)
      {
      switch(idx)
         {
         case OMR::RealRegister::NoReg:
            return OMR::RealRegister::noRegMask;
         case OMR::RealRegister::st0:
            return OMR::RealRegister::st0Mask;
         case OMR::RealRegister::st1:
            return OMR::RealRegister::st1Mask;
         case OMR::RealRegister::st2:
            return OMR::RealRegister::st2Mask;
         case OMR::RealRegister::st3:
            return OMR::RealRegister::st3Mask;
         case OMR::RealRegister::st4:
            return OMR::RealRegister::st4Mask;
         case OMR::RealRegister::st5:
            return OMR::RealRegister::st5Mask;
         case OMR::RealRegister::st6:
            return OMR::RealRegister::st6Mask;
         case OMR::RealRegister::st7:
            return OMR::RealRegister::st7Mask;
         default:
            TR_ASSERT(false, "fprMask is only valid for registers st0 to st7");
            return OMR::RealRegister::noRegMask;
         }
      }

   static RegMask vectorMaskMask(RegNum idx)
      {
      switch (idx)
         {
         case OMR::RealRegister::NoReg:
            return OMR::RealRegister::noRegMask;
         case OMR::RealRegister::k0:
            return OMR::RealRegister::k0Mask;
         case OMR::RealRegister::k1:
            return OMR::RealRegister::k1Mask;
         case OMR::RealRegister::k2:
            return OMR::RealRegister::k2Mask;
         case OMR::RealRegister::k3:
            return OMR::RealRegister::k3Mask;
         case OMR::RealRegister::k4:
            return OMR::RealRegister::k4Mask;
         case OMR::RealRegister::k5:
            return OMR::RealRegister::k5Mask;
         case OMR::RealRegister::k6:
            return OMR::RealRegister::k6Mask;
         case OMR::RealRegister::k7:
            return OMR::RealRegister::k7Mask;
         default:
            TR_ASSERT_FATAL(0, "vector mask mask valid for k0-k7 only");
            return OMR::RealRegister::noRegMask;
         }
      }

   static RegMask xmmrMask(RegNum idx)
      {
      switch(idx)
         {
         case OMR::RealRegister::NoReg:
            return OMR::RealRegister::noRegMask;
         case OMR::RealRegister::xmm0:
            return OMR::RealRegister::xmm0Mask;
         case OMR::RealRegister::xmm1:
            return OMR::RealRegister::xmm1Mask;
         case OMR::RealRegister::xmm2:
            return OMR::RealRegister::xmm2Mask;
         case OMR::RealRegister::xmm3:
            return OMR::RealRegister::xmm3Mask;
         case OMR::RealRegister::xmm4:
            return OMR::RealRegister::xmm4Mask;
         case OMR::RealRegister::xmm5:
            return OMR::RealRegister::xmm5Mask;
         case OMR::RealRegister::xmm6:
            return OMR::RealRegister::xmm6Mask;
         case OMR::RealRegister::xmm7:
            return OMR::RealRegister::xmm7Mask;
         default:
            TR_ASSERT(false, "xmmrMask is only valid for registers xmm0 to xmm7");
            return OMR::RealRegister::noRegMask;
         }
      }

   void setRegisterNumber() { TR_ASSERT(0, "X86 RealRegister doesn't have setRegisterNumber() implementation"); }

   void setRegisterFieldInOpcode(uint8_t *opcodeByte)
      {
      *opcodeByte |= _fullRegisterBinaryEncodings[_registerNumber].id; // reg field is in bits 0-2 of opcode
      }

   void setMaskRegisterInEvex(uint8_t *evex, bool zero = false)
      {
      uint8_t regNum = getRegisterNumber() - OMR::RealRegister::k0;
      *evex = (*evex & 0xf8) | (0x7 & regNum) | (zero ? 0x80 : 0);
      }

   void setSourceRegisterFieldInEVEX(uint8_t *evexP0)
      {
      uint8_t regNum = ((_fullRegisterBinaryEncodings[_registerNumber].needsRexPlusRXB << 3) | _fullRegisterBinaryEncodings[_registerNumber].id);
      uint8_t bits = 0;
      *evexP0 &= 0x9F;

      if (regNum & 0x10)
         {
         bits |= 0x4;
         }

      if (regNum & 0x8)
         {
         bits |= 0x2;
         }

      *evexP0 |= (~bits & 0x6) << 4;
      }

   void setSource2ndRegisterFieldInEVEX(uint8_t *evexP1)
      {
      uint8_t regNum = ((_fullRegisterBinaryEncodings[_registerNumber].needsRexPlusRXB << 3) | _fullRegisterBinaryEncodings[_registerNumber].id);

      *evexP1 &= 0x87; // zero out vvvv bits
      *evexP1 |= (~(regNum << 3)) & 0x78;
      uint8_t *evexP2 = evexP1 + 1;
      *evexP2 &= 0xf7;

      if (!(regNum & 0x10))
         {
         *evexP2 |= 0x8;
         }
      }

   void setTargetRegisterFieldInEVEX(uint8_t *evexP0)
      {
      uint8_t regNum = ((_fullRegisterBinaryEncodings[_registerNumber].needsRexPlusRXB << 3) | _fullRegisterBinaryEncodings[_registerNumber].id);
      uint8_t bits = 0;
      *evexP0 &= 0x6F;

      if (regNum & 0x10)
         {
         bits |= 0x1;
         }

      if (regNum & 0x8)
         {
         bits |= 0x8;
         }

      *evexP0 |= (~bits & 0x9) << 4;
      }

   /** \brief
   *    Fill vvvv field in a VEX prefix
   *
   *  \param opcodeByte
   *    The address of VEX prefix byte containing vvvv field
   */
   void setRegisterFieldInVEX(uint8_t *opcodeByte)
      {
      *opcodeByte ^= ((_fullRegisterBinaryEncodings[_registerNumber].needsRexPlusRXB << 3) | _fullRegisterBinaryEncodings[_registerNumber].id) << 3; // vvvv is in bits 3-6 of last byte of VEX
      }

   void setRegisterFieldInModRM(uint8_t *modRMByte)
      {
      *modRMByte |= _fullRegisterBinaryEncodings[_registerNumber].id << 3; // reg field is in bits 3-5 of ModRM byte
      }

   void setRMRegisterFieldInModRM(uint8_t *modRMByte)
      {
      *modRMByte |= _fullRegisterBinaryEncodings[_registerNumber].id; // RM field is in bits 0-2 of ModRM byte
      }

   void setBaseRegisterFieldInSIB(uint8_t *SIBByte)
      {
      *SIBByte |= _fullRegisterBinaryEncodings[_registerNumber].id; // base register field is in bits 0-2 of SIB byte
      }

   void setIndexRegisterFieldInSIB(uint8_t *SIBByte)
      {
      *SIBByte |= _fullRegisterBinaryEncodings[_registerNumber].id << 3; // index register field is in bits 3-5 SIB byte
      }

   uint8_t rexBits(uint8_t rxbBits, bool isByteOperand) { return 0; }

   // (AMD64: see x86-64 Architecture Programmer's Manual, Volume 3, section 1.7.2.
   bool needsDisp() { return _fullRegisterBinaryEncodings[_registerNumber].needsDisp; }
   bool needsSIB()  { return _fullRegisterBinaryEncodings[_registerNumber].needsSIB;  }

   private:

   // TODO: Consider making this back into a plain old byte for consistency with other platforms.
   static const struct TR_RegisterBinaryEncoding _fullRegisterBinaryEncodings[NumRegisters];
   };

}

}

}

#endif
