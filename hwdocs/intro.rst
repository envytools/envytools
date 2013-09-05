.. _intro:

=======================
nVidia GPU introduction
=======================

.. contents::


Introduction
============

This file is a short introduction to nvidia GPUs and graphics cards. Note
that the schematics shown here are simplified and do not take all details
into account - consult specific unit documentation when needed.


Card schmatic
=============

An nvidia-based graphics card is made of a main GPU chip and many supporting
chips. Note that the following schematic attempts to show as many chips as
possible - not all of them are included on all cards.

::

      +------+ memory bus +---------+ analog video    +-------+
      | VRAM |------------|         |-----------------|       |
      +------+            |         | I2C bus         |  VGA  |
                          |         |-----------------|       |
 +--------------+         |         |                 +-------+
 | PCI/AGP/PCIE |---------|         |
 +--------------+         |         | TMDS video      +-------+
                          |         |-----------------|       |
    +----------+ parallel |         | analog video    |       |
    | BIOS ROM |----------|         |-----------------| DVI-I |
    +----------+  or SPI  |         | I2C bus + GPIO  |       |
                          |         |-----------------|       |
    +----------+ I2C bus  |         |                 +-------+ 
    | HDCP ROM |----------|         |
    +----------+          |         | videolink out   +------------+
                          |         |-----------------|  external  |  +----+
   +-----------+ VID GPIO |         | I2C bus         |     TV     |--| TV |
   |  voltage  |----------|         |-----------------|  encoder   |  +----+
   | regulator |          |   GPU   |                 +------------+
   +-----------+          |         |
         |       I2C bus  |         |
         +----------------|         | videolink in+out +-----+
         |                |         |------------------| SLI |
  +--------------+        |         | GPIOs            +-----+
  |   thermal    | ALERT  |         |
  |  monitoring  |--------|         | ITU-R-656 +------------+
  | +fan control | GPIO   |         |-----------|            |  +-------+
  +--------------+        |         | I2C bus   | TV decoder |--| TV in |
         |                |         |-----------|            |  +-------+
         |                |         |           +------------+
      +-----+    FAN GPIO |         |
      | fan |-------------|         | media port +--------------+
      +-----+             |         |------------| MPEG decoder |
                          |         |            +--------------+
   +-------+ HDMI bypass  |         |
   | SPDIF |--------------|         |     +----------------------+
   +-------+ audio input  |         |-----| configuration straps |
                          |         |     +----------------------+
                          +---------+

.. note:: while this schematic shows a TV output using an external encoder
          chip, newer cards have an internal TV encoder and can connect
          the output directly to the GPU. Also, external encoders are not
          limitted to TV outputs - they're also used for TMDS, DisplayPort
          and LVDS outputs on some cards.

.. note:: in many cases, I2C buses can be shared between various devices even
          when not shown by the above schema.

In summary, a card contains:

- a GPU chipset [see :ref:`chipsets` for a list]
- a PCI, AGP, or PCI-Express host interface
- on-board GPU memory [aka VRAM] - depending on GPU, various memory types can
  be supported: VRAM, EDO, SGRAM, SDR, DDR, DDR2, GDDR3, DDR3, GDDR5.
- a parallel or SPI-connected flash ROM containing the video BIOS. The BIOS
  image, in addition to standard VGA BIOS code, contains information about
  the devices and connectors present on the card and scripts to boot up and
  manage devices on the card.
- configuration straps - a set of resistors used to configure various
  functions of the card that need to be up before the card is POSTed.
  [`<io/pstraps.txt>`_]
- a small I2C EEPROM with encrypted HDCP keys [optional, some NV84:NVA3, now
  discontinued in favor of storing the keys in fuses on the GPU]
- a voltage regulator [starting with NV10 [?] family] - starting with roughly
  NV30 family, the target voltage can be set via GPIO pins on the GPU. The
  voltage regulator may also have "power good" and "emergency shutdown"
  signals connected to the GPU via GPIOs. In some rare cases, particularly
  on high-end cards, the voltage regulator may also be accessible via I2C.
- optionally [usually on high-end cards], a thermal monitoring chip
  accessible via I2C, to supplement/replace the bultin thermal sensor of
  the GPU. May or may not include autonomous fan control and fan speed
  measurement capability. Usually has a "thermal alert" pin connected to
  a GPIO.
- a fan - control and speed measurement done either by the thermal monitoring
  chip, or by the GPU via GPIOs.
- SPDIF input [rare, some NV84:NVA3] - used for audio bypass to HDMI-capable
  TMDS outputs, newer GPUs include a builtin audio codec instead.
- on-chip video outputs - video output connectors connected directly to
  the GPU. Supported output types depend on the GPU and include VGA, TV
  [composite, S-Video, or component], TMDS [ie. the protocol used in DVI
  digital and HDMI], FPD-Link [aka LVDS], DisplayPort.
- external output encoders - usually found with older GPUs which don't
  support TV, TMDS or FPD-Link outputs directly. The encoder is connected
  to the GPU via a parallel data bus ["videolink"] and a controlling I2C
  bus.
- SLI connectors [optional, newer high-end cards only] - video links used
  to transmit video to display from slave cards in SLI configuration to the
  master. Uses the same circuitry as outputs to external output encoders.
- TV decoder chip [sometimes with a tuner] connected to the capture port of
  the GPU and to an I2C bus - rare, on old cards only
- external MPEG decoder chip connected to so-called mediaport on the GPU -
  alleged to exist on some NV03/NV04/NV10 cards, but never seen in the wild

In addition to normal cards, nvidia GPUs may be found integrated on
motherboards - in this case they're often missing own BIOS and HDCP ROMs,
instead having them intergrated with the main system ROM. There are also
IGPs [Integrated Graphics Processors], which are a special variant of GPU
integrated into the main system chipset. They don't have on-board memory
or memory controller, sharing the main system RAM instead.


GPU schematic - NV03:NV50
=========================


::

  PCI/AGP/PCIE bus  +----------+        +--------+
 -------------------| PMC+PBUS |--+     |  VRAM  |
                    +----------+  |     +--------+
                         |        |          |
                         |        |          |
                         |        |          |
                   +-----------+  |       +-----+  +------+  +---------+
                   |PTIMER+PPMI|  |       | PFB |  | PROM |  | PSTRAPS |
                   +-----------+  |       +-----+  +------+  +---------+
                         |        |          |
              SYSRAM     |        +----------+
              access bus |                   | VRAM
                         |     +-------+     | access bus
                         +-----| PFIFO |-----+
                         |     +-------+     |
                         |         | |       |
                         |         | +---+   |
                         |         |     |   |  +-------------+
      +----------+       |    +--------+ |   |  | video input |
      | PCOUNTER |       +----| PGRAPH |-----+  +-------------+    
      +----------+       |    +--------+ |   |         |
                         |               |   |    +--------+
       +--------+        |         +-----+   +----| PMEDIA |
       | therm  |        |         |         |    +--------+
       | sensor |        |      +------+     |         |
       +--------+        +------| PVPE |-----+  +--------------+
                                +------+     |  | MPEG decoder |
                                             |  +--------------+
                                             |
                                +--------+   |   +-------+   +----------+
                                | PVIDEO |---+---| PCRTC |---| I2C+GPIO |
                                +--------+       +-------+   +----------+
                                     |               |
                                 +---+-------+-------+
                                 |           |
                              +-----+   +---------+   +-----------------+
                              | PTV |   | PRAMDAC |   | PCLOCK+PCONTROL |
                              +-----+   +---------+   +-----------------+
                                 |           |
                                 |           |
                           +--------+ +--------------+
                           | TV out | | video output |
                           +--------+ +--------------+

The GPU is made of:

- control circuitry:

  - :ref:`PMC <pmc>`: master control area

  - PBUS: bus control and an area where "misc" registers are thrown in. Known
    to contain at least:

    - :ref:`HWSQ <hwsq>`, a simple script engine, can poke card registers and
      sleep in a given sequence [NV17+]
    - a thermal sensor [NV30+]
    - clock gating control [NV17+]
    - indirect VRAM access from host circuitry [NV30+]
    - ROM timings control
    - PWM controller for fans and panel backlight [NV17+]

  - PPMI: PCI Memory Interface, handles SYSRAM accesses from other units of
    the GPU

  - :ref:`PTIMER <ptimer>`: measures wall time and delivers alarm interrupts

  - PCLOCK+PCONTROL: clock generation and distribution [contained in PRAMDAC
    on pre-NV40 GPUs]

  - PFB: memory controller and arbiter

  - PROM: VIOS ROM access

  - PSTRAPS: configuration straps access [`<io/pstraps.txt>`_]

- processing engines:

  - PFIFO: gathers processing commands from the command buffers prepared by
    the host and delivers them to PGRAPH and PVPE engines in orderly manner
    [`<fifo/intro.txt>`_]

  - PGRAPH: memory copying, 2d and 3d rendering engine

  - PVPE: a trio of video decoding/encoding engines

    - PMPEG: MPEG1 and MPEG2 mocomp and IDCT decoding engine [NV17+]
    - PME: motion estimation engine [NV40+]
    - PVP1: VP1 video processor [NV41+]

  - PCOUNTER: performance monitoring counters for the processing engines and
    memory controller [`<pcounter/intro.txt>`_] 

- display engines:

  - PCRTC: generates display control signals and reads framebuffer data for
    display, present in two instances on NV11+ cards; also handles GPIO and I2C

  - PVIDEO: reads and preprocesses overlay video data

  - PRAMDAC: multiplexes PCRTC, PVIDEO and cursor image data, applies palette
    LUT, coverts to output signals, present in two instances on NV11+ cards;
    on pre-NV40 cards also deals with clock generation

  - PTV: an on-chip TV encoder

- misc engines:

  - PMEDIA: controls video capture input and the mediaport, acts as a DMA
    controller for them

Almost all units of the GPU are controlled through MMIO registers accessible
by a common bus and visible through PCI BAR0 [see :ref:`bars`]. This bus is
not shown above.


GPU schematic - NV50:NVC0
=========================

::

                               +---------------+
  PCIE bus  +----------+    +--|--+   +------+ |
 -----------| PMC+PBUS |----| PFB |---| VRAM | |
            +----------+    +--|--+   +------+ |
                      |      | | |             |
           +--------+ ++-----+ | |   memory    |
           | PTHERM |  |       | |   partition |
           +--------+  |  +----|---+           |
               |       +--| PGRAPH |           |
          +---------+  |  +----|---+           |
          | PDAEMON |--+    |  +---------------+
          +---------+  |    |
                       |  +-------+       +----------+
           +-------+   +--| PFIFO |----+  | PCOUNTER |
           | PNVIO |   |  +-------+    |  +----------+
           +-------+   |      |        |
               |       |  +-------+    |  +-------+
               |       +--| PCOPY |    |  | PFUSE |
          +----------+ |  +-------+    |  +-------+
          | PDISPLAY |-+               |
          +----------+ |  +--------+   |  +--------+
               |       +--| PVCOMP |---+  | PKFUSE |
           +--------+  |  +--------+   |  +--------+
           | PCODEC |  |               |
           +--------+  |  +-----------------------+
                       +--| video decoding, crypt |
           +--------+  |  +-----------------------+
           | PMEDIA |--+
           +--------+

The GPU is made of:

- control circuitry:

  - :ref:`PMC <pmc>`: master control area
  - PBUS: bus control and an area where "misc" registers are thrown in. Known
    to contain at least:

    - :ref:`HWSQ <hwsq>`, a simple script engine, can poke card registers and
      sleep in a given sequence
    - clock gating control
    - indirect VRAM access from host circuitry

  - :ref:`PTIMER <ptimer>`: measures wall time and delivers alarm interrupts

  - PCLOCK+PCONTROL: clock generation and distribution

  - PTHERM: thermal sensor and clock throttling circuitry [`<pm/ptherm.txt>`_]

  - PDAEMON: card management microcontroller [`<pm/pdaemon.txt>`_]

  - PFB: memory controller and arbiter

- processing engines:

  - PFIFO: gathers processing commands from the command buffers prepared by
    the host and delivers them to PGRAPH and PVPE engines in orderly manner
    [`<fifo/intro.txt>`_]
  - PGRAPH: memory copying, 2d and 3d rendering engine
  - video decoding engines, see below
  - PCOPY: asynchronous copy engine [`<fifo/pcopy.txt>`_]
  - PVCOMP: video compositing engine [`<vdec/pvcomp.txt>`_]
  - PCOUNTER: performance monitoring counters for the processing engines and
    memory controller [`<pcounter/intro.txt>`_] 

- display and IO port units:

  - PNVIO: deals with misc external devices

    - GPIOs
    - fan PWM controllers
    - I2C bus controllers
    - videolink controls
    - ROM interface
    - straps interface
    - PNVIO/PDISPLAY clock generation

  - PDISPLAY: a unified display engine

  - PCODEC: audio codec for HDMI audio

- misc engines:

  - PMEDIA: controls video capture input and the mediaport, acts as a DMA
    controller for them


GPU schematic - NVC0-
=====================

.. todo:: finish file
