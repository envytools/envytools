.. _vp1-scalar:

===========
Scalar unit
===========

.. contents::


Introduction
============

The scalar unit is one of the four execution units of VP1. It is used for
general-purpose arithmetic.


.. _vp1-reg-scalar:

Scalar registers
----------------

The scalar unit has 31 GPRs, ``$r0-$r30``.  They are 32 bits wide, and are
usually used as 32-bit integers, but there are also SIMD instructions treating
them as arrays of 4 bytes.  In such cases, array notation is used to denote
the individual bytes.  Bits 0-7 are considered to be ``$rX[0]``, bits 8-15 are
``$rX[1]`` and so on.  ``$r31`` is a special register hardwired to 0.

There are also 8 bits in each ``$c`` register belonging to the scalar unit.
Most scalar instructions can (if requested) set these bits according to the
computation result.  The bits are:

- bit 0: sign flag - set equal to bit 31 of the result
- bit 1: zero flag - set if the result is 0
- bit 2: b19 flag - set equal to bit 19 of the result
- bit 3: b20 difference flag - set if bit 20 of the result is different from
  bit 20 of the first source
- bit 4: b20 flag - set equal to bit 20 of the result
- bit 5: b21 flag - set equal to bit 21 of the result
- bit 6: alt b19 flag (G80 only) - set equal to bit 19 of the result
- bit 7: b18 flag (G80 only) - set equal to bit 18 of the result

The purpose of the last 6 bits is so far unknown.


.. _vp1-s2v:

Scalar to vector data bus
-------------------------

In addition to performing computations of its own, the scalar unit is also
used in tandem with the vector unit to perform complex instructions.  Certain
scalar opcodes expose data on so-called s2v path (scalar to vector data bus),
and certain vector opcodes consume this data.

The data is ephemeral and only exists during the execution of a single bundle
- the producing and consuming instructions must be located in the same bundle.
If a consuming instruction is used without a producing instruction, it'll read
junk.  If a producing instruction is used without a consuming instruction,
the data is discarded.

The s2v data consists of:

- 4 signed 10-bits factors, used for multiplication
- ``$vc`` selection and transformation, for use as mask input in vector unit,
  made of:

  - valid flag: 1 if s2v data was emitted by proper s2v-emitting instruction
    (if false, vector unit will use an alternate source not involving s2v)
  - 2-bit ``$vc`` register index
  - 1-bit zero flag or sign flag selection (selects which half of ``$vc`` will
    be used)
  - 3-bit transform mode: used to mangle the ``$vc`` value before use as mask

The factors can alternatively be treated as two 16-bit masks by some
instructions.  In that case, mask 0 consists of bits 1-8 of factor 0, then
bits 1-8 of factor 1 and mask 1 likewise consists of bits 1-8 of factors 2
and 3::

    s2v.mask[0] = (s2v.factor[0] >> 1 & 0xff) | (s2v.factor[1] >> 1 & 0xff) << 8;
    s2v.mask[1] = (s2v.factor[2] >> 1 & 0xff) | (s2v.factor[3] >> 1 & 0xff) << 8;

The ``$vc`` based mask is derived as follows::

    def xfrm(val, tab):
        res = 0
        for idx in range(16):
            # bit x of result is set if bit tab[x] of input is set
            if val & 1 << tab[idx]:
                res |= 1 << idx
        return res

    val = $vc[s2v.vcsel.idx]
    # val2 is only used for transform mode 7
    val2 = $vc[s2v.vcsel.idx | 1]

    if s2v.vcsel.flag == 'sf':
        val = val & 0xffff
        val2 = val2 & 0xffff
    else: # 'zf'
        val = val >> 16 & 0xffff
        val2 = val2 >> 16 & 0xffff

    if s2v.vcsel.xfrm == 0:
        # passthrough
        s2v.vcmask = val
    elif s2v.vcsel.xfrm == 1:
        s2v.vcmask = xfrm(val, [2,  2,  2,  2,  6,  6,  6,  6, 10, 10, 10, 10, 14, 14, 14, 14])
    elif s2v.vcsel.xfrm == 2:
        s2v.vcmask = xfrm(val, [4,  5,  4,  5,  4,  5,  4,  5, 12, 13, 12, 13, 12, 13, 12, 13])
    elif s2v.vcsel.xfrm == 3:
        s2v.vcmask = xfrm(val, [0,  0,  2,  0,  4,  4,  6,  4,  8,  8, 10,  8, 12, 12, 14, 12])
    elif s2v.vcsel.xfrm == 4:
        s2v.vcmask = xfrm(val, [1,  1,  1,  3,  5,  5,  5,  7,  9,  9,  9, 11, 13, 13, 13, 15])
    elif s2v.vcsel.xfrm == 5:
        s2v.vcmask = xfrm(val, [0,  0,  2,  2,  4,  4,  6,  6,  8,  8, 10, 10, 12, 12, 14, 14])
    elif s2v.vcsel.xfrm == 6:
        s2v.vcmask = xfrm(val, [1,  1,  1,  1,  5,  5,  5,  5,  9,  9,  9,  9, 13, 13, 13, 13])
    elif s2v.vcsel.xfrm == 7:
        # mode 7 is special: it uses two $vc inputs and takes every second bit
        s2v.vcmask = xfrm(val | val2 << 16, [0,  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30])


Instruction format
==================

The instruction word fields used in scalar instructions are:

- bits 0-2: ``CDST`` - if < 4, index of the ``$c`` register to set according
  to the instruction's result. Otherwise, an indication that ``$c`` is not to
  be written (nVidia appears to use 7 in such case).

- bits 0-7: ``BIMMBAD`` - an immediate field used only in :ref:`bad opcodes
  <vp1-bad-opcode>`

- bit 1: ``SIGN2`` - determines if byte multiplication source 2 is signed

  - 0: ``u`` - unsigned
  - 1: ``s`` - signed

- bit 2: ``SIGN1`` - likewise for source 1

- bits 3-10: ``BIMM``: an 8-bit immediate for bytewise operations, signed or
  unsigned depending on instruction.

- bits 3-13: ``IMM``: signed 13-bit immediate.

- bits 3-4: ``COND`` - if source mangling is used, the ``$c`` register index
  to use for source mangling.

- bits 5-8: ``SLCT`` - if source mangling is used, the condition to use for
  source mangling.

- bit 8: ``RND`` - determines byte multiplication rounding behaviour

  - 0: ``rd`` - round down
  - 1: ``rn`` - round to nearest, ties rounding up

- btis 9-13: ``SRC2`` - the second source ``$r`` register, often mangled via
  source mangling.

- bits 9-13 (low 5 bits) and bit 0 (high bit): ``BIMMMUL`` - a 6-bit immediate
  for bytewise multiplication, signed or unsigned depending on instruction.

- bits 14-18: ``SRC1`` - the first source ``$r`` register.

- bits 19-23: ``DST`` - the destination ``$r`` register.

- bits 24-31: ``OP`` - the opcode.


Opcodes
-------

The opcode range assigned to the scalar unit is ``0x00-0x7f``.  The opcodes are:

- ``0x01``, ``0x11``, ``0x21``, ``0x31``: :ref:`bytewise multiplication: bmul <vp1-ops-bmul>`
- ``0x02``, ``0x12``, ``0x22``, ``0x32``: :ref:`bytewise multiplication: bmul (bad opcode) <vp1-ops-bmul>`
- ``0x04``: :ref:`s2v multiply/add/send: bvecmad <vp1-ops-bvecmad>`
- ``0x24``: :ref:`s2v immediate send: vec <vp1-ops-vec>`
- ``0x05``: :ref:`s2v multiply/add/select/send: bvecmadsel <vp1-ops-bvecmad>`
- ``0x25``: :ref:`bytewise immediate and: band <vp1-ops-byte-bitop>`
- ``0x26``: :ref:`bytewise immediate or: bor <vp1-ops-byte-bitop>`
- ``0x27``: :ref:`bytewise immediate xor: bxor <vp1-ops-byte-bitop>`
- ``0x08``, ``0x18``, ``0x28``, ``0x38``: :ref:`bytewise minimum: bmin <vp1-ops-byte>`
- ``0x09``, ``0x19``, ``0x29``, ``0x39``: :ref:`bytewise maximum: bmax <vp1-ops-byte>`
- ``0x0a``, ``0x1a``, ``0x2a``, ``0x3a``: :ref:`bytewise absolute value: babs <vp1-ops-byte>`
- ``0x0b``, ``0x1b``, ``0x2b``, ``0x3b``: :ref:`bytewise negate: bneg <vp1-ops-byte>`
- ``0x0c``, ``0x1c``, ``0x2c``, ``0x3c``: :ref:`bytewise addition: badd <vp1-ops-byte>`
- ``0x0d``, ``0x1d``, ``0x2d``, ``0x3d``: :ref:`bytewise substract: bsub <vp1-ops-byte>`
- ``0x0e``, ``0x1e``, ``0x2e``, ``0x3e``: :ref:`bytewise shift: bshr, bsar <vp1-ops-byte>`
- ``0x0f``: :ref:`s2v send: bvec <vp1-ops-bvec>`
- ``0x41``, ``0x51``, ``0x61``, ``0x71``: :ref:`16-bit multiplication: mul <vp1-ops-arith>`
- ``0x42``: :ref:`bitwise operation: bitop <vp1-ops-bitop>`
- ``0x62``: :ref:`immediate and <vp1-ops-bitop-imm>`
- ``0x63``: :ref:`immediate xor <vp1-ops-bitop-imm>`
- ``0x64``: :ref:`immediate or <vp1-ops-bitop-imm>`
- ``0x45``: :ref:`s2v 4-bit mask send and shift: vecms <vp1-ops-vecms>`
- ``0x65``: :ref:`load immediate: mov <vp1-ops-mov>`
- ``0x75``: :ref:`set high bits immediate: sethi <vp1-ops-sethi>`
- ``0x6a``: :ref:`mov to other register file: mov <vp1-ops-mov-sr>`
- ``0x6b``: :ref:`mov from other register file: mov <vp1-ops-mov-sr>`
- ``0x48``, ``0x58``, ``0x68``, ``0x78``: :ref:`minimum: min <vp1-ops-arith>`
- ``0x49``, ``0x59``, ``0x69``, ``0x79``: :ref:`maximum: max <vp1-ops-arith>`
- ``0x4a``, ``0x5a``, ``0x7a``: :ref:`absolute value: abs <vp1-ops-arith>`
- ``0x4b``, ``0x5b``, ``0x7b``: :ref:`negation: neg <vp1-ops-arith>`
- ``0x4c``, ``0x5c``, ``0x6c``, ``0x7c``: :ref:`addition: add <vp1-ops-arith>`
- ``0x4d``, ``0x5d``, ``0x6d``, ``0x7d``: :ref:`substraction: sub <vp1-ops-arith>`
- ``0x4e``, ``0x5e``, ``0x6e``, ``0x7e``: :ref:`shift: shr, sar <vp1-ops-arith>`


.. _vp1-bad-opcode:

Bad opcodes
~~~~~~~~~~~

Some of the VP1 instructions look like they're either buggy or just unintended
artifacts of incomplete decoding hardware.  These are known as bad opcodes and
are characterised by using colliding bitfields.  It's probably a bad idea
to use them, but they do seem to reliably perform as documented here.


Source mangling
---------------

Some instructions perform source mangling: the source register(s) they use are
not taken directly from a register index bitfield in the instruction.  Instead,
the register index from the instruction is... "adjusted" before use.  There
are several algorithms used for source mangling, most of them used only in
a single instruction.

The most common one, known as ``SRC2S``, takes the register index from
``SRC2`` field, a ``$c`` register index from ``COND``, and ``$c`` bit index
from ``SLCT``.  If ``SLCT`` is anything other than 4, the selected bit is
extracted from ``$c`` and XORed into the lowest bit of the register index
to use.  Otherwise (``SLCT`` is 4), bits 4-5 of ``$c`` are extracted, and
added to bits 0-1 of the register index, discarding overflow out of bit 1::

    if SLCT == 4:
        adjust = $c[COND] >> 4 & 3;
        SRC2S = (SRC2 & ~3) | ((SRC2 + adjust) & 3)
    else:
        adjust = $c[COND] >> SLCT & 1;
        SRC2S = SRC2 ^ adjust


Instructions
============


.. _vp1-ops-mov:

Load immediate: mov
-------------------

.. todo:: write me


.. _vp1-ops-sethi:

Set high bits: sethi
--------------------

.. todo:: write me


.. _vp1-ops-mov-sr:

Move to/from other register file: mov
-------------------------------------

.. todo:: write me


.. _vp1-ops-arith:

Arithmetic operations: mul, min, max, abs, neg, add, sub, shr, sar
------------------------------------------------------------------

.. todo:: write me


.. _vp1-ops-bitop:

Bit operations: bitop
---------------------

.. todo:: write me


.. _vp1-ops-bitop-imm:

Bit operations with immediate: and, or, xor
-------------------------------------------

.. todo:: write me


.. _vp1-ops-byte:

Simple bytewise operations: bmin, bmax, babs, bneg, badd, bsub
--------------------------------------------------------------

Those perform the corresponding operation (minumum, maximum, absolute value,
negation, addition, substraction) in SIMD manner on 8-bit signed or unsigned
numbers from one or two sources.  Source 1 is always a register selected by
``SRC1`` bitfield.  Source 2, if it is used (ie. instruction is not ``babs``
nor ``bneg``), is either a register selected by ``SRC2S`` mangled bitfield,
or immediate taken from ``BIMM`` bitfield.

Each of these instructions comes in signed and unsigned variants and both
perform result clipping.  Note that abs is rather uninteresting in its
unsigned variant (it's just the identity function), and so is neg (result is
always 0 or clipped to 0.

These instruction have a ``$c`` output, but it's always set to all-0 if used.

Also note that ``babs`` and ``bneg`` have two redundant opcodes each: the bit
that normally selects immediate or register second source doesn't apply
to them.

Instructions:
    =========== ============================== ========
    Instruction Operands                       Opcode
    =========== ============================== ========
    ``bmin s``  ``$r[DST] $r[SRC1] $r[SRC2S]`` ``0x08``
    ``bmax s``  ``$r[DST] $r[SRC1] $r[SRC2S]`` ``0x09``
    ``babs s``  ``$r[DST] $r[SRC1]``           ``0x0a``
    ``bneg s``  ``$r[DST] $r[SRC1]``           ``0x0b``
    ``badd s``  ``$r[DST] $r[SRC1] $r[SRC2S]`` ``0x0c``
    ``bsub s``  ``$r[DST] $r[SRC1] $r[SRC2S]`` ``0x0d``
    ``bmin u``  ``$r[DST] $r[SRC1] $r[SRC2S]`` ``0x18``
    ``bmax u``  ``$r[DST] $r[SRC1] $r[SRC2S]`` ``0x19``
    ``babs u``  ``$r[DST] $r[SRC1]``           ``0x1a``
    ``bneg u``  ``$r[DST] $r[SRC1]``           ``0x1b``
    ``badd u``  ``$r[DST] $r[SRC1] $r[SRC2S]`` ``0x1c``
    ``bsub u``  ``$r[DST] $r[SRC1] $r[SRC2S]`` ``0x1d``
    ``bmin s``  ``$r[DST] $r[SRC1] BIMM``      ``0x28``
    ``bmax s``  ``$r[DST] $r[SRC1] BIMM``      ``0x29``
    ``babs s``  ``$r[DST] $r[SRC1]``           ``0x2a``
    ``bneg s``  ``$r[DST] $r[SRC1]``           ``0x2b``
    ``badd s``  ``$r[DST] $r[SRC1] BIMM``      ``0x2c``
    ``bsub s``  ``$r[DST] $r[SRC1] BIMM``      ``0x2d``
    ``bmin u``  ``$r[DST] $r[SRC1] BIMM``      ``0x38``
    ``bmax u``  ``$r[DST] $r[SRC1] BIMM``      ``0x39``
    ``babs u``  ``$r[DST] $r[SRC1]``           ``0x3a``
    ``bneg u``  ``$r[DST] $r[SRC1]``           ``0x3b``
    ``badd u``  ``$r[DST] $r[SRC1] BIMM``      ``0x3c``
    ``bsub u``  ``$r[DST] $r[SRC1] BIMM``      ``0x3d``
    =========== ============================== ========
Operation:
    ::

        for idx in range(4):
            s1 = $r[SRC1][idx]
            if opcode & 0x20:
                s2 = BIMM
            else:
                s2 = $r[SRC2S][idx]

            if opcode & 0x10:
                # unsigned
                s1 &= 0xff
                s2 &= 0xff
            else:
                # signed
                s1 = sext(s1, 7)
                s2 = sext(s2, 7)

            if op == 'bmin':
                res = min(s1, s2)
            elif op == 'bmax':
                res = max(s1, s2)
            elif op == 'babs':
                res = abs(s1)
            elif op == 'bneg':
                res = -s1
            elif op == 'badd':
                res = s1 + s2
            elif op == 'bsub':
                res = s1 - s2

            if opcode & 0x10:
                # unsigned: clip to 0..0xff
                if res < 0:
                    res = 0
                if res > 0xff:
                    res = 0xff
            else:
                # signed: clip to -0x80..0x7f
                if res < -0x80:
                    res = -0x80
                if res > 0x7f:
                    res = 0x7f

            $r[DST][idx] = res

        if CDST < 4:
            $c[CDST].scalar = 0


.. _vp1-ops-byte-bitop:

Bytewise bit operations: band, bor, bxor
----------------------------------------

.. todo:: write me


.. _vp1-ops-bmul:

Bytewise multiplication: bmul
-----------------------------

.. todo:: write me


.. _vp1-ops-vec:

Send immediate to vector unit: vec
----------------------------------

.. todo:: write me


.. _vp1-ops-vecms:

Send mask to vector unit and shift: vecms
-----------------------------------------

.. todo:: write me


.. _vp1-ops-bvec:

Send bytes to vector unit: bvec
-------------------------------

.. todo:: write me


.. _vp1-ops-bvecmad:

Bytewise multiply, add, and send to vector unit: bvecmad
--------------------------------------------------------

.. todo:: write me
