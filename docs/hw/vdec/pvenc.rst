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
    v0:
        GK104:GM107
    v1:
        GM107+
BAR0 address:
    v0:
        0x1c2000
    v1:
        0x1c8000
PMC interrupt line:
    16
PMC enable bit:
    18
Version:
    v0:
        4
    v1:
        5
Code segment size:
    0x4000
Data segment size:
    v0:
        0x1800
    v1:
        0x2c00
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
Fermi VM engine:
    0x19
Fermi VM client:
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

.. space:: 8 gk104-pvenc 0x1000 H.264 video encoding engine

   .. todo:: write me

.. space:: 8 gm107-pvenc 0x2000 H.264 video encoding engine

   .. todo:: write me

.. todo:: write me
