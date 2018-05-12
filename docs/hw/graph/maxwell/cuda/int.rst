.. _maxwell-int:

===============================
Integer Arithmetic Instructions
===============================

.. contents::

Introduction
============

Common Flags
============

neg
---

Negate the operand.

h0/h1
-----

An optional flag that can be either H0 or H1. With H1, the high 16 bits of the operand are used. With H0, the low 16 bits.

x
-

Use the carry flag.

cc
--

Set the carry flag.

.. _maxwell-opg-iadd3

Addition: iadd3
===============

::

  iadd3 [mode,x,cc] REG0 [neg,h0/h1] REG1 [neg,h0/h1] REG2 [neg,h0/h1] REG3
  iadd3 [x,cc] REG0 [neg] REG1 [neg] CB2 [neg] REG3
  iadd3 [x,cc] REG0 [neg] REG1 [neg] S20_2 [neg] REG3

Adds three integers. The flag "mode" may optionally by RS or LS.

::

    switch (mode) {
      case RS: DST = add_with_carry(((SRC1 + SRC2) >> 16), SRC3); break;
      case LS: DST = add_with_carry(((SRC1 + SRC2) << 16), SRC3); break;
      default: DST = add_with_carry(SRC1, SRC2, SRC3); break;
    }
