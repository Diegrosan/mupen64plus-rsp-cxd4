/******************************************************************************\
* Project:  SP VU Emulation Table:  Load Packed Unsigned to Vector Unit        *
* Authors:  Iconoclast                                                         *
* Release:  2013.04.20                                                         *
* License:  none (public domain)                                               *
\******************************************************************************/

void LUV(int vt, int element, signed int offset, int base)
{
    register unsigned int addr;
    int b;

    if (element != 0x0)
        goto ILLEGAL_EL;
    addr  = SR[base] + (offset <<= 3);
    b = addr & 0x00000007;
    addr &= 0x00000FF8;
    switch (b)
    {
        case 00:
            VR[vt][07] = RSP.DMEM[addr + BES(0x007)] << 7;
            VR[vt][06] = RSP.DMEM[addr + BES(0x006)] << 7;
            VR[vt][05] = RSP.DMEM[addr + BES(0x005)] << 7;
            VR[vt][04] = RSP.DMEM[addr + BES(0x004)] << 7;
            VR[vt][03] = RSP.DMEM[addr + BES(0x003)] << 7;
            VR[vt][02] = RSP.DMEM[addr + BES(0x002)] << 7;
            VR[vt][01] = RSP.DMEM[addr + BES(0x001)] << 7;
            VR[vt][00] = RSP.DMEM[addr + BES(0x000)] << 7;
            return;
        case 01: /* PKMN Puzzle League HVQM decoder */
            VR[vt][00] = RSP.DMEM[addr + BES(0x001)] << 7;
            VR[vt][01] = RSP.DMEM[addr + BES(0x002)] << 7;
            VR[vt][02] = RSP.DMEM[addr + BES(0x003)] << 7;
            VR[vt][03] = RSP.DMEM[addr + BES(0x004)] << 7;
            VR[vt][04] = RSP.DMEM[addr + BES(0x005)] << 7;
            VR[vt][05] = RSP.DMEM[addr + BES(0x006)] << 7;
            VR[vt][06] = RSP.DMEM[addr + BES(0x007)] << 7;
            addr += BES(0x008);
            addr &= 0x00000FFF;
            VR[vt][07] = RSP.DMEM[addr] << 7;
            return;
        case 02: /* PKMN Puzzle League HVQM decoder */
            VR[vt][00] = RSP.DMEM[addr + BES(0x002)] << 7;
            VR[vt][01] = RSP.DMEM[addr + BES(0x003)] << 7;
            VR[vt][02] = RSP.DMEM[addr + BES(0x004)] << 7;
            VR[vt][03] = RSP.DMEM[addr + BES(0x005)] << 7;
            VR[vt][04] = RSP.DMEM[addr + BES(0x006)] << 7;
            VR[vt][05] = RSP.DMEM[addr + BES(0x007)] << 7;
            addr += BES(0x008);
            addr &= 0x00000FFF;
            VR[vt][06] = RSP.DMEM[addr | 0x000] << 7;
            VR[vt][07] = RSP.DMEM[addr + 0x001] << 7;
            return;
        case 03: /* PKMN Puzzle League HVQM decoder */
            VR[vt][00] = RSP.DMEM[addr + BES(0x003)] << 7;
            VR[vt][01] = RSP.DMEM[addr + BES(0x004)] << 7;
            VR[vt][02] = RSP.DMEM[addr + BES(0x005)] << 7;
            VR[vt][03] = RSP.DMEM[addr + BES(0x006)] << 7;
            VR[vt][04] = RSP.DMEM[addr + BES(0x007)] << 7;
            addr += BES(0x008);
            addr &= 0x00000FFF;
            VR[vt][05] = RSP.DMEM[addr | 0x000] << 7;
            VR[vt][06] = RSP.DMEM[addr + 0x001] << 7;
            VR[vt][07] = RSP.DMEM[addr + 0x002] << 7;
            return;
        case 04: /* PKMN Puzzle League HVQM decoder */
            VR[vt][00] = RSP.DMEM[addr + BES(0x004)] << 7;
            VR[vt][01] = RSP.DMEM[addr + BES(0x005)] << 7;
            VR[vt][02] = RSP.DMEM[addr + BES(0x006)] << 7;
            VR[vt][03] = RSP.DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            VR[vt][04] = RSP.DMEM[addr + BES(0x000)] << 7;
            VR[vt][05] = RSP.DMEM[addr + BES(0x001)] << 7;
            VR[vt][06] = RSP.DMEM[addr + BES(0x002)] << 7;
            VR[vt][07] = RSP.DMEM[addr + BES(0x003)] << 7;
            return;
        case 05: /* PKMN Puzzle League HVQM decoder */
            VR[vt][00] = RSP.DMEM[addr + BES(0x005)] << 7;
            VR[vt][01] = RSP.DMEM[addr + BES(0x006)] << 7;
            VR[vt][02] = RSP.DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            VR[vt][03] = RSP.DMEM[addr + BES(0x000)] << 7;
            VR[vt][04] = RSP.DMEM[addr + BES(0x001)] << 7;
            VR[vt][05] = RSP.DMEM[addr + BES(0x002)] << 7;
            VR[vt][06] = RSP.DMEM[addr + BES(0x003)] << 7;
            VR[vt][07] = RSP.DMEM[addr + BES(0x004)] << 7;
            return;
        case 06: /* PKMN Puzzle League HVQM decoder */
            VR[vt][00] = RSP.DMEM[addr + BES(0x006)] << 7;
            VR[vt][01] = RSP.DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            VR[vt][02] = RSP.DMEM[addr + BES(0x000)] << 7;
            VR[vt][03] = RSP.DMEM[addr + BES(0x001)] << 7;
            VR[vt][04] = RSP.DMEM[addr + BES(0x002)] << 7;
            VR[vt][05] = RSP.DMEM[addr + BES(0x003)] << 7;
            VR[vt][06] = RSP.DMEM[addr + BES(0x004)] << 7;
            VR[vt][07] = RSP.DMEM[addr + BES(0x005)] << 7;
            return;
        case 07: /* PKMN Puzzle League HVQM decoder */
            VR[vt][00] = RSP.DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            VR[vt][01] = RSP.DMEM[addr + BES(0x000)] << 7;
            VR[vt][02] = RSP.DMEM[addr + BES(0x001)] << 7;
            VR[vt][03] = RSP.DMEM[addr + BES(0x002)] << 7;
            VR[vt][04] = RSP.DMEM[addr + BES(0x003)] << 7;
            VR[vt][05] = RSP.DMEM[addr + BES(0x004)] << 7;
            VR[vt][06] = RSP.DMEM[addr + BES(0x005)] << 7;
            VR[vt][07] = RSP.DMEM[addr + BES(0x006)] << 7;
            return;
    }
ILLEGAL_EL:
    message("LUV\nIllegal element.", 3);
    return;
}
