.. _nv50-clock:

================
NV50:NVA3 clocks
================

.. contents::

.. todo:: write me


Introduction
============

NV50:NVA3 cards have the following clocks:

- crystal clock: input from a quartz crystal, user as the base for other
  clocks
- PCIE reference clock [HREF]: input from the PCIE bus, used as the base
  for other clocks
- root clocks [RPLL1, RPLL2]: used as the base for other clocks
- host clock [HCLK]: clocks the host interface parts, like PFIFO
- timer clock [TCLK]: clocks the PTIMER circuitry, only present on NV84+
- NVIO clock [IOCLK]: used for communication with the NVIO chip, NV50 and
  NVA0 only
- memory clock [MCLK]: used to clock the VRAM, not present on IGPs
- unknown clock 4010: present on NV50, NV92, NVA0 only
- unknown clock 4018: present on NV50, NVA0 only
- unknown clock 4088: present on NVA0 only
- core clock [NVCLK]: clocks most of the card's logic
- shader clock [SCLK]: clocks the CUDA multiprocessor / shader units
- xtensa clock [XTCLK]: clocks the xtensa cores used for video decoding,
  only present on NV84:NV98 and NVA0
- vdec clock [VDCLK]: clocks the remaining parts of video decoding engines,
  only present on NV84+
- video clocks [VCLK1,VCLK2]: used to drive the video outputs

.. todo:: figure out IOCLK, ZPLL, DOM6
.. todo:: figure out 4010, 4018, 4088

The root clocks are set up in PNVIO area, VPLLs are set up in PDISPLAY area,
and the other clocks are set up in PCLOCK area.

.. todo:: write me


.. _nv50-pclock-mmio:
.. _nv50-pioclock-mmio:
.. _nv50-pcontrol-mmio:

MMIO registers
==============

.. todo:: write me


.. _nv50-clock-hclk:

HCLK: host clock
================

.. todo:: write me


.. _nv50-clock-nvclk:

NVCLK: core clock
=================

.. todo:: write me


.. _nv84-clock-tclk:

TCLK: timer clock
=================

.. todo:: write me


.. _nv98-clock-vdclk:

VDCLK: video decoding clock
===========================

.. todo:: write me
