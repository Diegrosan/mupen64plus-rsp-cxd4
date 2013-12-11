#include "Rsp_#1.1.h"
#include "rsp.h"

#include "su.h"
#include "vu/vu.h"
#include "matrix.h"

void run_task(void)
{
    register int PC;

    if (CFG_WAIT_FOR_CPU_HOST != 0)
    {
        register int i;

        for (i = 0; i < 32; i++)
            MFC0_count[i] = 0;
    }
    PC = *RSP.SP_PC_REG & 0x00000FFC;
    while ((*RSP.SP_STATUS_REG & 0x00000001) == 0x00000000)
    {
        const unsigned long inst = *(long *)(RSP.IMEM + PC);
        const unsigned short imm = *(short *)(RSP.IMEM + PC + HES(0x002));
        const short IW_upper_16  = *(short *)(RSP.IMEM + PC + HES(0x000));

#ifdef EMULATE_STATIC_PC
        if (stage != 0) /* stage == 1 */
        {
            PC = *RSP.SP_PC_REG & 0x00000FFC;
            stage = 0;
        }
        else
        {
            PC = (PC + 0x004) & 0xFFC;
            *RSP.SP_PC_REG = 0x04001000 + PC;
        }
#endif
#ifdef SP_EXECUTE_LOG
        step_SP_commands(inst);
#endif
        if (inst >> 25 == 0x25) /* is a VU instruction */
        {
            const int opcode = inst % 64; /* inst.R.func */
            const int vd = (inst & 0x000007FF) >> 6; /* inst.R.sa */
            const int vs = imm >> 11; /* inst.R.rd */
            const int vt = IW_upper_16 & 31; /* inst.R.rt */
            const int e  = (inst & 0x01FFFFFF) >> 21; /* rs & 0xF */

            COP2_C2[opcode](vd, vs, vt, e);
        }
        else
        {
            const int op = inst >> 26;
            const int rs = inst >> 21; /* &= 31 */
            const int rt = IW_upper_16 & 31;
            const int rd = imm >> 11;
            const int sa = (inst/*& 0x000007FF*/) >> 6;
            const int func = (inst & 0x0000003F) >> 0;
            const int element = (sa & 31) >> 1;
            const int base = rs & 31;

#if (0)
            SR[0] = 0x00000000; /* already handled on per-instruction basis */
#endif
            switch (op)
            {
                signed int offset;
                register unsigned long addr;

                case 000: /* SPECIAL */
                    if (imm == 0)
                        CONTINUE
                    switch (func)
                    {
                        case 000: /* SLL */
                            SR[rd] = SR[rt] << MASK_SA(imm >> 6);
                            SR[0] = 0x00000000;
                            CONTINUE
                        case 002: /* SRL */
                            SR[rd] = (unsigned)(SR[rt]) >> MASK_SA(imm >> 6);
                            SR[0] = 0x00000000;
                            CONTINUE
                        case 003: /* SRA */
                            SR[rd] = (signed)(SR[rt]) >> MASK_SA(imm >> 6);
                            SR[0] = 0x00000000;
                            CONTINUE
                        case 004: /* SLLV */
                            SR[rd] = SR[rt] << MASK_SA(SR[rs]);
                            SR[0] = 0x00000000;
                            CONTINUE
                        case 006: /* SRLV */
                            SR[rd] = (unsigned)(SR[rt]) >> MASK_SA(SR[rs]);
                            SR[0] = 0x00000000;
                            CONTINUE
                        case 007: /* SRAV */
                            SR[rd] = (signed)(SR[rt]) >> MASK_SA(SR[rs]);
                            SR[0] = 0x00000000;
                            CONTINUE
                        case 011: /* JALR */
                            SR[rd] = (*RSP.SP_PC_REG + LINK_OFF) & 0x00000FFC;
                            SR[0] = 0x00000000;
                        case 010: /* JR */
                            set_PC(SR[rs]);
                            CONTINUE
                        case 015: /* BREAK */
                            *RSP.SP_STATUS_REG |= 0x00000003; /* BROKE | HALT */
                            if (*RSP.SP_STATUS_REG & 0x00000040)
                            { /* SP_STATUS_INTR_BREAK */
                                *RSP.MI_INTR_REG |= 0x00000001;
                                RSP.CheckInterrupts();
                            }
                            return;
                        case 040: /* ADD */
                        case 041: /* ADDU */
                            SR[rd] = SR[rs] + SR[rt];
                            SR[0] = 0x00000000; /* needed for Rareware ucodes */
                            CONTINUE
                        case 042: /* SUB */
                        case 043: /* SUBU */
                            SR[rd] = SR[rs] - SR[rt];
                            SR[0] = 0x00000000;
                            CONTINUE
                        case 044: /* AND */
                            SR[rd] = SR[rs] & SR[rt];
                            SR[0] = 0x00000000; /* needed for Rareware ucodes */
                            CONTINUE
                        case 045: /* OR */
                            SR[rd] = SR[rs] | SR[rt];
                            SR[0] = 0x00000000;
                            CONTINUE
                        case 046: /* XOR */
                            SR[rd] = SR[rs] ^ SR[rt];
                            SR[0] = 0x00000000;
                            CONTINUE
                        case 047: /* NOR */
                            SR[rd] = ~(SR[rs] | SR[rt]);
                            SR[0] = 0x00000000;
                            CONTINUE
                        case 052: /* SLT */
                            SR[rd] = ((signed)(SR[rs]) < (signed)(SR[rt]));
                            SR[0] = 0x00000000;
                            CONTINUE
                        case 053: /* SLTU */
                            SR[rd] = ((unsigned)(SR[rs]) < (unsigned)(SR[rt]));
                            SR[0] = 0x00000000;
                            CONTINUE
                        default:
                            res_S();
                            CONTINUE
                    }
                    CONTINUE
                case 001: /* REGIMM */
                    switch (rt)
                    {
                        case 020: /* BLTZAL */
                            SR[31] = (*RSP.SP_PC_REG + LINK_OFF) & 0x00000FFC;
                        case 000: /* BLTZ */
                            if (!(SR[rs & 31] < 0))
                                CONTINUE
                            set_PC(*RSP.SP_PC_REG + 4*imm + SLOT_OFF);
                            CONTINUE
                        case 021: /* BGEZAL */
                            SR[31] = (*RSP.SP_PC_REG + LINK_OFF) & 0x00000FFC;
                        case 001: /* BGEZ */
                            if (!(SR[rs & 31] >= 0))
                                CONTINUE
                            set_PC(*RSP.SP_PC_REG + 4*imm + SLOT_OFF);
                            CONTINUE
                        default:
                            res_S();
                            CONTINUE
                    }
                    CONTINUE
                case 003: /* JAL */
                    SR[31] = (*RSP.SP_PC_REG + LINK_OFF) & 0x00000FFC;
                case 002: /* J */
                    set_PC(4*imm);
                    CONTINUE
                case 004: /* BEQ */
                    if (!(SR[rs & 31] == SR[rt]))
                        CONTINUE
                    set_PC(*RSP.SP_PC_REG + 4*imm + SLOT_OFF);
                    CONTINUE
                case 005: /* BNE */
                    if (!(SR[rs & 31] != SR[rt]))
                        CONTINUE
                    set_PC(*RSP.SP_PC_REG + 4*imm + SLOT_OFF);
                    CONTINUE
                case 006: /* BLEZ */
                    if (!((signed)SR[rs & 31] <= 0x00000000))
                        CONTINUE
                    set_PC(*RSP.SP_PC_REG + 4*imm + SLOT_OFF);
                    CONTINUE
                case 007: /* BGTZ */
                    if (!((signed)SR[rs & 31] >  0x00000000))
                        CONTINUE
                    set_PC(*RSP.SP_PC_REG + 4*imm + SLOT_OFF);
                    CONTINUE
                case 010: /* ADDI */
                case 011: /* ADDIU */
                    SR[rt] = SR[rs & 31] + (signed short)(imm);
                    SR[0] = 0x00000000;
                    CONTINUE
                case 012: /* SLTI */
                    SR[rt] = ((signed)(SR[rs & 31]) < (signed short)(imm));
                    SR[0] = 0x00000000;
                    CONTINUE
                case 013: /* SLTIU */
                    SR[rt] = ((unsigned)(SR[rs & 31]) < imm);
                    SR[0] = 0x00000000;
                    CONTINUE
                case 014: /* ANDI */
                    SR[rt] = SR[rs & 31] & imm;
                    SR[0] = 0x00000000;
                    CONTINUE
                case 015: /* ORI */
                    SR[rt] = SR[rs & 31] | imm;
                    SR[0] = 0x00000000;
                    CONTINUE
                case 016: /* XORI */
                    SR[rt] = SR[rs & 31] ^ imm;
                    SR[0] = 0x00000000;
                    CONTINUE
                case 017: /* LUI */
                    SR[rt] = imm << 16;
                    SR[0] = 0x00000000;
                    CONTINUE
                case 020: /* COP0 */
                    switch (rs & 31)
                    {
                        case 000: /* MFC0 */
                            MFC0(rt, rd & 0xF);
                            CONTINUE
                        case 004: /* MTC0 */
                            MTC0[rd & 0xF](rt);
                            CONTINUE
                        default:
                            res_S();
                            CONTINUE
                    }
                    CONTINUE
                case 022: /* COP2 */
                    switch (rs & 31)
                    {
                        case 000: /* MFC2 */
                            MFC2(rt, rd, element);
                            CONTINUE
                        case 002: /* CFC2 */
                            CFC2(rt, rd);
                            CONTINUE
                        case 004: /* MTC2 */
                            MTC2(rt, rd, element);
                            CONTINUE
                        case 006: /* CTC2 */
                            CTC2(rt, rd);
                            CONTINUE
                        default:
                            res_S();
                            CONTINUE
                    }
                    CONTINUE
                case 040: /* LB */
                    offset = (signed short)(imm);
                    addr = (SR[base] + offset) & 0x00000FFF;
                    SR[rt] = RSP.DMEM[BES(addr)];
                    SR[rt] = (signed char)(SR[rt]);
                    SR[0] = 0x00000000;
                    CONTINUE
                case 041: /* LH */
                    offset = (signed short)(imm);
                    addr = (SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 == 0x003)
                    {
                        SR_B(rt, 2) = RSP.DMEM[addr - BES(0x000)];
                        addr = (addr + 0x00000001) & 0x00000FFF;
                        SR_B(rt, 3) = RSP.DMEM[addr + BES(0x000)];
                        SR[rt] = (signed short)(SR[rt]);
                    }
                    else
                    {
                        addr -= HES(0x000)*(addr%0x004 - 1);
                        SR[rt] = *(signed short *)(RSP.DMEM + addr);
                    }
                    SR[0] = 0x00000000;
                    CONTINUE
                case 043: /* LW */
                    offset = (signed short)(imm);
                    addr = (SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 != 0x000)
                        ULW(rt, addr);
                    else
                        SR[rt] = *(int32_t *)(RSP.DMEM + addr);
                    SR[0] = 0x00000000;
                    CONTINUE
                case 044: /* LBU */
                    offset = (signed short)(imm);
                    addr = (SR[base] + offset) & 0x00000FFF;
                    SR[rt] = RSP.DMEM[BES(addr)];
                    SR[rt] = (unsigned char)(SR[rt]);
                    SR[0] = 0x00000000;
                    CONTINUE
                case 045: /* LHU */
                    offset = (signed short)(imm);
                    addr = (SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 == 0x003)
                    {
                        SR_B(rt, 2) = RSP.DMEM[addr - BES(0x000)];
                        addr = (addr + 0x00000001) & 0x00000FFF;
                        SR_B(rt, 3) = RSP.DMEM[addr + BES(0x000)];
                        SR[rt] = (unsigned short)(SR[rt]);
                    }
                    else
                    {
                        addr -= HES(0x000)*(addr%0x004 - 1);
                        SR[rt] = *(unsigned short *)(RSP.DMEM + addr);
                    }
                    SR[0] = 0x00000000;
                    CONTINUE
                case 050: /* SB */
                    offset = (signed short)(imm);
                    addr = (SR[base] + offset) & 0x00000FFF;
                    RSP.DMEM[BES(addr)] = (unsigned char)(SR[rt]);
                    CONTINUE
                case 051: /* SH */
                    offset = (signed short)(imm);
                    addr = (SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 == 0x003)
                    {
                        RSP.DMEM[addr - BES(0x000)] = SR_B(rt, 2);
                        addr = (addr + 0x00000001) & 0x00000FFF;
                        RSP.DMEM[addr + BES(0x000)] = SR_B(rt, 3);
                        CONTINUE
                    }
                    addr -= HES(0x000)*(addr%0x004 - 1);
                    *(short *)(RSP.DMEM + addr) = (short)(SR[rt]);
                    CONTINUE
                case 053: /* SW */
                    offset = (signed short)(imm);
                    addr = (SR[base] + offset) & 0x00000FFF;
                    if (addr%0x004 != 0x000)
                        USW(rt, addr);
                    else
                        *(int32_t *)(RSP.DMEM + addr) = SR[rt];
                    CONTINUE
                case 062: /* LWC2 */
                    offset = SE(imm, 6);
                    switch (rd)
                    {
                        case 000: /* LBV */
                            LBV(rt, element, offset, base);
                            CONTINUE
                        case 001: /* LSV */
                            LSV(rt, element, offset, base);
                            CONTINUE
                        case 002: /* LLV */
                            LLV(rt, element, offset, base);
                            CONTINUE
                        case 003: /* LDV */
                            LDV(rt, element, offset, base);
                            CONTINUE
                        case 004: /* LQV */
                            LQV(rt, element, offset, base);
                            CONTINUE
                        case 005: /* LRV */
                            LRV(rt, element, offset, base);
                            CONTINUE
                        case 006: /* LPV */
                            LPV(rt, element, offset, base);
                            CONTINUE
                        case 007: /* LUV */
                            LUV(rt, element, offset, base);
                            CONTINUE
                        case 010: /* LHV */
                            LHV(rt, element, offset, base);
                            CONTINUE
                        case 011: /* LFV */
                            LFV(rt, element, offset, base);
                            CONTINUE
                        case 013: /* LTV */
                            LTV(rt, element, offset, base);
                            CONTINUE
                        default:
                            res_S();
                            CONTINUE
                    }
                    CONTINUE
                case 072: /* SWC2 */
                    offset = SE(imm, 6);
                    switch (rd)
                    {
                        case 000: /* SBV */
                            SBV(rt, element, offset, base);
                            CONTINUE
                        case 001: /* SSV */
                            SSV(rt, element, offset, base);
                            CONTINUE
                        case 002: /* SLV */
                            SLV(rt, element, offset, base);
                            CONTINUE
                        case 003: /* SDV */
                            SDV(rt, element, offset, base);
                            CONTINUE
                        case 004: /* SQV */
                            SQV(rt, element, offset, base);
                            CONTINUE
                        case 005: /* SRV */
                            SRV(rt, element, offset, base);
                            CONTINUE
                        case 006: /* SPV */
                            SPV(rt, element, offset, base);
                            CONTINUE
                        case 007: /* SUV */
                            SUV(rt, element, offset, base);
                            CONTINUE
                        case 010: /* SHV */
                            SHV(rt, element, offset, base);
                            CONTINUE
                        case 011: /* SFV */
                            SFV(rt, element, offset, base);
                            CONTINUE
                        case 012: /* SWV */
                            SWV(rt, element, offset, base);
                            CONTINUE
                        case 013: /* STV */
                            STV(rt, element, offset, base);
                            CONTINUE
                        default:
                            res_S();
                            CONTINUE
                    }
                    CONTINUE
                default:
                    res_S();
                    CONTINUE
            }
        }
#ifndef EMULATE_STATIC_PC
        if (stage == 2) /* branch phase of scheduler */
        {
            stage = 0*stage;
            PC = temp_PC & 0x00000FFC;
            *RSP.SP_PC_REG = temp_PC;
        }
        else
        {
            stage = 2*stage; /* next IW in branch delay slot? */
            PC = (PC + 0x004) & 0xFFC;
            *RSP.SP_PC_REG = 0x04001000 + PC;
        }
        continue;
#endif
    }
    if (*RSP.SP_STATUS_REG & 0x00000002) /* normal exit, from executing BREAK */
        return;
    else if (*RSP.MI_INTR_REG & 0x00000001) /* interrupt set by MTC0 to break */
        RSP.CheckInterrupts();
    else if (CFG_WAIT_FOR_CPU_HOST != 0) /* plugin system hack to re-sync */
        {}
    else if (*RSP.SP_SEMAPHORE_REG != 0x00000000) /* semaphore lock fixes */
        {}
    else /* ??? unknown, possibly external intervention from CPU memory map */
    {
        message("SP_SET_HALT", 3);
        return;
    }
    *RSP.SP_STATUS_REG &= ~0x00000001; /* CPU restarts with the correct SIGs. */
    return;
}
