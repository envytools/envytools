.. _nv40-clock:

================
NV40:NV50 clocks
================

.. contents::

.. todo:: write me


Introduction
============

NV40 generation cards have the following clocks:

- host clock: clocks the host interface parts, like PFIFO
- core clock [NVPLL]: clocks most of the card's logic
- unknown clock 4008
- video clocks [VPLL1,VPLL2]: used to drive the video outputs
- memory clocks [MPLL1-MPLL4]: used to clock the VRAM, there's one of them
  for each memory partition connected to the card
- unknown clock 4050 - not present on all cards
- unknown clock 4058 - not present on all cards

.. todo:: figure out where host clock comes from
.. todo:: figure out 4008/shader clock
.. todo:: figure out 4050, 4058

The clocks other than VPLLs are set through the new PCLOCK area, VPLLs are set
through PRAMDAC like on NV03.

.. todo:: write me


.. _nv40-pclock-mmio:
.. _nv40-pcontrol-mmio:

MMIO registers
==============

.. todo:: write me


.. _nv40-clock-hclk:

HCLK: host clock
================

.. todo:: write me
