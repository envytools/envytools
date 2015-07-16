.. _vp1-vector:

===========
Vector unit
===========

.. contents::


Introduction
============

The vector unit is one of the four execution units of VP1.  It operates in
SIMD manner on 16-element vectors.


.. _vp1-reg-vector:

Vector registers
----------------

The vector unit has 32 vector registers, ``$v0-$v31``.  They are 128 bits wide
and are treated as 16 components of 8 bits each.  Depending on element, they
can be treated as signed or unsigned.

There are also 4 vector condition code registers, ``$vc0-$vc3``.  They are like
``$c`` for vector registers - each of them has 16 "sign flag" and 16 "zero flag"
bits, one of each per vector component.  When read as a 32-word, bits 0-15 are
the sign flags and bits 16-31 are the zero flags.

Further, the vector unit has a singular 448-bit vector accumulator register,
``$va``.  It is made of 16 components, each of them a 28-bit signed number
with 16 fractional bits.  It's used to store intermediate unrounded results
of multiply-add computations.

Finally, there's an extra 128-bit register, ``$vx``, which works quite like
the usual ``$v`` registers.  It's only read by :ref:`vlrp4b <vp1-opv-lrp4b>`
instructions and written only by special :ref:`load to vector extra register
<vp1-opa-ldax>` instructions.  The reasons for its existence are unclear.


.. _vp1-vector-insn-format:

Instruction format
==================

The instruction word fields used in vector instructions in addition to
:ref:`the ones used in scalar instructions <vp1-scalar-insn-format>` are:

- bit 0: ``S2VMODE`` - selects how s2v data is used:

  - 0: ``factor`` - s2v data is interpreted as factors
  - 1: ``mask`` - s2v data is interpreted as masks

- bits 0-2: ``VCDST`` - if < 4, index of ``$vc`` register to set according
  to the instruction's results.  Otherwise, an indication that ``$vc``
  is not to be written (the canonical value for such case appears to be 7).

- bits 0-1: ``VCSRC`` - selects ``$vc`` input for ``vlrp2``

- bit 2: ``VCSEL`` - the ``$vc`` flag selection for ``vlrp2``:

  - 0: ``sf``
  - 1: ``zf``

- bit 3: ``SWZLOHI`` - selects how the swizzle selectors are decoded:

  - 0: ``lo`` - bits 0-3 are component selector, bit 4 is source selector
  - 1: ``hi`` - bits 4-7 are component selector, bit 0 is source selector

- bit 3: ``FRACTINT`` - selects whether the multiplication is considered
  to be integer or fixed-point:

  - 0: ``fract``: fixed-point
  - 1: ``int``: integer

- bit 4: ``HILO`` - selects which part of multiplication result to read:

  - 0: ``hi``: high part
  - 1: ``lo``: low part

- bits 5-7: ``SHIFT`` - a 3-bit signed immediate, used as an extra right shift
  factor

- bits 4-8: ``SRC3`` - the third source ``$v`` register.

- bit 9: ``ALTRND`` - like ``RND``, but for different instructions

- bit 9: ``SIGNS`` - determines if double-interpolation input is signed

  - 0: ``u`` - unsigned
  - 1: ``s`` - signed

- bit 10: ``LRP2X`` - determines if base input is XORed with 0x80 for ``vlrp2``.

- bit 11: ``VAWRITE`` - determines if ``$va`` is written for ``vlrp2``.

- bits 11-13: ``ALTSHIFT`` - a 3-bit signed immediate, used as an extra right
  shift factor

- bit 12: ``SIGND`` - determines if double-interpolation output is signed

  - 0: ``u`` - unsigned
  - 1: ``s`` - signed

- bits 19-22: ``CMPOP``: selects the bit operation to perform on comparison
  result and previous flag value


Opcodes
-------

The opcode range assigned to the vector unit is ``0x80-0xbf``.  The opcodes
are:

- ``0x80``, ``0xa0``, ``0xb0``, ``0x81``, ``0x91``, ``0xa1``, ``0xb1``: :ref:`multiplication: vmul <vp1-opv-mul>`
- ``0x90``: :ref:`linear interpolation: vlrp <vp1-opv-lrp>`
- ``0x82``, ``0x92``, ``0xa2``, ``0xb2``, ``0x83``, ``0x93``, ``0xa3``: :ref:`multiplication with accumulation: vmac <vp1-opv-mul>`
- ``0x84``, ``0x85``, ``0x95``: :ref:`dual multiplication with accumulation: vmac2 <vp1-opv-mul2>`
- ``0x86``, ``0x87``, ``0x97``: :ref:`dual multiplication with addition: vmad2 <vp1-opv-mul2>`
- ``0x96``, ``0xa6``, ``0xa7``: :ref:`dual multiplication with addition: vmad2 (bad opcode) <vp1-opv-mul2>`
- ``0x94``: :ref:`bitwise operation: vbitop <vp1-opv-bitop>`
- ``0xa4``: :ref:`clip to range: vclip <vp1-opv-clip>`
- ``0xa5``: :ref:`minimum of absolute values: vminabs <vp1-opv-minabs>`
- ``0xb3``: :ref:`dual linear interpolation: vlrp2 <vp1-opv-lrp2>`
- ``0xb4``: :ref:`quad linear interpolation, part 1: vlrp4a <vp1-opv-lrp4a>`
- ``0xb5``: :ref:`factor linear interpolation: vlrpf <vp1-opv-lrpf>`
- ``0xb6``, ``0xb7``: :ref:`quad linear interpolation, part 2: vlrp4b <vp1-opv-lrp4b>`
- ``0x88``, ``0x98``, ``0xa8``, ``0xb8``: :ref:`minimum: vmin <vp1-opv-arith>`
- ``0x89``, ``0x99``, ``0xa9``, ``0xb9``: :ref:`maximum: vmax <vp1-opv-arith>`
- ``0x8a``, ``0x9a``: :ref:`absolute value: vabs <vp1-opv-arith>`
- ``0xaa``: :ref:`immediate and: vand <vp1-opv-bitop-imm>`
- ``0xba``: :ref:`move: mov <vp1-opv-mov>`
- ``0x8b``: :ref:`negation: vneg <vp1-opv-arith>`
- ``0x9b``: :ref:`swizzle: vswz <vp1-opv-swz>`
- ``0xab``: :ref:`immediate xor: vxor <vp1-opv-bitop-imm>`
- ``0xbb``: :ref:`move from $vc: mov <vp1-opv-mov-vc>`
- ``0x8c``, ``0x9c``, ``0xac``, ``0xbc``: :ref:`addition: vadd <vp1-opv-arith>`
- ``0x8d``, ``0x9d``, ``0xbd``: :ref:`substraction: vsub <vp1-opv-arith>`
- ``0xad``: :ref:`move immediate: vmov <vp1-opv-mov-imm>`
- ``0x8e``, ``0x9e``, ``0xae``, ``0xbe``: :ref:`shift: vshr, vsar <vp1-opv-shift>`
- ``0x8f``: :ref:`compare with absolute difference: vcmpad <vp1-opv-cmpad>`
- ``0x9f``: :ref:`add 9-bit: vadd9 <vp1-opv-add9>`
- ``0xaf``: :ref:`immediate or: vor <vp1-opv-bitop-imm>`
- ``0xbf``: the canonical vector nop opcode


Multiplication, accumulation, and rounding
==========================================

The most advanced vector instructions involve multiplication and the vector
accumulator.  The vector unit has two multipliers (signed 10-bit * 10-bit
-> signed 20-bit) and three wide adders (performing 28-bit addition): the first
two add the multiplication results, and the third adds a rounding correction.
In other words, it can compute A + (B * C << S) + (D * E << S) + R, where A is
28-bit input, B, C, D, E are signed 10-bit inputs, S is either 0 or 8, and R
is the rounding correction, determined from the readout parameters.  The B, C,
D, E inputs can in turn be computed from other inputs using one of the narrower
ALUs.

The A input can come from the vector accumulator, be fixed to 0, or come from
a vector register component shifted by some shift amount.  The shift amount,
if used, is the inverse of the shift amount used by the readout process.

There are three things that can happen to the result of the multiply-accumulate
calculations:

- written in its entirety to the vector accumulator
- shifted, rounded, clipped, and written to a vector register
- both of the above

The vector register readout process takes the following parameters:

- sign: whether the result should be unsigned or signed
- fract/int selection: if int, the multiplication is considered to be done
  on integers, and the 16-bit result is at bits 8-23 of the value added
  to the accumulator (ie. S is 8).  Otherwise, the multiplication is performed
  as if the inputs were fractions (unsigned with 8 fractional bits, signed
  with 7), and the results are aligned so that bits 16-27 of the accumulator
  are integer part, and 0-15 are fractional part.
- hi/lo selection: selects whether high or low 8 bits of the results are read.
  For integers, the result is treated as 16-bit integer.  For fractions, the
  high part is either an unsigned fixed-point number with 8 fractional bits,
  or a signed number with 7 fractional bits, and the low part is always 8 bits
  lower than the high part.
- a right shift, in range of -4..3: the result is shifted right by that amount
  before readout (as usual, negative means left shift).
- rounding mode: either round down, or round to nearest.  If round to nearest
  is selected, a configuration bit in ``$uccfg`` register selects if ties are
  rounded up or down (to accomodate video codecs which switch that on frame
  basis).

First, any inputs from vector registers are read, converted as signed or
unsigned integers, and normalized if needed::

    def mad_input(val, fractint, isign):
        if isign == 'u':
            return val & 0xff
        else:
            if fractint == 'int':
                return sext(val, 7)
            else:
                return sext(val, 7) << 1

The readout shift factor is determined as follows::

    def mad_shift(fractint, sign, shift):
        if fractint == 'int':
            return 16 - shift
        elif sign == 'u':
            return 8 - shift
        elif sign == 's':
            return 9 - shift

If A is taken from a vector register, it's expanded as follows::

    def mad_expand(val, fractint, sign, shift):
        return val << mad_shift(fractint, sign, shift)

The actual multiply-add process works like that::

    def mad(a, b, c, d, e, rnd, fractint, sign, shift, hilo):
        res = a

        if fractint == 'fract':
            res += b * c + d * e
        else:
            res += (b * c + d * e) << 8

        # rounding correction
        if rnd == 'rn':

            # determine the final readout shift
            if hilo == 'lo':
                rshift = mad_shift(fractint, sign, shift) - 8
            else:
                rshift = mad_shift(fractint, sign, shift)

            # only add rounding correction if there's going to be an actual
            # right shift
            if rshift > 0:
                res += 1 << (rshift - 1)
                if $uccfg.tiernd == 'down':
                    res -= 1

        # the accumulator is only 28 bits long, and it wraps
        return sext(res, 27)

And the readout process is::

    def mad_read(val, fractint, sign, shift, hilo):
        # first, shift it to the position
        rshift = mad_shift(fractint, sign, shift) - 8
        if rshift >= 0:
            res = val >> rshift
        else:
            res = val << -rshift

        # second, clip to 16-bit signed or unsigned
        if sign == 'u':
            if res < 0:
                res = 0
            if res > 0xffff:
                res = 0xffff
        else:
            if res < -0x8000:
                res = -0x8000
            if res > 0x7fff:
                res = 0x7fff

        # finally, extract high/low part of the final result
        if hilo == 'hi':
            return res >> 8 & 0xff
        else:
            return res & 0xff

Note that high/low selection, apart from actual result readout, also affects
the rounding computation.  This means that, if rounding is desired and
the full 16-bit result is to be read, the low part should be read first with
rounding (which will add the rounding correction to the accumulator) and
then the high part should be read without rounding (since the rounding
correction is already applied).


Instructions
============


.. _vp1-opv-mov:

Move: mov
---------

Copies one register to another.  ``$vc`` output supported for zero flag only.

Instructions:
    =========== ================================= ========
    Instruction Operands                          Opcode
    =========== ================================= ========
    ``mov``     ``[$vc[VCDST]] $v[DST] $v[SRC1]`` ``0xba``
    =========== ================================= ========
Operation:
    ::

        for idx in range(16):
            $v[DST][idx] = $v[SRC1][idx]
            if VCDST < 4:
                $vc[VCDST].sf[idx] = 0
                $vc[VCDST].zf[idx] = $v[DST][idx] == 0


.. _vp1-opv-mov-imm:

Move immediate: vmov
--------------------

Loads an 8-bit immediate to each component of destination.  ``$vc`` output
is fully supported, with sign flag set to bit 7 of the value.

Instructions:
    =========== ============================= ========
    Instruction Operands                      Opcode
    =========== ============================= ========
    ``vmov``    ``[$vc[VCDST]] $v[DST] BIMM`` ``0xad``
    =========== ============================= ========
Operation:
    ::

        for idx in range(16):
            $v[DST][idx] = BIMM
            if VCDST < 4:
                $vc[VCDST].sf[idx] = BIMM >> 7 & 1
                $vc[VCDST].zf[idx] = BIMM == 0


.. _vp1-opv-mov-vc:

Move from $vc: mov
------------------

Reads the contents of all ``$vc`` registers to a selected vector register.
Bytes 0-3 correspond to ``$vc0``, bytes 4-7 to ``$vc1``, and so on.  The sign
flags are in bytes 0-1, and the zero flags are in bytes 2-3.

Instructions:
    =========== ================= ========
    Instruction Operands          Opcode
    =========== ================= ========
    ``mov``     ``$v[DST] $vc``   ``0xbb``
    =========== ================= ========
Operation:
    ::

        for idx in range(4):
            $v[DST][idx * 4] = $vc[idx].sf & 0xff;
            $v[DST][idx * 4 + 1] = $vc[idx].sf >> 8 & 0xff;
            $v[DST][idx * 4 + 2] = $vc[idx].zf & 0xff;
            $v[DST][idx * 4 + 3] = $vc[idx].zf >> 8 & 0xff;


.. _vp1-opv-swz:

Swizzle: vswz
-------------

Performs a swizzle, also known as a shuffle: builds a result vector from
arbitrarily selected components of two input vectors.  There are three
source vectors: sources 1 and 2 supply the data to be used, while source 3
selects the mapping of output vector components to input vector components.
Each component of source 3 consists of source selector and component selector.
They select the source (1 or 2) and its component that will be used as
the corresponding component of the result.

Instructions:
    =========== =================================================== ========
    Instruction Operands                                            Opcode
    =========== =================================================== ========
    ``vswz``    ``SWZLOHI $v[DST] $v[SRC1] $v[SRC2] $v[SRC3]``      ``0x9b``
    =========== =================================================== ========
Operation:
    ::

        for idx in range(16):
            # read the component and source selectors
            if SWZLOHI == 'lo':
                comp = $v[SRC3][idx] & 0xf
                src = $v[SRC3][idx] >> 4 & 1
            else:
                comp = $v[SRC3][idx] >> 4 & 0xf
                src = $v[SRC3][idx] & 1

            # read the source & component
            if src == 0:
                $v[DST][idx] = $v[SRC1][comp]
            else:
                $v[DST][idx] = $v[SRC2][comp]


.. _vp1-opv-arith:

Simple arithmetic operations: vmin, vmax, vabs, vneg, vadd, vsub
----------------------------------------------------------------

Those perform the corresponding operation (minumum, maximum, absolute value,
negation, addition, substraction) in SIMD manner on 8-bit signed or unsigned
numbers from one or two sources.  Source 1 is always a register selected by
``SRC1`` bitfield.  Source 2, if it is used (ie. instruction is not ``vabs``
nor ``vneg``), is either a register selected by ``SRC2`` bitfield, or
immediate taken from ``BIMM`` bitfield.

Most of these instructions come in signed and unsigned variants and both
perform result clipping.  The exception is ``vneg``, which only has a signed
version.  Note that ``vabs`` is rather uninteresting in its unsigned variant
(it's just the identity function).  Note that ``vsub`` lacks a signed version
with immediat: it can be replaced with ``vadd`` with negated immediate.

``$vc`` output is fully supported.  For signed variants, the sign flag output
is the sign of the result.  For unsigned variants, the sign flag is used as
an overflow flag: it's set if the true unclipped result is not in ``0..0xff``
range.

Instructions:
    =========== ========================================== ========
    Instruction Operands                                   Opcode
    =========== ========================================== ========
    ``vmin s``  ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2]`` ``0x88``
    ``vmax s``  ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2]`` ``0x89``
    ``vabs s``  ``[$vc[VCDST]] $v[DST] $v[SRC1]``          ``0x8a``
    ``vneg s``  ``[$vc[VCDST]] $v[DST] $v[SRC1]``          ``0x8b``
    ``vadd s``  ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2]`` ``0x8c``
    ``vsub s``  ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2]`` ``0x8d``
    ``vmin u``  ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2]`` ``0x98``
    ``vmax u``  ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2]`` ``0x99``
    ``vabs u``  ``[$vc[VCDST]] $v[DST] $v[SRC1]``          ``0x9a``
    ``vadd u``  ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2]`` ``0x9c``
    ``vsub u``  ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2]`` ``0x9d``
    ``vmin s``  ``[$vc[VCDST]] $v[DST] $v[SRC1] BIMM``     ``0xa8``
    ``vmax s``  ``[$vc[VCDST]] $v[DST] $v[SRC1] BIMM``     ``0xa9``
    ``vadd s``  ``[$vc[VCDST]] $v[DST] $v[SRC1] BIMM``     ``0xac``
    ``vmin u``  ``[$vc[VCDST]] $v[DST] $v[SRC1] BIMM``     ``0xb8``
    ``vmax u``  ``[$vc[VCDST]] $v[DST] $v[SRC1] BIMM``     ``0xb9``
    ``vadd u``  ``[$vc[VCDST]] $v[DST] $v[SRC1] BIMM``     ``0xbc``
    ``vsub u``  ``[$vc[VCDST]] $v[DST] $v[SRC1] BIMM``     ``0xbd``
    =========== ========================================== ========
Operation:
    ::

        for idx in range(16):
            s1 = $v[SRC1][idx]
            if opcode & 0x20:
                s2 = BIMM
            else:
                s2 = $v[SRC2][idx]

            if opcode & 0x10:
                # unsigned
                s1 &= 0xff
                s2 &= 0xff
            else:
                # signed
                s1 = sext(s1, 7)
                s2 = sext(s2, 7)

            if op == 'vmin':
                res = min(s1, s2)
            elif op == 'vmax':
                res = max(s1, s2)
            elif op == 'vabs':
                res = abs(s1)
            elif op == 'vneg':
                res = -s1
            elif op == 'vadd':
                res = s1 + s2
            elif op == 'vsub':
                res = s1 - s2

            sf = 0
            if opcode & 0x10:
                # unsigned: clip to 0..0xff
                if res < 0:
                    res = 0
                    sf = 1
                if res > 0xff:
                    res = 0xff
                    sf = 1
            else:
                # signed: clip to -0x80..0x7f
                if res < 0:
                    sf = 1
                if res < -0x80:
                    res = -0x80
                if res > 0x7f:
                    res = 0x7f

            $v[DST][idx] = res

            if VCDST < 4:
                $vc[VCDST].sf[idx] = sf
                $vc[VCDST].zf[idx] = res == 0


.. _vp1-opv-clip:

Clip to range: vclip
--------------------

Performs a SIMD range clipping operation: first source is the value to clip,
second and third sources are the range endpoints.  Or, equivalently,
calculates the median of three inputs.  ``$vc`` output is supported, with
the sign flag set if clipping was performed (value equal to range endpoint
is considered to be clipped) or the range is improper (second endpoint not
larger than the first).  All inputs are treated as signed.

Instructions:
    =========== =================================================== ========
    Instruction Operands                                            Opcode
    =========== =================================================== ========
    ``vclip``   ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2] $v[SRC3]`` ``0xa4``
    =========== =================================================== ========
Operation:
    ::

        for idx in range(16):
            s1 = sext($v[SRC1][idx], 7)
            s2 = sext($v[SRC2][idx], 7)
            s3 = sext($v[SRC3][idx], 7)

            sf = 0

            # determine endpoints
            if s2 < s3:
                # proper order
                start = s2
                end = s3
            else:
                # reverse order
                start = s3
                end = s2
                sf = 1

            # and clip
            res = s1
            if res <= start:
                res = start
                sf = 1
            if res >= end:
                res = end
                sf = 1

            $v[DST][idx] = res

            if VCDST < 4:
                $vc[VCDST].sf[idx] = sf
                $vc[VCDST].zf[idx] = res == 0


.. _vp1-opv-minabs:

Minimum of absolute values: vminabs
-----------------------------------

Performs ``min(abs(a), abs(b))``.  Both inputs are treated as signed.
``$vc`` output is supported for zero flag only.  The result is clipped
to ``0..0x7f`` range (which only matters if both inputs are ``-0x80``).

Instructions:
    =========== ========================================== ========
    Instruction Operands                                   Opcode
    =========== ========================================== ========
    ``vminabs`` ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2]`` ``0xa5``
    =========== ========================================== ========
Operation:
    ::

        for idx in range(16):
            s1 = sext($v[SRC1][idx], 7)
            s2 = sext($v[SRC2][idx], 7)

            res = min(abs(s1, s2))

            if res > 0x7f:
                res = 0x7f

            $v[DST][idx] = res

            if VCDST < 4:
                $vc[VCDST].sf[idx] = 0
                $vc[VCDST].zf[idx] = res == 0


.. _vp1-opv-add9:

Add 9-bit: vadd9
----------------

Performs an 8-bit unsigned + 9-bit signed addition (ie. exactly what's needed
for motion compensation).  The first source provides the 8-bit inputs, while
the second and third are uniquely treated as vectors of 8 16-bit components
(of which only low 9 are actually used).  Second source provides components
0-7, and third provides 8-15.  The result is unsigned and clipped. ``$vc``
output is supported, with sign flag set to 1 if the true result was out of
8-bit unsigned range.

Instructions:
    =========== =================================================== ========
    Instruction Operands                                            Opcode
    =========== =================================================== ========
    ``vadd9``   ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2] $v[SRC3]`` ``0x9f``
    =========== =================================================== ========
Operation:
    ::

        for idx in range(16):
            # read source 1
            s1 = $v[SRC1][idx]

            if idx < 8:
                # 0-7: SRC2
                s2l = $v[SRC2][idx * 2]
                s2h = $v[SRC2][idx * 2 + 1]
            else:
                # 8-15: SRC3
                s2l = $v[SRC3][(idx - 8) * 2]
                s2h = $v[SRC3][(idx - 8) * 2 + 1]

            # read as 9-bit signed number
            s2 = sext(s2h << 8 | s2l, 8)

            # add
            res = s1 + s2

            # clip
            sf = 0
            if res > 0xff:
                sf = 1
                res = 0xff
            if res < 0:
                sf = 1
                res = 0

            $v[DST][idx] = res

            if VCDST < 4:
                $vc[VCDST].sf[idx] = sf
                $vc[VCDST].zf[idx] = res == 0


.. _vp1-opv-cmpad:

Compare with absolute difference: vcmpad
----------------------------------------

This instruction performs the following operations:

- substract source 1.1 from source 2
- take the absolute value of the difference
- compare the result with source 1.2
- if equal, set zero flag of selected ``$vc`` output
- set sign flag of ``$vc`` output to :ref:`an arbitrary bitwise operation
  <bitop>` of s2v ``$vc`` input and "less than" comparison result

All inputs are treated as unsigned.  If s2v scalar instruction is not used
together with this instruction, ``$vc`` input defaults to sign flag of
the ``$vc`` register selected as output, with no transformation.

This instruction has two sources: source 1 is a register pair, while source 2
is a single register.  The second register of the pair is selected by ORing
1 to the index of the first register of the pair.  Source 2 is selected by
mangled field ``SRC2S``.

Instructions:
    =========== =================================================== ========
    Instruction Operands                                            Opcode
    =========== =================================================== ========
    ``vcmppad`` ``CMPOP [$vc[VCDST]] $v[SRC1]d $v[SRC2S]``          ``0x8f``
    =========== =================================================== ========
Operation:
    ::

        if s2v.vcsel.valid:
            vcin = s2v.vcmask
        else:
            vcin = $vc[VCDST & 3].sf

        for idx in range(16):
            ad = abs($v[SRC2S][idx] - $v[SRC1][idx])
            other = $v[SRC1 | 1][idx]

            if VCDST < 4:
                $vc[VCDST].sf[idx] = sf
                $vc[VCDST].zf[idx] = ad == bitop(CMPOP, vcin >> idx & 1, ad < other)


.. _vp1-opv-bitop:

Bit operations: vbitop
----------------------

Performs an :ref:`arbitrary two-input bit operation <bitop>` on two registers.
``$vc`` output supported for zero flag only.

Instructions:
    =========== =============================================== ========
    Instruction Operands                                        Opcode
    =========== =============================================== ========
    ``vbitop``  ``BITOP [$vc[CDST]] $v[DST] $v[SRC1] $v[SRC2]`` ``0x94``
    =========== =============================================== ========
Operation:
    ::

        for idx in range(16):
            s1 = $v[SRC1][idx]
            s2 = $v[SRC2][idx]

            res = bitop(BITOP, s2, s1) & 0xff

            $v[DST][idx] = res
            if VCDST < 4:
                $vc[VCDST].sf[idx] = 0
                $vc[VCDST].zf[idx] = res == 0


.. _vp1-opv-bitop-imm:

Bit operations with immediate: vand, vor, vxor
----------------------------------------------

Performs a given bitwise operation on a register and an 8-bit immediate
replicated for each component.  ``$vc`` output supported for zero flag only.

Instructions:
    =========== ====================================== ========
    Instruction Operands                               Opcode
    =========== ====================================== ========
    ``vand``    ``[$vc[VCDST]] $v[DST] $v[SRC1] BIMM`` ``0xaa``
    ``vxor``    ``[$vc[VCDST]] $v[DST] $v[SRC1] BIMM`` ``0xab``
    ``vor``     ``[$vc[VCDST]] $v[DST] $v[SRC1] BIMM`` ``0xaf``
    =========== ====================================== ========
Operation:
    ::

        for idx in range(16):
            s1 = $v[SRC1][idx]

            if op == 'vand':
                res = s1 & BIMM
            elif op == 'vxor':
                res = s1 ^ BIMM
            elif op == 'vor':
                res = s1 | BIMM

            $v[DST][idx] = res
            if VCDST < 4:
                $vc[VCDST].sf[idx] = 0
                $vc[VCDST].zf[idx] = res == 0


.. _vp1-opv-shift:

Shift operations: vshr, vsar
----------------------------

Performs a SIMD right shift, like the :ref:`scalar bytewise shift instruction
<vp1-ops-byte-shift>`.  ``$vc`` output is fully supported, with bit 7 of the
result used as the sign flag.

Instructions:
    =========== =========================================== ========
    Instruction Operands                                    Opcode
    =========== =========================================== ========
    ``vsar``    ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2]``  ``0x8e``
    ``vshr``    ``[$vc[VCDST]] $v[DST] $v[SRC1] $v[SRC2]``  ``0x9e``
    ``vsar``    ``[$vc[VCDST]] $v[DST] $v[SRC1] BIMM``      ``0xae``
    ``vshr``    ``[$vc[VCDST]] $v[DST] $v[SRC1] BIMM``      ``0xbe``
    =========== =========================================== ========
Operation:
    ::

        for idx in range(16):
            s1 = $v[SRC1][idx]
            if opcode & 0x20:
                s2 = BIMM
            else:
                s2 = $v[SRC2][idx]

            if opcode & 0x10:
                # unsigned
                s1 &= 0xff
            else:
                # signed
                s1 = sext(s1, 7)

            shift = sext(s2, 3)

            if shift < 0:
                res = s1 << -shift
            else:
                res = s1 >> shift

            $v[DST][idx] = res

            if VCDST < 4:
                $vc[VCDST].sf[idx] = res >> 7 & 1
                $vc[VCDST].zf[idx] = res == 0

.. _vp1-opv-lrp:

Linear interpolation: vlrp
--------------------------

A SIMD linear interpolation instruction.  Takes two sources: a register pair
containing the two values to interpolate, and a register containing the
interpolation factor.  The result is basically ``SRC1.1 * (SRC2 >> SHIFT) +
SRC1.2 * (1 - (SRC2 >> SHIFT))``.  All inputs are unsigned fractions.

Instructions:
    =========== =========================================== ========
    Instruction Operands                                    Opcode
    =========== =========================================== ========
    ``vlrp``    ``RND SHIFT $v[DST] $v[SRC1]d $v[SRC2]``    ``0x90``
    =========== =========================================== ========
Operation:
    ::

        for idx in range(16):
            val1 = $v[SRC1][idx]
            val2 = $v[SRC1 | 1][idx]
            a = mad_expand(val2, 'fract', 'u', SHIFT)
            res = mad(a, val1 - val2, $v[SRC2][idx], 0, 0, RND, 'fract', 'u', SHIFT, 'hi')
            $v[DST][idx] = mad_read(res, 'fract', 'u', SHIFT, 'hi')


.. _vp1-opv-mul:

Multiply and multiply with accumulate: vmul, vmac
-------------------------------------------------

Performs a simple multiplication of two sources (but with the full set of weird
options available).  The result is either added to the vector accumulator
(``vmac``) or replaces it (``vmul``).  The result can additionally be read
to a vector register, but doesn't have to be.

The instructions come in many variants: they can store the result in a vector
register or not, have unsigned or signed output, and register or immediate
second source.  The set of available combinations is incomplete, however:
while the ``$v``-writing variants have all combinations available, there are
no unsigned variants of register-register ``vmul`` with no ``$v`` write, nor
unsigned register-immediate ``vmac`` with no ``$v`` write.  Also, unsigned
register-immediate ``vmul`` with no ``$v`` output is a :ref:`bad opcode
<vp1-bad-opcode>`.

Instructions:
    =========== ================================================================= ========
    Instruction Operands                                                          Opcode
    =========== ================================================================= ========
    ``vmul s``  ``RND FRACTINT SHIFT HILO # SIGN1 $v[SRC1] SIGN2 $v[SRC2]``       ``0x80``
    ``vmul s``  ``RND FRACTINT SHIFT HILO # SIGN1 $v[SRC1] SIGN2 BIMMMUL``        ``0xa0``
    ``vmul u``  ``RND FRACTINT SHIFT HILO # SIGN1 $v[SRC1] SIGN2 BIMMBAD``        ``0xb0`` (bad opcode)
    ``vmul s``  ``RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1] SIGN2 $v[SRC2]`` ``0x81``
    ``vmul u``  ``RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1] SIGN2 $v[SRC2]`` ``0x91``
    ``vmul s``  ``RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1] SIGN2 BIMMMUL``  ``0xa1``
    ``vmul u``  ``RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1] SIGN2 BIMMMUL``  ``0xb1``
    ``vmac s``  ``RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1] SIGN2 $v[SRC2]`` ``0x82``
    ``vmac u``  ``RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1] SIGN2 $v[SRC2]`` ``0x92``
    ``vmac s``  ``RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1] SIGN2 BIMMMUL``  ``0xa2``
    ``vmac u``  ``RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1] SIGN2 BIMMMUL``  ``0xb2``
    ``vmac s``  ``RND FRACTINT SHIFT HILO # SIGN1 $v[SRC1] SIGN2 $v[SRC2]``       ``0x83``
    ``vmac u``  ``RND FRACTINT SHIFT HILO # SIGN1 $v[SRC1] SIGN2 $v[SRC2]``       ``0x93``
    ``vmac s``  ``RND FRACTINT SHIFT HILO # SIGN1 $v[SRC1] SIGN2 BIMMMUL``        ``0xa3``
    =========== ================================================================= ========
Operation:
    ::

        for idx in range(16):
            # read inputs
            s1 = $v[SRC1][idx]
            if opcode & 0x20:
                if op == 0x30:
                    s2 = BIMMBAD
                else:
                    s2 = BIMMMUL << 2
            else:
                s2 = $v[SRC2][idx]

            # convert inputs
            s1 = mad_input(s1, FRACTINT, SIGN1)
            s2 = mad_input(s2, FRACTINT, SIGN2)

            # do the computation
            if op == 'vmac':
                a = $va[idx]
            else:
                a = 0
            res = mad(a, s1, s2, 0, 0, RND, FRACTINT, op.sign, SHIFT, HILO)

            # write result
            $va[idx] = res
            if DST is not None:
                $v[DST][idx] = mad_read(res, FRACTINT, op.sign, SHIFT, HILO)


.. _vp1-opv-mul2:

Dual multiply and add/accumulate: vmac2, vmad2
----------------------------------------------

Performs two multiplications and adds the result to a given source or to
the vector accumulator.  The result is written to the vector accumulator
and can also be written to a ``$v`` register.  For each multiplication,
one input is a register source, and the other is s2v factor.  The register
sources for the multiplications are a register pair.  The s2v sources
for the multiplications are either s2v factors (one factor from each pair
is selected  according to s2v ``$vc`` input) or 0/1 as decided by s2v
mask.

The instructions come in signed and unsigned variants.  Apart from some
bad opcodes (which overlay ``SRC3`` with mad param fields), only ``$v``
writing versions have unsigned variants.

Instructions:
    =========== ========================================================================== ========
    Instruction Operands                                                                   Opcode
    =========== ========================================================================== ========
    ``vmad2 s`` ``S2VMODE RND FRACTINT SHIFT HILO # SIGN1 $v[SRC1]d SIGN2 $v[SRC2]``       ``0x84``
    ``vmad2 s`` ``S2VMODE RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1]d SIGN2 $v[SRC2]`` ``0x85``
    ``vmad2 u`` ``S2VMODE RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1]d SIGN2 $v[SRC2]`` ``0x95``
    ``vmac2 s`` ``S2VMODE RND FRACTINT SHIFT HILO # SIGN1 $v[SRC1]d``                      ``0x86``
    ``vmac2 u`` ``S2VMODE RND FRACTINT SHIFT HILO # SIGN1 $v[SRC1] $v[SRC3]``              ``0x96`` (bad opcode)
    ``vmac2 s`` ``S2VMODE RND FRACTINT SHIFT HILO # SIGN1 $v[SRC1] $v[SRC3]``              ``0xa6`` (bad opcode)
    ``vmac2 s`` ``S2VMODE RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1]d``                ``0x87``
    ``vmac2 u`` ``S2VMODE RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1]d``                ``0x97``
    ``vmac2 s`` ``S2VMODE RND FRACTINT SHIFT HILO $v[DST] SIGN1 $v[SRC1] $v[SRC3]``        ``0xa7`` (bad opcode)
    =========== ========================================================================== ========
Operation:
    ::

        for idx in range(16):
            # read inputs
            s11 = $v[SRC1][idx]
            if opcode in (0x96, 0xa6, 0xa7):
                # one of the bad opcodes
                s12 = $v[SRC3][idx]
            else:
                s12 = $v[SRC1 | 1][idx]

            s2 = $v[SRC2][idx]

            # convert inputs
            s11 = mad_input(s11, FRACTINT, SIGN1)
            s12 = mad_input(s12, FRACTINT, SIGN1)
            s2 = mad_input(s2, FRACTINT, SIGN2)

            # prepare A value
            if op == 'vmad2':
                a = mad_expand(s2, FRACTINT, sign, SHIFT)
            else:
                a = $va[idx]

            # prepare factors
            if S2VMODE == 'mask':
                if s2v.mask[0] & 1 << idx:
                    f1 = 0x100
                else:
                    f1 = 0
                if s2v.mask[1] & 1 << idx:
                    f2 = 0x100
                else:
                    f2 = 0
            else:
                # 'factor'
                cc = s2v.vcmask >> idx & 1
                f1 = s2v.factor[0 | cc]
                f2 = s2v.factor[2 | cc]

            # do the operation
            res = mad(a, s11, f1, s12, f2, RND, FRACTINT, sign, SHIFT, HILO)

            # write result
            $va[idx] = res
            if DST is not None:
                $v[DST][idx] = mad_read(res, FRACTINT, op.sign, SHIFT, HILO)


.. _vp1-opv-lrp2:

Dual linear interpolation: vlrp2
--------------------------------

This instruction performs the following steps:

- read a quad register source selected by ``SRC1``
- rotate the source quad by the amount selected by bits 4-5 of a selected ``$c``
  register
- for each component:

  - treat register 0 of the quad as function value at (0, 0)
  - treat register 2 as value at (1, 0)
  - treat register 3 as value at (0, 1)
  - select a pair of factors from s2v input based on selected flag of selected
    ``$vc`` register
  - treat the factors as a coordinate pair and interpolate function value at
    these coordinates
  - write result to ``$v`` register and optionally ``$va``

The inputs and outputs may be signed or unsigned.  A shift and rounding mode
can be selected.  Additionally, there's an option to XOR register 0 with 0x80
before use as the base value (but not for the differences used in
interpolation).  Don't ask me.

Instructions:
    =========== =================================================================================== ========
    Instruction Operands                                                                            Opcode
    =========== =================================================================================== ========
    ``vlrp2``   ``SIGND VAWRITE RND SHIFT $v[DST] SIGNS LRP2X $v[SRC1]q $c[COND] $vc[VCSRC] VCSEL`` ``0xb3``
    =========== =================================================================================== ========
Operation:
    ::

        # a function selecting the factors
        def get_lrp2_factors(idx):
            if VCSEL == 'sf':
                vcmask = $vc[VCSRC].sf
            else:
                vcmask = $vc[VCSRC].zf

            cc = vcmask >> idx & 1;
            f1 = s2v.factor[0 | cc]
            f2 = s2v.factor[2 | cc]

            return f1, f2

        # determine rotation
        rot = $c[COND] >> 4 & 3

        for idx in range(16):
            # read inputs, maybe do the xor
            s10x = s10 = $v[(SRC1 & 0x1c) | ((SRC1 + rot) & 3)][idx]
            s12 = $v[(SRC1 & 0x1c) | ((SRC1 + rot + 2) & 3)][idx]
            s13 = $v[(SRC1 & 0x1c) | ((SRC1 + rot + 3) & 3)][idx]
            if LRP2X:
                s10x ^= 0x80

            # convert inputs if necessary
            s10 = mad_input(s10, 'fract', SIGNS)
            s12 = mad_input(s12, 'fract', SIGNS)
            s13 = mad_input(s13, 'fract', SIGNS)
            s10x = mad_input(s10x, 'fract', SIGNS)

            # do it
            a = mad_expand(s10x, 'fract', SIGND, SHIFT)
            f1, f2 = get_lrp2_factors(idx)
            res = mad(a, s12 - s10, f1, s13 - s10, f2, RND, 'fract', SIGND, SHIFT, 'hi')

            # write outputs
            if VAWRITE:
                $va[idx] = res
            $v[DST][idx] = mad_read(res, 'fract', SIGND, SHIFT, 'hi')


.. _vp1-opv-lrp4a:

Quad linear interpolation, part 1: vlrp4a
-----------------------------------------

Works like the previous variant, but only outputs to ``$va`` and lacks some
flags.  Both outputs and inputs are unsigned.

Instructions:
    =========== =================================================== ========
    Instruction Operands                                            Opcode
    =========== =================================================== ========
    ``vlrp4a``  ``RND SHIFT # $v[SRC1]q $c[COND] $vc[VCSRC] VCSEL`` ``0xb4``
    =========== =================================================== ========
Operation:
    ::

        rot = $c[COND] >> 4 & 3

        for idx in range(16):

            s10 = $v[(SRC1 & 0x1c) | ((SRC1 + rot) & 3)][idx]
            s12 = $v[(SRC1 & 0x1c) | ((SRC1 + rot + 2) & 3)][idx]
            s13 = $v[(SRC1 & 0x1c) | ((SRC1 + rot + 3) & 3)][idx]

            a = mad_expand(s10, 'fract', 'u', SHIFT)
            f1, f2 = get_lrp2_factors(idx)

            $va[idx] = mad(a, s12 - s10, f1, s13 - s10, f2, RND, 'fract', 'u', SHIFT, 'lo')


.. _vp1-opv-lrpf:

Factor linear interpolation: vlrpf
----------------------------------

Has similiar input processing to ``vlrp2``, but instead uses source 1 registers
2 and 3 to interpolate s2v input.  Result is ``SRC2 + SRC1.2 * F1 + SRC1.3 *
(F2 - F1)``.

Instructions:
    =========== ============================================================ ========
    Instruction Operands                                                     Opcode
    =========== ============================================================ ========
    ``vlrpf``   ``RND SHIFT # $v[SRC1]q $c[COND] $v[SRC2] $vc[VCSRC] VCSEL`` ``0xb5``
    =========== ============================================================ ========
Operation:
    ::

        rot = $c[COND] >> 4 & 3

        for idx in range(16):

            s12 = $v[(SRC1 & 0x1c) | ((SRC1 + rot + 2) & 3)][idx]
            s13 = $v[(SRC1 & 0x1c) | ((SRC1 + rot + 3) & 3)][idx]
            s2 = sext($v[SRC2][idx], 7)

            a = mad_expand(s2, 'fract', 'u', SHIFT)
            f1, f2 = get_lrp2_factors(idx)

            $va[idx] = mad(a, s12 - s13, f1, s13, f2, RND, 'fract', 'u', SHIFT, 'lo')


.. _vp1-opv-lrp4b:

Quad linear interpolation, part 2: vlrp4b
-----------------------------------------

Can be used together with ``vlrp4a`` for quad linear interpolation.  First
s2v factor is the interpolation coefficient for register 1, and second factor
is the interpolation coefficient for the extra register (``$vx``).

Alternatively, can be coupled with ``vlrpf``.

Instructions:
    ============ ==================================================================== ========
    Instruction  Operands                                                             Opcode
    ============ ==================================================================== ========
    ``vlrp4b u`` ``ALTRND ALTSHIFT $v[DST] $v[SRC1]q $c[COND] SLCT $vc[VCSRC] VCSEL`` ``0xb6``
    ``vlrp4b s`` ``ALTRND ALTSHIFT $v[DST] $v[SRC1]q $c[COND] SLCT $vc[VCSRC] VCSEL`` ``0xb7``
    ============ ==================================================================== ========
Operation:
    ::

        for idx in range(16):
            if SLCT == 4:
                    rot = $c[COND] >> 4 & 3
                    s10 = $v[(SRC1 & 0x1c) | ((SRC1 + rot) & 3)][idx]
                    s11 = $v[(SRC1 & 0x1c) | ((SRC1 + rot + 1) & 3)][idx]
            else:
                    adjust = $c[COND] >> SLCT & 1
                    s10 = s11 = $v[src1 ^ adjust][idx]

            f1, f2 = get_lrp2_factors(idx)

            res = mad($va[idx], s11 - s10, f1, $vx[idx] - s10, f2, ALTRND, 'fract', op.sign, ALTSHIFT, 'hi')

            $va[idx] = res
            $v[DST][idx] = mad_read(res, 'fract', op.sign, ALTSHIFT, 'hi')
