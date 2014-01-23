.. _nv01-prm:

==========================
PRM: NV01 Real Mode engine
==========================

.. contents::


Introduction
============

.. todo:: figure out what the fuck this engine does


.. _nv01-prm-mmio:

The MMIO registers
==================

.. todo:: write me


.. _nv01-prmio-mmio:

The IO ports
============

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


.. _nv01-prmfb-mmio:

The VGA memory window
=====================

.. todo:: write me


.. _nv01-prm-intr:

Interrupts
==========

.. todo:: write me
