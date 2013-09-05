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

The PMC's MMIO range is 0x0000:0x1000. It is always active.


.. _pmc-mmio:

MMIO register list
==================

============== ========= ====================== ====================
Address        Variants  Name                   Description
============== ========= ====================== ====================
000000         all       ID                     :ref:`card identification <pmc-mmio-id>`
000004         NV11-     ENDIAN                 :ref:`endian switch <pmc-mmio-endian>`
000008         NV92-     BOOT_2                 :ref:`??? <pmc-mmio-id>`
000100         all       INTR_HOST              :ref:`interrupt status - host <pmc-mmio-intr>`
000104         NVA3-     INTR_NRHOST            :ref:`interrupt status - non-redirectable host <pmc-mmio-intr>`
000108         NVA3-     INTR_DAEMON            :ref:`interrupt status - PDAEMON <pmc-mmio-intr>`
000100         all       INTR_EN_HOST           :ref:`interrupt enable - host <pmc-mmio-intr>`
000144         NVA3-     INTR_EN_NRHOST         :ref:`interrupt enable - non-redirectable host <pmc-mmio-intr>`
000148         NVA3-     INTR_EN_DAEMON         :ref:`interrupt enable - PDAEMON <pmc-mmio-intr>`
000160         all       INTR_LN_HOST           :ref:`interrupt line state - host <pmc-mmio-intr>`
000164         NVA3-     INTR_LN_NRHOST         :ref:`interrupt line state - non-redirectable host <pmc-mmio-intr>`
000168         NVA3-     INTR_LN_DAEMON         :ref:`interrupt line state - PDAEMON <pmc-mmio-intr>`
000200         all       ENABLE                 :ref:`engine master enable <pmc-mmio-enable>`
000204         NVC0-     SUBFIFO_ENABLE         :ref:`PSUBFIFO enables <pmc-mmio-enable>`
000208         NVC0-     ???                    :ref:`??? [related to enable?] <pmc-mmio-enable>`
00020c         NVC4-     ???                    :ref:`??? [related to enable?] <pmc-mmio-enable>`
000260:000274  NVC0-     ???                    :ref:`??? related to PFIFO engines <pmc-mmio-enable>`
000300         NV17-     VRAM_HIDE_LOW          :ref:`VRAM hidden area low address and enable <pmc-mmio-vram-hide>`
000304         NV17-     VRAM_HIDE_HIGH         :ref:`VRAM hidden area high address <pmc-mmio-vram-hide>`
000640         NVA3-     INTR_MASK_HOST         :ref:`interrupt mask - host <pmc-mmio-intr>`
000644         NVA3-     INTR_MASK_NRHOST       :ref:`interrupt mask - non-redirectable host <pmc-mmio-intr>`
000648         NVA3-     INTR_MASK_PDAEMON      :ref:`interrupt mask - PDAEMON <pmc-mmio-intr>`
000a00         NV94-     NEW_ID                 :ref:`card identification <pmc-mmio-id>`
============== ========= ====================== ====================

.. todo:: figure out 208, 20c, 260


.. _pmc-mmio-id:

Card identification
===================

The main register used to identify the card is the ID register. However,
the ID register has different formats depending on the chipset family:

MMIO 0x000000: ID [NV01:NV04]
  - bits 0-3: minor revision.
  - bits 4-7: major revision.
    These two bitfields together are also visible as PCI revision. For
    NV03, revisions equal or higher than 0x20 mean NV03T.
  - bits 8-11: implementation - always 1 except on NV02
  - bits 12-15: always 0
  - bits 16-19: chipset - 1 is NV01, 2 is NV02, 3 is NV03 or NV03T
  - bits 20-27: always 0
  - bits 28-31: foundry - 0 is SGS, 1 is Helios, 2 is TMSC

MMIO 0x000000: ID [NV04:NV10]
  - bits 0-3: ???
  - bits 4-11: always 0
  - bits 12-15: architecture - always 4
  - bits 16-19: minor revision
  - bits 20-23: major revision - 0 is NV04, 1 and 2 are NV05.
    These two bitfields together are also visible as PCI revision.
  - bits 24-27: always 0
  - bits 28-31: foundry - 0 is SGS, 1 is Helios, 2 is TMSC

MMIO 0x000000: ID [NV10-]
  - bits 0-7: stepping
  - bits 16-19: device id [NV10:NV92]
  - bits 15-19: device id [NV92:NVD8]
  - bits 12-19: device id [NVD9-]
    The value of this bitfield is equal to low 4, 5, or 6 bits of the PCI
    device id. The bitfield size and position changed between cards due to
    varying amount of changeable bits. See `<io/pstraps.txt>`_ and
    :ref:`chipsets` for more details.
  - bits 20-27: chipset id.
    This is THE chipset id that comes after "NV". See chipsets.txt for the
    list.
  - bits 28-31: ???

.. todo:: unk bitfields

NV92[?] introduced another identification register in PMC, with unknown
purpose:

MMIO 0x000008: BOOT_2 [NV92?-]
  ???
 
.. todo:: what is this? when was it introduced? seen non-0 on at least NV92

NV94 introduced a new identification register with rearranged bitfields:

MMIO 0x000a00: NEW_ID
  - bits 0-7: device id
  - bits 8-11: same value as BOOT_2 register
  - bits 12-19: stepping
  - bits 20-27: chipset id

.. todo:: there are cards where the steppings don't match
   between registers - does this mean something or is it just
   a random screwup?


.. _pmc-mmio-endian:

Endian switch
=============

PMC also contains the endian switch register. The endian switch can be set to
either little or big endian, and affects all accesses to BAR0 and, if present,
BAR2/BAR3 - see bus/bars.txt for more details. It is controlled by the ENDIAN
register:

MMIO 0x000004: ENDIAN [NV11-]
  When read, returns 0x01000001 if in big-endian mode, 0 if in little-endian
  mode. When written, if bit 24 of the written value is 1, flips the endian
  switch to the opposite value, otherwise does nothing.

The register operates in such idiosyncratic way because it is itself affected
by the endian switch - thus the read value was chosen to be unaffected by
wrong endian setting, while write behavior was chosen so that writing "1" in
either endianness will switch the card to that endianness.

This register and the endian switch don't exist on pre-NV11 cards - they're
always little-endian.

Note that this switch is also used by NV50+ PFIFO as its default endianness
- see `<fifo/nv50-pfifo.txt>`_ for details.

The MMIO areas containing aliases of 8-bit VGA registers are unaffected by
this switch, despite being in BAR0.


.. _pmc-mmio-enable:

Engine enables
==============

PMC contains the main engine enable register, which is used to turn whole
engines on and off:

MMIO 0x000200: ENABLE
  When given bit is set to 0, the corresponding engine is disabled, when set
  to 1, it is enabled. Most engines disappear from MMIO space and reset to
  default state when disabled.

On NV01, the bits are:

- 0: :ref:`PAUDIO <nv01-paudio>`
- 4: PDMA [memory/nv01-pdma.txt] and :ref:`PTIMER <ptimer>`
- 8: PFIFO [fifo/nv01-pfifo.txt]
- 12: PGRAPH [graph/nv01-pgraph.txt]
- 16: :ref:`PRM <nv01-prm>`
- 24: PFB [display/nv01/pfb.txt]

On NV03:NV04, the bits are:

- 0: ??? [XXX]
- 4: PMEDIA [io/pmedia.txt]
- 8: PFIFO [fifo/nv01-pfifo.txt]
- 12: PGRAPH [graph/nv03-pgraph.txt] and PDMA [graph/nv03-pdma.txt]
- 16: :ref:`PTIMER <ptimer>`
- 20: PFB [memory/nv03-pfb.txt]
- 24: CRTC [display/nv03/vga.txt]
- 28: PRAMDAC.VIDEO [display/nv03/pramdac.txt]

On NV04:NV50, the bits are:

- 0: ??? - alleged to be related to I2C [NV10-] [XXX]
- 1: VPE [vdec/vpe/intro.txt] [NV17-]
- 4: PMEDIA [io/pmedia.txt]
- 8: PFIFO [fifo/nv04-pfifo.txt]
- 12: PGRAPH [graph/nv04-pgraph.txt, graph/nv10-pgraph.txt, graph/nv20-pgraph.txt, graph/nv40-pgraph.txt]
- 13: PGRAPH CS??? apparently exists on some late NV4x... [graph/nv40-pgraph.txt] [NV4?-]
- [XXX: figure out the CS thing, figure out the variants. Known not to exist on NV40, NV43, NV44, NV49]
- 16: :ref:`PTIMER <ptimer>`
- 20: PFB [memory/nv03-pfb.txt, memory/nv10-pfb.txt, memory/nv40-pfb.txt, memory/nv44-pfb.txt]
- 24: PCRTC [display/nv03/vga.txt]
- 25: PCRTC2 [display/nv03/vga.txt] [NV11-]
- 26: PTV [display/nv03/ptv.txt] [NV17:NV20, NV25:NV50]
- 28: PRAMDAC.VIDEO [display/nv03/pramdac.txt] [NV04:NV10] or PVIDEO [display/nv03/pvideo.txt] [NV10:NV50]

On NV50:NVC0, the bits are:

- 0: ??? - alleged to be related to I2C
- 1: VPE [vdec/vpe/intro.txt] [NV50:NV98 NVA0:NVAA]
- 1: PPPP [vdec/vp3/pppp.txt] [NV98:NVA0 NVAA-]
- 4: PMEDIA [io/pmedia.txt]
- 8: PFIFO [fifo/nv50-pfifo.txt]
- 12: PGRAPH [graph/nv50-pgraph.txt]
- 13: PCOPY [fifo/pcopy.txt] [NVA3-]
- 14: PCRYPT2 [vdec/vp2/pcrypt2.txt] [NV84:NV98 NVA0:NVAA]
- 14: PCRYPT3 [vdec/vp3/pcrypt3.txt] [NV98:NVA0 NVAA:NVA3]
- 14: PVCOMP [vdec/pvcomp.txt] [NVAF]
- 15: PBSP [vdec/vp2/pbsp.txt] [NV84:NV98 NVA0:NVAA]
- 15: PVLD [vdec/vp3/pvld.txt] [NV98:NVA0 NVAA-]
- 16: :ref:`PTIMER <ptimer>`
- 17: PVP2 [vdec/vp2/pvp2.txt] [NV84:NV98 NVA0:NVAA]
- 17: PVDEC [vdec/vp3/pvdec.txt] [NV98:NVA0 NVAA-]
- 20: PFB [memory/nv50-pfb.txt]
- 21: PGRAPH CHSW [NV84-] [fifo/nv50-pfifo.txt]
- 22: PMPEG CHSW [NV84-]
- 23: PCOPY CHSW [NVA3-]
- 24: PVP2 CHSW [NV84:NV98 NVA0:NVAA] [fifo/nv50-pfifo.txt]
- 24: PVDEC CHSW [NV98:NVA0 NVAA-] [fifo/nv50-pfifo.txt]
- 25: PCRYPT2 CHSW [NV84:NV98 NVA0:NVAA] [fifo/nv50-pfifo.txt]
- 25: PCRYPT3 CHSW [NV98:NVA0 NVAA:NVA3] [fifo/nv50-pfifo.txt]
- 25: PVCOMP CHSW [NVAF] [fifo/nv50-pfifo.txt]
- 26: PBSP CHSW [NV84:NV98 NVA0:NVAA] [fifo/nv50-pfifo.txt]
- 26: PVLD CHSW [NV98:NVA0 NVAA-] [fifo/nv50-pfifo.txt]
- 27: ??? [NV84-]
- 28: ??? [NV84-]
- 30: PDISPLAY [display/nv50/pdisplay.txt]
- 31: ???

.. todo:: unknowns

On NVC0+, the bits are:

- 0: ??? - alleged to be related to I2C
- 1: PPPP [vdec/vp3/pppp.txt]
- 2: PXBAR [memory/nvc0-pxbar.txt]
- 3: PMFB [memory/nvc0-pmfb.txt]
- 4: PMEDIA [io/pmedia.txt]
- 5: PIBUS [bus/pibus.txt]
- 6: PCOPY[0] [fifo/pcopy.txt]
- 7: PCOPY[1] [fifo/pcopy.txt]
- 8: PFIFO [fifo/nvc0-pfifo.txt]
- 12: PGRAPH [graph/nvc0-pgraph.txt]
- 13: PDAEMON [pm/pdaemon.txt]
- 15: PVLD [vdec/vp3/pvld.txt]
- 16: :ref:`PTIMER <ptimer>`
- 17: PVDEC [vdec/vp3/pvdec.txt]
- 18: PVENC [NVE4-] [vdec/pvenc.txt]
- 20: PBFB [memory/nvc0-pbfb.txt]
- 21: PCOPY[2] [NVE4-] [fifo/pcopy.txt]
- 26: ??? [NVE4-]
- 27: ???
- 28: PCOUNTER [pcounter/intro.txt]
- 29: PFFB [memory/nvc0-pffb.txt]
- 30: PDISPLAY [display/nv50/pdisplay.txt]
- 31: ???

NVC0 also introduced SUBFIFO_ENABLE register:

MMIO 0x000204: SUBFIFO_ENABLE
  Enables PFIFO's PSUBFIFOs. Bit i corresponds to PSUBFIFO[i]. See
  `<fifo/nvc0-pfifo.txt>`_ for details.

There are also two other registers looking like ENABLE, but with seemingly
no effect and currently unknown purpose:

MMIO 0x000208: ??? [NVC0-]
  Has the same bits as ENABLE, comes up as all-1 on boot, except for PDISPLAY
  bit which comes up as 0.

MMIO 0x00020c: ??? [NVC4-]
  Has bits which correspond to PFIFO engines in ENABLE, ie.

  - 1: PPPP
  - 6: PCOPY[0]
  - 7: PCOPY[1]
  - 12: PGRAPH
  - 15: PVLD
  - 17: PVDEC

  Comes up as all-1.

.. todo:: RE these two

.. todo:: describe 260


.. _pmc-mmio-intr:
.. _pmc-intr:

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
PDAEMON [see `<pm/pdaemon.txt>`_] - while normally routed to the PCI interrupt line,
they may be switched over to PDAEMON delivery when it so decides. As a side
effect of that, powering off PDAEMON will disable host interrupt delivery.
A subset of interrupt types can also be routed to NRHOST destination, which
is identical to HOST, but doesn't go through the PDAEMON redirection circuitry.

MMIO 0x000100: INTR_HOST
MMIO 0x000104: INTR_NRHOST [NVA3-]
MMIO 0x000108: INTR_DAEMON [NVA3-]
  Interrupt status. Bits 0-30 are hardware interrupts, bit 31 is software
  interrupt. 1 if the relevant input interrupt line is active and, for NVA3+
  chipsets, enabled in INTR_MASK_*. Bits 0-30 are read-only, bit 31 can be
  written to set/clear the software interrupt. Bit 31 can only be set to 1 if
  software interrupts are enabled in INTR_MASK_*, except for NRHOST on NVC0+,
  where it works even if masked.

MMIO 0x000140: INTR_EN_HOST
MMIO 0x000144: INTR_EN_NRHOST [NVA3-]
MMIO 0x000148: INTR_EN_DAEMON [NVA3-]
  - bit 0: hardware interrupt enable - if 1, and any of bits 0-30 of INTR_* are
    active, the corresponding output interrupt line will be asserted.
  - bit 1: software interrupt enable - if 1, bit 31 of INTR_* is active, the
    corresponding output interrupt line will be asserted.

MMIO 0x000160: INTR_LN_HOST
MMIO 0x000164: INTR_LN_NRHOST [NVA3-]
MMIO 0x000168: INTR_LN_DAEMON [NVA3-]
  Provides a way to peek at the status of corresponding output interrupt line.
  On NV01:NVC0, 0 if the output line is active, 1 if inactive. On NVC0+, 1 if
  active, 0 if inactive.

MMIO 0x000640: INTR_MASK_HOST [NVA3-]
MMIO 0x000644: INTR_MASK_NRHOST [NVA3-]
MMIO 0x000648: INTR_MASK_DAEMON [NVA3-]
  Interrupt mask. If a bit is set to 0 here, it'll be masked off to always-0
  in the INTR_* register, otherwise it'll be connected to the corresponding
  input interrupt line. For HOST and DAEMON, all interrupts can be enabled.
  For NRHOST on pre-NVC0 cards, only input line #8 [PFIFO] can be enabled, for
  NRHOST on NVC0+ cards all interrupts but the software interrupt can be
  enabled - however in this case software interrupt works even without being
  enabled.

The HOST and NRHOST output interrupt lines are connected to the PCI INTA pin
on the card. HOST goes through PDAEMON's HOST interrupt masking circuitry
[IHM], while NRHOST doesn't. DAEMON goes to PDAEMON's falcon interrupt line #10
[PMC_DAEMON].

On pre-NVA3, each PMC interrupt input is a single 0/1 line. On NVA3+, some
inputs have a single line for all three outputs, while some others have 2
lines: one for HOST and DAEMON outputs, and one for NRHOST outuput.

The input interrupts are, for NV01:

- 0: :ref:`PAUDIO <nv01-paudio-intr>`
- 4: PDMA [memory/nv01-pdma.txt]
- 8: PFIFO [fifo/nv01-pfifo.txt]
- 12: PGRAPH [graph/nv01-pgraph.txt]
- 16: :ref:`PRM <nv01-prm-intr>`
- 20: :ref:`PTIMER <ptimer-intr>`
- 24: PGRAPH's vblank interrupt [graph/nv01-pgraph.txt]
- 28: software

.. todo:: check

For NV03:

- 4: PMEDIA [io/pmedia.txt]
- 8: PFIFO [fifo/nv01-pfifo.txt]
- 12: PGRAPH [graph/nv03-pgraph.txt]
- 13: PDMA [graph/nv03-pdma.txt]
- 16: PRAMDAC.VIDEO [display/nv03/pramdac.txt]
- 20: :ref:`PTIMER <ptimer-intr>`
- 24: PGRAPH's vblank interrupt [graph/nv03-pgraph.txt, display/nv03/vga.txt]
- 28: :ref:`PBUS <pbus-intr>`
- 31: software

For NV04:NV50:

- 0: VPE [vdec/vpe/intro.txt] [NV17:NV20 and NV25:NV50]
- 4: PMEDIA [io/pmedia.txt]
- 8: PFIFO [fifo/nv04-pfifo.txt]
- 12: PGRAPH [graph/nv04-pgraph.txt, graph/nv10-pgraph.txt, graph/nv20-pgraph.txt, graph/nv40-pgraph.txt]
- 16: PRAMDAC.VIDEO [display/nv03/pramdac.txt] [NV04:NV10] or PVIDEO [display/nv03/pvideo.txt] [NV10:NV50]
- 20: :ref:`PTIMER <ptimer-intr>`
- 24: PCRTC [display/nv03/vga.txt]
- 25: PCRTC2 [display/nv03/vga.txt] [NV17:NV20 and NV25:NV50]
- 28: :ref:`PBUS <pbus-intr>`
- 31: software

For NV50:NVC0:

- 0: VPE [vdec/vpe/intro.txt] [NV50:NV98 NVA0:NVAA]
- 0: PPPP [vdec/vp3/pppp.txt] [NV98:NVA0 NVAA-]
- 4: PMEDIA [io/pmedia.txt]
- 8: PFIFO [fifo/nv50-pfifo.txt] - has separate NRHOST line on NVA3+
- 9: ??? [NVA3?-]
- 11: ??? [NVA3?-]
- 12: PGRAPH [graph/nv50-pgraph.txt]
- 13: ??? [NVA3?-]
- 14: PCRYPT2 [vdec/vp2/pcrypt2.txt] [NV84:NV98 NVA0:NVAA]
- 14: PCRYPT3 [vdec/vp3/pcrypt3.txt] [NV98:NVA0 NVAA:NVA3]
- 14: PVCOMP [vdec/pvcomp.txt] [NVAF-]
- 15: PBSP [vdec/vp2/pbsp.txt] [NV84:NV98 NVA0:NVAA]
- 15: PVLD [vdec/vp3/pvld.txt] [NV98:NVA0 NVAA-]
- 16: ??? [NVA3?-]
- 17: PVP2 [vdec/vp2/pvp2.txt] [NV84:NV98 NVA0:NVAA]
- 17: PVDEC [vdec/vp3/pvdec.txt] [NV98:NVA0 NVAA-]
- 18: PDAEMON [pm/pdaemon.txt] [NVA3-]
- 19: PTHERM [pm/ptherm.txt] [NVA3-]
- 20: :ref:`PTIMER <ptimer-intr>`
- 21: PNVIO's GPIO interrupts [io/pnvio.txt]
- 22: PCOPY [fifo/pcopy.txt]
- 26: PDISPLAY [display/nv50/pdisplay.txt]
- 27: ??? [NVA3?-]
- 28: :ref:`PBUS <pbus-intr>`
- 29: PPCI [bus/pci.txt] [NV84-]
- 31: software

.. todo:: figure out unknown interrupts. They could've been introduced much
   earlier, but we only know them from bitscanning the INTR_MASK regs. on NVA3+.

For NVC0+:

- 0: PPPP [vdec/vp3/pppp.txt] - has separate NRHOST line
- 4: PMEDIA [io/pmedia.txt]
- 5: PCOPY[0] [fifo/pcopy.txt] - has separate NRHOST line
- 6: PCOPY[1] [fifo/pcopy.txt] - has separate NRHOST line
- 7: PCOPY[2] [NVE4-] [fifo/pcopy.txt] - has separate NRHOST line
- 8: PFIFO [fifo/nvc0-pfifo.txt]
- 9: ???
- 12: PGRAPH [graph/nvc0-pgraph.txt] - has separate NRHOST line
- 13: PBFB [memory/nvc0-pbfb.txt]
- 15: PVLD [vdec/vp3/pvld.txt] - has separate NRHOST line
- 16: PVENC [NVE4-] [vdec/pvenc.txt] - has separate NRHOST line
- 17: PVDEC [vdec/vp3/pvdec.txt] - has separate NRHOST line
- 18: PTHERM [pm/ptherm.txt]
- 19: ??? [NVD9-]
- 20: :ref:`PTIMER <ptimer-intr>`
- 21: PNVIO's GPIO interrupts [io/pnvio.txt]
- 23: ???
- 24: PDAEMON [pm/pdaemon.txt]
- 25: PMFB [memory/nvc0-pmfb.txt]
- 26: PDISPLAY [display/nv50/pdisplay.txt]
- 27: PFFB [memory/nvc0-pffb.txt]
- 28: :ref:`PBUS <pbus-intr>` - has separate NRHOST line
- 29: PPCI [bus/pci.txt]
- 30: PIBUS [bus/pibus.txt]
- 31: software

.. todo:: unknowns


.. _pmc-mmio-vram-hide:

VRAM hidden area
================

NV17/NV20 added a feature to disable host reads through selected range of
VRAM. The registers are:

MMIO 0x000300: VRAM_HIDE_LOW
  - bits 0-28: address of start of the hidden area. bits 0-1 are ignored, the
    area is always 4-byte aligned.
  - bit 31: hidden area enabled

MMIO 0x000304: VRAM_HIDE_HIGH
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
