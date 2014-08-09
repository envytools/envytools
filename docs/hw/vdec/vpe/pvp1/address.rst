.. _vp1-address:

============
Address unit
============

.. contents::


Introduction
============

The address unit is one of the four execution units of VP1.  It transfers
data between that :ref:`data store <vp1-data>` and registers, controls
the :ref:`DMA unit <vp1-dma>`, and performs address calculations.


.. _vp1-reg-address:

Address registers
-----------------

The address unit has 32 address registers, ``$a0-$a31``.  These are used for
address storage.  If they're used to store data store addresses (and not DMA
command parameters), they have the following bitfields:

- bits 0-15: ``addr`` - the actual data store address
- bits 16-29: ``limit`` - can store the high bounduary of an array, to assist
  in looping
- bits 30-31: ``stride`` - selects data store stride:

  - 0: 0x10 bytes
  - 1: 0x20 bytes
  - 2: 0x40 bytes
  - 3: 0x80 bytes

There are also 3 bits in each ``$c`` register belonging to the address unit.
They are:

- bits 8-9: long address flags

  - bit 8: sign flag - set equal to bit 31 of the result
  - bit 9: zero flag - set if the result is 0

- bit 10: short address flag

  - bit 10: end flag - set if ``addr`` field of the result is greater than or
    equal to ``limit``

Some address instructions set either the long or short flags of a given ``$c``
register according to the result.


.. _vp1-address-insn-format:

Instruction format
==================

The instruction word fields used in address instructions in addition to
:ref:`the ones used in scalar instructions <vp1-scalar-insn-format>` are:

.. todo:: list me


Opcodes
-------

The opcode range assigned to the address unit is ``0xc0-0xdf``.  The opcodes
are:

- ``0xca``: :ref:`address addition: aadd <vp1-opa-aadd>`
- ``0xcb``: :ref:`addition: add <vp1-opa-add>`
- ``0xcc``: :ref:`set low bits: setlo <vp1-opa-set>`
- ``0xcd``: :ref:`set high bits: sethi <vp1-opa-set>`
- ``0xd3``: :ref:`bitwise operation: bitop <vp1-opa-bitop>`
- ``0xdf``: the canonical address nop opcode

.. todo:: complete the list


Instructions
============


.. _vp1-opa-set:

Set low/high bits: setlo, sethi
-------------------------------

Sets low or high 16 bits of a register to an immediate value.  The other half
is unaffected.

Instructions:
    =========== ================= ========
    Instruction Operands          Opcode
    =========== ================= ========
    ``setlo``   ``$a[DST] IMM16`` ``0xcc``
    ``sethi``   ``$a[DST] IMM16`` ``0xcd``
    =========== ================= ========
Operation:
    ::

        if op == 'setlo':
            $a[DST] = ($a[DST] & 0xffff0000) | IMM16
        else:
            $a[DST] = ($a[DST] & 0xffff) | IMM16 << 16


.. _vp1-opa-add:

Addition: add
-------------

Does what it says on the tin.  The second source comes from a mangled register
index.  The long address flags are set.

Instructions:
    =========== ========================================= ========
    Instruction Operands                                  Opcode
    =========== ========================================= ========
    ``add``     ``[$c[CDST]] $a[DST] $a[SRC1] $a[SRC2S]`` ``0xcb``
    =========== ========================================= ========
Operation:
    ::

        res = $a[SRC1] + $a[SRC2S]

        $a[DST] = res

        cres = 0
        if res & 1 << 31:
            cres |= 1
        if res == 0:
            cres |= 2
        if CDST < 4:
            $c[CDST].address.long = cres


.. _vp1-opa-bitop:

Bit operations: bitop
---------------------

Performs an :ref:`arbitrary two-input bit operation <bitop>` on two registers,
selected by ``SRC1`` and ``SRC2``.  The long address flags are set.

Instructions:
    =========== ============================================== =========
    Instruction Operands                                       Opcode
    =========== ============================================== =========
    ``bitop``   ``BITOP [$c[CDST]] $a[DST] $a[SRC1] $a[SRC2]``  ``0xd3``
    =========== ============================================== =========
Operation:
    ::

        res = bitop(BITOP, $a[SRC1], $a[SRC2]) & 0xffffffff

        $a[DST] = res

        cres = 0
        if res & 1 << 31:
            cres |= 1
        if res == 0:
            cres |= 2
        if CDST < 4:
            $c[CDST].address.long = cres


.. _vp1-opa-aadd:

Address addition: aadd
----------------------

Adds the contents of a register to the ``addr`` field of another register.
Short address flag is set.

Instructions:
    =========== ======================================= ========
    Instruction Operands                                Opcode
    =========== ======================================= ========
    ``aadd``    ``[$c[CDST]] $a[DST] $a[SRC2S]``        ``0xca``
    =========== ======================================= ========
Operation:
    ::

        $a[DST].addr += $a[SRC2S]

        if CDST < 4:
            $c[CDST].address.short = $a[DST].addr >= $a[DST].limit
