.. _pramdac:

==============================
PRAMDAC: display output engine
==============================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


MMIO registers
==============

.. space:: 8 pramdac 0x1000 RAMDAC, video overlay, cursor, and PLL control
   0x500 NVPLL nv4-pramdac-nvpll
   0x504 MPLL nv3-pramdac-mpll
   0x508 VPLL nv3-pramdac-vpll
   
   .. todo:: complete me

.. space:: 8 prmdio 0x1000 VGA DAC registers

   .. todo:: write me


.. todo:: complete me

The bit layout for all NV4 PLLs is that bits 18-16 are P, bits 15-8 are N, and bits 7-0 are M.

The clocks are calculated as such: (Crystal frequency * N) / (1 << P) / M.
