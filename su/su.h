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
* Project:  MSP Emulation Table for Scalar Unit Operations                     *
* Authors:  Iconoclast                                                         *
* Release:  2013.08.18                                                         *
* License:  none (public domain)                                               *
\******************************************************************************/
#ifndef _SU_H
#define _SU_H

/*
 * RSP virtual registers (of scalar unit)
 * The most important are the 32 general-purpose scalar registers.
 * We have the convenience of using a 32-bit machine (Win32) to emulate
 * another 32-bit machine (MIPS/N64), so the most natural way to accurately
 * emulate the scalar GPRs is to use the standard `int` type.  Situations
 * specifically requiring sign-extension or lack thereof are forcibly
 * applied as defined in the MIPS quick reference card and user manuals.
 * Remember that these are not the same "GPRs" as in the MIPS ISA and totally
 * abandon their designated purposes on the master CPU host (the VR4300),
 * hence most of the MIPS names "k0, k1, t0, t1, v0, v1 ..." no longer apply.
 */
static int SR[32];

/*
 * system control register indexing pointer table
 * This is particularly useful for directly executing MFC0.
 * MTC0 is too complicated to make regular use of this.
 * I cannot recreate as an array; they are engraved in zilmar plugin module.
 */
#if 0
static const unsigned long **CR[32] = {
    &RSP.SP_MEM_ADDR_REG,
    &RSP.SP_DRAM_ADDR_REG,
    &RSP.SP_RD_LEN_REG,
    &RSP.SP_WR_LEN_REG,
    &RSP.SP_STATUS_REG,
    &RSP.SP_DMA_FULL_REG,
    &RSP.SP_DMA_BUSY_REG,
    &RSP.SP_SEMAPHORE_REG, /* also "SP_RESERVED" */
    &RSP.DPC_START_REG,
    &RSP.DPC_END_REG,
    &RSP.DPC_CURRENT_REG,
    &RSP.DPC_STATUS_REG,
    &RSP.DPC_CLOCK_REG,
    &RSP.DPC_BUFBUSY_REG,
    &RSP.DPC_PIPEBUSY_REG,
    &RSP.DPC_TMEM_REG,
    NULL, /* Usually there are 32 scalar CPU control registers. */
    NULL, /* In fact, RSP assembler manual defines 32 indices, but lists 16. */
    NULL, /* Conflicting sources and undefined doc reference leave this gray. */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};
#endif

static void res_S(void)
{
    message("RESERVED", 3);
    return;
}

union {
    struct {
        unsigned int func:  6;
        unsigned int sa:  5;
        unsigned int rd:  5;
        unsigned int rt:  5;
        unsigned int rs:  5;
        unsigned int op:  6;
    } R;
    struct {
        unsigned int imm:  16;
        unsigned int rt:  5;
        unsigned int rs:  5;
        unsigned int op:  6;
    } I;
    struct {
        unsigned int target:  26;
        unsigned int op:  6;
    } J;
    unsigned W:  32;
    signed SW:  32;
} inst;

/*** Scalar, Special Operations ***/
static void BREAK(void) /* 000000 ----- ----- ----- ----- 001101 */
{
    *RSP.SP_STATUS_REG |= 0x00000003; /* SP_STATUS_BROKE | SP_STATUS_HALT */
    if (*RSP.SP_STATUS_REG & 0x00000040) /* SP_STATUS_INTR_BREAK */
    {
        *RSP.MI_INTR_REG |= 0x00000001;
        RSP.CheckInterrupts();
        return;
    }
    return;
}

/*** Scalar, Jump and Branch Operations ***/
#ifdef EMULATE_STATIC_PC
#define BASE_OFF    0x000
#else
#define BASE_OFF    0x004
#endif

#define SLOT_OFF    (BASE_OFF + 0x000)
#define LINK_OFF    (BASE_OFF + 0x004)
void set_PC(int address)
{
#ifdef EMULATE_STATIC_PC
    *RSP.SP_PC_REG = 0x04001000 + (address & 0xFFC);
#else
    temp_PC        = 0x04001000 | (address & 0xFFC);
#endif
    stage = 1;
    return;
}
static void J(void) /* 000010 iiiiiiiiiiiiiiiiiiiiiiiiii */
{
    set_PC(4*inst.J.target);
    return;
}
static void JAL(void) /* 000011 iiiiiiiiiiiiiiiiiiiiiiiiii */
{
    SR[31] = (*RSP.SP_PC_REG + LINK_OFF) & 0x00000FFC;
    J();
    return;
}
static void JR(void) /* 000000 sssss ----- ----- ----- 001000 */
{
    set_PC(SR[inst.R.rs]);
    return;
}
static void JALR(void) /* 000000 sssss ----- ddddd ----- 001001 */
{
    SR[inst.R.rd] = (*RSP.SP_PC_REG + LINK_OFF) & 0x00000FFC;
    SR[0] = 0x00000000;
    JR();
    return;
}
static void BEQ(void) /* 000100 sssss ttttt iiiiiiiiiiiiiiii */
{
    const int BC = (SR[inst.I.rs] == SR[inst.I.rt]);
    const int offset = (signed short)(inst.I.imm);

    if (BC == 0)
        return;
    set_PC(*RSP.SP_PC_REG + 4*offset + SLOT_OFF);
    return;
}
static void BNE(void) /* 000101 sssss ttttt iiiiiiiiiiiiiiii */
{
    const int BC = (SR[inst.I.rs] != SR[inst.I.rt]);
    const int offset = (signed short)(inst.I.imm);

    if (BC == 0)
        return;
    set_PC(*RSP.SP_PC_REG + 4*offset + SLOT_OFF);
    return;
}
static void BLEZ(void) /* 000110 sssss 00000 iiiiiiiiiiiiiiii */
{
    const int BC = ((signed)(SR[inst.I.rs]) <= 0);
    const int offset = (signed short)(inst.I.imm);

    if (BC == 0)
        return;
    set_PC(*RSP.SP_PC_REG + 4*offset + SLOT_OFF);
    return;
}
static void BGTZ(void) /* 000111 sssss 00000 iiiiiiiiiiiiiiii */
{
    const int BC = ((signed)(SR[inst.I.rs])  > 0);
    const int offset = (signed short)(inst.I.imm);

    if (BC == 0)
        return;
    set_PC(*RSP.SP_PC_REG + 4*offset + SLOT_OFF);
    return;
}
static void BLTZ(void) /* 000001 sssss 00000 iiiiiiiiiiiiiiii */
{
    const int BC = ((signed)(SR[inst.I.rs])  < 0);
    const int offset = (signed short)(inst.I.imm);

    if (BC == 0)
        return;
    set_PC(*RSP.SP_PC_REG + 4*offset + SLOT_OFF);
    return;
}
static void BGEZ(void) /* 000001 sssss 00001 iiiiiiiiiiiiiiii */
{
    const int BC = ((signed)(SR[inst.I.rs]) >= 0);
    const int offset = (signed short)(inst.I.imm);

    if (BC == 0)
        return;
    set_PC(*RSP.SP_PC_REG + 4*offset + SLOT_OFF);
    return;
}
static void BLTZAL(void) /* 000001 sssss 10000 iiiiiiiiiiiiiiii */
{
    SR[31] = (*RSP.SP_PC_REG + LINK_OFF) & 0x00000FFC;
    BLTZ();
    return;
}
static void BGEZAL(void) /* 000001 sssss 10001 iiiiiiiiiiiiiiii */
{
    SR[31] = (*RSP.SP_PC_REG + LINK_OFF) & 0x00000FFC;
    BGEZ();
    return;
}

/*** Scalar, Shift Operations ***/
#if (0)
#define MASK_SA(sa) (sa & 31) /* Force masking in software. */
#else
#define MASK_SA(sa) (sa) /* Let hardware architecture do the mask for us. */
#endif
static void SLL(void) /* 000000 ----- ttttt ddddd aaaaa 000000 */
{
    SR[inst.R.rd] = SR[inst.R.rt] << MASK_SA(inst.W >> 6);
    SR[0] = 0x00000000;
    return;
}
static void SRL(void) /* 000000 ----- ttttt ddddd aaaaa 000010 */
{
    SR[inst.R.rd] = (unsigned)(SR[inst.R.rt]) >> MASK_SA(inst.W >> 6);
    SR[0] = 0x00000000;
    return;
}
static void SRA(void) /* 000000 ----- ttttt ddddd aaaaa 000011 */
{
    SR[inst.R.rd] = (signed)(SR[inst.R.rt]) >> MASK_SA(inst.W >> 6);
    SR[0] = 0x00000000;
    return;
}
static void SLLV(void) /* 000000 sssss ttttt ddddd ----- 000100 */
{
    SR[inst.R.rd] = SR[inst.R.rt] << MASK_SA(SR[inst.W >> 21]);
    SR[0] = 0x00000000;
    return;
}
static void SRLV(void) /* 000000 sssss ttttt ddddd ----- 000110 */
{
    SR[inst.R.rd] = (unsigned)(SR[inst.R.rt]) >> MASK_SA(SR[inst.W >> 21]);
    SR[0] = 0x00000000;
    return;
}
static void SRAV(void) /* 000000 sssss ttttt ddddd ----- 000111 */
{
    SR[inst.R.rd] = (signed)(SR[inst.R.rt]) >> MASK_SA(SR[inst.W >> 21]);
    SR[0] = 0x00000000;
    return;
}

/*** Scalar, Arithmetic and Logical Operations ***/
static void ADDU(void);
static void ADD(void) /* 000000 sssss ttttt ddddd ----- 100000 */
{
    const int trap = 0; /* There is no overflow trap on the RSP.  Use ADDU. */

    if (!trap)
        ADDU();
    return;
}
static void ADDIU(void);
static void ADDI(void) /* 001000 sssss ttttt iiiiiiiiiiiiiiii */
{
    const int trap = 0; /* There is no overflow trap on the RSP.  Use ADDIU. */

    if (!trap)
        ADDIU();
    return;
}
static void ADDIU(void) /* 001001 sssss ttttt iiiiiiiiiiiiiiii */
{
    SR[inst.I.rt] = SR[inst.I.rs] + (signed short)(inst.I.imm);
    SR[0] = 0x00000000;
    return;
}
static void ADDU(void) /* 000000 sssss ttttt ddddd ----- 100001 */
{
    SR[inst.R.rd] = SR[inst.W >> 21] + SR[inst.R.rt];
    SR[0] = 0x00000000;
    return;
}
static void SUBU(void);
static void SUB(void) /* 000000 sssss ttttt ddddd ----- 100010 */
{
    const int trap = 0; /* There is no overflow trap on the RSP.  Use SUBU. */

    if (!trap)
        SUBU();
    return;
} /* There is no overflow trap on the RSP.  Use SUBU. */
static void SUBU(void) /* 000000 sssss ttttt ddddd ----- 100011 */
{
    SR[inst.R.rd] = SR[inst.W >> 21] - SR[inst.R.rt];
    SR[0] = 0x00000000;
    return;
}
static void SLT(void) /* 000000 sssss ttttt ddddd ----- 101010 */
{
    SR[inst.R.rd] = ((signed)(SR[inst.W >> 21]) < (signed)(SR[inst.R.rt]));
    SR[0] = 0x00000000;
    return;
}
static void SLTI(void) /* 001010 sssss ttttt iiiiiiiiiiiiiiii */
{
    SR[inst.I.rt] = ((signed)(SR[inst.I.rs]) < (signed short)(inst.I.imm));
#if (0)
    SR[0] = 0x00000000; /* if (rt == 0), then NOP is called, not SLTI. */
#endif
    return;
}
static void SLTIU(void) /* 001011 sssss ttttt iiiiiiiiiiiiiiii */
{
    SR[inst.I.rt] = ((unsigned)(SR[inst.I.rs]) < inst.I.imm);
#if (0)
    SR[0] = 0x00000000; /* if (rt == 0), then NOP is called, not SLTIU. */
#endif
    return;
}
static void SLTU(void) /* 000000 sssss ttttt ddddd ----- 101011 */
{
    SR[inst.R.rd] = ((unsigned)(SR[inst.W >> 21]) < (unsigned)(SR[inst.R.rt]));
    SR[0] = 0x00000000;
    return;
}
static void AND(void) /* 000000 sssss ttttt ddddd ----- 100100 */
{
    SR[inst.R.rd] = SR[inst.W >> 21] & SR[inst.R.rt];
    SR[0] = 0x00000000;
    return;
}
static void OR(void) /* 000000 sssss ttttt ddddd ----- 100101 */
{
    SR[inst.R.rd] = SR[inst.W >> 21] | SR[inst.R.rt];
    SR[0] = 0x00000000;
    return;
}
static void XOR(void) /* 000000 sssss ttttt ddddd ----- 100110 */
{
    SR[inst.R.rd] = SR[inst.W >> 21] ^ SR[inst.R.rt];
    SR[0] = 0x00000000;
    return;
}
static void NOR(void) /* 000000 sssss ttttt ddddd ----- 100111 */
{
    SR[inst.R.rd] = ~(SR[inst.W >> 21] | SR[inst.R.rt]);
    SR[0] = 0x00000000;
    return;
}
static void ANDI(void) /* 001100 sssss ttttt iiiiiiiiiiiiiiii */
{
    SR[inst.I.rt] = SR[inst.I.rs] & inst.I.imm;
    SR[0] = 0x00000000;
    return;
}
static void ORI(void) /* 001101 sssss ttttt iiiiiiiiiiiiiiii */
{
    SR[inst.I.rt] = SR[inst.I.rs] | inst.I.imm;
    SR[0] = 0x00000000;
    return;
}
static void XORI(void) /* 001110 sssss ttttt iiiiiiiiiiiiiiii */
{
    SR[inst.I.rt] = SR[inst.I.rs] ^ inst.I.imm;
    SR[0] = 0x00000000;
    return;
}
static void LUI(void) /* 001111 ----- ttttt iiiiiiiiiiiiiiii */
{
    SR[inst.I.rt] = inst.I.imm << 16;
#if (0)
    SR[0] = 0x00000000; /* if (rt == 0), then NOP is called, not LUI. */
#endif
    return;
}

/*** Scalar, Load and Store Operations ***/
#if (0)
#define ENDIAN   0
#else
#define ENDIAN  ~0
#endif
#define BES(address) ((address) ^ ((ENDIAN) & 03))
#define HES(address) ((address) ^ ((ENDIAN) & 02))
#define MES(address) ((address) ^ ((ENDIAN) & 01))
#define WES(address) ((address) ^ ((ENDIAN) & 00))
#define SR_B(s, i) (*(unsigned char *)(((unsigned char *)(SR + s)) + BES(i)))
#define SR_S(s, i) (*(short *)(((unsigned char *)(SR + s)) + HES(i)))
#define SE(x, b)    (-(x & (1 << b)) | (x & ~(~0 << b)))
#define ZE(x, b)    (+(x & (1 << b)) | (x & ~(~0 << b)))

static union {
    unsigned char B[4];
    signed char SB[4];
    unsigned short H[2];
    signed short SH[2];
    unsigned W:  32;
} SR_temp;

static void LB(void) /* 100000 sssss ttttt iiiiiiiiiiiiiiii */
{
    register unsigned long addr;
    const signed int offset = (signed short)(inst.I.imm);

    addr = BES(SR[inst.I.rs] + offset) & 0x00000FFF;
    SR[inst.I.rt] = (signed char)(RSP.DMEM[addr]);
#if (0)
    SR[0] = 0x00000000; /* if (rt == 0), then NOP is called, not LB. */
#endif
    return;
}
static void LH(void) /* 100001 sssss ttttt iiiiiiiiiiiiiiii */
{
    register unsigned long addr;
    const int rt = inst.I.rt;
    const signed int offset = (signed short)(inst.I.imm);

    addr = (SR[inst.I.rs] + offset) & 0x00000FFF;
#if (0)
    SR_B(rt, 2) = RSP.DMEM[BES(addr)];
    addr = (addr + 0x001) & 0xFFF;
    SR_B(rt, 3) = RSP.DMEM[BES(addr)];
#else
    if (addr%4 == 0x003)
    {
        SR_B(rt, 2) = RSP.DMEM[addr - BES(0x000)];
        addr = (addr + 0x001) & 0xFFF;
        SR_B(rt, 3) = RSP.DMEM[addr + BES(0x000)];
    }
    else
        SR[rt] = *(short *)(RSP.DMEM + addr - HES(0x000)*(addr%4 - 1));
#endif
    SR[rt] = (signed short)(SR[rt]);
#if (0)
    SR[0] = 0x00000000; /* if (rt == 0), then NOP is called, not LH. */
#endif
    return;
}
extern void ULW(int rd, unsigned long addr);
static void LW(void) /* 100011 sssss ttttt iiiiiiiiiiiiiiii */
{
    register unsigned long addr;
    const int rt = inst.I.rt;
    const signed int offset = (signed short)(inst.I.imm);

    addr = (SR[inst.I.rs] + offset) & 0x00000FFF;
    if (addr & 0x00000003)
    {
        ULW(rt, addr); /* Address Error exception:  RSP bypass MIPS pseudo-op */
        return;
    }
    SR[rt] = *(int32_t *)(RSP.DMEM + addr);
#if (0)
    SR[0] = 0x00000000; /* if (rt == 0), then NOP is called, not LW. */
#endif
    return;
}
static void LBU(void) /* 100100 sssss ttttt iiiiiiiiiiiiiiii */
{
    register unsigned long addr;
    const signed int offset = (signed short)(inst.I.imm);

    addr = BES(SR[inst.I.rs] + offset) & 0x00000FFF;
    SR[inst.I.rt] = (unsigned char)(RSP.DMEM[addr]);
#if (0)
    SR[0] = 0x00000000; /* if (rt == 0), then NOP is called, not LBU. */
#endif
    return;
}
static void LHU(void) /* 100101 sssss ttttt iiiiiiiiiiiiiiii */
{
    register unsigned long addr;
    const int rt = inst.I.rt;
    const signed int offset = (signed short)(inst.I.imm);

    addr = (SR[inst.I.rs] + offset) & 0x00000FFF;
#if (0)
    SR_B(rt, 2) = RSP.DMEM[BES(addr)];
    addr = (addr + 0x001) & 0xFFF;
    SR_B(rt, 3) = RSP.DMEM[BES(addr)];
#else
    if (addr%4 == 0x003)
    {
        SR[rt]  = RSP.DMEM[addr - BES(0x000)] << 8;
        addr = (addr + 0x001) & 0xFFF;
        SR[rt] |= RSP.DMEM[addr + BES(0x000)];
        return;
    }
    SR[rt] = *(short *)(RSP.DMEM + addr - HES(0x000)*(addr%4 - 1));
#endif
    SR[rt] = (unsigned short)(SR[rt]);
#if (0)
    SR[0] = 0x00000000; /* if (rt == 0), then NOP is called, not LHU. */
#endif
    return;
}
static void SB(void) /* 101000 sssss ttttt iiiiiiiiiiiiiiii */
{
    register unsigned long addr;
    const signed int offset = (signed short)(inst.I.imm);

    addr = BES(SR[inst.I.rs] + offset) & 0x00000FFF;
    RSP.DMEM[addr] = (unsigned char)(SR[inst.I.rt]);
    return;
}
static void SH(void) /* 101001 sssss ttttt iiiiiiiiiiiiiiii */
{
    register unsigned long addr;
    const int rt = inst.I.rt;
    const signed int offset = (signed short)(inst.I.imm);

    addr = (SR[inst.I.rs] + offset) & 0x00000FFF;
#if (0)
    RSP.DMEM[BES(addr)] = SR_B(rt, 2);
    addr = (addr + 0x001) & 0xFFF;
    RSP.DMEM[BES(addr)] = SR_B(rt, 3);
#else
    if (addr%4 == 0x003)
    {
        RSP.DMEM[addr - BES(0x000)] = SR_B(rt, 2);
        addr = (addr + 0x001) & 0xFFF;
        RSP.DMEM[addr + BES(0x000)] = SR_B(rt, 3);
        return;
    }
    *(short *)(RSP.DMEM + addr - HES(0x000)*(addr%4 - 1)) = (short)(SR[rt]);
#endif
    return;
}
extern void USW(int rs, unsigned long addr);
static void SW(void) /* 101011 sssss ttttt iiiiiiiiiiiiiiii */
{
    register unsigned long addr;
    const int rt = inst.I.rt;
    const signed int offset = (signed short)(inst.I.imm);

    addr = (SR[inst.I.rs] + offset) & 0x00000FFF;
    if (addr & 0x00000003)
    {
        USW(rt, addr); /* Address Error exception:  RSP bypass MIPS pseudo-op */
        return;
    }
    *(int32_t *)(RSP.DMEM + addr) = SR[rt];
    return;
}

/*
 * All other behaviors defined below this point in the file are specific to
 * the SGI N64 extension to the MIPS R4000 and are not entirely implemented.
 */

/*** Scalar, Coprocessor Operations (system control) ***/
extern void SP_DMA_READ(void);
extern void SP_DMA_WRITE(void);
static void MFC0(void)
{
    const int rt = inst.R.rt;

    if (rt == 0)
        return;
    switch (inst.R.rd & 0xF)
    {
        case 0x0:
            SR[rt] = *RSP.SP_MEM_ADDR_REG;
            return;
        case 0x1:
            SR[rt] = *RSP.SP_DRAM_ADDR_REG;
            return;
        case 0x2: /* have not verified / been able to test yet ? */
            message("MFC0\nDMA_READ_LENGTH", 2);
            SR[rt] = *RSP.SP_RD_LEN_REG;
            return;
        case 0x3:
            message("MFC0\nDMA_WRITE_LENGTH", 3);
            return; /* dunno what to do, so error */
        case 0x4:
            SR[rt] = *RSP.SP_STATUS_REG;
#ifdef WAIT_FOR_CPU_HOST
            if (CFG_WAIT_FOR_CPU_HOST == 0)
                return;
            ++MFC0_count[rt];
            if (MFC0_count[rt] > 07)
                *RSP.SP_STATUS_REG |= 0x00000001; /* Let OS restart the task. */
#endif
            return;
        case 0x5: /* SR[rt] = !!(SP_STATUS_REG & SP_STATUS_DMAFULL) */
            SR[rt] = *RSP.SP_DMA_FULL_REG;
            return;
        case 0x6: /* SR[rt] = !!(SP_STATUS_REG & SP_STATUS_DMABUSY) */
            SR[rt] = *RSP.SP_DMA_BUSY_REG;
            return;
        case 0x7:
            SR[rt] = *RSP.SP_SEMAPHORE_REG;
#ifdef SEMAPHORE_LOCK_CORRECTIONS
            if (CFG_MEND_SEMAPHORE_LOCK == 0)
                return;
            *RSP.SP_SEMAPHORE_REG = 0x00000001;
            *RSP.SP_STATUS_REG |= 0x00000001; /* temporary bit to break CPU */
#endif
            return;
        case 0x8:
            SR[rt] = *RSP.DPC_START_REG;
            return;
        case 0x9:
            SR[rt] = *RSP.DPC_END_REG;
            return;
        case 0xA:
            SR[rt] = *RSP.DPC_CURRENT_REG;
            return;
        case 0xB:
            if (*RSP.DPC_STATUS_REG & 0x00000600) /* end/start valid ? */
                message("MFC0\nCMD_STATUS", 0); /* This is just CA-related. */
            SR[rt] = *RSP.DPC_STATUS_REG;
            return;
        case 0xC:
            SR[rt] = *RSP.DPC_CLOCK_REG;
            return;
        case 0xD:
            SR[rt] = *RSP.DPC_BUFBUSY_REG;
            return;
        case 0xE:
            SR[rt] = *RSP.DPC_PIPEBUSY_REG;
            return;
        case 0xF:
            SR[rt] = *RSP.DPC_TMEM_REG;
            return;
    }
}
static void MTC0(void)
{
    const int rt = inst.R.rt;

    switch (inst.R.rd & 0xF)
    {
        case 0x0:
            *RSP.SP_MEM_ADDR_REG = SR[rt] & 0xFFFFFFF8;
            return; /* Reserved upper bits are filtered out on DMA R/W. */
        case 0x1: /* 24-bit RDRAM pointer */
            *RSP.SP_DRAM_ADDR_REG = SR[rt] & 0xFFFFFFF8;
            return; /* Again, we don't *yet* care about the reserved bits. */
        case 0x2:
            *RSP.SP_RD_LEN_REG = SR[rt] | 07;
            SP_DMA_READ();
            return;
        case 0x3:
            *RSP.SP_WR_LEN_REG = SR[rt] | 07;
            SP_DMA_WRITE();
            return;
        case 0x4:
            if (SR[rt] & 0xFE000040)
                message("MTC0\nSP_STATUS", 2);
            *RSP.SP_STATUS_REG &= ~(!!(SR[rt] & 0x00000001) <<  0);
            *RSP.SP_STATUS_REG |=  (!!(SR[rt] & 0x00000002) <<  0);
            *RSP.SP_STATUS_REG &= ~(!!(SR[rt] & 0x00000004) <<  1);
            *RSP.MI_INTR_REG &= ~((SR[rt] & 0x00000008) >> 3); /* SP_CLR_INTR */
            *RSP.MI_INTR_REG |=  ((SR[rt] & 0x00000010) >> 4); /* SP_SET_INTR */
            *RSP.SP_STATUS_REG |= (SR[rt] & 0x00000010) >> 4; /* int set halt */
            *RSP.SP_STATUS_REG &= ~(!!(SR[rt] & 0x00000020) <<  5);
         /* *RSP.SP_STATUS_REG |=  (!!(SR[rt] & 0x00000040) <<  5); */
            *RSP.SP_STATUS_REG &= ~(!!(SR[rt] & 0x00000080) <<  6);
            *RSP.SP_STATUS_REG |=  (!!(SR[rt] & 0x00000100) <<  6);
            *RSP.SP_STATUS_REG &= ~(!!(SR[rt] & 0x00000200) <<  7);
            *RSP.SP_STATUS_REG |=  (!!(SR[rt] & 0x00000400) <<  7);
            *RSP.SP_STATUS_REG &= ~(!!(SR[rt] & 0x00000800) <<  8);
            *RSP.SP_STATUS_REG |=  (!!(SR[rt] & 0x00001000) <<  8);
            *RSP.SP_STATUS_REG &= ~(!!(SR[rt] & 0x00002000) <<  9);
            *RSP.SP_STATUS_REG |=  (!!(SR[rt] & 0x00004000) <<  9);
            *RSP.SP_STATUS_REG &= ~(!!(SR[rt] & 0x00008000) << 10);
            *RSP.SP_STATUS_REG |=  (!!(SR[rt] & 0x00010000) << 10);
            *RSP.SP_STATUS_REG &= ~(!!(SR[rt] & 0x00020000) << 11);
            *RSP.SP_STATUS_REG |=  (!!(SR[rt] & 0x00040000) << 11);
            *RSP.SP_STATUS_REG &= ~(!!(SR[rt] & 0x00080000) << 12);
            *RSP.SP_STATUS_REG |=  (!!(SR[rt] & 0x00100000) << 12);
            *RSP.SP_STATUS_REG &= ~(!!(SR[rt] & 0x00200000) << 13);
            *RSP.SP_STATUS_REG |=  (!!(SR[rt] & 0x00400000) << 13);
            *RSP.SP_STATUS_REG &= ~(!!(SR[rt] & 0x00800000) << 14);
            *RSP.SP_STATUS_REG |=  (!!(SR[rt] & 0x01000000) << 14);
            return;
        case 0x5: /* read-only register, cannot directly write using MTC0 */
            message("MTC0\nDMA_FULL", 1);
            return;
        case 0x6: /* read-only register, cannot directly write using MTC0 */
            message("MTC0\nDMA_BUSY", 1);
            return;
        case 0x7:
            *RSP.SP_SEMAPHORE_REG = 0x00000000; /* Forced (zilmar + dox). */
            return;
        case 0x8:
            if (*RSP.DPC_BUFBUSY_REG) /* lock hazards not implemented */
                message("MTC0\nCMD_START", 0);
            *RSP.DPC_START_REG   = SR[rt] & ~07; /* Funnelcube demo--marshall */
            *RSP.DPC_CURRENT_REG = *RSP.DPC_START_REG;
            *RSP.DPC_END_REG     = *RSP.DPC_START_REG;
            return;
        case 0x9:
            if (*RSP.DPC_BUFBUSY_REG)
                message("MTC0\nCMD_END", 0); /* This is just CA-related. */
            *RSP.DPC_END_REG = SR[rt] & 0xFFFFFFF8;
            if (RSP.ProcessRdpList == NULL) /* zilmar GFX #1.2 */
                return;
            RSP.ProcessRdpList();
            return;
        case 0xA: /* read-only register, cannot directly write using MTC0 */
            message("MTC0\nCMD_CURRENT", 1);
            return;
        case 0xB:
            if (SR[rt] & 0xFFFFFD80) /* unsupported or reserved bits */
                message("MTC0\nCMD_STATUS", 2);
            *RSP.DPC_STATUS_REG &= ~(!!(SR[rt] & 0x00000001) << 0);
            *RSP.DPC_STATUS_REG |=  (!!(SR[rt] & 0x00000002) << 0);
            *RSP.DPC_STATUS_REG &= ~(!!(SR[rt] & 0x00000004) << 1);
            *RSP.DPC_STATUS_REG |=  (!!(SR[rt] & 0x00000008) << 1);
            *RSP.DPC_STATUS_REG &= ~(!!(SR[rt] & 0x00000010) << 2);
            *RSP.DPC_STATUS_REG |=  (!!(SR[rt] & 0x00000020) << 2);
/* Some NUS-CIC-6105 SP tasks try to clear some zeroed DPC registers. */
            *RSP.DPC_TMEM_REG     &= !(SR[rt] & 0x00000040) * -1;
         /* *RSP.DPC_PIPEBUSY_REG &= !(SR[rt] & 0x00000080) * -1; */
         /* *RSP.DPC_BUFBUSY_REG  &= !(SR[rt] & 0x00000100) * -1; */
            *RSP.DPC_CLOCK_REG    &= !(SR[rt] & 0x00000200) * -1;
            return;
        case 0xC:
            message("MTC0\nCMD_CLOCK", 1);
            *RSP.DPC_CLOCK_REG = SR[rt];
            return; /* Doc appendix says this is RW; elsewhere it says R. */
        case 0xD: /* read-only register, cannot directly write using MTC0 */
            message("MTC0\nCMD_BUSY", 2);
            return;
        case 0xE: /* read-only register, cannot directly write using MTC0 */
            message("MTC0\nCMD_PIPE_BUSY", 2);
            return;
        case 0xF: /* read-only register, cannot directly write using MTC0 */
            message("MTC0\nCMD_TMEM_BUSY", 2);
            return;
    }
}
void SP_DMA_READ(void)
{
    register unsigned int length;
    register unsigned int count;
    register unsigned int skip;

    length = (*RSP.SP_RD_LEN_REG & 0x00000FFF) >>  0;
    count  = (*RSP.SP_RD_LEN_REG & 0x000FF000) >> 12;
    skip   = (*RSP.SP_RD_LEN_REG & 0xFFF00000) >> 20;
    /* length |= 07; // already corrected by mtc0 */
    ++length;
    ++count;
    skip += length;
    do
    { /* `count` always starts > 0, so we begin with `do` instead of `while`. */
        unsigned int offC, offD; /* SP cache and dynamic DMA pointers */
        register unsigned int i = 0;

        --count;
        do
        {
            offC = (count*length + *RSP.SP_MEM_ADDR_REG + i) & 0x00001FF8;
            offD = (count*skip + *RSP.SP_DRAM_ADDR_REG + i) & 0x00FFFFF8;
            memcpy(RSP.DMEM + offC, RSP.RDRAM + offD, 8);
            i += 0x008;
        } while (i < length);
    } while (count);
    *RSP.SP_DMA_BUSY_REG = 0x00000000;
    *RSP.SP_STATUS_REG &= ~0x00000004; /* SP_STATUS_DMABUSY */
}
void SP_DMA_WRITE(void)
{
    register unsigned int length;
    register unsigned int count;
    register unsigned int skip;

    length = (*RSP.SP_WR_LEN_REG & 0x00000FFF) >>  0;
    count  = (*RSP.SP_WR_LEN_REG & 0x000FF000) >> 12;
    skip   = (*RSP.SP_WR_LEN_REG & 0xFFF00000) >> 20;
    /* length |= 07; // already corrected by mtc0 */
    ++length;
    ++count;
    skip += length;
    do
    { /* `count` always starts > 0, so we begin with `do` instead of `while`. */
        unsigned int offC, offD; /* SP cache and dynamic DMA pointers */
        register unsigned int i = 0;

        --count;
        do
        {
            offC = (count*length + *RSP.SP_MEM_ADDR_REG + i) & 0x00001FF8;
            offD = (count*skip + *RSP.SP_DRAM_ADDR_REG + i) & 0x00FFFFF8;
            memcpy(RSP.RDRAM + offD, RSP.DMEM + offC, 8);
            i += 0x000008;
        } while (i < length);
    } while (count);
    *RSP.SP_DMA_BUSY_REG = 0x00000000;
    *RSP.SP_STATUS_REG &= ~0x00000004; /* SP_STATUS_DMABUSY */
}

/*** Scalar, Coprocessor Operations (vector unit) ***/
extern ALIGNED short VR[32][8];
#define VR_B(v, e)  (*(unsigned char *)(((unsigned char *)(VR + v)) + MES(e)))
#define VR_S(v, e)  (*(short *)((unsigned char *)(*(VR + v)) + ((e + 1) & ~1)))
/* to-do:  check this stupid thing for (unsigned char *)(VR+v) like above? */
extern unsigned short get_VCO(void);
extern unsigned short get_VCC(void);
extern unsigned char get_VCE(void);
extern void set_VCO(unsigned short VCO);
extern void set_VCC(unsigned short VCC);
extern void set_VCE(unsigned char VCE);
unsigned short rwR_VCE(void)
{ /* never saw a game try to read VCE out to a scalar GPR yet */
#if (0)
    register unsigned short ret_slot;

    ret_slot = 0x00 | (unsigned short)get_VCE();
    return (ret_slot);
#else
    char* debug = "CFC2\nrd = 0o00";

 /* *(debug +  1) = (inst.R.rs & 4) ? 'T' : 'F'; */
    *(debug + 12) |= inst.R.rd >> 3;
    *(debug + 13) |= inst.R.rd & 07;
    message(debug, 3);
    return 0x00FF;
#endif
}
void rwW_VCE(unsigned short VCE)
{ /* never saw a game try to write VCE using a scalar GPR yet */
#if (0)
    register int i;

    VCE = 0x00 | (VCE & 0xFF);
    for (i = 0; i < 8; i++)
        vce[i] = (VCE >> i) & 1;
#else
    char *debug = "CTC2\nrd = 0o00";

 /* *(debug +  1) = (inst.R.rs & 4) ? 'T' : 'F'; */
    *(debug + 12) |= inst.R.rd >> 3;
    *(debug + 13) |= inst.R.rd & 07;
    message(debug, 3);
#endif
    return;
}

static unsigned short (*R_VCF[32])(void) = {
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
/* Hazard reaction barrier:  RD = (UINT16)(inst) >> 11, without &= 3. */
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE,
    get_VCO,get_VCC,rwR_VCE,rwR_VCE
};
static void (*W_VCF[32])(unsigned short) = {
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
/* Hazard reaction barrier:  RD = (UINT16)(inst) >> 11, without &= 3. */
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE,
    set_VCO,set_VCC,rwW_VCE,rwW_VCE
};
static void MFC2(void)
{
    const int rt = inst.R.rt;
    const int vs = inst.R.rd;
    const int e = inst.R.sa >> 1;

    if (e == 0xF)
        goto WRAP; /* Various games do this, actually. */
    SR[rt] = VR_S(vs, e);
    SR[rt] = (signed short)(SR[rt]);
    SR[0] = 0x00000000;
    return;
WRAP:
    SR_B(rt, 2) = VR_B(vs, 0xF);
    SR_B(rt, 3) = VR_B(vs, 0x0);
    SR[rt] = (signed short)(SR[rt]);
    SR[0] = 0x00000000;
    return;
}
static void MTC2(void)
{
    const int rt = inst.R.rt;
    const int vd = inst.R.rd;
    const int e = inst.R.sa >> 1;

    VR_S(vd, e) = (short)(SR[rt]);
    return; /* If element == 0xF, it does not matter; loads do not wrap over. */
}
static void CFC2(void)
{
    SR[inst.R.rt] = (signed short)R_VCF[inst.R.rd]();
    SR[0] = 0x00000000;
    return;
}
static void CTC2(void)
{
    W_VCF[inst.R.rd](SR[inst.R.rt] & 0x0000FFFF);
    return;
}
static void C2(void)
{
    message("SU leaked to VU!", 3);
    return;
}

/*** Scalar, Coprocessor Operations (vector unit, scalar cache transfers) ***/
void LS_Group_I(int direction, int length)
{ /* Group I vector loads and stores, as defined in SGI's patent. */
    register unsigned long addr;
    register int i;
    register int e = (inst.R.sa >> 1) & 0xF;
    const signed int offset = SE(inst.SW, 6);

    addr = (SR[inst.R.rs] + length*offset);
    if (direction == 0) /* "Load %s to Vector Unit" */
        for (i = 0; i < length; i++)
            VR_B(inst.R.rt, (e + i) | 0x0) = RSP.DMEM[BES(addr + i) & 0xFFF];
    else /* "Store %s from Vector Unit" */
        for (i = 0; i < length; i++)
            RSP.DMEM[BES(addr + i) & 0xFFF] = VR_B(inst.R.rt, (e + i) & 0xF);
    return;
}
static void LBV(void)
{
    LS_Group_I(0, sizeof(unsigned char));
    return;
}
static void LSV(void)
{
#if (0)
    LS_Group_I(0, sizeof(short) > 2 ? 2 : sizeof(short));
    return;
#else
    register unsigned long addr;
    const int vt   = inst.R.rt;
    const int e    = inst.R.sa >> 1;
    const signed int offset = SE(inst.SW, 6);

    addr = (SR[inst.R.rs] + 2*offset) & 0x00000FFF;
    if (addr%0x004 == 0x003)
    { /* possibly not actually need this branch */
        message("LSV\nWeird addr.", 3);
        /* LBV(); */
        return;
    }
    VR_S(vt, e) = *(short *)(RSP.DMEM + addr - HES(0x000)*(addr%0x004 - 1));
    return;
#endif
}
static void LLV(void)
{
#if (0)
    LS_Group_I(0, sizeof(long) > 4 ? 4 : sizeof(long));
    return;
#else
    int correction;
    register unsigned long addr;
    const int vt = inst.R.rt;
    const int e  = inst.R.sa >> 1;
    const signed int offset = SE(inst.SW, 6);

    addr = (SR[inst.R.rs] + 4*offset) & 0x00000FFF;
    if (addr%0x004 & 0x001)
    {
        message("LLV\nOdd addr.", 3);
        return;
    }
    correction = HES(0x000)*(addr%0x004 - 1);
    VR_S(vt, e+0x0) = *(short *)(RSP.DMEM + addr - correction);
    addr = (addr + 0x002) & 0x00000FFF; /* F3DLX 1.23:  addr%4 is 0x002. */
    VR_S(vt, e+0x2) = *(short *)(RSP.DMEM + addr + correction);
    return;
#endif
}
static void LDV(void)
{
#if (0)
    LS_Group_I(0, 8);
    return;
#else
    register unsigned long addr;
    const int vt = inst.R.rt;
    const int e  = inst.R.sa >> 1;
    const signed int offset = SE(inst.SW, 6);

    addr = (SR[inst.R.rs] + 8*offset) & 0x00000FFF;
    switch (addr & 07)
    {
        case 00:
            VR_S(vt, e+0x0) = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR_S(vt, e+0x2) = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR_S(vt, e+0x4) = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR_S(vt, e+0x6) = *(short *)(RSP.DMEM + addr + HES(0x006));
            return;
        case 01: /* standard ABI ucodes (unlike e.g. MusyX w/ even addresses) */
            VR_S(vt, e+0x0) = *(short *)(RSP.DMEM + addr + 0x000);
            VR_B(vt, e+0x2) = RSP.DMEM[addr + 0x002 - BES(0x000)];
            VR_B(vt, e+0x3) = RSP.DMEM[addr + 0x003 + BES(0x000)];
            VR_S(vt, e+0x4) = *(short *)(RSP.DMEM + addr + 0x004);
            VR_B(vt, e+0x6) = RSP.DMEM[addr + 0x006 - BES(0x000)];
            addr += 0x007 + BES(00);
            addr &= 0x00000FFF;
            VR_B(vt, e+0x7) = RSP.DMEM[addr];
            return;
        case 02:
            VR_S(vt, e+0x0) = *(short *)(RSP.DMEM + addr + 0x000 - HES(0x000));
            VR_S(vt, e+0x2) = *(short *)(RSP.DMEM + addr + 0x002 + HES(0x000));
            VR_S(vt, e+0x4) = *(short *)(RSP.DMEM + addr + 0x004 - HES(0x000));
            addr += 0x006 + HES(00);
            addr &= 0x00000FFF;
            VR_S(vt, e+0x6) = *(short *)(RSP.DMEM + addr);
            return;
        case 03: /* standard ABI ucodes (unlike e.g. MusyX w/ even addresses) */
            VR_B(vt, e+0x0) = RSP.DMEM[addr + 0x000 - BES(0x000)];
            VR_B(vt, e+0x1) = RSP.DMEM[addr + 0x001 + BES(0x000)];
            VR_S(vt, e+0x2) = *(short *)(RSP.DMEM + addr + 0x002);
            VR_B(vt, e+0x4) = RSP.DMEM[addr + 0x004 - BES(0x000)];
            addr += 0x005 + BES(00);
            addr &= 0x00000FFF;
            VR_B(vt, e+0x5) = RSP.DMEM[addr];
            VR_S(vt, e+0x6) = *(short *)(RSP.DMEM + addr + 0x001 - BES(0x000));
            return;
        case 04:
            VR_S(vt, e+0x0) = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR_S(vt, e+0x2) = *(short *)(RSP.DMEM + addr + HES(0x002));
            addr += 0x004 + WES(00);
            addr &= 0x00000FFF;
            VR_S(vt, e+0x4) = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR_S(vt, e+0x6) = *(short *)(RSP.DMEM + addr + HES(0x002));
            return;
        case 05: /* standard ABI ucodes (unlike e.g. MusyX w/ even addresses) */
            VR_S(vt, e+0x0) = *(short *)(RSP.DMEM + addr + 0x000);
            VR_B(vt, e+0x2) = RSP.DMEM[addr + 0x002 - BES(0x000)];
            addr += 0x003;
            addr &= 0x00000FFF;
            VR_B(vt, e+0x3) = RSP.DMEM[addr + BES(0x000)];
            VR_S(vt, e+0x4) = *(short *)(RSP.DMEM + addr + 0x001);
            VR_B(vt, e+0x6) = RSP.DMEM[addr + BES(0x003)];
            VR_B(vt, e+0x7) = RSP.DMEM[addr + BES(0x004)];
            return;
        case 06:
            VR_S(vt, e+0x0) = *(short *)(RSP.DMEM + addr - HES(0x000));
            addr += 0x002;
            addr &= 0x00000FFF;
            VR_S(vt, e+0x2) = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR_S(vt, e+0x4) = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR_S(vt, e+0x6) = *(short *)(RSP.DMEM + addr + HES(0x004));
            return;
        case 07: /* standard ABI ucodes (unlike e.g. MusyX w/ even addresses) */
            VR_B(vt, e+0x0) = RSP.DMEM[addr - BES(0x000)];
            addr += 0x001;
            addr &= 0x00000FFF;
            VR_B(vt, e+0x1) = RSP.DMEM[addr + BES(0x000)];
            VR_S(vt, e+0x2) = *(short *)(RSP.DMEM + addr + 0x001);
            VR_B(vt, e+0x4) = RSP.DMEM[addr + BES(0x003)];
            VR_B(vt, e+0x5) = RSP.DMEM[addr + BES(0x004)];
            VR_S(vt, e+0x6) = *(short *)(RSP.DMEM + addr + 0x005);
            return;
    }
#endif
}
static void SBV(void)
{
    LS_Group_I(1, sizeof(unsigned char));
    return;
}
static void SSV(void)
{
#if (0)
    LS_Group_I(1, sizeof(short) > 2 ? 2 : sizeof(short));
    return;
#else
    register unsigned long addr;
    int e = inst.R.sa >> 1;
    const int vt = inst.R.rt;
    const signed int offset = SE(inst.SW, 6);

    addr = (SR[inst.R.rs] + 2*offset) & 0x00000FFF;
    if (addr%0x004 == 0x003)
    {
        message("SSV\nWeird addr.", 3);
        return;
    }
    if (e == 0xF)
    {
        message("SSV\nIllegal element.", 3);
        return;
    }
    *(short *)(RSP.DMEM + addr - HES(0x000)*(addr%0x004 - 1)) = VR_S(vt, e);
    return;
#endif
}
static void SLV(void)
{
#if (0)
    LS_Group_I(1, sizeof(long) > 4 ? 4 : sizeof(long));
    return;
#else
    int correction;
    register unsigned long addr;
    const int vt = inst.R.rt;
    const int e  = inst.R.sa >> 1;
    const signed int offset = SE(inst.SW, 6);

    addr = (SR[inst.R.rs] + 4*offset) & 0x00000FFF;
    if (addr & 0x001)
    {
        message("SLV\nOdd addr.", 3);
        return;
    }
    if (e > 0xC)
    {
        message("SLV\nIllegal element.", 3);
        return;
    }
    correction = HES(0x000)*(addr%0x004 - 1);
    *(short *)(RSP.DMEM + addr - correction) = VR_S(vt, e+0x0);
    addr += 0x002;
    addr &= 0x00000FFF; /* F3DLX 0.95:  "Mario Kart 64" */
    *(short *)(RSP.DMEM + addr + correction) = VR_S(vt, e+0x2);
    return;
#endif
}
static void SDV(void)
{
#if (1)
    LS_Group_I(1, 8);
    return;
#else
    0 = 0 / 0;
#endif
}

/*
 * Group II vector loads and stores:
 * PV and UV (As of RCP implementation, XV and ZV are reserved opcodes.)
 */
static void LPV(void)
{
    register unsigned long addr;
    register int b;
    const int vt = inst.R.rt;
    const int e  = inst.R.sa >> 1; /* Boss Game Studios audio illegal els */
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 8*offset) & 0x00000FFF;
    if (e != 0x0)
    {
        message("LPV\nIllegal element.", 3);
        return;
    }
    b = addr & 07;
    addr &= ~07;
    switch (b)
    { /** to-do:  vectorize shifts ??? **/
        case 00:
            VR[vt][07] = RSP.DMEM[addr + BES(0x007)] << 8;
            VR[vt][06] = RSP.DMEM[addr + BES(0x006)] << 8;
            VR[vt][05] = RSP.DMEM[addr + BES(0x005)] << 8;
            VR[vt][04] = RSP.DMEM[addr + BES(0x004)] << 8;
            VR[vt][03] = RSP.DMEM[addr + BES(0x003)] << 8;
            VR[vt][02] = RSP.DMEM[addr + BES(0x002)] << 8;
            VR[vt][01] = RSP.DMEM[addr + BES(0x001)] << 8;
            VR[vt][00] = RSP.DMEM[addr + BES(0x000)] << 8;
            return;
        case 01: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            VR[vt][00] = RSP.DMEM[addr + BES(0x001)] << 8;
            VR[vt][01] = RSP.DMEM[addr + BES(0x002)] << 8;
            VR[vt][02] = RSP.DMEM[addr + BES(0x003)] << 8;
            VR[vt][03] = RSP.DMEM[addr + BES(0x004)] << 8;
            VR[vt][04] = RSP.DMEM[addr + BES(0x005)] << 8;
            VR[vt][05] = RSP.DMEM[addr + BES(0x006)] << 8;
            VR[vt][06] = RSP.DMEM[addr + BES(0x007)] << 8;
            addr += BES(0x008);
            addr &= 0x00000FFF;
            VR[vt][07] = RSP.DMEM[addr] << 8;
            return;
        case 02: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            VR[vt][00] = RSP.DMEM[addr + BES(0x002)] << 8;
            VR[vt][01] = RSP.DMEM[addr + BES(0x003)] << 8;
            VR[vt][02] = RSP.DMEM[addr + BES(0x004)] << 8;
            VR[vt][03] = RSP.DMEM[addr + BES(0x005)] << 8;
            VR[vt][04] = RSP.DMEM[addr + BES(0x006)] << 8;
            VR[vt][05] = RSP.DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            VR[vt][06] = RSP.DMEM[addr + BES(0x000)] << 8;
            VR[vt][07] = RSP.DMEM[addr + BES(0x001)] << 8;
            return;
        case 03: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            VR[vt][00] = RSP.DMEM[addr + BES(0x003)] << 8;
            VR[vt][01] = RSP.DMEM[addr + BES(0x004)] << 8;
            VR[vt][02] = RSP.DMEM[addr + BES(0x005)] << 8;
            VR[vt][03] = RSP.DMEM[addr + BES(0x006)] << 8;
            VR[vt][04] = RSP.DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            VR[vt][05] = RSP.DMEM[addr + BES(0x000)] << 8;
            VR[vt][06] = RSP.DMEM[addr + BES(0x001)] << 8;
            VR[vt][07] = RSP.DMEM[addr + BES(0x002)] << 8;
            return;
        case 04: /* "Resident Evil 2" in-game 3-D, F3DLX 2.08--"WWF No Mercy" */
            VR[vt][00] = RSP.DMEM[addr + BES(0x004)] << 8;
            VR[vt][01] = RSP.DMEM[addr + BES(0x005)] << 8;
            VR[vt][02] = RSP.DMEM[addr + BES(0x006)] << 8;
            VR[vt][03] = RSP.DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            VR[vt][04] = RSP.DMEM[addr + BES(0x000)] << 8;
            VR[vt][05] = RSP.DMEM[addr + BES(0x001)] << 8;
            VR[vt][06] = RSP.DMEM[addr + BES(0x002)] << 8;
            VR[vt][07] = RSP.DMEM[addr + BES(0x003)] << 8;
            return;
        case 05: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            VR[vt][00] = RSP.DMEM[addr + BES(0x005)] << 8;
            VR[vt][01] = RSP.DMEM[addr + BES(0x006)] << 8;
            VR[vt][02] = RSP.DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            VR[vt][03] = RSP.DMEM[addr + BES(0x000)] << 8;
            VR[vt][04] = RSP.DMEM[addr + BES(0x001)] << 8;
            VR[vt][05] = RSP.DMEM[addr + BES(0x002)] << 8;
            VR[vt][06] = RSP.DMEM[addr + BES(0x003)] << 8;
            VR[vt][07] = RSP.DMEM[addr + BES(0x004)] << 8;
            return;
        case 06: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            VR[vt][00] = RSP.DMEM[addr + BES(0x006)] << 8;
            VR[vt][01] = RSP.DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            VR[vt][02] = RSP.DMEM[addr + BES(0x000)] << 8;
            VR[vt][03] = RSP.DMEM[addr + BES(0x001)] << 8;
            VR[vt][04] = RSP.DMEM[addr + BES(0x002)] << 8;
            VR[vt][05] = RSP.DMEM[addr + BES(0x003)] << 8;
            VR[vt][06] = RSP.DMEM[addr + BES(0x004)] << 8;
            VR[vt][07] = RSP.DMEM[addr + BES(0x005)] << 8;
            return;
        case 07: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            VR[vt][00] = RSP.DMEM[addr + BES(0x007)] << 8;
            addr += 0x008;
            addr &= 0x00000FFF;
            VR[vt][01] = RSP.DMEM[addr + BES(0x000)] << 8;
            VR[vt][02] = RSP.DMEM[addr + BES(0x001)] << 8;
            VR[vt][03] = RSP.DMEM[addr + BES(0x002)] << 8;
            VR[vt][04] = RSP.DMEM[addr + BES(0x003)] << 8;
            VR[vt][05] = RSP.DMEM[addr + BES(0x004)] << 8;
            VR[vt][06] = RSP.DMEM[addr + BES(0x005)] << 8;
            VR[vt][07] = RSP.DMEM[addr + BES(0x006)] << 8;
            return;
    }
}
static void LUV(void)
{
    register unsigned long addr;
    register int b;
    const int vt = inst.R.rt;
    int e  = inst.R.sa >> 1; /* fixme >.< */
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 8*offset) & 0x00000FFF;
    if (e != 0x0)
    { /* "Mia Hamm Soccer 64" SP exception override (zilmar) */
        addr += -e & 0xF;
        for (b = 0; b < 8; b++)
        {
            addr &= 0x00000FFF;
            VR[vt][b] = RSP.DMEM[BES(addr)] << 7;
            --e;
            addr -= 16 * (e == 0x0);
            ++addr;
        } /* to-do:  this shit can't be straightforward, like SQV ? */
        return;
    }
    b = addr & 07;
    addr &= ~07;
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
            addr += 0x008;
            addr &= 0x00000FFF;
            VR[vt][06] = RSP.DMEM[addr + BES(0x000)] << 7;
            VR[vt][07] = RSP.DMEM[addr + BES(0x001)] << 7;
            return;
        case 03: /* PKMN Puzzle League HVQM decoder */
            VR[vt][00] = RSP.DMEM[addr + BES(0x003)] << 7;
            VR[vt][01] = RSP.DMEM[addr + BES(0x004)] << 7;
            VR[vt][02] = RSP.DMEM[addr + BES(0x005)] << 7;
            VR[vt][03] = RSP.DMEM[addr + BES(0x006)] << 7;
            VR[vt][04] = RSP.DMEM[addr + BES(0x007)] << 7;
            addr += 0x008;
            addr &= 0x00000FFF;
            VR[vt][05] = RSP.DMEM[addr + BES(0x000)] << 7;
            VR[vt][06] = RSP.DMEM[addr + BES(0x001)] << 7;
            VR[vt][07] = RSP.DMEM[addr + BES(0x002)] << 7;
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
}
static void SPV(void)
{
    register int b;
    register unsigned long addr;
    const int e = inst.R.sa >> 1;
    const int vt = inst.R.rt;
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 8*offset) & 0x00000FFF;
    b = addr & 07;
    addr &= ~07;
    if (e != 0x0)
    {
        message("SPV\nIllegal element.", 3);
        return;
    }
    switch (b)
    {
        case 00:
            RSP.DMEM[addr + BES(0x007)] = (unsigned char)(VR[vt][07] >> 8);
            RSP.DMEM[addr + BES(0x006)] = (unsigned char)(VR[vt][06] >> 8);
            RSP.DMEM[addr + BES(0x005)] = (unsigned char)(VR[vt][05] >> 8);
            RSP.DMEM[addr + BES(0x004)] = (unsigned char)(VR[vt][04] >> 8);
            RSP.DMEM[addr + BES(0x003)] = (unsigned char)(VR[vt][03] >> 8);
            RSP.DMEM[addr + BES(0x002)] = (unsigned char)(VR[vt][02] >> 8);
            RSP.DMEM[addr + BES(0x001)] = (unsigned char)(VR[vt][01] >> 8);
            RSP.DMEM[addr + BES(0x000)] = (unsigned char)(VR[vt][00] >> 8);
            return;
        case 01: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            RSP.DMEM[addr + BES(0x001)] = (unsigned char)(VR[vt][00] >> 8);
            RSP.DMEM[addr + BES(0x002)] = (unsigned char)(VR[vt][01] >> 8);
            RSP.DMEM[addr + BES(0x003)] = (unsigned char)(VR[vt][02] >> 8);
            RSP.DMEM[addr + BES(0x004)] = (unsigned char)(VR[vt][03] >> 8);
            RSP.DMEM[addr + BES(0x005)] = (unsigned char)(VR[vt][04] >> 8);
            RSP.DMEM[addr + BES(0x006)] = (unsigned char)(VR[vt][05] >> 8);
            RSP.DMEM[addr + BES(0x007)] = (unsigned char)(VR[vt][06] >> 8);
            addr += BES(0x008);
            addr &= 0x00000FFF;
            RSP.DMEM[addr] = (unsigned char)(VR[vt][07] >> 8);
            return;
        case 02: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            RSP.DMEM[addr + BES(0x002)] = (unsigned char)(VR[vt][00] >> 8);
            RSP.DMEM[addr + BES(0x003)] = (unsigned char)(VR[vt][01] >> 8);
            RSP.DMEM[addr + BES(0x004)] = (unsigned char)(VR[vt][02] >> 8);
            RSP.DMEM[addr + BES(0x005)] = (unsigned char)(VR[vt][03] >> 8);
            RSP.DMEM[addr + BES(0x006)] = (unsigned char)(VR[vt][04] >> 8);
            RSP.DMEM[addr + BES(0x007)] = (unsigned char)(VR[vt][05] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            RSP.DMEM[addr + BES(0x000)] = (unsigned char)(VR[vt][06] >> 8);
            RSP.DMEM[addr + BES(0x001)] = (unsigned char)(VR[vt][07] >> 8);
            return;
        case 03: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            RSP.DMEM[addr + BES(0x003)] = (unsigned char)(VR[vt][00] >> 8);
            RSP.DMEM[addr + BES(0x004)] = (unsigned char)(VR[vt][01] >> 8);
            RSP.DMEM[addr + BES(0x005)] = (unsigned char)(VR[vt][02] >> 8);
            RSP.DMEM[addr + BES(0x006)] = (unsigned char)(VR[vt][03] >> 8);
            RSP.DMEM[addr + BES(0x007)] = (unsigned char)(VR[vt][04] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            RSP.DMEM[addr + BES(0x000)] = (unsigned char)(VR[vt][05] >> 8);
            RSP.DMEM[addr + BES(0x001)] = (unsigned char)(VR[vt][06] >> 8);
            RSP.DMEM[addr + BES(0x002)] = (unsigned char)(VR[vt][07] >> 8);
            return;
        case 04: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            RSP.DMEM[addr + BES(0x004)] = (unsigned char)(VR[vt][00] >> 8);
            RSP.DMEM[addr + BES(0x005)] = (unsigned char)(VR[vt][01] >> 8);
            RSP.DMEM[addr + BES(0x006)] = (unsigned char)(VR[vt][02] >> 8);
            RSP.DMEM[addr + BES(0x007)] = (unsigned char)(VR[vt][03] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            RSP.DMEM[addr + BES(0x000)] = (unsigned char)(VR[vt][04] >> 8);
            RSP.DMEM[addr + BES(0x001)] = (unsigned char)(VR[vt][05] >> 8);
            RSP.DMEM[addr + BES(0x002)] = (unsigned char)(VR[vt][06] >> 8);
            RSP.DMEM[addr + BES(0x003)] = (unsigned char)(VR[vt][07] >> 8);
            return;
        case 05: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            RSP.DMEM[addr + BES(0x005)] = (unsigned char)(VR[vt][00] >> 8);
            RSP.DMEM[addr + BES(0x006)] = (unsigned char)(VR[vt][01] >> 8);
            RSP.DMEM[addr + BES(0x007)] = (unsigned char)(VR[vt][02] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            RSP.DMEM[addr + BES(0x000)] = (unsigned char)(VR[vt][03] >> 8);
            RSP.DMEM[addr + BES(0x001)] = (unsigned char)(VR[vt][04] >> 8);
            RSP.DMEM[addr + BES(0x002)] = (unsigned char)(VR[vt][05] >> 8);
            RSP.DMEM[addr + BES(0x003)] = (unsigned char)(VR[vt][06] >> 8);
            RSP.DMEM[addr + BES(0x004)] = (unsigned char)(VR[vt][07] >> 8);
            return;
        case 06: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            RSP.DMEM[addr + BES(0x006)] = (unsigned char)(VR[vt][00] >> 8);
            RSP.DMEM[addr + BES(0x007)] = (unsigned char)(VR[vt][01] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            RSP.DMEM[addr + BES(0x000)] = (unsigned char)(VR[vt][02] >> 8);
            RSP.DMEM[addr + BES(0x001)] = (unsigned char)(VR[vt][03] >> 8);
            RSP.DMEM[addr + BES(0x002)] = (unsigned char)(VR[vt][04] >> 8);
            RSP.DMEM[addr + BES(0x003)] = (unsigned char)(VR[vt][05] >> 8);
            RSP.DMEM[addr + BES(0x004)] = (unsigned char)(VR[vt][06] >> 8);
            RSP.DMEM[addr + BES(0x005)] = (unsigned char)(VR[vt][07] >> 8);
            return;
        case 07: /* F3DZEX 2.08J "Doubutsu no Mori" (Animal Forest) CFB layer */
            RSP.DMEM[addr + BES(0x007)] = (unsigned char)(VR[vt][00] >> 8);
            addr += 0x008;
            addr &= 0x00000FFF;
            RSP.DMEM[addr + BES(0x000)] = (unsigned char)(VR[vt][01] >> 8);
            RSP.DMEM[addr + BES(0x001)] = (unsigned char)(VR[vt][02] >> 8);
            RSP.DMEM[addr + BES(0x002)] = (unsigned char)(VR[vt][03] >> 8);
            RSP.DMEM[addr + BES(0x003)] = (unsigned char)(VR[vt][04] >> 8);
            RSP.DMEM[addr + BES(0x004)] = (unsigned char)(VR[vt][05] >> 8);
            RSP.DMEM[addr + BES(0x005)] = (unsigned char)(VR[vt][06] >> 8);
            RSP.DMEM[addr + BES(0x006)] = (unsigned char)(VR[vt][07] >> 8);
            return;
    }
}
static void SUV(void)
{
    register int b;
    register unsigned long addr;
    const int e = inst.R.sa >> 1;
    const int vt = inst.R.rt;
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 8*offset) & 0x00000FFF;
    b = addr & 07;
    addr &= ~07;
    if (e != 0x0)
    {
        message("SUV\nIllegal element.", 3);
        return;
    }
    switch (b)
    {
        case 00:
            RSP.DMEM[addr + BES(0x007)] = (unsigned char)(VR[vt][07] >> 7);
            RSP.DMEM[addr + BES(0x006)] = (unsigned char)(VR[vt][06] >> 7);
            RSP.DMEM[addr + BES(0x005)] = (unsigned char)(VR[vt][05] >> 7);
            RSP.DMEM[addr + BES(0x004)] = (unsigned char)(VR[vt][04] >> 7);
            RSP.DMEM[addr + BES(0x003)] = (unsigned char)(VR[vt][03] >> 7);
            RSP.DMEM[addr + BES(0x002)] = (unsigned char)(VR[vt][02] >> 7);
            RSP.DMEM[addr + BES(0x001)] = (unsigned char)(VR[vt][01] >> 7);
            RSP.DMEM[addr + BES(0x000)] = (unsigned char)(VR[vt][00] >> 7);
            return;
        case 04: /* "Indiana Jones and the Infernal Machine" in-game */
            RSP.DMEM[addr + BES(0x004)] = (unsigned char)(VR[vt][00] >> 7);
            RSP.DMEM[addr + BES(0x005)] = (unsigned char)(VR[vt][01] >> 7);
            RSP.DMEM[addr + BES(0x006)] = (unsigned char)(VR[vt][02] >> 7);
            RSP.DMEM[addr + BES(0x007)] = (unsigned char)(VR[vt][03] >> 7);
            addr += 0x008;
            addr &= 0x00000FFF;
            RSP.DMEM[addr + BES(0x000)] = (unsigned char)(VR[vt][04] >> 7);
            RSP.DMEM[addr + BES(0x001)] = (unsigned char)(VR[vt][05] >> 7);
            RSP.DMEM[addr + BES(0x002)] = (unsigned char)(VR[vt][06] >> 7);
            RSP.DMEM[addr + BES(0x003)] = (unsigned char)(VR[vt][07] >> 7);
            return;
        default: /* Completely legal, just never seen it be done. */
            message("SUV\nWeird addr.", 3);
            return;
    }
}

/*
 * Group III vector loads and stores:
 * HV, FV, and AV (As of RCP implementation, AV opcodes are reserved.)
 */
static void LHV(void)
{
    register unsigned long addr;
    const int vt = inst.R.rt;
    const int e  = inst.R.sa >> 1;
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 16*offset) & 0x00000FFF;
    if (e != 0x0)
    {
        message("LHV\nIllegal element.", 3);
        return;
    }
    if (addr & 0x0000000E)
    {
        message("LHV\nIllegal addr.", 3);
        return;
    }
    addr ^= MES(00);
    VR[vt][07] = RSP.DMEM[addr + HES(0x00E)] << 7;
    VR[vt][06] = RSP.DMEM[addr + HES(0x00C)] << 7;
    VR[vt][05] = RSP.DMEM[addr + HES(0x00A)] << 7;
    VR[vt][04] = RSP.DMEM[addr + HES(0x008)] << 7;
    VR[vt][03] = RSP.DMEM[addr + HES(0x006)] << 7;
    VR[vt][02] = RSP.DMEM[addr + HES(0x004)] << 7;
    VR[vt][01] = RSP.DMEM[addr + HES(0x002)] << 7;
    VR[vt][00] = RSP.DMEM[addr + HES(0x000)] << 7;
    return;
}
static void LFV(void)
{ /* Dummy implementation only:  Do any games execute this? */
    char debugger[24] = "LFV\t$v00[X], 0x000($00)";
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;
    const char digits[16] = {
        '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
    };

    debugger[006] |= inst.R.rt / 10;
    debugger[007] |= inst.R.rt % 10;
    debugger[011]  = digits[inst.R.sa >> 1];
    debugger[017]  = digits[(offset >> 8) & 0xF];
    debugger[020]  = digits[(offset >> 4) & 0xF];
    debugger[021]  = digits[(offset >> 0) & 0xF];
    debugger[024] |= inst.R.rs / 10;
    debugger[025] |= inst.R.rs % 10;
    message(debugger, 3);
    return;
}
static void SHV(void)
{
    register unsigned long addr;
    const int e = inst.R.sa >> 1;
    const int vt = inst.R.rt;
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 16*offset) & 0x00000FFF;
    if (addr & 0x0000000E)
    {
        message("LHV\nIllegal addr.", 3);
        return;
    }
    if (e != 0x0)
    {
        message("LHV\nIllegal element.", 3);
        return;
    }
    addr ^= MES(00);
    RSP.DMEM[addr + HES(0x00E)] = (unsigned char)(VR[vt][07] >> 7);
    RSP.DMEM[addr + HES(0x00C)] = (unsigned char)(VR[vt][06] >> 7);
    RSP.DMEM[addr + HES(0x00A)] = (unsigned char)(VR[vt][05] >> 7);
    RSP.DMEM[addr + HES(0x008)] = (unsigned char)(VR[vt][04] >> 7);
    RSP.DMEM[addr + HES(0x006)] = (unsigned char)(VR[vt][03] >> 7);
    RSP.DMEM[addr + HES(0x004)] = (unsigned char)(VR[vt][02] >> 7);
    RSP.DMEM[addr + HES(0x002)] = (unsigned char)(VR[vt][01] >> 7);
    RSP.DMEM[addr + HES(0x000)] = (unsigned char)(VR[vt][00] >> 7);
    return;
}
static void SFV(void)
{
    register unsigned long addr;
    const int e = inst.R.sa >> 1;
    const int vt = inst.R.rt;
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 16*offset) & 0x00000FFF;
    addr &= 0x00000FF3;
    addr ^= BES(00);
    switch (e)
    {
        case 0x0:
            RSP.DMEM[addr + 0x000] = (unsigned char)(VR[vt][00] >> 7);
            RSP.DMEM[addr + 0x004] = (unsigned char)(VR[vt][01] >> 7);
            RSP.DMEM[addr + 0x008] = (unsigned char)(VR[vt][02] >> 7);
            RSP.DMEM[addr + 0x00C] = (unsigned char)(VR[vt][03] >> 7);
            return;
        case 0x8:
            RSP.DMEM[addr + 0x000] = (unsigned char)(VR[vt][04] >> 7);
            RSP.DMEM[addr + 0x004] = (unsigned char)(VR[vt][05] >> 7);
            RSP.DMEM[addr + 0x008] = (unsigned char)(VR[vt][06] >> 7);
            RSP.DMEM[addr + 0x00C] = (unsigned char)(VR[vt][07] >> 7);
            return;
        default:
            message("SFV\nIllegal element.", 3);
            return;
    }
}

/*
 * Group IV vector loads and stores:
 * QV and RV
 */
static void LQV(void)
{
    register unsigned long addr;
    register int b;
    const int vt = inst.R.rt;
    const int e  = inst.R.sa >> 1; /* Boss Game Studios audio illegal els */
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 16*offset) & 0x00000FFF;
    if (addr & 0x001)
    {
        message("LQV\nOdd addr.", 3);
        return;
    }
    b = addr & 0x0000000F;
    addr &= ~0x0000000F;
    switch (b/2) /* mistake in SGI patent regarding LQV */
    {
        case 0x0/2:
            VR_S(vt,e+0x0) = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR_S(vt,e+0x2) = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR_S(vt,e+0x4) = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR_S(vt,e+0x6) = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR_S(vt,e+0x8) = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR_S(vt,e+0xA) = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR_S(vt,e+0xC) = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR_S(vt,e+0xE) = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0x2/2:
            VR_S(vt,e+0x0) = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR_S(vt,e+0x2) = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR_S(vt,e+0x4) = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR_S(vt,e+0x6) = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR_S(vt,e+0x8) = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR_S(vt,e+0xA) = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR_S(vt,e+0xC) = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0x4/2:
            VR_S(vt,e+0x0) = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR_S(vt,e+0x2) = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR_S(vt,e+0x4) = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR_S(vt,e+0x6) = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR_S(vt,e+0x8) = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR_S(vt,e+0xA) = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0x6/2:
            VR_S(vt,e+0x0) = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR_S(vt,e+0x2) = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR_S(vt,e+0x4) = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR_S(vt,e+0x6) = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR_S(vt,e+0x8) = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0x8/2: /* "Resident Evil 2" cinematics and Boss Game Studios */
            VR_S(vt,e+0x0) = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR_S(vt,e+0x2) = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR_S(vt,e+0x4) = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR_S(vt,e+0x6) = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0xA/2: /* "Conker's Bad Fur Day" audio microcode by Rareware */
            VR_S(vt,e+0x0) = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR_S(vt,e+0x2) = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR_S(vt,e+0x4) = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0xC/2: /* "Conker's Bad Fur Day" audio microcode by Rareware */
            VR_S(vt,e+0x0) = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR_S(vt,e+0x2) = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0xE/2: /* "Conker's Bad Fur Day" audio microcode by Rareware */
            VR_S(vt,e+0x0) = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
    }
}
static void LRV(void)
{
    register unsigned long addr;
    register int b;
    const int vt = inst.R.rt;
    const int e  = inst.R.sa >> 1; /* Boss Game Studios audio illegal els */
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 16*offset) & 0x00000FFF;
    if (e != 0x0)
    {
        message("LRV\nIllegal element.", 3);
        return;
    }
    if (addr & 0x001)
    {
        message("LRV\nOdd addr.", 3);
        return;
    }
    b = addr & 0x0000000F;
    addr &= ~0x0000000F;
    switch (b/2)
    {
        case 0xE/2:
            VR[vt][01] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt][02] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt][03] = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR[vt][04] = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR[vt][05] = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR[vt][06] = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR[vt][07] = *(short *)(RSP.DMEM + addr + HES(0x00C));
            return;
        case 0xC/2:
            VR[vt][02] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt][03] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt][04] = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR[vt][05] = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR[vt][06] = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR[vt][07] = *(short *)(RSP.DMEM + addr + HES(0x00A));
            return;
        case 0xA/2:
            VR[vt][03] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt][04] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt][05] = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR[vt][06] = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR[vt][07] = *(short *)(RSP.DMEM + addr + HES(0x008));
            return;
        case 0x8/2:
            VR[vt][04] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt][05] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt][06] = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR[vt][07] = *(short *)(RSP.DMEM + addr + HES(0x006));
            return;
        case 0x6/2:
            VR[vt][05] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt][06] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt][07] = *(short *)(RSP.DMEM + addr + HES(0x004));
            return;
        case 0x4/2:
            VR[vt][06] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt][07] = *(short *)(RSP.DMEM + addr + HES(0x002));
            return;
        case 0x2/2:
            VR[vt][07] = *(short *)(RSP.DMEM + addr + HES(0x000));
            return;
        case 0x0/2:
            return;
    }
}
static void SQV(void)
{
    register unsigned int i;
    register unsigned long addr;
    const int e = inst.R.sa >> 1;
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 16*offset) & 0x00000FFF;
    for (i = 0; i < 16 - addr%16; i++)
        RSP.DMEM[BES((addr + i) & 0xFFF)] = VR_B(inst.R.rt, (e + i) & 0xF);
    return; /* "Mia Hamm Soccer 64" SP exception override (Ville Linde) */
}
static void SRV(void)
{
    register int b;
    register unsigned long addr;
    const int e = inst.R.sa >> 1;
    const int vt = inst.R.rt;
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 16*offset) & 0x00000FFF;
    b = addr & 0x0000000F;
    addr &= ~0x0000000F;
    if (e != 0x0)
    {
        message("SRV\nIllegal element.", 3);
        return;
    }
    switch (b/2)
    {
        case 0xE/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt][01];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt][02];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt][03];
            *(short *)(RSP.DMEM + addr + HES(0x006)) = VR[vt][04];
            *(short *)(RSP.DMEM + addr + HES(0x008)) = VR[vt][05];
            *(short *)(RSP.DMEM + addr + HES(0x00A)) = VR[vt][06];
            *(short *)(RSP.DMEM + addr + HES(0x00C)) = VR[vt][07];
            return;
        case 0xC/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt][02];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt][03];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt][04];
            *(short *)(RSP.DMEM + addr + HES(0x006)) = VR[vt][05];
            *(short *)(RSP.DMEM + addr + HES(0x008)) = VR[vt][06];
            *(short *)(RSP.DMEM + addr + HES(0x00A)) = VR[vt][07];
            return;
        case 0xA/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt][03];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt][04];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt][05];
            *(short *)(RSP.DMEM + addr + HES(0x006)) = VR[vt][06];
            *(short *)(RSP.DMEM + addr + HES(0x008)) = VR[vt][07];
            return;
        case 0x8/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt][04];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt][05];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt][06];
            *(short *)(RSP.DMEM + addr + HES(0x006)) = VR[vt][07];
            return;
        case 0x6/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt][05];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt][06];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt][07];
            return;
        case 0x4/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt][06];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt][07];
            return;
        case 0x2/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt][07];
            return;
        case 0x0/2:
            return;
    }
}

/*
 * Group V vector loads and stores
 * TV and SWV (As of RCP implementation, LTWV opcode was undesired.)
 */
static void LTV(void)
{ /* Warning:  N64 has bugged LTV behavior from normal SGI one.  (STV is OK.) */
    register unsigned long addr;
    const int vt = inst.R.rt;
    const int e  = inst.R.sa >> 1;
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 16*offset) & 0x00000FFF;
    if (addr & 0x00F)
    {
        message("LTV\nIllegal addr.", 3);
        return;
    }
    if (vt & 07)
    {
        message("LTV\nUncertain case!", 3);
        return; /* For LTV I am not sure; for STV I have an idea. */
    }
    if (e & 1)
    {
        message("LTV\nIllegal element.", 3);
        return;
    }
    switch (e/2)
    {
        case 0x0/2:
            VR[vt+0][00] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt+1][01] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt+2][02] = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR[vt+3][03] = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR[vt+4][04] = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR[vt+5][05] = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR[vt+6][06] = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR[vt+7][07] = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0x2/2:
            VR[vt+0][07] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt+1][00] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt+2][01] = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR[vt+3][02] = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR[vt+4][03] = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR[vt+5][04] = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR[vt+6][05] = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR[vt+7][06] = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0x4/2:
            VR[vt+0][06] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt+1][07] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt+2][00] = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR[vt+3][01] = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR[vt+4][02] = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR[vt+5][03] = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR[vt+6][04] = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR[vt+7][05] = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0x6/2:
            VR[vt+0][05] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt+1][06] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt+2][07] = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR[vt+3][00] = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR[vt+4][01] = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR[vt+5][02] = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR[vt+6][03] = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR[vt+7][04] = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0x8/2:
            VR[vt+0][04] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt+1][05] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt+2][06] = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR[vt+3][07] = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR[vt+4][00] = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR[vt+5][01] = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR[vt+6][02] = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR[vt+7][03] = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0xA/2:
            VR[vt+0][03] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt+1][04] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt+2][05] = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR[vt+3][06] = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR[vt+4][07] = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR[vt+5][00] = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR[vt+6][01] = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR[vt+7][02] = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0xC/2:
            VR[vt+0][02] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt+1][03] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt+2][04] = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR[vt+3][05] = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR[vt+4][06] = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR[vt+5][07] = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR[vt+6][00] = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR[vt+7][01] = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
        case 0xE/2:
            VR[vt+0][01] = *(short *)(RSP.DMEM + addr + HES(0x000));
            VR[vt+1][02] = *(short *)(RSP.DMEM + addr + HES(0x002));
            VR[vt+2][03] = *(short *)(RSP.DMEM + addr + HES(0x004));
            VR[vt+3][04] = *(short *)(RSP.DMEM + addr + HES(0x006));
            VR[vt+4][05] = *(short *)(RSP.DMEM + addr + HES(0x008));
            VR[vt+5][06] = *(short *)(RSP.DMEM + addr + HES(0x00A));
            VR[vt+6][07] = *(short *)(RSP.DMEM + addr + HES(0x00C));
            VR[vt+7][00] = *(short *)(RSP.DMEM + addr + HES(0x00E));
            return;
    }
}
static void SWV(void)
{ /* Dummy implementation only:  Do any games execute this? */
    char debugger[24] = "SWV\t$v00[X], 0x000($00)";
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;
    const char digits[16] = {
        '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
    };
    debugger[006] |= inst.R.rt / 10;
    debugger[007] |= inst.R.rt % 10;
    debugger[011]  = digits[inst.R.sa >> 1];
    debugger[017]  = digits[(offset >> 8) & 0xF];
    debugger[020]  = digits[(offset >> 4) & 0xF];
    debugger[021]  = digits[(offset >> 0) & 0xF];
    debugger[024] |= inst.R.rs / 10;
    debugger[025] |= inst.R.rs % 10;
    message(debugger, 3);
    return;
}
static void STV(void)
{
#if (0)
    register int i;
#endif
    register unsigned long addr;
    const int e = inst.R.sa >> 1;
    const int vt = inst.R.rt;
    const signed int offset = -(inst.SW & 0x00000040) | inst.R.func;

    addr = (SR[inst.R.rs] + 16*offset) & 0x00000FFF;
    if (addr & 0x00F)
    {
        message("STV\nIllegal addr.", 3);
        return;
    }
    if (vt & 07)
    {
        message("STV\nUncertain case!", 2);
        return; /* vt &= 030; */
    }
    if (e & 1)
    {
        message("STV\nIllegal element.", 3);
        return;
    }
    addr &= ~0x0000000F;
#if (0)
    for (i = 0; i < 16; i++)
        *(short *)(RSP.DMEM + addr + HES(2*i)) = VR[vt + e/2%8][i];
    return;
#endif
    switch (e/2)
    {
        case 0x0/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt+0][00];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt+1][01];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt+2][02];
            *(short *)(RSP.DMEM + addr + HES(0x006)) = VR[vt+3][03];
            *(short *)(RSP.DMEM + addr + HES(0x008)) = VR[vt+4][04];
            *(short *)(RSP.DMEM + addr + HES(0x00A)) = VR[vt+5][05];
            *(short *)(RSP.DMEM + addr + HES(0x00C)) = VR[vt+6][06];
            *(short *)(RSP.DMEM + addr + HES(0x00E)) = VR[vt+7][07];
            return;
        case 0x2/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt+1][00];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt+2][01];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt+3][02];
            *(short *)(RSP.DMEM + addr + HES(0x006)) = VR[vt+4][03];
            *(short *)(RSP.DMEM + addr + HES(0x008)) = VR[vt+5][04];
            *(short *)(RSP.DMEM + addr + HES(0x00A)) = VR[vt+6][05];
            *(short *)(RSP.DMEM + addr + HES(0x00C)) = VR[vt+7][06];
            *(short *)(RSP.DMEM + addr + HES(0x00E)) = VR[vt+0][07];
            return;
        case 0x4/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt+2][00];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt+3][01];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt+4][02];
            *(short *)(RSP.DMEM + addr + HES(0x006)) = VR[vt+5][03];
            *(short *)(RSP.DMEM + addr + HES(0x008)) = VR[vt+6][04];
            *(short *)(RSP.DMEM + addr + HES(0x00A)) = VR[vt+7][05];
            *(short *)(RSP.DMEM + addr + HES(0x00C)) = VR[vt+0][06];
            *(short *)(RSP.DMEM + addr + HES(0x00E)) = VR[vt+1][07];
            return;
        case 0x6/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt+3][00];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt+4][01];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt+5][02];
            *(short *)(RSP.DMEM + addr + HES(0x006)) = VR[vt+6][03];
            *(short *)(RSP.DMEM + addr + HES(0x008)) = VR[vt+7][04];
            *(short *)(RSP.DMEM + addr + HES(0x00A)) = VR[vt+0][05];
            *(short *)(RSP.DMEM + addr + HES(0x00C)) = VR[vt+1][06];
            *(short *)(RSP.DMEM + addr + HES(0x00E)) = VR[vt+2][07];
            return;
        case 0x8/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt+4][00];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt+5][01];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt+6][02];
            *(short *)(RSP.DMEM + addr + HES(0x006)) = VR[vt+7][03];
            *(short *)(RSP.DMEM + addr + HES(0x008)) = VR[vt+0][04];
            *(short *)(RSP.DMEM + addr + HES(0x00A)) = VR[vt+1][05];
            *(short *)(RSP.DMEM + addr + HES(0x00C)) = VR[vt+2][06];
            *(short *)(RSP.DMEM + addr + HES(0x00E)) = VR[vt+3][07];
            return;
        case 0xA/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt+5][00];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt+6][01];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt+7][02];
            *(short *)(RSP.DMEM + addr + HES(0x006)) = VR[vt+0][03];
            *(short *)(RSP.DMEM + addr + HES(0x008)) = VR[vt+1][04];
            *(short *)(RSP.DMEM + addr + HES(0x00A)) = VR[vt+2][05];
            *(short *)(RSP.DMEM + addr + HES(0x00C)) = VR[vt+3][06];
            *(short *)(RSP.DMEM + addr + HES(0x00E)) = VR[vt+4][07];
            return;
        case 0xC/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt+6][00];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt+7][01];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt+0][02];
            *(short *)(RSP.DMEM + addr + HES(0x006)) = VR[vt+1][03];
            *(short *)(RSP.DMEM + addr + HES(0x008)) = VR[vt+2][04];
            *(short *)(RSP.DMEM + addr + HES(0x00A)) = VR[vt+3][05];
            *(short *)(RSP.DMEM + addr + HES(0x00C)) = VR[vt+4][06];
            *(short *)(RSP.DMEM + addr + HES(0x00E)) = VR[vt+5][07];
            return;
        case 0xE/2:
            *(short *)(RSP.DMEM + addr + HES(0x000)) = VR[vt+7][00];
            *(short *)(RSP.DMEM + addr + HES(0x002)) = VR[vt+0][01];
            *(short *)(RSP.DMEM + addr + HES(0x004)) = VR[vt+1][02];
            *(short *)(RSP.DMEM + addr + HES(0x006)) = VR[vt+2][03];
            *(short *)(RSP.DMEM + addr + HES(0x008)) = VR[vt+3][04];
            *(short *)(RSP.DMEM + addr + HES(0x00A)) = VR[vt+4][05];
            *(short *)(RSP.DMEM + addr + HES(0x00C)) = VR[vt+5][06];
            *(short *)(RSP.DMEM + addr + HES(0x00E)) = VR[vt+6][07];
            return;
    }
}

/*** Modern pseudo-operations (not real instructions, but nice shortcuts) ***/
/*
 * cannot implement the following pseudo-instructions due to 2-D table limit:
 *
 * BAL     `BGEZAL  $zero, offset`         "Branch and Link"
 * NOT     `NOR     rd, rs, $zero`         "Not"
 * LA      [like LI but with label token]  "Load Address"
 * LI      `ORI     rd, $at, imm_lo`       "Load Immediate" (LUI $at, imm_hi)
 * MOVE    `ADD     rd, src, $zero`        "Move"
 * NEGU    `SUB     rd, $zero, src`        "Negate without Overflow"
 */
static void NOP(void)
{ /* "No Operation" */
    SR[0] = SR[0] << 0;
    return;
}
static void B(void)
{ /* "Unconditional Branch" */
    const int BC = (SR[0] == SR[0]);
    const int offset = (signed short)(inst.I.imm);

    if (BC == 0) /* impossible */
        return;
    set_PC(*RSP.SP_PC_REG + 4*offset + SLOT_OFF);
    return;
}
static void BEQZ(void)
{ /* "Branch on Equal to Zero" */
    const int BC = (SR[inst.I.rs] == 0);
    const int offset = (signed short)(inst.I.imm);

    if (BC == 0)
        return;
    set_PC(*RSP.SP_PC_REG + 4*offset + SLOT_OFF);
    return;
}
static void BNEZ(void)
{ /* "Branch on Not Equal to Zero" */
    const int BC = (SR[inst.I.rs] != 0);
    const int offset = (signed short)(inst.I.imm);

    if (BC == 0)
        return;
    set_PC(*RSP.SP_PC_REG + 4*offset + SLOT_OFF);
    return;
}
void ULW(int rd, unsigned long addr)
{ /* "Unaligned Load Word" */
    if (addr & 0x00000001)
    {
        SR_temp.B[03] = RSP.DMEM[BES(addr)];
        addr = (addr + 0x001) & 0xFFF;
        SR_temp.B[02] = RSP.DMEM[BES(addr)];
        addr = (addr + 0x001) & 0xFFF;
        SR_temp.B[01] = RSP.DMEM[BES(addr)];
        addr = (addr + 0x001) & 0xFFF;
        SR_temp.B[00] = RSP.DMEM[BES(addr)];
    }
    else /* addr & 0x00000002 */
    {
        SR_temp.H[01] = *(short *)(RSP.DMEM + addr - HES(0x000));
        addr = (addr + 0x002) & 0xFFF;
        SR_temp.H[00] = *(short *)(RSP.DMEM + addr + HES(0x000));
    }
    SR[rd] = SR_temp.W;
 /* SR[0] = 0x00000000; */
    return;
}
void USW(int rs, unsigned long addr)
{ /* "Unaligned Store Word" */
    SR_temp.W = SR[rs];
    if (addr & 0x00000001)
    {
        RSP.DMEM[BES(addr)] = SR_temp.B[03];
        addr = (addr + 0x001) & 0xFFF;
        RSP.DMEM[BES(addr)] = SR_temp.B[02];
        addr = (addr + 0x001) & 0xFFF;
        RSP.DMEM[BES(addr)] = SR_temp.B[01];
        addr = (addr + 0x001) & 0xFFF;
        RSP.DMEM[BES(addr)] = SR_temp.B[00];
    }
    else /* addr & 0x00000002 */
    {
        *(short *)(RSP.DMEM + addr - HES(0x000)) = SR_temp.H[01];
        addr = (addr + 0x002) & 0xFFF;
        *(short *)(RSP.DMEM + addr + HES(0x000)) = SR_temp.H[00];
    }
    return;
}

/*
 * All below pseudo-op-codes are unofficial and were invented by me.
 */
static void CLEAR(void)
{ /* based on an existing "CLEAR" pseudo-op, but translates differently */
    SR[inst.I.rt] = 0x00000000 & inst.I.imm;
    return;
} /* officially `AND rd, $zero, $zero`; implemented as `ANDI rt, $zero, imm` */
static void LXI(void)
{ /* "Load Sign-Extended Lower Immediate" (unofficial, created by me) */
    SR[inst.I.rt] = (signed short)(inst.I.imm);
    SR[0] = 0x00000000;
    return;
}
static void LZI(void)
{ /* "Load Zero-Extended Lower Immediate" (unofficial, created by me) */
    SR[inst.I.rt] = (0x0000 << 16) | inst.I.imm;
    SR[0] = 0x00000000;
    return;
}

static void (*EX_SCALAR[64][64])(void) = {
    { /* SPECIAL */
        SLL    ,res_S  ,SRL    ,SRA    ,SLLV   ,res_S  ,SRLV   ,SRAV   ,
        JR     ,JALR   ,res_S  ,res_S  ,res_S  ,BREAK  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        ADD    ,ADDU   ,SUB    ,SUBU   ,AND    ,OR     ,XOR    ,NOR    ,
        res_S  ,res_S  ,SLT    ,SLTU   ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* REGIMM */
        BLTZ   ,BGEZ   ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        BLTZAL ,BGEZAL ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
/* hazard reaction barrier -- rt only covers 32 sub-op-codes, not 64. */
        BLTZ   ,BGEZ   ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        BLTZAL ,BGEZAL ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* Jump */
        J      ,J      ,J      ,J      ,J      ,J      ,J      ,J      ,
        J      ,J      ,J      ,J      ,J      ,J      ,J      ,J      ,
        J      ,J      ,J      ,J      ,J      ,J      ,J      ,J      ,
        J      ,J      ,J      ,J      ,J      ,J      ,J      ,J      ,
        J      ,J      ,J      ,J      ,J      ,J      ,J      ,J      ,
        J      ,J      ,J      ,J      ,J      ,J      ,J      ,J      ,
        J      ,J      ,J      ,J      ,J      ,J      ,J      ,J      ,
        J      ,J      ,J      ,J      ,J      ,J      ,J      ,J
    },
    { /* Jump and Link */
        JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,
        JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,
        JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,
        JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,
        JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,
        JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,
        JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,
        JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL    ,JAL
    },
    { /* Branch on Equal */
        BEQZ   ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,
        BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,
        BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,
        BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,
        BEQZ   ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,
        BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,
        BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,
        BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ    ,BEQ
    },
    { /* Branch on Not Equal */
        BNEZ   ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,
        BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,
        BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,
        BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,
        BNEZ   ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,
        BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,
        BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,
        BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE    ,BNE
    },
    { /* Branch on Less Than or Equal to Zero */
        B      ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,
        BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,
        BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,
        BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,
        B      ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,
        BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,
        BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,
        BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ   ,BLEZ
    },
    { /* Branch on Greater Than Zero */
        NOP    ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,
        BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,
        BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,
        BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,
        NOP    ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,
        BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,
        BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,
        BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ   ,BGTZ
    },
    { /* Add Immediate Word */
        LXI    ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,
        ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,
        ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,
        ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,
        LXI    ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,
        ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,
        ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,
        ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI   ,ADDI
    },
    { /* Add Immediate Unsigned Word */
        LXI    ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,
        ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,
        ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,
        ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,
        LXI    ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,
        ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,
        ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,
        ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU  ,ADDIU
    }, /* Note:  Because the RSP is free of exception-handling, ADDI = ADDIU. */
    { /* Set on Less Than Immediate */
        NOP    ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,
        SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,
        SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,
        SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,
        NOP    ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,
        SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,
        SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,
        SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI   ,SLTI
    },
    { /* Set on Less Than Immediate Unsigned */
        NOP    ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,
        SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,
        SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,
        SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,
        NOP    ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,
        SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,
        SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,
        SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU  ,SLTIU
    },
    { /* And Immediate */
        CLEAR  ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,
        ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,
        ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,
        ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,
        CLEAR  ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,
        ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,
        ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,
        ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI   ,ANDI
    },
    { /* Or Immediate */
        LZI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,
        ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,
        ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,
        ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,
        LZI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,
        ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,
        ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,
        ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI    ,ORI
    },
    { /* Exclusive Or Immediate */
        LZI    ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,
        XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,
        XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,
        XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,
        LZI    ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,
        XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,
        XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,
        XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI   ,XORI
    },
    { /* Load Upper Immediate */
        NOP    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,
        LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,
        LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,
        LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,
        NOP    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,
        LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,
        LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,
        LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI    ,LUI
    },
    { /* COP0 */
        MFC0   ,res_S  ,res_S  ,res_S  ,MTC0   ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
/* hazard reaction barrier -- rs only covers 32 sub-op-codes, not 64. */
        MFC0   ,res_S  ,res_S  ,res_S  ,MTC0   ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* COP2 */
        MFC2   ,res_S  ,CFC2   ,res_S  ,MTC2   ,res_S  ,CTC2   ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        C2     ,C2     ,C2     ,C2     ,C2     ,C2     ,C2     ,C2     ,
        C2     ,C2     ,C2     ,C2     ,C2     ,C2     ,C2     ,C2     ,
/* hazard reaction barrier -- rs only covers 32 sub-op-codes, not 64. */
        MFC2   ,res_S  ,CFC2   ,res_S  ,MTC2   ,res_S  ,CTC2   ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        C2     ,C2     ,C2     ,C2     ,C2     ,C2     ,C2     ,C2     ,
        C2     ,C2     ,C2     ,C2     ,C2     ,C2     ,C2     ,C2
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* Load Byte */
        NOP    ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,
        LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,
        LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,
        LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,
        NOP    ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,
        LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,
        LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,
        LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB     ,LB
    },
    { /* Load Halfword */
        NOP    ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,
        LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,
        LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,
        LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,
        NOP    ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,
        LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,
        LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,
        LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH     ,LH
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* Load Word */
        NOP    ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,
        LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,
        LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,
        LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,
        NOP    ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,
        LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,
        LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,
        LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW     ,LW
    },
    { /* Load Byte Unsigned */
        NOP    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,
        LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,
        LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,
        LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,
        NOP    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,
        LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,
        LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,
        LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU    ,LBU
    },
    { /* Load Halfword Unsigned */
        NOP    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,
        LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,
        LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,
        LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,
        NOP    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,
        LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,
        LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,
        LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU    ,LHU
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* Store Byte */
        SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,
        SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,
        SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,
        SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,
        SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,
        SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,
        SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,
        SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB     ,SB
    },
    { /* Store Halfword */
        SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,
        SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,
        SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,
        SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,
        SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,
        SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,
        SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,
        SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH     ,SH
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* Store Word */
        SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,
        SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,
        SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,
        SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,
        SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,
        SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,
        SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,
        SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW     ,SW
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* LWC2 */
        LBV    ,LSV    ,LLV    ,LDV    ,LQV    ,LRV    ,LPV    ,LUV    ,
        LHV    ,LFV    ,res_S  ,LTV    ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
/* hazard reaction barrier -- rd only covers 32 sub-op-codes, not 64. */
        LBV    ,LSV    ,LLV    ,LDV    ,LQV    ,LRV    ,LPV    ,LUV    ,
        LHV    ,LFV    ,res_S  ,LTV    ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* SWC2 */
        SBV    ,SSV    ,SLV    ,SDV    ,SQV    ,SRV    ,SPV    ,SUV    ,
        SHV    ,SFV    ,SWV    ,STV    ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
/* hazard reaction barrier -- rd only covers 32 sub-op-codes, not 64. */
        SBV    ,SSV    ,SLV    ,SDV    ,SQV    ,SRV    ,SPV    ,SUV    ,
        SHV    ,SFV    ,SWV    ,STV    ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    },
    { /* illegal */
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,
        res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S  ,res_S
    }
};

#define OFF_FUNCTION     0
#define OFF_SA           6
#define OFF_E            7
#define OFF_RD          11
#define OFF_RT          16
#define OFF_RS          21
#define OFF_OPCODE      26
const int sub_op_table[64] = {
    OFF_FUNCTION, /* SPECIAL */
    OFF_RT, /* REGIMM */
    OFF_OPCODE, /* J */
    OFF_OPCODE, /* JAL */
    OFF_RT, /* BEQ (if rt is 0, treat as pseudo-operation BEQZ) */
    OFF_RT, /* BNE (if rt is 0, treat as pseudo-operation BNEZ) */
    OFF_RS, /* BLEZ (if rs is 0, then 0 <= 0 is B) */
    OFF_RS, /* BGTZ (if rs is 0, then 0 > 0 is NOP) */
    OFF_RS, /* ADDI (if rs is 0, then load sign-extended lower imm) */
    OFF_RS, /* ADDIU (if rs is 0, then load sign-extended lower imm) */
    OFF_OPCODE, /* SLTI */
    OFF_OPCODE, /* SLTIU */
    OFF_RS, /* ANDI (if rs is 0, then clear destination register) */
    OFF_RS, /* ORI (if rs is 0, then load zero-extended lower imm) */
    OFF_RS, /* XORI (if rs is 0, then load zero-extended lower imm) */
    OFF_RT, /* LUI */
    OFF_RS, /* COP0 */
    OFF_RS,
    OFF_RS, /* COP2 */
    OFF_RS,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_RT, /* LB */
    OFF_RT, /* LH */
    OFF_OPCODE,
    OFF_RT, /* LW */
    OFF_RT, /* LBU */
    OFF_RT, /* LHU */
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE, /* SB */
    OFF_OPCODE, /* SH */
    OFF_OPCODE,
    OFF_OPCODE, /* SW */
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_RD, /* LWC2 */
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_RD, /* SWC2 */
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE,
    OFF_OPCODE
};

#endif
