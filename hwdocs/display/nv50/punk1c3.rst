.. _punk1c3:

============
PUNK1C3: ???
============

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


.. _punk1c3-falcon:

falcon parameters
=================

Present on:
    NVD9+
BAR0 address:
    0x1c3000
PMC interrupt line:
    ???
PMC enable bit:
    none
Version:
    4
Code segment size:
    0x1000
Data segment size:
    0x1000
Fifo size:
    3
Xfer slots:
    8
Secretful:
    yes
Code TLB index bits:
    8
Code ports:
    1
Data ports:
    1
Version 4 unknown caps:
    none
Unified address space:
    no
IO addressing type:
    simple
Core clock:
    ???
NVC0 VM engine:
    none
NVC0 VM client:
    HUB 0x1e
Interrupts:
    ===== ===== ================== ===============
    Line  Type  Name               Description
    ===== ===== ================== ===============
    8     level CRYPT              :ref:`crypto coprocessor <falcon-crypt-intr>`
    ===== ===== ================== ===============
Status bits:
    ===== ====== ============
    Bit   Name   Description
    ===== ====== ============
    0     FALCON :ref:`Falcon unit <falcon-status>`
    1     MEMIF  :ref:`Memory interface <falcon-memif-status>`
    ===== ====== ============
IO registers:
    :ref:`punk1c3-io`

.. todo:: MEMIF interrupts
.. todo:: determine core clock


.. _punk1c3-io:

IO registers
============

The IO registers:

=========== ===== ============
Address     Name  Description
=========== ===== ============
0x000:0x400 N/A   :ref:`Falcon registers <falcon-io-common>`
0x600:0x700 MEMIF :ref:`Memory interface <falcon-memif-io>`
0x800:0x900 CRYPT :ref:`Crypto coprocessor <falcon-crypt-io>`
0x900:0xa00 ???   :ref:`??? <falcon-crypt-io>`
0xa00:0xc00 ???   ???
0xc00:0xc40 ???   :ref:`??? <falcon-crypt-io>`
0xd00:0xd40 ???   :ref:`??? <falcon-crypt-io>`
=========== ===== ============

.. todo:: figure out unknowns
