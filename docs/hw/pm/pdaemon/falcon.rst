.. _pdaemon-falcon:

=================
falcon parameters
=================

Present on:
	v0:
        NVA3:NVAF
	v1:
        NVAF:NVC0
	v2:
        NVC0:NVD9
	v3:
        NVD9:NVE4
	v4: 
        NVE4+
BAR0 address:
    0x10a000
PMC interrupt line:
	v0-v1:
        18
	v2+:
        24
PMC enable bit:
	v0-v1:
        none, use reg 0x22210 instead
	v2+:
        13
Version:
	v0-v2:
        3
	v3,v4:
        4
Code segment size:
	v0:
        0x4000
	v1+:
        0x6000
Data segment size:
	v0:
        0x3000
	v1+:
        0x6000
Fifo size:
	v0-v1:
        0x10
	v2+:
        3
Xfer slots:
	v0-v2:
        8
	v3-v4:
        0x10
Secretful:
    no
Code TLB index bits:
	v0-v2:
        8
	v3-v4:
        9
Code ports:
    1
Data ports:
    4
Version 4 unknown caps:
    31
Unified address space:
    yes [on v3, v4]
IO addressing type:
	v0-v2:
        indexed
	v3-v4:
        simple
Core clock:
    v0-v1:
        :ref:`nva3-clock-dclk`
    v2-v4:
        :ref:`nvc0-clock-dclk`
NV50 VM engine:
    0xe
NV50 VM client: 
    0x11
NV50 context DMA:
    [none]
NVC0 VM engine:
    0x17
NVC0 VM client:
    HUB 0x12
Interrupts:
    ===== ===== ========== ================== ===============
    Line  Type  Present on Name               Description
    ===== ===== ========== ================== ===============
    8     edge  NVA3:NVC0  MEMIF_PORT_INVALID :ref:`MEMIF port not initialised <falcon-memif-intr-port-invalid>`
    9     edge  NVA3:NVC0  MEMIF_FAULT        :ref:`MEMIF VM fault <falcon-memif-intr-fault>`
    9     edge  NVC0-      MEMIF_BREAK        :ref:`MEMIF breakpoint <falcon-memif-intr-break>`
    10    level all        PMC_DAEMON         :ref:`PMC interrupts routed directly to PDAEMON <pdaemon-intr-pmc-daemon>`
    11    level all        SUBINTR            :ref:`second-level interrupt <pdaemon-intr-subintr>`
    12    level all        THERM              :ref:`PTHERM subinterrupts routed to PDAEMON <pdaemon-intr-therm>`
    13    level all        SIGNAL             :ref:`input signal rise/fall interrupts <pdaemon-intr-signal>`
    14    level all        TIMER              :ref:`the timer interrupt <pdaemon-intr-timer>`
    15    level all        IREDIR_PMC         :ref:`PMC interrupts redirected to PDAEMON by IREDIR <pdaemon-intr-iredir-pmc>`
    ===== ===== ========== ================== ===============
Status bits:
    ===== ========== ========== ============
    Bit   Present on Name       Description
    ===== ========== ========== ============
    0     all        FALCON     :ref:`Falcon unit <falcon-status>`
    1     all        EPWR_GRAPH :ref:`PGRAPH engine power gating <pdaemon-status-epwr>`
    2     all        EPWR_VDEC  :ref:`video decoding engine power gating <pdaemon-status-epwr>`
    3     all        MEMIF      :ref:`Memory interface <falcon-memif-status>`
    4     NVA3:NVAF  USER       :ref:`User controlled <pdaemon-status-user>`
          NVC0-
    4     NVAF:NVC0  EPWR_VCOMP :ref:`PVCOMP engine power gating <pdaemon-status-epwr>`
    5     NVAF:NVC0  USER       :ref:`User controlled <pdaemon-status-user>`
    ===== ========== ========== ============
IO registers:
    :ref:`pdaemon-io`
