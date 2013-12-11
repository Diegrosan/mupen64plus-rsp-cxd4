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

static void VMADM(int vd, int vs, int vt, int e)
{
    register int i;

    for (i = 0; i < N; i++)
        VACC[i].DW += VR[vs][i] * (unsigned short)VR_T(i);
    SIGNED_CLAMP(VMUL_PTR, SM_MUL_X);
    return;
}

static void VMADM_v(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] = VR[vs][i] * (unsigned short)(VR[vt][i]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM0q(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0x2 & 01) + (i & 0xE)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM1q(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0x3 & 01) + (i & 0xE)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM0h(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0x4 & 03) + (i & 0xC)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM1h(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0x5 & 03) + (i & 0xC)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM2h(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0x6 & 03) + (i & 0xC)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM3h(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0x7 & 03) + (i & 0xC)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM0w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0x8 & 07) + (i & 0x0)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM1w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0x9 & 07) + (i & 0x0)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM2w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0xA & 07) + (i & 0x0)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM3w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0xB & 07) + (i & 0x0)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM4w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0xC & 07) + (i & 0x0)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM5w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0xD & 07) + (i & 0x0)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM6w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0xE & 07) + (i & 0x0)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
static void VMADM7w(void)
{
    register int i;
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    for (i = 0; i < N; i++)
        result[i] =
            VR[vs][i]
          * (unsigned short)(VR[vt][(0xF & 07) + (i & 0x0)]);
    for (i = 0; i < N; i++)
        VACC[i].DW += result[i];
    SIGNED_CLAMP(VR[vd], SM_MUL_X);
    return;
}
