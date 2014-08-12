.. _vp1-address:

============
Address unit
============

.. contents::


Introduction
============

The address unit is one of the four execution units of VP1.  It transfers
data between that data store and registers, controls the :ref:`DMA unit
<vp1-dma>`, and performs address calculations.


.. _vp1-data:

The data store
==============

The data store is the working memory of VP1, 8kB in size.  Data can be
transferred between the data store and ``$r``/``$v`` registers using load/store
instructions, or between the data store and main memory using :ref:`the DMA
engine <vp1-dma>`.  It's often treated as two-dimensional, with row stride
selectable between ``0x10``, ``0x20``, ``0x40``, and ``0x80`` bytes: there are
"load vertical" instructions which gather consecutive bytes vertically rather
than horizontally.

Because of its 2D capabilities, the data store is internally organized into 16
independently addressable 16-bit wide banks of 256 cells each, and the memory
addresses are carefully spread between the banks so that both horizontal and
vertical loads from any address will require at most one access to every bank.
The bank assignments differ between the supported strides, so row stride is
basically a part of the address, and an area of memory always has to be
accessed with the same stride (unless you don't care about its previous
contents).  Specifially, the translation of (address, stride) pair into (bank,
cell index, high/low byte) is as follows::

    def address_xlat(addr, stride):
        bank = addr & 0xf
        hilo = addr >> 4 & 1
        cell = addr >> 5 & 0xff
        if stride == 0:
            # 0x10 bytes
            bank += (addr >> 5) & 7
        elif stride == 1:
            # 0x20 bytes
            bank += addr >> 5
        elif stride == 0x40:
            # 0x40 bytes
            bank += addr >> 6
        elif stride == 0x80:
            # 0x80 bytes
            bank += addr >> 7
        bank &= 0xf
        return bank, cell, hilo

In pseudocode, data store bytes are denoted by ``DS[bank, cell, hilo]``.

In case of vertical access with 0x10 bytes stride, all 16 bits of 8 banks will
be used by a 16-byte access.  In all other cases, 8 bits of all 16 banks will
be used for such access.  DMA transfers can make use of the full 256-bit width
of the data store, by transmitting 0x20 consecutive bytes at a time.

The data store can be accessed by load/store instructions in one of four ways:

- horizontal: 16 consecutive naturally aligned addresses are used::

    def addresses_horizontal(addr, stride):
        addr &= 0x1ff0
        return [address_xlat(addr | idx, stride) for idx in range(16)]

- vertical: 16 addresses separated by stride bytes are used, also naturally
  aligned::

    def addresses_vertical(addr, stride):
        addr &= 0x1fff
        # clear the bits used for y coord
        addr &= ~(0xf << (4 + stride))
        return [address_xlat(addr | idx << (4 + stride)) for idx in range(16)]

- horizontal short (for scalar accesses): like horizontal, but 4 bytes::

    def addresses_horizontal_short(addr, stride):
        addr &= 0x1ffc
        return [address_xlat(addr | idx, stride) for idx in range(4)]

- raw: the raw data store coordinates are provided directly


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
    ``bitop``   ``BITOP [$c[CDST]] $a[DST] $a[SRC1] $a[SRC2]`` ``0xd3``
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
