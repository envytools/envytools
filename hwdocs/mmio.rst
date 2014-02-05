.. _mmio:

====================
MMIO register ranges
====================

.. contents::


Introduction
============

.. todo:: write me


NV01 MMIO map
=============

.. space:: 8 nv01-mmio 0x2000000 -
   0x0000000 PMC pmc
   0x0001000 PBUS pbus
   0x0002000 PFIFO nv01-pfifo
   0x0100000 PDMA nv01-pdma
   0x0101000 PTIMER nv01-ptimer
   0x0300000 PAUDIO nv01-paudio
   0x0400000 PGRAPH nv01-pgraph
   0x0410000[subc:8] UBETA nv01-ubeta
   0x0420000[subc:8] UROP nv01-urop
   0x0430000[subc:8] UCHROMA nv01-uchroma
   0x0440000[subc:8] UPLANE nv01-uplane
   0x0450000[subc:8] UCLIP nv01-uclip
   0x0460000[subc:8] UPATTERN nv01-upattern
   0x0480000[subc:8] UPOINT nv01-upoint
   0x0490000[subc:8] ULINE nv01-uline
   0x04a0000[subc:8] ULIN nv01-ulin
   0x04b0000[subc:8] UTRI nv01-utri
   0x04c0000[subc:8] URECT nv01-urect
   0x04d0000[subc:8] UTEXLIN nv01-utexlin
   0x04e0000[subc:8] UTEXQUAD nv01-utexquad
   0x0500000[subc:8] UBLIT nv01-ublit
   0x0510000[subc:8] UIFC nv01-uifc
   0x0520000[subc:8] UBITMAP nv01-ubitmap
   0x0530000[subc:8] UIFM nv01-uifm
   0x0540000[subc:8] UITM nv01-uitm
   0x05d0000[subc:8] UTEXLINBETA nv01-utexlinbeta
   0x05e0000[subc:8] UTEXQUADBETA nv01-utexquadbeta
   0x0600000 PFB nv01-pfb
   0x0602000 PRAM nv01-pram
   0x0604000 PRAMUNK1 nv01-pramunk1
   0x0605000 PCHIPID pchipid
   0x0606000 PRAMUNK2 nv01-pramunk2
   0x0608000 PSTRAPS nv01-pstraps
   0x0609000 PDAC nv01-pdac
   0x060a000 PEEPROM peeprom
   0x0610000 PROM nv01-prom
   0x0618000 PALT nv01-palt
   0x0640000 PRAMHT nv01-pramht
   0x0648000 PRAMFC nv01-pramfc
   0x0650000 PRAMRO nv01-pramro
   0x06c0000 PRM nv01-prm
   0x06d0000 PRMIO nv01-prmio
   0x06e0000 PRMFB nv01-prmfb
   0x0700000 PRAMIN nv01-pramin
   0x0800000[chid:0x80][subc:8] USER nv01-user
   0x1000000 FB nv01-fb


NV03:NV50 MMIO map
==================

.. space:: 8 nv03-mmio 0x1000000 -
   0x000000 PMC pmc
   0x001000 PBUS pbus
   0x002000 PFIFO nv01-pfifo NV03:NV04
   0x002000 PFIFO nv04-pfifo NV04:NV50
   0x004000 UNK004000 nv03-unk004000 NV03:NV04
   0x004000 PCLOCK nv40-pclock NV40:NV50
   0x005000 UNK005000 nv04-unk005000 NV04:NV30
   0x007000 PRMA prma
   0x008000 PVIDEO pvideo NV10:NV50
   0x009000 PTIMER nv03-ptimer
   0x00a000 PCOUNTER nv10-pcounter NV10:NV40
   0x00a000 PCOUNTER nv40-pcounter NV40:NV50
   0x00b000 PVPE pvpe NV17:NV20,NV30:NV50
   0x00c000 PCONTROL nv40-pcontrol NV40:NV50
   0x00d000 PTV ptv NV17:NV20,NV30:NV50
   0x00f000 PVP1 pvp1 NV41:NV50
   0x088000 PPCI ppci NV40:NV50
   0x090000 PFIFO_CACHE nv40-pfifo-cache NV40:NV50
   0x0a0000 PRMFB nv03-prmfb
   0x0c0000 PRMVIO prmvio NV03:NV40
   0x0c0000 PRMVIO[2/0x2000] prmvio NV40:NV50
   0x100000 PFB nv03-pfb NV03:NV10
   0x100000 PFB nv10-pfb NV10:NV40
   0x100000 PFB nv40-pfb NV40:NV50&!TC
   0x100000 PFB nv44-pfb NV44:NV50&TC
   0x101000 PSTRAPS nv03-pstraps
   0x102000 UNK102000 nv4e-unk102000 IGP4X
   0x110000 PROM nv03-prom NV03:NV04
   0x200000 PMEDIA pmedia
   0x300000 PROM nv03-prom NV04:NV17,NV20:NV25
   0x300000 PROM nv17-prom NV17:NV20,NV20:NV50
   0x400000 PGRAPH nv03-pgraph NV03:NV04
   0x400000 PGRAPH nv04-pgraph NV04:NV10
   0x400000 PGRAPH nv10-pgraph NV10:NV20
   0x400000 PGRAPH nv20-pgraph NV20:NV40
   0x400000 PGRAPH nv40-pgraph NV40:NV50
   0x401000 PDMA nv03-pdma NV03:NV04
   0x600000 PCRTC pcrtc NV04:NV11,NV20:NV25
   0x600000 PCRTC[2/0x2000] pcrtc NV11:NV20,NV25:NV50
   0x601000 PRMCIO prmcio NV03:NV11,NV20:NV25
   0x601000 PRMCIO[2/0x2000] prmcio NV11:NV20,NV25:NV50
   0x680000 PRAMDAC pramdac NV03:NV11,NV20:NV25
   0x680000 PRAMDAC[2/0x2000] pramdac NV11:NV20,NV25:NV50
   0x681000 PRMDIO prmdio NV03:NV11,NV20:NV25
   0x681000 PRMDIO[2/0x2000] prmdio NV11:NV20,NV25:NV50
   0x700000 PRAMIN nv04-pramin NV04:NV50
   0x0800000[chid:0x80][subc:8] USER nv01-user NV03:NV04
   0x0800000[chid:0x10][subc:8] USER nv04-user NV04:NV10
   0x0800000[chid:0x20][subc:8] USER nv04-user NV10:NV50
   0x0c00000[chid:0x200] DMA_USER nv40-dma-user NV40:NV50

   .. todo:: check UNK005000 variants [verified not present on NV35, present on NV11]
   .. todo:: check PCOUNTER variants
   .. todo:: some IGP don't have PVPE/PVP1
   .. todo:: check PSTRAPS on IGPs
   .. todo:: check PROM on IGPs
   .. todo:: PMEDIA not on IGPs and some other cards?
   .. todo:: PFB not on IGPs


NV50:NVC0 MMIO map
==================

.. space:: 8 nv50-mmio 0x1000000 -
   0x000000 PMC pmc
   0x001000 PBUS pbus
   0x009000 PTIMER nv03-ptimer
   0x101000 PSTRAPS nv03-pstraps
   0x300000 PROM nv17-prom NV50:NVA0
   0x300000 PROM nva0-prom NVA0:

   ============== ===== ============= ========= ================================ ======================
   Address range  Port  Name          Variants  Reference                        Description
   ============== ===== ============= ========= ================================ ======================
   000000:001000  ROOT  PMC           all       :obj:`pmc`                       card master control
   001000:002000  ROOT  PBUS          all       :obj:`pbus`                      bus control
   002000:004000  ROOT  PFIFO         all       :ref:`nv50-pfifo-mmio`           DMA FIFO submission to execution engines
   004000:005000  IBUS  PCLOCK        NV50:NVA3 :ref:`nv50-pclock-mmio`          PLL control
   004000:005000  IBUS  PCLOCK        NVA3:NVC0 :ref:`nva3-pclock-mmio`          PLL control
   007000:008000  ROOT  PRMA          all       :obj:`prma`                      access to BAR0 from real mode
   009000:00a000  ROOT  PTIMER        all       :obj:`nv03-ptimer`               time measurement and time-based alarms
   00a000:00b000  IBUS  PCOUNTER      all       :obj:`nv40-pcounter`             performance monitoring counters
   00b000:00c000  IBUS  PVPE          all       :obj:`pvpe`                      MPEG2 decoding engine
   00c000:00d000  IBUS  PCONTROL      NV50:NVA3 :ref:`nv50-pcontrol-mmio`        control of misc stuff
   00c000:00d000  IBUS  PCONTROL      NVA3:NVC0 :ref:`nva3-pcontrol-mmio`        control of misc stuff
   00e000:00e800  IBUS  PNVIO         all       :ref:`pnvio-mmio`                GPIOs, I2C buses, PWM fan control, and other external devices
   00e800:00f000  IBUS  PIOCLOCK      NV50:NVA3 :ref:`nv50-pioclock-mmio`        PNVIO's clock setup
   00e800:00f000  IBUS  PIOCLOCK      NVA3:NVC0 :ref:`nva3-pioclock-mmio`        PNVIO's clock setup
   00f000:010000  IBUS  PVP1          VP1       :obj:`pvp1`                      VP1 video processing engine
   00f000:010000  IBUS  PVP2          VP2       :ref:`pvp2-mmio`                 VP2 xtensa video processing engine
   010000:020000  ROOT  ???           all       ???                              has something to do with PCI config spaces of other devices?
   020000:021000  IBUS  PTHERM        all       :ref:`ptherm-mmio`               thermal sensor
   021000:022000  IBUS  PFUSE         all       :ref:`pfuse-mmio`                efuses storing not so secret stuff
   022000:022400  IBUS  ???           ???       ???                              ???
   060000:061000  ROOT  PEEPHOLE      NV84:NVC0 :ref:`peephole-mmio`             indirect VM access
   070000:071000  ROOT  PFLUSH        NV84:NVC0 :ref:`nv50-pflush-mmio`          used to flush BAR writes
   080000:081000  ROOT  PBUS HWSQ     NV92:NVC0 :ref:`hwsq-mmio`                 extended HWSQ code space
                        NEW_CODE                                                 
   084000:085000  IBUS  PVLD          VP3, VP4  :ref:`pvld-io`                   VP3 variable length decoding engine
   085000:086000  IBUS  PVDEC         VP3, VP4  :ref:`pvdec-io`                  VP3 video decoding engine
   086000:087000  IBUS  PPPP          VP3, VP4  :ref:`pppp-io`                   VP3 video postprocessing engine
   087000:088000  IBUS  PCRYPT3       VP3       :ref:`pcrypt3-io`                VP3 cryptographic engine
   088000:089000  IBUS  PPCI          all       :obj:`ppci`                      PCI config space access
   089000:08a000  IBUS  ???           NV84:NVC0 ???                              ???
   08a000:08b000  IBUS  PPCI_HDA      NVA3:NVC0 :obj:`ppci-hda`                  PCI config space access for the HDA codec function
   090000:0a0000  ROOT  PFIFO cache   all       :ref:`nv50-pfifo-mmio-cache`     part of PFIFO
   0a0000:0c0000  ROOT  PRMFB         all       :obj:`nv50-prmfb`                aliases VGA memory window
   100000:101000  IBUS  PFB           all       :ref:`nv50-pfb-mmio`             memory interface and VM control
   101000:102000  IBUS  PSTRAPS       all       :obj:`nv03-pstraps`              straps readout / override
   102000:103000  IBUS  PCRYPT2       VP2       :ref:`pcrypt2-mmio`              VP2 cryptographic engine
   102000:103000  ROOT  ???           IGPs only ???                              ???
   103000:104000  IBUS  PBSP          VP2       :ref:`pbsp-mmio`                 VP2 BSP engine
   104000:105000  IBUS  PCOPY         NVA3:NVC0 :ref:`pcopy-io`                  memory copy engine
   108000:109000  IBUS  PCODEC        NVA3:NVC0 :ref:`pcodec-mmio`               the HDA codec doing HDMI audio
   109000:10a000  IBUS  PKFUSE        NVA3:NVC0 :ref:`pkfuse-mmio`               efuses storing secret key stuff
   10a000:10b000  IBUS  PDAEMON       NVA3:NVC0 :ref:`pdaemon-io`                a falcon engine used to run management code in background
   1c1000:1c2000  IBUS  PVCOMP        NVAF:NVC0 :ref:`pvcomp-io`                 video compositor engine
   200000:201000  IBUS  PMEDIA        all       :obj:`pmedia`                    mediaport
   280000:2a0000  ROOT  ???           NVAF      ???                              ???
   2ff000:300000  IBUS  PBRIDGE_PCI   IGPs      :ref:`pbus-mmio`                 access to PCI config registers of the GPU's upstream PCIE bridge
   300000:400000  IBUS  PROM          all       :obj:`nv17-prom`                 ROM access window
   400000:410000  IBUS  PGRAPH        all       :ref:`nv50-pgraph-mmio`          accelerated 2d/3d drawing and CUDA engine
   601000:602000  IBUS  PRMIO         all       :obj:`nv50-prmio`                aliases VGA registers
   610000:640000  IBUS  PDISPLAY      all       :ref:`pdisplay-mmio`             the DMA FIFO controlled unified display engine
   640000:650000  IBUS  DISPLAY_USER  all       :ref:`display-user-mmio`         DMA submission to PDISPLAY
   700000:800000  ROOT  PMEM          all       :ref:`pmem-mmio`                 indirect VRAM/host memory access
   800000:810000  ROOT  USER_PIO      all       :ref:`fifo-user-mmio-pio`        PFIFO PIO submission area
   c00000:1000000 ROOT  USER_DMA      all       :ref:`fifo-user-mmio-dma`        PFIFO DMA submission area
   ============== ===== ============= ========= ================================ ======================

.. note:: VP1 is NV50:NV84

          VP2 is NV84:NV98 and NVA0:NVAA

          VP3 is NV98:NVA0 and NVAA:NVA3

          VP4 is NVA3:NVC0

.. todo:: 10f000:112000 range on NVA3-


NVC0+ MMIO map
==============

.. space:: 8 nvc0-mmio 0x1000000 -
   0x000000 PMC pmc
   0x001000 PBUS pbus
   0x009000 PTIMER nv03-ptimer
   0x101000 PSTRAPS nv03-pstraps
   0x300000 PROM nva0-prom

   ============== ===== ============= ========= ================================ ======================
   Address range  Port  Name          Variants  Reference                        Description
   ============== ===== ============= ========= ================================ ======================
   000000:001000  ROOT  PMC           all       :obj:`pmc`                       card master control
   001000:002000  ROOT  PBUS          all       :obj:`pbus`                      bus control
   002000:004000  ROOT  PFIFO         all       :ref:`nvc0-pfifo-mmio`           DMA FIFO submission to execution engines
   005000:006000  ROOT  PFIFO_BYPASS  all       :ref:`nvc0-pfifo-mmio-bypass`    PFIFO bypass interface
   007000:008000  ROOT  PRMA          all       :ref:`prma`                      access to BAR0 from real mode
   009000:00a000  ROOT  PTIMER        all       :obj:`nv03-ptimer`               time measurement and time-based alarms
   00c800:00cc00  IBUS  ???           all       ???                              ???
   00cc00:00d000  IBUS  ???           all       ???                              ???
   00d000:00e000  IBUS  PGPIO         NVD9-     :ref:`pgpio-mmio`                GPIOs, I2C buses
   00e000:00e800  IBUS  PNVIO         all       :ref:`pnvio-mmio`                GPIOs, I2C buses, PWM fan control, and other external devices
   00e800:00f000  IBUS  PIOCLOCK      all       :ref:`nvc0-pioclock-mmio`        PNVIO's clock setup
   010000:020000  ROOT  ???           all       ???                              has something to do with PCI config spaces of other devices?
   020000:021000  IBUS  PTHERM        all       :ref:`ptherm-mmio`               thermal sensor
   021000:022000  IBUS  PFUSE         all       :ref:`pfuse-mmio`                efuses storing not so secret stuff
   022400:022800  IBUS  PUNITS        all       :ref:`punits-mmio`               control over enabled card units
   040000:060000  ROOT  PSUBFIFOs     all       :ref:`nvc0-psubfifo-mmio`        individual SUBFIFOs of PFIFO
   060000:061000  ROOT  PEEPHOLE      all       :ref:`peephole-mmio`             indirect VM access
   070000:071000  ROOT  PFLUSH        all       :ref:`nvc0-pflush-mmio`          used to flush BAR writes
   082000:082400  IBUS  ???           all       ???                              ???
   082800:083000  IBUS  ???           NVC0:NVE4 ???                              ???
   084000:085000  IBUS  PVLD          all       :ref:`pvld-io`                   VP3 VLD engine
   085000:086000  IBUS  PVDEC         all       :ref:`pvdec-io`                  VP3 video decoding engine
   086000:087000  IBUS  PPPP          all       :ref:`pppp-io`                   VP3 video postprocessing engine
   088000:089000  IBUS  PPCI          all       :obj:`ppci`                      PCI config space access
   089000:08a000  IBUS  ???           NVC0:NVE4 ???                              ???
   08a000:08b000  IBUS  PPCI_HDA      all       :obj:`ppci-hda`                  PCI config space access for the HDA codec function
   08b000:08f000  IBUS  ???           NVE4-     ???                              seems to be a new version of former 89000 area
   0a0000:0c0000  both  PRMFB         all       :obj:`nv50-prmfb`                aliases VGA memory window
   100700:100800  IBUS  PBFB_COMMON   all       :ref:`pbfb-mmio`                 some regs shared between PBFBs???
   100800:100e00  IBUS  PFFB          all       :ref:`pffb-mmio`                 front memory interface and VM control
   100f00:101000  IBUS  PFFB          all       :ref:`pffb-mmio`                 front memory interface and VM control
   101000:102000  IBUS  PSTRAPS       all       :obj:`nv03-pstraps`              straps readout / override
   104000:105000  IBUS  PCOPY[0]      NVC0:NVE4 :ref:`pcopy-io`                  memory copy engine #0
   105000:106000  IBUS  PCOPY[1]      NVC0:NVE4 :ref:`pcopy-io`                  memory copy engine #1
   104000:105000  IBUS  PCOPY[0]      NVE4-     :ref:`pcopy-mmio`                memory copy engine #0
   105000:106000  IBUS  PCOPY[1]      NVE4-     :ref:`pcopy-mmio`                memory copy engine #1
   106000:107000  IBUS  PCOPY[2]      NVE4-     :ref:`pcopy-mmio`                memory copy engine #2
   108000:108800  IBUS  PCODEC        all       :ref:`pcodec-mmio`               the HDA codec doing HDMI audio
   109000:10a000  IBUS  PKFUSE        all       :ref:`pkfuse-mmio`               efuses storing secret key stuff
   10a000:10b000  IBUS  PDAEMON       all       :ref:`pdaemon-io`                a falcon engine used to run management code in background
   10c000:10f000  IBUS  ???           ???       ???                              ???
   10f000:120000  IBUS  PBFBs         all       :ref:`pbfb-mmio`                 memory controller backends
   120000:130000  IBUS  PIBUS         all       :ref:`pibus-mmio`                deals with internal bus used to reach most other areas of MMIO
   130000:135000  IBUS  ???           ???       ???                              ???
   137000:138000  IBUS  PCLOCK        all       :ref:`nvc0-pclock-mmio`          clock setting
   138000:139000  IBUS  ???           ???       ???                              ???
   139000:13b000  IBUS  PP2P          all       :ref:`pp2p-mmio`                 peer to peer memory access
   13b000:13f000  IBUS  PXBAR         all       :ref:`pxbar-mmio`                crossbar between memory controllers and GPCs
   140000:180000  IBUS  PMFBs         all       :ref:`pmfb-mmio`                 middle memory controllers: compression and L2 cache
   180000:1c0000  IBUS  PCOUNTER      all       :ref:`nvc0-pcounter-mmio`        performance monitoring counters
   1c0000:1c1000  ROOT  ???           all       ???                              related to PFIFO and playlist?
   1c2000:1c3000  IBUS  PVENC         NVE4-     :ref:`pvenc-io`                  H.264 video encoding engine
   1c3000:1c4000  IBUS  ???           NVD9-     :ref:`punk1c3-io`                some falcon engine
   200000:201000  ???   PMEDIA        all       :obj:`pmedia`                    mediaport
   300000:380000  IBUS  PROM          all       :obj:`nva0-prom`                 ROM access window
   400000:600000  IBUS  PGRAPH        all       :ref:`nvc0-pgraph-mmio`          accelerated 2d/3d drawing and CUDA engine
   601000:602000  IBUS  PRMIO         all       :obj:`nv50-prmio`                aliases VGA registers
   610000:6c0000  IBUS  PDISPLAY      all       :ref:`pdisplay-mmio`             the DMA FIFO controlled unified display engine
   700000:800000  ROOT  PMEM          all       :ref:`pmem-mmio`                 indirect VRAM/host memory access
   800000:810000  ROOT  PFIFO_CHAN    NVE4-     :ref:`nvc0-pfifo-mmio-chan`      PFIFO channel table
   ============== ===== ============= ========= ================================ ======================

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

- IBUS errors [all give PBUS intr 2 if accessed via ROOT]:

  - badf1000: target refused transaction
  - badf1100: no target for given address
  - badf1200: target disabled in PMC.ENABLE
  - badf1300: target disabled in PIBUS

- badf3000: access to GPC/PART targets before initialising them?

- badf5000: ??? seen on accesses to PIBUS own areas and some PCOUNTER regs


Unknown ranges
==============

.. space:: 8 nv03-unk004000 0x1000 ???

   .. todo:: RE me

.. space:: 8 nv04-unk005000 0x1000 ???

   .. todo:: RE me

.. space:: 8 nv4e-unk102000 0x1000 ???

   .. todo:: RE me
