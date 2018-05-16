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

An optional flag that can be either ``h0`` or ``h1``. With ``h1``, the high 16 bits of the operand are used. With ``h0``, the low 16 bits are used.

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

Adds three integers. The flag ``mode`` may optionally be ``rs`` or ``ls``.

::

    switch (mode) {
      case rs: DST = add_with_carry(((SRC1 + SRC2) >> 16), SRC3); break;
      case ls: DST = add_with_carry(((SRC1 + SRC2) << 16), SRC3); break;
      default: DST = add_with_carry((SRC1 + SRC2), SRC3); break;
    }

.. _maxwell-opg-xmad

Multiply-add: xmad
==================

::

  xmad [src1_type,src2_type,psl,mrg,cmode,x,cc] REG0 [h1] REG1 [h1] REG2 REG3
  xmad [src1_type,src2_type,cmode,x,cc] REG0 [h1] REG1 [h1] REG2 CB3
  xmad [src1_type,src2_type,psl,mrg,cmode,x,cc] REG0 [h1] REG1 [h1] CB2 REG3
  xmad [src1_type,src2_type,psl,mrg,cmode,x,cc] REG0 [h1] REG1 [h1] S20_2 REG3

Multiplies two 16-bit integers and adds a 32 bit integer, along with a bunch of
other stuff.

If one of ``src1_type`` or ``src2_type`` is set, the other must also be set. They can be ``s16 u16``, ``u16 s16`` or ``s16 s16``.

The flag ``cmode`` may optionally be ``clo``, ``chi``, ``csfu`` or ``cbcc``. The ``cbcc``
mode may not be specified for the constant buffer forms.

::

    uint32_t p_a = SRC1.h1 ? SRC1>>16 : SRC1&0xffff;
    uint32_t p_b = SRC2.h1 ? SRC2>>16 : SRC2&0xffff;
    if (src1_type == s16) p_a = sign_extend_from_16_to_32(p_a);
    if (src2_type == s16) p_b = sign_extend_from_16_to_32(p_b);

    uint32_t p = p_a * p_b;
    if (psl) p <<= 16;

    uint32_t c = SRC3;
    switch (cmode) {
      case clo: c = c & 0xffff; break;
      case chi: c = c >> 16; break;
      case cbcc: c += SRC2 << 16; break;
      case csfu: {
        if (p_a==0 || p_b==0) break;
        //v & 0x80000000 -> as_twos_complement(v) < 0
        if (p_a & 0x80000000) c -= 65536;
        if (p_b & 0x80000000) c -= 65536;
        break;
      }
    }

    DST0 = add_with_carry(p, c);
    if (mrg) DST0 = (DST0 & 0xffff) | (SRC2<<16);
