.. _tesla-int:

===============================
Integer arithmetic instructions
===============================

.. contents::


Introduction
============

.. todo:: write me

::

    S(x): 31th bit of x for 32-bit x, 15th for 16-bit x.
    SEX(x): sign-extension of x
    ZEX(x): zero-extension of x


Addition/substraction: (h)add, (h)sub, (h)subr, (h)addc
=======================================================

.. todo:: write me

::

  add [sat] b32/b16 [CDST] DST SRC1 SRC2        O2=0, O1=0
  sub [sat] b32/b16 [CDST] DST SRC1 SRC2        O2=0, O1=1
  subr [sat] b32/b16 [CDST] DST SRC1 SRC2       O2=1, O1=0
  addc [sat] b32/b16 [CDST] DST SRC1 SRC2 COND      O2=1, O1=1

  All operands are 32-bit or 16-bit according to size specifier.

    b16/b32 s1, s2;
    bool c;
    switch (OP) {
        case add: s1 = SRC1, s2 = SRC2, c = 0; break;
        case sub: s1 = SRC1, s2 = ~SRC2, c = 1; break;
        case subr: s1 = ~SRC1, s2 = SRC2, c = 1; break;
        case addc: s1 = SRC1, s2 = SRC2, c = COND.C; break;
    }
    res = s1+s2+c;  // infinite precision
    CDST.C = res >> (b32 ? 32 : 16);
    res = res & (b32 ? 0xffffffff : 0xffff);
    CDST.O = (S(s1) == S(s2)) && (S(s1) != S(res));
    if (sat && CDST.O)
        if (S(res)) res = (b32 ? 0x7fffffff : 0x7fff);
        else res = (b32 ? 0x80000000 : 0x8000);
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Short/imm:    0x20000000 base opcode
        0x10000000 O2 bit
        0x00400000 O1 bit
        0x00008000 0: b16, 1: b32
        0x00000100 sat flag
        operands: S*DST, S*SRC1/S*SHARED, S*SRC2/S*CONST/IMM, $c0

  Long:     0x20000000 0x00000000 base opcode
        0x10000000 0x00000000 O2 bit
        0x00400000 0x00000000 O1 bit
        0x00000000 0x04000000 0: b16, 1: b32
        0x00000000 0x08000000 sat flag
        operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC3/L*CONST3, COND


Multiplication: mul(24)
=======================

.. todo:: write me

::

  mul [CDST] DST u16/s16 SRC1 u16/s16 SRC2

  DST is 32-bit, SRC1 and SRC2 are 16-bit.

    b32 s1, s2;
    if (src1_signed)
        s1 = SEX(SRC1);
    else
        s1 = ZEX(SRC1);
    if (src2_signed)
        s2 = SEX(SRC2);
    else
        s2 = ZEX(SRC2);
    b32 res = s1*s2;    // modulo 2^32
    CDST.O = 0;
    CDST.C = 0;
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Short/imm:    0x40000000 base opcode
        0x00008000 src1 is signed
        0x00000100 src2 is signed
        operands: SDST, SHSRC/SHSHARED, SHSRC2/SHCONST/IMM

  Long:     0x40000000 0x00000000 base opcode
        0x00000000 0x00008000 src1 is signed
        0x00000000 0x00004000 src2 is signed
        operands: MCDST, LLDST, LHSRC1/LHSHARED, LHSRC2/LHCONST2

::

  mul [CDST] DST [high] u24/s24 SRC1 SRC2

  All operands are 32-bit.

    b48 s1, s2;
    if (signed) {
        s1 = SEX((b24)SRC1);
        s2 = SEX((b24)SRC2);
    } else {
        s1 = ZEX((b24)SRC1);
        s2 = ZEX((b24)SRC2);
    }
    b48 m = s1*s2;  // modulo 2^48
    b32 res = (high ? m >> 16 : m & 0xffffffff);
    CDST.O = 0;
    CDST.C = 0;
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Short/imm:    0x40000000 base opcode
        0x00008000 src are signed
        0x00000100 high
        operands: SDST, SSRC/SSHARED, SSRC2/SCONST/IMM

  Long:     0x40000000 0x00000000 base opcode
        0x00000000 0x00008000 src are signed
        0x00000000 0x00004000 high
        operands: MCDST, LLDST, LSRC1/LSHARED, LSRC2/LCONST2


Multiply-add: madd(24), msub(24), msubr(24), maddc(24)
======================================================

.. todo:: write me

::

  addop [CDST] DST mul u16 SRC1 SRC2 SRC3       O1=0 O2=000 S2=0 S1=0
  addop [CDST] DST mul s16 SRC1 SRC2 SRC3       O1=0 O2=001 S2=0 S1=1
  addop sat [CDST] DST mul s16 SRC1 SRC2 SRC3       O1=0 O2=010 S2=1 S1=0
  addop [CDST] DST mul u24 SRC1 SRC2 SRC3       O1=0 O2=011 S2=1 S1=1
  addop [CDST] DST mul s24 SRC1 SRC2 SRC3       O1=0 O2=100
  addop sat [CDST] DST mul s24 SRC1 SRC2 SRC3       O1=0 O2=101
  addop [CDST] DST mul high u24 SRC1 SRC2 SRC3  O1=0 O2=110
  addop [CDST] DST mul high s24 SRC1 SRC2 SRC3  O1=0 O2=111
  addop sat [CDST] DST mul high s24 SRC1 SRC2 SRC3  O1=1 O2=000

  addop is one of:

  add   O3=00   S4=0 S3=0
  sub   O3=01   S4=0 S3=1
  subr  O3=10   S4=1 S3=0
  addc  O3=11   S4=1 S3=1

  If addop is addc, insn also takes an additional COND parameter. DST and
  SRC3 are always 32-bit, SRC1 and SRC2 are 16-bit for u16/s16 variants,
  32-bit for u24/s24 variants. Only a few of the variants are encodable as
  short/immediate, and they're restricted to DST=SRC3.

    if (u24 || s24) {
        b48 s1, s2;
        if (s24) {
            s1 = SEX((b24)SRC1);
            s2 = SEX((b24)SRC2);
        } else {
            s1 = ZEX((b24)SRC1);
            s2 = ZEX((b24)SRC2);
        }
        b48 m = s1*s2;  // modulo 2^48
        b32 mres = (high ? m >> 16 : m & 0xffffffff);
    } else {
        b32 s1, s2;
        if (s16) {
            s1 = SEX(SRC1);
            s2 = SEX(SRC2);
        } else {
            s1 = ZEX(SRC1);
            s2 = ZEX(SRC2);
        }
        b32 mres = s1*s2;   // modulo 2^32
    }
    b32 s1, s2;
    bool c;
    switch (OP) {
        case add: s1 = mres, s2 = SRC3, c = 0; break;
        case sub: s1 = mres, s2 = ~SRC3, c = 1; break;
        case subr: s1 = ~mres, s2 = SRC3, c = 1; break;
        case addc: s1 = mres, s2 = SRC3, c = COND.C; break;
    }
    res = s1+s2+c;  // infinite precision
    CDST.C = res >> 32;
    res = res & 0xffffffff;
    CDST.O = (S(s1) == S(s2)) && (S(s1) != S(res));
    if (sat && CDST.O)
        if (S(res)) res = 0x7fffffff;
        else res = 0x80000000;
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Short/imm:    0x60000000 base opcode
        0x00000100 S1
        0x00008000 S2
        0x00400000 S3
        0x10000000 S4
        operands: SDST, S*SRC/S*SHARED, S*SRC2/S*CONST/IMM, SDST, $c0

  Long:     0x60000000 0x00000000 base opcode
        0x10000000 0x00000000 O1
        0x00000000 0xe0000000 O2
        0x00000000 0x0c000000 O3
        operands: MCDST, LLDST, L*SRC1/L*SHARED, L*SRC2/L*CONST2, L*SRC3/L*CONST3, COND


Sum of absolute differences: sad, hsad
======================================

.. todo:: write me

::

  sad [CDST] DST u16/s16/u32/s32 SRC1 SRC2 SRC3

  Short variant is restricted to DST same as SRC3. All operands are 32-bit or
  16-bit according to size specifier.

    int s1, s2; // infinite precision
    if (signed) {
        s1 = SEX(SRC1);
        s2 = SEX(SRC2);
    } else {
        s1 = ZEX(SRC1);
        s2 = ZEX(SRC2);
    }
    b32 mres = abs(s1-s2);  // modulo 2^32
    res = mres+s3;      // infinite precision
    CDST.C = res >> (b32 ? 32 : 16);
    res = res & (b32 ? 0xffffffff : 0xffff);
    CDST.O = (S(mres) == S(s3)) && (S(mres) != S(res));
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Short:    0x50000000 base opcode
        0x00008000 0: b16 1: b32
        0x00000100 src are signed
        operands: DST, SDST, S*SRC/S*SHARED, S*SRC2/S*CONST, SDST

  Long:     0x50000000 0x00000000 base opcode
        0x00000000 0x04000000 0: b16, 1: b32
        0x00000000 0x08000000 src sre signed
        operands: MCDST, LLDST, L*SRC1/L*SHARED, L*SRC2/L*CONST2, L*SRC3/L*CONST3


Min/max selection: (h)min, (h)max
=================================

.. todo:: write me

::

  min u16/u32/s16/s32 [CDST] DST SRC1 SRC2
  max u16/u32/s16/s32 [CDST] DST SRC1 SRC2

  All operands are 32-bit or 16-bit according to size specifier.

    if (SRC1 < SRC2) { // signed comparison for s16/s32, unsigned for u16/u32.
        res = (min ? SRC1 : SRC2);
    } else {
        res = (min ? SRC2 : SRC1);
    }
    CDST.O = 0;
    CDST.C = 0;
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Long:     0x30000000 0x80000000 base opcode
        0x00000000 0x20000000 0: max, 1: min
        0x00000000 0x08000000 0: u16/u32, 1: s16/s32
        0x00000000 0x04000000 0: b16, 1: b32
        operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC2/L*CONST2


Comparison: set, hset
=====================

.. todo:: write me

::

  set [CDST] DST cond u16/s16/u32/s32 SRC1 SRC2

  cond can be any subset of {l, g, e}.

  All operands are 32-bit or 16-bit according to size specifier.

    int s1, s2; // infinite precision
    if (signed) {
        s1 = SEX(SRC1);
        s2 = SEX(SRC2);
    } else {
        s1 = ZEX(SRC1);
        s2 = ZEX(SRC2);
    }
    bool c;
    if (s1 < s2)
        c = cond.l;
    else if (s1 == s2)
        c = cond.e;
    else /* s1 > s2 */
        c = cond.g;
    if (c) {
        res = (b32?0xffffffff:0xffff);
    } else {
        res = 0;
    }
    CDST.O = 0;
    CDST.C = 0;
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Long:     0x30000000 0x60000000 base opcode
        0x00000000 0x08000000 0: u16/u32, 1: s16/s32
        0x00000000 0x04000000 0: b16, 1: b32
        0x00000000 0x00010000 cond.g
        0x00000000 0x00008000 cond.e
        0x00000000 0x00004000 cond.l
        operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC2/L*CONST2


Bitwise operations: (h)and, (h)or, (h)xor, (h)mov2
==================================================

.. todo:: write me

::

  and b32/b16 [CDST] DST [not] SRC1 [not] SRC2      O2=0, O1=0
  or b32/b16 [CDST] DST [not] SRC1 [not] SRC2       O2=0, O1=1
  xor b32/b16 [CDST] DST [not] SRC1 [not] SRC2      O2=1, O1=0
  mov2 b32/b16 [CDST] DST [not] SRC1 [not] SRC2     O2=1, O1=1

  Immediate forms only allows 32-bit operands, and cannot negate second op.

    s1 = (not1 ? ~SRC1 : SRC1);
    s2 = (not2 ? ~SRC2 : SRC2);
    switch (OP) {
        case and: res = s1 & s2; break;
        case or: res = s1 | s2; break;
        case xor: res = s1 ^ s2; break;
        case mov2: res = s2; break;
    }
    CDST.O = 0;
    CDST.C = 0;
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Imm:      0xd0000000 base opcode
        0x00400000 not1
        0x00008000 O2 bit
        0x00000100 O1 bit
        operands: SDST, SSRC/SSHARED, IMM
        assumed: not2=0 and b32.

  Long:     0xd0000000 0x00000000 base opcode
        0x00000000 0x04000000 0: b16, 1: b32
        0x00000000 0x00020000 not2
        0x00000000 0x00010000 not1
        0x00000000 0x00008000 O2 bit
        0x00000000 0x00004000 O1 bit
        operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC2/L*CONST2


Bit shifts: (h)shl, (h)shr, (h)sar
==================================

.. todo:: write me

::

  shl b16/b32 [CDST] DST SRC1 SRC2
  shl b16/b32 [CDST] DST SRC1 SHCNT
  shr u16/u32 [CDST] DST SRC1 SRC2
  shr u16/u32 [CDST] DST SRC1 SHCNT
  shr s16/s32 [CDST] DST SRC1 SRC2
  shr s16/s32 [CDST] DST SRC1 SHCNT

    All operands 16/32-bit according to size specifier, except SHCNT. Shift
    counts are always treated as unsigned, passing negative value to shl
    doesn't get you a shr.

        int size = (b32 ? 32 : 16);
    if (shl) {
        res = SRC1 << SRC2; // infinite precision, shift count doesn't wrap.
        if (SRC2 < size) { // yes, <. So if you shift 1 left by 32 bits, you DON'T get CDST.C set. but shift 2 left by 31 bits, and it gets set just fine.
            CDST.C = (res >> size) & 1; // basically, the bit that got shifted out.
        } else {
            CDST.C = 0;
        }
        res = res & (b32 ? 0xffffffff : 0xffff);
    } else {
        res = SRC1 >> SRC2; // infinite precision, shift count doesn't wrap.
        if (signed && S(SRC1)) {
            if (SRC2 < size)
                res |= (1<<size)-(1<<(size-SRC2)); // fill out the upper bits with 1's.
            else
                res |= (1<<size)-1;
        }
        if (SRC2 < size && SRC2 > 0) {
            CDST.C = (SRC1 >> (SRC2-1)) & 1;
        } else {
            CDST.C = 0;
        }
    }
    if (SRC2 == 1) {
        CDST.O = (S(SRC1) != S(res));
    } else {
        CDST.O = 0;
    }
    CDST.S = S(res);
    CDST.Z = res == 0;
    DST = res;

  Long:     0x30000000 0xc0000000 base opcode
        0x00000000 0x20000000 0: shl, 1: shr
        0x00000000 0x08000000 0: u16/u32, 1: s16/s32 [shr only]
        0x00000000 0x04000000 0: b16, 1: b32
        0x00000000 0x00010000 0: use SRC2, 1: use SHCNT
        operands: MCDST, LL*DST, L*SRC1/L*SHARED, L*SRC2/L*CONST2/SHCNT
