.. _pvdec:

============================
PVDEC: video decoding engine
============================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


.. _pvdec-falcon:

falcon parameters
=================

Present on:
    GM107+
BAR0 address:
    0x084000
PMC interrupt line:
    17
PMC enable bit:
    15
Secretful:
    yes
Version:
    5
Code segment size:
    0x2000
Data segment size:
    0x1800
Fifo size:
    0x10
Xfer slots:
    8
Code TLB index bits:
    9
Code ports:
    1
Data ports:
    1
Version 4 unknown caps:
    31, 27
Unified address space:
    no
IO addressing type:
    indexed
Core clock:
    :ref:`gf100-clock-vdclk`
Fermi VM engine:
    ???
Fermi VM client:
    ???
Interrupts:
    ???
Status bits:
    ???
IO registers:
    :ref:`pvdec-io`
MEMIF ports:
    ???

.. todo:: interrupts
.. todo:: VM engine/client
.. todo:: MEMIF ports
.. todo:: status bits


.. _pvdec-io:

IO registers
============

.. space:: 8 pvdec 0x1000 VP6 video decoding engine

   .. todo:: write me

============ =============== ========== =========== ===========
Host         Falcon          Present on Name        Description
============ =============== ========== =========== ===========
0x000:0x400  0x00000:0x10000 all        N/A         :ref:`Falcon registers <falcon-io-common>`
0x600:0x640  0x18000:0x19000 all        MEMIF       :ref:`Memory interface <falcon-memif-io>`
============ =============== ========== =========== ===========

.. todo:: write me
