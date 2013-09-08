.. _pcounter-signal-nvc0:

=============
NVC0- signals
=============

.. contents::

.. todo:: convert

::

[XXX: write me]

= NVCF signals =

[XXX: figure out what the fuck is going on]

HUB domain 0 sources:
 - 0: ???
 - 1: ???

HUB domain 0 signals:
 - 0x01: SRC1 ???
 - 0x02: SRC1 ???

HUB domain 1 sources:
 - 0: ???
 - 1: ???
 - 2: ???
 - 3: ???

HUB domain 1 signals:
 - 0x00-0x02: SRC0 ???
 - 0x13-0x14: SRC2 ???
 - 0x16: SRC3 ???

HUB domain 2 sources:
 - 0: CTXCTL [?]
 - 1: ???
 - 2: PDAEMON

HUB domain 2 signals:
 - 0x14,0x15: PDAEMON PM_SEL_2,3
 - 0x18: CTXCTL ???
 - 0x22-0x25: CTXCTL USER_0..USER_5
 - 0x2c: PDAEMON PM_SEL_0
 - 0x2d: PDAEMON PM_SEL_1
 - 0x2e-0x2f: SRC1 ???
 - 0x30: PDAEMON ???

HUB domain 3 sources:
 - 0: PCOPY[0].???
 - 1: PCOPY[0].FALCON
 - 2: PCOPY[0].???
 - 3: PCOPY[1].???
 - 4: PCOPY[1].FALCON
 - 5: PCOPY[1].???
 - 6: PVDEC.???
 - 7: PPPP.???
 - 8: PVLD.???

HUB domain 3 signals:
 - 0x00: PCOPY[0].SRC0 ???
 - 0x02: PCOPY[0].SRC0 ???
 - 0x05-0x07: PCOPY[1].SRC3 ???
 - 0x0a: PPPP.SRC7 ???
 - 0x0c: PVDEC.SRC6 ???
 - 0x0e-0x10: PVLD.SRC8 ???
 - 0x12: PCOPY[0].SRC2 ???
 - 0x14: PCOPY[1].SRC5 ???
 - 0x16: PCOPY[1].SRC5 ???
 - 0x17,0x18: PCOPY[0].FALCON PM_SEL_2,3
 - 0x19,0x1a: PCOPY[1].FALCON PM_SEL_2,3
 - 0x1d: PPPP.SRC7 ???
 - 0x1f: PPPP.SRC7 ???
 - 0x22: PVDEC.SRC6 ???
 - 0x24: PVDEC.SRC6 ???
 - 0x27: PVLD.SRC8 ???
 - 0x29: PVLD.SRC8 ???
 - 0x2e: PCOPY[0].FALCON ???
 - 0x34: PCOPY[1].FALCON ???
 - 0x38: PCOPY[0].SRC0 ???
 - 0x39: PCOPY[0].FALCON ???
 - 0x3a: PCOPY[0].SRC2 ???
 - 0x3b: PCOPY[1].SRC3 ???
 - 0x3c: PCOPY[1].FALCON ???
 - 0x3d: PCOPY[1].SRC5 ???
 - 0x3e: PVDEC.SRC6 ???
 - 0x3f: PPPP.SRC7 ???
 - 0x40: PVLD.SRC8 ???

HUB domain 4 sources:
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

