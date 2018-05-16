.. _maxwell-isa:

================
Maxwell CUDA ISA
================

.. contents::


Introduction
============

This currently is not a complete reference of known functionality, but where
behaviour not obvious from envydis/gm107.c can be documented.

Some notes for reading this documentation:

- An instruction's docs is split into three sections, the forms text, the description and the behaviour text.
- The first operand is usually the destination.
- The behaviour text uses the notation SRC<n>/DST, while the forms text does not.
- REG<n> is a reference to a register.
- CB<n> is a reference to the contents of a constant buffer.
- U<b>_<n> is a b-bit unsigned immediate value.
- S<b>_<n> is a b-bit signed immediate value.
- The "carry flag" is not a instruction flag, but a register.
- Some subtleties may lie in an instruction's description if putting it in the behaviour text would be too verbose.
- add_with_carry(a, b) returns the sum of ``a`` and ``b`` using the carry flag, and writes the carry flag.
    - It does not use and/or set the carry flag if the appropriate instruction flags are not specified.
- Instruction flags in between [ and ] are optional.
- The order of the flags (even in between [ and ]) is what is expected by envyas.

Instructions
============

The instructions are roughly divided into the following groups:

- :ref:`maxwell-int`
