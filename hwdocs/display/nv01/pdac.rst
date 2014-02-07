.. _nv01-pdac:

===========================================
PDAC: NV01 DAC and external devices control
===========================================

.. contents::


Introduction
============

On NV01 cards, many display and IO tasks are handled by a separate
SGS DAC chip. This chip's registers are accessed through card's
main MMIO range. Its tasks are:

- generating video, memory, and audio clocks
- converting pixel data in memory to analog VGA signals
- handling the joystick port
- handling the saturn ports


The MMIO registers
==================

.. space:: 8 nv01-pdac 0x1000 DAC control
   0x00 PAL_WRITE nv01-pdac-pal-write
   0x04 PAL_DATA nv01-pdac-pal-data
   0x08 PAL_MASK nv01-pdac-pal-mask
   0x0c PAL_READ nv01-pdac-pal-read
   0x10 INDEX_LOW nv01-pdac-index-low
   0x14 INDEX_HIGH nv01-pdac-index-high
   0x18 DATA nv01-pdac-data
   0x1c GAME_PORT nv01-pdac-game-port

   This range contains DAC registers mapped directly to the MMIO space. Note
   that DAC is connected to NV01 by an 8-bit bus, so all these registers are
   in fact effectively 8-bit (with high 24 bits ignored on write, returned as
   0 on reads).


The DAC registers
=================

The inner DAC registers are accessed by the following MMIO registers:

.. reg:: 8 nv01-pdac-index-low indirect DAC register index, low part

   .. todo:: write me

.. reg:: 8 nv01-pdac-index-high indirect DAC register index, high part

   .. todo:: write me

.. reg:: 8 nv01-pdac-data indirect DAC register data

   .. todo:: write me

The registers are:

.. space:: 8 nv01-dac 0x10000 -
   0x0004 CONFIG_0 nv01-dac-config-0
   0x0008 PAL_INDEX nv01-dac-pal-index
   0x0009 PAL_STATE nv01-dac-pal-state
   0x000a PAL_RED nv01-dac-pal-red
   0x000b PAL_GREEN nv01-dac-pal-green

   .. todo:: write me

.. todo:: write me


Clocks
======

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

.. reg:: 8 nv01-dac-pal-index Current palette index

   Stores the current read/write index. Read only.

.. reg:: 8 nv01-dac-pal-state Palette state

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

.. reg:: 8 nv01-dac-pal-red Palette inflight red value

   Stores the current red value. Read only.

.. reg:: 8 nv01-dac-pal-green Palette inflight green value

   Stores the current green value. Read only.

The palette access registers are:

.. reg:: 8 nv01-pdac-pal-write Palette write index

   When written, sets the current mode to write, sets the current index
   to the written value, and sets the current color to red.

   When read, returns the current index.

.. reg:: 8 nv01-pdac-pal-read Palette read index

   When written, sets the current mode to read, sets the current index
   to the written value + 1, and sets the current color to red. When read,
   returns the current index.

   The behavior on reads depends on value of :obj:`nv01-dac-config-0` bit 4.
   If it's to INDEX, the current index is returned. Otherwise, returns
   the current mode in low 2 bits (same values as in CURRENT_MODE), junk
   in high 6 bits.

.. reg:: 8 nv01-pdac-pal-data Palette data

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

.. reg:: 8 nv01-pdac-pal-mask Palette index mask

   Stores the palette index mask.


DAC config
==========

.. todo:: write me

.. reg:: 8 nv01-dac-config-0 Configuration 0

   - bit 4: PAL_READ_READ, selects :obj:`nv01-pdac-pal-read` value returned on
     reads

     - 0: INDEX, current index will be returned
     - 1: MODE, current mode will be returned (like on VGA)

   .. todo:: write me


Joystick
========

.. reg:: 8 nv01-pdac-game-port ISA-like game port

   .. todo:: write me

.. todo:: write me


Saturn ports
============

.. todo:: write me
