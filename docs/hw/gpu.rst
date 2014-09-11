.. _gpu:

=========
GPU chips
=========

.. contents::


Introduction
============

Each nvidia GPU has several identifying numbers that can be used to determine
supported features, the engines it contains, and the register set. The most
important of these numbers is an 8-bit number known as the "GPU id".
If two cards have the same GPU id, their GPUs support identical features,
engines, and registers, with very minor exceptions. Such cards can however
still differ in the external devices they contain: output connectors,
encoders, capture chips, temperature sensors, fan controllers, installed
memory, supported clocks, etc. You can get the GPU id of a card by reading
from its :ref:`PMC area <pmc-id>`.

The GPU id is usually written as NVxx, where xx is the id written as
uppercase hexadecimal number. Note that, while cards before NV10 used another
format for their ID register and don't have the GPU id stored directly,
they are usually considered as NV1-NV5 anyway.

Nvidia uses "GPU code names" in their materials. They started out
identical to the GPU id, but diverged midway through the NV40 series
and started using a different numbering. However, for the most part nvidia
code names correspond 1 to 1 with the GPU ids.

The GPU id has a mostly one-to-many relationship with pci device ids. Note that
the last few bits [0-6 depending on GPU] of PCI device id are
changeable through straps [see :ref:`pstraps`]. When pci ids of a GPU are
listed in this file, the following shorthands are used:

1234
    PCI device id 0x1234
1234*
    PCI device ids 0x1234-0x1237, choosable by straps
123X
    PCI device ids 0x1230-0x123X, choosable by straps
124X+
    PCI device ids 0x1240-0x125X, choosable by straps
124X*
    PCI device ids 0x1240-0x127X, choosable by straps


The GPU families
================

The GPUs can roughly be grouped into a dozen or so families: NV1, NV3/RIVA,
NV4/TNT, Celsius, Kelvin, Rankine, Curie, Tesla, Fermi, Kepler, Maxwell. This
aligns with big revisions of PGRAPH, the drawing engine of the card. While most
functionality was introduced in sync with PGRAPH revisions, some other
functionality [notably video decoding hardware] gets added in GPUs late in a GPU
family and sometimes doesn't even get to the first GPU in the next GPU family.
For example, NV11 expanded upon the previous NV15 chipset by adding dual-head
support, while NV20 added new PGRAPH revision with shaders, but didn't have
dual-head - the first GPU to feature both was NV25.

Also note that a bigger GPU id doesn't always mean a newer card / card
with more features: there were quite a few places where the numbering actually
went backwards. For example, NV11 came out later than NV15 and added several
features.

Nvidia's card release cycle always has the most powerful high-end GPU
first, subsequently filling in the lower-end positions with new cut-down
GPUs. This means that newer cards in a single sub-family get progressively
smaller, but also more featureful - the first GPUs to introduce minor
changes like DX10.1 support or new video decoding are usually the low-end
ones.

The full known GPU list, sorted roughly according to introduced features,
is:

- NV1 family: NV1
- NV3 (aka RIVA) family: NV3, NV3T
- NV4 (aka TNT)  family: NV4, NV5
- Calsius family: NV10, NV15, NV1A, NV11, NV17, NV1F, NV18
- Kelvin family: NV20, NV2A, NV25, NV28
- Rankine family: NV30, NV35, NV31, NV36, NV34
- Curie family:

  - NV40 subfamily: NV40, NV45, NV41, NV42, NV43, NV44, NV44A
  - G70 subfamily: G70, G71, G73, G72
  - the IGPs: C51, MCP61, MCP67, MCP68, MCP73

- Tesla family:

  - G80 subfamily: G80
  - G84 subfamily: G84, G86, G92, G94, G96, G98
  - G200 subfamily: G200, MCP77, MCP79
  - GT215 subfamily: GT215, GT216, GT218, MCP89

- Fermi family:

  - GF100 subfamily: GF100, GF104, GF106, GF114, GF116, GF108, GF110
  - GF119 subfamily: GF119, GF117

- Kepler family: GK104, GK107, GK106, GK110, GK110B, GK208, GK20A
- Maxwell family: GM107

Whenever a range of GPUs is mentioned in the documentation, it's written as
"NVxx:NVyy". This is left-inclusive, right-noninclusive range of GPU ids
as sorted in the preceding list. For example, G200:GT218 means GPUs G200,
MCP77, MCP79, GT215, GT216. NV20:NV30 effectively means all NV20 family GPUs.


NV1 family: NV1
---------------

The first nvidia GPU. It has semi-legendary status, as it's very rare and hard
to get. Information is mostly guesswork from ancient xfree86 driver. The GPU
is also known by its SGS-Thomson code number, SGS-2000. The most popular card
using this GPU is Diamond EDGE 3D.

The GPU has integrated audio output, MIDI synthetiser and Sega Saturn game
controller port. Its rendering pipeline, as opposed to all later families,
deals with quadratic surfaces, as opposed to triangles. Its video output
circuitry is also totally different from NV3+, and replaces the VGA part as
opposed to extending it like NV3:G80 do.

There's also NV2, which has even more legendary status. It was supposed to be
another card based on quadratic surfaces, but it got stuck in development hell
and never got released. Apparently it never got to the stage of functioning
silicon.

The GPU was jointly manufactured by SGS-Thomson and NVidia, earning it
pci vendor id of 0x12d2. The pci device ids are 0x0008 and 0x0009. The device
id of NV2 was supposed to be 0x0010.

========= ==== =======
id        GPU  date
========= ==== =======
0008/0009 NV1  09.1995
========= ==== =======


NV3 (RIVA) family: NV3, NV3T
----------------------------

The first [moderately] sane GPUs from nvidia, and also the first to use AGP
bus. There are two chips in this family, and confusingly both use GPU id
NV3, but can be told apart by revision. The original NV3 is used in RIVA 128
cards, while the revised NV3, known as NV3T, is used in RIVA 128 ZX. NV3
supports AGP 1x and a maximum of 4MB of VRAM, while NV3T supports AGP 2x and
8MB of VRAM. NV3T also increased number of slots in PFIFO cache. These GPUs
were also manufactured by SGS-Thomson and bear the code name of STG-3000.

The pci vendor id is 0x12d2. The pci device ids are:

==== ==== ==========
id   GPU  date
==== ==== ==========
0018 NV3  ??.04.1997
0019 NV3T 23.02.1998
==== ==== ==========

The NV3 GPU is made of the following functional blocks:

- host interface, connected to the host machine via PCI or AGP
- two PLLs, to generate video pixel clock and memory clock
- memory interface, connected to 2MB-8MB of external VRAM via 64-bit or
  128-bit memory bus, shared with an 8-bit parallel flash ROM
- PFIFO, controlling command submission to PGRAPH and gathering commands
  through DMA to host memory or direct MMIO submission
- PGRAPH, the 2d/3d drawing engine, supporting windows GDI and Direct3D 5
  acceleration
- VGA-compatible CRTC, RAMDAC, and associated video output circuitry,
  enabling direct connection of VGA analog displays and TV connection via
  an external AD722 encoder chip
- i2c bus to handle DDC and control mediaport devices
- double-buffered video overlay and cursor circuitry in RAMDAC
- mediaport, a proprietary interface with ITU656 compatibility mode, allowing
  connection of external video capture or MPEG2 decoding chip

NV3 introduced RAMIN, an area of memory at the end of VRAM used to hold
various control structures for PFIFO and PGRAPH. On NV3, RAMIN can be
accessed in BAR1 at addresses starting from 0xc00000, while later cards have
it in BAR0. It also introduced DMA objects, a RAMIN structure used to define
a VRAM or host memory area that PGRAPH is allowed to use when executing
commands on behalf of an application. These early DMA objects are limitted to
linear VRAM and paged host memory objects, and have to be switched manually
by host. See :ref:`nv3-dmaobj` for details.


NV4 (TNT) family: NV4, NV5
--------------------------

Improved and somewhat redesigned NV3. Notable changes:

- AGP x4 support
- redesigned and improved DMA command submission
- separated core and memory clocks
- DMA objects made more orthogonal, and switched automatically by card
- redesigned PGRAPH objects, introducing the concept of object class in hardware
- added BIOS ROM shadow in RAMIN
- Direct3D 6 / multitexturing support in PGRAPH
- bumped max supported VRAM to 16MB
- [NV5] bumped max supported VRAM to 32MB
- [NV5] PGRAPH 2d context object binding in hardware

This family includes the original NV4, used in RIVA TNT cards, and NV5 used
in RIVA TNT2 and Vanta cards.

This is the first chip marked as solely nvidia chip, the pci vendor id is
0x10de. The pci device ids are:

===== ========= ==========
id    GPU       date
===== ========= ==========
0020  NV4       23.03.1998
0028* NV5       15.03.1998
002c* NV5       15.03.1998
00a0  NVA IGP   08.09.1999
===== ========= ==========

.. todo:: what the fuck?


Celsius family: NV10, NV15, NV1A, NV11, NV17, NV1F, NV18
--------------------------------------------------------

The notable changes in this generation are:

- NV10:

  - redesigned memory controller
  - max VRAM bumped to 128MB
  - redesigned VRAM tiling, with support for multiple tiled regions
  - greatly expanded 3d engine: hardware T&L, D3D7, and other features
  - GPIO pins introduced for ???
  - PFIFO: added REF_CNT and NONINC commands
  - added PCOUNTER: the performance monitoring engine
  - new and improved video overlay engine
  - redesigned mediaport

- NV15:

  - introduced vblank wait PGRAPH commands
  - minor 3d engine additions [logic operation, ...]

- NV1A:

  - big endian mode
  - PFIFO: semaphores and subroutines

- NV11:

  - dual head support, meant for laptops with flat panel + external display

- NV17:

  - builtin TV encoder
  - ZCULL
  - added VPE: MPEG2 decoding engine

- NV18:

  - AGP x8 support
  - second straps set

.. todo:: what were the GPIOs for?

The GPUs are:

===== ==== ========= ======= ========== ========
pciid GPU  pixel     texture date       notes
           pipelines units
           and ROPs
===== ==== ========= ======= ========== ========
0100* NV10 4         4       11.10.1999 the first GeForce card [GeForce 256]
0150* NV15 4         8       26.04.2000 the high-end card of GeForce 2 lineup [GeForce 2 Ti, ...]
01a0* NV1A 2         4       04.06.2001 the IGP of GeForce 2 lineup [nForce]
0110* NV11 2         4       28.06.2000 the low-end card of GeForce 2 lineup [GeForce 2 MX]
017X  NV17 2         4       06.02.2002 the low-end card of GeForce 4 lineup [GeForce 4 MX]
01fX  NV1F 2         4       01.10.2002 the IGP of GeForce 4 lineup [nForce 2]
018X  NV18 2         4       25.09.2002 like NV17, but with added AGP x8 support
===== ==== ========= ======= ========== ========

The pci vendor id is 0x10de.

NV1A and NV1F are IGPs and lack VRAM, memory controller, mediaport, and ROM
interface. They use the internal interfaces of the northbridge to access
an area of system memory set aside as fake VRAM and BIOS image.


Kelvin family: NV20, NV2A, NV25, NV28
-------------------------------------

The first cards of this family were actually developed before NV17, so they
miss out on several features introduced in NV17. The first card to merge NV20
and NV17 additions is NV25. Notable changes:

- NV20:

  - no dual head support again
  - no PTV, VPE
  - no ZCULL
  - a new memory controller with Z compression
  - RAMIN reversal unit bumped to 0x40 bytes
  - 3d engine extensions:

    - programmable vertex shader support
    - D3D8, shader model 1.1

  - PGRAPH automatic context switching

- NV25:

  - a merge of NV17 and NV20: has dual-head, ZCULL, ...
  - still no VPE and PTV

- NV28:

  - AGP x8 support

The GPUs are:

===== ==== ======= ========= ======= ========== ========
pciid GPU  vertex  pixel     texture date       notes
           shaders pipelines units
                   and ROPs
===== ==== ======= ========= ======= ========== ========
0200* NV20 1       4         8       27.02.2001 the only GPU of GeForce 3 lineup [GeForce 3 Ti, ...]
02a0* NV2A 2       4         8       15.11.2001 the XBOX IGP [XGPU]
025X  NV25 2       4         8       06.02.2002 the high-end GPU of GeForce 4 lineup [GeForce 4 Ti]
028X  NV28 2       4         8       20.01.2003 like NV25, but with added AGP x8 support
===== ==== ======= ========= ======= ========== ========

NV2A is a GPU designed exclusively for the original xbox, and can't be
found anywhere else. Like NV1A and NV1F, it's an IGP.

.. todo:: verify all sorts of stuff on NV2A

The pci vendor id is 0x10de.


Rankine family: NV30, NV35, NV31, NV36, NV34
--------------------------------------------

The infamous GeForce FX series. Notable changes:

- NV30:

  - 2-stage PLLs introduced [still located in PRAMDAC]
  - max VRAM size bumped to 256MB
  - 3d engine extensions:

    - programmable fragment shader support
    - D3D9, shader model 2.0

  - added PEEPHOLE indirect memory access
  - return of VPE and PTV
  - new-style memory timings

- NV35:

  - 3d engine additions:

    - ???

- NV31:

  - no NV35 changes, this GPU is derived from NV30
  - 2-stage PLLs split into two registers
  - VPE engine extended to work as a PFIFO engine

- NV36:

  - a merge of NV31 and NV35 changes from NV30

- NV34:

  - a comeback of NV10 memory controller!
  - NV10-style mem timings again
  - no Z compression again
  - RAMIN reversal unit back at 16 bytes
  - 3d engine additions:

    - ???

.. todo:: figure out 3d engine changes

The GPUs are:

===== ==== ======= ========= ========== ========
pciid GPU  vertex  pixel     date       notes
           shaders pipelines
                   and ROPs
===== ==== ======= ========= ========== ========
030X  NV30 2       8         27.01.2003 high-end GPU [GeForce FX 5800]
033X  NV35 3       8         12.05.2003 very high-end GPU [GeForce FX 59X0]
031X  NV31 1       4         06.03.2003 low-end GPU [GeForce FX 5600]
034X  NV36 3       4         23.10.2003 middle-end GPU [GeForce FX 5700]
032X  NV34 1       4         06.03.2003 low-end GPU [GeForce FX 5200]
===== ==== ======= ========= ========== ========

The pci vendor id is 0x10de.


Curie family
------------

This family was the first to feature PCIE cards, and many fundamental areas
got significant changes, which later paved the way for G80. It is also the
family where GPU ids started to diverge from nvidia code names. The changes:

- NV40:

  - RAMIN bumped in size to max 16MB, many structure layout changes
  - RAMIN reversal unit bumped to 512kB
  - 3d engine: support for shader model 3 and other additions
  - Z compression came back
  - PGRAPH context switching microcode
  - redesigned clock setup
  - separate clock for shaders
  - rearranged PCOUNTER to handle up to 8 clock domains
  - PFIFO cache bumped in size and moved location
  - added independent PRMVIO for two heads
  - second set of straps added, new strap override registers
  - new PPCI PCI config space access window
  - MPEG2 encoding capability added to VPE
  - FIFO engines now identify the channels by their context addresses, not chids
  - BIOS uses all-new BIT structure to describe the card
  - individually disablable shader and ROP units.
  - added PCONTROL area to... control... stuff?
  - memory controller uses NV30-style timings again

- NV41:

  - introduced context switching to VPE
  - introduced PVP1, microcoded video processor
  - first natively PCIE card
  - added PCIE GART to memory controller

- NV43:

  - added a thermal sensor to the GPU

- NV44:

  - a new PCIE GART page table format
  - 3d engine: ???

- NV44A:

  - like NV44, but AGP instead of PCIE

.. todo:: more changes
.. todo:: figure out 3d engine changes

The GPUs are [vertex shaders : pixel shaders : ROPs]:

========= ========= ============== ======= ======= ==== ========== =====
pciid     GPU id    GPU names      vertex  pixel   ROPs date       notes
                                   shaders shaders
========= ========= ============== ======= ======= ==== ========== =====
004X 021X 0x40/0x45 NV40/NV45/NV48 6       16      16   14.04.2004 AGP
00cX      0x41/0x42 NV41/NV42      5       12      12   08.11.2004
014X      0x43      NV43           3       8       4    12.08.2004
016X      0x44      NV44           3       4       2    15.12.2004 TURBOCACHE
022X      0x4a      NV44A          3       4       2    04.04.2005 AGP
009X      0x47      G70            8       24      16   22.06.2005
01dX      0x46      G72            3       4       2    18.01.2006 TURBOCACHE
029X      0x49      G71            8       24      16   09.03.2006
039X      0x4b      G73            8       12      8    09.03.2006
024X      0x4e      C51            1       2       1    20.10.2005 IGP, TURBOCACHE
03dX      0x4c      MCP61          1       2       1    ??.06.2006 IGP, TURBOCACHE
053X      0x67      MCP67          1       2       2    01.02.2006 IGP, TURBOCACHE
053X      0x68      MCP68          1       2       2    ??.07.2007 IGP, TURBOCACHE
07eX      0x63      MCP73          1       2       2    ??.07.2007 IGP, TURBOCACHE
\-        ???       RSX            ?       ?       ?    11.11.2006 FlexIO bus interface, used in PS3
========= ========= ============== ======= ======= ==== ========== =====

.. todo:: all geometry information unverified

.. todo:: any information on the RSX?

It's not clear how NV40 is different from NV45, or NV41 from NV42,
or MCP67 from MCP68 - they even share pciid ranges.

The NV4x IGPs actually have a memory controller as opposed to earlier ones.
This controller still accesses only host memory, though.

As execution units can be disabled on NV40+ cards, these configs are just the
maximum configs - a card can have just a subset of them enabled.


Tesla family
------------

The card where they redesigned everything. The most significant change was the
redesigned memory subsystem, complete with a paging MMU [see :ref:`g80-vm`].

- G80:

  - a new VM subsystem, complete with redesigned DMA objects
  - RAMIN is gone, all structures can be placed arbitrarily in VRAM, and
    usually host memory memory as well
  - all-new channel structure storing page tables, RAMFC, RAMHT, context
    pointers, and DMA objects
  - PFIFO redesigned, PIO mode dropped
  - PGRAPH redesigned: based on unified shader architecture, now supports
    running standalone computations, D3D10 support, unified 2d acceleration
    object
  - display subsystem reinvented from scratch: a stub version of the old
    VGA-based one remains for VGA compatibility, the new one is not VGA based
    and is controlled by PFIFO-like DMA push buffers
  - memory partitions tied directly to ROPs

- G84:

  - redesigned channel structure with a new layout
  - got rid of VP1 video decoding and VPE encoding support, but VPE decoder
    still exists
  - added VP2 xtensa-based programmable video decoding and BSP engines
  - removed restrictions on host memory access by rendering: rendering to host
    memory and using tiled textures from host are now ok
  - added VM stats write support to PCOUNTER
  - PEEPHOLE moved out of PBUS
  - PFIFO_BAR_FLUSH moved out of PFIFO

- G98:

  - introduced VP3 video decoding engines, and the falcon microcode with them
  - got rid of VP2 video decoding

- G200:

  - developped in parallel with G98
  - VP2 again, no VP3
  - PGRAPH rearranged to make room for more MPs/TPCs
  - streamout enhancements [ARB_transform_feedback2]
  - CUDA ISA 1.3: 64-bit g[] atomics, s[] atomics, voting, fp64 support

- MCP77:

  - merged G200 and G98 changes: has both VP3 and new PGRAPH
  - only CUDA ISA 1.2 now: fp64 support got cut out again

- GT215:

  - a new revision of the falcon ISA
  - a revision to VP3 video decoding, known as VP4. Adds MPEG-4 ASP support.
  - added PDAEMON, a falcon engine meant to do card monitoring and power maanagement
  - PGRAPH additions for D3D10.1 support
  - added HDA audio codec for HDMI sound support, on a separate PCI function
  - Added PCOPY, the dedicated copy engine
  - Merged PCRYPT3 functionality into PVLD

- MCP89:

  - added PVCOMP, the video compositor engine

The GPUs in this family are:

===== ===== ==== =========== ==== ======= ===== ========== ======
core  hda   id   name        TPCs MPs/TPC PARTs date       notes
pciid pciid
===== ===== ==== =========== ==== ======= ===== ========== ======
019X  \-    0x50 G80         8    2       6     08.11.2006
040X  \-    0x84 G84         2    2       2     17.04.2007
042X  \-    0x86 G86         1    2       2     17.04.2007
060X+ \-    0x92 G92         8    2       4     29.10.2007
062X+ \-    0x94 G94         4    2       4     29.07.2008
064X+ \-    0x96 G96         2    2       2     29.07.2008
06eX+ \-    0x98 G98         1    1       1     04.12.2007
05eX+ \-    0xa0 G200        10   3       8     16.06.2008
084X+ \-    0xaa MCP77/MCP78 1    1       1     ??.06.2008 IGP
086X+ \-    0xac MCP79/MCP7A 1    2       1     ??.06.2008 IGP
0caX+ 0be4  0xa3 GT215       4    3       2     15.06.2009
0a2X+ 0be2  0xa5 GT216       2    3       2     15.06.2009
0a6X+ 0be3  0xa8 GT218       1    2       1     15.06.2009
08aX+ \-    0xaf MCP89       2    3       2     01.04.2010 IGP
===== ===== ==== =========== ==== ======= ===== ========== ======

Like NV40, these are just the maximal numbers.

The pci vendor id is 0x10de.

.. todo:: geometry information not verified for G94, MCP77


Fermi/Kepler/Maxwell family
---------------------------

The card where they redesigned everything again.

- GF100:

  - redesigned PFIFO, now with up to 3 subfifos running in parallel
  - redesigned PGRAPH:

    - split into a central HUB managing everything and several GPCs
      doing all actual work
    - GPCs further split into a common part and several TPCs
    - using falcon for context switching
    - D3D11 support

  - redesigned memory controller

    - split into three parts:

      - per-partition low-level memory controllers [PBFB]
      - per-partition middle memory controllers: compression, ECC, ... [PMFB]
      - a single "hub" memory controller: VM control, TLB control, ... [PFFB]

  - memory partitions, GPCs, TPCs have independent register areas, as well
    as "broadcast" areas that can be used to control all units at once
  - second PCOPY engine
  - redesigned PCOUNTER, now having multiple more or less independent subunits
    to monitor various parts of GPU
  - redesigned clock setting
  - ...

- GF119:

  - a major revision to VP3 video decoding, now called VP5. vµc microcode removed.
  - another revision to the falcon ISA, allowing 24-bit PC
  - added PUNK1C3 falcon engine
  - redesigned I2C bus interface
  - redesigned PDISPLAY
  - removed second PCOPY engine

- GF117:

  - PGRAPH changes:

    - ???

- GK104:

  - redesigned PCOPY: the falcon controller is now gone, replaced with hardware
    control logic, partially in PFIFO
  - an additional PCOPY engine
  - PFIFO redesign - a channel can now only access a single engine selected on
    setup, with PCOPY2+PGRAPH considered as one engine
  - PGRAPH changes:

    - subchannel to object assignments are now fixed
    - m2mf is gone and replaced by a new p2mf object that only does simple
      upload, other m2mf functions are now PCOPY's responsibility instead
    - the ISA requires explicit scheduling information now
    - lots of setup has been moved from methods/registers into memory
      structures
    - ???

- GK110:

  - PFIFO changes:

    - ???

  - PGRAPH changes:

    - ISA format change
    - ???

.. todo:: figure out PGRAPH/PFIFO changes

GPUs in Fermi/Kepler/Maxwell families:

===== ===== ===== ====== ==== ==== ===== === ====== ====== ===== ==== ===== ====== ==========
core  hda   id    name   GPCs TPCs PARTs MCs ZCULLs PCOPYs CRTCs PPCs SUBPs SPOONs date
pciid pciid                   /GPC           /GPC                /GPC /PART
===== ===== ===== ====== ==== ==== ===== === ====== ====== ===== ==== ===== ====== ==========
06cX+ 0be5  0xc0  GF100  4    4    6     [6] [4]    [2]    [2]   \-   2     3      26.03.2010
0e2X+ 0beb  0xc4  GF104  2    4    4     [4] [4]    [2]    [2]   \-   2     3      12.07.2010
120X+ 0e0c  0xce  GF114  2    4    4     [4] [4]    [2]    [2]   \-   2     3      25.01.2011
0dcX+ 0be9  0xc3  GF106  1    4    3     [3] [4]    [2]    [2]   \-   2     3      03.09.2010
124X+ 0bee  0xcf  GF116  1    4    3     [3] [4]    [2]    [2]   \-   2     3      15.03.2011
0deX+ 0bea  0xc1  GF108  1    2    1     2   4      [2]    [2]   \-   2     1      03.09.2010
108X+ 0e09  0xc8  GF110  4    4    6     [6] [4]    [2]    [2]   \-   2     3      07.12.2010
104X* 0e08  0xd9  GF119  1    1    1     1   4      1      2     \-   1     1      05.01.2011
1140  \-    0xd7  GF117  1    2    1     1   4      1      4     1    2     1      ??.04.2012
118X* 0e0a  0xe4  GK104  4    2    4     4   4      3      4     1    4     3      22.03.2012
0fcX* 0e1b  0xe7  GK107  1    2    2     2   4      3      4     1    4     3      24.04.2012
11cX+ 0e0b  0xe6  GK106  3    2    3     3   4      3      4     1    4     3      22.04.2012
100X+ 0e1a  0xf0  GK110  5    3    6     6   4      3      4     2    4     3      21.02.2013
100X+ 0e1a  0xf1  GK110B 5    3    6     6   4      3      4     2    4     3      07.11.2013
128X+ 0e0f  0x108 GK208  1    2    1     1   4      3      4     1    2     2      19.02.2013
\-    \-    0xea  GK20A  ?    ?    ?     ?   ?      \-     \-    ?    ?     1      ?
138X+ 0fbc  0x117 GM107  1    5    2     2   4      3      4     2    4     2      18.02.2014
===== ===== ===== ====== ==== ==== ===== === ====== ====== ===== ==== ===== ====== ==========

.. todo:: it is said that one of the GPCs [0th one] has only one TPC on GK106

.. todo:: what the fuck is GK110B?

.. todo:: GK20A

.. todo:: another design counter available on GM107
