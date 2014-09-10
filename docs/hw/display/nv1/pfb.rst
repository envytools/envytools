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


.. _nv1-pfb-mmio-vram-size:

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


.. _nv1-pfb-mmio-config:

Framebuffer configuration
=========================

.. todo:: write me


Scanout hardware
================

.. todo:: write me

.. reg:: 32 nv1-pfb-power-sync Power state and sync pulse config

   .. todo:: write me

.. reg:: 32 nv1-pfb-config Display and framebuffer configuration

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
