.. _pvp1:

===============================
Overview of VP1 video processor
===============================

.. contents::


Introduction
============

VP1 is basically a vector processor with PFIFO interface and custom ISA
designed for video decoding.  It's made of the following units:

- the processor proper:

  - :ref:`scalar unit <vp1-scalar>`
  - :ref:`vector unit <vp1-vector>`
  - :ref:`address unit <vp1-address>`
  - :ref:`branch unit <vp1-branch>`

- :ref:`8kb of dedicated data RAM <vp1-data>`
- instruction cache
- memory interface
- :ref:`DMA engine <vp1-dma>`
- :ref:`FIFO interface <vp1-fifo>`

There are 3 variants of VP1:

- the original NV41 variant [NV41:NV44]
- the NV44 variant, with improved context switching and method processing
  capabilities [NV44:G80]
- the G80 variant, with changes to match G80 memory model [G80:G84]


MMIO registers
==============

.. space:: 8 pvp1 0x1000 VP1 video vector processor engine

   .. todo:: write me


.. _pvp1-intr:

Interrupts
==========

.. todo:: write me


Instruction fetching
====================

.. todo:: write me


Instruction format
==================

On VP1, all instructions are stored as 32-bit little-endian words.  The top
8 bits of the instruction word determine the opcode, and the highest bits
of the opcode determine its execution unit:

- ``0x00-0x7f``: scalar instructions
- ``0x80-0xbf``: vector instructions
- ``0xc0-0xdf``: address instructions
- ``0xe0-0xff``: branch instructions

The executed instruction stream is divided into so-called instruction bundles,
which are groups of up to 4 instructions targetting distinct execution units.
The instructions of a bundle are executed in parallel - they don't see each
others' changes to register state.  In some cases, however, the scalar
instruction of a bundle computes data to be used by the vector instruction
of the same bundle (so-called :ref:`s2v path <vp1-s2v>`).

The instructions are grouped into bundles as follows:

.. todo:: figure that out


Registers
=========

VP1 has 15 distinct register files:

- ``$r0-$r30`` (with ``$r31`` hardwired to 0): :ref:`32-bit scalar registers
  <vp1-reg-scalar>`, sometimes treated as groups of 4 bytes for SIMD
  instructions
- ``$v0-$v31``: :ref:`128-bit vector registers <vp1-reg-vector>`, treated
  as groups of 16 bytes for SIMD instructions
- ``$a0-$a31``: :ref:`32-bit address registers <vp1-reg-address>`, they have
  funny bitfields used for memory addressing, looping, and mode selection
- ``$c0-$c3``: :ref:`16-bit condition code registers <vp1-reg-cond>`, split
  into individual bits belonging to one of the four execution units, used
  for branching and conditionally selecting inputs
- ``$vc0-$vc3``: :ref:`32-bit vector condition code registers
  <vp1-reg-vector>`, like ``$c``, but with different fields, and each
  bit is duplicated 16 times (one for each vector component)
- ``$va``, :ref:`448-bit vector accumulator <vp1-reg-vector>`, split into
  16 components, each with 12 integer and 16 fractional bits
- ``$vx``, :ref:`128-bit vector extra register <vp1-reg-vector>`, split into
  16 components like normal ``$v``
- ``$l0-$l3``: :ref:`16-bit loop registers <vp1-reg-branch>`, split into 8-bit
  loop counter and 8-bit loop total count
- ``$m0-$m63``: :ref:`32-bit method registers <vp1-reg-mthd>`
- ``$x0-$x15``: :ref:`32-bit extra registers <vp1-reg-extra>` (G80 only)
- ``$d0-$d7``: :ref:`17-bit DMA object registers <vp1-reg-dma>` (G80 only)
- ``$f0-$f1``: :ref:`FIFO special registers <vp1-reg-fifo>`
- ``$sr0-$sr31``: :ref:`misc special registers <vp1-reg-special>`
- ``$mi0-$mi31``: :ref:`memory interface special registers <vp1-reg-special>`
- ``$uc0-$uc31``: :ref:`processor control special registers <vp1-reg-special>`

.. todo:: incomplete for <G80


.. _vp1-reg-cond:

Condition code registers
------------------------

There are 4 condition code registers, ``$c0-$c3``.  Each of them has
the following bitfields:

- bits 0-7: :ref:`scalar flags <vp1-reg-scalar>`
- bits 8-10: :ref:`address flags <vp1-reg-address>`
- bits 11-12: unused, always 0
- bit 13: :ref:`branch flag <vp1-reg-branch>`
- bit 14: always 0
- bit 15: always 1


.. _vp1-reg-special:

Special registers
-----------------

.. todo:: write me


.. _vp1-reg-extra:

Extra registers
---------------

The G80 variant of VP1 introduced 16 extra registers, ``$x0-$x15``, each of
them 32 bits long. They have no special semantics and the only way to access
them is by using the :ref:`mov to/from alternate register file scalar
instruction <vp1-ops-mov-sr>`.
