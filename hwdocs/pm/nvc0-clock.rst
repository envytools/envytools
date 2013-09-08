.. _nvc0-clock:

============
NVC0+ clocks
============

.. contents::

.. todo:: write me


Introduction
============

NVC0+ cards have the following clocks:

- root clocks [RPLL1-4]: used as base for other clocks
- host clock: clocks the host interface parts, like PFIFO
- GPC clock [GPCPLL, clock #0]: used to clock the GPCs
- ROP clock [ROPPLL, clock #1]: used to clock the ROPs - not present on NVC1,
  XBAR clock used instead
- XBAR clock [XBARPLL, clock #2]: used to clock the crossbar between GPCs and
  ROPs, as well as ROPs on NVC1
- core clock [NVPLL, clock #7]: clocks the core card logic
- VM clock [clock #8]: clocks the TLBs and page table lookup circuitry
- HUB clock [clock #9]: clocks the common part of PGRAPH, and the PCOPY engines
- timer clock [TCLK, clock #11]: clocks the PTIMER circuitry
- daemon clock [clock #12]: used to clock PDAEMON
- vdec clock [clock #14]: used to clock the falcon video decoding engines
- memory clock [MPLL, clock M]: used to clock the VRAM, one per memory partition
- video clocks [VPLL1,VPLL2]: used to drive the video outputs

.. todo:: how many RPLLs are there exactly?
.. todo:: figure out where host clock comes from
.. todo:: VM clock is a guess
.. todo:: memory clock uses two PLLs, actually

The root clocks are set up in PNVIO area, VPLLs are set up in PDISPLAY area,
the MPLLs are set up in PMCLOCK area, and the other clocks are set up in
PCLOCK area.


.. _nvc0-pclock-mmio:
.. _nvc0-pioclock-mmio:

MMIO registers
==============

.. todo:: write me


.. _nvc0-clock-tclk:

TCLK: timer clock
=================

.. todo:: write me


.. _nvc0-clock-dclk:

DCLK: daemon clock
==================

.. todo:: write me


.. _nvc0-clock-vdclk:

VDCLK: video decoding clock
===========================

.. todo:: write me
