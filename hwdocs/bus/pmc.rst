.. _pmc:

========================
PMC: Master control unit
========================

.. contents::


Introduction
============

PMC is the "master control" engine of the card. Its purpose is to provide
card identication, manage enable/disable bits of other engines, and handle
top-level interrupt routing.


.. _pmc-mmio:

MMIO register list
==================

.. space:: 8 pmc 0x1000 card master control
   0x000 ID pmc-id-nv01 NV01:NV04
   0x000 ID pmc-id-nv04 NV04:NV10
   0x000 ID pmc-id-nv10 NV10:
   0x004 ENDIAN pmc-endian NV1A:
   0x008 BOOT_2 pmc-boot-2 NV92:
   0x100 INTR_HOST pmc-intr-host
   0x104 INTR_NRHOST pmc-intr-nrhost NVA3:
   0x108 INTR_DAEMON pmc-intr-daemon NVA3:
   0x140 INTR_ENABLE_HOST pmc-intr-enable-host
   0x144 INTR_ENABLE_NRHOST pmc-intr-enable-nrhost NVA3:
   0x148 INTR_ENABLE_DAEMON pmc-intr-enable-daemon NVA3:
   0x160 INTR_LINE_HOST pmc-intr-line-host
   0x164 INTR_LINE_NRHOST pmc-intr-line-nrhost NVA3:
   0x168 INTR_LINE_DAEMON pmc-intr-line-daemon NVA3:
   0x17c INTR_PMFB pmc-intr-pmfb NVC0:
   0x180 INTR_PBFB pmc-intr-pbfb NVC0:
   0x200 ENABLE pmc-enable
   0x204 SPOON_ENABLE pmc-spoon-enable NVC0:
   0x208 ENABLE_UNK08 pmc-enable-unk08 NVC0:
   0x20c ENABLE_UNK0C pmc-enable-unk0c NVC4:
   0x260[6] FIFO_ENG_UNK260 pmc-fifo-eng-unk260 NVC0:
   0x300 VRAM_HIDE_LOW pmc-vram-hide-low NV17:NVF0
   0x304 VRAM_HIDE_HIGH pmc-vram-hide-high NV17:NVF0
   0x640 INTR_MASK_HOST pmc-intr-mask-host NVA3:
   0x644 INTR_MASK_NRHOST pmc-intr-mask-nrhost NVA3:
   0x648 INTR_MASK_DAEMON pmc-intr-mask-daemon NVA3:
   0xa00 NEW_ID pmc-new-id NV94:

   The PMC register range is always active.


Card identification
===================

The main register used to identify the card is the ID register. However,
the ID register has different formats depending on the chipset family:

.. reg:: 32 pmc-id-nv01 card identification

   - bits 0-3: minor revision.
   - bits 4-7: major revision.
     These two bitfields together are also visible as PCI revision. For
     NV03, revisions equal or higher than 0x20 mean NV03T.
   - bits 8-11: implementation - always 1 except on NV02
   - bits 12-15: always 0
   - bits 16-19: chipset - 1 is NV01, 2 is NV02, 3 is NV03 or NV03T
   - bits 20-27: always 0
   - bits 28-31: foundry - 0 is SGS, 1 is Helios, 2 is TMSC

.. reg:: 32 pmc-id-nv04 card identification

   - bits 0-3: ???
   - bits 4-11: always 0
   - bits 12-15: architecture - always 4
   - bits 16-19: minor revision
   - bits 20-23: major revision - 0 is NV04, 1 and 2 are NV05.
     These two bitfields together are also visible as PCI revision.
   - bits 24-27: always 0
   - bits 28-31: foundry - 0 is SGS, 1 is Helios, 2 is TMSC

.. reg:: 32 pmc-id-nv10 card identification

   - bits 0-7: stepping
   - bits 16-19: device id [NV10:NV92]
   - bits 15-19: device id [NV92:NVD9]
   - bits 12-19: device id [NVD9-]
     The value of this bitfield is equal to low 4, 5, or 6 bits of the PCI
     device id. The bitfield size and position changed between cards due to
     varying amount of changeable bits. See :ref:`pstraps` and
     :ref:`chipsets` for more details.
   - bits 20-27: chipset id.
     This is THE chipset id that comes after "NV". See :ref:`chipsets` for the
     list.
   - bits 28-31: ???

.. todo:: unk bitfields

NV92[?] introduced another identification register in PMC, with unknown
purpose:

.. reg:: 32 pmc-boot-2 ???

   ???
 
.. todo:: what is this? when was it introduced? seen non-0 on at least NV92

NV94 introduced a new identification register with rearranged bitfields:

.. reg:: 32 pmc-new-id card identification

   - bits 0-7: device id
   - bits 8-11: same value as BOOT_2 register
   - bits 12-19: stepping
   - bits 20-27: chipset id

.. todo:: there are cards where the steppings don't match
   between registers - does this mean something or is it just
   a random screwup?


Endian switch
=============

PMC also contains the endian switch register. The endian switch can be set to
either little or big endian, and affects all accesses to BAR0 and, if present,
BAR2/BAR3 - see :ref:`bars` for more details. It is controlled by the ENDIAN
register:

.. reg:: 32 pmc-endian endian switch

   When read, returns 0x01000001 if in big-endian mode, 0 if in little-endian
   mode. When written, if bit 24 of the written value is 1, flips the endian
   switch to the opposite value, otherwise does nothing.

The register operates in such idiosyncratic way because it is itself affected
by the endian switch - thus the read value was chosen to be unaffected by
wrong endian setting, while write behavior was chosen so that writing "1" in
either endianness will switch the card to that endianness.

This register and the endian switch don't exist on pre-NV1A cards - they're
always little-endian.

Note that this switch is also used by NV50+ PFIFO as its default endianness
- see :ref:`NV50+ PFIFO<nv50-pfifo>` for details.

The MMIO areas containing aliases of 8-bit VGA registers are unaffected by
this switch, despite being in BAR0.


.. _pmc-enable:

Engine enables
==============

PMC contains the main engine enable register, which is used to turn whole
engines on and off:

.. reg:: 32 pmc-enable engine master enable

   When given bit is set to 0, the corresponding engine is disabled, when set
   to 1, it is enabled. Most engines disappear from MMIO space and reset to
   default state when disabled.

On NV01, the bits are:

- 0: :ref:`PAUDIO <nv01-paudio>`
- 4: :ref:`PDMA <nv01-pdma>` and :ref:`PTIMER <ptimer>`
- 8: :ref:`PFIFO <nv01-pfifo>`
- 12: :ref:`PGRAPH <nv01-pgraph>`
- 16: :ref:`PRM <nv01-prm>`
- 24: :ref:`PFB <nv01-pfb>`

On NV03:NV04, the bits are:

- 0: ??? [XXX]
- 4: :ref:`PMEDIA <pmedia>`
- 8: :ref:`PFIFO <nv01-pfifo>`
- 12: :ref:`PGRAPH <nv03-pgraph>` and :ref:`PDMA <nv03-pdma>`
- 16: :ref:`PTIMER <ptimer>`
- 20: :ref:`PFB <nv03-pfb>`
- 24: :ref:`PCRTC <pcrtc>`
- 28: :ref:`PRAMDAC.VIDEO <pvideo>`

On NV04:NV50, the bits are:

- 0: ??? - alleged to be related to I2C [NV10-] [XXX]
- 1: :ref:`PVPE <pvpe>` [NV17-]
- 4: :ref:`PMEDIA <pmedia>`
- 8: :ref:`PFIFO <nv04-pfifo>`
- 12: :ref:`PGRAPH <nv04-pgraph>` [NV04:NV10]
- 12: :ref:`PGRAPH <nv10-pgraph>` [NV10:NV20]
- 12: :ref:`PGRAPH <nv20-pgraph>` [NV20:NV40]
- 12: :ref:`PGRAPH <nv40-pgraph>` [NV40:NV50]
- 13: PGRAPH CS??? apparently exists on some late NV4x... [NV4?-]
- 16: :ref:`PTIMER <ptimer>`
- 20: PFB [:ref:`NV03 <nv03-pfb>`, :ref:`NV10 <nv10-pfb>`, :ref:`NV40 <nv40-pfb>`, :ref:`NV44 <nv44-pfb>`]
- 24: :ref:`PCRTC <pcrtc>`
- 25: :ref:`PCRTC2 <pcrtc>` [NV11-]
- 26: :ref:`PTV <ptv>` [NV17:NV20, NV25:NV50]
- 28: :ref:`PRAMDAC.VIDEO <pvideo>` [NV04:NV10] or :ref:`PVIDEO <pvideo>` [NV10:NV50]

.. todo:: figure out the CS thing, figure out the variants. Known not to exist on NV40, NV43, NV44, NV4E, NV49; known to exist on NV63

On NV50:NVC0, the bits are:

- 0: ??? - alleged to be related to I2C
- 1: :ref:`PVPE <pvpe>` [NV50:NV98 NVA0:NVAA]
- 1: :ref:`PPPP <pppp>` [NV98:NVA0 NVAA-]
- 4: :ref:`PMEDIA <pmedia>`
- 8: :ref:`PFIFO <nv50-pfifo>`
- 12: :ref:`PGRAPH <nv50-pgraph>`
- 13: :ref:`PCOPY <pcopy>` [NVA3-]
- 14: :ref:`PCRYPT2 <pcrypt2>` [NV84:NV98 NVA0:NVAA]
- 14: :ref:`PCRYPT3 <pcrypt3>` [NV98:NVA0 NVAA:NVA3]
- 14: :ref:`PVCOMP <pvcomp>` [NVAF]
- 15: :ref:`PBSP <pbsp>` [NV84:NV98 NVA0:NVAA]
- 15: :ref:`PVLD <pvld>` [NV98:NVA0 NVAA-]
- 16: :ref:`PTIMER <ptimer>`
- 17: :ref:`PVP2 <pvp2>` [NV84:NV98 NVA0:NVAA]
- 17: :ref:`PVDEC <pvdec>` [NV98:NVA0 NVAA-]
- 20: :ref:`PFB <nv50-pfb>`
- 21: :ref:`PGRAPH CHSW <nv50-pfifo-chsw>` [NV84-]
- 22: :ref:`PMPEG CHSW <nv50-pfifo-chsw>` [NV84-]
- 23: :ref:`PCOPY CHSW <nv50-pfifo-chsw>` [NVA3-]
- 24: :ref:`PVP2 CHSW <nv50-pfifo-chsw>` [NV84:NV98 NVA0:NVAA]
- 24: :ref:`PVDEC CHSW <nv50-pfifo-chsw>` [NV98:NVA0 NVAA-]
- 25: :ref:`PCRYPT2 CHSW <nv50-pfifo-chsw>` [NV84:NV98 NVA0:NVAA]
- 25: :ref:`PCRYPT3 CHSW <nv50-pfifo-chsw>` [NV98:NVA0 NVAA:NVA3]
- 25: :ref:`PVCOMP CHSW <nv50-pfifo-chsw>` [NVAF]
- 26: :ref:`PBSP CHSW <nv50-pfifo-chsw>` [NV84:NV98 NVA0:NVAA]
- 26: :ref:`PVLD CHSW <nv50-pfifo-chsw>` [NV98:NVA0 NVAA-]
- 27: ??? [NV84-]
- 28: ??? [NV84-]
- 30: :ref:`PDISPLAY <pdisplay>`
- 31: ???

.. todo:: unknowns

On NVC0+, the bits are:

- 0: ??? - alleged to be related to I2C
- 1: :ref:`PPPP <pppp>`
- 2: :ref:`PXBAR <pxbar>`
- 3: :ref:`PMFB <pmfb>`
- 4: :ref:`PMEDIA <pmedia>`
- 5: :ref:`PIBUS <pibus>`
- 6: :ref:`PCOPY[0] <pcopy>`
- 7: :ref:`PCOPY[1] <pcopy>`
- 8: :ref:`PFIFO <nvc0-pfifo>`
- 12: :ref:`PGRAPH <nvc0-pgraph>`
- 13: :ref:`PDAEMON <pdaemon>`
- 15: :ref:`PVLD <pvld>`
- 16: :ref:`PTIMER <ptimer>`
- 17: :ref:`PVDEC <pvdec>`
- 18: :ref:`PVENC <pvenc>` [NVE4-]
- 20: :ref:`PBFB <pbfb>`
- 21: :ref:`PCOPY[2] <pcopy>` [NVE4-]
- 26: ??? [NVE4-]
- 27: ???
- 28: :ref:`PCOUNTER <pcounter>`
- 29: :ref:`PFFB <pffb>`
- 30: :ref:`PDISPLAY <pdisplay>`
- 31: ???

NVC0 also introduced SUBFIFO_ENABLE register:

.. reg:: 32 pmc-spoon-enable PSPOON enables

   Enables PFIFO's PSUBFIFOs. Bit i corresponds to PSUBFIFO[i]. See
   :ref:`NVC0+ PFIFO <nvc0-psubfifo>` for details.

There are also two other registers looking like ENABLE, but with seemingly
no effect and currently unknown purpose:

.. reg:: 32 pmc-enable-unk08 ??? related to enable

   Has the same bits as ENABLE, comes up as all-1 on boot, except for PDISPLAY
   bit which comes up as 0.

.. reg:: 32 pmc-enable-unk0c ??? related to enable

   Has bits which correspond to PFIFO engines in ENABLE, ie.

   - 1: PPPP
   - 6: PCOPY[0]
   - 7: PCOPY[1]
   - 12: PGRAPH
   - 15: PVLD
   - 17: PVDEC

   Comes up as all-1.

.. reg:: 32 pmc-fifo-eng-unk260 ??? related to PFIFO engines

   Single-bit registers, 6 of them.

.. todo:: RE these three


.. _pmc-intr:
.. _pdaemon-intr-pmc-daemon:

Interrupts
==========

Another thing that PMC handles is the top-level interrupt routing. On cards
earlier than NVA3, PMC gets interrupt lines from all interested engines on
the card, aggregates them together, adds in an option to trigger a "software"
interrupt manually, and routes them to the PCI INTA pin. There is an enable
register, but it only allows one to enable/disable all hardware or all
software interrupts.

NVA3 introduced fine-grained interrupt masking, as well as an option to route
interrupts to PDAEMON. The HOST interrupts have a new redirection stage in
PDAEMON [see :ref:`pdaemon-iredir`] - while normally routed to the PCI interrupt line,
they may be switched over to PDAEMON delivery when it so decides. As a side
effect of that, powering off PDAEMON will disable host interrupt delivery.
A subset of interrupt types can also be routed to NRHOST destination, which
is identical to HOST, but doesn't go through the PDAEMON redirection circuitry.

.. todo:: change all this duplication to indexing

.. reg:: 32 pmc-intr-host interrupt status - host

   Interrupt status. Bits 0-30 are hardware interrupts, bit 31 is software
   interrupt. 1 if the relevant input interrupt line is active and, for NVA3+
   chipsets, enabled in INTR_MASK_*. Bits 0-30 are read-only, bit 31 can be
   written to set/clear the software interrupt. Bit 31 can only be set to 1 if
   software interrupts are enabled in INTR_MASK_*, except for NRHOST on NVC0+,
   where it works even if masked.

.. reg:: 32 pmc-intr-nrhost interrupt status - non-redirectable host

   Like :obj:`pmc-intr-host`, but for NRHOST.

.. reg:: 32 pmc-intr-daemon interrupt status - PDAEMON

   Like :obj:`pmc-intr-host`, but for DAEMON.

.. reg:: 32 pmc-intr-enable-host interrupt enable - host

  - bit 0: hardware interrupt enable - if 1, and any of bits 0-30 of INTR_* are
    active, the corresponding output interrupt line will be asserted.
  - bit 1: software interrupt enable - if 1, bit 31 of INTR_* is active, the
    corresponding output interrupt line will be asserted.

.. reg:: 32 pmc-intr-enable-nrhost interrupt enable - non-redirectable host

   Like :obj:`pmc-intr-enable-nrhost`, but for NRHOST.

.. reg:: 32 pmc-intr-enable-daemon interrupt enable - PDAEMON

   Like :obj:`pmc-intr-enable-host`, but for DAEMON.

.. reg:: 32 pmc-intr-line-host interrupt line status - host

   Provides a way to peek at the status of corresponding output interrupt line.
   On NV01:NVC0, 0 if the output line is active, 1 if inactive. On NVC0+, 1 if
   active, 0 if inactive.

.. reg:: 32 pmc-intr-line-nrhost interrupt line status - non-redirectable host

   Like :obj:`pmc-intr-line-host`, but for NRHOST.

.. reg:: 32 pmc-intr-line-daemon interrupt line status - PDAEMON

   Like :obj:`pmc-intr-line-host`, but for DAEMON.

.. reg:: 32 pmc-intr-mask-host interrupt mask - host

   Interrupt mask. If a bit is set to 0 here, it'll be masked off to always-0
   in the INTR_* register, otherwise it'll be connected to the corresponding
   input interrupt line. For HOST and DAEMON, all interrupts can be enabled.
   For NRHOST on pre-NVC0 cards, only input line #8 [PFIFO] can be enabled, for
   NRHOST on NVC0+ cards all interrupts but the software interrupt can be
   enabled - however in this case software interrupt works even without being
   enabled.

.. reg:: 32 pmc-intr-mask-nrhost interrupt mask - non-redirectable host

   Like :obj:`pmc-intr-mask-host`, but for NRHOST.

.. reg:: 32 pmc-intr-mask-daemon interrupt mask - PDAEMON

   Like :obj:`pmc-intr-mask-host`, but for DAEMON.

The HOST and NRHOST output interrupt lines are connected to the PCI INTA pin
on the card. HOST goes through PDAEMON's HOST interrupt redirection circuitry
[IREDIR], while NRHOST doesn't. DAEMON goes to PDAEMON's falcon interrupt line #10
[PMC_DAEMON].

On pre-NVA3, each PMC interrupt input is a single 0/1 line. On NVA3+, some
inputs have a single line for all three outputs, while some others have 2
lines: one for HOST and DAEMON outputs, and one for NRHOST outuput.

The input interrupts are, for NV01:

- 0: :ref:`PAUDIO <nv01-paudio-intr>`
- 4: :ref:`PDMA <nv01-pdma-intr>`
- 8: :ref:`PFIFO <nv01-pfifo-intr>`
- 12: :ref:`PGRAPH <nv01-pgraph-intr>`
- 16: :ref:`PRM <nv01-prm-intr>`
- 20: :ref:`PTIMER <ptimer-intr>`
- 24: :ref:`PGRAPH's vblank interrupt <nv01-pgraph-intr-vblank>`
- 28: software

.. todo:: check

For NV03:

- 4: :ref:`PMEDIA <pmedia-intr>`
- 8: :ref:`PFIFO <nv01-pfifo-intr>`
- 12: :ref:`PGRAPH <nv03-pgraph>`
- 13: :ref:`PDMA <nv03-pdma>`
- 16: :ref:`PRAMDAC.VIDEO <pvideo-intr>`
- 20: :ref:`PTIMER <ptimer-intr>`
- 24: :ref:`PGRAPH's vblank interrupt <pcrtc-intr>`
- 28: :ref:`PBUS <pbus-intr>`
- 31: software

For NV04:NV50:

- 0: :ref:`PVPE <pvpe-intr>` [NV17:NV20 and NV25:NV50]
- 4: :ref:`PMEDIA <pmedia-intr>`
- 8: :ref:`PFIFO <nv04-pfifo-intr>`
- 12: :ref:`PGRAPH <nv04-pgraph-intr>`
- 16: :ref:`PRAMDAC.VIDEO <pvideo-intr>` [NV04:NV10] or :ref:`PVIDEO <pvideo-intr>` [NV10:NV50]
- 20: :ref:`PTIMER <ptimer-intr>`
- 24: :ref:`PCRTC <pcrtc-intr>`
- 25: :ref:`PCRTC2 <pcrtc-intr>` [NV17:NV20 and NV25:NV50]
- 28: :ref:`PBUS <pbus-intr>`
- 31: software

For NV50:NVC0:

- 0: :ref:`PVPE <pvpe-intr>` [NV50:NV98 NVA0:NVAA]
- 0: :ref:`PPPP <pppp-falcon>` [NV98:NVA0 NVAA-]
- 4: :ref:`PMEDIA <pmedia-intr>`
- 8: :ref:`PFIFO <nv50-pfifo-intr>` - has separate NRHOST line on NVA3+
- 9: ??? [NVA3?-]
- 11: ??? [NVA3?-]
- 12: :ref:`PGRAPH <nv50-pgraph-intr>`
- 13: ??? [NVA3?-]
- 14: :ref:`PCRYPT2 <pcrypt2-intr>` [NV84:NV98 NVA0:NVAA]
- 14: :ref:`PCRYPT3 <pcrypt3-falcon>` [NV98:NVA0 NVAA:NVA3]
- 14: :ref:`PVCOMP <pvcomp-falcon>` [NVAF-]
- 15: :ref:`PBSP <pbsp-intr>` [NV84:NV98 NVA0:NVAA]
- 15: :ref:`PVLD <pvld-falcon>` [NV98:NVA0 NVAA-]
- 16: ??? [NVA3?-]
- 17: :ref:`PVP2 <pvp2-intr>` [NV84:NV98 NVA0:NVAA]
- 17: :ref:`PVDEC <pvdec-falcon>` [NV98:NVA0 NVAA-]
- 18: :ref:`PDAEMON [NVA3-] <pdaemon-falcon>`
- 19: :ref:`PTHERM [NVA3-] <ptherm-intr>`
- 20: :ref:`PTIMER <ptimer-intr>`
- 21: :ref:`PNVIO's GPIO interrupts <nv50-gpio-intr>`
- 22: :ref:`PCOPY <pcopy-falcon>`
- 26: :ref:`PDISPLAY <pdisplay-intr>`
- 27: ??? [NVA3?-]
- 28: :ref:`PBUS <pbus-intr>`
- 29: :ref:`PPCI <ppci-intr>` [NV84-]
- 31: software

.. todo:: figure out unknown interrupts. They could've been introduced much
   earlier, but we only know them from bitscanning the INTR_MASK regs. on NVA3+.

For NVC0+:

- 0: :ref:`PPPP <pppp-falcon>` - has separate NRHOST line
- 4: :ref:`PMEDIA <pmedia-intr>`
- 5: PCOPY[0] [:ref:`NVC0 <pcopy-falcon>`, :ref:`NVE4 <pcopy-intr>`] - has separate NRHOST line
- 6: PCOPY[1] [:ref:`NVC0 <pcopy-falcon>`, :ref:`NVE4 <pcopy-intr>`] - has separate NRHOST line
- 7: :ref:`PCOPY[2] <pcopy-intr>` [NVE4-] - has separate NRHOST line
- 8: :ref:`PFIFO <nvc0-pfifo-intr>`
- 9: ???
- 12: :ref:`PGRAPH <nvc0-pgraph-intr>` - has separate NRHOST line
- 13: :ref:`PBFB <pbfb-intr>`
- 15: :ref:`PVLD <pvld-falcon>` - has separate NRHOST line
- 16: :ref:`PVENC <pvenc-falcon>` [NVE4-] - has separate NRHOST line
- 17: :ref:`PVDEC <pvdec-falcon>` - has separate NRHOST line
- 18: :ref:`PTHERM <ptherm-intr>`
- 19: ??? [NVD9-]
- 20: :ref:`PTIMER <ptimer-intr>`
- 21: :ref:`PNVIO's GPIO interrupts <nv50-gpio-intr>`
- 23: ???
- 24: :ref:`PDAEMON <pdaemon-falcon>`
- 25: :ref:`PMFB <pmfb-intr>`
- 26: :ref:`PDISPLAY <pdisplay-intr>`
- 27: :ref:`PFFB <pffb-intr>`
- 28: :ref:`PBUS <pbus-intr>` - has separate NRHOST line
- 29: :ref:`PPCI <ppci-intr>`
- 30: :ref:`PIBUS <pibus-intr>`
- 31: software

.. todo:: unknowns

.. todo:: document these two

.. reg:: 32 pmc-intr-pmfb PMFB interrupt status

   Bit x == interrupt for PMFB part x pending.

.. reg:: 32 pmc-intr-pbfb PBFB interrupt status

   Bit x == interrupt for PBFB part x pending.

.. todo:: verify variants for these?


.. _pmc-vram-hide:

VRAM hidden area
================

NV17/NV20 added a feature to disable host reads through selected range of
VRAM. The registers are:

.. reg:: 32 pmc-vram-hide-low VRAM hidden area low address

   - bits 0-28: address of start of the hidden area. bits 0-1 are ignored, the
     area is always 4-byte aligned.
   - bit 31: hidden area enabled

.. reg:: 32 pmc-vram-hide-high VRAM hidden area high address

   - bits 0-28: address of end of the hidden area. bits 0-1 are ignored, the
     area is always 4-byte aligned.

The start and end addresses are both inclusive. All BAR1, BAR2/BAR3, PEEPHOLE
and PMEM/PRAMIN reads whose offsets fall into this window will be silently
mangled to read 0 instead. Writes are unaffected. Note that offset from start
of the BAR/PEEPHOLE/PRAMIN/PMEM is used for the comparison, not the actual
VRAM address - thus the selected window will cover a different thing in each
affected space.

The VRAM hidden area functionality got silently nuked on NVC0+ chipsets. The
registers are still present, but they don't do anything.
