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


MMIO register list
==================

.. space:: 8 pmc 0x1000 card master control
   0x000 ID pmc-id-nv1 NV1:NV4
   0x000 ID pmc-id-nv4 NV4:NV10
   0x000 ID pmc-id-nv10 NV10:
   0x004 ENDIAN pmc-endian NV1A:
   0x008 BOOT_2 pmc-boot-2 G92:
   0x100 INTR_HOST pmc-intr-host
   0x104 INTR_NRHOST pmc-intr-nrhost GT215:
   0x108 INTR_DAEMON pmc-intr-daemon GT215:
   0x140 INTR_ENABLE_HOST pmc-intr-enable-host
   0x144 INTR_ENABLE_NRHOST pmc-intr-enable-nrhost GT215:
   0x148 INTR_ENABLE_DAEMON pmc-intr-enable-daemon GT215:
   0x160 INTR_LINE_HOST pmc-intr-line-host
   0x164 INTR_LINE_NRHOST pmc-intr-line-nrhost GT215:
   0x168 INTR_LINE_DAEMON pmc-intr-line-daemon GT215:
   0x17c INTR_PMFB pmc-intr-pmfb GF100:
   0x180 INTR_PBFB pmc-intr-pbfb GF100:
   0x200 ENABLE pmc-enable
   0x204 SPOON_ENABLE pmc-spoon-enable GF100:
   0x208 ENABLE_UNK08 pmc-enable-unk08 GF100:
   0x20c ENABLE_UNK0C pmc-enable-unk0c GF104:
   0x260[6] FIFO_ENG_UNK260 pmc-fifo-eng-unk260 GF100:
   0x300 VRAM_HIDE_LOW pmc-vram-hide-low NV17:GK110
   0x304 VRAM_HIDE_HIGH pmc-vram-hide-high NV17:GK110
   0x640 INTR_MASK_HOST pmc-intr-mask-host GT215:
   0x644 INTR_MASK_NRHOST pmc-intr-mask-nrhost GT215:
   0x648 INTR_MASK_DAEMON pmc-intr-mask-daemon GT215:
   0xa00 NEW_ID pmc-new-id G94:

   The PMC register range is always active.


.. _pmc-id:

Card identification
===================

The main register used to identify the card is the ID register. However,
the ID register has different formats depending on the GPU family:

.. reg:: 32 pmc-id-nv1 card identification

   - bits 0-3: minor revision.
   - bits 4-7: major revision.
     These two bitfields together are also visible as PCI revision. For
     NV3, revisions equal or higher than 0x20 mean NV3T.
   - bits 8-11: implementation - always 1 except on NV2
   - bits 12-15: always 0
   - bits 16-19: GPU - 1 is NV1, 2 is NV2, 3 is NV3 or NV3T
   - bits 20-27: always 0
   - bits 28-31: foundry - 0 is SGS, 1 is Helios, 2 is TMSC

.. reg:: 32 pmc-id-nv4 card identification

   - bits 0-3: ???
   - bits 4-11: always 0
   - bits 12-15: architecture - always 4
   - bits 16-19: minor revision
   - bits 20-23: major revision - 0 is NV4, 1 and 2 are NV5.
     These two bitfields together are also visible as PCI revision.
   - bits 24-27: always 0
   - bits 28-31: foundry - 0 is SGS, 1 is Helios, 2 is TMSC

.. reg:: 32 pmc-id-nv10 card identification

   - bits 0-7: stepping
   - bits 16-19: device id [NV10:G92]
   - bits 15-19: device id [G92:GF119]
   - bits 12-19: device id [GF119-]
     The value of this bitfield is equal to low 4, 5, or 6 bits of the PCI
     device id. The bitfield size and position changed between cards due to
     varying amount of changeable bits. See :ref:`pstraps` and
     :ref:`gpu` for more details.
   - bits 20-27: GPU id.
     This is THE GPU id that comes after "NV". See :ref:`gpu` for the
     list.
   - bits 28-31: ???

.. todo:: unk bitfields

G92[?] introduced another identification register in PMC, with unknown
purpose:

.. reg:: 32 pmc-boot-2 ???

   ???
 
.. todo:: what is this? when was it introduced? seen non-0 on at least G92

G94 introduced a new identification register with rearranged bitfields:

.. reg:: 32 pmc-new-id card identification

   - bits 0-7: device id
   - bits 8-11: same value as BOOT_2 register
   - bits 12-19: stepping
   - bits 20-27: GPU id

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

Note that this switch is also used by G80+ PFIFO as its default endianness
- see :ref:`G80+ PFIFO<g80-pfifo>` for details.

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

On NV1, the bits are:

- 0: :ref:`PAUDIO <nv1-paudio>`
- 4: :ref:`PDMA <nv1-pdma>` and :ref:`PTIMER <ptimer>`
- 8: :ref:`PFIFO <nv1-pfifo>`
- 12: :ref:`PGRAPH <nv1-pgraph>`
- 16: :ref:`PRM <nv1-prm>`
- 24: :ref:`PFB <nv1-pfb>`

On NV3:NV4, the bits are:

- 0: ??? [XXX]
- 4: :ref:`PMEDIA <pmedia>`
- 8: :ref:`PFIFO <nv1-pfifo>`
- 12: :ref:`PGRAPH <nv3-pgraph>` and :ref:`PDMA <nv3-pdma>`
- 16: :ref:`PTIMER <ptimer>`
- 20: :ref:`PFB <nv3-pfb>`
- 24: :ref:`PCRTC <pcrtc>`
- 28: :ref:`PRAMDAC.VIDEO <pvideo>`

On NV4:G80, the bits are:

- 0: ??? - alleged to be related to I2C [NV10-] [XXX]
- 1: :ref:`PVPE <pvpe>` [NV17-]
- 4: :ref:`PMEDIA <pmedia>`
- 8: :ref:`PFIFO <nv4-pfifo>`
- 12: :ref:`PGRAPH <nv4-pgraph>` [NV4:NV10]
- 12: :ref:`PGRAPH <nv10-pgraph>` [NV10:NV20]
- 12: :ref:`PGRAPH <nv20-pgraph>` [NV20:NV40]
- 12: :ref:`PGRAPH <nv40-pgraph>` [NV40:G80]
- 13: PGRAPH CS??? apparently exists on some late NV4x... [NV4?-]
- 16: :ref:`PTIMER <ptimer>`
- 20: PFB [:ref:`NV3 <nv3-pfb>`, :ref:`NV10 <nv10-pfb>`, :ref:`NV40 <nv40-pfb>`, :ref:`NV44 <nv44-pfb>`]
- 24: :ref:`PCRTC <pcrtc>`
- 25: :ref:`PCRTC2 <pcrtc>` [NV11-]
- 26: :ref:`PTV <ptv>` [NV17:NV20, NV25:G80]
- 28: :ref:`PRAMDAC.VIDEO <pvideo>` [NV4:NV10] or :ref:`PVIDEO <pvideo>` [NV10:G80]

.. todo:: figure out the CS thing, figure out the variants. Known not to exist on NV40, NV43, NV44, C51, G71; known to exist on MCP73

On G80:GF100, the bits are:

- 0: ??? - alleged to be related to I2C
- 1: :ref:`PVPE <pvpe>` [G80:G98 G200:MCP77]
- 1: :ref:`PPPP <pppp>` [G98:G200 MCP77-]
- 4: :ref:`PMEDIA <pmedia>`
- 8: :ref:`PFIFO <g80-pfifo>`
- 12: :ref:`PGRAPH <g80-pgraph>`
- 13: :ref:`PCOPY <pcopy>` [GT215-]
- 14: :ref:`PCRYPT2 <pcrypt2>` [G84:G98 G200:MCP77]
- 14: :ref:`PSEC <psec>` [G98:G200 MCP77:GT215]
- 14: :ref:`PVCOMP <pvcomp>` [MCP89]
- 15: :ref:`PBSP <pbsp>` [G84:G98 G200:MCP77]
- 15: :ref:`PVLD <pvld>` [G98:G200 MCP77-]
- 16: :ref:`PTIMER <ptimer>`
- 17: :ref:`PVP2 <pvp2>` [G84:G98 G200:MCP77]
- 17: :ref:`PPDEC <ppdec>` [G98:G200 MCP77-]
- 20: :ref:`PFB <g80-pfb>`
- 21: :ref:`PGRAPH CHSW <g80-pfifo-chsw>` [G84-]
- 22: :ref:`PMPEG CHSW <g80-pfifo-chsw>` [G84-]
- 23: :ref:`PCOPY CHSW <g80-pfifo-chsw>` [GT215-]
- 24: :ref:`PVP2 CHSW <g80-pfifo-chsw>` [G84:G98 G200:MCP77]
- 24: :ref:`PPDEC CHSW <g80-pfifo-chsw>` [G98:G200 MCP77-]
- 25: :ref:`PCRYPT2 CHSW <g80-pfifo-chsw>` [G84:G98 G200:MCP77]
- 25: :ref:`PSEC CHSW <g80-pfifo-chsw>` [G98:G200 MCP77:GT215]
- 25: :ref:`PVCOMP CHSW <g80-pfifo-chsw>` [MCP89]
- 26: :ref:`PBSP CHSW <g80-pfifo-chsw>` [G84:G98 G200:MCP77]
- 26: :ref:`PVLD CHSW <g80-pfifo-chsw>` [G98:G200 MCP77-]
- 27: ??? [G84-]
- 28: ??? [G84-]
- 30: :ref:`PDISPLAY <pdisplay>`
- 31: ???

.. todo:: unknowns

On GF100+, the bits are:

- 0: ??? - alleged to be related to I2C
- 1: :ref:`PPPP <pppp>` [GF100:GM107]
- 2: :ref:`PXBAR <pxbar>`
- 3: :ref:`PMFB <pmfb>`
- 4: :ref:`PMEDIA <pmedia>` [GF100:GM107]
- 5: :ref:`PRING <pring>`
- 6: :ref:`PCOPY[0] <pcopy>`
- 7: :ref:`PCOPY[1] <pcopy>` [GF100:GM107]
- 8: :ref:`PFIFO <gf100-pfifo>`
- 12: :ref:`PGRAPH <gf100-pgraph>`
- 13: :ref:`PDAEMON <pdaemon>`
- 14: :ref:`PSEC <psec>` [GM107:]
- 15: :ref:`PVLD <pvld>` [GF100:GM107]
- 15: :ref:`PVDEC <pvdec>` [GM107:]
- 16: :ref:`PTIMER <ptimer>`
- 17: :ref:`PPDEC <ppdec>` [GF100:GM107]
- 18: :ref:`PVENC <pvenc>` [GK104-]
- 20: :ref:`PBFB <pbfb>`
- 21: :ref:`PCOPY[2] <pcopy>` [GK104-]
- 26: ??? allegedly zpw [GK104-]
- 27: ??? allegedly blg
- 28: :ref:`PCOUNTER <pcounter>`
- 29: :ref:`PFFB <pffb>`
- 30: :ref:`PDISPLAY <pdisplay>`
- 31: ??? allegedly isohub

GF100 also introduced SUBFIFO_ENABLE register:

.. reg:: 32 pmc-spoon-enable PSPOON enables

   Enables PFIFO's PSPOONs. Bit i corresponds to PSPOON[i]. See
   :ref:`GF100+ PFIFO <gf100-pspoon>` for details.

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
   - 17: PPDEC

   Comes up as all-1.

.. reg:: 32 pmc-fifo-eng-unk260 ??? related to PFIFO engines

   Single-bit registers, 6 of them.

.. todo:: RE these three


.. _pmc-intr:
.. _pdaemon-intr-pmc-daemon:

Interrupts
==========

Another thing that PMC handles is the top-level interrupt routing. On cards
earlier than GT215, PMC gets interrupt lines from all interested engines on
the card, aggregates them together, adds in an option to trigger a "software"
interrupt manually, and routes them to the PCI INTA pin. There is an enable
register, but it only allows one to enable/disable all hardware or all
software interrupts.

GT215 introduced fine-grained interrupt masking, as well as an option to route
interrupts to PDAEMON. The HOST interrupts have a new redirection stage in
PDAEMON [see :ref:`pdaemon-iredir`] - while normally routed to the PCI interrupt line,
they may be switched over to PDAEMON delivery when it so decides. As a side
effect of that, powering off PDAEMON will disable host interrupt delivery.
A subset of interrupt types can also be routed to NRHOST destination, which
is identical to HOST, but doesn't go through the PDAEMON redirection circuitry.

.. todo:: change all this duplication to indexing

.. reg:: 32 pmc-intr-host interrupt status - host

   Interrupt status. Bits 0-30 are hardware interrupts, bit 31 is software
   interrupt. 1 if the relevant input interrupt line is active and, for GT215+
   GPUs, enabled in INTR_MASK_*. Bits 0-30 are read-only, bit 31 can be
   written to set/clear the software interrupt. Bit 31 can only be set to 1 if
   software interrupts are enabled in INTR_MASK_*, except for NRHOST on GF100+,
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
   On NV1:GF100, 0 if the output line is active, 1 if inactive. On GF100+, 1 if
   active, 0 if inactive.

.. reg:: 32 pmc-intr-line-nrhost interrupt line status - non-redirectable host

   Like :obj:`pmc-intr-line-host`, but for NRHOST.

.. reg:: 32 pmc-intr-line-daemon interrupt line status - PDAEMON

   Like :obj:`pmc-intr-line-host`, but for DAEMON.

.. reg:: 32 pmc-intr-mask-host interrupt mask - host

   Interrupt mask. If a bit is set to 0 here, it'll be masked off to always-0
   in the INTR_* register, otherwise it'll be connected to the corresponding
   input interrupt line. For HOST and DAEMON, all interrupts can be enabled.
   For NRHOST on pre-GF100 cards, only input line #8 [PFIFO] can be enabled, for
   NRHOST on GF100+ cards all interrupts but the software interrupt can be
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

On pre-GT215, each PMC interrupt input is a single 0/1 line. On GT215+, some
inputs have a single line for all three outputs, while some others have 2
lines: one for HOST and DAEMON outputs, and one for NRHOST outuput.

The input interrupts are, for NV1:

- 0: :ref:`PAUDIO <nv1-paudio-intr>`
- 4: :ref:`PDMA <nv1-pdma-intr>`
- 8: :ref:`PFIFO <nv1-pfifo-intr>`
- 12: :ref:`PGRAPH <nv1-pgraph-intr>`
- 16: :ref:`PRM <nv1-prm-intr>`
- 20: :ref:`PTIMER <ptimer-intr>`
- 24: :ref:`PGRAPH's vblank interrupt <nv1-pgraph-intr-vblank>`
- 28: software

.. todo:: check

For NV3:

- 4: :ref:`PMEDIA <pmedia-intr>`
- 8: :ref:`PFIFO <nv1-pfifo-intr>`
- 12: :ref:`PGRAPH <nv3-pgraph>`
- 13: :ref:`PDMA <nv3-pdma>`
- 16: :ref:`PRAMDAC.VIDEO <pvideo-intr>`
- 20: :ref:`PTIMER <ptimer-intr>`
- 24: :ref:`PGRAPH's vblank interrupt <pcrtc-intr>`
- 28: :ref:`PBUS <pbus-intr>`
- 31: software

For NV4:G80:

- 0: :ref:`PVPE <pvpe-intr>` [NV17:NV20 and NV25:G80]
- 4: :ref:`PMEDIA <pmedia-intr>`
- 8: :ref:`PFIFO <nv4-pfifo-intr>`
- 12: :ref:`PGRAPH <nv4-pgraph-intr>`
- 16: :ref:`PRAMDAC.VIDEO <pvideo-intr>` [NV4:NV10] or :ref:`PVIDEO <pvideo-intr>` [NV10:G80]
- 20: :ref:`PTIMER <ptimer-intr>`
- 24: :ref:`PCRTC <pcrtc-intr>`
- 25: :ref:`PCRTC2 <pcrtc-intr>` [NV17:NV20 and NV25:G80]
- 28: :ref:`PBUS <pbus-intr>`
- 31: software

For G80:GF100:

- 0: :ref:`PVPE <pvpe-intr>` [G80:G98 G200:MCP77]
- 0: :ref:`PPPP <pppp-falcon>` [G98:G200 MCP77-]
- 4: :ref:`PMEDIA <pmedia-intr>`
- 8: :ref:`PFIFO <g80-pfifo-intr>` - has separate NRHOST line on GT215+
- 9: ??? [GT215?-]
- 11: ??? [GT215?-]
- 12: :ref:`PGRAPH <g80-pgraph-intr>`
- 13: ??? [GT215?-]
- 14: :ref:`PCRYPT2 <pcrypt2-intr>` [G84:G98 G200:MCP77]
- 14: :ref:`PSEC <psec-falcon>` [G98:G200 MCP77:GT215]
- 14: :ref:`PVCOMP <pvcomp-falcon>` [MCP89-]
- 15: :ref:`PBSP <pbsp-intr>` [G84:G98 G200:MCP77]
- 15: :ref:`PVLD <pvld-falcon>` [G98:G200 MCP77-]
- 16: ??? [GT215?-]
- 17: :ref:`PVP2 <pvp2-intr>` [G84:G98 G200:MCP77]
- 17: :ref:`PPDEC <ppdec-falcon>` [G98:G200 MCP77-]
- 18: :ref:`PDAEMON [GT215-] <pdaemon-falcon>`
- 19: :ref:`PTHERM [GT215-] <ptherm-intr>`
- 20: :ref:`PTIMER <ptimer-intr>`
- 21: :ref:`PNVIO's GPIO interrupts <g80-gpio-intr>`
- 22: :ref:`PCOPY <pcopy-falcon>`
- 26: :ref:`PDISPLAY <pdisplay-intr>`
- 27: ??? [GT215?-]
- 28: :ref:`PBUS <pbus-intr>`
- 29: :ref:`PPCI <ppci-intr>` [G84-]
- 31: software

.. todo:: figure out unknown interrupts. They could've been introduced much
   earlier, but we only know them from bitscanning the INTR_MASK regs. on GT215+.

For GF100+:

- 0: :ref:`PPPP <pppp-falcon>` - has separate NRHOST line [GF100:GM107]
- 4: :ref:`PMEDIA <pmedia-intr>` [GF100:GM107]
- 5: PCOPY[0] [:ref:`GF100 <pcopy-falcon>`, :ref:`GK104 <pcopy-intr>`] - has separate NRHOST line
- 6: PCOPY[1] [:ref:`GF100 <pcopy-falcon>`, :ref:`GK104 <pcopy-intr>`] - has separate NRHOST line
- 7: :ref:`PCOPY[2] <pcopy-intr>` [GK104-] - has separate NRHOST line
- 8: :ref:`PFIFO <gf100-pfifo-intr>`
- 9: ??? allegedly remapper
- 12: :ref:`PGRAPH <gf100-pgraph-intr>` - has separate NRHOST line
- 13: :ref:`PBFB <pbfb-intr>`
- 15: :ref:`PSEC <psec-falcon>` - has separate NRHOST line [GM107:]
- 15: :ref:`PVLD <pvld-falcon>` - has separate NRHOST line [GF100:GM107]
- 16: :ref:`PVENC <pvenc-falcon>` [GK104-] - has separate NRHOST line
- 17: :ref:`PPDEC <ppdec-falcon>` - has separate NRHOST line [GF100:GM107]
- 17: :ref:`PVDEC <pvdec-falcon>` - has separate NRHOST line [GM107:]
- 18: :ref:`PTHERM <ptherm-intr>`
- 19: ??? allegedly HDA codec [GF119-]
- 20: :ref:`PTIMER <ptimer-intr>`
- 21: :ref:`PNVIO's GPIO interrupts <g80-gpio-intr>`
- 23: ??? allegedly dfd
- 24: :ref:`PDAEMON <pdaemon-falcon>`
- 25: :ref:`PMFB <pmfb-intr>`
- 26: :ref:`PDISPLAY <pdisplay-intr>`
- 27: :ref:`PFFB <pffb-intr>`
- 28: :ref:`PBUS <pbus-intr>` - has separate NRHOST line
- 29: :ref:`PPCI <ppci-intr>`
- 30: :ref:`PRING <pring-intr>`
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

The VRAM hidden area functionality got silently nuked on GF100+ GPUs. The
registers are still present, but they don't do anything.
