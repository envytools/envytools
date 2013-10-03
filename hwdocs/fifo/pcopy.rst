.. _pcopy:
.. _pcopy-falcon:
.. _pcopy-io:
.. _pcopy-mmio:
.. _pcopy-intr:

====================
PCOPY copying engine
====================

Present on:
    cv0 [1 engine]: NVA3:NVC0

    cv1 [2 engines]: NVC0:NVE4

    cv2 [3 engines]: NVE4+
BAR0 address:
    engine #0: 0x104000

    engine #1: 0x105000

    engine #2: 0x105000
PMC interrupt line:
    cv0: 22

    cv1, engine #0: 5

    cv1, engine #1: 6
PMC enable bit:
    cv0: 13

    cv1, engine #0: 6

    cv1, engine #0: 7
Version:
    cv0, cv1: 3

    cv2: [none]
Code segment size:
    0x1200
Data segment size:
    0x800
Fifo size:
    0x10
Xfer slots:
    8
Secretful:
    no
Code TLB index bits:
    cv0: 5

    cv1: 7
Code ports:
    1
Data ports:
    1
IO addressing type:
    indexed
Core clock:
    cv0: NVCLK

    cv1: hub clock [nvc0 clock #9]
NV50 VM engine:
    0xd
NV50 VM client:
    0x13
NV50 context DMA:
    0xc
NVC0 VM engine:
    engine #0: 0x15

    engine #1: 0x16

    engine #2: 0x1b
NVC0 VM client:
    engine #0: HUB 0x01

    engine #1: HUB 0x02

    engine #2: HUB 0x18

The IO registers:

    ============ =============== =========== ===========
    Host         Falcon          Name        Description
    ============ =============== =========== ===========
    0x600:0x640  0x18000:0x19000 MEMIF       :ref:`Falcon memory interface<falcon-memif>`
    0x640:0x680  0x19000:0x1a000 ???         ??? :ref:`pcopy`
    0x800:0x880  0x20000:0x22000 COPY        :ref:`Copy engine<pcopy>`
    0x900:0x980  0x24000:0x26000 ???         ??? :ref:`pcopy`
    ============ =============== =========== ===========

Interrupts:
    ===== ===== ==================== ===============
    Line  Type  Name                 Description
    ===== ===== ==================== ===============
    8     edge  MEMIF_TARGET_INVALID [NVA3:NVC0] :ref:`MEMIF port not initialised <falcon-memif-intr-port-invalid>`
    9     edge  MEMIF_FAULT          [NVA3:NVC0] :ref:`MEMIF VM fault <falcon-memif-intr-fault>`
    9     edge  MEMIF_BREAK          [NVC0-] :ref:`MEMIF Break <falcon-memif-intr-break>`
    10    level COPY_BLOCK
    11    level COPY_NONBLOCK
    ===== ===== ==================== ===============

Status bits:
    ==== ======= ===========
    Bit  Name    Description
    ==== ======= ===========
    0    FALCON  :ref:`Falcon unit <falcon-proc>`
    1    MEMIF   :ref:`Memory interface <falcon-memif>` and :ref:`COPY <pcopy>`
    ==== ======= ===========

.. todo:: describe PCOPY
