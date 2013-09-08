.. _pppp:

==================================
PPPP: video post-processing engine
==================================

.. contents::

.. todo:: write me


Introduction
============

.. todo:: write me


.. _pppp-falcon:

falcon parameters
=================

Present on:
    v0:
        NV98, NVAA, NVAC
    v1:
        NVA3:NVC0
    v2:
        NVC0:NVD9
    v3:
        NVD9+
BAR0 address:
    0x086000
PMC interrupt line:
    0
PMC enable bit:
    1
Version:
    v0:
        0
    v1-v2:
        3
    v3:
        4
Code segment size:
    0xa00
Data segment size:
    0x800
Fifo size:
    0x10
Xfer slots:
    8
Secretful:
    no
Code TLB index bits:
    6
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
    v1:
        :ref:`nva3-clock-vdclk`
    v2-v3:
        :ref:`nvc0-clock-vdclk`
NV50 VM engine:
    0x8
NV50 VM client:
    0x06
NV50 context DMA:
    0x6
NVC0 VM engine:
    0x11
NVC0 VM client:
    HUB 0x0c
Interrupts:
    ===== ===== ========== ================== ===============
    Line  Type  Present on Name               Description
    ===== ===== ========== ================== ===============
    8     edge  NVA3:NVC0  MEMIF_PORT_INVALID :ref:`MEMIF port not initialised <falcon-memif-intr-port-invalid>`
    9     edge  NVA3:NVC0  MEMIF_FAULT        :ref:`MEMIF VM fault <falcon-memif-intr-fault>`
    9     edge  NVC0-      MEMIF_BREAK        :ref:`MEMIF breakpoint <falcon-memif-intr-break>`
    10    level all        POUT_DONE          :ref:`Picture output finished <pppp-intr-pout-done>`
    11    level all        POUT_ERR           :ref:`Picture output error <pppp-intr-pout-err>`
    12    level all        FE_ERR             :ref:`Frontend error <pppp-intr-fe-err>`
    13    level all        VC1_ERR            :ref:`VC1 error <pppp-intr-vc1-err>`
    14    level all        FG_ERR             :ref:`Film grain error <pppp-intr-fg-err>`
    ===== ===== ========== ================== ===============
Status bits:
    ===== ========== ========== ============
    Bit   Present on Name       Description
    ===== ========== ========== ============
    0     all        FALCON     :ref:`Falcon unit <falcon-status>`
    1     all        MEMIF      :ref:`Memory interface <falcon-memif-status>`
    2     all        POUT       :ref:`Picture output <pppp-status-pout>`
    3     all        UNKE4      ???
    4     all        VC1        :ref:`VC1 <pppp-status-vc1>`
    5     all        FG         :ref:`Film grain <pppp-status-fg>`
    6     all        ???        ???
    7     all        HIST       :ref:`Histogram <pppp-status-hist>`
    8     v1-v2      UNK480     ???
    ===== ========== ========== ============
IO registers:
    :ref:`pppp-io`
MEMIF ports:
    ==== ======= ============
    Port Name    Description
    ==== ======= ============
    1    PIN     picture input
    2    POUT    picture output
    3    FG      ??? read [XXX]
    5    UNK480  ??? write [XXX]
    ==== ======= ============

.. todo:: interrupts
.. todo:: more MEMIF ports?
.. todo:: status bits


.. _pppp-io:

IO registers
============

============ =============== ========== =========== ===========
Host         Falcon          Present on Name        Description
============ =============== ========== =========== ===========
0x000:0x400  0x00000:0x10000 all        N/A         :ref:`Falcon registers <falcon-io-common>`
0x400:0x480  0x10000:0x12000 all        FE          :ref:`Front end <pppp-io-fe>`
0x480:0x500  0x12000:0x14000 v1-v2      ???         ???
0x500:0x5c0  0x14000:0x17000 all        FG          :ref:`Film grain effect <pppp-io-fg>`
0x5c0:0x600  0x17000:0x18000 all        VC1         :ref:`VC-1 postprocessing <pppp-io-vc1>`
0x600:0x640  0x18000:0x19000 all        MEMIF       :ref:`Memory interface <falcon-memif-io>`
0x640:0x680  0x19000:0x1a000 all        POUT        :ref:`Picture output <pppp-io-pout>`
0x680:0x740  0x1a000:0x1d000 all        HIST        :ref:`Histogram <pppp-io-hist>`
0x740:0x780  0x1d000:0x1e000 v1-        JOE         ???
0x780:0x7c0  0x1e000:0x1f000 v2-        ???         ???
0xfe0:0x1000 \-              v0:v3      FALCON_HOST :ref:`Falcon host registers <falcon-io-common>`
============ =============== ========== =========== ===========


Front end
=========

.. todo:: write me


.. _pppp-io-fe:

IO registers
------------

.. todo:: write


.. _pppp-intr-fe-err:

Interrupts
----------

.. todo:: write


Film grain synthesis
====================

.. todo:: write me


.. _pppp-io-fg:

IO registers
------------

.. todo:: write


.. _pppp-intr-fg-err:

Interrupts
----------

.. todo:: write


.. _pppp-status-fg:

Status report
-------------

.. todo:: write


VC-1 post-processing
====================

.. todo:: write me


.. _pppp-io-vc1:

IO registers
------------

.. todo:: write


.. _pppp-intr-vc1-err:

Interrupts
----------

.. todo:: write


.. _pppp-status-vc1:

Status report
-------------

.. todo:: write


Picture output
==============

.. todo:: write me


.. _pppp-io-pout:

IO registers
------------

.. todo:: write


.. _pppp-intr-pout-err:
.. _pppp-intr-pout-done:

Interrupts
----------

.. todo:: write


.. _pppp-status-pout:

Status report
-------------

.. todo:: write


Histogram
=========

.. todo:: write me


.. _pppp-io-hist:

IO registers
------------

.. todo:: write


.. _pppp-status-hist:

Status report
-------------

.. todo:: write
