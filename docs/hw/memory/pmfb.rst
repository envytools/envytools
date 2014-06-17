.. _pmfb:

===================================
PFFB: NVC0 middle memory controller
===================================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


MMIO registers
==============

.. space:: 8 pmfb 0x40000 middle memory controllers: compression and L2 cache
   0x00000[part:16] PART pmfb-part * PART
   0x3e000 PARTS pmfb-part * PART

.. space:: 8 pmfb-part 0x2000 middle memory controller partition
   0x0800 SLICES pmfb-slice
   0x1000[4] SLICE pmfb-slice

   .. todo:: fill me

.. space:: 8 pmfb-slice 0x400 middle memory controller slice

   .. todo:: fill me


.. _pmfb-intr:

Interrupts
==========

.. todo:: write me
