.. _nv01-clock:

================
NV01:NV40 clocks
================

.. contents::

.. todo:: write me


Introduction
============

NV01:NV40 chipsets have the following clocks:

- core clock [NVPLL]: used to clock the graphics engine - on NV04+ cards only
- memory clock [MPLL]: used to clock memory and, before NV04, the graphics
  engine - not present on IGPs
- video clock 1 [VPLL]: used to drive first/only video output
- video clock 2 [VPLL2]: used to drive second video output - only on NV11+
  cards

.. todo:: DLL on NV03
.. todo:: NV01???

These clocks are set in PDAC [NV01] or PRAMDAC [NV03 and up] areas.

.. todo:: write me


.. _nv01-clock-nvclk:

NVCLK: core clock
=================

.. todo:: write me


.. _nv01-clock-mclk:

MCLK: memory clock
==================

.. todo:: write me
