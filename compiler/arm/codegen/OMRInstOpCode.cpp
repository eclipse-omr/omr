/*******************************************************************************
 * Copyright IBM Corp. and others 2019
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

#include "codegen/OMRInstOpCode.hpp"

const OMR::ARM::InstOpCode::OpCodeMetaData OMR::ARM::InstOpCode::metadata[NumOpCodes] = {
#include "codegen/OMRInstOpCodeProperties.hpp"
};

const OMR::ARM::InstOpCode::TR_OpCodeBinaryEntry OMR::ARM::InstOpCode::binaryEncodings[NumOpCodes] = {
    0x00000000, // assocreg
    0xE6000010, // bad
    0x00000000, // dd
    0x00000000, // fence
    0x00000000, // label
    0x00000000, // proc
    0x00000000, // retn
    0x00000000, // vgnop
    0x00800000, // add
    0x00900000, // add_r
    0x00A00000, // adc
    0x00B00000, // adc_r
    0x00000000, // and
    0x00100000, // and_r
    0x0A000000, // b
    0x0B000000, // bl
    0x01C00000, // bic
    0x01D00000, // bic_r
    0x01500000, // cmp
    0x01700000, // cmn
    0x00200000, // eor
    0x00300000, // eor_r
    0x0D100100, // ldfs
    0x0D108100, // ldfd
    0x08900000, // ldm
    0x08b00000, // ldmia
    0x05100000, // ldr
    0x05500000, // ldrb
    0x011000D0, // ldrsb
    0x011000B0, // ldrh
    0x011000F0, // ldrsh
    0x00000090, // mul
    0x00100090, // mul_r
    0x00200090, // mla
    0x00300090, // mla_r
    0x00C00090, // smull
    0x00D00090, // smull_r
    0x00800090, // umull
    0x00900090, // umull_r
    0x00E00090, // smlal
    0x00F00090, // smlal_r
    0x00A00090, // umlal
    0x00B00090, // umlal_r
    0x01A00000, // mov
    0x01B00000, // mov_r
    0x01E00000, // mvn
    0x01F00000, // mvn_r
    0x01800000, // orr
    0x01900000, // orr_r
    0x01200000, // teq
    0x01100000, // tst
    0x00400000, // sub
    0x00500000, // sub_r
    0x00C00000, // sbc
    0x00D00000, // sbc_r
    0x00600000, // rsb
    0x00700000, // rsb_r
    0x00E00000, // rsc
    0x00F00000, // rsc_r
    0x0D000100, // stfs
    0x0D008100, // stfd
    0x05000000, // str
    0x05400000, // strb
    0x010000B0, // strh
    0x08800000, // stm
    0x09200000, // stmdb
    0x01000090, // swp
    0x06AF0070, // sxtb
    0x06BF0070, // sxth
    0x06EF0070, // uxtb
    0x06FF0070, // uxth
    0x00000000, // wrtbar
    0x0E070FBA, // dmb_v6
    0xF57FF05F, // dmb
    0xF57FF05E, // dmb_st
    0x01900F9F, // ldrex
    0x01800F90, // strex
    0x00000000, // iflong
    0x00000000, // setblong
    0x00000000, // setbool
    0x00000000, // setbflt
    0x00000000, // lcmp
    0x00000000, // flcmpl
    0x00000000, // flcmpg
    0x00000000, // idiv
    0x00000000, // irem
    0xE1A00000, // nop (mov r0, r0)
    0x0EB00BC0, // fabsd (vabs<c>.f64)
    0x0EB00AC0, // fabss (vabs<c>.f32)
    0x0E300B00, // faddd (vadd<c>.f64)
    0x0E300A00, // fadds (vadd<c>.f32)
    0x0EB40B40, // fcmpd (vcmp<c>.f64)
    0x0EB40A40, // fcmps (vcmp<c>.f32)
    0x0EB50B40, // fcmpzd (vcmp<c>.f64)
    0x0EB50A40, // fcmpzs (vcmp<c>.f32)
    0x0EB00B40, // fcpyd (vmov<c>.f64)
    0x0EB00A40, // fcpys (vmov<c>.f32)
    0x0EB70AC0, // fcvtds (vcvt<c>.f64.f32)
    0x0EB70BC0, // fcvtsd (vcvt<c>.f32.f64)
    0x0E800B00, // fdivd (vdiv<c>.f64)
    0x0E800A00, // fdivs (vdiv<c>.f32)
    0x0D100B00, // fldd  (vldr)
    0x0C100B00, // fldmd (vldm)
    0x0C100A00, // fldms (vldm)
    0x0D100A00, // flds (vldr)
    0x0E000B00, // fmacd (vmla.f64)
    0x0E000A00, // fmacs (vmla.f32)
    0x0C400B10, // fmdrr (vmov)
    0x0C500B10, // fmrrd (vmov)
    0x0C500A10, // fmrrs (vmov)
    0x0E100A10, // fmrs (vmov)
    0x0EF00A10, // fmrx (vmrs)
    0x0E100B00, // fmscd (vmls.f64)
    0x0E100A00, // fmscs (vmls.f32)
    0x0E000A10, // fmsr (vmov)
    0x0C400A10, // fmsrr (vmov)
    0x0EF1FA10, // fmstat (vmrs APSR_nzcv, FPSCR)
    0x0E200B00, // fmuld (vmul.f64)
    0x0E200A00, // fmuls (vmul.f32)
    0x0EE00A10, // fmxr (vmsr)
    0x0EB10B40, // fnegd (vneg.f64)
    0x0EB10A40, // fnegs (vneg.f32)
    0x0E000B40, // fnmacd (vnmla.f64)
    0x0E000A40, // fnmacs (vnmla.f32)
    0x0E100B40, // fnmscd (vnmls.f64)
    0x0E100A40, // fnmscs (vnmls.f32)
    0x0E200B40, // fnmuld (vnmul.f64)
    0x0E200A40, // fnmuls (vnmul.f32)
    0x0EB80BC0, // fsitod (vcvt.f64.s32)
    0x0EB80AC0, // fsitos (vcvt.f32.s32)
    0x0EB10BC0, // fsqrtd (vsqrt.f64)
    0x0EB10AC0, // fsqrts (vsqrt.f32)
    0x0D000B00, // fstd (vstr)
    0x0C000B00, // fstmd (vstm)
    0x0C000A00, // fstms (vstm)
    0x0D000A00, // fsts (vstr)
    0x0E300B40, // fsubd (vsub.f64)
    0x0E300A40, // fsubs (vsub.f32)
    0x0EBD0BC0, // ftosizd (vcvt.s32.f64)
    0x0EBD0AC0, // ftosizs (vcvt.s32.f32)
    0x0EBC0BC0, // ftouizd (vcvt.u32.f64)
    0x0EBC0AC0, // ftouizs (vcvt.u32.f32)
    0x0EB80B40, // fuitod (vcvt.f64,u32)
    0x0EB80A40 // fuitos (vcvt.f32.u32)
};

const uint32_t OMR::ARM::InstOpCode::properties[NumOpCodes] = {
    // assocreg
    0,

    // bad
    0,

    // dd
    0,

    // fence
    0,

    // label
    ARMOpProp_Label,

    // proc
    0,

    // retn
    0,

    // vgnop
    0,

    // add
    ARMOpProp_HasRecordForm,

    // add_r
    ARMOpProp_IsRecordForm,

    // adc
    ARMOpProp_HasRecordForm,

    // adc_r
    ARMOpProp_IsRecordForm,

    // and
    ARMOpProp_HasRecordForm,

    // and_r
    ARMOpProp_IsRecordForm,

    // b
    ARMOpProp_BranchOp,

    // bl
    ARMOpProp_BranchOp,

    // bic
    ARMOpProp_HasRecordForm,

    // bic_r
    ARMOpProp_IsRecordForm,

    // cmp
    ARMOpProp_IsRecordForm,

    // cmn
    ARMOpProp_IsRecordForm,

    // eor
    ARMOpProp_HasRecordForm,

    // eor_r
    ARMOpProp_IsRecordForm,

    // ldfs
    0,

    // ldfd
    0,

    // ldm
    0,

    // ldmia
    0,

    // ldr
    0,

    // ldrb
    0,

    // ldrsb
    ARMOpProp_Arch4,

    // ldrh
    ARMOpProp_Arch4,

    // ldrsh
    ARMOpProp_Arch4,

    // mul
    ARMOpProp_HasRecordForm,

    // mul_r
    ARMOpProp_IsRecordForm,

    // mla
    ARMOpProp_HasRecordForm,

    // mla_r
    ARMOpProp_IsRecordForm,

    // smull
    ARMOpProp_HasRecordForm,

    // smull_r
    ARMOpProp_IsRecordForm,

    // umull
    ARMOpProp_HasRecordForm,

    // umull_r
    ARMOpProp_IsRecordForm,

    // smlal
    ARMOpProp_HasRecordForm,

    // smlal_r
    ARMOpProp_IsRecordForm,

    // umlal
    ARMOpProp_HasRecordForm,

    // umlal_r
    ARMOpProp_IsRecordForm,

    // mov
    ARMOpProp_HasRecordForm,

    // mov_r
    ARMOpProp_IsRecordForm,

    // mvn
    ARMOpProp_HasRecordForm,

    // mvn_r
    ARMOpProp_IsRecordForm,

    // orr
    ARMOpProp_HasRecordForm,

    // orr_r
    ARMOpProp_IsRecordForm,

    // teq
    ARMOpProp_IsRecordForm,

    // tst
    ARMOpProp_IsRecordForm,

    // sub
    ARMOpProp_HasRecordForm,

    // sub_r
    ARMOpProp_IsRecordForm,

    // sbc
    ARMOpProp_HasRecordForm,

    // sbc_r
    ARMOpProp_IsRecordForm,

    // rsb
    ARMOpProp_HasRecordForm,

    // rsb_r
    ARMOpProp_IsRecordForm,

    // rsc
    ARMOpProp_HasRecordForm,

    // rsc_r
    ARMOpProp_IsRecordForm,

    // stfs
    0,

    // stfd
    0,

    // str
    0,

    // strb
    0,

    // strh
    ARMOpProp_Arch4,

    // stm
    0,

    // stmdb
    0,

    // swp
    0,

    // sxtb
    0,

    // sxth
    0,

    // uxtb
    0,

    // uxth
    0,

    // wrtbar
    0,

    // dmb_v6
    0,

    // dmb
    0,

    // dmb_st
    0,

    // ldrex
    0,

    // strex
    0,

    // iflong
    0,

    // setblong
    0,

    // setbool
    0,

    // setbflt
    0,

    // lcmp
    0,

    // flcmpl
    0,

    // flcmpg
    0,

    // idiv
    0,

    // irem
    0,

    // nop
    0,

    // fabsd (vabs<c>.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fabss (vabs<c>.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // faddd (vadd<c>.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fadds (vadd<c>.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fcmpd (vcmp<c>.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fcmps (vcmp<c>.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fcmpzd (vcmp<c>.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fcmpzs (vcmp<c>.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fcpyd (vmov<c>.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fcpys (vmov<c>.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fcvtds (vcvt<c>.f64.f32)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fcvtsd (vcvt<c>.f32.f64)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fdivd (vdiv<c>.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fdivs (vdiv<c>.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fldd  (vldr)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fldmd (vldm)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fldms (vldm)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // flds (vldr)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fmacd (vmla.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fmacs (vmla.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fmdrr (vmov)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fmrrd (vmov)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fmrrs (vmov)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fmrs (vmov)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fmrx (vmrs)
    ARMOpProp_VFP,

    // fmscd (vmls.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fmscs (vmls.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fmsr (vmov)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fmsrr (vmov)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fmstat (vmrs APSR_nzcv, FPSCR)
    ARMOpProp_IsRecordForm,

    // fmuld (vmul.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fmuls (vmul.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fmxr (vmsr)
    ARMOpProp_VFP,

    // fnegd (vneg.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fnegs (vneg.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fnmacd (vnmla.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fnmacs (vnmla.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fnmscd (vnmls.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fnmscs (vnmls.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fnmuld (vnmul.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fnmuls (vnmul.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fsitod (vcvt.f64.s32)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fsitos (vcvt.f32.s32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fsqrtd (vsqrt.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fsqrts (vsqrt.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fstd (vstr)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fstmd (vstm)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fstms (vstm)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fsts (vstr)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // fsubd (vsub.f64)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fsubs (vsub.f32)
    ARMOpProp_SingleFP | ARMOpProp_VFP,

    // ftosizd (vcvt.s32.f64)
    ARMOpProp_VFP,

    // ftosizs (vcvt.s32.f32)
    ARMOpProp_VFP,

    // ftouizd (vcvt.u32.f64)
    ARMOpProp_VFP,

    // ftouizs (vcvt.u32.f32)
    ARMOpProp_VFP,

    // fuitod (vcvt.f64,u32)
    ARMOpProp_DoubleFP | ARMOpProp_VFP,

    // fuitos (vcvt.f32.u32)
    ARMOpProp_SingleFP | ARMOpProp_VFP

};
