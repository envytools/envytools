.. _tesla-data:

==========================
Data movement instructions
==========================

.. contents::


Introduction
============

.. todo:: write me


Data movement: (h)mov
=====================

.. todo:: write me

::

  [lanemask] mov b32/b16 DST SRC

  lanemask assumed 0xf for short and immediate versions.

    if (lanemask & 1 << (laneid & 3)) DST = SRC;

  Short:    0x10000000 base opcode
        0x00008000 0: b16, 1: b32
        operands: S*DST, S*SRC1/S*SHARED

  Imm:      0x10000000 base opcode
        0x00008000 0: b16, 1: b32
        operands: L*DST, IMM

  Long:     0x10000000 0x00000000 base opcode
        0x00000000 0x04000000 0: b16, 1: b32
        0x00000000 0x0003c000 lanemask
        operands: LL*DST, L*SRC1/L*SHARED


Condition registers
===================

Reading condition registers: mov (from $c)
------------------------------------------

.. todo:: write me

::

  mov DST COND

  DST is 32-bit $r.

    DST = COND;

  Long:     0x00000000 0x20000000 base opcode
        operands: LDST, COND

Writing condition registers: mov (to $c)
----------------------------------------

.. todo:: write me

::

  mov CDST SRC

  SRC is 32-bit $r. Yes, the 0x40 $c write enable flag in second word is
  actually ignored.

    CDST = SRC;

  Long:     0x00000000 0xa0000000 base opcode
        operands: CDST, LSRC1


Address registers
=================

Reading address registers: mov (from $a)
----------------------------------------

.. todo:: write me

::

  mov DST AREG

  DST is 32-bit $r. Setting flag normally used for autoincrement mode doesn't
  work, but still causes crash when using non-writable $a's.

    DST = AREG;

  Long:     0x00000000 0x40000000 base opcode
        0x02000000 0x00000000 crashy flag
        operands: LDST, AREG

Writing address registers: shl (to $a)
--------------------------------------

.. todo:: write me

::

  shl ADST SRC SHCNT

  SRC is 32-bit $r.

    ADST = SRC << SHCNT;

  Long:     0x00000000 0xc0000000 base opcode
        operands: ADST, LSRC1/LSHARED, HSHCNT

Increasing address registers: add ($a)
--------------------------------------

.. todo:: write me

::

  add ADST AREG OFFS

  Like mov from $a, setting flag normally used for autoincrement mode doesn't
  work, but still causes crash when using non-writable $a's.

    ADST = AREG + OFFS;

  Long:     0xd0000000 0x20000000 base opcode
        0x02000000 0x00000000 crashy flag
        operands: ADST, AREG, OFFS


Reading special registers: mov (from $sr)
=========================================

.. todo:: write me

::

  mov DST physid    S=0
  mov DST clock     S=1
  mov DST sreg2     S=2
  mov DST sreg3     S=3
  mov DST pm0       S=4
  mov DST pm1       S=5
  mov DST pm2       S=6
  mov DST pm3       S=7

  DST is 32-bit $r.

    DST = SREG;

  Long:     0x00000000 0x60000000 base opcode
        0x00000000 0x0001c000 S
        operands: LDST


Memory space access
===================

Const space access: ld c[]
--------------------------

.. todo:: write me

Local space access: ld l[], st l[]
----------------------------------

.. todo:: write me

Shared space access: ld s[], st s[]
-----------------------------------

.. todo:: write me

::

  mov lock CDST DST s[]

    Tries to lock a word of s[] memory and load a word from it. CDST tells
    you if it was successfully locked+loaded, or no. A successfully locked
    word can't be locked by any other thread until it is unlocked.

  mov unlock s[] SRC

    Stores a word to previously-locked s[] word and unlocks it.

Input space access: ld a[]
--------------------------

.. todo:: write me

Output space access: st o[]
---------------------------

.. todo:: write me


Global space access
===================

Global load/stores: ld g[], st g[]
----------------------------------

.. todo:: write me

Global atomic operations: ld (add|inc|dec|max|min|and|or|xor) g[], xchg g[], cas g[]
------------------------------------------------------------------------------------

.. todo:: write me

Global reduction operations: (add|inc|dec|max|min|and|or|xor) g[]
-----------------------------------------------------------------

.. todo:: write me
