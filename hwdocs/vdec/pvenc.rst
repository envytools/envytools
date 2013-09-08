.. _pvenc:

============================
PVENC: video encoding engine
============================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


.. _pvenc-falcon:

falcon parameters
=================

Present on:
    NVE4+
BAR0 address:
    0x1c2000
PMC interrupt line:
    16
PMC enable bit:
    18
Version:
    4
Code segment size:
    0x4000
Data segment size:
    0x1800
Fifo size:
    0x10
Xfer slots:
    8
Secretful:
    no
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
    0x19
NVC0 VM client:
    HUB 0x1b
Interrupts:
    ===== ===== ================== ===============
    Line  Type  Name               Description
    ===== ===== ================== ===============
    9     edge  MEMIF_BREAK        :ref:`MEMIF breakpoint <falcon-memif-intr-break>`
    ===== ===== ================== ===============
Status bits:
    ===== ========== ============
    Bit   Name       Description
    ===== ========== ============
    0     FALCON     :ref:`Falcon unit <falcon-status>`
    1     MEMIF      :ref:`Memory interface <falcon-memif-status>`
    2     ???        ???
    3     ???        ???
    4     ???        ???
    5     ???        ???
    6     ???        ???
    7     ???        ???
    8     ???        ???
    9     ???        ???
    10    ???        ???
    11    ???        ???
    12    ???        ???
    ===== ========== ============
IO registers:
    :ref:`pvenc-io`

.. todo:: status bits
.. todo:: interrupts
.. todo:: MEMIF ports
.. todo:: core clock


.. _pvenc-io:

IO registers
============

.. todo:: write me
