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

=============== ======= =================================================
Address range   Name    Description
=============== ======= =================================================
000000:001000   PMC     :ref:`card master control <pmc-mmio>`
001000:002000   PBUS    :ref:`bus control <pbus-mmio>`
002000:004000   PFIFO   MMIO-mapped FIFO submission to PGRAPH [`<fifo/nv01-pfifo.txt>`_]
100000:101000   PDMA    :ref:`system memory DMA engine <nv01-pdma-mmio>`
101000:102000   PTIMER  :ref:`time measurement and time-based alarms <ptimer-mmio-nv01>`
300000:301000   PAUDIO  :ref:`audio capture and playback device <nv01-paudio-mmio>`
400000:401000   PGRAPH  accelerated drawing engine [`<graph/nv01-pgraph.txt>`_]
600000:601000   PFB     :ref:`VRAM and video output control <nv01-pfb-mmio>`
602000:603000   PRAM    :ref:`RAMIN layout control <nv01-pram-mmio>`
604000:605000   ???     :ref:`??? <nv01-pramunk1-mmio>`
605000:606000   PCHIPID :ref:`chip ID readout <pchipid-mmio>`
606000:607000   ???     :ref:`??? <nv01-pramunk2-mmio>`
608000:609000   PSTRAPS :ref:`straps readout / override <pstraps-mmio>`
609000:60a000   PDAC    :ref:`DAC control <nv01-pdac-mmio>`
60a000:60b000   PEEPROM :ref:`configuration EEPROM access <peeprom-mmio>`
610000:618000   PROM    :ref:`ROM access window <prom-mmio>`
618000:620000   PALT    :ref:`external memory access window <palt-mmio>`
640000:648000   PRAMHT  :ref:`RAMHT access <nv01-pramht-mmio>`
648000:64c000   PRAMFC  :ref:`RAMFC access <nv01-pramfc-mmio>`
650000:654000   PRAMRO  :ref:`RAMRO access <nv01-pramro-mmio>`
6c0000:6c8000   PRM     :ref:`VGA compatibility control - NV01 specific <nv01-prm-mmio>`
6d0000:6d1000   PRMIO   :ref:`VGA and ISA sound compat IO port access <nv01-prmio-mmio>`
6e0000:700000   PRMFB   :ref:`aliases VGA memory window <nv01-prmfb-mmio>`
700000:800000   PRAMIN  :ref:`RAMIN access <nv01-pramin-mmio>`
800000:1000000  USER    PFIFO MMIO submission area [`<fifo/pio.txt>`_]
1000000:2000000 FB      :ref:`VRAM access area <nv01-fb-mmio>`
=============== ======= =================================================


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
004000:005000   PCLOCK   NV40:NV50 :ref:`nv40-pclock-mmio`        PLL control
005000:006000   ???      all       ???                            ???
007000:008000   PRMA     all       :ref:`prma-mmio`               access to BAR0/BAR1 from real mode
008000:009000   PVIDEO   NV10:NV50 :ref:`pvideo-mmio`             video overlay
009000:00a000   PTIMER   all       :ref:`ptimer-mmio-nv03`        time measurement and time-based alarms
00a000:00b000   PCOUNTER NV10:NV50 `<pcounter/intro.txt>`_        performance monitoring counters
00b000:00c000   PVPE     NV17:NV20 :ref:`pvpe-mmio`               MPEG2 decoding engine
                         NV30:NV50 
00c000:00d000   PCONTROL NV40:NV50 :ref:`nv40-pcontrol-mmio`      control of misc stuff
00d000:00e000   PTV      NV17:NV20 :ref:`ptv-mmio`                TV encoder
                         NV30:NV50 
00f000:010000   PVP1     NV41:NV50 :ref:`pvp1-mmio`               VP1 video processing engine
088000:089000   PPCI     NV40:NV50 :ref:`ppci-mmio`               PCI config space access
090000:0a0000   PFIFO    NV40:NV50 `<fifo/nv04-pfifo.txt>`_       part of PFIFO
                cache
0a0000:0c0000   PRMFB    all       :ref:`prmfb-mmio`              aliases VGA memory window
0c0000:0c1000   PRMVIO   all       :ref:`prmvio-mmio`             aliases VGA sequencer and graphics controller registers
0c2000:0c3000   PRMVIO2  NV40:NV50 :ref:`prmvio-mmio`             like PRMVIO, but for second head
100000:101000   PFB      all       :ref:`nv03-pfb-mmio`           memory interface and PCIE GART
                         except    :ref:`nv03-pfb-mmio`    
                         IGPs      :ref:`nv40-pfb-mmio`    
                                   :ref:`nv44-pfb-mmio`    
101000:102000   PSTRAPS  all       :ref:`pstraps-mmio`            straps readout / override
                         except
                         IGPs
102000:103000   ???      NV40+     ???                            ???
                         IGPs only
110000:120000   PROM     NV03:NV04 :ref:`prom-mmio`               ROM access window
200000:201000   PMEDIA   all       :ref:`pmedia-mmio`             mediaport
                         except
                         IGPs
300000:400000   PROM     NV04:NV50 :ref:`prom-mmio`               ROM access window
                         except
                         IGPs
400000:401000   PGRAPH   NV03:NV04 `<graph/nv03-pgraph.txt>`_     accelerated 2d/3d drawing engine
401000:402000   PDMA     NV03:NV04 `<graph/nv03-pdma.txt>`_       system memory DMA engine
400000:402000   PGRAPH   NV04:NV40 `<graph/nv04-pgraph.txt>`_     accelerated 2d/3d drawing engine
                                   `<graph/nv10-pgraph.txt>`_
                                   `<graph/nv20-pgraph.txt>`_
400000:410000   PGRAPH   NV40:NV50 `<graph/nv40-pgraph.txt>`_     accelerated 2d/3d drawing engine
600000:601000   PCRTC    NV04:NV50 :ref:`pcrtc-mmio`              more CRTC controls
601000:602000   PRMCIO   all       :ref:`prmcio-mmio`             aliases VGA CRTC and attribute controller registers
602000:603000   PCRTC2   NV11:NV20 :ref:`pcrtc-mmio`              like PCRTC, but for second head
                         NV25:NV50
603000:604000   PRMCIO2  NV11:NV20 :ref:`prmcio-mmio`             like PRMCIO, but for second head
                         NV25:NV50
680000:681000   PRAMDAC  all       :ref:`pramdac-mmio`            RAMDAC, video overlay, cursor, and PLL control
681000:682000   PRMDIO   all       :ref:`prmdio-mmio`             aliases VGA palette registers
682000:683000   PRAMDAC2 NV11:NV20 :ref:`pramdac-mmio`            like PRAMDAC, but for second head
                         NV25:NV50
683000:684000   PRMDIO2  NV11:NV20 :ref:`prmdio-mmio`             like PRMDIO, but for second head
                         NV25:NV50
700000:800000   PRAMIN   NV04:NV50 :ref:`nv04-pramin-mmio`        RAMIN access
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
004000:005000  IBUS  PCLOCK        NV50:NVA3 :ref:`nv50-pclock-mmio`          PLL control
004000:005000  IBUS  PCLOCK        NVA3:NVC0 :ref:`nva3-pclock-mmio`          PLL control
007000:008000  ROOT  PRMA          all       :ref:`prma-mmio`                 access to BAR0 from real mode
009000:00a000  ROOT  PTIMER        all       :ref:`ptimer-mmio-nv03`          time measurement and time-based alarms
00a000:00b000  IBUS  PCOUNTER      all       `<pcounter/intro.txt>`_          performance monitoring counters
00b000:00c000  IBUS  PVPE          all       :ref:`pvpe-mmio`                 MPEG2 decoding engine
00c000:00d000  IBUS  PCONTROL      NV50:NVA3 :ref:`nv50-pcontrol-mmio`        control of misc stuff
00c000:00d000  IBUS  PCONTROL      NVA3:NVC0 :ref:`nva3-pcontrol-mmio`        control of misc stuff
00e000:00e800  IBUS  PNVIO         all       :ref:`pnvio-mmio`                GPIOs, I2C buses, PWM fan control, and other external devices
00e800:00f000  IBUS  PIOCLOCK      NV50:NVA3 :ref:`nv50-pioclock-mmio`        PNVIO's clock setup
00e800:00f000  IBUS  PIOCLOCK      NVA3:NVC0 :ref:`nva3-pioclock-mmio`        PNVIO's clock setup
00f000:010000  IBUS  PVP1          VP1       :ref:`pvp1-mmio`                 VP1 video processing engine
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
088000:089000  IBUS  PPCI          all       :ref:`ppci-mmio`                 PCI config space access
089000:08a000  IBUS  ???           NV84:NVC0 ???                              ???
08a000:08b000  IBUS  PPCI_HDA      NVA3:NVC0 :ref:`ppci-hda-mmio`             PCI config space access for the HDA codec function
090000:0a0000  ROOT  PFIFO cache   all       `<fifo/nv50-pfifo.txt>`_         part of PFIFO
0a0000:0c0000  ROOT  PRMFB         all       :ref:`nv50-prmfb-mmio`           aliases VGA memory window
100000:101000  IBUS  PFB           all       :ref:`nv50-pfb-mmio`             memory interface and VM control
101000:102000  IBUS  PSTRAPS       all       :ref:`pstraps-mmio`              straps readout / override
102000:103000  IBUS  PCRYPT2       VP2       :ref:`pcrypt2-mmio`              VP2 cryptographic engine
102000:103000  ROOT  ???           IGPs only ???                              ???
103000:104000  IBUS  PBSP          VP2       :ref:`pbsp-mmio`                 VP2 BSP engine
104000:105000  IBUS  PCOPY         NVA3:NVC0 :ref:`pcopy-io`                  memory copy engine
108000:109000  IBUS  PCODEC        NVA3:NVC0 :ref:`pcodec-mmio`               the HDA codec doing HDMI audio
109000:10a000  IBUS  PKFUSE        NVA3:NVC0 :ref:`pkfuse-mmio`               efuses storing secret key stuff
10a000:10b000  IBUS  PDAEMON       NVA3:NVC0 :ref:`pdaemon-io`                a falcon engine used to run management code in background
1c1000:1c2000  IBUS  PVCOMP        NVAF:NVC0 :ref:`pvcomp-io`                 video compositor engine
200000:201000  IBUS  PMEDIA        all       :ref:`pmedia-mmio`               mediaport
280000:2a0000  ROOT  ???           NVAF      ???                              ???
2ff000:300000  IBUS  PBRIDGE_PCI   IGPs      :ref:`pbus-mmio`                 access to PCI config registers of the GPU's upstream PCIE bridge
300000:400000  IBUS  PROM          all       :ref:`prom-mmio`                 ROM access window
400000:410000  IBUS  PGRAPH        all       `<graph/nv50-pgraph.txt>`_       accelerated 2d/3d drawing and CUDA engine
601000:602000  IBUS  PRMIO         all       :ref:`nv50-prmio-mmio`           aliases VGA registers
610000:640000  IBUS  PDISPLAY      all       :ref:`pdisplay-mmio`             the DMA FIFO controlled unified display engine
640000:650000  IBUS  DISPLAY_USER  all       :ref:`display-user-mmio`         DMA submission to PDISPLAY
700000:800000  ROOT  PMEM          all       :ref:`pmem-mmio`                 indirect VRAM/host memory access
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
00d000:00e000  IBUS  PGPIO         NVD9-     :ref:`pgpio-mmio`                GPIOs, I2C buses
00e000:00e800  IBUS  PNVIO         all       :ref:`pnvio-mmio`                GPIOs, I2C buses, PWM fan control, and other external devices
00e800:00f000  IBUS  PIOCLOCK      all       :ref:`nvc0-pioclock-mmio`        PNVIO's clock setup
010000:020000  ROOT  ???           all       ???                              has something to do with PCI config spaces of other devices?
020000:021000  IBUS  PTHERM        all       :ref:`ptherm-mmio`               thermal sensor
021000:022000  IBUS  PFUSE         all       :ref:`pfuse-mmio`                efuses storing not so secret stuff
022400:022800  IBUS  PUNITS        all       :ref:`punits-mmio`               control over enabled card units
040000:060000  ROOT  PSUBFIFOs     all       `<fifo/nvc0-pfifo.txt>`_         individual SUBFIFOs of PFIFO
060000:061000  ROOT  PEEPHOLE      all       :ref:`peephole-mmio`             indirect VM access
070000:071000  ROOT  PFLUSH        all       :ref:`nvc0-pflush-mmio`          used to flush BAR writes
082000:082400  IBUS  ???           all       ???                              ???
082800:083000  IBUS  ???           NVC0:NVE4 ???                              ???
084000:085000  IBUS  PVLD          all       :ref:`pvld-io`                   VP3 VLD engine
085000:086000  IBUS  PVDEC         all       :ref:`pvdec-io`                  VP3 video decoding engine
086000:087000  IBUS  PPPP          all       :ref:`pppp-io`                   VP3 video postprocessing engine
088000:089000  IBUS  PPCI          all       :ref:`ppci-mmio`                 PCI config space access
089000:08a000  IBUS  ???           NVC0:NVE4 ???                              ???
08a000:08b000  IBUS  PPCI_HDA      all       :ref:`ppci-hda-mmio`             PCI config space access for the HDA codec function
08b000:08f000  IBUS  ???           NVE4-     ???                              seems to be a new version of former 89000 area
0a0000:0c0000  both  PRMFB         all       :ref:`nv50-prmfb-mmio`           aliases VGA memory window
100700:100800  IBUS  PBFB_COMMON   all       :ref:`pbfb-mmio`                 some regs shared between PBFBs???
100800:100e00  IBUS  PFFB          all       :ref:`pffb-mmio`                 front memory interface and VM control
100f00:101000  IBUS  PFFB          all       :ref:`pffb-mmio`                 front memory interface and VM control
101000:102000  IBUS  PSTRAPS       all       :ref:`pstraps-mmio`              straps readout / override
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
180000:1c0000  IBUS  PCOUNTER      all       `<pcounter/intro.txt>`_          performance monitoring counters
1c0000:1c1000  ROOT  ???           all       ???                              related to PFIFO and playlist?
1c2000:1c3000  IBUS  PVENC         NVE4-     :ref:`pvenc-io`                  H.264 video encoding engine
1c3000:1c4000  IBUS  ???           NVD9-     :ref:`punk1c3-io`                some falcon engine
200000:201000  ???   PMEDIA        all       :ref:`pmedia-mmio`               mediaport
300000:380000  IBUS  PROM          all       :ref:`prom-mmio`                 ROM access window
400000:600000  IBUS  PGRAPH        all       `<graph/nvc0-pgraph.txt>`_       accelerated 2d/3d drawing and CUDA engine
601000:602000  IBUS  PRMIO         all       :ref:`nv50-prmio-mmio`           aliases VGA registers
610000:6c0000  IBUS  PDISPLAY      all       :ref:`pdisplay-mmio`             the DMA FIFO controlled unified display engine
700000:800000  ROOT  PMEM          all       :ref:`pmem-mmio`                 indirect VRAM/host memory access
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
