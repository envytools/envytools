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

.. space:: 8 nv01-prm 0x8000 VGA compatibility control

   .. todo:: write me


The IO ports
============

.. space:: 8 nv01-prmio 0x1000 VGA and ISA sound compat IO port access

   .. todo:: write me


.. _nv01-vga-regs:

VGA registers
=============

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


The VGA memory window
=====================

.. space:: 8 nv01-prmfb 0x20000 aliases VGA memory window

   .. todo:: write me


.. _nv01-prm-intr:

Interrupts
==========

.. todo:: write me
