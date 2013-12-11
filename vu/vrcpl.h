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

#include "vu.h"
#include "divrom.h"

INLINE static void do_rcpl(int data)
{
    unsigned int addr;
    int fetch;
    int shift = 32;

    if (data < 0)
        data = -data - (data < -32768); /* -(x) if >=; ~(x) if < */
    do
    {
        --shift;
        if (data & (1 << shift))
            goto FOUND_MSB;
    } while (shift); /* while (shift > 0) or ((shift ^ 31) < 32) */
    shift = 31 - 16*DPH; /* if (data == 0) shift = DPH ? 16 ^ 31 : 0 ^ 31; */
FOUND_MSB:
    DPH = 0;
    shift ^= 31; /* Right-to-left shift direction conversion. */
    addr = (data << shift) >> 22;
    fetch = div_ROM[addr &= 0x000001FF];
    shift ^= 31; /* Flipped shift direction back to right-. */
    DivOut = (0x40000000 | (fetch << 14)) >> shift;
    if (DivIn < 0)
        DivOut = ~DivOut;
    else if (DivIn == 0) /* corner case:  overflow via division by zero */
        DivOut = 0x7FFFFFFF;
    else if (DivIn == -32768) /* corner case:  signed underflow barrier */
        DivOut = 0xFFFF0000;
    return;
}

static void VRCPL(void)
{
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][inst.R.rs & 07];
    do_rcpl(DivIn);
    for (i = 0; i < N; i++)
        VACC_L[i] = ST[i];
    VR[vd][de] = (short)DivOut;
    return;
}

static void VRCPLv0(void)
{
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][00];
    do_rcpl(DivIn);
    for (i = 0; i < N; i++)
        VACC_L[i] = VR[vt][i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPLv1(void)
{
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][01];
    do_rcpl(DivIn);
    for (i = 0; i < N; i++)
        VACC_L[i] = VR[vt][i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL0q(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][02];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0x2);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL1q(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][03];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0x3);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL0h(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][04];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0x4);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL1h(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][05];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0x5);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL2h(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][06];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0x6);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL3h(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][07];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0x7);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL0w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][00];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0x8);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL1w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][01];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0x9);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL2w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][02];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0xA);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL3w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][03];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0xB);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL4w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][04];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0xC);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL5w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][05];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0xD);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL6w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][06];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0xE);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
static void VRCPL7w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int de = inst.R.rd & 07;
    const int vt = inst.R.rt;
    register int i;

    DivIn &= -DPH;
    DivIn |= (unsigned short)VR[vt][07];
    do_rcpl(DivIn);
    SHUFFLE_VECTOR(SV, VR[vt], 0xF);
    for (i = 0; i < N; i++)
        VACC_L[i] = SV[i];
    VR[vd][de] = (short)DivOut;
    return;
}
