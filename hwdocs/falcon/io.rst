.. _falcon-io:

========
IO space
========

.. contents::


Introduction
============

Every falcon engine has an associated IO space. The space consists of 32-bit IO
registers, and is accessible in two ways:

- host access by MMIO areas in BAR0
- falcon access by io* instructions

The IO space contains control registers for the microprocessor itself,
interrupt and timer setup, code/data space access ports, PFIFO communication
registers, as well as registers for the engine-specific hardware that falcon
is meant to control.

The addresses are different between falcon and host. From falcon POV, the IO space
is word-addressable 0x40000-byte space. However, most registers are duplicated
64 times: bits 2-7 of the address are ignored. The few registers that don't
ignore these bits are called "indexed" registers. From host POV, the falcon IO
space is a 0x1000-byte window in BAR0. Its base address is engine-dependent.
First 0xf00 bytes of this window are tied to the falcon IO space, while last 0x100
bytes contain several host-only registers. On nva3-d9, host mmio address
falcon_base + X is directed to falcon IO space address X << 6 | HOST_IO_INDEX << 2.
On nvd9+, some engines stopped using the indexed accesses. On those,
host mmio address falcon_base + X is directed to falcon IO space address X.
HOST_IO_INDEX is specified in the host-only MMIO register falcon_base + 0xffc:

MMIO 0xffc: HOST_IO_INDEX
  bits 0-5: selects bits 2-7 of the falcon IO space when accessed from host.

Unaligned accesses to the IO space are unsupported, both from host and falcon.
Low 2 bits of addresses should be 0 at all times.

.. todo:: document v4 new addressing


.. _falcon-io-common:

Common IO register list
=======================

===== ======= ============ ================= ===========
Host  Falcon  Present on   Name              Description
===== ======= ============ ================= ===========
0x000 0x00000 all units    INTR_SET          :ref:`trigger interrupt <falcon-io-intr>`
0x004 0x00100 all units    INTR_CLEAR        :ref:`clear interrupt <falcon-io-intr>`
0x008 0x00200 all units    INTR              :ref:`interrupt status <falcon-io-intr>`
0x00c 0x00300 v3+ units    INTR_MODE         :ref:`interrupt edge/level <falcon-io-intr-mode>`
0x010 0x00400 all units    INTR_EN_SET       :ref:`interrupt enable set <falcon-io-intr-enable>`
0x014 0x00500 all units    INTR_EN_CLR       :ref:`interrupt enable clear <falcon-io-intr-enable>`
0x018 0x00600 all units    INTR_EN           :ref:`interrupt enable status <falcon-io-intr-enable>`
0x01c 0x00700 all units    INTR_DISPATCH     :ref:`interrupt routing <falcon-io-intr-route>`
0x020 0x00800 all units    PERIODIC_PERIOD   :ref:`periodic timer period <falcon-io-periodic>`
0x024 0x00900 all units    PERIODIC_TIME     :ref:`periodic timer counter <falcon-io-periodic>`
0x028 0x00a00 all units    PERIODIC_ENABLE   :ref:`periodic interrupt enable <falcon-io-periodic>`
0x02c 0x00b00 all units    TIME_LOW          :ref:`PTIMER time low <falcon-io-ptimer>`
0x030 0x00c00 all units    TIME_HIGH         :ref:`PTIMER time high <falcon-io-ptimer>`
0x034 0x00d00 all units    WATCHDOG_TIME     :ref:`watchdog timer counter <falcon-io-watchdog>`
0x038 0x00e00 all units    WATCHDOG_ENABLE   :ref:`watchdog interrupt enable <falcon-io-watchdog>`
0x040 0x01000 all units    SCRATCH0          :ref:`scratch register <falcon-io-scratch>`
0x044 0x01100 all units    SCRATCH1          :ref:`scratch register <falcon-io-scratch>`
0x048 0x01200 all units    FIFO_ENABLE       :ref:`PFIFO access enable <falcon-io-fifo-enable>`
0x04c 0x01300 all units    STATUS            busy/idle status        [falcon/io.txt]
0x050 0x01400 all units    CHANNEL_CUR       :ref:`current PFIFO channel <falcon-io-channel>`
0x054 0x01500 all units    CHANNEL_NEXT      :ref:`next PFIFO channel <falcon-io-channel>`
0x058 0x01600 all units    CHANNEL_CMD       :ref:`PFIFO channel control <falcon-io-channel>`
0x05c 0x01700 all units    STATUS_MASK       busy/idle status mask?  [falcon/io.txt]
0x060 0x01800 all units    ???               ???
0x064 0x01900 all units    FIFO_DATA         :ref:`FIFO command data <falcon-io-fifo>`
0x068 0x01a00 all units    FIFO_CMD          :ref:`FIFO command <falcon-io-fifo>`
0x06c 0x01b00 v4+ units    ???               :ref:`FIFO ??? <falcon-io-fifo>`
0x070 0x01c00 all units    FIFO_OCCUPIED     :ref:`FIFO commands available <falcon-io-fifo>`
0x074 0x01d00 all units    FIFO_ACK          :ref:`FIFO command ack <falcon-io-fifo>`
0x078 0x01e00 all units    FIFO_LIMIT        :ref:`FIFO size <falcon-io-fifo>`
0x07c 0x01f00 all units    SUBENGINE_RESET   reset subengines        [falcon/io.txt]
0x080 0x02000 all units    SCRATCH2          :ref:`scratch register <falcon-io-scratch>`
0x084 0x02100 all units    SCRATCH3          :ref:`scratch register <falcon-io-scratch>`
0x088 0x02200 all units    PM_TRIGGER        :ref:`perfmon triggers <falcon-io-perf-user>`
0x08c 0x02300 all units    PM_MODE           :ref:`perfmon signal mode <falcon-io-perf-user>`
0x090 0x02400 all units    ???               ???
0x094 0x02500 v3+ units    ???               ???
0x098 0x02600 v3+ units    BREAKPOINT[0]     :ref:`code breakpoint <falcon-io-breakpoint>`
0x09c 0x02700 v3+ units    BREAKPOINT[1]     :ref:`code breakpoint <falcon-io-breakpoint>`
0x0a0 0x02800 v3+ units    ???               ???
0x0a4 0x02900 v3+ units    ???               ???
0x0a8 0x02a00 v4+ units    PM_SEL            perfmon signal select   [falcon/perf.txt]
0x0ac 0x02b00 v4+ units    HOST_IO_INDEX     IO space index for host [falcon/io.txt] [XXX: doc]
0x100 0x04000 all units    UC_CTRL           microprocessor control  [falcon/proc.txt]
0x104 0x04100 all units    UC_ENTRY          microcode entry point   [falcon/proc.txt]
0x108 0x04200 all units    UC_CAPS           microprocessor caps     [falcon/proc.txt]
0x10c 0x04300 all units    UC_BLOCK_ON_FIFO  microprocessor block    [falcon/proc.txt]
0x110 0x04400 all units    XFER_EXT_BASE     :ref:`xfer external base <falcon-io-xfer>`
0x114 0x04500 all units    XFER_FALCON_ADDR  :ref:`xfer falcon address <falcon-io-xfer>`
0x118 0x04600 all units    XFER_CTRL         :ref:`xfer control <falcon-io-xfer>`
0x11c 0x04700 all units    XFER_EXT_ADDR     :ref:`xfer external offset <falcon-io-xfer>`
0x120 0x04800 all units    XFER_STATUS       :ref:`xfer status <falcon-io-xfer-status>`
0x124 0x04900 crypto units CX_STATUS         crypt xfer status       [falcon/crypt.txt]
0x128 0x04a00 v3+ units    UC_STATUS         microprocessor status   [falcon/proc.txt]
0x12c 0x04b00 v3+ units    UC_CAPS2          microprocessor caps     [falcon/proc.txt]
0x140 0x05000 v3+ units    TLB_CMD           :ref:`code VM command <falcon-io-tlb>`
0x144 0x05100 v3+ units    TLB_CMD_RES       :ref:`code VM command result <falcon-io-tlb>`
0x148 0x05200 v4+ units    ???               ???
0x14c 0x05300 v4+ units    ???               ???
0x150 0x05400 UNK31 units  ???               ???
0x154 0x05500 UNK31 units  ???               ???
0x158 0x05600 UNK31 units  ???               ???
0x160 0x05800 UAS units    UAS_IO_WINDOW     UAS I[] space window    [falcon/data.txt]
0x164 0x05900 UAS units    UAS_CONFIG        UAS configuration       [falcon/data.txt]
0x168 0x05a00 UAS units    UAS_FAULT_ADDR    UAS MMIO fault address  [falcon/data.txt]
0x16c 0x05b00 UAS units    UAS_FAULT_STATUS  UAS MMIO fault status   [falcon/data.txt]
0x180 0x06000 v3+ units    CODE_INDEX        :ref:`code access window addr <falcon-io-code>`
0x184 0x06100 v3+ units    CODE              :ref:`code access window <falcon-io-code>`
0x188 0x06200 v3+ units    CODE_VIRT_ADDR    :ref:`code access virt addr <falcon-io-code>`
0x1c0 0x07000 v3+ units    DATA_INDEX[0]     :ref:`data access window addr <falcon-io-data>`
0x1c4 0x07100 v3+ units    DATA[0]           :ref:`data access window <falcon-io-data>`
0x1c8 0x07200 v3+ units    DATA_INDEX[1]     :ref:`data access window addr <falcon-io-data>`
0x1cc 0x07300 v3+ units    DATA[1]           :ref:`data access window <falcon-io-data>`
0x1d0 0x07400 v3+ units    DATA_INDEX[2]     :ref:`data access window addr <falcon-io-data>`
0x1d4 0x07500 v3+ units    DATA[2]           :ref:`data access window <falcon-io-data>`
0x1d8 0x07600 v3+ units    DATA_INDEX[3]     :ref:`data access window addr <falcon-io-data>`
0x1dc 0x07700 v3+ units    DATA[3]           :ref:`data access window <falcon-io-data>`
0x1e0 0x07800 v3+ units    DATA_INDEX[4]     :ref:`data access window addr <falcon-io-data>`
0x1e4 0x07900 v3+ units    DATA[4]           :ref:`data access window <falcon-io-data>`
0x1e8 0x07a00 v3+ units    DATA_INDEX[5]     :ref:`data access window addr <falcon-io-data>`
0x1ec 0x07b00 v3+ units    DATA[5]           :ref:`data access window <falcon-io-data>`
0x1f0 0x07c00 v3+ units    DATA_INDEX[6]     :ref:`data access window addr <falcon-io-data>`
0x1f4 0x07d00 v3+ units    DATA[6]           :ref:`data access window <falcon-io-data>`
0x1f8 0x07e00 v3+ units    DATA_INDEX[7]     :ref:`data access window addr <falcon-io-data>`
0x1fc 0x07f00 v3+ units    DATA[7]           :ref:`data access window <falcon-io-data>`
0x200 0x08000 v4+ units    DEBUG_CMD         debuging command        [falcon/debug.txt]
0x204 0x08100 v4+ units    DEBUG_ADDR        address for DEBUG_CMD   [falcon/debug.txt]
0x208 0x08200 v4+ units    DEBUG_DATA_WR     debug data to write     [falcon/debug.txt]
0x20c 0x08300 v4+ units    DEBUG_DATA_RD     debug data last read    [falcon/debug.txt]
0xfe8 \-      NVC0- v3     PM_SEL            perfmon signal select        [falcon/perf.txt]
0xfec \-      v0, v3       UC_SP             microprocessor $sp reg        [falcon/proc.txt]
0xff0 \-      v0, v3       UC_PC             microprocessor $pc reg        [falcon/proc.txt]
0xff4 \-      v0, v3       UPLOAD            :ref:`old code/data upload <falcon-io-upload>`
0xff8 \-      v0, v3       UPLOAD_ADDR       :ref:`old code/data up addr <falcon-io-upload>`
0xffc \-      v0, v3       HOST_IO_INDEX     IO space index for host        [falcon/io.txt]
===== ======= ============ ================= ===========

.. todo:: list incomplete for v4

Registers starting from 0x400/0x10000 are engine-specific and described in engine
documentation.


.. _falcon-io-scratch:

Scratch registers
=================

MMIO 0x040 / I[0x01000]: SCRATCH0
MMIO 0x044 / I[0x01100]: SCRATCH1
MMIO 0x080 / I[0x02000]: SCRATCH2
MMIO 0x084 / I[0x02100]: SCRATCH3
  Scratch 32-bit registers, meant for host <-> falcon communication.


.. _falcon-status:

Engine status and control registers
===================================

MMIO 0x04c / I[0x01300]: STATUS
  Status of various parts of the engine. For each bit, 1 means busy, 0 means
  idle.
  bit 0: UC. Microcode. 1 if microcode is running and not on a sleep insn.
  bit 1: ???
  Further bits are engine-specific.

MMIO 0x05c / I[0x01700]: STATUS_MASK
  A bitmask of nonexistent status bits. Each of bits 0-15 is set to 0 if
  corresponding STATUS line is tied to anything in this particular engine, 1
  if it's unused. [?]

.. todo:: clean. fix. write. move.

MMIO 0x07c / I[0x01f00]: SUBENGINE_RESET
  When written with value 1, resets all subengines that this falcon engine
  controls - that is, everything in IO space addresses 0x10000:0x20000. Note
  that this includes the memory interface - using this register while an xfer
  is in progress is ill-advised.


.. _falcon-io-upload:

v0 code/data upload registers
=============================

MMIO 0xff4: UPLOAD
  The data to upload, see below
MMIO 0xff8: UPLOAD_ADDR
  bits 2-15: bits 2-15 of the code/data address being uploaded.
  bit 20: target segment. 0 means data, 1 means code.
  bit 21: readback.
  bit 24: xfer busy [RO]
  bit 28: secret flag - secret engines only [see falcon/crypt.txt]
  bit 29: code busy [RO]

This pair of registers can be used on v0 to read/write code and data
segments. It's quite fragile and should only be used when no xfers are active.
bit 24 of UPLOAD_ADDR is set when this is the case. On v3+, this pair is
broken and should be avoided in favor of the new-style access via
:ref:`CODE <falcon-io-code>` and :ref:`DATA <falcon-io-data>` ports.

To write data, poke address to UPLOAD_ADDR, then poke the data words to
UPLOAD. The address will auto-increment as words are uploaded.

To read data or code, poke address + readback flag to UPLOAD_ADDR, then read
the word from UPLOAD. This only works for a single word, and you need to poke
UPLOAD_ADDR again for each subsequent word.

The code segment is organised in 0x100-byte pages. On secretful engines, each
page can be secret or not. Reading from secret pages doesn't work and you just
get 0. Writing code segment can only be done in aligned page units.

To write a code page, write start address of the page + secret flag [if
needed] to UPLOAD_ADDR, then poke multiple of 0x40 words to UPLOAD. The
address will autoincrement. The process cannot be interrupted except between
pages. The "code busy" flag in UPLOAD_ADDR will be lit when this is the case.


.. _falcon-isa-iowr:

IO space writes: iowr, iowrs
============================

Writes a word to IO space. iowr does asynchronous writes [queues the write,
but doesn't wait for completion], iowrs does synchronous write [write is
guaranteed to complete before executing next instruction]. On v0 cards,
iowrs doesn't exist and synchronisation can instead be done by re-reading
the relevant register.

Instructions:
    ===== ============================ ========== =========
    Name  Description                  Present on Subopcode
    ===== ============================ ========== =========
    iowr  Asynchronous IO space write  all units  0
    iowrs Synchronous IO space write   v3+ units  1
    ===== ============================ ========== =========
Instruction class:
    unsized
Operands:
    BASE, IDX, SRC
Forms:
    ========== =========
    Form       Subopcode
    ========== =========
    R2, I8, R1 d0
    R2, 0, R1  fa
    ========== =========
Immediates:
    zero-extended
Operation:
    ::

        if (op == iowr)
                IOWR(BASE + IDX * 4, SRC);
        else
                IOWRS(BASE + IDX * 4, SRC);


.. _falcon-isa-iord:

IO space reads: iord
====================

Reads a word from IO space.

Instructions:
    ===== ============================ ========== =========
    Name  Description                  Present on Subopcode
    ===== ============================ ========== =========
    ???   ???                          v3+ units  e
    iord  IO space read                all units  f
    ===== ============================ ========== =========
Instruction class:
    unsized
Operands:
    DST, BASE, IDX
Forms:
    ========== =========
    Form       Subopcode
    ========== =========
    R1, R2, I8 c0
    R3, R2, R1 ff
    ========== =========
Immediates:
    zero-extended
Operation:
    ::

        if (op == iord)
                DST = IORD(BASE + IDX * 4);
        else
                ???;

.. todo:: subop e
