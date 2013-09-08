.. _bars:

=============================================
PCI BARs and other means of accessing the GPU
=============================================

.. contents::



Nvidia GPU BARs, IO ports, and memory areas
===========================================

The nvidia GPUs expose the following areas to the outside world through PCI:

- PCI configuration space / PCIE extended configuration space
- MMIO registers: BAR0 - memory, 0x1000000 bytes or more depending on card type
- VRAM aperture: BAR1 - memory, 0x1000000 bytes or more depending on card type [NV03+ only]
- indirect memory access IO ports: BAR2 - 0x100 bytes of IO port space [NV03 only]
- ???: BAR2 [only NV1x IGPs?]
- RAMIN aperture: BAR2 or BAR3 - memory, 0x1000000 bytes or more depending on card type [NV40+]
- indirect memory access IO ports: BAR5 - 0x80 bytes of IO port space [NV50+]
- PCI ROM aperture
- PCI INTA interrupt line
- legacy VGA IO ports: 0x3b0-0x3bb and 0x3c0-0x3df [can be disabled in PCI config]
- legacy VGA memory: 0xa0000-0xbffff [can be disabled in PCI config]



PCI/PCIE configuration space
============================

Nvidia GPUs, like all PCI devices, have PCI configuration space. Its contents are
described in :ref:`pci`.


BAR0: MMIO registers
====================

This is the main control space of the card - all engines are controlled
through it, and it contains alternate means to access most of the other
spaces. This, along with the VRAM / RAMIN apertures, is everything that's
needed to fully control the cards.

This space is a 16MB area of memory sparsely populated with areas representing
individual engines, which in turn are sparsely populated with registers. The
list of engines depends on card type. While there are no known registers
outside 16MB range, the BAR itself can have a larger size on NV40+ cards if
configured so by :ref:`straps <pstraps>`.

Its address is set up through PCI BAR 0. The BAR uses 32-bit addressing and
is non-prefetchable memory.

The registers inside this BAR are 32-bit, with the exception of areas that are
aliases of the byte-oriented VGA legacy IO ports. They should be accessed
through aligned 32-bit memory reads/writes. On pre-NV11 cards, the registers
are always little endian, on NV11+ cards endianness of the whole area can be
selected by a switch in PMC. The endianness switch, however, only affects
BAR0 accesses to the MMIO space - accesses from inside the card are always
little-endian.

A particularly important subarea of MMIO space is PMC, the card's master
control. This subarea is present on all nvidia GPUs at addresses 0x000000
through 0x000fff. It contains the card chipset information, Big Red Switches
for engines that can be turned off, and master interrupt control. It's
described in more detail in :ref:`pmc`.

For full list of MMIO areas, see :ref:`mmio`.


BAR1: VRAM aperture
===================

This is an area of prefetchable memory that maps to the card's VRAM. On native
PCIE cards, it uses 64-bit addressing, on native PCI/AGP ones it uses 32-bit
addressing.

On non-TURBOCACHE pre-NV50 cards and on NV50+ cards with BAR1 VM disabled, BAR
addresses map directly to VRAM addresses. On TURBOCACHE cards, BAR1 is made of
controllable VRAM and GART windows [see :ref:`nv44-host-mem`].
NV50+ cards have a mode where all BAR references go through the card's VM
subsystem, see :ref:`nv50-host-mem` and :ref:`nvc0-host-mem`.

On NV03 cards, this BAR also contains RAMIN access aperture at address
0xc00000 [see :ref:`nv03-vram`]

.. todo:: map out the BAR fully

the BAR size depends on card type:

- NV03: 16MB [with RAMIN]
- NV04: 16MB
- NV05: 32MB
- NV10:NV17: 128MB
- NV17:NV50: 64MB-512MB, set via :ref:`straps <pstraps>`
- NV50-: 64MB-64GB, set via straps

Note that BAR size is independent from actual VRAM size, although on pre-NV30
cards the BAR is guaranteed not to be smaller than VRAM. This means it may
be impossible to map all of the card's memory through the BAR on NV30+ cards.


BAR2/BAR3: RAMIN aperture
=========================

RAMIN is, on pre-NV50 cards, a special area at the end of VRAM that contains
various control structures. RAMIN starts from end of VRAM and the addresses
go in reverse direction, thus it needs a special mapping to access it the way
it'll be used. While pre-NV40 cards limitted its size to 1MB and could fit the
mapping in BAR0, or BAR1 for NV03, NV40+ allow much bigger RAMIN addresses.
RAMIN BAR provides such RAMIN mapping on NV40 family cards.

NV50 did away with a special RAMIN area, but it kept the BAR around. It works
like BAR1, but is independent on it and can use a distinct VM DMA object. As
opposed to BAR1, all accesses done to BAR3 will be automatically byte-swapped
in 32-bit chunks like BAR0 if the big-endian switch is on. It's commonly
used to map control structures for kernel use, while BAR1 is used to map
user-accessible memory.

The BAR uses 64-bit addressing on native PCIE cards, 32-bit addressing on
native PCI/AGP. It uses BAR2 slot on native PCIE, BAR3 on native PCI/AGP.
It is non-prefetchable memory on cards up to and including NVA0, prefetchable
memory on NVAA+. The size is at least 16MB and is set via :ref:`straps <pstraps>`.


BAR2: NV03 indirect memory access
=================================

An area of IO ports used to access BAR0 or BAR1 indirectly by real mode code
that cannot map high memory addresses. Present only on NV03.

.. todo:: RE it. or not.


BAR5: NV50 indirect memory access
=================================

An area of IO ports used to access BAR0, BAR1, and BAR3 indirectly by real
mode code that cannot map high memory addresses. Present on NV50+ cards.
On earlier cards, the indirect access feature of VGA IO ports can be used
instead. This BAR can also be disabled via :ref:`straps <pstraps>`.

.. todo:: It's present on some NV4x

This area is 0x80 bytes of IO ports, but only first 0x20 bytes are actually
used; the rest are empty. The ports are all treated as 32-bit ports. They
are:

BAR5+0x00:
    when read, signature: 0x2469fdb9. When written, master enable:
    write 1 to enable remaining ports, 0 to disable. Only bit 0 of
    the written value is taken into account. When remaining ports
    are disabled, they read as 0xffffffff.
BAR5+0x04:
    enable. if bit 0 is 1, the "data" ports are active, otherwise
    they're inactive and merely store the last written value.
BAR5+0x08:
    BAR0 address port. bits 0-1 and 24-31 are ignored.
BAR5+0x0c:
    BAR0 data port. Reads and writes are translated to BAR0 reads
    and writes at address specified by BAR0 address port.
BAR5+0x10:
    BAR1 address port. bits 0-1 are ignored.
BAR5+0x14:
    BAR1 data port. Reads and writes are translated to BAR1 reads
    and writes at address specified by BAR1 address port.
BAR5+0x18:
    BAR3 address port. bits 0-1 and 24-31  are ignored.
BAR5+0x1c:
    BAR3 data port. Reads and writes are translated to BAR3 reads
    and writes at address specified by BAR3 address port.

BAR0 addresses are masked to low 24 bits, allowing access to exactly 16MB
of MMIO space. The BAR1 addresses aren't masked, and the window actually
allows access to more BAR space than the BAR1 itself - up to 4GB of VRAM
or VM space can be accessed this way. BAR3 addresses, on the other hand,
are masked to low 24 bits even though the real BAR3 is larger.


BAR6: PCI ROM aperture
======================

.. todo:: figure out size
.. todo:: figure out NV03
.. todo:: verify NV50

The nvidia GPUs expose their BIOS as standard PCI ROM. The exposed ROM aliases
either the actual BIOS EEPROM, or the shadow BIOS in VRAM. This setting is
exposed in PCI config space. If the "shadow enabled" PCI config register is
0, the PROM MMIO area is enabled, and both PROM and the PCI ROM aperture will
access the EEPROM. Disabling the shadowing has a side effect of disabling
video output on pre-NV50 cards. If shadow is enabled, EEPROM is disabled,
PROM reads will return garbage, and PCI ROM aperture will access the VRAM
shadow copy of BIOS. On pre-NV50 cards, the shadow BIOS is located at address
0 of RAMIN, on NV50+ cards the shadow bios is pointed to by
PDISPLAY.VGA.ROM_WINDOW register - see :ref:`nv50-vga` for details.


INTA: the card interrupt
========================

.. todo:: MSI

The GPU reports all interrupts through the PCI INTA line. The interrupt enable
and status registers are located in PMC area - see :ref:`pmc-intr`.


Legacy VGA IO ports and memory
==============================

The nvidia GPU cards are backwards compatible with VGA and expose the usual
VGA ranges: IO ports 0x3b0-0x3bb and 0x3c0-0x3df, memory at 0xa0000-0xbffff.
The VGA ranges can however be disabled in PCI config space. The VGA registers
and memory are still accessible through their aliases in BAR0, and disabling
the legacy ranges has no effect on the operation of the card. The IO range
contains an extra top-level register that allows indirect access to the MMIO
area for use by real mode code, as well as many nvidia-specific extra
registers in the VGA subunits. For details, see :ref:`nv03-vga`.
