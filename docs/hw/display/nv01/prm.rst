.. _nv01-prm:

==========================
PRM: NV01 Real Mode engine
==========================

.. contents::


Introduction
============

.. todo:: figure out what the fuck this engine does


The MMIO registers
==================

.. space:: 8 nv01-prm 0x8000 VGA and ISA sound emulation
   0x0080 UNK0080 nv01-prm-unk0080
   0x0100 INTR nv01-prm-intr
   0x0140 INTR_ENABLE nv01-prm-intr-enable
   0x0200 VGA_CONFIG nv01-prm-vga-config
   0x0400 MPU_STATE nv01-prm-mpu-state
   0x1f00 ALOG_CONFIG nv01-prm-alog-config
   0x1f10 ALOG_POS nv01-prm-alog-pos
   0x1f20 ALOG_IGNORE_0 nv01-prm-alog-ignore-0
   0x1f24 ALOG_IGNORE_1 nv01-prm-alog-ignore-1

   .. todo:: write me


The IO ports
============

.. space:: 8 nv01-prmio 0x1000 VGA and ISA sound compat IO port access
   0x201 GAME_PORT nv01-io-game-port
   0x3c6 PAL_MASK nv01-io-pal-mask
   0x3c7 PAL_READ nv01-io-pal-read
   0x3c8 PAL_WRITE nv01-io-pal-write
   0x3c9 PAL_DATA nv01-io-pal-data

   .. todo:: write me


VGA configuration
=================

.. reg:: 32 nv01-prm-vga-config VGA emulation config

   - bit 0: TEXT_RENDER_ENABLE, if set to 1 enables the text rendering process
   - bit 4: PAL_WIDTH, selects palette data port mode:

     - 0: VGA, data is converted to/from 6-bit format
     - 1: FULL, data is passed through unmodified


.. _nv01-vga-regs:

VGA registers
=============

.. todo:: write me

SR 0x02: PLANE_WRITE_MASK
  - bits 0-3: enable writes to corresponding planes

SR 0x03: FONT_SELECT
  - bits 0-1: bits 0-1 of font A select
  - bits 2-3: bits 0-1 of font B select
  - bit 4: bit 2 of font A select
  - bit 5: bit 2 of font B select

SR 0x04: MEMORY_CONTROL
  - bit 1: 1 if >64kiB memory on card [unused by hw]
  - bit 2:
    - 0: odd/even
    - 1: planar or chained mode
  - bit 3:
    - 0: planar or odd/even mode
    - 1: chained mode

Palette registers
-----------------

VGA palette registers provided by PRM are simply aliases of DAC registers,
except data port that also performs conversion.

.. reg:: 8 nv01-io-pal-mask Palette index mask

   All accesses forwarded to :obj:`nv01-pdac-pal-mask`.

.. reg:: 8 nv01-io-pal-read Palette read index

   All accesses forwarded to :obj:`nv01-pdac-pal-read`.

.. reg:: 8 nv01-io-pal-write Palette write index

   All accesses forwarded to :obj:`nv01-pdac-pal-write`.

.. reg:: 8 nv01-io-pal-data Palette data

   All accesses forwarded to :obj:`nv01-pdac-pal-data`. If :obj:`nv01-prm-vga-config`.DAC_WIDTH
   field is set to FULL, data is passed through unchanged. Otherwise, data is
   shifted left by 2 bits on writes, and 2 bits right on reads.


The VGA memory window
=====================

.. space:: 8 nv01-prmfb 0x20000 VGA memory window access

   .. todo:: write me


.. _nv01-prm-intr:

Interrupts
==========

.. todo:: write me


Game port
=========

PRM provides ISA-style game port access:

.. reg:: 8 nv01-io-game-port Game port

   All accesses forwarded to :obj:`nv01-pdac-game-port`.
