.. _pme:

================================
Overview of VPE motion estimator
================================

.. contents::


Introduction
============

.. todo:: write me


MMIO registers
==============

.. space:: 8 pme 0x800 VPE motion estimator


   .. todo:: write me


.. _me-fifo:

FIFO methods
============

PME is controlled by PFIFO.  Its PFIFO engine id is 3.  It doesn't care about
PFIFO subchannels or object bindings - it exports only a single engine object
and treats all incoming methods as belonging to that object.

.. space:: 8 me 0x2000 motion estimation FIFO methods
   0x0000 OBJECT pme-mthd-object
   0x0108 UNK108 pme-mthd-unk108
   0x0120 UNK120 pme-mthd-unk120
   0x0160 UNK160 pme-mthd-unk160
   0x0164 UNK164 pme-mthd-unk164
   0x0168 UNK168 pme-mthd-unk168
   0x016c UNK16C pme-mthd-unk16c
   0x0170 UNK170 pme-mthd-unk170
   0x0174 UNK174 pme-mthd-unk174
   0x0178 UNK178 pme-mthd-unk178
   0x017c UNK17C pme-mthd-unk17c
   0x01b0 DMA_UNK1B0 pme-mthd-unk1b0 G80:
   0x01b4 DMA_UNK1B4 pme-mthd-unk1b4 G80:
   0x01c0 DMA_UNK1C0 pme-mthd-unk1c0 G80:
   0x0200 UNK200 pme-mthd-unk200
   0x0204 UNK204 pme-mthd-unk204
   0x0208 UNK208 pme-mthd-unk208
   0x020c UNK20C pme-mthd-unk20c
   0x0210 UNK210 pme-mthd-unk210
   0x0214 UNK214 pme-mthd-unk214
   0x0228 UNK228 pme-mthd-unk228
   0x022c UNK22C pme-mthd-unk22c

   .. todo:: write me

.. reg:: 32 pme-mthd-object

   .. todo:: write me


Unknown methods
---------------

.. todo:: figure these out

.. reg:: 32 pme-mthd-unk108

   The valid values seem to be 0 and 1.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk120

   All values are valid.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk160

   - bits 0-15: ???, has to be aligned to 0x40
   - bits 16-31: ???, has to be aligned to 0x40

   .. todo:: write me

.. reg:: 32 pme-mthd-unk164

   - bits 0-15: ???, has to be aligned to 0x10 and non-0
   - bits 16-31: ???, has to be aligned to 2 and non-0

   .. todo:: write me

.. reg:: 32 pme-mthd-unk168

   - bits 0-15: ???, has to be aligned to 0x10 and non-0
   - bits 16-31: ???, has to be aligned to 0x10 and non-0

   .. todo:: write me

.. reg:: 32 pme-mthd-unk16c

   - bits 0-15: ???, has to be aligned to 0x10
   - bits 16-31: ???, has to be aligned to 2

   .. todo:: write me

.. reg:: 32 pme-mthd-unk170

   - bits 0-15: ???, has to be aligned to 0x10
   - bits 16-31: ???, has to be aligned to 2

   Not all values are valid...

   .. todo:: write me

.. reg:: 32 pme-mthd-unk174

   - bits 0-15: ???, has to be aligned to 0x10
   - bits 16-31: ???, has to be aligned to 2

   .. todo:: write me

.. reg:: 32 pme-mthd-unk178

   Has to be aligned to 8.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk17c

   Seems to be the trigger method.  Valid values unknown.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk1b0

   A DMA method of unknown purpose.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk1b4

   A DMA method of unknown purpose.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk1c0

   A DMA method of unknown purpose.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk200

   Has to be aligned to 0x40.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk204

   Has to be aligned to 0x40.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk208

   Has to be aligned to 0x40.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk20c

   Has to be aligned to 0x40.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk210

   Has to be aligned to 0x40.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk214

   Has to be aligned to 0x40.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk228

   Has to be aligned to 8.

   .. todo:: write me

.. reg:: 32 pme-mthd-unk22c

   Has to be aligned to 8.

   .. todo:: write me


.. _pme-intr:

Interrupts
==========

.. todo:: write me
