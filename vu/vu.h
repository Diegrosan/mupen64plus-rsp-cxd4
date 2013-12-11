/*
 * mupen64plus-rsp-cxd4 - RSP Interpreter
 * Copyright (C) 2012-2013  RJ 'Iconoclast' Swedlow
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/******************************************************************************\
* Project:  MSP Emulation Layer for Vector Unit Computational Operations       *
* Authors:  Iconoclast                                                         *
* Release:  2013.09.22                                                         *
* License:  none (public domain)                                               *
\******************************************************************************/
#ifndef _VU_H
#define _VU_H

/*
 * `memcpy` as the intrinsic for parallel vector accumulator write-back
 */
#include <memory.h>
#include <string.h>

/*
 * vector-scalar element decoding
 * Obsolete.  Consider using at least the SSE2 algorithms instead.
 */
static const int ei[16][8] = {
    { 00, 01, 02, 03, 04, 05, 06, 07 }, /* none (vector-only operand) */
    { 00, 01, 02, 03, 04, 05, 06, 07 },
    { 00, 00, 02, 02, 04, 04, 06, 06 }, /* 0Q */
    { 01, 01, 03, 03, 05, 05, 07, 07 }, /* 1Q */
    { 00, 00, 00, 00, 04, 04, 04, 04 }, /* 0H */
    { 01, 01, 01, 01, 05, 05, 05, 05 }, /* 1H */
    { 02, 02, 02, 02, 06, 06, 06, 06 }, /* 2H */
    { 03, 03, 03, 03, 07, 07, 07, 07 }, /* 3H */
    { 00, 00, 00, 00, 00, 00, 00, 00 }, /* 0 */
    { 01, 01, 01, 01, 01, 01, 01, 01 }, /* 1 */
    { 02, 02, 02, 02, 02, 02, 02, 02 }, /* 2 */
    { 03, 03, 03, 03, 03, 03, 03, 03 }, /* 3 */
    { 04, 04, 04, 04, 04, 04, 04, 04 }, /* 4 */
    { 05, 05, 05, 05, 05, 05, 05, 05 }, /* 5 */
    { 06, 06, 06, 06, 06, 06, 06, 06 }, /* 6 */
    { 07, 07, 07, 07, 07, 07, 07, 07 }  /* 7 */
};

#define N      8
/* N:  number of processor elements in SIMD processor */

/*
 * RSP virtual registers (of vector unit)
 * The most important are the 32 general-purpose vector registers.
 * The correct way to accurately store these is using big-endian vectors.
 *
 * For ?WC2 we may need to do byte-precision access just as directly.
 * This is amended by using the `VU_S` and `VU_B` macros defined in `rsp.h`.
 */
ALIGNED short VR[32][N];
ALIGNED short ST[N]; /* shuffled scalar-decoded target paired source */

/*
 * Un-define this if your environment lacks the SSE2 instruction set.
 */
#define ARCH_MIN_SSE2

#ifdef ARCH_MIN_SSE2
#include "shuffle.h"
#else
int sub_mask[16] = {
    0x0,
    0x0,
    0x1, 0x1,
    0x3, 0x3, 0x3, 0x3,
    0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7, 0x7
};

INLINE static void SHUFFLE_VECTOR(short* VD, short* VT, const int e)
{
    short SV[8];
    register int i, j;
#if (0 == 0)
    j = sub_mask[e];
    e &= j;
    j ^= 07;
    for (i = 0; i < N; i++)
        SV[i] = VT[(i & j) | e];
#else
    if (e & 0x8)
        for (i = 0; i < N; i++)
            SV[i] = VT[(i & 0x0) | (e & 0x7)];
    else if (e & 0x4)
        for (i = 0; i < N; i++)
            SV[i] = VT[(i & 0xC) | (e & 0x3)];
    else if (e & 0x2)
        for (i = 0; i < N; i++)
            SV[i] = VT[(i & 0xE) | (e & 0x1)];
    else /* if ((e == 0b0000) || (e == 0b0001)) */
        for (i = 0; i < N; i++)
            SV[i] = VT[(i & 0x7) | (e & 0x0)];
#endif
    for (i = 0; i < N; i++)
        *(VD + i) = *(SV + i);
    return;
}
#endif

/*
 * accumulator-indexing macros (inverted access dimensions, suited for SSE)
 */
#define HI      00
#define MD      01
#define LO      02

ALIGNED static short VACC[3][N];

#define VACC_L      (VACC[LO])
#define VACC_M      (VACC[MD])
#define VACC_H      (VACC[HI])

#define ACC_L(i)    (VACC_L[i])
#define ACC_M(i)    (VACC_M[i])
#define ACC_H(i)    (VACC_H[i])

/*
 * modes of saturation (unofficial labels, just made up by file author)
 */
enum {
    SM_MUL_X, /* clamp acc. bits 31:16, crossing through zero (0x8000:0x7FFF) */
    SM_MUL_Z, /* clamp acc. bits 15:0 with zero-extension (0x0000:0xFFFF) */
    SM_MUL_Q, /* oddified DCT inverse quantization (for N64 SP, VMACQ only) */
    SM_MUL_I, /* (reserved) for VMULI and VMACI (VRNDP and VRNDN) */
    SM_ADD_A, /* VADD and VSUB arithmetic */
    SM_ADD_L, /* VABS dynamic inversion/negation logic */
    SM_DIV_R, /* reciprocal of the vector */
    SM_DIV_S, /* square root of the vector reciprocal */
    EOL /* more stuff here if you want */
};

static signed int result[N];

INLINE void SIGNED_CLAMP(short* VD, int mode)
{
    short hi[N], lo[N];
    register int i;

    switch (mode)
    {
        case SM_MUL_X: /* typical sign-clamp of accumulator-mid (bits 31:16) */
            for (i = 0; i < N; i++)
                VD[i]  = ACC_M(i);
            for (i = 0; i < N; i++)
                lo[i] = (result[i] + 32768) >> 31;
            for (i = 0; i < N; i++)
                hi[i] = (32767 - result[i]) >> 31;
            for (i = 0; i < N; i++)
                VD[i] &= ~lo[i];
            for (i = 0; i < N; i++)
                VD[i] |=  hi[i];
            for (i = 0; i < N; i++)
                VD[i] ^= 0x8000 & (hi[i] | lo[i]);
            return;
        case SM_MUL_Z: /* sign-clamp accumulator-low (bits 15:0) */
            for (i = 0; i < N; i++)
                VD[i]  = ACC_L(i);
            for (i = 0; i < N; i++)
                lo[i] = (result[i] + 32768) >> 31;
            for (i = 0; i < N; i++)
                hi[i] = (32767 - result[i]) >> 31;
            for (i = 0; i < N; i++)
                VD[i] &= ~lo[i];
            for (i = 0; i < N; i++)
                VD[i] |=  hi[i];
            return;
        case SM_MUL_Q: /* possible DCT inverse quantization (VMACQ only) */
            for (i = 0; i < N; i++)
                result[i] = (short)(ACC_H(i) << 31);
            for (i = 0; i < N; i++)
                result[i] = result[i] | (ACC_M(i) << 15);
            for (i = 0; i < N; i++)
                result[i] = result[i] | ((unsigned short)ACC_L(i) >> 1);
            for (i = 0; i < N; i++)
                if (result[i] < -32768)
                    VD[i] = -32768 & ~0x000F;
                else if (result[i] > +32767)
                    VD[i] = +32767 & ~0x000F;
                else
                    VD[i] = result[i] & 0x0000FFF0;
            return;
        case SM_ADD_A: /* VADD and VSUB */
            for (i = 0; i < N; i++)
                VD[i]  = result[i] & 0x0000FFFF;
            for (i = 0; i < N; i++)
                lo[i] = (result[i] + 32768) >> 31;
            for (i = 0; i < N; i++)
                hi[i] = (32767 - result[i]) >> 31;
            for (i = 0; i < N; i++)
                VD[i] &= ~lo[i];
            for (i = 0; i < N; i++)
                VD[i] |=  hi[i];
            for (i = 0; i < N; i++)
                VD[i] ^= 0x8000 & (hi[i] | lo[i]);
            return;
    }
}

/*
 * special-purpose, vector control registers
 * (traditionally called "VCF" for "vector condition flags" outside of SGI)
 *
 * These normally should have type `int` because they are Boolean T/F arrays.
 * However, since SSE2 uses 128-bit XMM's, and Win32 `int` storage is 32-bit,
 * we have the problem of 32*N > 128 bits, so we use `short` to reduce packs.
 */
short ne[N]; /* $vco:  vector carry out register (high byte:  NOTEQUAL) */
short co[N]; /* $vco:  vector carry out register (low byte:  cry./borrow I/O) */
short clip[N]; /* $vcc:  vector compare code register (high byte:  clip only) */
short comp[N]; /* $vcc:  vector compare code register (low byte:  compare) */
short vce[N]; /* $vce:  vector compare extension register (one byte) */

/*
 * Note about the third vector condition flags register (VCE).
 *
 * Only CFC2, CTC2, and VCH may set this value without clearing.
 * My personal method of saying it:
 *     VCE[i] = (VS[slice] == ~VT[slice]); // || (VT[slice] == ~VS[slice])
 *
 * Literally, the honest algorithm is:
 *     if ((VS[slice] ^ VT[slice]) < 0)
 *         if (VS[slice] + VT[slice] == -1)
 *             VCE[i] = true;
 *         else
 *             VCE[i] = false;
 *     else
 *         VCE[i] = false;
 */

unsigned short get_VCO(void)
{
    register unsigned short VCO;

    VCO = 0x0000
      | (ne[0xF % N] << 0xF)
      | (ne[0xE % N] << 0xE)
      | (ne[0xD % N] << 0xD)
      | (ne[0xC % N] << 0xC)
      | (ne[0xB % N] << 0xB)
      | (ne[0xA % N] << 0xA)
      | (ne[0x9 % N] << 0x9)
      | (ne[0x8 % N] << 0x8)
      | (co[0x7 % N] << 0x7)
      | (co[0x6 % N] << 0x6)
      | (co[0x5 % N] << 0x5)
      | (co[0x4 % N] << 0x4)
      | (co[0x3 % N] << 0x3)
      | (co[0x2 % N] << 0x2)
      | (co[0x1 % N] << 0x1)
      | (co[0x0 % N] << 0x0);
    return (VCO); /* Big endian becomes little. */
}
unsigned short get_VCC(void)
{
    register unsigned short VCC;

    VCC = 0x0000
      | (clip[0xF % N] << 0xF)
      | (clip[0xE % N] << 0xE)
      | (clip[0xD % N] << 0xD)
      | (clip[0xC % N] << 0xC)
      | (clip[0xB % N] << 0xB)
      | (clip[0xA % N] << 0xA)
      | (clip[0x9 % N] << 0x9)
      | (clip[0x8 % N] << 0x8)
      | (comp[0x7 % N] << 0x7)
      | (comp[0x6 % N] << 0x6)
      | (comp[0x5 % N] << 0x5)
      | (comp[0x4 % N] << 0x4)
      | (comp[0x3 % N] << 0x3)
      | (comp[0x2 % N] << 0x2)
      | (comp[0x1 % N] << 0x1)
      | (comp[0x0 % N] << 0x0);
    return (VCC); /* Big endian becomes little. */
}
unsigned char get_VCE(void)
{
    register unsigned char VCE;

    VCE = 0x00
      | (vce[07] << 0x7)
      | (vce[06] << 0x6)
      | (vce[05] << 0x5)
      | (vce[04] << 0x4)
      | (vce[03] << 0x3)
      | (vce[02] << 0x2)
      | (vce[01] << 0x1)
      | (vce[00] << 0x0);
    return (VCE); /* Big endian becomes little. */
}
void set_VCO(unsigned short VCO)
{
    register int i;

    for (i = 0; i < N; i++)
        co[i] = (VCO >> (i + 0x0)) & 1;
    for (i = 0; i < N; i++)
        ne[i] = (VCO >> (i + 0x8)) & 1;
    return; /* Little endian becomes big. */
}
void set_VCC(unsigned short VCC)
{
    register int i;

    for (i = 0; i < N; i++)
        comp[i] = (VCC >> (i + 0x0)) & 1;
    for (i = 0; i < N; i++)
        clip[i] = (VCC >> (i + 0x8)) & 1;
    return; /* Little endian becomes big. */
}
void set_VCE(unsigned char VCE)
{
    register int i;

    for (i = 0; i < N; i++)
        vce[i] = (VCE >> i) & 1;
    return; /* Little endian becomes big. */
}

static void res_V(void)
{
    register int i;

    message("C2\nRESERVED", 2); /* uncertain how to handle reserved, untested */
    for (i = 0; i < N; i++)
        VR[inst.R.sa][i] = 0x0000; /* override behavior (Michael Tedder) */
    return;
}
static void res_M(void)
{
    message("VMUL IQ", 1);
    res_V();
    return; /* Ultra64 OS did have these, so one could implement this ext. */
}

#include "vabs.h"
#include "vadd.h"
#include "vaddc.h"
#include "vand.h"
#include "vch.h"
#include "vcl.h"
#include "vcr.h"
#include "veq.h"
#include "vge.h"
#include "vlt.h"
#include "vmacf.h"
#include "vmacq.h"
#include "vmacu.h"
#include "vmadh.h"
#include "vmadl.h"
#include "vmadm.h"
#include "vmadn.h"
#include "vmov.h"
#include "vmrg.h"
#include "vmudh.h"
#include "vmudl.h"
#include "vmudm.h"
#include "vmudn.h"
#include "vmulf.h"
#include "vmulu.h"
#include "vnand.h"
#include "vne.h"
#include "vnop.h"
#include "vnor.h"
#include "vnxor.h"
#include "vor.h"
#include "vrcp.h"
#include "vrcph.h"
#include "vrcpl.h"
#include "vrsq.h"
#include "vrsqh.h"
#include "vrsql.h"
#include "vsaw.h"
#include "vsub.h"
#include "vsubc.h"
#include "vxor.h"

static void (*COP2_C2[64])(void) = {
    VMULF  ,VMULU  ,res_M  ,res_M  ,VMUDL  ,VMUDM  ,VMUDN  ,VMUDH  , /* 000 */
    VMACF  ,VMACU  ,res_M  ,VMACQ  ,VMADL  ,VMADM  ,VMADN  ,VMADH  , /* 001 */
    VADD   ,VSUB   ,res_V  ,VABS   ,VADDC  ,VSUBC  ,res_V  ,res_V  , /* 010 */
    res_V  ,res_V  ,res_V  ,res_V  ,res_V  ,VSAW   ,res_V  ,res_V  , /* 011 */
    VLT    ,VEQ    ,VNE    ,VGE    ,VCL    ,VCH    ,VCR    ,VMRG   , /* 100 */
    VAND   ,VNAND  ,VOR    ,VNOR   ,VXOR   ,VNXOR  ,res_V  ,res_V  , /* 101 */
    VRCP   ,VRCPL  ,VRCPH  ,VMOV   ,VRSQ   ,VRSQL  ,VRSQH  ,VNOP   , /* 110 */
    res_V  ,res_V  ,res_V  ,res_V  ,res_V  ,res_V  ,res_V  ,res_V  , /* 111 */
}; /* 000     001     010     011     100     101     110     111 */

static void (*EX_VECTOR[64][16])(void) = {
    { /* "Vector Multiply Signed Fractions" */
        VMULF_v,VMULF_v,VMULF0q,VMULF1q,
        VMULF0h,VMULF1h,VMULF2h,VMULF3h,
        VMULF0w,VMULF1w,VMULF2w,VMULF3w,
        VMULF4w,VMULF5w,VMULF6w,VMULF7w
    },
    { /* "Vector Multiply Unsigned Fractions" */
        VMULU_v,VMULU_v,VMULU0q,VMULU1q,
        VMULU0h,VMULU1h,VMULU2h,VMULU3h,
        VMULU0w,VMULU1w,VMULU2w,VMULU3w,
        VMULU4w,VMULU5w,VMULU6w,VMULU7w
    },
    {
        res_M  ,res_M  ,res_M  ,res_M  ,
        res_M  ,res_M  ,res_M  ,res_M  ,
        res_M  ,res_M  ,res_M  ,res_M  ,
        res_M  ,res_M  ,res_M  ,res_M
    },
    {
        res_M  ,res_M  ,res_M  ,res_M  ,
        res_M  ,res_M  ,res_M  ,res_M  ,
        res_M  ,res_M  ,res_M  ,res_M  ,
        res_M  ,res_M  ,res_M  ,res_M
    },
    { /* DP:  "Vector Multiply Low Partial Products" */
        VMUDL_v,VMUDL_v,VMUDL0q,VMUDL1q,
        VMUDL0h,VMUDL1h,VMUDL2h,VMUDL3h,
        VMUDL0w,VMUDL1w,VMUDL2w,VMUDL3w,
        VMUDL4w,VMUDL5w,VMUDL6w,VMUDL7w
    },
    { /* DP:  "Vector Multiply Middle Partial Products" */
        VMUDM_v,VMUDM_v,VMUDM0q,VMUDM1q,
        VMUDM0h,VMUDM1h,VMUDM2h,VMUDM3h,
        VMUDM0w,VMUDM1w,VMUDM2w,VMUDM3w,
        VMUDM4w,VMUDM5w,VMUDM6w,VMUDM7w
    },
    { /* DP:  "Vector Multiply Middle Partial Products" inverse */
        VMUDN_v,VMUDN_v,VMUDN0q,VMUDN1q,
        VMUDN0h,VMUDN1h,VMUDN2h,VMUDN3h,
        VMUDN0w,VMUDN1w,VMUDN2w,VMUDN3w,
        VMUDN4w,VMUDN5w,VMUDN6w,VMUDN7w
    },
    { /* DP:  "Vector Multiply High Partial Products" */
        VMUDH_v,VMUDH_v,VMUDH0q,VMUDH1q,
        VMUDH0h,VMUDH1h,VMUDH2h,VMUDH3h,
        VMUDH0w,VMUDH1w,VMUDH2w,VMUDH3w,
        VMUDH4w,VMUDH5w,VMUDH6w,VMUDH7w
    },
    { /* "Vector Multiply-Accumulate Signed Fractions" */
        VMACF_v,VMACF_v,VMACF0q,VMACF1q,
        VMACF0h,VMACF1h,VMACF2h,VMACF3h,
        VMACF0w,VMACF1w,VMACF2w,VMACF3w,
        VMACF4w,VMACF5w,VMACF6w,VMACF7w
    },
    { /* "Vector Multiply-Accumulate Unsigned Fractions" */
        VMACU_v,VMACU_v,VMACU0q,VMACU1q,
        VMACU0h,VMACU1h,VMACU2h,VMACU3h,
        VMACU0w,VMACU1w,VMACU2w,VMACU3w,
        VMACU4w,VMACU5w,VMACU6w,VMACU7w
    },
    {
        res_M  ,res_M  ,res_M  ,res_M  ,
        res_M  ,res_M  ,res_M  ,res_M  ,
        res_M  ,res_M  ,res_M  ,res_M  ,
        res_M  ,res_M  ,res_M  ,res_M
    },
    { /* "Vector Accumulator Oddification" */
        VMACQ  ,VMACQ  ,VMACQ  ,VMACQ  ,
        VMACQ  ,VMACQ  ,VMACQ  ,VMACQ  ,
        VMACQ  ,VMACQ  ,VMACQ  ,VMACQ  ,
        VMACQ  ,VMACQ  ,VMACQ  ,VMACQ
    },
    { /* DP:  "Vector Multiply-Accumulate Low Partial Products" */
        VMADL_v,VMADL_v,VMADL0q,VMADL1q,
        VMADL0h,VMADL1h,VMADL2h,VMADL3h,
        VMADL0w,VMADL1w,VMADL2w,VMADL3w,
        VMADL4w,VMADL5w,VMADL6w,VMADL7w
    },
    { /* DP:  "Vector Multiply-Accumulate Middle Partial Products" */
        VMADM_v,VMADM_v,VMADM0q,VMADM1q,
        VMADM0h,VMADM1h,VMADM2h,VMADM3h,
        VMADM0w,VMADM1w,VMADM2w,VMADM3w,
        VMADM4w,VMADM5w,VMADM6w,VMADM7w
    },
    { /* DP:  "Vector Multiply-Accumulate Middle Partial Products" inverse */
        VMADN_v,VMADN_v,VMADN0q,VMADN1q,
        VMADN0h,VMADN1h,VMADN2h,VMADN3h,
        VMADN0w,VMADN1w,VMADN2w,VMADN3w,
        VMADN4w,VMADN5w,VMADN6w,VMADN7w
    },
    { /* DP:  "Vector Multiply-Accumulate High Partial Products" */
        VMADH_v,VMADH_v,VMADH0q,VMADH1q,
        VMADH0h,VMADH1h,VMADH2h,VMADH3h,
        VMADH0w,VMADH1w,VMADH2w,VMADH3w,
        VMADH4w,VMADH5w,VMADH6w,VMADH7w
    },
    { /* "Vector Addition of Short Elements" */
        VADD_v ,VADD_v ,VADD0q ,VADD1q ,
        VADD0h ,VADD1h ,VADD2h ,VADD3h ,
        VADD0w ,VADD1w ,VADD2w ,VADD3w ,
        VADD4w ,VADD5w ,VADD6w ,VADD7w
    },
    { /* "Vector Subtraction of Short Elements" */
        VSUB_v ,VSUB_v ,VSUB0q ,VSUB1q ,
        VSUB0h ,VSUB1h ,VSUB2h ,VSUB3h ,
        VSUB0w ,VSUB1w ,VSUB2w ,VSUB3w ,
        VSUB4w ,VSUB5w ,VSUB6w ,VSUB7w
    },
    {
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V
    },
    { /* "Vector Absolute Value of Short Elements" */
        VABS_v ,VABS_v ,VABS0q ,VABS1q ,
        VABS0h ,VABS1h ,VABS2h ,VABS3h ,
        VABS0w ,VABS1w ,VABS2w ,VABS3w ,
        VABS4w ,VABS5w ,VABS6w ,VABS7w
    },
    { /* "Vector Add Short Elements with Carry" */
        VADDC_v,VADDC_v,VADDC0q,VADDC1q,
        VADDC0h,VADDC1h,VADDC2h,VADDC3h,
        VADDC0w,VADDC1w,VADDC2w,VADDC3w,
        VADDC4w,VADDC5w,VADDC6w,VADDC7w
    },
    { /* "Vector Subtract Short Elements with Carry" */
        VSUBC_v,VSUBC_v,VSUBC0q,VSUBC1q,
        VSUBC0h,VSUBC1h,VSUBC2h,VSUBC3h,
        VSUBC0w,VSUBC1w,VSUBC2w,VSUBC3w,
        VSUBC4w,VSUBC5w,VSUBC6w,VSUBC7w
    },
    {
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V
    },
    {
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V
    },
    {
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V
    },
    {
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V
    },
    {
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V
    },
    {
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V
    },
    {
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V
    },
    { /* "Vector Accumulator Write" (store/slice/scalar?) */
        VSAWH  ,VSAWM  ,VSAWL  ,res_V  ,
        VSAWH  ,VSAWM  ,VSAWL  ,res_V  ,
        VSAWH  ,VSAWM  ,VSAWL  ,res_V  ,
        VSAWH  ,VSAWM  ,VSAWL  ,res_V
    },
    {
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V
    },
    {
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V
    },
    { /* "Vector Select Less Than" */
        VLT_v  ,VLT_v  ,VLT0q  ,VLT1q  ,
        VLT0h  ,VLT1h  ,VLT2h  ,VLT3h  ,
        VLT0w  ,VLT1w  ,VLT2w  ,VLT3w  ,
        VLT4w  ,VLT5w  ,VLT6w  ,VLT7w
    },
    { /* "Vector Select Equal" */
        VEQ_v  ,VEQ_v  ,VEQ0q  ,VEQ1q  ,
        VEQ0h  ,VEQ1h  ,VEQ2h  ,VEQ3h  ,
        VEQ0w  ,VEQ1w  ,VEQ2w  ,VEQ3w  ,
        VEQ4w  ,VEQ5w  ,VEQ6w  ,VEQ7w
    },
    { /* "Vector Select Not Equal" */
        VNE_v  ,VNE_v  ,VNE0q  ,VNE1q  ,
        VNE0h  ,VNE1h  ,VNE2h  ,VNE3h  ,
        VNE0w  ,VNE1w  ,VNE2w  ,VNE3w  ,
        VNE4w  ,VNE5w  ,VNE6w  ,VNE7w
    },
    { /* "Vector Select Greater Than or Equal" */
        VGE_v  ,VGE_v  ,VGE0q  ,VGE1q  ,
        VGE0h  ,VGE1h  ,VGE2h  ,VGE3h  ,
        VGE0w  ,VGE1w  ,VGE2w  ,VGE3w  ,
        VGE4w  ,VGE5w  ,VGE6w  ,VGE7w
    },
    { /* "Vector Select Clip Test Low" */
        VCL_v  ,VCL_v  ,VCL0q  ,VCL1q  ,
        VCL0h  ,VCL1h  ,VCL2h  ,VCL3h  ,
        VCL0w  ,VCL1w  ,VCL2w  ,VCL3w  ,
        VCL4w  ,VCL5w  ,VCL6w  ,VCL7w
    },
    { /* "Vector Select Clip Test High" */
        VCH_v  ,VCH_v  ,VCH0q  ,VCH1q  ,
        VCH0h  ,VCH1h  ,VCH2h  ,VCH3h  ,
        VCH0w  ,VCH1w  ,VCH2w  ,VCH3w  ,
        VCH4w  ,VCH5w  ,VCH6w  ,VCH7w
    },
    { /* SP, one's complement:  "Vector Select Clip Test Low" */
        VCR_v  ,VCR_v  ,VCR0q  ,VCR1q  ,
        VCR0h  ,VCR1h  ,VCR2h  ,VCR3h  ,
        VCR0w  ,VCR1w  ,VCR2w  ,VCR3w  ,
        VCR4w  ,VCR5w  ,VCR6w  ,VCR7w
    },
    { /* "Vector Select Merge" */
        VMRG_v ,VMRG_v ,VMRG0q ,VMRG1q ,
        VMRG0h ,VMRG1h ,VMRG2h ,VMRG3h ,
        VMRG0w ,VMRG1w ,VMRG2w ,VMRG3w ,
        VMRG4w ,VMRG5w ,VMRG6w ,VMRG7w
    },
    { /* "Vector AND of Short Elements" */
        VAND_v ,VAND_v ,VAND0q ,VAND1q ,
        VAND0h ,VAND1h ,VAND2h ,VAND3h ,
        VAND0w ,VAND1w ,VAND2w ,VAND3w ,
        VAND4w ,VAND5w ,VAND6w ,VAND7w
    },
    { /* "Vector NAND of Short Elements" */
        VNAND_v,VNAND_v,VNAND0q,VNAND1q,
        VNAND0h,VNAND1h,VNAND2h,VNAND3h,
        VNAND0w,VNAND1w,VNAND2w,VNAND3w,
        VNAND4w,VNAND5w,VNAND6w,VNAND7w
    },
    { /* "Vector OR of Short Elements" */
        VOR_v  ,VOR_v  ,VOR0q  ,VOR1q  ,
        VOR0h  ,VOR1h  ,VOR2h  ,VOR3h  ,
        VOR0w  ,VOR1w  ,VOR2w  ,VOR3w  ,
        VOR4w  ,VOR5w  ,VOR6w  ,VOR7w
    },
    { /* "Vector NOR of Short Elements" */
        VNOR_v ,VNOR_v ,VNOR0q ,VNOR1q ,
        VNOR0h ,VNOR1h ,VNOR2h ,VNOR3h ,
        VNOR0w ,VNOR1w ,VNOR2w ,VNOR3w ,
        VNOR4w ,VNOR5w ,VNOR6w ,VNOR7w
    },
    { /* "Vector XOR of Short Elements" */
        VXOR_v ,VXOR_v ,VXOR0q ,VXOR1q ,
        VXOR0h ,VXOR1h ,VXOR2h ,VXOR3h ,
        VXOR0w ,VXOR1w ,VXOR2w ,VXOR3w ,
        VXOR4w ,VXOR5w ,VXOR6w ,VXOR7w
    },
    { /* "Vector NXOR of Short Elements" */
        VNXOR_v,VNXOR_v,VNXOR0q,VNXOR1q,
        VNXOR0h,VNXOR1h,VNXOR2h,VNXOR3h,
        VNXOR0w,VNXOR1w,VNXOR2w,VNXOR3w,
        VNXOR4w,VNXOR5w,VNXOR6w,VNXOR7w
    },
    {
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V
    },
    {
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V  ,
        res_V  ,res_V  ,res_V  ,res_V
    },
    { /* SP:  "Vector Element Scalar Reciprocal" */
        VRCPv0 ,VRCPv1 ,VRCP0q ,VRCP1q ,
        VRCP0h ,VRCP1h ,VRCP2h ,VRCP3h ,
        VRCP0w ,VRCP1w ,VRCP2w ,VRCP3w ,
        VRCP4w ,VRCP5w ,VRCP6w ,VRCP7w
    },
    { /* DP:  "Vector Element Scalar Reciprocal Low" */
        VRCPLv0,VRCPLv1,VRCPL0q,VRCPL1q,
        VRCPL0h,VRCPL1h,VRCPL2h,VRCPL3h,
        VRCPL0w,VRCPL1w,VRCPL2w,VRCPL3w,
        VRCPL4w,VRCPL5w,VRCPL6w,VRCPL7w
    },
    { /* DP:  "Vector Element Scalar Reciprocal High" */
        VRCPHv0,VRCPHv1,VRCPH0q,VRCPH1q,
        VRCPH0h,VRCPH1h,VRCPH2h,VRCPH3h,
        VRCPH0w,VRCPH1w,VRCPH2w,VRCPH3w,
        VRCPH4w,VRCPH5w,VRCPH6w,VRCPH7w
    },
    { /* "Vector Element Scalar Move" */
        VMOVv0 ,VMOVv1 ,VMOV0q ,VMOV1q ,
        VMOV0h ,VMOV1h ,VMOV2h ,VMOV3h ,
        VMOV0w ,VMOV1w ,VMOV2w ,VMOV3w ,
        VMOV4w ,VMOV5w ,VMOV6w ,VMOV7w
    },
    { /* SP:  "Vector Element Scalar SQRT Reciprocal" */
        VRSQ   ,VRSQ   ,VRSQ   ,VRSQ   ,
        VRSQ   ,VRSQ   ,VRSQ   ,VRSQ   ,
        VRSQ   ,VRSQ   ,VRSQ   ,VRSQ   ,
        VRSQ   ,VRSQ   ,VRSQ   ,VRSQ
    },
    { /* DP:  "Vector Element Scalar SQRT Reciprocal Low" */
        VRSQLv0,VRSQLv1,VRSQL0q,VRSQL1q,
        VRSQL0h,VRSQL1h,VRSQL2h,VRSQL3h,
        VRSQL0w,VRSQL1w,VRSQL2w,VRSQL3w,
        VRSQL4w,VRSQL5w,VRSQL6w,VRSQL7w
    },
    { /* DP:  "Vector Element Scalar SQRT Reciprocal High" */
        VRSQHv0,VRSQHv1,VRSQH0q,VRSQH1q,
        VRSQH0h,VRSQH1h,VRSQH2h,VRSQH3h,
        VRSQH0w,VRSQH1w,VRSQH2w,VRSQH3w,
        VRSQH4w,VRSQH5w,VRSQH6w,VRSQH7w
    },
    { /* "Vector Null Instruction" */
        VNOP   ,VNOP   ,VNOP   ,VNOP   ,
        VNOP   ,VNOP   ,VNOP   ,VNOP   ,
        VNOP   ,VNOP   ,VNOP   ,VNOP   ,
        VNOP   ,VNOP   ,VNOP   ,VNOP
    }
};
#endif
