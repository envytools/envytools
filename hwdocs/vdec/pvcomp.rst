.. _pvcomp:

===============================
PVCOMP: video compositor engine
===============================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


.. _pvcomp-falcon:

falcon parameters
=================

Present on:
    NVAF
BAR0 address:
    0x1c1000
PMC interrupt line:
    14
PMC enable bit:
    14
Version:
    3
Code segment size:
    0x1000
Data segment size:
    0xb00
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
IO addressing type:
    indexed
Core clock:
    :ref:`nva3-clock-vdclk`
NV50 VM engine:
    7
NV50 VM client:
    0x14
NV50 context DMA:
    0xa
Interrupts:
    ===== ===== ================== ===============
    Line  Type  Name               Description
    ===== ===== ================== ===============
    8     edge  MEMIF_PORT_INVALID :ref:`MEMIF port not initialised <falcon-memif-intr-port-invalid>`
    9     edge  MEMIF_FAULT        :ref:`MEMIF VM fault <falcon-memif-intr-fault>`
    10    level VCOMP              :ref:`compositor interrupt <pvcomp-intr>`
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
    ===== ========== ============
IO registers:
    :ref:`pvcomp-io`

.. todo:: status bits


.. _pvcomp-io:

IO registers
============

.. todo:: write me


.. _pvcomp-intr:

Interrupts
==========

.. todo:: write me
