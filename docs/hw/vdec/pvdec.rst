.. _pvdec:

================================
PVDEC: VP6 video decoding engine
================================

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
    0x84000
PMC interrupt line:
    15
PMC enable bit:
    15
Version:
    5
Code segment size:
    ???
Data segment size:
    ???
Fifo size:
    0x10
Xfer slots:
    8
Secretful:
    yes
Code TLB index bits:
    9
Code ports:
    1
Data ports:
    1
Version 4 unknown caps:
    ???
Unified address space:
    no
IO addressing type:
    ???
Core clock:
    ???
Fermi VM engine:
    ???
Fermi VM client:
    ???
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
    ===== ========== ============
IO registers:
    :ref:`pvdec-io`

.. todo:: status bits
.. todo:: interrupts
.. todo:: MEMIF ports
.. todo:: core clock


.. _pvdec-io:

IO registers
============

.. space:: 8 pvdec 0x1000 VP6 video decoding engine

   .. todo:: write me

.. todo:: write me
