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

=============== ======= ========================== ======================
Address range   Name    Reference                  Description
=============== ======= ========================== ======================
000000:001000   PMC     :ref:`pmc-mmio`            card master control
001000:002000   PBUS    :ref:`pbus-mmio`           bus control
002000:004000   PFIFO   `<fifo/nv01-pfifo.txt>`_   MMIO-mapped FIFO submission to PGRAPH
100000:101000   PDMA    `<memory/nv01-pdma.txt>`_  system memory DMA engine
101000:102000   PTIMER  :ref:`ptimer-mmio-nv01`    time measurement and time-based alarms
300000:301000   PAUDIO  :ref:`nv01-paudio-mmio`    audio capture and playback device
400000:401000   PGRAPH  `<graph/nv01-pgraph.txt>`_ accelerated drawing engine
600000:601000   PFB     `<display/nv01/pfb.txt>`_  VRAM and video output control
602000:603000   PRAM    `<memory/nv01-vram.txt>`_  RAMIN layout control
604000:605000   ???     `<memory/nv01-vram.txt>`_  ???
605000:606000   PCHIPID `<io/nv01-peeprom.txt>`_   chip ID readout
606000:607000   ???     `<memory/nv01-vram.txt>`_  ???
608000:609000   PSTRAPS `<io/pstraps.txt>`_        straps readout / override
609000:60a000   PDAC    `<display/nv01/pdac.txt>`_ DAC control
60a000:60b000   PEEPROM `<io/nv01-peeprom.txt>`_   configuration EEPROM access
610000:618000   PROM    `<io/prom.txt>`_           ROM access window
618000:620000   PALT    `<io/prom.txt>`_           external memory access window
640000:648000   PRAMHT  `<memory/nv01-vram.txt>`_  RAMHT access
                        `<fifo/nv01-pfifo.txt>`_
648000:64c000   PRAMFC  `<memory/nv01-vram.txt>`_  RAMFC access
                        `<fifo/nv01-pfifo.txt>`_
650000:654000   PRAMRO  `<memory/nv01-vram.txt>`_  RAMRO access
                        `<fifo/nv01-pfifo.txt>`_
6c0000:6c8000   PRM     :ref:`nv01-prm-mmio`       VGA compatibility control - NV01 specific
6d0000:6d1000   PRMIO   :ref:`nv01-prmio-mmio`     VGA and ISA sound compat IO port access
6e0000:700000   PRMFB   :ref:`nv01-prmfb-mmio`     aliases VGA memory window
700000:800000   PRAMIN  `<memory/nv01-vram.txt>`_  RAMIN access
800000:1000000  USER    `<fifo/pio.txt>`_          PFIFO MMIO submission area
1000000:2000000 FB      `<memory/nv01-vram.txt>`_  VRAM access area
=============== ======= ========================== ======================


NV03:NV50 MMIO map
==================

=============== ======== ========= ============================== ======================
Address range   Name     Variants  Reference                      Description
=============== ======== ========= ============================== ======================
000000:001000   PMC      all       :ref:`pmc-mmio`                card master control
001000:002000   PBUS     all       :ref:`pbus-mmio`               bus control
002000:004000   PFIFO    all       `<fifo/nv01-pfifo.txt>`_       MMIO and DMA FIFO submission to PGRAPH and VPE
                                   `<fifo/nv04-pfifo.txt>`_
004000:005000   ???      NV03:NV10 ???                            ???
004000:005000   PCLOCK   NV40:NV50 `<pm/nv40-pclock.txt>`_        PLL control
005000:006000   ???      all       ???                            ???
007000:008000   PRMA     all       :ref:`prma-mmio`               access to BAR0/BAR1 from real mode
008000:009000   PVIDEO   NV10:NV50 `<display/nv03/pvideo.txt>`_   video overlay
009000:00a000   PTIMER   all       :ref:`ptimer-mmio-nv03`        time measurement and time-based alarms
00a000:00b000   PCOUNTER NV10:NV50 `<pcounter/intro.txt>`_        performance monitoring counters
00b000:00c000   PVPE     NV17:NV20 `<vdec/vpe/intro.txt>`_        MPEG2 decoding engine
                         NV30:NV50 
00c000:00d000   PCONTROL NV40:NV50 `<pm/nv40-pclock.txt>`_        control of misc stuff
00d000:00e000   PTV      NV17:NV20 `<display/nv03/ptv.txt>`_      TV encoder
                         NV30:NV50 
00f000:010000   PVP1     NV41:NV50 `<vdec/vpe/vp1.txt>`_          VP1 video processing engine
088000:089000   PPCI     NV40:NV50 :ref:`ppci-mmio`               PCI config space access
090000:0a0000   PFIFO    NV40:NV50 `<fifo/nv04-pfifo.txt>`_       part of PFIFO
                cache
0a0000:0c0000   PRMFB    all       `<display/nv03/vga.txt>`_      aliases VGA memory window
0c0000:0c1000   PRMVIO   all       `<display/nv03/vga.txt>`_      aliases VGA sequencer and graphics controller registers
0c2000:0c3000   PRMVIO2  NV40:NV50 `<display/nv03/vga.txt>`_      like PRMVIO, but for second head
100000:101000   PFB      all       `<memory/nv03-pfb.txt>`_       memory interface and PCIE GART
                         except    `<memory/nv10-pfb.txt>`_
			 IGPs      `<memory/nv40-pfb.txt>`_
			           `<memory/nv44-pfb.txt>`_
101000:102000   PSTRAPS  all       `<io/pstraps.txt>`_            straps readout / override
                         except
			 IGPs
102000:103000   ???      NV40+     ???                            ???
                         IGPs only
110000:120000   PROM     NV03:NV04 `<io/prom.txt>`_               ROM access window
200000:201000   PMEDIA   all       `<io/pmedia.txt>`_             mediaport
                         except
			 IGPs
300000:400000   PROM     NV04:NV50 `<io/prom.txt>`_               ROM access window
                         except
			 IGPs
400000:401000   PGRAPH   NV03:NV04 `<graph/nv03-pgraph.txt>`_     accelerated 2d/3d drawing engine
401000:402000   PDMA     NV03:NV04 `<graph/nv03-pdma.txt>`_       system memory DMA engine
400000:402000   PGRAPH   NV04:NV40 `<graph/nv04-pgraph.txt>`_     accelerated 2d/3d drawing engine
                                   `<graph/nv10-pgraph.txt>`_
                                   `<graph/nv20-pgraph.txt>`_
400000:410000   PGRAPH   NV40:NV50 `<graph/nv40-pgraph.txt>`_     accelerated 2d/3d drawing engine
600000:601000   PCRTC    NV04:NV50 `<display/nv03/pcrtc.txt>`_    more CRTC controls
601000:602000   PRMCIO   all       `<display/nv03/vga.txt>`_      aliases VGA CRTC and attribute controller registers
602000:603000   PCRTC2   NV11:NV20 `<display/nv03/pcrtc.txt>`_    like PCRTC, but for second head
                         NV25:NV50
603000:604000   PRMCIO2  NV11:NV20 `<display/nv03/vga.txt>`_      like PRMCIO, but for second head
                         NV25:NV50
680000:681000   PRAMDAC  all       `<display/nv03/pramdac.txt>`_  RAMDAC, video overlay, cursor, and PLL control
681000:682000   PRMDIO   all       `<display/nv03/vga.txt>`_      aliases VGA palette registers
682000:683000   PRAMDAC2 NV11:NV20 `<display/nv03/pramdac.txt>`_  like PRAMDAC, but for second head
                         NV25:NV50
683000:684000   PRMDIO2  NV11:NV20 `<display/nv03/vga.txt>`_      like PRMDIO, but for second head
                         NV25:NV50
700000:800000   PRAMIN   NV04:NV50 `<memory/nv04-vram.txt>`_      RAMIN access
800000:1000000  USER     all       `<fifo/pio.txt>`_              PFIFO MMIO and DMA submission area
                                   `<fifo/dma-pusher.txt>`_
c00000:1000000  NEW_USER NV40:NV50 `<fifo/dma-pusher.txt>`_       PFIFO DMA submission area
=============== ======== ========= ============================== ======================

.. todo:: check PSTRAPS on IGPs


NV50:NVC0 MMIO map
==================

============== ===== ============= ========= ================================ ======================
Address range  Port  Name          Variants  Reference                        Description
============== ===== ============= ========= ================================ ======================
000000:001000  ROOT  PMC           all       :ref:`pmc-mmio`                  card master control
001000:002000  ROOT  PBUS          all       :ref:`pbus-mmio`                 bus control
002000:004000  ROOT  PFIFO         all       `<fifo/nv50-pfifo.txt>`_         DMA FIFO submission to execution engines
004000:005000  IBUS  PCLOCK        all       `<pm/nv50-pclock.txt>`_          PLL control
                                             `<pm/nva3-pclock.txt>`_          
007000:008000  ROOT  PRMA          all       :ref:`prma-mmio`                 access to BAR0 from real mode
009000:00a000  ROOT  PTIMER        all       :ref:`ptimer-mmio-nv03`          time measurement and time-based alarms
00a000:00b000  IBUS  PCOUNTER      all       `<pcounter/intro.txt>`_          performance monitoring counters
00b000:00c000  IBUS  PVPE          all       `<vdec/vpe/intro.txt>`_          MPEG2 decoding engine
00c000:00d000  IBUS  PCONTROL      all       `<pm/nv50-pclock.txt>`_          control of misc stuff
                                             `<pm/nva3-pclock.txt>`_          
00e000:00e800  IBUS  PNVIO         all       `<io/pnvio.txt>`_                GPIOs, I2C buses, PWM fan control, and other external devices
00e800:00f000  IBUS  PIOCLOCK      all       `<pm/nv50-pclock.txt>`_          PNVIO's clock setup
00f000:010000  IBUS  PVP1          VP1       `<vdec/vpe/vp1.txt>`_            VP1 video processing engine
00f000:010000  IBUS  PVP2          VP2       `<vdec/vp2/pvp2.txt>`_           VP2 xtensa video processing engine
010000:020000  ROOT  ???           all       ???                              has something to do with PCI config spaces of other devices?
020000:021000  IBUS  PTHERM        all       `<pm/ptherm.txt>`_               thermal sensor
021000:022000  IBUS  PFUSE         all       :ref:`pfuse-mmio`                efuses storing not so secret stuff
022000:022400  IBUS  ???           ???       ???                              ???
060000:061000  ROOT  PEEPHOLE      NV84:NVC0 `<memory/peephole.txt>`_         indirect VM access
070000:071000  ROOT  PFIFO         NV84:NVC0 `<memory/nv50-host-mem.txt>`_    used to flush BAR writes
                     BAR_FLUSH                                                
080000:081000  ROOT  PBUS HWSQ     NV92:NVC0 :ref:`hwsq-mmio`                 extended HWSQ code space
                     NEW_CODE                                                 
084000:085000  IBUS  PVLD          VP3, VP4  `<vdec/vp3/pvld.txt>`_           VP3 variable length decoding engine
085000:086000  IBUS  PVDEC         VP3, VP4  `<vdec/vp3/pvdec.txt>`_          VP3 video decoding engine
086000:087000  IBUS  PPPP          VP3, VP4  `<vdec/vp3/pppp.txt>`_           VP3 video postprocessing engine
087000:088000  IBUS  PCRYPT3       VP3       `<vdec/vp3/pcrypt3.txt>`_        VP3 cryptographic engine
088000:089000  IBUS  PPCI          all       :ref:`ppci-mmio`                 PCI config space access
089000:08a000  IBUS  ???           NV84:NVC0 ???                              ???
08a000:08b000  IBUS  PPCI_HDA      NVA3:NVC0 :ref:`ppci-hda-mmio`             PCI config space access for the HDA codec function
090000:0a0000  ROOT  PFIFO cache   all       `<fifo/nv50-pfifo.txt>`_         part of PFIFO
0a0000:0c0000  ROOT  PRMFB         all       `<display/nv50/vga.txt>`_        aliases VGA memory window
100000:101000  IBUS  PFB           all       `<memory/nv50-pfb.txt>`_         memory interface and VM control
101000:102000  IBUS  PSTRAPS       all       `<io/pstraps.txt>`_              straps readout / override
102000:103000  IBUS  PCRYPT2       VP2       `<vdec/vp2/pcrypt2.txt>`_        VP2 cryptographic engine
102000:103000  ROOT  ???           IGPs only ???                              ???
103000:104000  IBUS  PBSP          VP2       `<vdec/vp2/pbsp.txt>`_           VP2 BSP engine
104000:105000  IBUS  PCOPY         NVA3:NVC0 `<fifo/pcopy.txt>`_              memory copy engine
108000:109000  IBUS  PCODEC        NVA3:NVC0 `<display/nv50/pcodec.txt>`_     the HDA codec doing HDMI audio
109000:10a000  IBUS  PKFUSE        NVA3:NVC0 `<display/nv50/pkfuse.txt>`_     efuses storing secret key stuff
10a000:10b000  IBUS  PDAEMON       NVA3:NVC0 `<pm/pdaemon.txt>`_              a falcon engine used to run management code in background
1c1000:1c2000  IBUS  PVCOMP        NVAF:NVC0 `<vdec/pvcomp.txt>`_             video compositor engine
200000:201000  IBUS  PMEDIA        all       `<io/pmedia.txt>`_               mediaport
280000:2a0000  ROOT  ???           NVAF      ???                              ???
2ff000:300000  IBUS  PBRIDGE_PCI   IGPs      :ref:`pbus-mmio`                 access to PCI config registers of the GPU's upstream PCIE bridge
300000:400000  IBUS  PROM          all       `<io/prom.txt>`_                 ROM access window
400000:410000  IBUS  PGRAPH        all       `<graph/nv50-pgraph.txt>`_       accelerated 2d/3d drawing and CUDA engine
601000:602000  IBUS  PRMIO         all       `<display/nv50/vga.txt>`_        aliases VGA registers
610000:640000  IBUS  PDISPLAY      all       `<display/nv50/pdisplay.txt>`_   the DMA FIFO controlled unified display engine
640000:650000  IBUS  DISPLAY_USER  all       `<display/nv50/pdisplay.txt>`_   DMA submission to PDISPLAY
700000:800000  ROOT  PMEM          all       `<memory/nv50-host-mem.txt>`_    indirect VRAM/host memory access
800000:810000  ROOT  USER_PIO      all       `<fifo/pio.txt>`_                PFIFO PIO submission area
c00000:1000000 ROOT  USER_DMA      all       `<fifo/dma-pusher.txt>`_         PFIFO DMA submission area
============== ===== ============= ========= ================================ ======================

.. note:: VP1 is NV50:NV84

          VP2 is NV84:NV98 and NVA0:NVAA

	  VP3 is NV98:NVA0 and NVAA:NVA3

	  VP4 is NVA3:NVC0

.. todo:: 10f000:112000 range on NVA3-


NVC0+ MMIO map
==============

============== ===== ============= ========= ================================ ======================
Address range  Port  Name          Variants  Reference                        Description
============== ===== ============= ========= ================================ ======================
000000:001000  ROOT  PMC           all       :ref:`pmc-mmio`                  card master control
001000:002000  ROOT  PBUS          all       :ref:`pbus-mmio`                 bus control
002000:004000  ROOT  PFIFO         all       `<fifo/nvc0-pfifo.txt>`_         DMA FIFO submission to execution engines
005000:006000  ROOT  PFIFO_BYPASS  all       `<fifo/nvc0-pfifo.txt>`_         PFIFO bypass interface
007000:008000  ROOT  PRMA          all       :ref:`prma-mmio`                 access to BAR0 from real mode
009000:00a000  ROOT  PTIMER        all       :ref:`ptimer-mmio-nv03`          time measurement and time-based alarms
00c800:00cc00  IBUS  ???           all       ???                              ???
00cc00:00d000  IBUS  ???           all       ???                              ???
00d000:00e000  IBUS  PGPIO         NVD9-     `<io/pnvio.txt>`_                GPIOs, I2C buses
00e000:00e800  IBUS  PNVIO         all       `<io/pnvio.txt>`_                GPIOs, I2C buses, PWM fan control, and other external devices
00e800:00f000  IBUS  PIOCLOCK      all       `<pm/nvc0-pclock.txt>`_          PNVIO's clock setup
010000:020000  ROOT  ???           all       ???                              has something to do with PCI config spaces of other devices?
020000:021000  IBUS  PTHERM        all       `<pm/ptherm.txt>`_               thermal sensor
021000:022000  IBUS  PFUSE         all       :ref:`pfuse-mmio`                efuses storing not so secret stuff
022400:022800  IBUS  PUNITS        all       :ref:`punits-mmio`               control over enabled card units
040000:060000  ROOT  PSUBFIFOs     all       `<fifo/nvc0-pfifo.txt>`_         individual SUBFIFOs of PFIFO
060000:061000  ROOT  PEEPHOLE      all       `<memory/peephole.txt>`_         indirect VM access
070000:071000  ROOT  PFIFO         all       `<memory/nvc0-host-mem.txt>`_    used to flush BAR writes
                     BAR_FLUSH
082000:082400  IBUS  ???           all       ???                              ???
082800:083000  IBUS  ???           NVC0:NVE4 ???                              ???
084000:085000  IBUS  PVLD          all       `<vdec/vp3/pvld.txt>`_           VP3 VLD engine
085000:086000  IBUS  PVDEC         all       `<vdec/vp3/pvdec.txt>`_          VP3 video decoding engine
086000:087000  IBUS  PPPP          all       `<vdec/vp3/pppp.txt>`_           VP3 video postprocessing engine
088000:089000  IBUS  PPCI          all       :ref:`ppci-mmio`                 PCI config space access
089000:08a000  IBUS  ???           NVC0:NVE4 ???                              ???
08a000:08b000  IBUS  PPCI_HDA      all       :ref:`ppci-hda-mmio`             PCI config space access for the HDA codec function
08b000:08f000  IBUS  ???           NVE4-     ???                              seems to be a new version of former 89000 area
0a0000:0c0000  both  PRMFB         all       `<display/nv50/vga.txt>`_        aliases VGA memory window
100700:100800  IBUS  PBFB_COMMON   all       `<memory/nvc0-pbfb.txt>`_        some regs shared between PBFBs???
100800:100e00  IBUS  PFFB          all       `<memory/nvc0-pffb.txt>`_        front memory interface and VM control
100f00:101000  IBUS  PFFB          all       `<memory/nvc0-pffb.txt>`_        front memory interface and VM control
101000:102000  IBUS  PSTRAPS       all       `<io/pstraps.txt>`_              straps readout / override
104000:105000  IBUS  PCOPY0        all       `<fifo/pcopy.txt>`_              memory copy engine #1
105000:106000  IBUS  PCOPY1        all       `<fifo/pcopy.txt>`_              memory copy engine #2
106000:107000  IBUS  PCOPY2        NVE4-     `<fifo/pcopy.txt>`_              memory copy engine #3
108000:108800  IBUS  PCODEC        all       `<display/nv50/pcodec.txt>`_     the HDA codec doing HDMI audio
109000:10a000  IBUS  PKFUSE        all       `<display/nv50/pkfuse.txt>`_     efuses storing secret key stuff
10a000:10b000  IBUS  PDAEMON       all       `<pm/pdaemon.txt>`_              a falcon engine used to run management code in background
10c000:10f000  IBUS  ???           ???       ???                              ???
10f000:120000  IBUS  PBFBs         all       `<memory/nvc0-pbfb.txt>`_        memory controller backends
120000:130000  IBUS  PIBUS         all       :ref:`pibus-mmio`                deals with internal bus used to reach most other areas of MMIO
130000:135000  IBUS  ???           ???       ???                              ???
137000:138000  IBUS  PCLOCK        all       `<pm/nvc0-pclock.txt>`_          clock setting
138000:139000  IBUS  ???           ???       ???                              ???
139000:13b000  IBUS  PP2P          all       `<memory/nvc0-p2p.txt>`_         peer to peer memory access
13b000:13f000  IBUS  PXBAR         all       `<memory/nvc0-pxbar.txt>`_       crossbar between memory controllers and GPCs
140000:180000  IBUS  PMFBs         all       `<memory/nvc0-pmfb.txt>`_        middle memory controllers: compression and L2 cache
180000:1c0000  IBUS  PCOUNTER      all       `<pcounter/intro.txt>`_          performance monitoring counters
1c0000:1c1000  ROOT  ???           all       ???                              related to PFIFO and playlist?
1c2000:1c3000  IBUS  PVENC         NVE4-     `<vdec/pvenc.txt>`_              H.264 video encoding engine
1c3000:1c4000  IBUS  ???           NVD9-     `<display/nv50/punk1c1.txt>`_    some falcon engine
200000:201000  ???   PMEDIA        all       `<io/pmedia.txt>`_               mediaport
300000:380000  IBUS  PROM          all       `<io/prom.txt>`_                 ROM access window
400000:600000  IBUS  PGRAPH        all       `<graph/nvc0-pgraph.txt>`_       accelerated 2d/3d drawing and CUDA engine
601000:602000  IBUS  PRMIO         all       `<display/nv50/vga.txt>`_        aliases VGA registers
610000:6c0000  IBUS  PDISPLAY      all       `<display/nv50/pdisplay.txt>`_   the DMA FIFO controlled unified display engine
700000:800000  ROOT  PMEM          all       `<memory/nvc0-host-mem.txt>`_    indirect VRAM/host memory access
800000:810000  ROOT  PFIFO_CHAN    NVE4-     `<fifo/nvc0-pfifo.txt>`_         PFIFO channel table
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
