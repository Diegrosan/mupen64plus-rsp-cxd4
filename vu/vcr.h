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

static void VCR(int vd, int vs, int vt, int e)
{
    int ge, le;
    register int i;

    VCC = 0x0000;
    for (i = 0; i < N; i++)
    {
        const signed short VS = VR[vs][i];
        const signed short VT = VR_T(i);
        const int sn = (VS ^ VT) < 0; /* sn = (unsigned short)(VS ^ VT) >> 15 */

        if (sn)
        {
            ge = (VT < 0); /* -VT > -0; (-VT - 1) > -1; (~VT) >= 0 */
            le = (VS + VT + 1 <= 0); /* VS + VT < 0; VS < -VT:  "VS <= ~VT" */
            ACC_R(i) = le ? ~VT : VS;
        }
        else
        {
            le = (VT < 0);
            ge = (VS - VT >= 0); /* VS - VT + 1 > 0; VS > VT - 1:  "VS >= VT" */
            ACC_R(i) = le ? VT : VS;
        }
        VCC |= (ge <<= (i + 8)) | (le <<= (i + 0));
    }
    for (i = 0; i < N; i++)
        ACC_W(i) = ACC_R(i);
    VCO = 0x0000;
    VCE = 0x00;
    return;
}

void do_cr(int vs)
{
    int ge[8], le[8];
    short VS[8];
    signed short sn[8];
    register int i;

    for (i = 0; i < N; i++)
        sn[i] = VR[vs][i] ^ VC[i];
    for (i = 0; i < N; i++)
        sn[i] = sn[i] >> 15;
#if (0)
    for (i = 0; i < N; i++)
        le[i] = sn[i] ? (VC[i] <= ~VR[vs][i]) : (VC[i] <= ~0x0000);
    for (i = 0; i < N; i++)
        ge[i] = sn[i] ? (~0x0000 >= VC[i]) : (VR[vs][i] >= VC[i]);
#else
    for (i = 0; i < N; i++)
        VS[i] = VR[vs][i] & sn[i];
    for (i = 0; i < N; i++)
        le[i] = (VC[i] <= ~VS[i]);
    for (i = 0; i < N; i++)
        VS[i] = VR[vs][i] | sn[i];
    for (i = 0; i < N; i++)
        ge[i] = (VS[i] >=  VC[i]);
#endif
    for (i = 0; i < N; i++)
        VC[i] ^= sn[i]; /* if (sn == ~0) {VT = ~VT;} else {VT =  VT;} */
    for (i = 0; i < N; i++)
        VACC[i].s[LO] = le[i] ? VC[i] : VR[vs][i];
#if (0)
    VCC = 0x0000;
    for (i = 0; i < N; i++)
        VCC |= le[i] << (i + 0x0);
    for (i = 0; i < N; i++)
        VCC |= ge[i] << (i + 0x8);
#else
    VCO =
        (ge[07] << 0xF) | (ge[06] << 0xE) | (ge[05] << 0xD) | (ge[04] << 0xC) |
        (ge[03] << 0xB) | (ge[02] << 0xA) | (ge[01] << 0x9) | (ge[00] << 0x8) |
        (le[07] << 0x7) | (le[06] << 0x6) | (le[05] << 0x5) | (le[04] << 0x4) |
        (le[03] << 0x3) | (le[02] << 0x2) | (le[01] << 0x1) | (le[00] << 0x0);
#endif
    VCO = 0x0000;
    VCE = 0x00;
    return;
}

static void VCR_v(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][i];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR0q(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0x2 & 01) + (i & 0xE)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR1q(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0x3 & 01) + (i & 0xE)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR0h(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0x4 & 03) + (i & 0xC)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR1h(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0x5 & 03) + (i & 0xC)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR2h(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0x6 & 03) + (i & 0xC)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR3h(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0x7 & 03) + (i & 0xC)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR0w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0x8 & 07) + (i & 0x0)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR1w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0x9 & 07) + (i & 0x0)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR2w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0xA & 07) + (i & 0x0)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR3w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0xB & 07) + (i & 0x0)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR4w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0xC & 07) + (i & 0x0)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR5w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0xD & 07) + (i & 0x0)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR6w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0xE & 07) + (i & 0x0)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
static void VCR7w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        VC[i] = VR[vt][(0xF & 07) + (i & 0x0)];
    do_cr(vs);
    for (i = 0; i < N; i++)
        VR[vd][i] = VACC[i].s[LO];
    return;
}
