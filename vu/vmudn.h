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

INLINE static void do_mudn(short* VD, short* VS, short* VT)
{
    register int i;

    for (i = 0; i < N; i++)
        VACC_L[i] = ((unsigned short)(VS[i])*VT[i] & 0x00000000FFFF) >>  0;
    for (i = 0; i < N; i++)
        VACC_M[i] = ((unsigned short)(VS[i])*VT[i] & 0x0000FFFF0000) >> 16;
    for (i = 0; i < N; i++)
        VACC_H[i] = -(VACC_M[i] < 0);
    vector_copy(VD, VACC_L); /* no possibilities to clamp */
    return;
}

static void VMUDN(void)
{
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;

    do_mudn(VR[vd], VR[vs], ST);
    return;
}

static void VMUDN_v(void)
{
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    do_mudn(VR[vd], VR[vs], VR[vt]);
    return;
}
static void VMUDN0q(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x2);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN1q(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x3);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN0h(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x4);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN1h(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x5);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN2h(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x6);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN3h(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x7);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN0w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x8);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN1w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x9);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN2w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0xA);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN3w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0xB);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN4w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0xC);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN5w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0xD);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN6w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0xE);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
static void VMUDN7w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0xF);
    do_mudn(VR[vd], VR[vs], SV);
    return;
}
