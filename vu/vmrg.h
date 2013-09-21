#include "vu.h"

INLINE static void do_mrg(short* VD, short* VS, short* VT)
{
    register int i;

    for (i = 0; i < N; i++)
        ACC_L(i) = comp[i] ? VS[i] : VT[i];
    memcpy(VD, VACC_L, N*sizeof(short));
    return;
}

static void VMRG(void)
{
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;

    do_mrg(VR[vd], VR[vs], ST);
    return;
}

static void VMRG_v(void)
{
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    do_mrg(VR[vd], VR[vs], VR[vt]);
    return;
}
static void VMRG0q(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x2);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG1q(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x3);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG0h(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x4);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG1h(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x5);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG2h(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x6);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG3h(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x7);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG0w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x8);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG1w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0x9);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG2w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0xA);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG3w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0xB);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG4w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0xC);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG5w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0xD);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG6w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0xE);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
static void VMRG7w(void)
{
    short SV[N];
    const int vd = inst.R.sa;
    const int vs = inst.R.rd;
    const int vt = inst.R.rt;

    SHUFFLE_VECTOR(SV, VR[vt], 0xF);
    do_mrg(VR[vd], VR[vs], SV);
    return;
}
