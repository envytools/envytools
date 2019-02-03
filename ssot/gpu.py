from ssot.gentype import (
    GenType, GenFieldRef, GenFieldBits, GenFieldInt, GenFieldString,
)

from ssot.fb import *
from ssot.bus import *


class GpuGen(GenType):
    class_prefix = 'Gen'
    slug_prefix = 'gpu-gen'


class GenNV1(metaclass=GpuGen):
    """
    The first generation of nVidia GPUs.  Includes only one GPU -- the NV1.
    It has semi-legendary status, as it's very rare and hard to get.
    The GPU is also known by its SGS-Thomson code number, STG-2000.  The most
    popular card using this GPU is Diamond EDGE 3D.

    This GPU is unusual for multiple reasons:

    - It has a builtin sound mixer with a MIDI synthetizer (aka PAUDIO).
      It is supposed to be paired with an audio codec (AD1848) for full
      integrated soundcard functionality.
    - It is not fully VGA-compatible -- there is some VGA emulation, but it's
      quite rough and many features are not supported.
    - It has no integrated DAC or clock generators -- it has to be paired with
      an accompanying external DAC, the STG-1732 or STG-1764 that will convert
      raw framebuffer contents to display pixels.  It is also charged with
      generating the clocks for the GPU.
    - The accompanying DAC chip also contains game port functionality, for
      a complete soundcard replacement.
    - As if the game port was not enough, the DAC also supports two Sega
      Saturn controller ports.
    - The so-called 3D engine renders textured quadratic surfaces, instead
      of triangles (as opposed to all later GPUs).  Rendering triangles with it
      is pretty much impossible.

    The GPU was jointly manufactured by SGS-Thomson and nVidia, and uses SGS'
    PCI vendor ID (there are apparently variants using nVidia's vendor id,
    but not much is known about these).

    There's also NV2, which has even more legendary status.  It was supposed to be
    another card based on quadratic surfaces, but it got stuck in development hell
    and never got released.  Apparently it never got to the stage of functioning
    silicon.  The device id of NV2 was supposed to be 0x0010.
    """


class GenNV3(metaclass=GpuGen):
    """
    The first [moderately] sane GPUs from nvidia, and also the first to use AGP
    bus. There are two chips in this family, and confusingly both use GPU id
    NV3, but can be told apart by revision. The original NV3 is used in RIVA 128
    cards, while the revised NV3, known as NV3T, is used in RIVA 128 ZX. NV3
    supports AGP 1x and a maximum of 4MB of VRAM, while NV3T supports AGP 2x and
    8MB of VRAM. NV3T also increased number of slots in PFIFO cache. These GPUs
    were also manufactured by SGS-Thomson and bear the code name of STG-3000.

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
    """

class GenNV4(metaclass=GpuGen):
    """
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
    """

class GenCelsius(metaclass=GpuGen):
    """
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
    """

class GenKelvin(metaclass=GpuGen):
    """
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
    """

class GenRankine(metaclass=GpuGen):
    """
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

      - 3d engine now supports depth bounds check

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

    .. todo:: figure out NV34 3d engine changes

    """

class GenCurie(metaclass=GpuGen):
    """
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
    """

class GenTesla(metaclass=GpuGen):
    """
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
        memory and using blocklinear textures from host are now ok
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
      - Merged PSEC functionality into PVLD

    - MCP89:

      - added PVCOMP, the video compositor engine
    """

class GenFermi(metaclass=GpuGen):
    """
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
    """

class GenKepler(metaclass=GpuGen):
    """
    An upgrade to Fermi.

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
    """


class GenMaxwell(metaclass=GpuGen):
    """
    """


class GenPascal(metaclass=GpuGen):
    """
    """


class GenVolta(metaclass=GpuGen):
    """
    """


class GenTuring(metaclass=GpuGen):
    """
    """


class Gpu(GenType):
    class_prefix = "Gpu"
    slug_prefix = 'gpu'
    # The major card generation.
    gen = GenFieldRef(GpuGen)
    # The chip ID, as seen in the new-format PMC.ID register.  Missing for
    # GPUs released before it.
    id = GenFieldBits(9, optional=True)
    # The bus interface used by this GPU.
    bus = GenFieldRef(BusInterface)
    # The PCI vendor ID.
    vendorid = GenFieldBits(16, optional=True)
    # The (default) PCI device ID, if any.
    pciid = GenFieldBits(16, optional=True)
    # The number of low PCI device ID bits that are variable.
    pciid_varbits = GenFieldInt(optional=True)
    # The PCI device ID for the HDA device.
    hda_pciid = GenFieldBits(16, optional=True)
    # The PCI device ID for the USB controller device.
    usb_pciid = GenFieldBits(16, optional=True)
    # The PCI device ID for the UCSI controller device.
    # USB Type-C Connector System Software Interface (UCSI).
    ucsi_pciid = GenFieldBits(16, optional=True)
    # The memory controller used by this GPU.
    fb = GenFieldRef(FbGen)
    # The approximate release date (this is cribbed straight from Wikipedia
    # and is entirely unreliable -- use at your own risk).
    date = GenFieldString()
    # Number of XF units.
    xf_count = GenFieldInt(optional=True)
    # Number of texture units on GPU (Kelvin/Rankine), pixel pipelines on GPU
    # (Curie), TPC units on GPU (Tesla) or TPCs per GPC (Fermi+).
    tpc_count = GenFieldInt(optional=True)
    # Number of FB partitions (valid on Tesla+ only so far).
    fbpart_count = GenFieldInt(optional=True)
    # Number of SMs per TPC.
    sm_tpc_count = GenFieldInt(optional=True)
    # Number of GPCs.
    gpc_count = GenFieldInt(optional=True)
    # Number of MCs per FB partitions (valid on Fermi+)
    mc_fbpart_count = GenFieldInt(optional=True)
    # Number of SUBPs per FB partitions (valid on Fermi+)
    subp_fbpart_count = GenFieldInt(optional=True)
    # Number of CEs
    ce_count = GenFieldInt(optional=True)
    # Number of PPCs per GPC.
    ppc_count = GenFieldInt(optional=True)
    # BIOS major version
    bios_major = GenFieldBits(8, optional=True)
    # BIOS chip version
    bios_chip = GenFieldBits(8, optional=True)


# Old

class GpuNV1(metaclass=Gpu):
    gen = GenNV1
    bus = BusPci
    vendorid = 0x104a
    pciid = 0x0008
    # XXX not true, but...
    pciid_varbits = 1
    fb = FbNV1
    date = "09.1995"

class GpuNV3(GpuNV1):
    gen = GenNV3
    pciid = 0x0018
    pciid_varbits = 1
    vendorid = 0x12d2
    fb = FbNV3
    date = "04.1997"

class GpuNV3T(GpuNV3):
    fb = FbNV3T
    date = "23.02.1998"

class GpuNV4(GpuNV3T):
    gen = GenNV4
    vendorid = 0x10de
    pciid = 0x0020
    pciid_varbits = 0
    fb = FbNV4
    date = "23.03.1998"

class GpuNV5(GpuNV4):
    pciid = 0x0028
    pciid_varbits = 2
    fb = FbNV5
    date = "15.03.1999"
    # BIOS major.chip.minor.patch scheme did not exist before this point
    bios_major = 2
    bios_chip = 0x05

# XXX wtf? this entry is just to cover the second pciid set...
class GpuNV6(GpuNV5):
    pciid = 0x002c

class GpuNVA(GpuNV6):
    pciid = 0x00a0
    pciid_varbits = 0
    fb = FbNVA
    date = "08.09.1999"
    # XXX
    bios_chip = ...

# Celsius

class GpuNV10(GpuNV5):
    gen = GenCelsius
    id = 0x10
    pciid = 0x0100
    xf_count = 1
    fb = FbNV10
    date = "11.10.1999"
    bios_chip = 0x10

class GpuNV15(GpuNV10):
    id = 0x15
    pciid = 0x0150
    date = "26.04.2000"
    bios_major = 0x03
    bios_chip = 0x15

class GpuNV1A(GpuNV15):
    id = 0x1a
    pciid = 0x01a0
    fb = FbNV1A
    date = "04.06.2001"
    bios_chip = 0x1a

class GpuNV11(GpuNV15):
    id = 0x11
    pciid = 0x0110
    date = "28.06.2000"
    bios_chip = 0x11

class GpuNV17(GpuNV11):
    id = 0x17
    pciid = 0x0170
    pciid_varbits = 4
    date = "06.02.2002"
    bios_major = 0x04
    bios_chip = 0x17

class GpuNV1F(GpuNV17):
    id = 0x1f
    pciid = 0x01f0
    fb = FbNV1F
    date = "01.10.2002"
    bios_chip = 0x1f

class GpuNV18(GpuNV17):
    id = 0x18
    pciid = 0x0180
    date = "25.09.2002"
    bios_chip = 0x18

# Kelvin

class GpuNV20(GpuNV11):
    gen = GenKelvin
    id = 0x20
    pciid = 0x0200
    fbpart_count = 4
    tpc_count = 2
    fb = FbNV20
    date = "27.02.2001"
    bios_chip = 0x20


class GpuNV2A(GpuNV20):
    id = 0x2a
    pciid = 0x02a0
    fb = FbNV2A
    xf_count = 2
    date = "15.11.2001"
    bios_major = None
    bios_chip = None

class GpuNV25(GpuNV20):
    id = 0x25
    pciid = 0x0250
    pciid_varbits = 4
    fb = FbNV25
    xf_count = 2
    date = "06.02.2002"
    bios_major = 0x04
    bios_chip = 0x25

class GpuNV28(GpuNV25):
    id = 0x28
    pciid = 0x0280
    date = "20.01.2003"
    bios_chip = 0x28

# Rankine

class GpuNV30(GpuNV28):
    gen = GenRankine
    id = 0x30
    pciid = 0x0300
    fb = FbNV30
    xf_count = 2
    date = "27.01.2003"
    bios_chip = 0x30

class GpuNV35(GpuNV30):
    id = 0x35
    pciid = 0x0330
    fb = FbNV35
    xf_count = 3
    date = "12.05.2003"
    bios_chip = 0x35

class GpuNV31(GpuNV30):
    id = 0x31
    pciid = 0x0310
    fb = FbNV31
    xf_count = 1
    tpc_count = 1
    fbpart_count = 2
    date = "06.03.2003"
    bios_chip = 0x31

class GpuNV36(GpuNV35):
    id = 0x36
    pciid = 0x0340
    fb = FbNV36
    xf_count = 3
    tpc_count = 1
    fbpart_count = 2
    date = "23.10.2003"
    bios_chip = 0x36

class GpuNV34(GpuNV31):
    id = 0x34
    pciid = 0x0320
    fb = FbNV10
    xf_count = 1
    fbpart_count = None
    date = "06.03.2003"
    bios_chip = 0x34

# Curie

class GpuNV40(GpuNV35):
    gen = GenCurie
    id = 0x40
    # XXX what about 0x0210?
    pciid = 0x0040
    fb = FbNV40
    xf_count = 6
    tpc_count = 4
    date = "14.04.2004"
    bios_major = 0x05
    bios_chip = 0x40

class GpuNV45(GpuNV40):
    id = 0x45

class GpuNV41(GpuNV40):
    id = 0x41
    bus = BusPcie
    pciid = 0x00c0
    fb = FbNV41
    xf_count = 5
    tpc_count = 3
    date = "08.11.2004"
    bios_chip = 0x41

class GpuNV42(GpuNV41):
    id = 0x42

class GpuNV43(GpuNV42):
    id = 0x43
    pciid = 0x0140
    fb = FbNV43
    xf_count = 3
    tpc_count = 2
    fbpart_count = 2
    date = "12.08.2004"
    bios_chip = 0x43

class GpuNV44(GpuNV43):
    id = 0x44
    pciid = 0x0160
    fb = FbNV44
    tpc_count = 2
    fbpart_count = ...
    date = "15.12.2004"
    bios_chip = 0x44

class GpuNV44A(GpuNV44):
    id = 0x4a
    bus = BusPci
    pciid = 0x0220
    date = "04.04.2005"

class GpuG70(GpuNV43):
    id = 0x47
    pciid = 0x0090
    fb = FbG70
    xf_count = 8
    tpc_count = 6
    fbpart_count = 4
    date = "22.06.2005"
    bios_chip = 0x70

class GpuG72(GpuNV44):
    id = 0x46
    pciid = 0x01d0
    fb = FbG72
    date = "18.01.2006"
    bios_chip = 0x72

class GpuG71(GpuG70):
    id = 0x49
    pciid = 0x0290
    date = "09.03.2006"
    bios_chip = 0x71

class GpuG73(GpuG71):
    id = 0x4b
    pciid = 0x0390
    fb = FbG73
    tpc_count = 3
    fbpart_count = 2
    date = "09.03.2006"
    bios_chip = 0x73

class GpuC51(GpuNV44):
    id = 0x4e
    bus = BusIgp
    pciid = 0x0240
    fb = FbC51
    xf_count = 1
    tpc_count = 1
    date = "20.10.2005"
    bios_chip = 0x51

class GpuMCP61(GpuC51):
    id = 0x4c
    pciid = 0x03d0
    fb = FbMCP61
    date = "06.2006"
    bios_chip = 0x61

class GpuMCP67(GpuMCP61):
    id = 0x67
    pciid = 0x0530
    date = "01.02.2006"
    bios_chip = 0x67

class GpuMCP68(GpuMCP67):
    id = 0x68
    date = "07.2007"

class GpuMCP73(GpuMCP68):
    id = 0x63
    pciid = 0x07e0
    date = "07.2007"
    # Yeah, collides with G73.
    bios_chip = 0x73

class GpuRSX(GpuG72):
    id = 0x4d
    bus = BusFlexIO
    vendorid = None
    pciid = None
    xf_count = 8
    tpc_count = 6
    date = "11.11.2006"
    bios_major = None
    bios_chip = None

# Tesla

class GpuG80(GpuG73):
    gen = GenTesla
    id = 0x50
    pciid = 0x0190
    xf_count = None
    tpc_count = 8
    sm_tpc_count = 2
    fbpart_count = 6
    fb = FbG80
    date = "08.11.2006"
    bios_major = 0x60
    bios_chip = 0x80

class GpuG84(GpuG80):
    id = 0x84
    pciid = 0x0400
    tpc_count = 2
    fbpart_count = 2
    date = "17.04.2007"
    bios_chip = 0x84

class GpuG86(GpuG84):
    id = 0x86
    pciid = 0x0420
    tpc_count = 1
    fbpart_count = 2
    date = "17.04.2007"
    bios_chip = 0x86

class GpuG92(GpuG84):
    id = 0x92
    pciid = 0x0600
    pciid_varbits = 5
    tpc_count = 8
    fbpart_count = 4
    date = "29.10.2007"
    bios_major = 0x62
    bios_chip = 0x92

class GpuG94(GpuG92):
    id = 0x94
    pciid = 0x0620
    tpc_count = 4
    fbpart_count = 4
    date = "29.07.2008"
    bios_chip = 0x94

class GpuG96(GpuG94):
    id = 0x96
    pciid = 0x0640
    tpc_count = 2
    fbpart_count = 2
    date = "29.07.2008"

class GpuG98(GpuG96):
    id = 0x98
    pciid = 0x06e0
    tpc_count = 1
    sm_tpc_count = 1
    fbpart_count = 1
    date = "04.12.2007"
    bios_chip = 0x98

class GpuG200(GpuG96):
    id = 0xa0
    pciid = 0x05e0
    tpc_count = 10
    sm_tpc_count = 3
    fbpart_count = 8
    date = "16.06.2008"
    bios_chip = 0x00

class GpuMCP77(GpuG200):
    id = 0xaa
    bus = BusIgp
    fb = FbMCP77
    pciid = 0x0840
    tpc_count = 1
    sm_tpc_count = 1
    fbpart_count = 1
    date = "06.2008"
    bios_chip = 0x77

class GpuMCP79(GpuMCP77):
    id = 0xac
    pciid = 0x0860
    sm_tpc_count = 2
    date = "06.2008"
    bios_chip = 0x79

class GpuGT215(GpuG200):
    id = 0xa3
    pciid = 0x0ca0
    hda_pciid = 0x0be4
    tpc_count = 4
    sm_tpc_count = 3
    fbpart_count = 2
    ce_count = 1
    date = "15.06.2009"
    bios_major = 0x70
    bios_chip = 0x15

class GpuGT216(GpuGT215):
    id = 0xa5
    pciid = 0x0a20
    hda_pciid = 0x0be2
    tpc_count = 2
    sm_tpc_count = 3
    fbpart_count = 2
    date = "15.06.2009"
    bios_chip = 0x16

class GpuGT218(GpuGT216):
    id = 0xa8
    pciid = 0x0a60
    hda_pciid = 0x0be3
    tpc_count = 1
    sm_tpc_count = 2
    fbpart_count = 1
    date = "15.06.2009"
    bios_chip = 0x18

class GpuMCP89(GpuGT218):
    id = 0xaf
    bus = BusIgp
    fb = FbMCP77
    pciid = 0x08a0
    hda_pciid = None
    tpc_count = 2
    sm_tpc_count = 3
    fbpart_count = 2
    date = "01.04.2010"
    bios_chip = 0x89

# Fermi

class GpuGF100(GpuGT218):
    gen = GenFermi
    id = 0xc0
    pciid = 0x06c0
    hda_pciid = 0x0be5
    sm_tpc_count = 1
    gpc_count = 4
    tpc_count = 4
    fbpart_count = 6
    mc_fbpart_count = 1
    subp_fbpart_count = 2
    ce_count = 2
    fb = FbGF100
    date = "26.03.2010"
    bios_chip = 0x00

class GpuGF104(GpuGF100):
    id = 0xc4
    pciid = 0x0e20
    hda_pciid = 0x0beb
    gpc_count = 2
    tpc_count = 4
    fbpart_count = 4
    date = "12.07.2010"
    bios_chip = 0x04

class GpuGF114(GpuGF104):
    id = 0xce
    pciid = 0x1200
    hda_pciid = 0x0e0c
    date = "25.01.2011"
    bios_chip = 0x24

class GpuGF106(GpuGF104):
    id = 0xc3
    pciid = 0x0dc0
    hda_pciid = 0x0be9
    gpc_count = 1
    tpc_count = 4
    fbpart_count = 3
    date = "03.09.2010"
    bios_chip = 0x06

class GpuGF116(GpuGF106):
    id = 0xcf
    pciid = 0x1240
    hda_pciid = 0x0bee
    date = "15.03.2011"
    bios_chip = 0x26

class GpuGF108(GpuGF106):
    id = 0xc1
    pciid = 0x0de0
    hda_pciid = 0x0bea
    gpc_count = 1
    tpc_count = 2
    fbpart_count = 1
    mc_fbpart_count = 2
    date = "03.09.2010"
    bios_chip = 0x08

class GpuGF110(GpuGF100):
    id = 0xc8
    pciid = 0x1080
    hda_pciid = 0x0e09
    date = "07.12.2010"
    bios_chip = 0x10


class GpuGF119(GpuGF108):
    id = 0xd9
    pciid = 0x1040
    pciid_varbits = 6
    hda_pciid = 0x0e08
    subp_fbpart_count = 1
    mc_fbpart_count = 1
    ce_count = 1
    date = "05.01.2011"
    bios_major = 0x75
    bios_chip = 0x19

class GpuGF117(GpuGF119):
    id = 0xd7
    pciid = 0x1140
    hda_pciid = None
    subp_fbpart_count = 2
    ppc_count = 1
    date = "04.2012"
    # XXX
    bios_chip = ...

# Kepler

class GpuGK104(GpuGF119):
    gen = GenKepler
    id = 0xe4
    pciid = 0x1180
    hda_pciid = 0x0e0a
    gpc_count = 4
    tpc_count = 2
    fbpart_count = 4
    ce_count = 3
    subp_fbpart_count = 4
    ppc_count = 1
    date = "22.03.2012"
    bios_major = 0x80
    bios_chip = 0x04

class GpuGK107(GpuGK104):
    id = 0xe7
    pciid = 0x0fc0
    hda_pciid = 0x0e1b
    gpc_count = 1
    tpc_count = 2
    fbpart_count = 2
    date = "24.04.2012"
    bios_chip = 0x07

class GpuGK106(GpuGK107):
    id = 0xe6
    pciid = 0x11c0
    hda_pciid = 0x0e0b
    gpc_count = 3
    tpc_count = 2
    fbpart_count = 3
    date = "22.04.2012"
    bios_chip = 0x06

class GpuGK110(GpuGK106):
    id = 0xf0
    pciid = 0x1000
    hda_pciid = 0x0e1a
    gpc_count = 5
    tpc_count = 3
    fbpart_count = 6
    ppc_count = 2
    date = "21.02.2013"
    bios_chip = 0x10

class GpuGK110B(GpuGK110):
    id = 0xf1
    date = "07.11.2013"
    bios_chip = 0x80

class GpuGK210(GpuGK110):
    # XXX
    id = ...
    pciid = ...
    hda_pciid = ...
    date = ...
    bios_chip = ...

class GpuGK208(GpuGK110):
    id = 0x108
    pciid = 0x1280
    hda_pciid = 0x0e0f
    gpc_count = 1
    tpc_count = 2
    fbpart_count = 1
    ppc_count = 1
    subp_fbpart_count = 2
    date = "19.02.2013"
    bios_chip = 0x28

class GpuGK208B(GpuGK208):
    id = 0x106
    date = ...

class GpuGK20A(GpuGK208):
    id = 0xea
    bus = BusTegra
    fb = FbGK20A
    vendorid = None
    pciid = None
    hda_pciid = None
    gpc_count = 1
    tpc_count = 1
    fbpart_count = 1
    subp_fbpart_count = 1
    date = ...
    bios_major = None
    bios_chip = None

# Maxwell

class GpuGM107(GpuGK208):
    gen = GenMaxwell
    id = 0x117
    pciid = 0x1380
    hda_pciid = 0x0fbc
    gpc_count = 1
    tpc_count = 5
    fbpart_count = 2
    ppc_count = 2
    subp_fbpart_count = 4
    date = "18.02.2014"
    bios_major = 0x82
    bios_chip = 0x07

class GpuGM108(GpuGM107):
    id = 0x118
    pciid = 0x1340
    # XXX
    hda_pciid = ...
    gpc_count = 1
    tpc_count = 3
    fbpart_count = 1
    ppc_count = ...
    subp_fbpart_count = ...
    date = ...
    bios_chip = 0x08

class GpuGM204(GpuGM108):
    id = 0x124
    pciid = 0x13c0
    hda_pciid = 0x0fbb
    gpc_count = ...
    tpc_count = ...
    fbpart_count = ...
    bios_major = 0x84
    bios_chip = 0x04

class GpuGM200(GpuGM204):
    id = 0x120
    pciid = 0x17c0
    hda_pciid = 0x0fb0
    # XXX unverified
    bios_chip = 0x00

class GpuGM206(GpuGM204):
    id = 0x126
    pciid = 0x1400
    hda_pciid = 0x0fba
    bios_chip = 0x06

class GpuGM20B(GpuGM204):
    id = 0x12b
    bus = BusTegra
    fb = FbGK20A
    vendorid = None
    pciid = None
    hda_pciid = None
    bios_major = None
    bios_chip = None

# Pascal

class GpuGP100(GpuGM204):
    gen = GenPascal
    id = 0x130
    pciid = 0x1580
    pciid_varbits = 7
    hda_pciid = ...
    ce_count = 3
    bios_major = 0x86
    bios_chip = 0x00

class GpuGP102(GpuGP100):
    id = 0x132
    pciid = 0x1b00
    hda_pciid = 0x10ef
    bios_chip = 0x02

class GpuGP104(GpuGP100):
    id = 0x134
    pciid = 0x1b80
    hda_pciid = 0x10f0
    gpc_count = 4
    tpc_count = 5
    fbpart_count = 4
    bios_chip = 0x04

class GpuGP106(GpuGP100):
    id = 0x136
    pciid = 0x1c00
    hda_pciid = 0x10f1
    bios_chip = 0x06

class GpuGP107(GpuGP100):
    id = 0x137
    pciid = 0x1c80
    hda_pciid = 0x0fb9
    bios_chip = 0x07

class GpuGP108(GpuGP100):
    id = 0x138
    pciid = 0x1d00
    hda_pciid = 0x0fb8
    bios_chip = 0x08

# Volta

class GpuGV100(GpuGP100):
    gen = GenVolta
    id = 0x140
    pciid = 0x1d80
    pciid_varbits = 7
    hda_pciid = 0x10f2
    gpc_count = 6
    tpc_count = 7
    bios_major = 0x88
    bios_chip = 0x00

# Turing

class GpuTU102(GpuGV100):
    gen = GenTuring
    id = 0x162
    pciid = 0x1e00
    pciid_varbits = 7
    hda_pciid = 0x10f7
    usb_pciid = 0x1ad6
    gpc_count = 6
    tpc_count = 6
    bios_major = 0x90
    bios_chip = 0x02

class GpuTU104(GpuTU102):
    id = 0x164
    pciid = 0x1e80
    hda_pciid = 0x10f8
    usb_pciid = 0x1ad8
    gpc_count = 6
    tpc_count = 4
    bios_major = 0x90
    bios_chip = 0x04

class GpuTU106(GpuTU102):
    id = 0x166
    pciid = 0x1f00
    hda_pciid = 0x10f9
    usb_pciid = 0x1ada
    gpc_count = 3
    tpc_count = 6
    bios_major = 0x90
    bios_chip = 0x06


from ssot.cgen import CGenerator, CPartEnum, CPartStruct, StructName
from ssot.bus import cgen as cgen_bus
from ssot.fb import cgen as cgen_fb

cgen = CGenerator('gpu', [cgen_bus, cgen_fb], [
    CPartEnum(GpuGen),
    CPartEnum(Gpu),
    CPartStruct(Gpu, [
        StructName,
        Gpu.id,
        Gpu.pciid,
        Gpu.pciid_varbits,
        Gpu.bios_major,
        Gpu.bios_chip,
        Gpu.bus,
        Gpu.fb,
        Gpu.gen,
    ]),
])
