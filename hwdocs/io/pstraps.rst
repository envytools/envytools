.. _pstraps:

====================================
PSTRAPS: straps readout and override
====================================

.. contents::


Introduction
============

The nvidia GPU chips are used in multiple cards from multiple manufacturers.
Thus, the a single GPU can end up in many different configurations, with
varying memory amount, memory type, bus type, TV norm, crystal frequency, and
many other parameters. Since the GPU often needs to know what configuration
it is used in, a "straps" mechanism was invented to tell it this information.
On the first few cycles after reset, the memory bus pins are sampled. Since
nothing else is driving them at that point, their logic state is decided by
the pull-up or pull-down resistors placed by board manufacturer. The value
this read is used as the "straps" value and is used to configure many aspects
of GPU operation. Some of the straps are not used by the GPU itself, but are
intended for use by the BIOS or the driver.

NV01 has 5 straps bits, NV03 has 10 bits. NV04 bumped this count to 16 straps
bits, and allowed the driver to override the straps value at runtime. NV11
bumped the count to 22 bits, NV20 bumped it to 31 bits.

NV18 introduced a second set of 31 straps, and added another override
mechanism. Each straps set now consists of 2 31-bit values: primary and
secondary, and a 31-bit "select" mask that chooses which value is the source
of a given bit in the "effective" value used internally by the card. Both
primary and secondary value and select mask are set on reset from the BIOS
in ROM, but can be modified later. Straps 0 select, straps 0 secondary,
straps 1 select and straps 1 secondary are loaded from BIOS ROM offsets
0x58, 0x5c, 0x60, 0x64, respectively.

NVD9 introduced a third set.

When effective straps value changes for any reason, the changed value starts
being used by the card immediately.

The straps can be read or overriden by accessing the PSTRAPS area of MMIO,
which resides at addresses 0x608000-0x608fff [NV01] or 0x101000-0x101fff
[NV03+] in BAR0. It's also known as PEXTDEV by nvidia.

On NV03:NV17, PSTRAPS is enabled by PMC.ENABLE bit 20 [PFB], on later cards
and on NV01 it's unaffected by :ref:`PMC.ENABLE <pmc-enable>`.

On NV03:NV04 cards, the PSTRAPS area is also home to an unrelated register
controlling ROM timings.


.. _pstraps-mmio:

MMIO register list
==================

NV01 registers:

======== ====== =============
Address  Name   Description
======== ====== =============
0x608000 STRAPS :ref:`straps value <pstraps-mmio-nv01-straps>`
======== ====== =============

NV03+ registers:

no annotation - available on all NV03+ cards
[1] - available on NV03:NV04 only
[2] - available on NV18:NV20 and NV25+ only
[3] - available on NVD9+ only

======== =============== ================= ================
Address  Present on      Name              Description
======== =============== ================= ================
0x101000 all             STRAPS0_PRIMARY   :ref:`straps set 0 primary value <pstraps-mmio-nv03-straps>`
0x101004 NV18:NV20 NV25- STRAPS0_SELECT    :ref:`straps set 0 select mask <pstraps-mmio-nv03-straps>`
0x101008 NV18:NV20 NV25- STRAPS0_SECONDARY :ref:`straps set 0 secondary value <pstraps-mmio-nv03-straps>`
0x10100c NV18:NV20 NV25- STRAPS1_PRIMARY   :ref:`straps set 1 primary value <pstraps-mmio-nv03-straps>`
0x101010 NV18:NV20 NV25- STRAPS1_SELECT    :ref:`straps set 1 select mask <pstraps-mmio-nv03-straps>`
0x101014 NV18:NV20 NV25- STRAPS1_SECONDARY :ref:`straps set 1 secondary value <pstraps-mmio-nv03-straps>`
0x101028 NVD9-           ???               RO 0
0x10102c NVD9-           ???               RO 0
0x101030 NVD9-           ???               RW mask ff
0x101034 NVD9-           STRAPS2_PRIMARY   :ref:`straps set 2 primary value <pstraps-mmio-nv03-straps>`
0x101038 NVD9-           STRAPS2_SELECT    :ref:`straps set 2 select mask <pstraps-mmio-nv03-straps>`
0x10103c NVD9-           STRAPS2_SECONDARY :ref:`straps set 2 secondary value <pstraps-mmio-nv03-straps>`
0x101040 NVD9-           ???               RO 0
0x101200 NV03:NV04       ROM_TIMINGS       :ref:`ROM timing configuration <prom-mmio-rom-timings>`
======== =============== ================= ================

.. todo:: 0x101028, 0x10102c, 0x101030, 0x101040


.. _pstraps-mmio-nv01-straps:

NV01 straps
===========

On NV01, all straps bits are available in a single register:

MMIO 0x608000: STRAPS
  - bits 0-1: memory type
    - 0 - VRAM
    - 3 - DRAM
  - bits 2-3: board type
    - 0 - motherboard
    - 1 - adapter #1 [normal add-on cards use this value]
    - 2 - adapter #2
    - 3 - adapter #3
  - bit 4: bus type
    - 0 - PCI
    - 1 - VESA local bus


.. _pstraps-mmio-nv03-straps:

NV03+ straps readout/override registers
=======================================

MMIO 0x101000: STRAPS0_PRIMARY
MMIO 0x10100c: STRAPS1_PRIMARY [NV18:NV20 and NV25+ only]
MMIO 0x101034: STRAPS2_PRIMARY [NVD9+ only]
  - bits 0-30: straps primary value
  - bit 31: override enable [NV04+ only]

When writing, if bit 31 is 0, override is disabled, and the straps register
is restored to the original straps as read by the card on reset. If bit 31
is 1, override is enabled, and the straps value is set to the value written
by host.

MMIO 0x101004: STRAPS0_SELECT [NV18:NV20 and NV25+ only]
MMIO 0x101010: STRAPS1_SELECT [NV18:NV20 and NV25+ only]
MMIO 0x101038: STRAPS2_SELECT [NVD9+ only]
  - bits 0-30: strap source selection for strap bit X

When corresponding bit is set to 1, the card takes its value from the main
straps value, when corresponding bit is set to 0, the card takes its value from
the secondary value. This register is always writable and not affected by
override enable.

MMIO 0x101008: STRAPS0_SECONDARY [NV18:NV20 and NV25+ only]
MMIO 0x101014: STRAPS1_SECONDARY [NV18:NV20 and NV25+ only]
MMIO 0x10103c: STRAPS2_SECONDARY [NVD9+ only]
  - bits 0-30: straps secondary value

This register is always writable and not affected by override enable.


NV03 family straps
==================

- bit 0: if set, PCI 66MHz mode is supported
- bit 1: if 0, this GPU is part of a motherboard and ROMless, subsystem device
  id will be initialised to 0x00000000 and should be written with a valid
  value by system bios. if 1, this is a standalone card and has ROM -
  subsystem will be read from 32-bit LE word at address 0x54 in the ROM
- bits 2-3: [original NV03 only]: memory type, apparently useless
- bit 2 [NV03T]: memory type, apparently useless
- bit 3 [NV03T]: if 0, no Power Management capability is exposed and GPU uses
  pci id 0x0018, if 1 Power Management capability exposed and GPU uses
  pci id 0x0019
- bit 4: ram width: 0 64-bit, 1 128-bit. Apparently useless.
- bit 5: host bus type: 0 PCI, 1 AGP
- bit 6: crystal frequency: 0 - 13.500MHz, 1 - 14.31818MHz
- bits 7-8: TV mode: 0 - no TV encoder, 1 - NTSC TV encoder present, 2 - PAL TV
  encoder present
- bit 9 [original NV03]: PCI version: 0 PCI 2.0, 1 PCI 2.1.
- bit 9 [NV03T]: if set, AGP x2 is supported


NV04-NV40 families straps
=========================

Set 0:

- bit 0: if 0, PCI AD lines have reversed polarity, if 1 normal
- bit 1: if 0, this GPU is part of a motherboard and ROMless, subsystem device
  id will be initialised to 0x00000000 and should be written with a valid
  value by system bios. if 1, this is a standalone card and has ROM -
  subsystem will be read from 32-bit LE word at address 0x54 in the ROM.
  Same applies to select/secondary values.
- bits 2-5: RAM config, for use by BIOS
- bit 6: crystal type bit 0
- bits 7-8: TV mode: 0 - SECAM, 1 - NTSC, 2 - PAL, 3 - disabled
- bit 9: if 1, AGP x4 disabled [PCI/AGP cards only]
- bit 10: if 1, AGP side band addressing disabled [PCI/AGP cards only]
- bit 11: if 1, AGP fast writes is disabled [PCI/AGP cards only]
- bits 12-13: DEVICE_ID bits 0-1
- bit 14: bus type, 0 - PCI, 1 - AGP [PCI/AGP cards only]
- bit 15: flat panel interface width: 0 - 12 bits, 1 - 24 bits
- bits 16-17 [NV20:NV25 only]: BAR1 size

  - 0: 64MB
  - 1: 128MB
  - 2: 256MB
  - 3: 512MB

- bit 18 [NV20:NV25 only]: BAR0 size [XXX: I'm almost sure it does something else too]

  - 0: 16MB
  - 1: 128MB

- bits 16-19 [NV17:NV20 and NV25:NV50]: flat panel config [used to select entry from fp mode table]
- bits 20-21: DEVICE_ID bits 2-3 [NV17:NV20 and NV25:NV50]
- bit 22: crystal type bit 1 [NV17:NV20 and NV25:NV50]
- bits 23-24 [NV17:NV20 and NV25:NV50]: BAR1 size

  - 0: 64MB
  - 1: 128MB
  - 2: 256MB
  - 3: 512MB

- bit 25 [NV17:NV20 and NV25:NV50]: BAR0 size [XXX: I'm almost sure it does something else too]

  - 0: 16MB
  - 1: 128MB

- bits 26-28: ?
- bits 29-30 [NV17:NV20 and NV25:NV50]: bios ROM type

  - 0: parallel
  - 1: serial [SPI]
  - 2: ???

Crystal type is:

- 0 - 13.500MHz
- 1 - 14.31818MHz
- 2 - 27.000MHz
- 3 - 25.000MHz

Set 1:

- bit 0: enables OHCI 1394 controller on PCI function 1 [NV18 only]
- bits 1-3: ?
- bit 4: pci device class

  - 0: 0x030200 [3d controller]
  - 1: 0x030000 [vga controller]

- bits 5-30: ?


NV50 and NVC0 families straps sets
==================================

Set 0:

- bit 0: ?
- bit 1: if 0, this GPU is part of a motherboard and ROMless, subsystem device
  id will be initialised to 0x00000000 and should be written with a valid
  value by system bios. if 1, this is a standalone card and has ROM -
  subsystem will be read from 32-bit LE word at address 0x54 in the ROM.
  Same applies to select/secondary values.
- bits 2-5: RAM config, for use by BIOS
- bit 6: crystal type

  - 0: 27MHz
  - 1: 25MHz

- bits 7-9: ?
- bits 10-13: DEVICE_ID, bits 0-3
- bits 14-15: BAR1 size, part 1
- bits 16-21: ?
- bits 22-23: bios ROM type

  - 0: parallel
  - 1: serial [SPI]
  - 2: ???

- bits 24-27: flat panel config [used to select entry from fp mode table]
- bit 28: DEVICE_ID bit 4 [NV92-]
- bits 29-30: ?

Set 1:

- bits 0-3: ?
- bit 4: pci device class

  - 0: 0x030200 [3d controller]
  - 1: 0x030000 [vga controller]

- bits 5-15: ?
- bit 16: BAR5 enable
- bits 17-19: BAR0 size

  - 0 16MB
  - 1 32MB
  - 2 64MB
  - 3 128MB
  - 4 256MB
  - 5 512MB
  - 6 1GB
  - 7 2GB

- bits 20-22 BAR1 size, part2
- bit 23: BAR3 size
  - 0 BAR0 size * 2
  - 1 BAR0 size
- 24-30: ?

For BAR1 size, the two parts are summed, and BAR1 size is computed as follows:

- 0 64MB
- 1 128MB
- 2 256MB
- 3 512MB
- 4 1GB
- 5 2GB
- 6 4GB
- 7 8GB
- 8 16GB
- 9 32GB
- 10 64GB
