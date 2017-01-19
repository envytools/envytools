.. _nv1-pfb:

==================================================
PFB: NV1 native display and VRAM controller engine
==================================================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


The MMIO registers
==================

.. space:: 8 nv1-pfb 0x1000 VRAM and video output control
   0x000 VRAM_CONFIG nv1-pfb-vram-config
   0x040 UNK040 nv1-pfb-unk040
   0x044 UNK040 nv1-pfb-unk044
   0x080 UNK080 nv1-pfb-unk080
   0x0c0 POWER_SYNC nv1-pfb-power-sync
   0x200 CONFIG nv1-pfb-config
   0x400 START nv1-pfb-start
   0x500 HOR_FRONT_PORCH nv1-pfb-hor-front-porch
   0x510 HOR_SYNC_WIDTH nv1-pfb-hor-sync-width
   0x520 HOR_BACK_PORCH nv1-pfb-hor-back-porch
   0x530 HOR_DISP_WIDTH nv1-pfb-hor-disp-width
   0x540 VER_FRONT_PORCH nv1-pfb-ver-front-porch
   0x550 VER_SYNC_WIDTH nv1-pfb-ver-sync-width
   0x560 VER_BACK_PORCH nv1-pfb-ver-back-porch
   0x570 VER_DISP_WIDTH nv1-pfb-ver-disp-width


VRAM control
============

.. todo:: write me

.. reg:: 32 nv1-pfb-vram-config VRAM configuration

   - bits 0-1: VRAM_SIZE

     VRAM size, one of:

     - 0: 1MB
     - 1: 2MB
     - 2: 4MB

   - bits 8-9: ???
   - bit 12: ???
   - bits 16-17: ???
   - bit 20: ???
   - bit 24: ???
   - bit 28: ???

   .. todo:: write me

.. reg:: 32 nv1-pfb-unk040 VRAM ???

   .. todo:: write me

.. reg:: 32 nv1-pfb-unk044 VRAM ???

   .. todo:: write me

.. reg:: 32 nv1-pfb-unk080 VRAM ???

   .. todo:: write me


Framebuffer configuration
=========================

One of PFB's tasks is managing the framebuffer that PGRAPH will draw to.
This (and a few assorted functions) is handled by the following register:

.. reg:: 32 nv1-pfb-config Display and framebuffer configuration

   - bit 0: VBLANK - read only, 1 iff the scanout hardware is currently
     in vblank.
   - bits 4-6: CANVAS_WIDTH - determines the width of canvas used by PGRAPH,
     in pixels.  One of:

     - 0: 576 pixels,
     - 1: 640 pixels,
     - 2: 800 pixels,
     - 3: 1024 pixels,
     - 4: 1152 pixels,
     - 5: 1280 pixels,
     - 6: 1600 pixels,
     - 7: 1856 pixels,

     This only affects PGRAPH, and not scanout.  However, PGRAPH is quite
     useless, unless this is set to be the same as the screen width used
     by scanout.

   - bits 8-9: BPP - determines the size of a pixel.  This affects both
     PGRAPH and scanout.  The DAC has a similiar setting in one of its
     registers, and they should be set identically, or display data will
     be mangled.  One of:

     - 0: 4bpp (not supported by PGRAPH, which will behave as if it was
       set to 8bpp),
     - 1: 8bpp,
     - 2: 16bpp,
     - 3: 32bpp.

   - bit 12: DOUBLE_BUFFER - if set, VRAM is split in two halves,
     which PGRAPH will treat as two separate framebuffers.  Also, RAMIN
     will be interleaved across the halves.  If not set, there is only
     one buffer.  This setting does not affect scanout.

   - bits 16-18: ???
   - bit 20: SCANLINE - ???
   - bits 24-26: PCLK_VCLK_RATIO - ???
   - bit 28: ???

   .. todo:: unknowns

See :ref:`nv1-fb` for details on framebuffer layout.

.. todo:: write me


Scanout hardware
================

.. todo:: write me

.. reg:: 32 nv1-pfb-power-sync Power state and sync pulse config

   .. todo:: write me

.. reg:: 32 nv1-pfb-start Display start address

   .. todo:: write me

.. reg:: 32 nv1-pfb-hor-front-porch Horizontal front porch size

   .. todo:: write me

.. reg:: 32 nv1-pfb-hor-sync-width Horizontal sync pulse size

   .. todo:: write me

.. reg:: 32 nv1-pfb-hor-back-porch Horizontal back porch size

   .. todo:: write me

.. reg:: 32 nv1-pfb-hor-disp-width Horizontal display size

   .. todo:: write me

.. reg:: 32 nv1-pfb-ver-front-porch Vertical front porch size

   .. todo:: write me

.. reg:: 32 nv1-pfb-ver-sync-width Vertical sync pulse size

   .. todo:: write me

.. reg:: 32 nv1-pfb-ver-back-porch Vertical back porch size

   .. todo:: write me

.. reg:: 32 nv1-pfb-ver-disp-width Vertical display size

   .. todo:: write me
