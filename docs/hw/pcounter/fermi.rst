.. _pcounter-signal-gf100:

==============
Fermi+ signals
==============

.. contents::

.. todo:: convert

GF100
=====

HUB domain 2:

- source 0: CTXCTL

  - 0x18: ???
  - 0x1b: ???
  - 0x22-0x27: CTXCTL.USER

- source 1: ???

  - 0x2e-0x2f: ???


HUB domain 6:

- source 1: DISPATCH

  - 0x01-0x06: DISPATCH.MUX

- source 8: CCACHE

  - 0x08-0x0f: CCACHE.MUX

- source 4: UNK6000

  - 0x28-0x2f: UNK6000.MUX

- source 2:

  - 0x36: ???

- source 5: UNK5900

  - 0x39-0x3c: UNK5900.MUX

- source 7: UNK7800

  - 0x42: UNK7800.MUX

- source 0: UNK5800

  - 0x44-0x47: UNK5800.MUX

- source 6:

  - 0x4c: ???


GPC domain 0:

- source 0x16:

  - 0x02-0x09: GPC.TPC.L1.MUX

- source 0x19: TEX.MUX_C

  - 0x0b-0x12: GPC.TPC.TEX.MUX_C

- source 0: CCACHE.MUX_A

  - 0x15-0x19: GPC.CCACHE.MUX_A

- source 5:

  - 0x1a-0x1f: GPC.UNKC00.MUX

- source 0x14:

  - 0x21-0x28: GPC.TPC.UNK400.MUX

- source 0x17:

  - 0x31-0x38: GPC.TPC.MP.MUX

- source 0x13: TPC.UNK500

  - 0x3a-0x3c: TPC.UNK500.MUX

- source 0xa: PROP

  - 0x40-0x47: GPC.PROP.MUX

- source 0x15: POLY

  - 0x48-0x4d: POLY.MUX

- source 0x11: FFB.MUX_B

  - 0x4f-0x53: GPC.FFB.MUX_B

- source 0xe: ESETUP

  - 0x54-0x57: GPC.ESETUP.MUX

- source 0x1a:

  - 0x5b-0x5e: GPC.TPC.TEX.MUX_A

- source 0x18:

  - 0x61-0x64: GPC.TPC.TEX.MUX_B

- source 0xb: UNKB00

  - 0x66-0x68: GPC.UNKB00.MUX

- source 0xc: UNK600

  - 0x6a: GPC.UNK600.MUX

- source 3: ???

  - 0x6e: ???

- source 8: FFB.MUX_A

  - 0x72: ???
  - 0x74: ???

- source 4:

  - 0x76-0x78: GPC.UNKD00.MUX

- source 6:

  - 0x7c-0x7f: GPC.UNKC80.MUX

- source 0xd: UNK380

  - 0x81-0x83: GPC.UNK380.MUX

- source 0x12:

  - 0x84-0x87: GPC.UNKE00.MUX

- source 0xf: UNK700

  - 0x88-0x8b: GPC.UNK700.MUX

- source 1: CCACHE.MUX_B

  - 0x8e: GPC.CCACHE.MUX_B

- source 0x1c:

  - 0x91-0x93: GPC.UNKF00.MUX

- source 0x10: UNK680

  - 0x95: GPC.UNK680.MUX

- source 0x1b: TPC.UNK300

  - 0x98-0x9b: MUX

- source 2: GPC.CTXCTL

  - 0x9c: ???
  - 0xa1-0xa2: GPC.CTXCTL.TA
  - 0xaf-0xba: GPC.CTXCTL.USER

- source 9: ???

  - 0xbf: ???


PART domain 1:

- source 1: CROP.MUX_A

  - 0x00-0x0f: CROP.MUX_A

- source 2: CROP.MUX_B

  - 0x10-0x16: CROP.MUX_B

- source 3: ZROP

  - 0x18-0x1c: ZROP.MUX_A

  - 0x23: ZROP.MUX_B

- source 0: ???

  - 0x27: ???


GF116 signals
=============

[XXX: figure out what the fuck is going on]

HUB domain 0:

- source 0: ???
- source 1: ???

  - 0x01-0x02: ???


HUB domain 1:

- source 0: ???

  - 0x00-0x02: ???

- source 1: ???

- source 2: ???

  - 0x13-0x14: ???

- source 3: ???

  - 0x16: ???


HUB domain 2:

- source 0: CTXCTL [?]

  - 0x18: CTXCTL ???
  - 0x22-0x25: CTXCTL USER_0..USER_5

- source 1: ???

  - 0x2e-0x2f: ???

- 2: PDAEMON

  - 0x14,0x15: PDAEMON PM_SEL_2,3
  - 0x2c: PDAEMON PM_SEL_0
  - 0x2d: PDAEMON PM_SEL_1
  - 0x30: PDAEMON ???


HUB domain 3:

- source 0: PCOPY[0].???

  - 0x00: ???
  - 0x02: ???
  - 0x38: PCOPY[0].SRC0 ???

- source 1: PCOPY[0].FALCON

  - 0x17,0x18: PM_SEL_2,3
  - 0x2e: PCOPY[0].FALCON ???
  - 0x39: PCOPY[0].FALCON ???

- source 2: PCOPY[0].???

  - 0x12: ???
  - 0x3a: PCOPY[0].SRC2 ???

- source 3: PCOPY[1].???

  - 0x05-0x07: ???
  - 0x3b: PCOPY[1].SRC3 ???

- source 4: PCOPY[1].FALCON

  - 0x19,0x1a: PM_SEL_2,3
  - 0x34: PCOPY[1].FALCON ???
  - 0x3c: PCOPY[1].FALCON ???

- source 5: PCOPY[1].???

  - 0x14: ???
  - 0x16: ???
  - 0x3d: PCOPY[1].SRC5 ???

- source 6: PVDEC.???

  - 0x0c: ???
  - 0x22: ???
  - 0x24: ???
  - 0x3e: ???

- source 7: PPPP.???

  - 0x0a: ???
  - 0x1d: ???
  - 0x1f: ???
  - 0x3f: ???

- source 8: PVLD.???

  - 0x0e-0x10: ???
  - 0x27: ???
  - 0x29: ???
  - 0x40: ???


HUB domain 4:
 - 0: PVDEC.???
 - 1: PVDEC.FALCON
 - 2: PPPP.???
 - 3: PPPP.FALCON
 - 4: PVLD.???
 - 5: PVLD.FALCON

HUB domain 4 signals:
 - 0x00-0x03: PPPP.SRC2 ???
 - 0x06-0x07: PVDEC.SRC0 ???
 - 0x09: PVLD.SRC4 ???
 - 0x0b: PVLD.SRC4 ???
 - 0x0c,0x0d: PPPP.FALCON PM_SEL_2,3
 - 0x0e,0x0f: PVDEC.FALCON PM_SEL_2,3
 - 0x10,0x11: PVLD.FALCON PM_SEL_2,3
 - 0x16-0x17: PPPP.FALCON ???
 - 0x1c-0x1d: PVDEC.FALCON ???
 - 0x1e: PVLD.FALCON ???
 - 0x24-0x25: PVDEC.SRC0 ???
 - 0x26: PVDEC.FALCON ???
 - 0x27: PPPP.SRC2 ???
 - 0x28: PPPP.FALCON ???
 - 0x29: PVLD.SRC4 ???
 - 0x2a: PVLD.FALCON ???


HUB domain 5 sources:
 - 0: ???

HUB domain 5 signals:
 - 0x00: SRC0 ???
 - 0x05-0x06: SRC0 ???
 - 0x09: SRC0 ???
 - 0x0c: SRC0 ???


HUB domain 6 sources:
 - 0: ???
 - 1: ???
 - 2: ???
 - 3: ???
 - 4: ???
 - 5: ???
 - 6: ???
 - 7: ???
 - 8: ???

HUB domain 6 signals:
 - 0x0a-0x0b: SRC8 ???
 - 0x36: SRC2 ???
 - 0x39: SRC5 ???
 - 0x45: SRC0 ???
 - 0x47: SRC0 ???
 - 0x4c: SRC6 ???
