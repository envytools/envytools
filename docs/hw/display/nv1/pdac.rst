.. _nv1-pdac:

==========================================
PDAC: NV1 DAC and external devices control
==========================================

.. contents::


Introduction
============

On NV1 cards, many display and IO tasks are handled by a separate
SGS DAC chip. This chip's registers are accessed through card's
main MMIO range. Its tasks are:

- generating video, memory, and audio clocks
- converting pixel data in memory to analog VGA signals
- handling the joystick port
- handling the saturn ports


The MMIO registers
==================

.. space:: 8 nv1-pdac 0x1000 DAC control
   0x00 PAL_WRITE nv1-pdac-pal-write
   0x04 PAL_DATA nv1-pdac-pal-data
   0x08 PAL_MASK nv1-pdac-pal-mask
   0x0c PAL_READ nv1-pdac-pal-read
   0x10 INDEX_LOW nv1-pdac-index-low
   0x14 INDEX_HIGH nv1-pdac-index-high
   0x18 DATA nv1-pdac-data
   0x1c GAME_PORT nv1-pdac-game-port

   This range contains DAC registers mapped directly to the MMIO space. Note
   that DAC is connected to NV1 by an 8-bit bus, so all these registers are
   in fact effectively 8-bit (with high 24 bits ignored on write, returned as
   0 on reads).

   This range is always active.


The DAC registers
=================

The DAC has a lot of registers, and only a handful are available directly in
the MMIO space. Most of its registers are so-called "inner DAC registers",
are selected by a 16-bit index, and can be accessed by indexed data port in
MMIO space:

.. reg:: 32 nv1-pdac-index-low indirect DAC register index, low part

   Stores low 8 bits of the register index to be accessed.

.. reg:: 32 nv1-pdac-index-high indirect DAC register index, high part

   Stores high 8 bits of the register index to be accessed.

.. reg:: 32 nv1-pdac-data indirect DAC register data

   All accesses to this register are forwarded to :obj:`nv1-dac` register
   selected by the above two index registers. After every access, the index
   stored in the index registers is incremented by 1 (with proper carry
   between high and low part).

The inner DAC registers are:

.. space:: 8 nv1-dac 0x10000 -
   0x0000 VENDOR_ID nv1-dac-vendor-id
   0x0001 DEVICE_ID nv1-dac-device-id
   0x0002 REVISION_ID nv1-dac-revision-id
   0x0004 CONFIG_0 nv1-dac-config-0
   0x0005 CONFIG_1 nv1-dac-config-1
   0x0006 DDCIN nv1-dac-ddcin
   0x0008 PAL_INDEX nv1-dac-pal-index
   0x0009 PAL_STATE nv1-dac-pal-state
   0x000a PAL_RED nv1-dac-pal-red
   0x000b PAL_GREEN nv1-dac-pal-green
   0x000c POWERDOWN_0 nv1-dac-powerdown-0
   0x000d POWERDOWN_1 nv1-dac-powerdown-1
   0x000e POWERDOWN_2 nv1-dac-powerdown-2
   0x0010[nv1-dac-clock/4] PLL_M nv1-dac-pll-m
   0x0011[nv1-dac-clock/4] PLL_N nv1-dac-pll-n
   0x0012[nv1-dac-clock/4] PLL_O nv1-dac-pll-o
   0x0013[nv1-dac-clock/4] PLL_P nv1-dac-pll-p
   0x0300[2/2] SATURN_PORT_DATA nv1-dac-saturn-port-data
   0x0301[2/2] SATURN_PORT_MODE nv1-dac-saturn-port-mode

   .. todo:: regs 0x1c-0xff
   .. todo:: regs 0x1xx and 0x5xx
   .. todo:: regs 0xf0xx


DAC identification
==================

The DAC can be identified by reading the 3 ID registers:

.. reg:: 8 nv1-dac-vendor-id Vendor ID

   The DAC vendor ID:

   - 0x44: SGS

   Read-only.

.. reg:: 8 nv1-dac-device-id Device ID

   The DAC device ID:

   - 0x32: STG1732
   - 0x64: STG1764

   Read-only.

.. reg:: 8 nv1-dac-revision-id Revision ID

   The DAC revision ID. No idea about the values, mine is 0xb2 [STG1764].

   Read-only.


Powerdown registers
===================

Parts of DAC functionality can be powered down when not used via powerdown
registers:

.. reg:: 8 nv1-dac-powerdown-0 Subunit powerdown 0

   - bits 0-2: ???
   - bit 3: ??? powered down by default by BIOS
   - bits 4-6: ???
   - bit 7: ??? powered down by default by BIOS

   .. todo:: RE me

.. reg:: 8 nv1-dac-powerdown-1 Subunit powerdown 1

   - bit 0: MPLL - powering that down will permanently hang the card
   - bit 1: VPLL
   - bit 2: APLL
   - bit 3: CRYSTAL - powering that down isn't a good idea either
   - bits 4-7: ???

   .. todo:: RE me

.. reg:: 8 nv1-dac-powerdown-2 Subunit powerdown 2

   - bits 0-3: ???

   .. todo:: RE me


Clocks
======

The DAC contains 3 PLLs, corresponding to the three clocks that NV1 uses:

- 0: MEMORY, used to control memory and PGRAPH operations
- 1: AUDIO, used to control PAUDIO operations
- 2: VIDEO, used to control scanout

Each PLL is controlled by 4 DAC registers:

.. reg:: 8 nv1-dac-pll-m PLL M parameter

   .. todo:: write me

.. reg:: 8 nv1-dac-pll-n PLL N parameter

   .. todo:: write me

.. reg:: 8 nv1-dac-pll-o PLL O parameter

   - bits 0-3: ???

   .. todo:: write me

.. reg:: 8 nv1-dac-pll-p PLL P parameter

   - bits 0-3: ???

   .. todo:: write me

.. todo:: write me


Palette
=======

The DAC contains two palettes. Each palette consists of 256 entries. Each
palette entry consists of three 8-bit values, one for each color.

Two palettes are present for VGA emulation: If a 16-color mode is in use,
BIOS can bind palette 0 to the access registers, and palette 1 to display:
user will be able to modify palette 0, and BIOS will periodically translate
it into palette 1 taking into account the ATC palette remap registers.

The palette is accessed through 3 registers, which behave like VGA palette
access registers.

The palette access circuitry has the following state:

- 8-bit current read/write index
- current mode: read or write
- current red and green value, 8-bit each
- current color: red, green, or blue

The state is stored in the following internal DAC registers:

.. reg:: 8 nv1-dac-pal-index Current palette index

   Stores the current read/write index. Read only.

.. reg:: 8 nv1-dac-pal-state Palette state

   - bits 0-2: CURRENT_COLOR, read only, one of:

     - 1: RED
     - 2: GREEN
     - 4: BLUE

   - bit 3: SELECT, selects which palette is accessed by the access register

   - bits 4-5: CURRENT_MODE, read only, one of:

     - 0: WRITE
     - 3: READ

   - bit 6: DISPLAY_SELECT, selects which palette is accessed by display
     pipeline

   - bit 7: WIDTH, selects whether palette values are passed as-is, or
     converted from/to 6-bit format, one of:

     - 0: FULL, values are passed as-is
     - 1: VGA, all values written to palette cells will be shifted left by
       2 bits, and all values read from palette cells will be shifted right
       by 2 bits, to simulate 6-bit palette cells as used on VGA

.. reg:: 8 nv1-dac-pal-red Palette inflight red value

   Stores the current red value. Read only.

.. reg:: 8 nv1-dac-pal-green Palette inflight green value

   Stores the current green value. Read only.

The palette access registers are:

.. reg:: 32 nv1-pdac-pal-write Palette write index

   When written, sets the current mode to write, sets the current index
   to the written value, and sets the current color to red.

   When read, returns the current index.

.. reg:: 32 nv1-pdac-pal-read Palette read index

   When written, sets the current mode to read, sets the current index
   to the written value + 1, and sets the current color to red. When read,
   returns the current index.

   The behavior on reads depends on value of :obj:`nv1-dac-config-0` bit 4.
   If it's to INDEX, the current index is returned. Otherwise, returns
   the current mode in low 2 bits (same values as in CURRENT_MODE), junk
   in high 6 bits.

.. reg:: 32 nv1-pdac-pal-data Palette data

   When written: If the current color is red or green, store the value as the
   current value for the corresponding color. Otherwise, write the palette
   entry selected by the current index with the current red and green values,
   and the written value as the blue value.

   When read: read entry (CURRENT_INDEX-1) of palette and return the color
   selected by current color.

   After both read and write, the current color is cycled to the next one
   (red -> green -> blue -> red). If blue -> red transition happens, current
   index is increased by one.

Like on VGA, whenever the display pipeline needs a color index looked up, it
is first ANDed together with the value of the palette index mask register:

.. reg:: 32 nv1-pdac-pal-mask Palette index mask

   Stores the palette index mask. This register is set to 0xff on DAC reset.


DAC config
==========

.. todo:: write me

.. reg:: 8 nv1-dac-config-0 Configuration 0

   - bits 0-3: ???

   - bit 4: PAL_READ_READ, selects :obj:`nv1-pdac-pal-read` value returned on
     reads

     - 0: INDEX, current index will be returned
     - 1: MODE, current mode will be returned (like on VGA)

   - bits 5-6: ???

   .. todo:: write me

.. reg:: 8 nv1-dac-config-1 Configuration 1

   - bits 0-4: ???
   - bit 5: ??? writing as 1 causes register to reset to 0
   - bit 6: ???
   - bit 7: ???, read-only, toggling randomly

   .. todo:: write me


DDC input
=========

The DAC supports DDC1 input. DDC1 protocol, as opposed to modern I2C-based
DDC2 protocol, is fully unidirectional. The monitor continuously sends the
entire EDID block in an endless cycle on the ID1 pin, clocked by the VSYNC
signal from card. On NV1, the ID1 line is connected to DDCIN pin on the DAC.
The raw state of this line is exposed directly as a DAC register:

.. reg:: 8 nv1-dac-ddcin DDC1 input

   - bit 0: Current state of the DDCIN line, read-only

To quickly read the EDID block, software can do bit-banging on the VSYNC line
via :obj:`nv1-pfb-power-sync` register.


Joystick
========

.. reg:: 32 nv1-pdac-game-port ISA-like game port

   .. todo:: write me

.. todo:: write me


Saturn ports
============

The saturn ports are controlled by simple GPIO:

.. reg:: 8 nv1-dac-saturn-port-data Saturn port data

   - bits 0-6: state of relevant saturn port pin. Read only if configured
     as input, read-write if configured as output.

.. reg:: 8 nv1-dac-saturn-port-mode Saturn port mode

   - bits 0-6: mode of relevant saturn port pin:

     - 0: OUTPUT
     - 1: INPUT

The bit assignments are:

- 0: DATA[0]
- 1: DATA[1]
- 2: DATA[2]
- 3: DATA[3]
- 4: SENSE
- 5: SELECT[1]
- 6: SELECT[0]

.. todo:: some newer DACs have more functionality?
