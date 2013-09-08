.. _pcrypt3:

=================================
PCRYPT3: AES cryptographic engine
=================================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


.. _pcrypt3-falcon:

falcon parameters
=================

Present on:
    NV98, NVAA, NVAC
BAR0 address:
    0x087000
PMC interrupt line:
    14
PMC enable bit:
    14
Version:
    0
Code segment size:
    0xa00
Data segment size:
    0x800
Fifo size:
    0x10
Xfer slots:
    8
Secretful:
    yes
IO addressing type:
    indexed
Core clock:
    :ref:`nv50-clock-nvclk`
NV50 VM engine:
    0xa
NV50 VM client:
    0x0e
NV50 context DMA:
    0xa
Interrupts:
    ===== ===== ================== ===============
    Line  Type  Name               Description
    ===== ===== ================== ===============
    8     edge  MEMIF_PORT_INVALID :ref:`MEMIF port not initialised <falcon-memif-intr-port-invalid>`
    9     edge  MEMIF_FAULT        :ref:`MEMIF VM fault <falcon-memif-intr-fault>`
    10    level CRYPT              :ref:`crypto coprocessor <falcon-crypt-intr>`
    ===== ===== ================== ===============
Status bits:
    ===== ========== ============
    Bit   Name       Description
    ===== ========== ============
    0     FALCON     :ref:`Falcon unit <falcon-status>`
    1     MEMIF      :ref:`Memory interface <falcon-memif-status>`
    ===== ========== ============
IO registers:
    :ref:`pcrypt3-io`

.. todo:: clock divider in 1530?


.. _pcrypt3-io:

IO registers
============

============ =============== =========== ===========
Host         Falcon          Name        Description
============ =============== =========== ===========
0x000:0x400  0x00000:0x10000 N/A         :ref:`Falcon registers <falcon-io-common>`
0x600:0x640  0x18000:0x19000 MEMIF       :ref:`Memory interface <falcon-memif-io>`
0x800:0x900  0x20000:0x24000 CRYPT       :ref:`Crypto coprocessor <falcon-crypt-io>`
0x900:0xa00  0x24000:0x28000 ???         :ref:`??? <falcon-crypt-io>`
0xc00:0xc40  0x30000:0x31000 ???         :ref:`??? <falcon-crypt-io>`
0xd00:0xd40  0x31000:0x32000 ???         :ref:`??? <falcon-crypt-io>`
0xfe0:0x1000 \-              FALCON_HOST :ref:`Falcon host registers <falcon-io-common>`
============ =============== =========== ===========
