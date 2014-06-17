.. _nv03-pgraph:

==================================
NV03 PGRAPH: 2d/3d graphics engine
==================================

.. contents::


Introduction
============

.. todo:: write me


MMIO registers
==============

.. space:: 8 nv03-pgraph 0x1000 2d/3d graphics engine

   .. todo:: write me


.. _nv03-pgraph-intr:

Interrupts
==========

.. todo:: write me


Method submission
=================

.. todo:: write me


Graph objects
=============

On NV03, object options were expanded and moved to a memory structure in
RAMIN. The data stored in RAMHT and passed to PGRAPH is just a [shifted]
pointer to the grobj structure. Most importantly, the DMA objects bound
to the graph object are now stored in the options structure and don't have
to be swapped by software on every graphics object switch. The graph
object options structure is made of 3 32-bit words aligned on 0x10-byte
bounduary:

word 0:
  ???
  
.. todo:: figure out the bits, should be similiar to the NV01 options

word 1:
  - bits 0-15: main DMA object. This is used for GDI, SIFM, ITM, D3D, M2MF.
    For M2MF, this is the source DMA object.
  - bits 16-31: NOTIFY DMA object.

.. todo:: check M2MF source

word 2:
  - bits 0-15: secondary DMA object. This is used for M2MF destination DMA
    object.
    
.. todo:: check

The options structure, and thus also the graph object, is selected by the
structure address in RAMIN shifted right by 4 bits. Thus graph object 0x1234
has its options structure at RAMIN address 0x12340.


Context
=======

.. todo:: write me


Surface setup
=============

.. todo:: write me


Drawing operation
=================

.. todo:: write me
