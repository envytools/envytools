.. _nv01-peeprom:

======================================
PEEPROM: NV01 configuration/key EEPROM
======================================

.. contents::


Introduction
============

NV01 cards, in addition to the usual BIOS ROM, have a small Microwire EEPROM
on board. The EEPROM is organised as 128 8-bit cells. It is used to store
an unique chip ID as well as driver data. The chipset has a Microwire master
controller and can natively read and write the EEPROM.

The user interface to the controller is known as PEEPROM and its MMIO range is
0x60a000:0x60b0000. It is not affected by any PMC.ENABLE bit.

Note that only addresses 0x10-0x7f can be accessed by PEEPROM - others are
reserved for NV01's internal operation and PEEPROM will refuse to touch them.
They will always read as 0.

The reserved EEPROM range stores a 64-bit unique chip ID. The MMIO range used
to read this id is known as PCHIPID and located at 0x605000:0x606000. Like
PEEPROM, it's not affected by any PMC.ENABLE bit.

For details on the EEPROM itself, see the datasheet for Microchip 93C46A.

.. todo:: figure out what else is stored in the EEPROM, if anything.
.. todo:: figure out *how* the chip ID is stored in the EEPROM.
.. todo:: figure out wtf the chip ID is used for


.. _peeprom-mmio:
.. _pchipid-mmio:

MMIO registers
==============

The MMIO registers in PCHIPID range are:

======== ===== =============
Address  Name  Description
======== ===== =============
0x605400 ID[0] :ref:`low 32 bits of ID <pchipid-mmio-id>`
0x605404 ID[1] :ref:`high 32 bits of ID <pchipid-mmio-id>`
======== ===== =============

There is only one MMIO register in PEEPROM range:

======== ===== =============
Address  Name  Description
======== ===== =============
0x60a400 PORT  :ref:`EEPROM port control <peeprom-mmio-port>`
======== ===== =============


.. _peeprom-mmio-port:

EEPROM access
=============

The EEPROM is accessed by reading and writing the PORT register:

MMIO 0x60a400: PORT
  - bits 0-7: DATA - the data read or written
  - bits 8-14: ADDR - the address to read or write
  - bit 24: WRITE_TRIGGER - when written as 1, will cause DATA to be written
    to the EEPROM at address ADDR. Does *not* auto-clear.
  - bit 25: READ_TRIGGER - when written as 1, will cause a byte to be read
    from address ADDR into DATA. Does *not* auto-clear.
  - bit 28: BUSY [RO] - reads as 1 when an EEPROM operation is in progress.

To read a byte:

- read PORT until BUSY is 0
- write PORT with ADDR set to the address to read, READ_TRIGGER set to 1,
  other fields set to 0
- read PORT until BUSY is 0
- the DATA field now contains the byte that was read

To write a byte:

- read PORT until BUSY is 0
- write PORT with ADDR set to the address to write, DATA set to the byte to
  write, WRITE_TRIGGER set to 1, other fields set to 0
- if a wait for completion is desired, read PORT until BUSY is 0


.. _pchipid-mmio-id:

Chip ID access
==============

The chip ID can be read through the ID registers:

MMIO 0x605400 + i*4, i < 2: ID[i]
  Reads low 32 bits [i == 0] or high 32 bits [i == 1] of the chip ID.
