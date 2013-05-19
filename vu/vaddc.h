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

static void VADDC(int vd, int vs, int vt, int e)
{
    unsigned int result[8];
    register int i;

    VCO = 0x0000;
    for (i = 0; i < 8; i++)
        result[i] = (unsigned short)VR[vs][i] + (unsigned short)VR_T(i);
    for (i = 0; i < 8; i++)
        ACC_R(i) = (short)result[i];
    for (i = 0; i < 8; i++)
        VCO |= (result[i] > 0x0000FFFF) << i;
    for (i = 0; i < 8; i++)
        ACC_W(i) = ACC_R(i);
    return;
}
