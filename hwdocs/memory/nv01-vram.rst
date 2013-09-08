.. _nv01-vram:

=============================
NV01 VRAM structure and usage
=============================

.. contents::


Introduction
============

NV01 cards can have 1MB, 2MB, or 4MB of memory. The memory is handled by the
:ref:`PFB unit <nv01-pfb>`. It is used for several purposes. While
the main function of VRAM is storage of pixel data to display, it is also used
to contain several control structures for various units of the card.

The area used for control structures is called RAMIN, uses different
addressing than the framebuffer, and is located at the end of VRAM, while
framebuffer is located at the beginning. There is no hardware register that
stores the bounduary between framebuffer and RAMIN - the areas as understood
by the hardware actually overlap and it's the driver's responsibility to make
sure the same chunk of VRAM isn't used as both framebuffer and RAMIN. The
framebuffer area covers all of VRAM, while RAMIN covers the last 1MB of VRAM.

The RAMIN is further split into several subareas according to one of several
fixed schemes. The RAMIN layout is set up in a unit known as PRAM, located
at MMIO range 0x602000:0x603000. PRAM is unaffected by PMC.ENABLE bits.

To learn the size of VRAM, read the `PFB.VRAM_SIZE register <nv01-pfb-mmio-vram-size>`.


.. _nv01-pram-mmio:

MMIO registers
==============

There is only one MMIO register in PRAM range:

======== ====== =============
Address  Name   Description
======== ====== =============
0x602200 CONFIG :ref:`Selects RAMIN fixed area layout and size <nv01-pram-mmio-config>`
======== ====== =============


.. _nv01-fb-mmio:

The framebuffer
===============

The framebuffer starts at address 0 of VRAM and continues until its end. The
framebuffer addresses directly correspond to VRAM addresses. It is accessed
by four clients:

- :ref:`PFB <nv01-pfb>`/:ref:`PDAC <nv01-pdac>` scanout hardware
- PGRAPH rendering [graph/nv01-pgraph.txt]
- the host, through FB map area [see below]
- :ref:`PRM <nv01-prm>`, for:

  - host access through PRMFB to the VGA memory area
  - reading VGA memory area for VGA rendering
  - writing VGA shadow scanout framebuffer for VGA rendering
  - writing ALOG entries
  - VGA and DOS audio registers backing storage

The host and scanout access the framebuffer simply by an address. PGRAPH,
however, accesses the framebuffer by X, Y coordinates of a pixel. For this
reason, the resolution and bpp fields of :ref:`PFB.CONFIG <nv01-pfb-mmio-config>`
have to be set up properly before rendering. Note that it's impossible to have
PGRAPH rendering and PFB/PDAC scanout use different bpp, since they share the
bpp configuration register. Having PGRAPH and PFB/PDAC use different
resolution is possible, but not particularly useful if the rendered data is
supposed to ever be displayed.

The framebuffer is mapped straight to MMIO area:

MMIO 0x1000000:0x2000000
    Address range mapped straight to the framebuffer. Can be accessed in
    arbitrarily-sized units.


.. _nv01-pram-mmio-config:

RAMIN
=====

The RAMIN contains control structures of the card. RAMIN effectively grows
from the end of VRAM - RAMIN data is reversed from VRAM data in 4-byte units.
For example, RAMIN address 0 corresponds to VRAM address 0x1ffffc [assuming
2MB card], RAMIN address 1 corresponds to VRAM address 0x1ffffd, RAMIN address
4 corresponds to 0x1ffff8, 0x1234 corresponds to 0x1fedc8. In general, RAMIN
address X corresponds to VRAM address ``vram_size + (X ^ (-4))``.

RAMIN is split into several subareas:

- RAMHT - PFIFO Hash Table, used by PFIFO to store PGRAPH objects and their
  handles [see fifo/nv01-pfifo.txt]
- RAMRO - PFIFO RunOut area, used by PFIFO to send naughty FIFO accesses to
  [see fifo/pio.txt]
- RAMFC - PFIFO Context, used by PFIFO to store context for currently
  inactive channels [see fifo/nv01-pfifo.txt]
- UNK1 - unknown 0x1000-byte long area. Or maybe 0xc00-byte - last 0x400
  bytes seem to conflict with UNK2. Related to PAUDIO.
- UNK2 - unknown 0x400-byte long area.
- RAMIN proper - PDMA INstance memory, used to store :ref:`DMA objects <nv01-dmaobj>`

.. todo:: figure out what UNK1 nad UNK2 are for

Of the above areas, the first 5 have fixed address and size, selected from
4 possible layout options by software. DMA objects, however, can be located
anywhere in RAMIN - including space taken up by one of the other areas, but
that's not a particularly good idea. For the fixed areas, the layout is
selected by PRAM.CONFIG register:

MMIO 0x602200: CONFIG
  Selects RAMIN fixed areas layout, one of:
    0: 0x1000-byte RAMHT, 0x800-byte RAMRO and RAMFC
    1: 0x2000-byte RAMHT, 0x1000-byte RAMRO and RAMFC
    2: 0x4000-byte RAMHT, 0x2000-byte RAMRO and RAMFC, *buggy*
    3: 0x8000-byte RAMHT, 0x4000-byte RAMRO and RAMFC

The addresses of fixed RAMIN areas for various configurations are:

====== ======= ======= ======= =======
CONFIG       0       1       2       3
====== ======= ======= ======= =======
RAMHT  0x00000 0x00000 0x00000 0x00000
RAMRO  0x01000 0x02000 0x02000 0x08000
RAMFC  0x01800 0x03000 0x06000 0x0c000
UNK1   0x02000 0x04000 0x08000 0x10000
UNK2   0x02c00 0x04c00 0x08c00 0x10c00
[end]  0x03000 0x05000 0x09000 0x11000
====== ======= ======= ======= =======

Due to a hardware bug, RAMFC location conflicts with RAMHT for CONFIG=2,
effectively making it unusable.


.. _nv01-pramht-mmio:
.. _nv01-pramfc-mmio:
.. _nv01-pramro-mmio:
.. _nv01-pramunk1-mmio:
.. _nv01-pramunk2-mmio:
.. _nv01-pramin-mmio:

RAMIN access areas
==================

The MMIO ranges that are mapped to VRAM areas are:

- 640000:648000 PRAMHT - mapped to RAMHT area
- 648000:64c000 PRAMFC - mapped to RAMFC area
- 650000:654000 PRAMRO - mapped to RAMRO area
- 604000:605000 ??? - mapped to UNK1 area
- 606000:607000 ??? - mapped to UNK2 area
- 700000:800000 PRAMIN - mapped to RAMIN area

If any of the above MMIO areas happens to be larger than the underlying VRAM
area it is mapped to, higher addresses will wrap over to the beginning of
that area.
