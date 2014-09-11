.. _psec:

=======================================
PSEC: AES cryptographic security engine
=======================================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


.. _psec-falcon:

falcon parameters
=================

Present on:
    v0:
        G98, MCP77, MCP79
    v1:
        GM107-
BAR0 address:
    0x087000
PMC interrupt line:
    v0:
        14
    v1:
        15
PMC enable bit:
    14
Version:
    v0:
        0
    v1:
        5
Code segment size:
    v0:
        0xa00
    v1:
        0x8000
Data segment size:
    v0:
        0x800
    v1:
        0x4000
Fifo size:
    0x10
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
    27
Unified address space:
    no
IO addressing type:
    indexed
Core clock:
    v0:
        :ref:`g80-clock-nvclk`
    v1:
        ???
Tesla VM engine:
    0xa
Tesla VM client:
    0x0e
Tesla context DMA:
    0xa
Fermi VM engine:
    ???
Fermi VM client:
    ???
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
    :ref:`psec-io`

.. todo:: clock divider in 1530?

.. todo:: find out something about the GM107 version


.. _psec-io:

IO registers
============

.. space:: 8 psec 0x1000 VP3 cryptographic engine

   .. todo:: write me

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

.. todo:: update for GM107
