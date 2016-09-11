.. _pcrtc:

=====================
PCRTC: scanout engine
=====================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


MMIO registers
==============

.. todo:: write me

.. space:: 8 pcrtc 0x1000 CRTC controls

   .. todo:: write me

.. space:: 8 prmcio 0x1000 VGA CRTC & attribute controller registers

   0x3d0 RMA_ACCESS nv3-io-rma-access
   0x3d4 CRTC_INDEX nv3-io-crtc-index
   0x3d5 CRTC_DATA nv3-io-crtc-data

.. _pcrtc-intr:

Interrupts
==========

.. todo:: write me


.. _pcrtc-blank:

Vertical/horizontal blanking signals
====================================

.. todo:: write me

RMA access port
===============

.. reg:: 32 nv3-io-rma-access RMA access port

This is a 32-bit port that writes into the RMA address specified by CRTC register 0x38.

CRTC registers
==============

.. reg:: 8 nv3-io-crtc-index CRTC index

.. reg:: 8 nv3-io-crtc-data CRTC data

.. space:: 8 nv3-crtc-ext-regs 0x100 Extended VGA registers

   0x19 REPAINT_0 nv3-crtc-ext-rpc0
   0x1a REPAINT_1 nv3-crtc-ext-rpc1
   0x1d WRITE_BANK nv3-crtc-ext-write-bank
   0x1e READ_BANK nv3-crtc-ext-read-bank
   0x25 EXTENDED_VERT nv3-crtc-ext-extvert
   0x28 PIXEL_FMT nv3-crtc-ext-pixel-fmt
   0x2d EXTENDED_HORZ nv3-crtc-ext-exthorz
   0x38 RMA_MODE nv3-crtc-ext-rmamode
   0x3e I2C_READ nv3-crtc-ext-i2c-read
   0x3f I2C_WRITE nv3-crtc-ext-i2c-write

.. reg:: 8 nv3-crtc-ext-rpc0 Extended Start Address and Row Offset

Bits 4-0 are Start Address bits 16-20. Bits 7-5 are Row Offset bits 3-5.

.. reg:: 8 nv3-crtc-ext-rpc1 Repaint 1

Bit 2 shifts the row offset either left or right by 1 bit, I forget which way.

.. reg:: 8 nv3-crtc-ext-write-bank Write bank

Write bank for real mode access of the VGA framebuffer in 32k units.

.. reg:: 8 nv3-crtc-ext-read-bank Read bank

Read bank for real mode access of the VGA framebuffer in 32k units.

.. reg:: 8 nv3-crtc-ext-extvert Extended Vertical Bits

Bit 0 is Vertical Total bit 10.
Bit 1 is Vertical Display End bit 10.
Bit 2 is Vertical Blank Start bit 10.
Bit 3 is Vertical Sync Start bit 10.
Bit 4 is Horizontal Total bit 8.

.. reg:: 8 nv3-crtc-ext-pixel-fmt Pixel Format

Bits 1-0 are Pixel Format. 0 is VGA, 1 is 8bpp, 2 is 16bpp, and 3 is 32bpp.

.. reg:: 8 nv3-crtc-ext-exthorz Extended Horizontal Bits

Bit 0 is Horizontal Display End bit 8.

.. reg:: 8 nv3-crtc-ext-rmamode RMA mode register

Bit 0 is enable when high, bits 3-1 are which RMA register to write to.

.. reg:: 8 nv3-crtc-ext-i2c-read I2C read register

Bit 3 is SDA, Bit 2 is SCL.

.. reg:: 8 nv3-crtc-ext-i2c-write I2C write register

Bit 5 is SCL, Bit 4 is SDA.
