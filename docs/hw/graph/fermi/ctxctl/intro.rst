=============================
Fermi context switching units
=============================

.. todo:: convert

Present on:
    cc0:
        GF100:GK104
    cc1:
        GK104:GK208
    cc2:
        GK208:GM107
    cc3:
        GM107:
BAR0 address:
    HUB:
        0x409000
    GPC:
        0x502000 + idx * 0x8000
PMC interrupt line: ???
PMC enable bit: 12 [all of PGRAPH]
Version:
    cc0, cc1:
        3
    cc2, cc3:
        5
Code segment size:
    HUB cc0: 0x4000
    HUB cc1, cc2: 0x5000
    HUB cc3: 0x6000
    GPC cc0: 0x2000
    GPC cc1, cc2: 0x2800
    GPC cc3: 0x3800
Data segment size:
    HUB: 0x1000
    GPC cc0-cc2: 0x800
    GPC cc3: 0xc00
Fifo size:
    HUB cc0-cc1: 0x10
    HUB cc2-cc3: 0x8
    GPC cc0-cc1: 0x8
    GPC cc2-cc3: 0x4
Xfer slots:
    8
Secretful:
    no
Code TLB index bits:
    8
Code ports:
    1
Data ports:
    cc0, cc1:
        1
    cc2, cc3:
        4
IO addressing type:
    indexed
Core clock:
    HUB:
        hub clock [GF100 clock #9]
    GPC:
        GPC clock [GF100 clock #0] [XXX: divider]

The IO register ranges:

400/10000:500/14000 CC		misc CTXCTL support	[graph/gf100-ctxctl/intro.txt]
500/14000:600/18000 FIFO	command FIFO submission	[graph/gf100-ctxctl/intro.txt]
600/18000:700/1c000 MC		PGRAPH master control	[graph/gf100-ctxctl/intro.txt]
700/1c000:800/20000 MMIO	MMIO bus access		[graph/gf100-ctxctl/mmio.txt]
800/20000:900/24000 MISC	misc/unknown stuff	[graph/gf100-ctxctl/intro.txt]
900/24000:a00/28000 STRAND	context strand control	[graph/gf100-ctxctl/strand.txt]
a00/28000:b00/2c000 MEMIF	memory interface	[graph/gf100-ctxctl/memif.txt]
b00/2c000:c00/30000 CSREQ	PFIFO switch requests	[graph/gf100-ctxctl/intro.txt]
c00/30000:d00/34000 GRAPH	PGRAPH status/control	[graph/gf100-ctxctl/intro.txt]
d80/36000:dc0/37000 ???		??? - related to MEMIF? [XXX] [GK104-]

Registers in CC range:
400/10000  INTR			interrupt signals
404/101xx  INTR_ROUTE		falcon interrupt routing
40c/1030x  BAR_REQMASK[0]	barrier required bits
410/1040x  BAR_REQMASK[1]	barrier required bits
414/1050x  BAR			barrier state
418/10600  BAR_SET[0]		set barrier bits, barrier 0
41c/10700  BAR_SET[1]		set barrier bits, barrier 1
420/10800  IDLE_STATUS		CTXCTL subunit idle status
424/10900  USER_BUSY		user busy flag
430/10c00  WATCHDOG		watchdog timer
484/12100H ???			[XXX]

Registers in FIFO range:
500/14000  DATA			FIFO command argument
504/14100  CMD			FIFO command submission

Registers in MC range:
604/18100H HUB_UNITS		PART/GPC count
608/18200G GPC_UNITS		TPC/ZCULL count
60c/18300H ???			[XXX]
610/18400H ???			[XXX]
614/18500  RED_SWITCH		enable/power/pause master control
618/18600G GPCID		the id of containing GPC
620/18800  UC_CAPS		falcon code and data size
698/1a600G ???			[XXX]
69c/1a700G ???			[XXX]

Registers in MISC range:
800/20000:820/20800  SCRATCH		scratch registers
820/20000:820/21000  SCRATCH_SET	set bits in scratch registers
840/20000:820/21800  SCRATCH_CLEAR	clear bits in scratch registers
86c/21b00  ???			related to strands? [XXX]
870/21c00  ???			[XXX]
874/21d00  ???			[XXX]
878/21e00  ???			[XXX]
880/22000  STRANDS		strand count
884/22100  ???			[XXX]
890/22400  ???			JOE? [XXX]
894/22500  ???			JOE? [XXX]
898/22600  ???			JOE? [XXX]
89c/22700  ???			JOE? [XXX]
8a0/22800  ???			[XXX]
8a4/22900  ???			[XXX]
8a8/22a00  ???			[XXX]
8b0/22c00  ???			[XXX] [GK104-]
8b4/22d00  ???			[XXX] [GK104-]

Registers in CSREQ range:
b00/2c000H CHAN_CUR		current channel
b04/2c100H CHAN_NEXT		next channel
b08/2c200H INTR_EN		interrupt enable?
b0c/2c300H INTR			interrupt
b80/2e000H ???			[XXX]
b84/2e100H ???			[XXX]

Registers in GRAPH range:
c00/30000H CMD_STATUS		some PGRAPH status bits?
c08/30200H CMD_TRIGGER		triggers misc commands to PGRAPH?
c14/305xxH INTR_UP_ROUTE	upstream interrupt routing
c18/30600H INTR_UP_STATUS	upstream interrupt status
c1c/30700H INTR_UP_SET		upstream interrupt trigger
c20/30800H INTR_UP_CLEAR	upstream interrupt clear
c24/30900H INTR_UP_ENABLE	upstream interrupt enable [XXX: more bits on GK104]
c80/32000G VSTATUS_0		subunit verbose status
c84/32100G VSTATUS_1		subunit verbose status
c88/32200G VSTATUS_2		subunit verbose status
c8c/32300G VSTATUS_3		subunit verbose status
c90/32400G TRAP			GPC trap status
c94/32500G TRAP_EN		GPC trap enable

Interrupts:
 0-7: standard falcon intterrupts
 8-15: controlled by INTR_ROUTE


[XXX: IO regs]
[XXX: interrupts]
[XXX: status bits]

[XXX: describe CTXCTL]


Signals
=======

0x00-0x1f: engine dependent [XXX]
0x20: ZERO - always 0
0x21: ??? - bit 9 of reg 0x128 of corresponding IBUS piece [XXX]
0x22: STRAND - strand busy executing command [graph/gf100-ctxctl/strand.txt]
0x23: ???, affected by RED_SWITCH [XXX]
0x24: IB_UNK40, last state of IB_UNK40 bit, from DISPATCH.SUBCH reg
0x25: MMCTX - MMIO transfer complete [graph/gf100-ctxctl/mmio.txt]
0x26: MMIO_RD - MMIO read complete [graph/gf100-ctxctl/mmio.txt]
0x27: MMIO_WRS - MMIO synchronous write complete [graph/gf100-ctxctl/mmio.txt]
0x28: BAR_0 - barrier #0 reached [see below]
0x29: BAR_1 - barrier #1 reached [see below]
0x2a: ??? - related to PCOUNTER [XXX]
0x2b: WATCHDOG - watchdog timer expired [see below]
0x2c: ??? - related to MEMIF [XXX]
0x2d: ??? - related to MEMIF [XXX]
0x2e: ??? - related to MEMIF [XXX]
