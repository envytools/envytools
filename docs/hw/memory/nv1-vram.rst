.. _nv1-vram:

============================
NV1 VRAM structure and usage
============================

.. contents::


Introduction
============

NV1 cards can have 1MB, 2MB, or 4MB of memory.  To learn the size of VRAM,
read the :obj:`PFB.VRAM_CONFIG register <nv1-pfb-vram-config>`.  The memory
is handled by the :ref:`PFB unit <nv1-pfb>`.  It is used for several purposes.
While the main function of VRAM is storage of pixel data to display, it is
also used to contain several control structures for various units of the card.

VRAM can be accessed by the following:

- the host, through :obj:`FB map area <nv1-fb>`
- :ref:`PFB <nv1-pfb>`/:ref:`PDAC <nv1-pdac>` scanout hardware (at any
  address, as selected by :obj:`nv1-pfb-start`).
- :ref:`PGRAPH rendering <nv1-pgraph-rop>`, as managed by PFB
- :ref:`PRM <nv1-prm>`, for:

  - the VGA memory area, covering addresses ``0x00000:0x40000``, which
    is in turn accessed by:

    - host access through PRMFB to the VGA memory area
    - reading VGA memory area for VGA rendering

  - writing VGA shadow scanout framebuffer for VGA rendering - starting
    at address ``0x40000``
  - writing ALOG entries - at addresses ``0xe0000:0xf0000``
  - VGA and DOS audio registers backing storage - at addresses
    ``0xf0000:0xf0600``

- :ref:`PAUDIO <nv1-paudio>` for sound data storage - at any address,
  as selected by the audio object descriptor.
- :ref:`PFIFO <nv1-pfifo>` for password storage - at addresses
  ``0x18000:0x18800`` (???)
- the RAMIN area [see below]

.. todo:: wtf is the password storage thing, and why is it located at
   an inconvenient and unmovable place?

Overall, VRAM addressing is very inflexible.

VRAM is mapped straight to MMIO area:

.. space:: 8 nv1-fb 0x1000000 VRAM access area

   Address range mapped straight to VRAM. Can be accessed in
   arbitrarily-sized units.


.. _nv1-fb:

The PGRAPH framebuffer
======================

There are two major modes to choose from for PGRAPH:

- Single buffer mode.  All of VRAM is treated as one big framebuffer.
- Double buffer mode.  VRAM is split into two equal-sized halves, buffer 0
  and buffer 1.

This choice also determines how :ref:`RAMIN <nv1-ramin>` is addressed, and
thus cannot be easily changed once it is in use.

The framebuffer also has configurable width (which can only be 576, 640,
800, 1024, 1152, 1280, 1600, or 1856 pixels) and pixel size (which can be
1, 2, or 4 bytes).  The lines are tightly packed, and the whole thing always
starts at address 0 of VRAM (or half of VRAM, for double buffer mode).
Thus, address of a pixel can be calculated as follows::

    def pixel_address(buf, x, y):
        width = [576, 640, 800, 1024, 1152, 1280, 1600, 1856][PFB.CONFIG.CANVAS_WIDTH]
        bytes = [1, 1, 2, 4][PFB.CONFIG.BPP]
        addr = (x & 0xfff) * bytes + (y & 0xfff) * width * bytes
        vram_size = [0x100000, 0x200000, 0x400000][PFB.VRAM_CONFIG.VRAM_SIZE]
        if PFB.CONFIG.DOUBLE_BUFFER:
            addr %= vram_size // 2
            addr += (vram_size // 2) * buf
        else:
            addr %= vram_size
        return addr

.. todo:: verify you cannot go between the two buffers by overflowing Y

All of that configuration is stored in :obj:`nv1-pfb-config` register.

.. note:: No verification is done on X and Y coordinates received from PGRAPH
  - X coordinates larger than framebuffer width will silently overflow into
  the next line(s), Y coordinates too large to fill into the buffer will wrap
  to the beginning.  To avoid that, as well as hitting other VRAM areas,
  PGRAPH canvas clipping registers should be set properly.

.. note:: It's impossible to have PGRAPH rendering and PFB/PDAC scanout use
  different bpp, since they share the bpp configuration register.  Having PGRAPH
  and PFB/PDAC use different resolution is possible, but not particularly useful
  if the rendered data is supposed to ever be displayed.


.. _nv1-ramin:

RAMIN
=====

RAMIN (aka instance memory) is a special area of VRAM, used to store various
control structures.  It uses different addressing than other parts of VRAM -
it effectively grows from the end of VRAM, or from the end of both halves of
VRAM in double buffer mode (in an interleaved fashion).  There is no hardware
register that stores the bounduary between "normal" VRAM and RAMIN - the areas
as understood by the hardware actually overlap and it's the driver's
responsibility to make sure the same chunk of VRAM isn't used as both
framebuffer and RAMIN.  RAMIN addressing covers the last 1MB of VRAM (or last
0.5MB of each buffer in double buffer mode).  RAMIN addresses correspond to
VRAM addresses as follows::

    def ramin_to_vram(addr):
        vram_size = [0x100000, 0x200000, 0x400000][PFB.VRAM_CONFIG.VRAM_SIZE]
        # In single buffer mode, just flip all bits of address, except the low 2
        # - this effectively means that RAMIN is split into 32-bit words, which
        # are stored starting at the end of VRAM, in reverse.
        addr ^= ~4
        if PFB.CONFIG.DOUBLE_BUFFER:
            # In double buffer mode, additionally switch between the two VRAM
            # halves every 0x100 bytes, starting from buffer 1.
            buf = (addr >> 8) & 1)
            addr = (addr & 0xff) | (addr >> 1 & ~0xff)
            addr %= vram_size // 2
            addr += (vram_size // 2) * buf
        else:
            addr %= vram_size
        return addr

RAMIN is split into several subareas:

- RAMHT - PFIFO Hash Table, used by PFIFO to store PGRAPH objects and their
  handles [see :ref:`nv1-pfifo-ramht`]
- RAMRO - PFIFO RunOut area, used by PFIFO to send naughty FIFO accesses to
  [see :ref:`fifo-ramro`]
- RAMFC - PFIFO Context, used by PFIFO to store context for currently
  inactive channels [see :ref:`nv1-pfifo-ramfc`]
- RAMAU - unknown 0xc00-byte long area, used by PAUDIO.
- UNK2 - unknown 0x400-byte long area.
- RAMIN proper - PDMA and PAUDIO INstance memory, used to store
  :ref:`DMA objects <nv1-dmaobj>` and audio objects.

.. todo:: figure out what RAMAU nad UNK2 are for

Of the above areas, the first 5 have fixed address and size, selected from
4 possible layout options by software.  DMA objects, however, can be located
anywhere in RAMIN - including space taken up by one of the other areas, but
that's not a particularly good idea. For the fixed areas, the layout is
selected by PRAM.CONFIG register:

.. space:: 8 nv1-pram 0x1000 RAMIN layout control
   0x200 CONFIG nv1-pram-config

.. reg:: 32 nv1-pram-config selects RAMIN fixed area layout and size

   Selects RAMIN fixed areas layout, one of:

   - 0: 0x1000-byte RAMHT, 0x800-byte RAMRO and RAMFC
   - 1: 0x2000-byte RAMHT, 0x1000-byte RAMRO and RAMFC
   - 2: 0x4000-byte RAMHT, 0x2000-byte RAMRO and RAMFC, *buggy*
   - 3: 0x8000-byte RAMHT, 0x4000-byte RAMRO and RAMFC

The addresses of fixed RAMIN areas for various configurations are:

====== ======= ======= ======= =======
CONFIG       0       1       2       3
====== ======= ======= ======= =======
RAMHT  0x00000 0x00000 0x00000 0x00000
RAMRO  0x01000 0x02000 0x02000 0x08000
RAMFC  0x01800 0x03000 0x06000 0x0c000
RAMAU  0x02000 0x04000 0x08000 0x10000
UNK2   0x02c00 0x04c00 0x08c00 0x10c00
[end]  0x03000 0x05000 0x09000 0x11000
====== ======= ======= ======= =======

Due to a hardware bug, RAMFC location conflicts with RAMHT for CONFIG=2,
effectively making it unusable.


RAMIN access areas
==================

The MMIO ranges that are mapped to RAMIN areas are:

.. space:: 8 nv1-pramht 0x8000 RAMHT access

   Mapped to RAMHT area

.. space:: 8 nv1-pramfc 0x4000 RAMFC access

   Mapped to RAMFC area

.. space:: 8 nv1-pramro 0x4000 RAMRO access

   Mapped to RAMRO area

.. space:: 8 nv1-pramau 0x1000 RAMAU access

   Mapped to RAMAU area

.. space:: 8 nv1-pramunk2 0x1000 UNK2 access

   Mapped to UNK2 area

.. space:: 8 nv1-pramin 0x100000 RAMIN access

   Mapped to RAMIN area

If any of the above MMIO areas happens to be larger than the underlying VRAM
area it is mapped to, higher addresses will wrap over to the beginning of
that area, except RAMAU, where higher addresses will go to UNK2.
