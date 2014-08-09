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

Finally, the vector unit has a singular 448-bit vector accumulator register,
``$va``.  It is made of 16 components, each of them a 28-bit signed number
with 16 fractional bits.  It's used to store intermediate unrounded results
of multiply-add computations.


.. _vp1-vector-insn-format:

Instruction format
==================

The instruction word fields used in vector instructions in addition to
:ref:`the ones used in scalar instructions <vp1-scalar-insn-format>` are:

- bits 0-2: ``VCDST`` - if < 4, index of ``$vc`` register to set according
  to the instruction's results.  Otherwise, an indication that ``$vc``
  is not to be written (the canonical value for such case appears to be 7).

- bit 3: ``SWZLOHI`` - selects how the swizzle selectors are decoded:

  - 0: ``lo`` - bits 0-3 are component selector, bit 4 is source selector
  - 1: ``hi`` - bits 4-7 are component selector, bit 0 is source selector

- bits 4-8: ``SRC3`` - the third source ``$v`` register.

- bits 19-22: ``CMPOP``: selects the bit operation to perform on comparison
  result and previous flag value

.. todo:: write me


Opcodes
-------

The opcode range assigned to the vector unit is ``0x80-0xbf``.  The opcodes
are:

- ``0x94``: :ref:`bitwise operation: vbitop <vp1-opv-bitop>`
- ``0xa4``: :ref:`clip to range: vclip <vp1-opv-clip>`
- ``0xa5``: :ref:`minimum of absolute values: vminabs <vp1-opv-minabs>`
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

.. todo:: list me


Instructions
============

.. todo:: write me


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
    ``vswz``    ``SWZLOHI $v[DST] $v[SRC1] $v[SRC2] $v[SRC3]``     ``0x9b``
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
1 to the index of the first register of the pair.

Instructions:
    =========== =================================================== ========
    Instruction Operands                                            Opcode
    =========== =================================================== ========
    ``vcmppad`` ``CMPOP [$vc[VCDST]] $v[SRC1]d $v[SRC2]``           ``0x8f``
    =========== =================================================== ========
Operation:
    ::

        if s2v.vcsel.valid:
            vcin = s2v.vcmask
        else:
            vcin = $vc[VCDST & 3].sf

        for idx in range(16):
            ad = abs($v[SRC2][idx] - $v[SRC1][idx])
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

            res = bitop(BITOP, s1, s2) & 0xff

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
