======================
Notational conventions
======================

.. contents::


Introduction
============

Semantics of many operations are described in pseudocode.  Here are some often
used primitives.


.. _bitop:

Bit operations
==============

In many places, the GPUs allow specifying arbitrary X-input boolean or bitwise
operations, where X is 2, 3, or 4.  They are described by a ``2**X``-bit mask
selecting the bit combinations for which the output should be true.  For
example, 2-input operation 0x4 (0b0100) is ``~v1 & v2``: only bit 2 (``0b10``)
is set, so the only input combination (0, 1) results in a true output.
Likewise, 3-input operation 0xaa (0b10101010) is simply a passthrough of first
input: the bits set in the mask are 1, 3, 5, 7 (``0b001, 0b011, 0b101,
0b111``), which corresponds exactly to the input combinations which have the
first input equal to 1.

The exact semantics of such operations are::

    def bitop(op, *inputs):
        # first, construct mask bit index from the inputs
        bitidx = 0
        for idx, input in enumerate(inputs):
            if input:
                bitidx |= 1 << idx
        # second, the result is the given bit of the mask
        return op >> bitidx & 1

As further example, the 2-input operations on ``a``, ``b`` are:

- ``0x0``: always 0
- ``0x1``: ``~a & ~b``
- ``0x2``: ``a & ~b``
- ``0x3``: ``~b``
- ``0x4``: ``~a & b``
- ``0x5``: ``~a``
- ``0x6``: ``a ^ b``
- ``0x7``: ``~a | ~b``
- ``0x8``: ``a & b``
- ``0x9``: ``~a ^ b``
- ``0xa``: ``a``
- ``0xb``: ``a | ~b``
- ``0xc``: ``b``
- ``0xd``: ``~a | b``
- ``0xe``: ``a | b``
- ``0xf``: always 1

For further enlightenment, you can search for GDI raster operations, which
correspond to 3-input bit operations.


.. _sext:

Sign extension
==============

An often used primitive is sign extension from a given bit.  This operation
is known as ``sext`` after xtensa instruction of the same name and is formally
defined as follows::

    def sext(val, bit):
        # mask with all bits up from #bit set
        mask = -1 << bit
        if val & 1 << bit:
            # sign bit set, negative, set all upper bits
            return val | mask
        else:
            # sign bit not set, positive, clear all upper bits
            return val & ~mask


.. _extr:

Bitfield extraction
===================

Another often used primitive is bitfield extraction.  Extracting an unsigned
bitfield of length ``l`` starting at position ``s`` in ``val`` is denoted
by ``extr(val, s, l)``, and signed one by ``extrs(val, s, l)``::

    def extr(val, s, l):
        return val >> s & ((1 << l) - 1)

    def extrs(val, s, l):
        return sext(extrs(val, s, l), l - 1)
