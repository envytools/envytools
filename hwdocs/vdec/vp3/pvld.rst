.. _pvld:

=====================================
PVLD: variable length decoding engine
=====================================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


.. _pvld-falcon:

falcon parameters
=================

Present on:
    v0:
        NV98, NVAA, NVAC
    v1:
        NVA3:NVAF
    v2:
        NVAF
    v3:
        NVC0:NVD9
    v4:
        NVD9+
BAR0 address:
    0x084000
PMC interrupt line:
    15
PMC enable bit:
    15
Secretful:
    v0:
        no
    v1+:
        yes
Version:
    v0:
        0
    v1-v3:
        3
    v4:
        4
Code segment size:
    v0:
        0x1000
    v1:
        0x1800
    v2+:
        0x2000
Data segment size:
    v0-v1:
        0x1000
    v2-v3:
        0x2000
    v4:
        0x1000
Fifo size:
    0x10
Xfer slots:
    8
Code TLB index bits:
    8
Code ports:
    1
Data ports:
    1
Version 4 unknown caps:
    31
Unified address space:
    no
IO addressing type:
    indexed
Core clock:
    v0:
        :ref:`nv98-clock-vdclk`
    v1-v2:
        :ref:`nva3-clock-vdclk`
    v3-v4:
        :ref:`nvc0-clock-vdclk`
NV50 VM engine:
    0x9
NV50 VM client:
    0x0d
NV50 context DMA:
    0x8
NVC0 VM engine:
    0x10
NVC0 VM client:
    HUB 0x0d
Interrupts:
    ===== ===== ========== ================== ===============
    Line  Type  Present on Name               Description
    ===== ===== ========== ================== ===============
    8     edge  NVA3:NVC0  MEMIF_PORT_INVALID :ref:`MEMIF port not initialised <falcon-memif-intr-port-invalid>`
    9     edge  NVA3:NVC0  MEMIF_FAULT        :ref:`MEMIF VM fault <falcon-memif-intr-fault>`
    9     edge  NVC0-      MEMIF_BREAK        :ref:`MEMIF breakpoint <falcon-memif-intr-break>`
    10    level all        VLD                :ref:`VLD interrupt <pvld-intr-vld>`
    11    level v1-        CRYPT              :ref:`crypto coprocessor <falcon-crypt-intr>`
    ===== ===== ========== ================== ===============
Status bits:
    ===== ========== ========== ============
    Bit   Present on Name       Description
    ===== ========== ========== ============
    0     all        FALCON     :ref:`Falcon unit <falcon-status>`
    1     all        MEMIF      :ref:`Memory interface <falcon-memif-status>`
    2     all        VLD        :ref:`VLD unit <pvld-status-vld>`
    3     v1-        ???        ???
    4     v2-        ???        ???
    ===== ========== ========== ============
IO registers:
    :ref:`pvld-io`
MEMIF ports:
    ==== ======= ============
    Port Name    Description
    ==== ======= ============
    1    STREAM  bitstream input
    2    MBRING  MBRING output
    4    BUCKET  temp bucket
    ==== ======= ============

.. todo:: MEMIF ports


.. _pvld-io:

IO registers
============

============ =============== ========== =========== ===========
Host         Falcon          Present on Name        Description
============ =============== ========== =========== ===========
0x000:0x400  0x00000:0x10000 all        N/A         :ref:`Falcon registers <falcon-io-common>`
0x400:0x600  0x10000:0x18000 all        VLD         :ref:`VLD registers <pvld-io-vld>`
0x600:0x640  0x18000:0x19000 all        MEMIF       :ref:`Memory interface <falcon-memif-io>`
0x640:0x680  0x19000:0x1a000 v1-        JOE         ???
0x680:0x700  0x1a000:0x1c000 ???        ???         ???
0x800:0x900  0x20000:0x24000 v1-        CRYPT       :ref:`Crypto coprocessor <falcon-crypt-io>`
0x900:0xa00  0x24000:0x28000 v1-        ???         :ref:`??? <falcon-crypt-io>`
0xc00:0xc40  0x30000:0x31000 v1-        ???         :ref:`??? <falcon-crypt-io>`
0xd00:0xd40  0x31000:0x32000 v1-        ???         :ref:`??? <falcon-crypt-io>`
0xfe0:0x1000 \-              v0:v4      FALCON_HOST :ref:`Falcon host registers <falcon-io-common>`
============ =============== ========== =========== ===========

.. todo:: unknowns
.. todo:: fix list


.. _pvld-vld:

VLD unit
========

.. todo:: write me


.. _pvld-io-vld:

IO registers
------------

.. todo:: write me


.. _pvld-intr-vld:

Interrupts
----------

.. todo:: write me


.. _pvld-status-vld:

Status report
-------------

.. todo:: write me
