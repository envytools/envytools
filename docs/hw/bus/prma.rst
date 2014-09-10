.. _prma:

==========================
PRMA: Real mode BAR access
==========================

.. contents::


Introduction
============

.. todo:: write me


The MMIO registers
==================

.. space:: 8 prma 0x1000 real mode BAR access
   0x080 CTRL prma-ctrl G80:
   0x084 SCRATCH prma-scratch G80:
   0x100 SIG prma-sig
   0x104 ADDR prma-addr
   0x10c DATA_PARTIAL prma-data-partial
   0x114 DATA_PARTIAL_INC prma-data-partial NV3:NV4

.. space:: 8 nv3-rma 0x100 real mode BAR access
   0x00 SIG prma-sig
   0x04 ADDR prma-addr
   0x08 DATA prma-data
   0x0c DATA_PARTIAL prma-data-partial
   0x10 DATA_INC prma-data NV3:NV4
   0x14 DATA_PARTIAL_INC prma-data-partial NV3:NV4

   On NV3:NV4, this space is accessible through PCI BAR #2.

.. reg:: 32 prma-sig signature

   Read-only register, always reads as 0x2b16d065. Can be used as a signature,
   to verify the RMA space is visible.

.. reg:: 32 prma-addr BAR address

   .. todo:: write me

.. reg:: 32 prma-data BAR data

   .. todo:: write me

.. reg:: 32 prma-data-partial BAR partial data for 8/16-bit accesses

   .. todo:: write me

.. reg:: 32 prma-ctrl RMA enable & register selection

   .. todo:: write me

.. reg:: 32 prma-scratch scratch register

   .. todo:: write me
