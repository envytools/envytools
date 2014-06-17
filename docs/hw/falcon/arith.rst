.. _falcon-arith:

=======================
Arithmetic instructions
=======================

.. contents::


Introduction
============

The arithmetic/logical instructions do operations on $r0-$r15 GPRs, sometimes
setting bits in $flags register according to the result. The instructions
can be "sized" or "unsized". Sized instructions have 8-bit, 16-bit, and 32-bit
variants. Unsized instructions don't have variants, and always operate on
full 32-bit registers. For 8-bit and 16-bit sized instructions, high 24 or 16
bits of destination registers are unmodified.


.. _falcon-flags-arith:

$flags result bits
==================

The :ref:`$flags <falcon-sr-flags>` bits often affected by ALU instructions are:

- bit 8: c, carry flag
- bit 9: o, signed overflow flag
- bit 10: s, sign flag
- bit 11: z, zero flag

Also, a few ALU instructions operate on $flags register as a whole.


.. _falcon-arith-conventions:

Pseudocode conventions
======================

All operations are done in infinite-precision arithmetic, all temporaries
are infinite-precision too.

sz, for sized instructions, is the selected size of operation: 8, 16, or 32.

S(x) evaluates to (x >> (sz - 1) & 1), ie. the sign bit of x. If insn
is unsized, assume sz = 32.


.. _falcon-isa-cmp:

Comparison: cmpu, cmps, cmp
===========================

Compare two values, setting flags according to results of comparison. cmp
sets the usual set of 4 flags. cmpu sets only c and z. cmps sets z normally,
and sets c if SRC1 is less then SRC2 when treated as signed number.

cmpu/cmps are the only comparison instructions available on falcon v0. Both of
them set only the c and z flags, with cmps setting c flag in an unusual way
to enable signed comparisons while using unsigned flags and condition codes.
To do an unsigned comparison, use cmpu and the unsigned branch conditions
[b/a/e]. To do a signed comparison, use cmps, also with unsigned branch
conditions.

The falcon v3+ new cmp instruction sets the full set of flags. To do an unsigned
comparison on v3+, use cmp and the unsigned branch conditions. To do a signed
comparison, use cmp and the signed branch conditions [l/g/e].

Instructions:
    ==== ================ ========== =========
    Name Description      Present on Subopcode
    ==== ================ ========== =========
    cmpu compare unsigned all units  4
    cmps compare signed   all units  5
    cmp  compare          v3+ units  6
    ==== ================ ========== =========
Instruction class:
    sized
Execution time:
    1 cycle
Operands:
    SRC1, SRC2
Forms:
    ========== ======
    Form       Opcode
    ========== ======
    R2, I8     30
    R2, I16    31
    R2, R1     38
    ========== ======
Immediates:
    cmpu:
        zero-extended
    cmps:
        sign-extended
    cmp:
        sign-extended
Operation:
    ::

        diff = SRC1 - SRC2; // infinite precision
        S = S(diff);
        O = S(SRC1) != S(SRC2) && S(SRC1) != S(diff);
        $flags.z = (diff == 0);
        if (op == cmps)
                $flags.c = S ^ O;
        else if (op == cmpu)
                $flags.c = diff >> sz & 1;
        else if (op == cmp) {
                $flags.c = diff >> sz & 1;
                $flags.o = O;
                $flags.s = S;
        }


.. _falcon-isa-add:

Addition/substraction: add, adc, sub, sbb
=========================================

Add or substract two values, possibly with carry/borrow.

Instructions:
    ==== ========================= =========
    Name Description               Subopcode
    ==== ========================= =========
    add  add                       0
    adc  add with carry            1
    sub  substract                 2
    sbb  substrace with borrow     3
    ==== ========================= =========
Instruction class:
    sized
Execution time:
    1 cycle
Operands:
    DST, SRC1, SRC2
Forms:
    =========== ======
    Form        Opcode
    =========== ======
    R1, R2, I8  10
    R1, R2, I16 20
    R2, R2, I8  36
    R2, R2, I16 37
    R2, R2, R1  3b
    R3, R2, R1  3c
    =========== ======
Immediates:
    zero-extended
Operation:
    ::

        s2 = SRC2;
        if (op == adc || op == sbb)
                s2 += $flags.c;
        if (op == sub || op == sbb)
                s2 = -s2;
        res = SRC1 + s2;
        DST = res;
        $flags.c = (res >> sz) & 1;
        if (op == add || op == adc) {
                $flags.o = S(SRC1) == S(SRC2) && S(SRC1) != S(res);
        } else {
                $flags.o = S(SRC1) != S(SRC2) && S(SRC1) != S(res);
        }
        $flags.s = S(DST);
        $flags.z = (DST == 0);


.. _falcon-isa-shift:

Shifts: shl, shr, sar, shlc, shrc
=================================

Shift a value. For shl/shr, the extra bits "shifted in" are 0. For sar,
they're equal to sign bit of source. For shlc/shrc, the first such bit
is taken from carry flag, the rest are 0.

Instructions:
    ==== ========================= =========
    Name Description               Subopcode
    ==== ========================= =========
    shl  shift left                4
    shr  shift right               5
    sar  shift right with sign bit 6
    shlc shift left with carry in  c
    shrc shift right with carry in d
    ==== ========================= =========
Instruction class:
    sized
Execution time:
    1 cycle
Operands:
    DST, SRC1, SRC2
Forms:
    ========== ======
    Form       Opcode
    ========== ======
    R1, R2, I8 10
    R2, R2, I8 36
    R2, R2, R1 3b
    R3, R2, R1 3c
    ========== ======
Immediates:
    truncated
Operation:
    ::

        if (sz == 8)
                shcnt = SRC2 & 7;
        else if (sz == 16)
                shcnt = SRC2 & 0xf;
        else // sz == 32
                shcnt = SRC2 & 0x1f;
        if (op == shl || op == shlc) {
                res = SRC1 << shcnt;
                if (op == shlc && shcnt != 0)
                        res |= $flags.c << (shcnt - 1);
                $flags.c = res >> sz & 1;
                DST = res;
        } else { // shr, sar, shrc
                res = SRC1 >> shcnt;
                if (op == shrc && shcnt != 0)
                        res |= $flags.c << (sz - shcnt);
                if (op == sar && S(SRC1))
                        res |= ~0 << (sz - shcnt);
                if (shcnt == 0)
                        $flags.c = 0;
                else
                        $flags.c = SRC1 >> (shcnt - 1) & 1;
                DST = res;
        }
        if (falcon_version != 0) {
                $flags.o = 0;
                $flags.s = S(DST);
                $flags.z = (DST == 0);
        }


.. _falcon-isa-unary:

Unary operations: not, neg, mov, movf, hswap
============================================

not flips all bits in a value. neg negates a value. mov and movf move a value
from one register to another. mov is the v3+ variant, which just does the
move. movf is the v0 variant, which additionally sets flags according to the
moved value. hswap rotates a value by half its size.

Instructions:
    ===== =========================== ========== =========
    Name  Description                 Present on Subopcode
    ===== =========================== ========== =========
    not   bitwise complement          all units  0
    neg   negate a value              all units  1
    movf  move a value and set flags  v0 units   2
    mov   move a value                v3+ units  2
    hswap Swap halves                 all units  3
    ===== =========================== ========== =========
Instruction class:
    sized
Execution time:
    1 cycle
Operands:
    DST, SRC
Forms:
    ====== ======
    Form   Opcode
    ====== ======
    R1, R2 39
    R2, R2 3d
    ====== ======
Operation:
    ::

        if (op == not) {
                DST = ~SRC;
                $flags.o = 0;
        } else if (op == neg) {
                DST = -SRC;
                $flags.o = (DST == 1 << (sz - 1));
        } else if (op == movf) {
                DST = SRC;
                $flags.o = 0;
        } else if (op == mov) {
                DST = SRC;
        } else if (op == hswap) {
                DST = SRC >> (sz / 2) | SRC << (sz / 2);
                $flags.o = 0;
        }
        if (op != mov) {
                $flags.s = S(DST);
                $flags.z = (DST == 0);
        }


.. _falcon-isa-loadimm:

Loading immediates: mov, sethi
==============================

mov sets a register to an immediate. sethi sets high 16 bits of a register to
an immediate, leaving low bits untouched. mov can be thus used to load small
[16-bit signed] immediates, while mov+sethi can be used to load any 32-bit
immediate.

Instructions
    ===== ================= =========
    Name  Description       Subopcode
    ===== ================= =========
    mov   Load an immediate 7
    sethi Set high bits     3
    ===== ================= =========
Instruction class:
    unsized
Execution time:
    1 cycle
Operands:
    DST, SRC
Forms:
    ======= ======
    Form    Opcode
    ======= ======
    R2, I8  f0
    R2, I16 f1
    ======= ======
Immediates:
    mov:
        sign-extended
    sethi:
        zero-extended
Operation:
    ::

        if (op == mov)
                DST = SRC;
        else if (op == sethi)
                DST = DST & 0xffff | SRC << 16;


.. _falcon-isa-clear:

Clearing registers: clear
=========================

Sets a register to 0.

Instructions:
    ===== ================ =========
    Name  Description      Subopcode
    ===== ================ =========
    clear Clear a register 4
    ===== ================ =========
Instruction class:
    sized
Operands:
    DST
Forms:
    ====== ======
    Form   Opcode
    ====== ======
    R2     3d
    ====== ======
Operation:
    ::

        DST = 0;


.. _falcon-isa-setf:

Setting flags from a value: setf
================================

Sets flags according to a value.

Instructions:
    ===== ============================== ========== =========
    Name  Description                    Present on Subopcode
    ===== ============================== ========== =========
    setf  Set flags according to a value v3+ units  5
    ===== ============================== ========== =========
Instruction class:
    sized
Execution time:
    1 cycle
Operands:
    SRC
Forms:
    ====== ======
    Form   Opcode
    ====== ======
    R2     3d
    ====== ======
Operation:
    ::

        $flags.o = 0;
        $flags.s = S(SRC);
        $flags.z = (SRC == 0);


.. _falcon-isa-mul:

Multiplication: mulu, muls
==========================

Does a 16x16 -> 32 multiplication.

Instructions:
    ===== ================= =========
    Name  Description       Subopcode
    ===== ================= =========
    mulu  Multiply unsigned 0
    muls  Multiply signed   1
    ===== ================= =========
Instruction class:
    unsized
Operands:
    DST, SRC1, SRC2
Forms:
    =========== ======
    Form        Opcode
    =========== ======
    R1, R2, I8  c0
    R1, R2, I16 e0
    R2, R2, I8  f0
    R2, R2, I16 f1
    R2, R2, R1  fd
    R3, R2, R1  ff
    =========== ======
Immediates:
    mulu:
        zero-extended
    muls:
        sign-extended
Operation:
    ::

        s1 = SRC1 & 0xffff;
        s2 = SRC2 & 0xffff;
        if (op == muls) {
                if (s1 & 0x8000)
                        s1 |= 0xffff0000;
                if (s2 & 0x8000)
                        s2 |= 0xffff0000;
        }
        DST = s1 * s2;


.. _falcon-isa-sext:

Sign extension: sext
====================

Does a sign-extension of low X bits of a value.

Instructions:
    ===== =========== =========
    Name  Description Subopcode
    ===== =========== =========
    sext  Sign-extend 2
    ===== =========== =========
Instruction class:
    unsized
Execution time:
    1 cycle
Operands:
    DST, SRC1, SRC2
Forms:
    ========== ======
    Form       Opcode
    ========== ======
    R1, R2, I8 c0
    R2, R2, I8 f0
    R2, R2, R1 fd
    R3, R2, R1 ff
    ========== ======
Immediates:
    truncated
Operation:
    ::

        bit = SRC2 & 0x1f;
        if (SRC1 & 1 << bit) {
                DST = SRC1 & ((1 << bit) - 1) | -(1 << bit);
        } else {
                DST = SRC1 & ((1 << bit) - 1);
        }
        $flags.s = S(DST);
        $flags.z = (DST == 0);


.. _falcon-isa-extr:

Bitfield extraction: extr, extrs
================================

Extracts a bitfield.

Instructions:
    ===== ============================== ========== =========
    Name  Description                    Present on Subopcode
    ===== ============================== ========== =========
    extrs Extract signed bitfield        v3+ units  3
    extr  Extract unsigned bitfield      v3+ units  7
    ===== ============================== ========== =========
Instruction class:
    unsized
Execution time:
    1 cycle
Operands:
    DST, SRC1, SRC2
Forms:
    =========== ======
    Form        Opcode
    =========== ======
    R1, R2, I8  c0
    R1, R2, I16 e0
    R3, R2, R1  ff
    =========== ======
Immediates:
    zero-extended
Operation:
    ::

        low = SRC2 & 0x1f;
        size = (SRC2 >> 5 & 0x1f) + 1;
        bf = (SRC1 >> low) & ((1 << size) - 1);
        if (op == extrs) {
                signbit = (low + size - 1) & 0x1f; // depending on the mask is probably a bad idea.
                if (SRC1 & 1 << signbit)
                        bf |= -(1 << size);
        }
        DST = bf;


.. _falcon-isa-ins:

Bitfield insertion: ins
=======================

Inserts a bitfield.

Instructions:
    ===== ============================== ========== =========
    Name  Description                    Present on Subopcode
    ===== ============================== ========== =========
    ins   Insert a bitfield              v3+ units  b
    ===== ============================== ========== =========
Instruction class:
    unsized
Execution time:
    1 cycle
Operands:
    DST, SRC1, SRC2
Forms:
    =========== ======
    Form        Opcode
    =========== ======
    R1, R2, I8  c0
    R1, R2, I16 e0
    =========== ======
Immediates:
    zero-extended.
Operation:
    ::

        low = SRC2 & 0x1f;
        size = (SRC2 >> 5 & 0x1f) + 1;
        if (low + size <= 32) { // nop if bitfield out of bounds - I wouldn't depend on it, though...
                DST &= ~(((1 << size) - 1) << low); // clear the current contents of the bitfield
                bf = SRC1 & ((1 << size) - 1);
                DST |= bf << low;
        }


.. _falcon-isa-logop:

Bitwise operations: and, or, xor
================================

Ands, ors, or xors two operands.

Instructions:
    ===== =========== =========
    Name  Description Subopcode
    ===== =========== =========
    and   Bitwise and 4
    or    Bitwise or  5
    xor   Bitwise xor 6
    ===== =========== =========
Instruction class:
    unsized
Execution time:
    1 cycle
Operands:
    DST, SRC1, SRC2
Forms:
    =========== ======
    Form        Opcode
    =========== ======
    R1, R2, I8  c0
    R1, R2, I16 e0
    R2, R2, I8  f0
    R2, R2, I16 f1
    R2, R2, R1  fd
    R3, R2, R1  ff
    =========== ======
Immediates:
    zero-extended
Operation:
    ::

        if (op == and)
                DST = SRC1 & SRC2;
        if (op == or)
                DST = SRC1 | SRC2;
        if (op == xor)
                DST = SRC1 ^ SRC2;
        if (falcon_version != 0) {
                $flags.c = 0;
                $flags.o = 0;
                $flags.s = S(DST);
                $flags.z = (DST == 0);
        }


.. _falcon-isa-xbit:

Bit extraction: xbit
====================

Extracts a single bit of a specified register. On v0, the bit is stored to bit
0 of DST, other bits are unmodified. On v3+, the bit is stored to bit 0 of
DST, and all other bits of DST are set to 0.

Instructions:
    ==== ============== ========================== ==========================
    Name Description    Subopcode - opcodes c0, ff Subopcode - opcodes f0, fe
    ==== ============== ========================== ==========================
    xbit Extract a bit  8                          c
    ==== ============== ========================== ==========================
Instruction class:
    unsized
Execution time:
    1 cycle
Operands:
    DST, SRC1, SRC2
Forms:
    ============== ======
    Form           Opcode
    ============== ======
    R1, R2, I8     c0
    R3, R2, R1     ff
    R2, $flags, I8 f0
    R1, $flags, R2 fe
    ============== ======
Immediates:
    truncated
Operation:
    ::

        if (falcon_version == 0) {
                DST = DST & ~1 | (SRC1 >> bit & 1);
        } else {
                DST = SRC1 >> bit & 1;
                $flags.s = S(DST); // always works out to 0.
                $flags.z = (DST == 0);
        }


.. _falcon-isa-bit:

Bit manipulation: bset, bclr, btgl
==================================

Set, clear, or flip a specified bit of a register.

Instructions:
    ==== =========== ============================== =====================
    Name Description Subopcode - opcodes f0, fd, f9 Subopcode - opcode f4
    ==== =========== ============================== =====================
    bset Set a bit   9                              31
    bclr Clear a bit a                              32
    btgl Flip a bit  b                              33
    ==== =========== ============================== =====================
Instruction class:
    unsized
Execution time:
    1 cycle
Operands:
    DST, SRC
Forms:
    ========== ======
    Form       Opcode
    ========== ======
    R2, I8     f0
    R2, R1     fd
    $flags, I8 f4
    $flags, R2 f9
    ========== ======
Immediates:
    truncated
Operation:
    ::

        bit = SRC & 0x1f;
        if (op == bset)
                DST |= 1 << bit;
        else if (op == bclr)
                DST &= ~(1 << bit);
        else // op == btgl
                DST ^= 1 << bit;


.. _falcon-isa-div:

Division and remainder: div, mod
================================

Does unsigned 32-bit division / modulus.

Instructions:
    ===== ============ ========== =========
    Name  Description  Present on Subopcode
    ===== ============ ========== =========
    div   Divide       v3+ units  c
    mod   Take modulus v3+ units  d
    ===== ============ ========== =========
Instruction class:
    unsized
Execution time:
    30-33 cycles
Operands:
    DST, SRC1, SRC2
Forms:
    =========== ======
    Form        Opcode
    =========== ======
    R1, R2, I8  c0
    R1, R2, I16 e0
    R3, R2, R1  ff
    =========== ======
Immediates:
    zero-extended
Operation:
    ::

        if (SRC2 == 0) {
                dres = 0xffffffff;
        } else {
                dres = SRC1 / SRC2;
        }
        if (op == div)
                DST = dres;
        else // op == mod
                DST = SRC1 - dres * SRC2;


.. _falcon-isa-setp:

Setting predicates: setp
========================

Sets bit #SRC2 in $flags to bit 0 of SRC1.

Instructions:
    ===== ============= =========
    Name  Description   Subopcode
    ===== ============= =========
    setp  Set predicate 8
    ===== ============= =========
Instruction class:
    unsized
Execution time:
    1 cycle
Operands:
    SRC1, SRC2
Forms:
    ====== ======
    Form   Opcode
    ====== ======
    R2, I8 f2
    R2, R1 fa
    ====== ======
Immediates:
    truncated
Operation:
    ::

        bit = SRC2 & 0x1f;
        $flags = ($flags & ~(1 << bit)) | (SRC1 & 1) << bit;
