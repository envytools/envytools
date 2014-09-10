.. _mmio:

====================
MMIO register ranges
====================

.. contents::


Introduction
============

.. todo:: write me


NV1 MMIO map
============

.. space:: 8 nv1-mmio 0x2000000 -
   0x0000000 PMC pmc
   0x0001000 PBUS pbus
   0x0002000 PFIFO nv1-pfifo
   0x0100000 PDMA nv1-pdma
   0x0101000 PTIMER nv1-ptimer
   0x0300000 PAUDIO nv1-paudio
   0x0400000 PGRAPH nv1-pgraph
   0x0410000[subc:8] UBETA nv1-ubeta
   0x0420000[subc:8] UROP nv1-urop
   0x0430000[subc:8] UCHROMA nv1-uchroma
   0x0440000[subc:8] UPLANE nv1-uplane
   0x0450000[subc:8] UCLIP nv1-uclip
   0x0460000[subc:8] UPATTERN nv1-upattern
   0x0480000[subc:8] UPOINT nv1-upoint
   0x0490000[subc:8] ULINE nv1-uline
   0x04a0000[subc:8] ULIN nv1-ulin
   0x04b0000[subc:8] UTRI nv1-utri
   0x04c0000[subc:8] URECT nv1-urect
   0x04d0000[subc:8] UTEXLIN nv1-utexlin
   0x04e0000[subc:8] UTEXQUAD nv1-utexquad
   0x0500000[subc:8] UBLIT nv1-ublit
   0x0510000[subc:8] UIFC nv1-uifc
   0x0520000[subc:8] UBITMAP nv1-ubitmap
   0x0530000[subc:8] UIFM nv1-uifm
   0x0540000[subc:8] UITM nv1-uitm
   0x05d0000[subc:8] UTEXLINBETA nv1-utexlinbeta
   0x05e0000[subc:8] UTEXQUADBETA nv1-utexquadbeta
   0x0600000 PFB nv1-pfb
   0x0602000 PRAM nv1-pram
   0x0604000 PRAMUNK1 nv1-pramunk1
   0x0605000 PCHIPID pchipid
   0x0606000 PRAMUNK2 nv1-pramunk2
   0x0608000 PSTRAPS nv1-pstraps
   0x0609000 PDAC nv1-pdac
   0x060a000 PEEPROM peeprom
   0x0610000 PROM nv1-prom
   0x0618000 PALT nv1-palt
   0x0640000 PRAMHT nv1-pramht
   0x0648000 PRAMFC nv1-pramfc
   0x0650000 PRAMRO nv1-pramro
   0x06c0000 PRM nv1-prm
   0x06d0000 PRMIO nv1-prmio
   0x06e0000 PRMFB nv1-prmfb
   0x0700000 PRAMIN nv1-pramin
   0x0800000[chid:0x80][subc:8] USER nv1-user
   0x1000000 FB nv1-fb


NV3:NV50 MMIO map
=================

.. space:: 8 nv3-mmio 0x1000000 -
   0x000000 PMC pmc
   0x001000 PBUS pbus
   0x002000 PFIFO nv1-pfifo NV3:NV4
   0x002000 PFIFO nv4-pfifo NV4:NV50
   0x004000 UNK004000 nv3-unk004000 NV3:NV4
   0x004000 UNK004000 nv34-unk004000 NV34:NV40
   0x004000 PCLOCK nv40-pclock NV40:NV50
   0x005000 UNK005000 nv4-unk005000 NV4:NV40,IGP4X
   0x006000 UNK006000 unk006000 NV20:NV34
   0x007000 PRMA prma
   0x008000 PVIDEO pvideo NV10:NV50
   0x009000 PTIMER nv3-ptimer
   0x00a000 PCOUNTER nv10-pcounter NV10:NV40
   0x00a000 PCOUNTER nv40-pcounter NV40:NV50
   0x00b000 PVPE pvpe NV17:NV20,NV30:NV50
   0x00c000 PCONTROL nv40-pcontrol NV40:NV50
   0x00d000 PTV ptv NV17:NV20,NV30:NV50
   0x00e000 UNK00E000 unk00e000 NV17:NV20
   0x00f000 PVP1 pvp1 NV41:NV50
   0x088000 PPCI ppci NV40:NV50
   0x090000 PFIFO_CACHE nv40-pfifo-cache NV40:NV50
   0x0a0000 PRMFB nv3-prmfb
   0x0c0000 PRMVIO prmvio NV3:NV40
   0x0c0000[2/0x2000] PRMVIO prmvio NV40:NV50
   0x100000 PFB nv3-pfb NV3:NV10
   0x100000 PFB nv10-pfb NV10:NV40&!IGP1X
   0x100000 PFB nv40-pfb NV40:NV50&!TC
   0x100000 PFB nv44-pfb NV44:NV50&TC
   0x101000 PSTRAPS nv3-pstraps !NV1A
   0x102000 UNK102000 nv4e-unk102000 NV63
   0x110000 PROM nv3-prom NV3:NV4
   0x120000 PALT nv3-palt NV3:NV4
   0x200000 PMEDIA pmedia !IGP4X
   0x300000 PROM nv3-prom NV4:NV17,NV20:NV25
   0x300000 PROM nv17-prom NV17:NV20,NV25:NV50&!IGP4X
   0x400000 PGRAPH nv3-pgraph NV3:NV4
   0x400000 PGRAPH nv4-pgraph NV4:NV10
   0x400000 PGRAPH nv10-pgraph NV10:NV20
   0x400000 PGRAPH nv20-pgraph NV20:NV40
   0x400000 PGRAPH nv40-pgraph NV40:NV50
   0x401000 PDMA nv3-pdma NV3:NV4
   0x600000 PCRTC pcrtc NV4:NV11,NV20:NV25
   0x600000[2/0x2000] PCRTC pcrtc NV11:NV20,NV25:NV50
   0x601000 PRMCIO prmcio NV3:NV11,NV20:NV25
   0x601000[2/0x2000] PRMCIO prmcio NV11:NV20,NV25:NV50
   0x680000 PRAMDAC pramdac NV3:NV11,NV20:NV25
   0x680000[2/0x2000] PRAMDAC pramdac NV11:NV20,NV25:NV50
   0x681000 PRMDIO prmdio NV3:NV11,NV20:NV25
   0x681000[2/0x2000] PRMDIO prmdio NV11:NV20,NV25:NV50
   0x6e0000 UNK6E0000 unk6e0000 NV17:NV40
   0x700000 PRAMIN nv4-pramin NV4:NV50
   0x0800000[chid:0x80][subc:8] USER nv1-user NV3:NV4
   0x0800000[chid:0x10][subc:8] USER nv4-user NV4:NV10
   0x0800000[chid:0x20][subc:8] USER nv4-user NV10:NV50
   0x0c00000[chid:0x200] DMA_USER nv40-dma-user NV40:NV50

   .. todo:: check UNK005000 variants [sorta present on NV35, NV34, NV4E, NV63; present on NV5, NV11, NV17, NV1A, NV20; not present on NV44]
   .. todo:: check PCOUNTER variants
   .. todo:: some IGP don't have PVPE/PVP1 [NV4E: present, but without PME; NV63: not present at all]
   .. todo:: check PSTRAPS on IGPs
   .. todo:: check PROM on IGPs
   .. todo:: PMEDIA not on IGPs [NV63 and NV4E: not present] and some other cards?
   .. todo:: PFB not on IGPs
   .. todo:: merge PCRTC+PRMCIO/PRAMDAC+PRMDIO?
   .. todo:: UNK6E0000 variants
   .. todo:: UNK006000 variants
   .. todo:: UNK00E000 variants
   .. todo:: 102000 variants; present on NV63, not NV4E

   .. note:: fully verified on NV3, NV5, NV11, NV17, NV34, NV35, NV44, NV4E, NV63
   
   .. note::
   
      NV1A and NV20 don't have second PCRTC/PRAMDAC, but still have the
      decoding circuitry for them. This may cause the card to hang when
      accessing these ranges. The same applies for NV2x and PVPE.


NV50:NVC0 MMIO map
==================

.. space:: 8 nv50-mmio 0x1000000 -
   0x000000 PMC pmc * ROOT
   0x001000 PBUS pbus * ROOT
   0x002000 PFIFO nv50-pfifo * ROOT
   0x004000 PCLOCK nv50-pclock NV50:NVA3 IBUS
   0x004000 PCLOCK nva3-pclock NVA3:NVC0 IBUS
   0x007000 PRMA prma * ROOT
   0x009000 PTIMER nv3-ptimer * ROOT
   0x00a000 PCOUNTER nv40-pcounter * IBUS
   0x00b000 PVPE pvpe VP1,VP2 IBUS
   0x00c000 PCONTROL nv50-pcontrol NV50:NVA3 IBUS
   0x00c000 PCONTROL nva3-pcontrol NVA3:NVC0 IBUS
   0x00e000 PNVIO pnvio * IBUS
   0x00e800 PIOCLOCK nv50-pioclock NV50:NVA3 IBUS
   0x00e800 PIOCLOCK nva3-pioclock NVA3:NVC0 IBUS
   0x00f000 PVP1 pvp1 VP1 IBUS
   0x00f000 PVP2 pvp2 VP2 IBUS
   0x010000 UNK010000 unk010000 * ROOT
   0x020000 PTHERM ptherm * IBUS
   0x021000 PFUSE pfuse * IBUS
   0x022000 UNK022000 unk022000 NV84: IBUS
   0x060000 PEEPHOLE peephole NV84: ROOT
   0x070000 PFLUSH nv50-pflush NV84:NVC0 ROOT
   0x080000 PHWSQ_LARGE_CODE phwsq-large-code NV92:NVC0 ROOT
   0x084000 PVLD pvld VP3,VP4 IBUS
   0x085000 PVDEC pvdec VP3,VP4 IBUS
   0x086000 PPPP pppp VP3,VP4 IBUS
   0x087000 PCRYPT3 pcrypt3 VP3 IBUS
   0x088000 PPCI ppci * IBUS
   0x089000 UNK089000 unk089000 NV84: IBUS
   0x08a000 PPCI_HDA ppci-hda NVA3:NVC0 IBUS
   0x090000 PFIFO_CACHE nv50-pfifo-cache * ROOT
   0x0a0000 PRMFB nv50-prmfb * ROOT
   0x100000 PFB nv50-pfb * IBUS
   0x101000 PSTRAPS nv3-pstraps * IBUS
   0x102000 PCRYPT2 pcrypt2 VP2 IBUS
   0x102000 UNK102000 unk102000 IGP ROOT
   0x103000 PBSP pbsp VP2 IBUS
   0x104000 PCOPY pcopy NVA3:NVC0 IBUS
   0x108000 PCODEC pcodec NVA3: IBUS
   0x109000 PKFUSE pkfuse NVA3: IBUS
   0x10a000 PDAEMON pdaemon NVA3:NVC0 IBUS
   0x1c1000 PVCOMP pvcomp NVAF:NVC0 IBUS
   0x200000 PMEDIA pmedia * IBUS
   0x280000 UNK280000 unk280000 NVAF ROOT
   0x2ff000 PBRIDGE_PCI pbridge-pci IGP IBUS
   0x300000 PROM nv17-prom NV50:NVA0 IBUS
   0x300000 PROM nva0-prom NVA0: IBUS
   0x400000 PGRAPH nv50-pgraph * IBUS
   0x601000 PRMIO nv50-prmio * IBUS
   0x610000 PDISPLAY nv50-pdisplay * IBUS
   0x700000 PMEM pmem * ROOT
   0x800000 PIO_USER[subc:8] nv50-pio-user * ROOT
   0xc00000 DMA_USER[chid:0x80] nv50-dma-user * ROOT

   .. todo:: 10f000:112000 range on NVA3-


NVC0+ MMIO map
==============

.. space:: 8 nvc0-mmio 0x1000000 -
   0x000000 PMC pmc * ROOT
   0x001000 PBUS pbus * ROOT
   0x002000 PFIFO nvc0-pfifo * ROOT
   0x005000 PFIFO_PIO nvc0-pfifo-pio * ROOT
   0x007000 PRMA prma * ROOT
   0x009000 PTIMER nv3-ptimer * ROOT
   0x00c800 UNK00C800 unk00c800
   0x00cc00 UNK00CC00 unk00cc00
   0x00d000 PGPIO pgpio NVD9: HUB
   0x00e000 PNVIO pnvio * HUB
   0x00e800 PIOCLOCK nvc0-pioclock * HUB
   0x010000 UNK010000 unk010000 * ROOT
   0x020000 PTHERM ptherm * HUB
   0x021000 PFUSE pfuse * HUB
   0x022400 PUNITS punits * HUB
   0x040000 PSPOON[3] pspoon * ROOT
   0x060000 PEEPHOLE peephole * ROOT
   0x070000 PFLUSH nvc0-pflush * ROOT
   0x082000 UNK082000 unk082000 * HUB
   0x082800 UNK082800 unk082800 NVC0:NVE4 HUB
   0x084000 PVLD pvld * HUB
   0x085000 PVDEC pvdec * HUB
   0x086000 PPPP pppp * HUB
   0x088000 PPCI ppci * HUB
   0x089000 UNK089000 unk089000 NVC0:NVE4 HUB
   0x08a000 PPCI_HDA ppci-hda * HUB
   0x08b000 UNK08B000 unk08b000 NVE4: HUB
   0x0a0000 PRMFB nv50-prmfb * ROOT
   0x100700 PBFB_COMMON pbfb-common
   0x100800 PFFB pffb * HUB
   0x101000 PSTRAPS nv3-pstraps * HUB
   0x104000[2] PCOPY pcopy NVC0:NVE4 HUB
   0x104000[3] PCOPY pcopy NVE4: HUB
   0x108000 PCODEC pcodec * HUB
   0x109000 PKFUSE pkfuse * HUB
   0x10a000 PDAEMON pdaemon * HUB
   0x10c000 UNK10C000 unk10c000
   0x10f000 PBFB pbfb
   0x120000 PRING pring
   0x130000 PCLOCK nvc0-pclock
   0x138000 UNK138000 unk138000
   0x139000 PP2P pp2p * HUB
   0x13b000 PXBAR pxbar
   0x140000 PMFB pmfb
   0x180000 PCOUNTER nvc0-pcounter
   0x1c0000 PFIFO_UNK1C0000 nvc0-pfifo-unk1c0000 * ROOT
   0x1c2000 PVENC pvenc NVE4: HUB
   0x1c3000 PUNK1C3 punk1c3 NVD9: HUB
   0x200000 PMEDIA pmedia * HUB
   0x300000 PROM nva0-prom * HUB
   0x400000 PGRAPH nvc0-pgraph
   0x601000 PRMIO nv50-prmio * HUB
   0x610000 PDISPLAY nv50-pdisplay NVC0:NVD9 HUB
   0x610000 PDISPLAY nvd9-pdisplay NVD9: HUB
   0x700000 PMEM pmem * ROOT
   0x800000 PFIFO_CHAN nvc0-pfifo-chan NVE4: ROOT

   .. todo:: verified accurate for NVE4, check on earlier cards
   .. todo:: did they finally kill off PMEDIA?


NVD9 MMIO errors
================

- ROOT errors:
 
  - bad001XX: nonexistent register [gives PBUS intr 3]
  - bad0acXX: VM fault when accessing memory
  - bad0daXX: disabled in PMC.ENABLE or PMC.SUBFIFO_ENABLE [gives PBUS intr 1]
  - bad0fbXX: problem accessing memory [gives PBUS intr 7 or maybe 5]

  The low 8 bits appear to be some sort of request id.

- PRING errors [all give PBUS intr 2 if accessed via ROOT]:

  - badf1000: target refused transaction
  - badf1100: no target for given address
  - badf1200: target disabled in PMC.ENABLE
  - badf1300: target disabled in PRING

- badf3000: access to GPC/PART targets before initialising them?

- badf5000: ??? seen on accesses to PRING own areas and some PCOUNTER regs


Unknown ranges
==============

.. space:: 8 nv3-unk004000 0x1000 ???

   .. todo:: RE me

.. space:: 8 nv4-unk005000 0x1000 ???

   rules.xml says HOST_DIAG

   .. todo:: RE me

.. space:: 8 unk006000 0x1000 ???

   Reads as all 0xdeadbeef

   .. todo:: RE me

.. space:: 8 unk00e000 0x1000 ???

   Reads cause device hang

   .. todo:: RE me

.. space:: 8 unk6e0000 0x1000 ???

   rules.xml says PREMAP

   .. todo:: RE me

.. space:: 8 nv34-unk004000 0x1000 ???

   .. todo:: RE me

.. space:: 8 nv4e-unk102000 0x1000 ???

   .. todo:: RE me

.. space:: 8 unk010000 0x10000 ???
   
   Has something to do with PCI config spaces of other devices?

   .. todo:: NV4x? NVCx?

.. space:: 8 unk022000 0x400 ???
   
   .. todo:: RE me

.. space:: 8 unk089000 0x1000 ???
   
   .. todo:: RE me

.. space:: 8 unk102000 0x1000 ???
   
   .. todo:: RE me

.. space:: 8 unk280000 0x20000 ???
   
   .. todo:: RE me

.. space:: 8 unk08b000 0x4000 ???
   
   Seems to be a new version of former 89000 area

   .. todo:: RE me

.. space:: 8 unk00c800 0x400 ???
   
   .. todo:: RE me

.. space:: 8 unk00cc00 0x400 ???
   
   .. todo:: RE me

.. space:: 8 unk082000 0x400 ???
   
   .. todo:: RE me

.. space:: 8 unk082800 0x800 ???
   
   .. todo:: RE me

.. space:: 8 unk10c000 0x3000 ???
   
   .. todo:: RE me

.. space:: 8 unk138000 0x1000 ???
   
   .. todo:: RE me
