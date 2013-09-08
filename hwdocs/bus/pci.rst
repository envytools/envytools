.. _pci:

=======================
PCI configuration space
=======================

.. contents::

.. todo:: nuke this file and write a better one - it sucks.


Introduction
============

.. todo:: write me


.. _ppci-mmio:
.. _ppci-hda-mmio:

MMIO registers
==============

.. todo:: write me


.. _ppci-intr:

Interrupts
==========

.. todo:: write me


.. _pbus-mmio-pci:

PCI/PCIE configuration space registers
======================================

- 0x00-0x3f: PCI configuration header, type 0
- 0x40-0x43: subsystem ID. Aliases standard PCI register 0x2c, but is writable.
- 0x44-0x4f: PCI AGP capability [AGP cards only]
- 0x50-0x53: ROM shadow enable flag - 0 to disable ROM shadow, disable video
  output, and use EEPROM for PCI ROM; 1 to enable ROM shadow and
  video output, using shadow copy in VRAM for PCI ROM. On NV04:NV50
  cards, enabling shadowing additionally disables PROM read/write
  circuitry.
 
.. todo:: wrong on NV03]
.. todo:: this register and possibly some others doesn't get written when
	  poked through actual PCI config accesses - PBUS writes work fine

- 0x54-0x57: VGA legacy IO/memory ranges decoding enable - 1 to enable, 0 to
  disable
- 0x60-0x67: PCI power management capability
- 0x68-0x77: PCI MSI capability - 64-bit addressing, no mask [PCIE cards and NV40+ IGPs only]
- 0x78-0x8b: PCI express capability - express endpoint [PCIE cards only]

.. todo:: NV40 has something at 0x98
.. todo:: NVAA, NVAC, NVAF stolen memory regs at 0xf4+

And for PCIE cards only:

- 0x100-0x127: PCIE virtual channel capability
- 0x128-0x137: PCIE power budgeting capability

.. todo:: very incomplete

All registers introduced by nvidia [ie. not in standard PCI config header or
capabilities] are 32-bit LE words.

On NV01:NV50 cards, PCI config space, or first 0x100 bytes of PCIE config
space, are also mapped to MMIO register space at addresses 0x1800-0x18ff.
On NV40+ cards, all 0x1000 bytes of PCIE config space are mapped to MMIO
register space at addresses 0x88000-0x88fff. It's a bad idea to access config
space addresses >= 0x100 on NV40/NV45/NV4A.

All NV01:NV40 cards, as well as NV40, NV45, NV4A are natively PCI/AGP devices,
all other cards are natively PCIE devices. Pre-NV40 IGPs are connected through
an internal AGP bus and are considered AGP devices, while NV40+ IGPs are
connected by northbridge-internal interfaces and are *not* considered PCIE
devices, thus lack the capability and extended config space. Some native AGP
cards have PCIE variants, which consist of the PCIE GPU and a thin bridge
between the GPU and the AGP bus, called BR02. In the same way, native PCIE
cards have AGP variants. On these devices, the BAR0 accesses will touch the
underlying GPU's config space, but real PCI config cycles will be intercepted
by the bridge, which will hide tha native bus type capability and expose the
other one, and show its own pciid instead of the GPU's.

.. todo:: is that all?

Note that bus master functionality may need to be enabled for NV50+ VM
circuitry to work even when only VRAM is being accessed. The reason for this
is currently unknown.

.. todo:: find it
