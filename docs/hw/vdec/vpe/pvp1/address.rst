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

- scalar: like horizontal, but 4 bytes::

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

- bits 3-13: ``UIMM``: unsigned 13-bit immediate.

.. todo:: list me


Opcodes
-------

The opcode range assigned to the address unit is ``0xc0-0xdf``.  The opcodes
are:

- ``0xc0``: :ref:`load vector horizontal and add: ldavh <vp1-opa-lda>`
- ``0xc1``: :ref:`load vector vertical and add: ldavv <vp1-opa-lda>`
- ``0xc2``: :ref:`load scalar and add: ldas <vp1-opa-lda>`
- ``0xc3``: ???
- ``0xc4``: :ref:`store vector horizontal and add: stavh <vp1-opa-sta>`
- ``0xc5``: :ref:`store vector vertical and add: stavv <vp1-opa-sta>`
- ``0xc6``: :ref:`store scalar and add: stas <vp1-opa-sta>`
- ``0xc7``: ???
- ``0xc8``: ???
- ``0xc9``: ???
- ``0xca``: :ref:`address addition: aadd <vp1-opa-aadd>`
- ``0xcb``: :ref:`addition: add <vp1-opa-add>`
- ``0xcc``: :ref:`set low bits: setlo <vp1-opa-set>`
- ``0xcd``: :ref:`set high bits: sethi <vp1-opa-set>`
- ``0xce``: ???
- ``0xcf``: ???
- ``0xd0``: :ref:`load vector horizontal and add: ldavh <vp1-opa-lda>`
- ``0xd1``: :ref:`load vector vertical and add: ldavv <vp1-opa-lda>`
- ``0xd2``: :ref:`load scalar and add: ldas <vp1-opa-lda>`
- ``0xd3``: :ref:`bitwise operation: bitop <vp1-opa-bitop>`
- ``0xd4``: :ref:`store vector horizontal and add: stavh <vp1-opa-sta>`
- ``0xd5``: :ref:`store vector vertical and add: stavv <vp1-opa-sta>`
- ``0xd6``: :ref:`store scalar and add: stas <vp1-opa-sta>`
- ``0xd7``: ???
- ``0xd8``: :ref:`load vector horizontal: ldvh <vp1-opa-ld>`
- ``0xd9``: :ref:`load vector vertical: ldvv <vp1-opa-ld>`
- ``0xda``: :ref:`load scalar: lds <vp1-opa-ld>`
- ``0xdb``: ???
- ``0xdc``: :ref:`store vector horizontal: stvh <vp1-opa-st>`
- ``0xdd``: :ref:`store vector vertical: stvv <vp1-opa-st>`
- ``0xde``: :ref:`store scalar: sts <vp1-opa-st>`
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


.. _vp1-opa-ld:

Load: ldvh, ldvv, lds
---------------------

Loads from the given address ORed with an unsigned 11-bit immediate.  ``ldvh``
is a horizontal vector load, ``ldvv`` is a vertical vector load, and ``lds`` is
a scalar load.  Curiously, while register is ORed with the immdiate to form the
address, they are *added* to make ``$c`` output.

Instructions:
    =========== ========================================= ========
    Instruction Operands                                  Opcode
    =========== ========================================= ========
    ``ldvh``   ``$v[DST] [$c[CDST]] $a[SRC1] UIMM``      ``0xd8``
    ``ldvv``   ``$v[DST] [$c[CDST]] $a[SRC1] UIMM``      ``0xd9``
    ``lds``    ``$r[DST] [$c[CDST]] $a[SRC1] UIMM``      ``0xda``
    =========== ========================================= ========
Operation:
    ::

        if op == 'ldvh':
            addr = addresses_horizontal($a[SRC1].addr | UIMM, $a[SRC1].stride)
            for idx in range(16):
                $v[DST][idx] = DS[addr[idx]]
        elif op == 'ldvv':
            addr = addresses_vertical($a[SRC1].addr | UIMM, $a[SRC1].stride)
            for idx in range(16):
                $v[DST][idx] = DS[addr[idx]]
        elif op == 'lds':
            addr = addresses_scalar($a[SRC1].addr | UIMM, $a[SRC1].stride)
            for idx in range(4):
                $r[DST][idx] = DS[addr[idx]]

        if CDST < 4:
            $c[CDST].address.short = (($a[SRC1].addr + UIMM) & 0xffff) >= $a[SRC1].limit


.. _vp1-opa-lda:

Load and add: ldavh, ldavv, ldas
--------------------------------

Loads from the given address, then post-increments the address by the contents
of a register (like :ref:`the aadd instruction <vp1-opa-aadd>`) or an immediate.
``ldavh`` is a horizontal vector load, ``ldavv`` is a vertical vector load, and
``ldas`` is a scalar load.

Instructions:
    =========== ========================================= ========
    Instruction Operands                                  Opcode
    =========== ========================================= ========
    ``ldavh``   ``$v[DST] [$c[CDST]] $a[SRC1] $a[SRC2S]`` ``0xc0``
    ``ldavv``   ``$v[DST] [$c[CDST]] $a[SRC1] $a[SRC2S]`` ``0xc1``
    ``ldas``    ``$r[DST] [$c[CDST]] $a[SRC1] $a[SRC2S]`` ``0xc2``
    ``ldavh``   ``$v[DST] [$c[CDST]] $a[SRC1] IMM``       ``0xd0``
    ``ldavv``   ``$v[DST] [$c[CDST]] $a[SRC1] IMM``       ``0xd1``
    ``ldas``    ``$r[DST] [$c[CDST]] $a[SRC1] IMM``       ``0xd2``
    =========== ========================================= ========
Operation:
    ::

        if op == 'ldavh':
            addr = addresses_horizontal($a[SRC1].addr, $a[SRC1].stride)
            for idx in range(16):
                $v[DST][idx] = DS[addr[idx]]
        elif op == 'ldavv':
            addr = addresses_vertical($a[SRC1].addr, $a[SRC1].stride)
            for idx in range(16):
                $v[DST][idx] = DS[addr[idx]]
        elif op == 'ldas':
            addr = addresses_scalar($a[SRC1].addr, $a[SRC1].stride)
            for idx in range(4):
                $r[DST][idx] = DS[addr[idx]]

        if IMM is None:
            $a[SRC1].addr += $a[SRC2S]
        else:
            $a[SRC1].addr += IMM

        if CDST < 4:
            $c[CDST].address.short = $a[SRC1].addr >= $a[SRC1].limit


.. _vp1-opa-st:

Store: stvh, stvv, sts
----------------------

Like corresponding :ref:`ld* instructions <vp1-opa-ld>`, but store instead of
load.  ``SRC1`` and ``DST`` fields are exchanged.

Instructions:
    =========== ========================================= ========
    Instruction Operands                                  Opcode
    =========== ========================================= ========
    ``stvh``   ``$v[SRC1] [$c[CDST]] $a[DST] UIMM``       ``0xdc``
    ``stvv``   ``$v[SRC1] [$c[CDST]] $a[DST] UIMM``       ``0xdd``
    ``sts``    ``$r[SRC1] [$c[CDST]] $a[DST] UIMM``       ``0xde``
    =========== ========================================= ========
Operation:
    ::

        if op == 'stvh':
            addr = addresses_horizontal($a[DST].addr | UIMM, $a[DST].stride)
            for idx in range(16):
                DS[addr[idx]] = $v[SRC1][idx]
        elif op == 'stvv':
            addr = addresses_vertical($a[DST].addr | UIMM, $a[DST].stride)
            for idx in range(16):
                DS[addr[idx]] = $v[SRC1][idx]
        elif op == 'sts':
            addr = addresses_scalar($a[DST].addr | UIMM, $a[DST].stride)
            for idx in range(4):
                DS[addr[idx]] = $r[SRC1][idx]

        if CDST < 4:
            $c[CDST].address.short = (($a[DST].addr + UIMM) & 0xffff) >= $a[DST].limit


.. _vp1-opa-sta:

Store and add: stavh, stavv, stas
---------------------------------

Like corresponding :ref:`lda* instructions <vp1-opa-lda>`, but store instead of
load.  ``SRC1`` and ``DST`` fields are exchanged.

Instructions:
    =========== ========================================= ========
    Instruction Operands                                  Opcode
    =========== ========================================= ========
    ``stavh``   ``$v[SRC1] [$c[CDST]] $a[DST] $a[SRC2S]`` ``0xc4``
    ``stavv``   ``$v[SRC1] [$c[CDST]] $a[DST] $a[SRC2S]`` ``0xc5``
    ``stas``    ``$r[SRC1] [$c[CDST]] $a[DST] $a[SRC2S]`` ``0xc6``
    ``stavh``   ``$v[SRC1] [$c[CDST]] $a[DST] IMM``       ``0xd4``
    ``stavv``   ``$v[SRC1] [$c[CDST]] $a[DST] IMM``       ``0xd5``
    ``stas``    ``$r[SRC1] [$c[CDST]] $a[DST] IMM``       ``0xd6``
    =========== ========================================= ========
Operation:
    ::

        if op == 'stavh':
            addr = addresses_horizontal($a[DST].addr, $a[DST].stride)
            for idx in range(16):
                DS[addr[idx]] = $v[SRC1][idx]
        elif op == 'stavv':
            addr = addresses_vertical($a[DST].addr, $a[DST].stride)
            for idx in range(16):
                DS[addr[idx]] = $v[SRC1][idx]
        elif op == 'stas':
            addr = addresses_scalar($a[DST].addr, $a[DST].stride)
            for idx in range(4):
                DS[addr[idx]] = $r[SRC1][idx]

        if IMM is None:
            $a[DST].addr += $a[SRC2S]
        else:
            $a[DST].addr += IMM

        if CDST < 4:
            $c[CDST].address.short = $a[DST].addr >= $a[DST].limit
