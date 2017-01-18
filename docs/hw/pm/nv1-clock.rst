.. _nv1-clock:

===============
NV1:NV40 clocks
===============

.. contents::

.. todo:: write me


Introduction
============

NV1:NV40 GPUs have the following clocks:

- core clock [NVPLL]: used to clock the graphics engine - on NV4+ cards only
- memory clock [MPLL]: used to clock memory and, before NV4, the graphics
  engine - not present on IGPs
- video clock 1 [VPLL]: used to drive first/only video output
- video clock 2 [VPLL2]: used to drive second video output - only on NV11+
  cards

.. todo:: DLL on NV3
.. todo:: NV1???

These clocks are set in PDAC [NV1] or PRAMDAC [NV3 and up] areas.

.. todo:: write me


.. _nv1-clock-nvclk:

NVCLK: core clock
=================

.. todo:: write me

.. reg:: 32 nv4-pramdac-nvpll Core PLL

   .. todo:: write me


.. _nv1-clock-mclk:

MCLK: memory clock
==================

.. todo:: write me

.. reg:: 32 nv3-pramdac-mpll Memory PLL

   .. todo:: write me


.. _nv1-clock-vclk:

VCLK: video clocks
==================

.. todo:: write me

.. reg:: 32 nv3-pramdac-vpll Video PLL

   .. todo:: write me
