.. _pdisplay-daemon:

============================
PDISPLAY's monitoring engine
============================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


.. _pdisplay-daemon-falcon:

falcon parameters
=================

Present on:
	v0:
        NVD9:NVE4
	v1:
        NVE4+
BAR0 address:
    0x627000
PMC interrupt line:
    26 [shared with the rest of PDISPLAY], also INTR_HOST_SUMMARY bit 8
PMC enable bit:
    30 [all of PDISPLAY]
Version:
    4
Code segment size:
    0x4000
Data segment size:
    0x2000
Fifo size:
    3
Xfer slots:
    8
Secretful:
    no
Code TLB index bits:
    8
Code ports:
    1
Data ports:
    4
Version 4 unknown caps:
    31
Unified address space:
    no
IO adressing type:
    full
Core clock:
    ???
NVC0 VM engine:
    none
NVC0 VM client:
    HUB 0x03 [shared with rest of PDISPLAY]
Interrupts:
    ===== ===== ========== ========= ===============
    Line  Type  Present on Name      Description
    ===== ===== ========== ========= ===============
    12    level all        PDISPLAY  DISPLAY_DAEMON-routed interrupt
    13    level all        FIFO      
    14    level all        ???       520? 524 apparently not required
    15    level v1-        PNVIO     DISPLAY_DAEMON-routed interrupt, but also 554?
    ===== ===== ========== ========= ===============
Status bits:
    ===== ====== ============
    Bit   Name   Description
    ===== ====== ============
    0     FALCON :ref:`Falcon unit <falcon-status>`
    1     MEMIF  :ref:`Memory interface <falcon-memif-status>`
    ===== ====== ============
IO registers:
    :ref:`pdisplay-daemon-mmio`

.. todo:: more interrupts?
.. todo:: interrupt refs
.. todo:: MEMIF interrupts
.. todo:: determine core clock


.. _pdisplay-daemon-mmio:

MMIO registers
==============

================= ========== ============ ============
Address           Present on Name         Description
================= ========== ============ ============
0x627000:0x627400 all        N/A          :ref:`Falcon registers <falcon-io-common>`
0x627400          all        ???          [alias of 610018]
0x627440+i*4      all        FIFO_PUT
0x627450+i*4      all        FIFO_GET
0x627460          all        FIFO_INTR
0x627464          all        FIFO_INTR_EN
0x627470+i*4      all        RFIFO_PUT
0x627480+i*4      all        RFIFO_GET
0x627490          all        RFIFO_STATUS
0x6274a0          v1-        ???          [ffffffff/ffffffff/0]
0x627500+i*4      all        ???
0x627520          v1-?       ???          interrupt 14
0x627524          v1-        ???          [0/ffffffff/0]
0x627550          v1-        ???          [2710/ffffffff/0]
0x627554          v1-        ???          interrupt 15 [0/1/0]
0x627600:0x627680 all        MEMIF        :ref:`Memory interface <falcon-memif-io>`
0x627680:0x627700 all        \-           [alias of 627600+]
================= ========== ============ ============

.. todo:: refs
