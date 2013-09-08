.. _pcounter:

====================================
PCOUNTER: performance counter engine
====================================

.. contents::

.. todo:: convert

::

    TOC

    0. Introduction
    1. MMIO registers
    1.1. NV10
    1.2. NV40
    1.3. NVC0
    2. The PCOUNTER signals
    2.1. The STATUS registers
    2.2. Trailer signals
    2.3. The EVENT and FLAG signals
    2.4. The PM_TRIGGER and WRCACHE_FLUSH signals
    2.5. The USER signals
    2.6. The PERIODIC signal
    3. Input selection
    4. Counters
    4.1. Counter modes
    5. Control registers
    6. Single event mode
    7. Quad event mode
    8. Record mode
    8.1. Record mode setup
    9. The flag

    = Introduction =

    PCOUNTER is the card units that contains performance monitoring counters.
    It is present on NV10+ GPUs, with the exception of NV11, NV17, NV18 for
    unknown reasons [XXX: why? any others excluded? NV1A, NV25, NV2A, NV30, NV36
    pending a check].

    PCOUNTER is actually made of several identical hardware counter units, one
    for each so-called domain. Each PCOUNTER domain can potentially run on
    a different source clock, allowing one to monitor events in various clock
    domains. The PCOUNTER domains are mostly independent, but there's some
    limitted communication and shared circuitry among them.

    There are two major revisions of PCOUNTER hardware, and some minor
    subrevisions:

     - NV10:NVC0 major revision:
      - NV10:NV15 - first version, one domain, only single-event mode available
      - NV15:NV20 - added one period / all periods event counter mode switch
      - NV20:NV30 - added second domain for events associated with memory clock
      - NV30:NV40 - removed separate clrflag/setflag input selection, changed
        from 40-bit to 32-bit counters, added quad event mode, added logic op
        chaining through SETFLAG.
      - NV40:NV84 - rearranged register space to make space for 8 domains, added
        3 new special counter modes
      - NV84:NV92 - added record mode, swap input selection, and PERIODIC signals
      - NV92:NVA3 - added slightly more flexible logic op delayed source selection
        and a register to set high 8 bits of address for record mode
      - NVA3:NVC0 - added USER signals
     - NVC0+ major revision:
      - NVC0+ - split PCOUNTER into hub, per-gpc and per-partition domain sets,
        ???
    [XXX: figure out what else]

    NOTE: the information in this document is at the moment not fully verified
    for NVC0+. [XXX: make it so]

    The inputs to PCOUNTER are various activity monitoring signals from all over
    the card. The PCOUNTER hardware selects a few of them, performs programmable
    logic operations on them, and aggregates it to a handful of actual counter
    inputs. Some of the inputs are special and control counting start/stop, while
    others are the events to be counted. PCOUNTER can be used in three modes:

     - single event mode - a single event is being counted, with fine-grained
       control of counting periods via pre-start/start/stop signals. Several
       counting periods per run may be configured, and a threshold counter may
       be used. The input signals used are:
       - PRE - a programmable amount of pulses on this input must happen before
         START is recognised
       - START - a pulse on this input starts a counting period
       - EVENT - the pulses on this input are counted
       - STOP - a pulse on this input stops a counting period
     - quad event mode [NV30-] - 4 events are being counted, with a simple "swap
       counter sets" trigger to delimit counting periods. The inputs used are:
       - PRE, START, EVENT, STOP - the pulses on these inputs are counted [in 4
         separate counters]
       - SWAP - a pulse on this input swaps counter sets, ie. copies the internal
         counters to the MMIO registers and resets internal counters to 0.
     - record mode [NV84-] - 12 simple events are being counted, and the counters
       written to a "record buffer" in memory on every pulse of STOP input. The
       inputs used are:
       - PRE_SRC[0..3], START_SRC[0..3], EVENT_SRC[0..3] - 12 events to be counted
       - STOP - a pulse on this input writes current counter values to memory and
         clears the counters to 0

    The PCOUNTER uses MMIO area 0x00a000:0x00b000 on NV10:NV40 and NV40:NVC0. On
    NVC0+, it uses 0x180000:0x1c0000.

    NV10:NVC0 PCOUNTER is unaffected by all PMC.ENABLE bits and has no interrupt
    lines. NVC0+ PCOUNTER is enabled by PMC.ENABLE bit 28 and [XXX: figure out
    interupt business]


    = MMIO registers =

    The MMIO registers are similiar among PCOUNTER revisions, but their placement
    is very different.


    == NV10 ==

    The MMIO registers for NV10:NV40 are [i is domain index]:

    00a400+i*0x100 PRE_SRC[i] - PRE input selection
    00a404+i*0x100 PRE_OP[i] - PRE logic operation
    00a408+i*0x100 START_SRC[i] - START input selection
    00a40c+i*0x100 START_OP[i] - START logic operation
    00a410+i*0x100 EVENT_SRC[i] - EVENT input selection
    00a414+i*0x100 EVENT_OP[i] - EVENT logic operation
    00a418+i*0x100 STOP_SRC[i] - STOP input selection
    00a41c+i*0x100 STOP_OP[i] - STOP logic operation
    00a420+i*0x100 SETFLAG_SRC[i] - SETFLAG input selection [NV10:NV30]
    00a424+i*0x100 SETFLAG_OP[i] - SETFLAG logic operation
    00a428+i*0x100 CLRFLAG_SRC[i] - CLRFLAG input selection [NV10:NV30]
    00a42c+i*0x100 CLRFLAG_OP[i] - CLRFLAG logic operation
    00a430+i*0x100+j*4,j<4 STATUS[i][j] - input status
    00a600+i*0x100 CTR_CYCLES[i] - elapsed cycles counter
    00a604+i*0x100 CTR_CYCLES_HI[i] - elapsed cycles counter high bits [NV10:NV30]
    00a608+i*0x100 CTR_CYCLES_ALT[i] - CYCLES copy ??? [XXX]
    00a60c+i*0x100 CTR_CYCLES_ALT_HI[i] - same, high bits [NV10:NV30]
    00a610+i*0x100 CTR_EVENT[i] - EVENT counter
    00a614+i*0x100 CTR_EVENT_HI[i] - EVENT counter high bits [NV10:NV30]
    00a618+i*0x100 CTR_START[i] - START counter
    00a61c+i*0x100 CTR_START_HI[i] - CTR_START high bits [NV10:NV30]
    00a620+i*0x100 CTR_PRE[i] - PRE counter
    00a624+i*0x100 CTR_STOP[i] - STOP counter
    00a628+i*0x100 THRESHOLD[i] - EVENT counter threshold
    00a62c+i*0x100 THRESHOLD_HI[i] - THRESHOLD high bits [NV10:NV30]
    00a630+i*0x100+(j-4)*4,4<=j<8 STATUS[i][j] - input status, second part
    00a738 QUAD_ACK_TRIGGER - used to ack counter data in quad event mode [NV30:NV40]
    00a73c CTRL - PCOUNTER control


    == NV40 ==

    The MMIO registers for NV40:NVC0 are [i is domain index]:

    00a400+i*4 PRE_SRC[i] - PRE input selection
    00a420+i*4 PRE_OP[i] - PRE logic operation
    00a440+i*4 START_SRC[i] - START input selection
    00a460+i*4 START_OP[i] - START logic operation
    00a480+i*4 EVENT_SRC[i] - EVENT input selection
    00a4a0+i*4 EVENT_OP[i] - EVENT logic operation
    00a4c0+i*4 STOP_SRC[i] - STOP input selection
    00a4e0+i*4 STOP_OP[i] - STOP logic operation
    00a500+i*4 SETFLAG_OP[i] - SETFLAG logic operation
    00a520+i*4 CLRFLAG_OP[i] - CLRFLAG logic operation
    00a540+i*4 SRC_STATUS[i] - selected inputs status
    00a560+i*4 SPEC_SRC[i] - SWAP and UNK8 input selection [NV84-]
    00a580+i*4 USER_TRIGGER[i] - triggers user-controllable signals [NVA3-]
    00a600+i*4 CTR_CYCLES[i] - elapsed cycles counter
    00a640+i*4 CTR_CYCLES_ALT[i] - CYCLES copy ??? [XXX]
    00a680+i*4 CTR_EVENT[i] - EVENT counter
    00a6a0+i*4 RECORD_ADDRESS_HIGH[i] - high 8 bits of record buffer address [NV92-]
    00a6c0+i*4 CTR_START[i] - START counter
    00a6e0+i*4 RECORD_STATUS[i] - current status and position of record buffer [NV84-]
    00a700+i*4 CTR_PRE[i] - PRE counter
    00a720+i*4 RECORD_LIMIT[i] - the highest valid address in the record buffer [NV84-]
    00a740+i*4 CTR_STOP[i] - STOP counter
    00a760+i*4 RECORD_START[i] - the starting address of the record buffer [NV84-]
    00a780+i*4 THRESHOLD[i] - EVENT counter threshold
    00a7a0 RECORD_CHAN - VM channel for record mode [NV84-]
    00a7a4 RECORD_DMA - DMA object for record mode [NV84-]
    00a7a8 GCTRL - PCOUNTER global control [shared between all domains] [NV84-]
    00a7c0+i*4 CTRL - PCOUNTER control
    00a7e0+i*4 QUAD_ACK_TRIGGER - used to ack counter data in quad event mode
    00a800+i*0x20+j*4,j<8 STATUS[i][j] - input status


    == NVC0 ==

    [XXX: write me]


    = The PCOUNTER signals =

    The raw inputs that PCOUNTER operates on are called "signals". A signal is
    a single 0/1 wire sampled on every clock. The signals come from many different
    areas of the card and represent various state information. Example signals may
    be:

     - is unit X busy? - counting 1s on this signal together with elapsed clock
       cycles will give activity percentage for given unit
     - did microcontroller X execute an instruction this cycle? - counting 1s
       will give the number of executed instructions

    The signals are grouped into so-called domains. A domain has a single base
    clock and its own counting circuitry - the counting process and counter
    registers are per-domain. Domains are further grouped into domain sets.
    Domains within a domain set can communicate to a limitted extend. NV10:NVC0
    GPUs have a single domain set, while on NVC0+ there's one domain set for each
    GPC, one for each partition, and one for all domains not associated with
    a GPC/partition.

    On NV10:NV20, there's only one domain. On NV20:NV40 there are 2 domains.
    On NV40+ there can be up to 8 domains per domain set. On all GPUs, there
    can be up to 256 signals per domain. The available signals and domains
    depend heavily on the chipset. The signals are packed tightly, so even
    a signal common to two GPUs may be at different position between them.
    The lists of known domains and signals may be found in pcounter/nv10.txt,
    pcounter/nv40.txt and pcounter/nvc0.txt


    == The STATUS registers ==

    The STATUS registers may be used to peek at the current value of each signal.

    MMIO 0x00a430 + i*0x100 + (j >> 2)*0x200 + (j&3)*4: STATUS[i][j] [NV10:NV84]
    MMIO 0x00a800 + i*0x20 + j*4: STATUS[i][j] [NV50:NVC0]
    MMIO domain_base+0x000 + j*4: STATUS[j] [NVC0+]
      Reading register #j gives current value of signals j*32..j*32+31 as bits
      0..31 of the read value. This register is per-domain [i is the domain id]
      and read-only. Only i and j values corresponding to actually present domains
      and signals are valid.


    == Trailer signals ==

    A special kind of signals is so-called "trailer signals". These signals are
    common for all domains in a domain set. The position of these signals is not
    exactly constant between the domains, but their position modulo 0x20 is
    [ie. they're at the same position inside a STATUS reg for all domains, but
    not necessarily in the same STATUS reg]. Therefore, the position of each
    trailer signal here is given as an offset from "trailer base".

    The trailer signals for NV10:NV20 are:

    base+0x1f: PCOUNTER.FLAG - the flag

    For NV20:NV40:

    base+0x1d: PGRAPH.PM_TRIGGER - the PM_TRIGGER pulse from PGRAPH
    base+0x1e: PCOUNTER.DOM[1].FLAG - the flag from domain 1
    base+0x1f: PCOUNTER.DOM[0].FLAG - the flag from domain 0

    For NV40:NVC0:

    base+0x0d: PCOUNTER.PERIODIC - the PERIODIC signal from current domain [NV84:NVC0]
    base+0x0e: PGRAPH.WRCACHE_FLUSH - the WRCACHE_FLUSH pulse from PGRAPH [NV84:NVC0]
    base+0x0f: PGRAPH.PM_TRIGGER - the PM_TRIGGER pulse from PGRAPH
    base+0x10: PCOUNTER.DOM[7].EVENT - the EVENT input from domain 7
    base+0x11: PCOUNTER.DOM[6].EVENT - the EVENT input from domain 6
    base+0x12: PCOUNTER.DOM[5].EVENT - the EVENT input from domain 5
    base+0x13: PCOUNTER.DOM[4].EVENT - the EVENT input from domain 4
    base+0x14: PCOUNTER.DOM[3].EVENT - the EVENT input from domain 3
    base+0x15: PCOUNTER.DOM[2].EVENT - the EVENT input from domain 2
    base+0x16: PCOUNTER.DOM[1].EVENT - the EVENT input from domain 1
    base+0x17: PCOUNTER.DOM[0].EVENT - the EVENT input from domain 0
    base+0x18: PCOUNTER.DOM[7].FLAG - the FLAG from domain 7
    base+0x19: PCOUNTER.DOM[6].FLAG - the FLAG from domain 6
    base+0x1a: PCOUNTER.DOM[5].FLAG - the FLAG from domain 5
    base+0x1b: PCOUNTER.DOM[4].FLAG - the FLAG from domain 4
    base+0x1c: PCOUNTER.DOM[3].FLAG - the FLAG from domain 3
    base+0x1d: PCOUNTER.DOM[2].FLAG - the FLAG from domain 2
    base+0x1e: PCOUNTER.DOM[1].FLAG - the FLAG from domain 1
    base+0x1f: PCOUNTER.DOM[0].FLAG - the FLAG from domain 0

    For NVC0+:

    base+0x1f..0x22: PCOUNTER.MAIN.???
    base+0x23..0x26: PCOUNTER.MAIN.???
    base+0x27: PCOUNTER.USER_0 - the USER_0 signal from current domain
    base+0x28: PCOUNTER.USER_1
    base+0x29: PCOUNTER.USER_2
    base+0x2a: PCOUNTER.USER_3
    base+0x2b: PGRAPH - ???
    base+0x2c: PCOUNTER.PAUSED - 1 if this domain is in the PAUSED state [XXX]
    base+0x2d: ???
    base+0x2e: PCOUNTER.PERIODIC - the PERIODIC signal from current domain
    base+0x2f: ???
    base+0x30: PCOUNTER.DOM[7].EVENT - the EVENT input from domain 7
    base+0x31: PCOUNTER.DOM[6].EVENT - the EVENT input from domain 6
    base+0x32: PCOUNTER.DOM[5].EVENT - the EVENT input from domain 5
    base+0x33: PCOUNTER.DOM[4].EVENT - the EVENT input from domain 4
    base+0x34: PCOUNTER.DOM[3].EVENT - the EVENT input from domain 3
    base+0x35: PCOUNTER.DOM[2].EVENT - the EVENT input from domain 2
    base+0x36: PCOUNTER.DOM[1].EVENT - the EVENT input from domain 1
    base+0x37: PCOUNTER.DOM[0].EVENT - the EVENT input from domain 0
    base+0x38: PCOUNTER.DOM[7].FLAG - the FLAG from domain 7
    base+0x39: PCOUNTER.DOM[6].FLAG - the FLAG from domain 6
    base+0x3a: PCOUNTER.DOM[5].FLAG - the FLAG from domain 5
    base+0x3b: PCOUNTER.DOM[4].FLAG - the FLAG from domain 4
    base+0x3c: PCOUNTER.DOM[3].FLAG - the FLAG from domain 3
    base+0x3d: PCOUNTER.DOM[2].FLAG - the FLAG from domain 2
    base+0x3e: PCOUNTER.DOM[1].FLAG - the FLAG from domain 1
    base+0x3f: PCOUNTER.DOM[0].FLAG - the FLAG from domain 0


    == The EVENT and FLAG signals ==

    The trailer signals include EVENT and FLAG signals from all domains in the same
    domain set, allowing limitted inter-domain communication. The EVENT signal is
    simply the output of the EVENT logic operation in a given domain. The FLAG
    signal is the status of the FLAG in a given domain.

    In a given domain, its own FLAG and EVENT signals are connected directly to
    the relevant sources. However, other domains' signals need to be first
    converted to the right clock domain. On NV20:NV40, this is done by a simple
    synchronizer - the state of DOM[x].FLAG signal in domain y will be the same
    as the state of FLAG in domain x as of two domain y clocks ago. While this is
    appropriate for many purposes, this means that, if the two domains don't share
    the same clock, single-clock pulses in domain x may appear as multi-clock
    pulses in domain y [if it has faster clock], or be lost entirely [if it has
    slower clock].

    On NV40+, one of two synchronization mode can be selected for signals coming
    from other domains:

     - CONTINUOUS: behaves like NV20:NV40
     - PULSE: mode converts all 0-to-1 transitions in source domain into
       single-clock pulses in destination domain

    There are two synchronization mode switches per domain. One applies to all
    incoming EVENT signals from other domains, while the other applies to all
    incoming FLAG signals. Note that the synchronization applies even between
    domains that do share a clock. However, the domain's own EVENT and FLAG signals
    aren't subject to synchronization when used inside it.


    == The PM_TRIGGER and WRCACHE_FLUSH signals ==

    [XXX: write me]


    == The USER signals ==

    On NVA3:NVC0, each domain has two "user" signals controllable directly by
    PCOUNTER's MMIO register. The signals are called USER_0 and USER_1.

    MMIO 0x00a580+i*4: USER_TRIGGER [NVA3:NVC0]
      bit 0: value for USER_0
      bit 1: value for USER_1
      bit 2: pulse mode for USER_0 - if set, will reset USER_0 to 0 one cycle
        after setting it to the value of bit 0.
      bit 3: pulse mode for USER_1
      Whenever this register is written, USER_0 signal is set to the value of bit
      0, and USER_1 is set to the value of bit 1. On the next cycle after the
      signal change, the USER signals for which the pulse mode bit is set are
      reset to 0. This register is write-only.

    On NVC0+, this number is bumped to 4, the USER_TRIGGER register is read/write,
    and the signals are now located in the trailer area.

    MMIO dombase+0x0ec: USER_TRIGGER [NVC0-]
      bits 0-3: value for USER_0..USER_3
      bits 4-7: pulse mode for USER_0..USER_3
      Works like the NVA3 USER_TRIGGER register, except it's also readable. Note
      that bits 0-3 will be auto-cleared by bits 4-7 after one cycle - bits 0-3
      of the read value correspond directly to the signals' current values.

    In effect:
      - write value = 0, pulse = any to set signal to 0 indefinitely
      - write value = 1, pulse = 0 to set signal to 1 indefinitely
      - write value = 1, pulse = 1 to set signal to 1 for one pulse only [and then set to 0 indefinitely]


    == The PERIODIC signal ==

    On NV84+, each domain has a single PERIODIC signal connected to a simple
    periodic pulse generator. The pulse generator will generate a single-clock
    '1' pulse every X clocks, with X selectable via the CTRL register from
    powers of two between 0x400 and 0x10000 clocks. The PERIODIC signal can
    also be disabled - it'll output a constant '0' signal in this case.

    The GCTRL register has a global PERIODIC_RESET bit that keeps the periodic
    generator in a reset state while it's set to 1. This bit can be used to start
    the PERIODIC signal generators synchronously for all domains.


    = Input selection =

    Each domain has up to 256 signals, but only a handful of inputs are used for
    the counting process. They are:

     - PRE, START, EVENT, STOP: created from 4 individually selected signals
       through an arbitrary 4-input logic operation, used by the counting process
     - CLRFLAG, SETFLAG: likewise created through an arbitrary 4-input logic
       operation, but on NV30+ the logic operation input signal selections are
       shared with PRE/START/EVENT/STOP inputs [NV10:NV30 have separate selections
       like the other inputs]. Used to control the FLAG.
     - SWAP [NV30-]: hardwired to PGRAPH.PM_TRIGGER on NV30:NV84, can be assigned
       to an arbitrary signal [without logic operation] on NV84+. Used by the quad
       event mode.
     - UNK8 [NV84:NVC0]: can be assigned to an arbitrary signal, also without logic
       operation. Purpose unknown [XXX]

    Starting with NV30, the SETFLAG input may also be used as an argument to the
    EVENT and STOP logic operations, allowing one to construct 7-input logic
    operations.

    The registers used to select the signals going into the logic operations
    are:

    MMIO 0x00a400+i*0x100: PRE_SRC[i] [NV10:NV40]
    MMIO 0x00a400+i*4: PRE_SRC[i] [NV40:NVC0]
    MMIO dombase+0x040: PRE_SRC [NVC0-]
      Selects the 4 signals used as inputs to PRE's logic operation.
      bits 0-7: signal 0
      bits 8-15: signal 1
      bits 16-23: signal 2
      bits 24-31: signal 3
      On NV30+, these signals are also used as inputs to CLRFLAG and SETFLAG logic
      operations.

    MMIO 0x00a408+i*0x100: START_SRC[i] [NV10:NV40]
    MMIO 0x00a440+i*4: START_SRC[i] [NV40:NVC0]
    MMIO dombase+0x048: START_SRC [NVC0-]
      Like PRE_SRC, but for START. On NV30+, these signals are also used as inputs
      to CLRFLAG and SETFLAG logic operations, and are used as a 4-bit integer
      or low 4 bits of 6-bit integer in special counter modes.

    MMIO 0x00a410+i*0x100: EVENT_SRC[i] [NV10:NV40]
    MMIO 0x00a480+i*4: EVENT_SRC[i] [NV40:NVC0]
    MMIO dombase+0x050: EVENT_SRC [NVC0-]
      Like PRE_SRC, but for EVENT. On NV40+, signals 2 and 3 are also used as high
      2 bits of a 6-bit integer in special counter modes, and signals 0 and 1 are
      used as a 2-bit integer.

    MMIO 0x00a418+i*0x100: STOP_SRC[i] [NV10:NV40]
    MMIO 0x00a4c0+i*4: STOP_SRC[i] [NV40:NVC0]
    MMIO dombase+0x058: STOP_SRC [NVC0-]
      Like PRE_SRC, but for STOP.

    MMIO 0x00a420+i*0x100: SETFLAG_SRC[i] [NV10:NV30]
      Like PRE_SRC, but for SETFLAG.

    MMIO 0x00a428+i*0x100: CLRFLAG_SRC[i] [NV10:NV30]
      Like PRE_SRC, but for CLRFLAG.

    For convenience, the status of all 16 source signals can be checked by reading
    the SRC_STATUS register on NV40+:

    MMIO 0x00a540+i*4: SRC_STATUS[i] [NV40:NVC0]
    MMIO dombase+0x068: SRC_STATUS [NVC0-]
      bits 0-3: current state of PRE_SRC signals 0-3
      bits 4-7: current state of START_SRC signals 0-3
      bits 8-11: current state of EVENT_SRC signals 0-3
      bits 12-15: current state of STOP_SRC signals 0-3

    The PRE/START/EVENT/STOP/SETFLAG/CLRFLAG input calculation goes like that:

    1. Start with the 4 signals selected by corresponding SRC register, call them
       SRC[0..3]. If on NV30+ and the input being calculated is SETFLAG/CLRFLAG,
       the SRC register doesn't exist, and SRC[0..3] are instead set to:
       - SETFLAG: START_SRC[2], START_SRC[3], PRE_SRC[0], PRE_SRC[1]
       - CLRFLAG: PRE_SRC[2], PRE_SRC[3], START_SRC[0], START_SRC[1]
    2. Initially, set ARG[0..3] to SRC[0..3]
    3. If argument 0 delay bit is set, set ARG[0] to SRC[0] as of previous clock
       cycle instead.
    4. If argument 1 delay bit is set, set ARG[1] to SRC[1] as of previous clock
       cycle instead.
    5. If on NV92+ and argument 2 SRC[0] delay replace bit is set, set ARG[2] to
       SRC[0] as of previous clock cycle instead.
    6. If on NV92+ and argument 3 SRC[1] delay replace bit is set, set ARG[3] to
       SRC[1] as of previous clock cycle instead.
    7. If on NV30+, the input being calculated is EVENT or STOP, and argument 3
       SETFLAG replace bit is set, set ARG[3] to the value of SETFLAG input
       [computed in the same clock cycle - *not* delayed]
    8. Perform the logic operation on ARG[0..3] to get the final value of the
       input. This is done as follows:
       - construct a 4-bit index i, with bit 0 set to ARG[0], bit 1 set to ARG[1],
         and so on
       - the value of the input is set to bit #i of the logic operation selector
       The logic operation selector thus effectively functions as a truth table
       for the logic operation.

    The registers selecting the actual logic operation are:

    MMIO 0x00a400+i*0x100: PRE_OP[i] [NV10:NV40]
    MMIO 0x00a400+i*4: PRE_OP[i] [NV40:NVC0]
    MMIO dombase+0x040: PRE_OP [NVC0-]
      bits 0-15: the logic operation to perform on the signals selected by PRE_SRC
      bit 16: if set, argument 0 of the logic operation is delayed by 1 clock cycle
      bit 17: if set, argument 1 of the logic operation is delayed by 1 clock cycle
      bit 18: selects argument 2 of the logic operation [NV92-]
        0: PRE_SRC[2]
        1: PRE_SRC[0] delayed by 1 clock cycle
      bit 19: selects argument 3 of the logic operation [NV92-]
        0: PRE_SRC[3]
        1: PRE_SRC[1] delayed by 1 clock cycle
      This register is special - writing it will cause a swap in quad event mode on
      NV84:NVC0, and start the single event mode counting process on NV10:NVC0.

    MMIO 0x00a408+i*0x100: START_OP[i] [NV10:NV40]
    MMIO 0x00a440+i*4: START_OP[i] [NV40:NVC0]
    MMIO dombase+0x048: START_OP [NVC0-]
      bits 0-15: the logic operation to perform on the signals selected by START_SRC
      bit 16: if set, argument 0 of the logic operation is delayed by 1 clock cycle
      bit 17: if set, argument 1 of the logic operation is delayed by 1 clock cycle
      bit 18: selects argument 2 of the logic operation [NV92-]
        0: START_SRC[2]
        1: START_SRC[0] delayed by 1 clock cycle
      bit 19: selects argument 3 of the logic operation [NV92-]
        0: START_SRC[3]
        1: START_SRC[1] delayed by 1 clock cycle

    MMIO 0x00a410+i*0x100: EVENT_OP[i] [NV10:NV40]
    MMIO 0x00a480+i*4: EVENT_OP[i] [NV40:NVC0]
    MMIO dombase+0x050: EVENT_OP [NVC0-]
      bits 0-15: the logic operation to perform on the signals selected by EVENT_SRC
      bit 16: if set, argument 0 of the logic operation is delayed by 1 clock cycle
      bit 17: if set, argument 1 of the logic operation is delayed by 1 clock cycle
      bit 18: selects argument 3 of the logic operation [NV30-]:
        0: EVENT_SRC[3] [NV30:NV92] or as selected by bit 20 [NV92-]
        1: SETFLAG
      bit 19: selects argument 2 of the logic operation [NV92-]
        0: EVENT_SRC[2]
        1: EVENT_SRC[0] delayed by 1 clock cycle
      bit 20: selects argument 3 of the logic operation, if not set to SETFLAG
        by bit 18 [NV92-]
        0: EVENT_SRC[3]
        1: EVENT_SRC[1] delayed by 1 clock cycle

    MMIO 0x00a418+i*0x100: STOP_OP[i] [NV10:NV40]
    MMIO 0x00a4c0+i*4: STOP_OP[i] [NV40:NVC0]
    MMIO dombase+0x058: STOP_OP [NVC0-]
      bits 0-15: the logic operation to perform on the signals selected by STOP_SRC
      bit 16: if set, argument 0 of the logic operation is delayed by 1 clock cycle
      bit 17: if set, argument 1 of the logic operation is delayed by 1 clock cycle
      bit 18: selects argument 3 of the logic operation [NV30-]:
        0: STOP_SRC[3] [NV30:NV92] or as selected by bit 20 [NV92-]
        1: SETFLAG
      bit 19: selects argument 2 of the logic operation [NV92-]
        0: STOP_SRC[2]
        1: STOP_SRC[0] delayed by 1 clock cycle
      bit 20: selects argument 3 of the logic operation, if not set to SETFLAG
        by bit 18 [NV92-]
        0: STOP_SRC[3]
        1: STOP_SRC[1] delayed by 1 clock cycle

    MMIO 0x00a424+i*0x100: SETFLAG_OP[i] [NV10:NV40]
    MMIO 0x00a500+i*4: SETFLAG_OP[i] [NV40:NVC0]
    MMIO dombase+0x060: SETFLAG_OP [NVC0-]
      bits 0-15: the logic operation to perform.
      bit 16: if set, argument 0 of the logic operation is delayed by 1 clock cycle
      bit 17: if set, argument 1 of the logic operation is delayed by 1 clock cycle
      bit 18: selects argument 2 of the logic operation [NV92-]
        0: PRE_SRC[0]
        1: START_SRC[2] delayed by 1 clock cycle
      bit 19: selects argument 3 of the logic operation [NV92-]
        0: PRE_SRC[1]
        1: START_SRC[3] delayed by 1 clock cycle

    MMIO 0x00a42c+i*0x100: CLRFLAG_OP[i] [NV10:NV40]
    MMIO 0x00a520+i*4: CLRFLAG_OP[i] [NV40:NVC0]
    MMIO dombase+0x064: CLRFLAG_OP [NVC0-]
      bits 0-15: the logic operation to perform. On NV10:NV30, the arguments are
        selected by SETFLAG_SRC. On NV30+, the arguments are: PRE_SRC[2],
        PRE_SRC[3], START_SRC[0], START_SRC[1].
      bit 16: if set, argument 0 of the logic operation is delayed by 1 clock cycle
      bit 17: if set, argument 1 of the logic operation is delayed by 1 clock cycle
      bit 18: selects argument 2 of the logic operation [NV92-]
        0: START_SRC[0]
        1: PRE_SRC[2] delayed by 1 clock cycle
      bit 19: selects argument 3 of the logic operation [NV92-]
        0: START_SRC[1]
        1: PRE_SRC[3] delayed by 1 clock cycle

    [XXX: check bits 16-20 on NVC0]

    The register used to select the SWAP and UNK8 inputs on NV84:NVC0 cards is:

    MMIO 0x00a560+i*4: SPEC_SRC[i] [NV84:NVC0]
      bits 0-7: the SWAP signal
      bits 8-15: the UNK8 signal

    And on NVC0+:

    MMIO dombase+0x06c: SWAP_SRC [NVC0-]
      bits 0-7: the SWAP signal

    On NV10:NVC0, writing any of the _SRC and _OP registers except PRE_OP in
    single event mode will result in the state being reset to INACTIVE. Writing
    PRE_OP will start the counting process, setting the state to WAIT_PRE.
    On NV84:NVC0 in quad event mode, writing PRE_OP will cause a swap, as if
    the SWAP input was asserted for one cycle.

    [XXX: figure out how single event mode is supposed to be used on NVC0+]


    = Counters =

    The single event mode and quad event mode use MMIO-visible counter registers.
    They are:

     - CTR_CYCLES: counts all clock cycles in a counting period
     - CTR_CYCLES_ALT: a copy of CTR_CYCLES? [XXX]
     - CTR_EVENT: counts 1s on EVENT input, or sums integers in EVENT_* special
       counter modes
     - CTR_START: in quad event mode, counts 1s on START input, or sums integers
       in EXTRA_* special counter modes; in single event mode counts measurement
       periods in which CTR_EVENT reached value >= THRESHOLD
     - CTR_PRE: in quad event mode, counts 1s on PRE input; in single event mode,
       counts down PRE assertions until WAIT_FOR_PRE state is left, then sums
       integers in EXTRA_* special counter modes and is unused otherwise.
     - CTR_STOP: in quad event mode, counts 1s on STOP input; in single event
       mode, counts down counting periods until the counting process ends.

    On NV10:NV30, the CTR_CYCLES, CTR_CYCLES_ALT, CTR_EVENT and CTR_START counters
    are 40-bit, while CTR_PRE and CTR_STOP are 32-bit. On NV30+, all counters
    are 32-bit. On NV30+, The counters are saturated - once they reach the largest
    possible value [0xffffffff], they stop incrementing. On NV10:NV30, the low 39
    bits will wrap normally, but bit 39 is sticky: that is, 0xffffffffff increments
    to 0x8000000000, while other values increment normally.

    The registers used to access the counters are:

    MMIO 0x00a600+i*0x100: CTR_CYCLES[i] [NV10:NV40]
    MMIO 0x00a600+i*4: CTR_CYCLES[i] [NV40:NVC0]
      Read-only, gives the current value of CTR_CYCLES. Returns low 32 bits
      on NV10:NV30.

    MMIO 0x00a604+i*0x100: CTR_CYCLES_HI[i] [NV10:NV30]
      Read-only, gives the high 8 bits of the current value of CTR_CYCLES.

    MMIO 0x00a608+i*0x100: CTR_CYCLES_ALT[i] [NV10:NV40]
    MMIO 0x00a640+i*4: CTR_CYCLES_ALT[i] [NV40:NVC0]
      Read-only, gives the current value of CTR_CYCLES_ALT. Returns low 32 bits
      on NV10:NV30.

    MMIO 0x00a60c+i*0x100: CTR_CYCLES_ALT_HI[i] [NV10:NV30]
      Read-only, gives the high 8 bits of the current value of CTR_CYCLES_ALT.

    MMIO 0x00a610+i*0x100: CTR_EVENT[i] [NV10:NV40]
    MMIO 0x00a680+i*4: CTR_EVENT[i] [NV40:NVC0]
      Read-only, gives the current value of CTR_EVENT. Returns low 32 bits
      on NV10:NV30.

    MMIO 0x00a614+i*0x100: CTR_EVENT_HI[i] [NV10:NV30]
      Read-only, gives the high 8 bits of the current value of CTR_EVENT.

    MMIO 0x00a618+i*0x100: CTR_START[i] [NV10:NV40]
    MMIO 0x00a6c0+i*4: CTR_START[i] [NV40:NVC0]
      Read-only, gives the current value of CTR_START. Returns low 32 bits
      on NV10:NV30.

    MMIO 0x00a61c+i*0x100: CTR_START_HI[i] [NV10:NV30]
      Read-only, gives the high 8 bits of the current value of CTR_START.

    MMIO 0x00a620+i*0x100: CTR_PRE[i] [NV10:NV40]
    MMIO 0x00a700+i*4: CTR_PRE[i] [NV40:NVC0]
      When read, gives the current value of CTR_PRE. When written, sets the
      initial CTR_PRE value for single-event mode.

    MMIO 0x00a624+i*0x100: CTR_STOP[i] [NV10:NV40]
    MMIO 0x00a740+i*4: CTR_STOP[i] [NV40:NVC0]
      When read, gives the current value of CTR_STOP. When written, sets the
      initial CTR_STOP value for single-event mode.

    The CTR_PRE and CTR_STOP counters have two values: the visible "current"
    value, and the hidden "initial" value. Reading the corresponding register
    reads the "current" value, while writing sets the "initial" value. The
    "initial" values are used when starting counting process in single event
    mode.

    Note that, in quad event mode, these registers access the copies of the
    counters from previous counting period, and the currently active counters
    are not visible.

    The record mode uses a different counting algorithm, and the counters are
    written to memory instead of being accessed directly via MMIO. The same
    underlying storage is used internally, so parts of the counter state may be
    visible via MMIO registers. This isn't particularly useful.

    [XXX: figure out what's the deal with NVC0 counters]


    == Special counter modes ==

    While the simplest way to use the counters is to have them increment by 1 every
    clock cycle when a given input is set, PCOUNTER supports a few more complex
    modes where a 4-bit, 6-bit, or 2-bit integer made of several signals is added
    to a counter on every cycle. This is used to count events which can happen
    multiple times in a single cycle - the relevant unit then exports a multi-bit
    event count, instead of simple event strobe.

    The integers used in special copunter modes are:

     - B4: 4-bit integer, made of the following signals, in low-to-high bit order:
       - START_SRC[0]
       - START_SRC[1]
       - START_SRC[2]
       - START_SRC[3]
     - B6: 6-bit integer, made of:
       - START_SRC[0]
       - START_SRC[1]
       - START_SRC[2]
       - START_SRC[3]
       - EVENT_SRC[2]
       - EVENT_SRC[3]
     - B2: 2-bit integer, made of:
       - EVENT_SRC[0]
       - EVENT_SRC[1]

    The modes are:

     - SIMPLE: CTR_EVENT is increased by 1 on every cycle when EVENT input is 1
       [ie. nothing interesting happens]
     - EVENT_B4: CTR_EVENT is increased by B4 on every cycle when EVENT input is 1
     - EVENT_B6 [NV40-]: CTR_EVENT is increased by B6 on every cycle when EVENT
       input is 1
     - EXTRA_B4 [NV40-]: CTR_EVENT behaves as in SIMPLE mode, but:
       - single event mode: CTR_PRE, instead of staying at 0 after leaving
         WAIT_FOR_PRE state, is used as a counter, and is increased by B4 on every
         clock cycle
       - quad event mode: CTR_START, instead of being controlled by START input,
         is increased by B4 on every clock cycle
     - EXTRA_B6_EVENT_B2 [NV40-]: CTR_EVENT is increased by B2 on every clock
       cycle, and:
       - single event mode: CTR_PRE behaves like in EXTRA_B4 mode, but is
         increased by B6 instead of B4 every cycle
       - quad event mode: CTR_START behaves like in EXTRA_B4 mode, bus is
         increased by B6 instead of B4 every cycle

    [XXX: figure out if there's anything new on NVC0]


    = Control registers =

    The operation of PCOUNTER is controlled by the CTRL registers. NV10:NV40 have
    a single CTRL register, shared between both domains:

    MMIO 0x00a73c: CTRL [NV10:NV40]
      bit 0: TVOUT_DEBUG_SEL - selects the signals that go to TV-out debug port,
        if enabled.
      bit 1: TVOUT_DEBUG_ENABLE - if 0, external TV encoder pins behave normally;
        if 1, the display circuitry signals are disconnected, and internal PCOUNTER
        debug pins are exposed via these pins.
      bit 2: CTR_MODE - selects counter mode [see above], affects both domains
        0: SIMPLE
        1: EVENT_B4
      bits 3-4: DOM0_SINGLE_STATE - read-only, reads as the current single event
        mode state for domain #0:
        0: INACTIVE
        1: WAIT_PRE
        2: WAIT_START
        3: COUNTING
      bits 5-6: DOM1_SINGLE_STATE [NV20:NV40] - like bits 3-4, but for domain #1
      bit 8: DOM0_EVENT_CTR_PERIOD [NV15:NV40] - EVENT_CTR_PERIOD for domain #0:
        0: ONE
        1: ALL
      bit 9: DOM1_EVENT_CTR_PERIOD [NV20:NV40] - like bit 8, but for domain #1
      bit 16: DOM0_MODE [NV30:NV40] - selects counting mode for domain #0:
        0: SINGLE - single event mode
        1: QUAD - quad event mode
      bit 18: DOM1_MODE [NV30:NV40] - like bit 16, but for domain #1
      bits 24-25: DOM0_QUAD_STATE [NV30:NV40] - read-only, reads as the current
        quad event mode state for domain #0:
        0: EMPTY
        1: VALID
        3: OVERFLOW
      bits 26-27: DOM1_QUAD_STATE [NV30:NV40] - like bits 24-25, but for domain #1

    NV40:NVC0 instead have per-domain CTRL registers:

    MMIO 0x00a7c0+i*4: CTRL[i] [NV40:NVC0]
      bits 0-1: MODE - selects counting mode
        0: SINGLE - single event mode
        1: QUAD - quad event mode
        2: RECORD - record mode
      bits 4-6: CTR_MODE - selects counter mode
        0: SIMPLE
        1: EVENT_B4
        2: EVENT_B6
        3: EXTRA_B4
        4: EXTRA_B6_EVENT_B2
      bit 8: EVENT_CTR_PERIOD - like on NV15
      bit 11: EVENT_IMPORT_MODE - selects synchronization mode for EVENT signals
        imported from other domains
        0: CONTINUOUS
        1: PULSE
      bit 13: FLAG_IMPORT_MODE - like bit 11, but for FLAG signals
      bit 16: ??? [XXX]
      bit 20: RECORD_FORMAT - selects packet format for record mode [NV84:NVC0]
        0: LONG - 32-byte packets with 12 usable event counters
        1: SHORT - 16-byte packets with 4 usable event counters
      bits 21-23: PERIODIC_PERIOD [NV84:NVC0] - selects PERIODIC signal period:
        0: disabled, PERIODIC signal is always 0
        1: 0x400 clocks
        2: 0x800 clocks
        3: 0x1000 clocks
        4: 0x2000 clocks
        5: 0x4000 clocks
        6: 0x8000 clocks
        7: 0x10000 clocks
      bits 24-25: QUAD_STATE - like on NV30
      bit 27: FAULT_CLEAR - write-only, when written as 1 clears the FAULT bit in
        RECORD_STATUS. Note, however, that the domain will still be in a wedged
        state due to [probably] a hardware bug. This bit is thus useless.
      bits 28-29: SINGLE_STATE - like on NV10
      bit 30: ??? [XXX] [NV92:NVC0]

    In addition, NV84:NVC0 have a global GCTRL register used for a few bits shared
    by all domains:

    MMIO 0x00a7a8: GCTRL [NV84:NVC0]
      bit 0: RECORD_RESET - when set to 0, record counters increment normally; when
        set, forces all record counters to 0 value
      bit 4: PERIODIC_RESET - when set to 0, PERIODIC signals operate normally;
        when set, PERIODIC signals are forced to 0 and will continue from the
        beginning of the cycle upon reenabling
    [XXX: more bits]

    [XXX: NVC0]


    = Single event mode =

    In single event mode, one event input is being monitored and counted, with
    quite complex counting period management. The inputs used by single event mode
    counting process are PRE, START, EVENT, STOP.

    The counting process may be in one of 4 states:

     - INACTIVE: nothing is happening, PCOUNTER needs to be set up
     - WAIT_FOR_PRE: counting process has started, but PRE pulses are reuired
       before it's actually possible to start a counting period
     - WAIT_FOR_START: counting process has started, a counting period is not
       currently active, but will be started on a START pulse
     - COUNTING: a counting period is currently active, and the counters are in
       use

    Counting process works like this:

    On every cycle:
        if (PCOUNTER config register other than PRE_OP written this cycle) {
            SINGLE_STATE = INACTIVE;
        }
        switch (SINGLE_STATE) {
            case INACTIVE:
                if (PRE_OP written this cycle) {
                    /* start counting process, init counters */
                    CTR_EVENT = 0;
                    CTR_START = 0;
                    CTR_CYCLES = CTR_CYCLES_ALT = 0;
                    CTR_PRE = CTR_PRE_init;
                    CTR_STOP = CTR_STOP_init;
                    FLAG = 0;
                    SINGLE_STATE = WAIT_FOR_PRE;
                }
                break;
            case WAIT_FOR_PRE:
                if (SETFLAG) FLAG = 1;
                if (CLRFLAG) FLAG = 0;
                if (PRE) {
                    if (CTR_PRE != 0) {
                        CTR_PRE--;
                    } else {
                        SINGLE_STATE = WAIT_FOR_START;
                    }
                }
                break;
            case WAIT_FOR_START:
                if (SETFLAG) FLAG = 1;
                if (CLRFLAG) FLAG = 0;
                if (START) {
                    CTR_CYCLES = CTR_CYCLES_ALT = 0;
                    if (chipset < NV15 || EVENT_CTR_PERIOD == ONE)
                        CTR_EVENT = 0;
                    SINGLE_STATE = COUNTING;
                }
                break;
            case COUNTING:
                if (SETFLAG) FLAG = 1;
                if (CLRFLAG) FLAG = 0;
                increase CTR_EVENT and maybe CTR_PRE according to
                the counter mode;
                if (STOP) {
                    if (CTR_EVENT >= THRESHOLD)
                        CTR_START++;
                    if (CTR_STOP != 0) {
                        CTR_STOP--;
                        SINGLE_STATE = WAIT_FOR_START;
                    } else {
                        SINGLE_STATE = INACTIVE;
                    }
                }
        }

    Or, in summary:
     - before actual counting, (CTR_PRE+1) 1s must happen on PRE input
     - a counting process consists of (CTR_STOP+1) counting periods
     - a counting period is started by 1 on START input and stopped by 1 on STOP
       input
     - events outside of a counting period don't count
     - if EVENT_CTR_PERIOD is ONE, CTR_EVENT effectively applies to a counting
       period, if it's ALL, it contains a sum over all counting periods.
       CTR_PRE, when EXTRA_* counter mode is in use, always contains a sum over
       all counting periods. NV10:NV15 cards don't have this submode bit and
       always behave as if it was ONE.
     - CTR_CYCLES always contains length of current [COUNTING] or last
       [WAIT_FOR_START] couting period
     - CTR_START will contain the number of counting periods that ended with
       CTR_EVENT >= THRESHOLD - probably only useful with EVENT_CTR_PERIOD = ONE.
     - writing any *_OP register except PRE_OP, any *_SRC register, any CTR
       register, THRESHOLD register, and CTRL register will abort the counting
       process
     - flag is frozen when in INACTIVE state, cleared to 0 when entering
       WAIT_FOR_PRE

    Single event mode doesn't use shadow counters - the values of all counters
    are immediately visible through MMIO registers.

    The threshold value for CTR_START counter can be set and read via the following
    registers:

    MMIO 0x00a628+i*0x100: THRESHOLD[i] [NV10:NV40]
    MMIO 0x00a780+i*4: THRESHOLD[i] [NV40:NVC0]
      The THRESHOLD value, or low 32 bits of THRESHOLD value on NV10:NV30.

    MMIO 0x00a62c+i*0x100: THRESHOLD_HI[i] [NV10:NV30]
      The high 8 bits of THRESHOLD value.

    [XXX: threshold on NVC0]


    = Quad event mode =

    In quad event mode, 4 different event inputs are counted, each in a dedicated
    counter. The events are counted in invisible "shadow" registers, while the
    visible registers contain the final values of counters from previous counting
    period. Counting periods are controlled by the special SWAP input, which
    copies the "shadow" counters to visible registers, and clears the shadow
    counters to 0. In addition, the SWAP signal marks the counter values as
    available in the CONTROL register.

    The counters used in quad event mode are:

     - CTR_CYCLES and CTR_CYCLES_ALT: increases by 1 for every cycle
     - CTR_EVENT: increases as per the counter mode, usually by 1 for every cycle
       when EVENT input is set
     - CTR_START: increases as per the counter mode, usually by 1 for every cycle
       when START input is set
     - CTR_PRE: increases by 1 for every cycle when PRE input is set
     - CTR_STOP: increases by 1 for every cycle when STOP input is set

    When in quad event mode, the counters are always active - there's no INACTIVE
    state like in single event mode.

    The counter swap is triggered on every cycle when SWAP input is set. On
    NV84:NVC0, the counter swap is also triggered on every write to the PRE_OP
    register. The PCOUNTER keeps track of how many counter value sets have been
    swapped and how many have been read. It can thus be in one of the three states:

     - EMPTY - no new counter values to read
     - VALID - swap has happened and counter values are available for reading
     - OVERFLOW - another swap has happened while in VALID state, and counter
       values were lost

    A swap bumps the state up one unit - EMPTY goes to VALID, VALID goes to
    OVERFLOW, and OVERFLOW is unchanged.

    Note that the swap is performed before updating the counters for a given
    cycle - thus if SWAP and one of the event inputs are active on the same cycle,
    the events will be counted for the *next* period.

    The software may inform the PCOUNTER of read completion by poking the
    write-only QUAD_ACK_TRIGGER register. The register is shared for all domains
    on NV30:NV40, and per-domain for NV40+:

    MMIO 0x00a738: QUAD_ACK_TRIGGER [NV30:NV40]
      bit 0: DOM0 - when written as 0, nothing happens. When written as 1, the
        status of domain #1 is bumped down one unit - VALID goes to EMPTY, OVERFLOW
        goes to VALID, and EMPTY is unchanged.
      bit 8: DOM1 - like DOM0, but affects domain #1

    MMIO 0x00a7e0+i*4: QUAD_ACK_TRIGGER[i] [NV40:NVC0]
    MMIO dombase+0x0a0: QUAD_ACK_TRIGGER [NVC0-]
      bit 0: Like NV30's DOM0/DOM1 bits, affects the domain the register is in.


    = Record mode =

    In record mode, counter values are written to memory for later analysis
    instead of being read via MMIO - this enables much more frequent sampling
    and simplifies software. The counter values are written to a given virtual
    memory buffer in 16-byte or 32-byte packets, consisting of 14 counters.
    A new packet is written whenever one of the 12 event counters is close to
    overflowing, or when the STOP input is asserted. The counters are:

     - 48-bit cycles counter, incremented by 1 on every cycle, cleared only when
       record mode operation is started by writting the RECORD_START register or
       GCTRL.RECORD_RESET is set to 1. This counter wraps on overflow.
     - 12 16-bit event counters, corresponding to 12 monitored signals selected by
       PRE_SRC[0..3], START_SRC[0..3], EVENT_SRC[0..3]. Incremented by 1 on every
       cycle when corresponding signal is 1. Cleared after writing a packet.
       A packet write is triggered whenever any of these counters reaches 0xf000.
       If a counter reaches 0xffff, it stops incrementing further.
     - 12-bit STOP counter, incremented by 1 whenever the STOP input is 1. Cleared
       after writing a packet. A packet write is triggered whenever this counter
       is non-0. If this counter reaches 0xfff, it stops incrementing further.

    There are two packet formats available: long and short. Long format packets
    are 32 bytes long and include all counters, while short format paackets are
    16 bytes long and have only 4 of the 12 event counters. A packet in long
    format is made of 16 16-bit little endian words:

    0x00: low 16 bits of cycle counter
    0x02: middle 16 bits of cycle counter
    0x04: high 16 bits of cycle counter
    0x06:
      bits 0-11: the STOP counter
      bits 12-15: always 0
    0x08: PRE_SRC[0] event counter
    0x0a: PRE_SRC[1] event counter
    0x0c: PRE_SRC[2] event counter
    0x0e: PRE_SRC[3] event counter
    0x10: START_SRC[0] event counter
    0x12: START_SRC[1] event counter
    0x14: START_SRC[2] event counter
    0x16: START_SRC[3] event counter
    0x18: EVENT_SRC[0] event counter
    0x1a: EVENT_SRC[1] event counter
    0x1c: EVENT_SRC[2] event counter
    0x1e: EVENT_SRC[3] event counter

    A packet in short format is simply the first 16 bytes of a packet in long
    format.

    Packets are normally written to memory when STOP input is asserted. For this
    reason, packets in memory will usually have the STOP counter equal to 1 [for
    the one pulse that triggered them]. However, to avoid saturating the event
    counters, a packet write will also be triggered whenever any event counter
    is >= 0xf000. The STOP counter in the memory packet will be equal to 0 in this
    case. STOP counter values greater than 1 are possible when STOP input is
    asserted too often for the memory interface to keep up - each domain has place
    for one outgoing packet. Whenever a packet write is triggered and there isn't
    an outgoing packet yet, the packet will be sent, and the counters reset. When
    a packet write is triggered and there already is an outgoing packet, nothing
    will happen - the counters will just keep incrementing until the current
    packet write is finished.

    [XXX: check if still valid on NVC0]


    == Record mode setup ==

    Before record mode is started, a few registers need to be set up.

    First, the channel and DMA object for the record buffer need to be bound. The
    PCOUNTER will access virtual memory as engine 0xb, client 0xf, DMA slot 0.
    The channel and DMA object are global for all domains. Note that the channel
    register has to be written *after* the DMA object register for a successful
    bind.

    MMIO 0x00a7a4: RECORD_DMA [NV84:NVC0]
      bits 0-15: the DMA object to be used by PCOUNTER. Writing this register only
        stores the DMA object, it doesn't actually bind it - the bind is done by
        RECORD_CHAN write.

    MMIO 0x00a7a0: RECORD_CHAN [NV84:NVC0]
      bits 0-29: CHAN - the channel to bind to PCOUNTER engine
      bit 31: VALID - if set, a channel bind and DMA object bind will be done when
        writing this register. If unset, the register will be written, but no
        binds will be done.

    The address of the record buffer is settable per-domain:

    MMIO 0x00a760+i*4: RECORD_START [NV84:NVC0]
      The start address of the record buffer. Only bits 4-31 are valid - the
      buffer has to be aligned to 16 byte bounduary. When this register is
      written, the address is copied to RECORD_STATUS position field, the "buffer
      valid" internal flag will be set, and all counters are reset if the domain
      is in record mode.

    Note that setting this register will not properly clear the counter state if
    the domain is not in record mode - in fact, a bogus packet will likely be
    written immediately after transitioning to the record mode if RECORD_START
    is written in another mode. To avoid that, write RECORD_START after entering
    record mode [and make sure the "buffer valid" flag is not set], or use the
    GCTRL.RECORD_RESET bit.

    MMIO 0x00a720+i*4: RECORD_LIMIT [NV84:NVC0]
      The last valid address in the record buffer. Only bits 4-31 are valid. After
      a packet is written with address >= the value of this register, the internal
      "buffer valid" flag will be cleared, and all further writes will be ignored
      until RECORD_START is written.

    Note that one packet write will always succeed before the limit hit flag is
    set and further writes are disabled - even if the position is set far beyond
    the limit.

    MMIO 0x00a6e0+i*4: RECORD_STATUS [NV84:NVC0]
      This register is read-only.
      bit 0: if set, a VM FAULT happened when writing the record buffer
      bits 4-31: bits 4-31 of the current record buffer position, ie. address of
        the next packet to be written

    The PCOUNTER internally operates on 32-bit addresses. On NV84:NV92, the high
    8 bits of 40-bit virtual address are always forced to 0, limitting the record
    buffer to low 4GB of the VM space. On NV92+, the high 8 bits of the address
    are instead taken from a register:

    MMIO 0x00a6a0+i*4: RECORD_ADDRESS_HIGH [NV92:NVC0]
      Sets the high 8 bits of the record buffer virtual address.

    Note, however, that the internal address size is still 32-bit: the position
    will thus wrap at 4GB bounduary, instead of incrementing bit 32 of address.
    For this reason, record buffers that cross a 4GB block bounduary in virtual
    space cannot be used.

    Note that VM faults on the record buffer will permanently hang the faulting
    domain until the GPU is reset - while there's a "clear VM FAULT status" bit
    in the control register, it only clears the status bit, while hardware is
    still in a wedged state. This is likely a hardware bug.

    [XXX: figure out record mode setup for NVC0]


    = The flag =

    The FLAG is a single per-domain bit that can be set and cleared via the
    SETFLAG and CLRFLAG inputs. On every clock cycle:

     - if CLRFLAG is 1, the FLAG is set to 0
     - if SETFLAG is 1 and CLRFLAG is 0, the FLAG is set to 1
     - if both CLRFLAG and SETFLAG are 0, the FLAG is unchanged

    In addition, when in single-event mode, the FLAG is frozen [will not respond
    to CLRFLAG/SETFLAG] when in INACTIVE state, and will be cleared to 0 when going
    to WAIT_FOR_PRE state.

    The current value of the FLAG is available as a common trailer signal to all
    domains in the same domain set, allowing complex operations to be performed.
    Note however that the effect of CLRFLAG/SETFLAG on the FLAG signal is delayed
    by 2 clock cycles - if the SETFLAG input becomes 1 on cycle X, the FLAG signal
    will become 1 on cycle X+2.
