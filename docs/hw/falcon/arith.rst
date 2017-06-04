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

- bit 8: c, carry flag.  Set by addition instructions iff a carry out of the
  high bit (or, equivalently, unsigned overflow) has occured.  Likewise set
  by subtraction instructions iff a borrow into the high bit (or unsigned
  overflow) has occured.  Also used by shift instructions to store the last
  shifted out bit.  Used as the less-than condition in old comparisons.
- bit 9: o, signed overflow flag - set by addition, subtraction, comparison,
  and negation instructions if a signed overflow occured.  Set to 0 by some
  other instructions.
- bit 10: s, sign flag - set according to the high bit of the result by most
  arithmetic instructions.
- bit 11: z, zero flag - set iff the result was equal to 0 by most arithmetic
  instructions.

Also, a few ALU instructions operate on $flags register as a whole.


.. _falcon-arith-conventions:

Pseudocode conventions
======================

``sz``, for sized instructions, is the selected size of operation: 8, 16, or 32.

``S(x)`` evaluates to ``(x >> (sz - 1) & 1)``, ie. the sign bit of ``x``. If insn
is unsized, assume ``sz == 32``.

``C(a, b, c)``, where ``a, b, c`` are booleans, is the carry flag for
an addition where the two inputs have high bits of ``a`` and ``b``,
and the result has a high bit of ``c``.  It is computed as follows::

    bool C(bool a, bool b, bool c) {
        // a and b both set - there is always carry out.
        if (a && b)
            return 1;
        // One of a and b is set - there is carry out iff result has high
        // bit 0.
        if ((a || b) && !c)
            return 1;
        # Otherwise (a and b both clear), there is no possibility of carry
        # out.
        return 0;
    }

Also, ``!C(a, !b, c)`` is the borrow flag for a subtraction where
the two inputs have high bits of ``a`` and ``b``, and the result has
a high bit of ``c``.

Likewise, ``O(a, b, c)`` is similarly defined as the signed overflow flag
for an addition::

    bool O(bool a, bool b, bool c) {
        return a == b && a != c;
        // equivalent definition (check it yourself):
        // return a ^ b ^ c ^ C(a, b, c);
    }

Similarly, ``O(a, !b, c)`` is the signer overflow flag for subtraction.


.. _falcon-isa-cmp:

Comparison: cmpu, cmps, cmp
===========================

Compare two values, setting flags according to results of comparison. ``cmp``
sets the usual set of 4 flags, and behaves identically to a subtraction
instruction that doesn't write its destination register.  ``cmpu`` sets
only ``c`` and ``z``, but otherwise behaves like ``cmp`` - thus it is only
useful for unsigned comparisons.  ``cmps`` sets ``z`` normally,
but sets ``c`` iff ``SRC1`` is less then ``SRC2`` when treated as signed
number (thus using unsigned condition codes to store the result of a signed
comparison instead).

``cmpu``/``cmps`` are the only comparison instructions available on Falcon v0.
Both of them set only the ``c`` and ``z`` flags, with ``cmps`` setting ``c``
flag in an unusual way to enable signed comparisons while using unsigned flags
and condition codes.  To do an unsigned comparison, use ``cmpu`` and the
unsigned branch conditions [``b/a/e``]. To do a signed comparison, use ``cmps``,
also with unsigned branch conditions.

The Falcon v3+ new ``cmp`` instruction sets the full set of flags.  To do
an unsigned comparison on v3+, use ``cmp`` and the unsigned branch conditions.
To do a signed comparison, use cmp and the signed branch conditions [``l/g/e``].

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

        uint<sz>_t diff = SRC1 - SRC2;
        $flags.z = (diff == 0);
        if (op == cmps)
            $flags.c = O(S(SRC1), !S(SRC2), S(diff)) ^ S(diff);
        else if (op == cmpu)
            $flags.c = !C(S(SRC1), !S(SRC2), S(diff));
        else if (op == cmp) {
            $flags.c = !C(S(SRC1), !S(SRC2), S(diff));
            $flags.o = O(S(SRC1), !S(SRC2), S(diff));
            $flags.s = S(diff);
        }


.. _falcon-isa-add:

Addition/substraction: add, adc, sub, sbb
=========================================

Add or substract two values, possibly with carry/borrow.  The full set
of arithmetic flags is always written.

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

        uint<sz>_t res;
        if (op == add)
            res = SRC1 + SRC2;
        else if (op == adc)
            res = SRC1 + SRC2 + $flags.c;
        else if (op == sub)
            res = SRC1 - SRC2;
        else if (op == sbb)
            res = SRC1 - SRC2 - $flags.c;

        if (op == add || op == adc) {
            $flags.c = C(S(SRC1), S(SRC2), S(res));
            $flags.o = O(S(SRC1), S(SRC2), S(res));
        } else {
            $flags.c = !C(S(SRC1), !S(SRC2), S(res));
            $flags.o = O(S(SRC1), !S(SRC2), S(res));
        }
        DST = res;
        $flags.s = S(res);
        $flags.z = (res == 0);


.. _falcon-isa-shift:

Shifts: shl, shr, sar, shlc, shrc
=================================

Shift a value. For ``shl/shr``, the extra bits "shifted in" are 0. For ``sar``,
they're equal to sign bit of source. For ``shlc/shrc``, the first such bit
is taken from carry flag, the rest are 0.  On Falcon v3+, these instructions
set all 4 arithmetic flags - ``s`` and ``z`` are set as usual, ``o`` is always
set to 0, and ``c`` is set to the value of the last shifted out bit, or 0
if the shift count was 0.  On Falcon v0, only ``c`` is set.

The shift count is always masked to 3 bits in case of 8-bit shift instructions,
4 bits in case of 16-bit shift instructions, and 5 bits in case of 32-bit shift
instructions.

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

        unsigned shcnt;
        if (sz == 8)
            shcnt = SRC2 & 7;
        else if (sz == 16)
            shcnt = SRC2 & 0xf;
        else // sz == 32
            shcnt = SRC2 & 0x1f;
        uint<sz>_t res;
        if (op == shl || op == shlc) {
            res = SRC1 << shcnt;
            if (op == shlc && shcnt != 0)
                res |= $flags.c << (shcnt - 1);
            if (shcnt == 0)
                $flags.c = 0;
            else
                $flags.c = SRC1 >> (sz - shcnt) & 1;
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
        }
        DST = res;
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
moved value. hswap rotates a value by half its size.  All instructions except
``mov`` set 3 flags: ``s`` and ``z`` (which are set as usual), as well as
``o`` (which is set iff signed overflow occured for ``neg``, and always set
to 0 for other instructions).

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

Sets ``z`` and ``s`` flags according to a value, sets ``o`` flag to 0.

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

Does a 16x16 -> 32 multiplication.  The inputs are unsigned for ``mulu``,
signed for ``muls``.  Sets no flags.

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

Does a sign-extension of low (X+1) bits of a value.  Sets ``s`` and ``z``
flags according to the result.  The second argument is, after masking to
5 bits, the bit index (counting from LSB) which contains the new sign bit
- the result will be equal to the source with all bits higher than that
replaced with a copy of the sign bit.

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

Extracts a bitfield.  The bitfield to extract is given as a pair of (low bit
index, size in bits - 1) packed in a single 10-bit source, with each part
taking 5 bits.  The value of the bitfield is returned in the low bits of
the destination register.  ``extr`` extracts an unsigned bitfield, setting
the remaining destination bits to 0, while ``extrs`` extracts a signed
bitfield, setting the remaining bits to a copy of the sign bit (ie. the
highest bit of the bitfield).

Both instructions set ``s`` and ``z`` flags.  While ``z`` is set as usual,
``s`` is set to the "fill" bit used for high bits of the destination - thus
it is always ``0`` for ``extr``.

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

        int low = SRC2 & 0x1f;
        int sizem1 = (SRC2 >> 5 & 0x1f);
        uint32_t bf = (SRC1 >> low) & ((2 << sizem1) - 1);
        bool fill_bit;
        if (op == extr) {
            fill_bit = 0;
        } else if (op == extrs) {
            // depending on the mask is probably a bad idea.
            int signbit = (low + sizem1) & 0x1f;
            fill_bit = SRC1 >> signbit & 1;
        }
        if (fill_bit)
            bf |= -(2 << sizem1);
        DST = bf;
        $flags.s = fill_bit;
        $flags.z = (DST == 0);


.. _falcon-isa-ins:

Bitfield insertion: ins
=======================

Inserts a bitfield, which is specified like for ``extr/extrs``.
Sets no flags.

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

Ands, ors, or xors two operands.  On Falcon v0, sets no flags.  On Falcon v3,
sets all flags - ``s`` and ``z`` are set as usual, ``c`` and ``o`` are always
set to 0.

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

Extracts a single bit of a specified register.  On Falcon v0, the bit is stored
to bit 0 of DST, while other destination bits are unmodified, and no flags are
set.  On Falcon v3+, the bit is stored to bit 0 of DST, all other bits of DST
are set to 0, ``s`` flag is set to 0, and ``z`` flag is set iff the extracted
bit was 0 (behaving exactly like an ``extr`` instruction with size 1).  In both
cases, the bit index is masked off to 5 bits.

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
                $flags.s = 0;
                $flags.z = (DST == 0);
        }


.. _falcon-isa-bit:

Bit manipulation: bset, bclr, btgl
==================================

Set, clear, or flip a specified bit of a register.  The requested bit index
is masked off to 5 bits.  No flags are set.

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

Does unsigned 32-bit division / modulus.  Sets no flags.  If a division by
0 is requested, no exception happens - the division result is always
``0xffffffff`` in this case, and the modulus result is equal to the first
source.

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

Sets bit #SRC2 in $flags to bit 0 of SRC1.  The bit index is masked off to
5 bits.

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
