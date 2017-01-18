.. _graph-intro:

===============
PGRAPH overview
===============

.. contents::


Introduction
============

.. todo:: write me

.. todo:: WAIT_FOR_IDLE and PM_TRIGGER


NV1/NV3 graph object types
==========================

The following graphics objects exist on NV1:NV4:

==== ======== =========== =====================================
id   variants name        description
==== ======== =========== =====================================
0x01 all      BETA        :ref:`sets beta factor for blending <obj-beta>`
0x02 all      ROP         :ref:`sets raster operation <obj-rop>`
0x03 all      CHROMA      :ref:`sets color for color key <obj-chroma>`
0x04 all      PLANE       :ref:`sets the plane mask <obj-plane>`
0x05 all      CLIP        :ref:`sets clipping rectangle <obj-clip>`
0x06 all      PATTERN     :ref:`sets pattern, ie. a small repeating image used as one of the inputs to a raster operation or blending <obj-pattern>`
0x07 NV3:NV4  RECT        :ref:`renders solid rectangles <obj-rect>`
0x08 all      POINT       :ref:`renders single points <obj-point>`
0x09 all      LINE        :ref:`renders solid lines <obj-line>`
0x0a all      LIN         :ref:`renders solid lins [ie. lines missing a pixel on one end] <obj-lin>`
0x0b all      TRI         :ref:`renders solid triangles <obj-tri>`
0x0c NV1:NV3  RECT        :ref:`renders solid rectangles <obj-rect>`
0x0c NV3:NV4  GDI         :ref:`renders Windows 95 primitives: rectangles and characters, with font read from a DMA object <obj-gdi>`
0x0d NV1:NV3  TEXLIN      :ref:`renders quads with linearly mapped textures <obj-texlin>`
0x0d NV3:NV4  M2MF        :ref:`copies data from one DMA object to another <obj-m2mf>`
0x0e NV1:NV3  TEXQUAD     :ref:`renders quads with quadratically mapped textures <obj-texquad>`
0x0e NV3:NV4  SIFM        :ref:`Scaled Image From Memory, like NV1's IFM, but with scaling <obj-sifm>`
0x10 all      BLIT        :ref:`copies rectangles of pixels from one place in framebuffer to another     <obj-blit>`
0x11 all      IFC         :ref:`Image From CPU, uploads a rectangle of pixels via methods <obj-ifc>`
0x12 all      BITMAP      :ref:`uploads and expands a bitmap [ie.  1bpp image] via methods <obj-bitmap>`
0x13 NV1:NV3  IFM         :ref:`Image From Memory, uploads a rectangle of pixels from a DMA object to framebuffer <obj-ifm>`
0x14 all      ITM         :ref:`Image To Memory, downloads a rectangle of pixels to a DMA object from framebuffer <obj-itm>`
0x15 NV3:NV4  SIFC        :ref:`Stretched Image From CPU, like IFC, but with image stretching       <obj-sifc>`
0x17 NV3:NV4  D3D         :ref:`Direct3D 5 textured triangles <obj-d3d>`
0x18 NV3:NV4  ZPOINT      :ref:`renders single points to a surface with depth buffer <obj-zpoint>`
0x1c NV3:NV4  SURF        :ref:`sets rendering surface parameters <obj-surf>`
0x1d NV1:NV3  TEXLINBETA  :ref:`renders lit quads with linearly mapped textures <obj-texlinbeta>`
0x1e NV1:NV3  TEXQUADBETA :ref:`renders lit quads with quadratically mapped textures <obj-texquadbeta>`
==== ======== =========== =====================================

.. todo:: check Direct3D version


NV4+ graph object classes
=========================

Not really graph objects, but usable as parameters for some object-bind
methods [all NV4:GF100]:

====== ========== ============
class  name       description
====== ========== ============
0x0030 NV1_NULL   :ref:`does nothing <nv1-null>`
0x0002 NV1_DMA_R  :ref:`DMA object for reading <nv4-dmaobj>`
0x0003 NV1_DMA_W  :ref:`DMA object for writing <nv4-dmaobj>`
0x003d NV3_DMA    :ref:`read/write DMA object <nv4-dmaobj>`
====== ========== ============

.. todo:: document NV1_NULL

NV1-style :ref:`operation objects <obj-op>` [all NV4:NV5]:

====== ====================== ============
class  name                   description
====== ====================== ============
0x0010 NV1_OP_CLIP            clipping
0x0011 NV1_OP_BLEND_AND       blending
0x0013 NV1_OP_ROP_AND         raster operation
0x0015 NV1_OP_CHROMA          color key 
0x0064 NV1_OP_SRCCOPY_AND     source copy with 0-alpha discard
0x0065 NV3_OP_SRCCOPY         source copy
0x0066 NV4_OP_SRCCOPY_PREMULT pre-multiplying copy
0x0067 NV4_OP_BLEND_PREMULT   pre-multiplied blending
====== ====================== ============

Memory to memory copy objects:

====== =========== ========== ============
class  variants    name       description
====== =========== ========== ============
0x0039 NV4:G80     NV3_M2MF   :ref:`copies data from one buffer to another <obj-m2mf>`
0x5039 G80:GF100   G80_M2MF   :ref:`copies data from one buffer to another <obj-m2mf>`
0x9039 GF100:GK104 GF100_M2MF :ref:`copies data from one buffer to another <obj-m2mf>`
0xa040 GK104:GK110 GK104_P2MF :ref:`copies data from FIFO to memory buffer <obj-p2mf>`
       GK20A
0xa140 GK110:GK20A GK110_P2MF :ref:`copies data from FIFO to memory buffer <obj-p2mf>`
       GM107-
====== =========== ========== ============

Context objects:

====== ========= =============== ============
class  variants  name            description
====== ========= =============== ============
0x0012 NV4:G84   NV1_BETA        :ref:`sets beta factor for blending <obj-beta>`
0x0017 NV4:G80   NV1_CHROMA      :ref:`sets color for color key <obj-chroma>`
0x0057 NV4:G84   NV4_CHROMA      :ref:`sets color for color key <obj-chroma>`
0x0018 NV4:G80   NV1_PATTERN     :ref:`sets pattern for raster op <obj-pattern>`
0x0044 NV4:G84   NV1_PATTERN     :ref:`sets pattern for raster op <obj-pattern>`
0x0019 NV4:G84   NV1_CLIP        :ref:`sets user clipping rectangle <obj-clip>`
0x0043 NV4:G84   NV1_ROP         :ref:`sets raster operation <obj-rop>`
0x0072 NV4:G84   NV4_BETA4       :ref:`sets component beta factors for pre-multiplied blending <obj-beta4>`
0x0058 NV4:G80   NV3_SURF_DST    :ref:`sets the 2d destination surface <obj-surf>`
0x0059 NV4:G80   NV3_SURF_SRC    :ref:`sets the 2d blit source surface <obj-surf>`
0x005a NV4:G80   NV3_SURF_COLOR  :ref:`sets the 3d color surface <obj-surf>`
0x005b NV4:G80   NV3_SURF_ZETA   :ref:`sets the 3d zeta surface <obj-surf>`
0x0052 NV4:G80   NV4_SWZSURF     :ref:`sets 2d swizzled destination surface <obj-swzsurf>`
0x009e NV10:G80  NV10_SWZSURF    :ref:`sets 2d swizzled destination surface <obj-swzsurf>`
0x039e NV30:NV40 NV30_SWZSURF    :ref:`sets 2d swizzled destination surface <obj-swzsurf>`
0x309e NV40:G80  NV30_SWZSURF    :ref:`sets 2d swizzled destination surface <obj-swzsurf>`
0x0042 NV4:G80   NV4_SURF2D      :ref:`sets 2d destination and source surfaces <obj-surf2d>`
0x0062 NV10:G80  NV10_SURF2D     :ref:`sets 2d destination and source surfaces <obj-surf2d>`
0x0362 NV30:NV40 NV30_SURF2D     :ref:`sets 2d destination and source surfaces <obj-surf2d>`
0x3062 NV40:G80  NV30_SURF2D     :ref:`sets 2d destination and source surfaces <obj-surf2d>`
0x5062 G80:G84   G80_SURF2D      :ref:`sets 2d destination and source surfaces <obj-surf2d>`
0x0053 NV4:NV20  NV4_SURF3D      :ref:`sets 3d color and zeta surfaces <obj-surf3d>`
0x0093 NV10:NV20 NV10_SURF3D     :ref:`sets 3d color and zeta surfaces <obj-surf3d>`
====== ========= =============== ============

Solids rendering objects:

====== ========= ========= ============
class  variants  name      description
====== ========= ========= ============
0x001c NV4:NV40  NV1_LIN   :ref:`renders a lin <obj-lin>`
0x005c NV4:G80   NV4_LIN   :ref:`renders a lin <obj-lin>`
0x035c NV30:NV40 NV30_LIN  :ref:`renders a lin <obj-lin>`
0x305c NV40:G84  NV30_LIN  :ref:`renders a lin <obj-lin>`
0x001d NV4:NV40  NV1_TRI   :ref:`renders a triangle <obj-tri>`
0x005d NV4:G84   NV4_TRI   :ref:`renders a triangle <obj-tri>`
0x001e NV4:NV40  NV1_RECT  :ref:`renders a rectangle <obj-rect>`
0x005e NV4:NV40  NV4_RECT  :ref:`renders a rectangle <obj-rect>`
====== ========= ========= ============

Image upload from CPU objects:

====== ========= ============ ============
class  variants  name         description
====== ========= ============ ============
0x0021 NV4:NV40  NV1_IFC      :ref:`image from CPU <obj-ifc>`
0x0061 NV4:G80   NV4_IFC      :ref:`image from CPU <obj-ifc>`
0x0065 NV5:G80   NV5_IFC      :ref:`image from CPU <obj-ifc>`
0x008a NV10:G80  NV10_IFC     :ref:`image from CPU <obj-ifc>`
0x038a NV30:NV40 NV30_IFC     :ref:`image from CPU <obj-ifc>`
0x308a NV40:G84  NV40_IFC     :ref:`image from CPU <obj-ifc>`
0x0036 NV4:G80   NV1_SIFC     :ref:`stretched image from CPU <obj-sifc>`
0x0076 NV4:G80   NV4_SIFC     :ref:`stretched image from CPU <obj-sifc>`
0x0066 NV5:G80   NV5_SIFC     :ref:`stretched image from CPU <obj-sifc>`
0x0366 NV30:NV40 NV30_SIFC    :ref:`stretched image from CPU <obj-sifc>`
0x3066 NV40:G84  NV40_SIFC    :ref:`stretched image from CPU <obj-sifc>`
0x0060 NV4:G80   NV4_INDEX    :ref:`indexed image from CPU <obj-index>`
0x0064 NV5:G80   NV5_INDEX    :ref:`indexed image from CPU <obj-index>`
0x0364 NV30:NV40 NV30_INDEX   :ref:`indexed image from CPU <obj-index>`
0x3064 NV40:G84  NV40_INDEX   :ref:`indexed image from CPU <obj-index>`
0x007b NV10:G80  NV10_TEXTURE :ref:`texture from CPU <obj-texture>`
0x037b NV30:NV40 NV30_TEXTURE :ref:`texture from CPU <obj-texture>`
0x307b NV40:G80  NV40_TEXTURE :ref:`texture from CPU <obj-texture>`
====== ========= ============ ============

.. todo:: figure out wtf is the deal with TEXTURE objects

Other 2d source objects:

====== ========= ========= ============
class  variants  name      description
====== ========= ========= ============
0x001f NV4:G80   NV1_BLIT  :ref:`blits inside framebuffer <obj-blit>`
0x005f NV4:G84   NV4_BLIT  :ref:`blits inside framebuffer <obj-blit>`
0x009f NV15:G80  NV15_BLIT :ref:`blits inside framebuffer <obj-blit>`
0x0037 NV4:G80   NV3_SIFM  :ref:`scaled image from memory <obj-sifm>`
0x0077 NV4:G80   NV4_SIFM  :ref:`scaled image from memory <obj-sifm>`
0x0063 NV10:G80  NV5_SIFM  :ref:`scaled image from memory <obj-sifm>`
0x0089 NV10:NV40 NV10_SIFM :ref:`scaled image from memory <obj-sifm>`
0x0389 NV30:NV40 NV30_SIFM :ref:`scaled image from memory <obj-sifm>`
0x3089 NV40:G80  NV30_SIFM :ref:`scaled image from memory <obj-sifm>`
0x5089 G80:G84   G80_SIFM  :ref:`scaled image from memory <obj-sifm>`
0x004b NV4:NV40  NV3_GDI   :ref:`draws GDI primitives <obj-gdi>`
0x004a NV4:G80   NV4_GDI   :ref:`draws GDI primitives <obj-gdi>`
====== ========= ========= ============

:ref:`YCbCr two-source blending objects <obj-dvd>`:

====== ========= =========
class  variants  name     
====== ========= =========
0x0038 NV4:G80   NV4_DVD_SUBPICTURE
0x0088 NV10:G80  NV10_DVD_SUBPICTURE
====== ========= =========

.. todo:: find better name for these two

:ref:`Unified 2d objects <obj-2d>`:

====== ========== =========
class  variants   name     
====== ========== =========
0x502d G80:GF100  G80_2D
0x902d GF100-     GF100_2D
====== ========== =========

NV3-style 3d objects:

====== =========== ========== ============
class  variants    name       description
====== =========== ========== ============
0x0048 NV4:NV15    NV3_D3D    :ref:`Direct3D textured triangles <obj-d3d>`
0x0054 NV4:NV20    NV4_D3D5   :ref:`Direct3D 5 textured triangles <obj-d3d5>`
0x0094 NV10:NV20   NV10_D3D5  :ref:`Direct3D 5 textured triangles <obj-d3d5>`
0x0055 NV4:NV20    NV4_D3D6   :ref:`Direct3D 6 multitextured triangles <obj-d3d6>`
0x0095 NV10:NV20   NV10_D3D6  :ref:`Direct3D 6 multitextured triangles <obj-d3d6>`
====== =========== ========== ============

.. todo:: check NV3_D3D version

NV10-style 3d objects:

====== ============ ========== ============
class  variants     name       description
====== ============ ========== ============
0x0056 NV10:NV30    NV10_3D    :ref:`Celsius Direct3D 7 engine <obj-celsius>`
0x0096 NV15:NV30    NV15_3D    :ref:`Celsius Direct3D 7 engine <obj-celsius>`
0x0098 NV17:NV20    NV11_3D    :ref:`Celsius Direct3D 7 engine <obj-celsius>`
0x0099 NV17:NV20    NV17_3D    :ref:`Celsius Direct3D 7 engine <obj-celsius>`
0x0097 NV20:NV34    NV20_3D    :ref:`Kelvin Direct3D 8 SM 1 engine <obj-kelvin>`
0x0597 NV25:NV40    NV25_3D    :ref:`Kelvin Direct3D 8 SM 1 engine <obj-kelvin>`
0x0397 NV30:NV40    NV30_3D    :ref:`Rankine Direct3D 9 SM 2 engine <obj-rankine>`
0x0497 NV35:NV34    NV35_3D    :ref:`Rankine Direct3D 9 SM 2 engine <obj-rankine>`
0x3597 NV40:NV41    NV35_3D    :ref:`Rankine Direct3D 9 SM 2 engine <obj-rankine>`
0x0697 NV34:NV40    NV34_3D    :ref:`Rankine Direct3D 9 SM 2 engine <obj-rankine>`
0x4097 NV40:G80 !TC NV40_3D    :ref:`Curie Direct3D 9 SM 3 engine <obj-curie>`
0x4497 NV40:G80 TC  NV44_3D    :ref:`Curie Direct3D 9 SM 3 engine <obj-curie>`
0x5097 G80:G200     G80_3D     :ref:`Tesla Direct3D 10 engine <obj-tesla-3d>`
0x8297 G84:G200     G84_3D     :ref:`Tesla Direct3D 10 engine <obj-tesla-3d>`
0x8397 G200:GT215   G200_3D    :ref:`Tesla Direct3D 10 engine <obj-tesla-3d>`
0x8597 GT215:MCP89  GT215_3D   :ref:`Tesla Direct3D 10.1 engine <obj-tesla-3d>`
0x8697 MCP89:GF100  MCP89_3D   :ref:`Tesla Direct3D 10.1 engine <obj-tesla-3d>`
0x9097 GF100:GK104  GF100_3D   :ref:`Fermi Direct3D 11 engine <obj-fermi-3d>`
0x9197 GF108:GK104  GF108_3D   :ref:`Fermi Direct3D 11 engine <obj-fermi-3d>`
0x9297 GF110:GK104  GF110_3D   :ref:`Fermi Direct3D 11 engine <obj-fermi-3d>`
0xa097 GK104:GK110  GK104_3D   :ref:`Kepler Direct3D 11.1 engine <obj-kepler-3d>`
0xa197 GK110:GK20A  GK110_3D   :ref:`Kepler Direct3D 11.1 engine <obj-kepler-3d>`
0xa297 GK20A:GM107  GK20A_3D   :ref:`Kepler Direct3D 11.1 engine <obj-kepler-3d>`
0xb097 GM107-       GM107_3D   :ref:`Maxwell Direct3D 12 engine <obj-maxwell-3d>`
====== ============ ========== ============

And the compute objects:

====== =========== ============= ============
class  variants    name          description
====== =========== ============= ============
0x50c0 G80:GF100   G80_COMPUTE   :ref:`CUDA 1.x engine <obj-tesla-compute>`
0x85c0 GT215:GF100 GT215_COMPUTE :ref:`CUDA 1.x engine <obj-tesla-compute>`
0x90c0 GF100:GK104 GF100_COMPUTE :ref:`CUDA 2.x engine <obj-fermi-compute>`
0x91c0 GF110:GK104 GF110_COMPUTE :ref:`CUDA 2.x engine <obj-fermi-compute>`
0xa0c0 GK104:GK110 GK104_COMPUTE :ref:`CUDA 3.x engine <obj-kepler-compute>`
       GK20A:GM107
0xa1c0 GK110:GK20A GK110_COMPUTE :ref:`CUDA 3.x engine <obj-kepler-compute>`
0xb0c0 GM107:GM200 GM107_COMPUTE :ref:`CUDA 4.x engine <obj-maxwell-compute>`
0xb1c0 GM200:-     GM200_COMPUTE :ref:`CUDA 4.x engine <obj-maxwell-compute>`
====== =========== ============= ============


.. _nv1-null:

The NULL object
===============

.. todo:: write me


The graphics context
====================

.. todo:: write something here


Channel context
---------------

The following information makes up non-volatile graphics context. This state
is per-channel and thus will apply to all objects on it, unless software does
trap-swap-restart trickery with object switches. It is guaranteed to be
unaffected by subchannel switches and object binds. Some of this state can be
set by submitting methods on the context objects, some can only be set by
accessing PGRAPH context registers.

- the beta factor - set by BETA object
- the 8-bit raster operation - set by ROP object
- the A1R10G10B10 color for chroma key - set by CHROMA object
- the A1R10G10B10 color for plane mask - set by PLANE object
- the user clip rectangle - set by CLIP object:

  - ???

- the pattern state - set by PATTERN object:

  - shape: 8x8, 64x1, or 1x64
  - 2x A8R10G10B10 pattern color
  - the 64-bit pattern itself

- the NOTIFY DMA object - pointer to DMA object used by NOTIFY methods.
  NV1 only - moved to graph object options on NV3+. Set by direct PGRAPH
  access only.
- the main DMA object - pointer to DMA object used by IFM and ITM objects.
  NV1 only - moved to graph object options on NV3+. Set by direct PGRAPH
  access only.
- On NV1, framebuffer setup - set by direct PGRAPH access only:

  - ???

- On NV3+, rendering surface setup:

  - ???

  There are 4 copies of this state, one for each surface used by PGRAPH:

  - DST - the 2d destination surface
  - SRC - the 2d source surface [used by BLIT object only]
  - COLOR - the 3d color surface
  - ZETA - the 3d depth surface

  Note that the M2MF source/destination, ITM destination, IFM/SIFM source,
  and D3D texture don't count as surfaces - even though they may be
  configured to access the same data as surfaces on NV3+, they're accessed
  through the DMA circuitry, not the surface circuitry, and their setup
  is part of volatile state.


.. todo:: beta factor size

.. todo:: user clip state

.. todo:: NV1 framebuffer setup

.. todo:: NV3 surface setup

.. todo:: figure out the extra clip stuff, etc.

.. todo:: update for NV4+


Graph object options
--------------------

In addition to the per-channel state, there is also per-object non-volatile
state, called graph object options. This state is stored in the RAMHT entry
for the object [NV1], or in a RAMIN structure [NV3-]. On subchannel switches
and object binds, the PFIFO will send this state [NV1] or the pointer to this
state [NV3-] to PGRAPH via method 0. On NV1:NV4, this state cannot be
modified by any object methods and requires RAMHT/RAMIN access to change.
On NV4+, PGRAPH can bind DMA objects on its own when requested via methods,
and update the DMA object pointers in RAMIN. On NV5+, PGRAPH can modify
most of this state when requested via methods. All NV4+ automatic options
modification methods can be disabled by software, if so desired.

The graph options contain the following information:

 - :ref:`2d pipeline configuration <graph-2d-pipe-config>`
 - :ref:`2d color and mono format <graph-2d-format-config>`
 - NOTIFY_VALID flag - if set, NOTIFY method will be enabled. If unset, NOTIFY
   method will cause an interrupt. Can be used by the driver to emulate
   per-object DMA_NOTIFY setting - this flag will be set on objects whose
   emulated DMA_NOTIFY value matches the one currently in PGRAPH context,
   and interrupt will cause a switch of the PGRAPH context value followed
   by a method restart.
 - SUBCONTEXT_ID - a single-bit flag that can be used to emulate more than
   one PGRAPH context on one channel. When an object is bound and its
   SUBCONTEXT_ID doesn't match PGRAPH's current SUBCONTEXT_ID, a context
   switch interrupt is raised to allow software to load an alternate context.

.. todo:: NV3+

See :ref:`nv1-pgraph` for detailed format.


Volatile state
--------------

In addition to the non-volatile state described above, PGRAPH also has plenty
of "volatile" state. This state deals with the currently requested operation
and may be destroyed by switching to a new subchannel or binding a new object
[though not by full channel switches - the channels are supposed to be
independent after all, and kernel driver is supposed to save/restore all
state, including volatile state].

Volatile state is highly object-specific, but common stuff is listed here:

 - the "notifier write pending" flag and requested notification type

.. todo:: more stuff?


Notifiers
=========

The notifiers are 16-byte memory structures accessed via DMA objects, used
for synchronization. Notifiers are written by PGRAPH when certain operations
are completed. Software can poll on the memory structure, waiting for it
to be written by PGRAPH. The notifier structure is:

base+0x0:
    64-bit timestamp - written by PGRAPH with current PTIMER time as of
    the notifier write. The timestamp is a concatenation of current
    values of :ref:`TIME_LOW and TIME_HIGH registers <ptimer-time>`
    When big-endian mode is in effect, this becomes a 64-bit
    big-endian number as expected.
base+0x8:
    32-bit word always set to 0 by PGRAPH. This field may be used by
    software to put a non-0 value for software-written error-caused
    notifications.
base+0xc:
    32-bit word always set to 0 by PGRAPH. This is used for
    synchronization - the software is supposed to set this field to
    a non-0 value before submitting the notifier write request,
    then wait for it to become 0. Since the notifier fields are written
    in order, it is guaranteed that the whole notifier structure has
    been written by the time this field is set to 0.

.. todo:: verify big endian on non-G80

There are two types of notifiers: ordinary notifiers [NV1-] and M2MF notifiers
[NV3-]. Normal notifiers are written when explicitely requested by the NOTIFY
method, M2MF notifiers are written on M2MF transfer completion. M2MF notifiers
cannot be turned off, thus it's required to at least set up a notifier DMA
object if M2MF is used, even if the software doesn't wish to use notifiers
for synchronization.

.. todo:: figure out NV20 mysterious warning notifiers

.. todo:: describe GF100+ notifiers

The notifiers are always written to the currently bound notifier DMA object.
The M2MF notifiers share the DMA object with ordinary notifiers. The layout
of the DMA object used for notifiers is fixed:

- 0x00: ordinary notifier #0
- 0x10: M2MF notifier [NV3-]
- 0x20: ordinary notifier #2 [NV3:NV4 only]
- 0x30: ordinary notifier #3 [NV3:NV4 only]
- 0x40: ordinary notifier #4 [NV3:NV4 only]
- 0x50: ordinary notifier #5 [NV3:NV4 only]
- 0x60: ordinary notifier #6 [NV3:NV4 only]
- 0x70: ordinary notifier #7 [NV3:NV4 only]
- 0x80: ordinary notifier #8 [NV3:NV4 only]
- 0x90: ordinary notifier #9 [NV3:NV4 only]
- 0xa0: ordinary notifier #10 [NV3:NV4 only]
- 0xb0: ordinary notifier #11 [NV3:NV4 only]
- 0xc0: ordinary notifier #12 [NV3:NV4 only]
- 0xd0: ordinary notifier #13 [NV3:NV4 only]
- 0xe0: ordinary notifier #14 [NV3:NV4 only]
- 0xf0: ordinary notifier #15 [NV3:NV4 only]

.. todo:: 0x20 - NV20 warning notifier?

Note that the notifiers always have to reside at the very beginning of the DMA
object. On NV1 and NV4+, this effectively means that only 1 notifier of each
type can be used per DMA object, requiring mulitple DMA objects if more than
one notifier per type is to be used, and likely requiring a dedicated DMA
object for the notifiers. On NV3:NV4, up to 15 ordinary notifiers may be used
in a single DMA object, though that DMA object likely still needs to be
dedicated for notifiers, and only one of the notifiers supports interrupt
generation.


NOTIFY method
-------------

Ordinary notifiers are requested via the NOTIFY method. Note that the NOTIFY
method schedules a notifier write on completion of the method *following* the
NOTIFY - NOTIFY merely sets "a notifier write is pending" state.

It is an error if a NOTIFY method is followed by another NOTIFY method,
a DMA_NOTIFY method, an object bind, or a subchannel switch.

In addition to a notifier write, the NOTIFY method may also request a NOTIFY
interrupt to be triggered on PGRAPH after the notifier write.

mthd 0x104: NOTIFY [all NV1:GF100 graph objects]
  Requests a notifier write and maybe an interrupt. The write/interrupt will
  be actually performed after the *next* method completes. Possible parameter
  values are:
    0: WRITE - write ordinary notifier #0
    1: WRITE_AND_AWAKEN - write ordinary notifier 0, then trigger NOTIFY
       interrupt [NV3-]
    2: WRITE_2 - write ordinary notifier #2 [NV3:NV4]
    3: WRITE_3 - write ordinary notifier #3 [NV3:NV4]
    [...]
    15: WRITE_15 - write ordinary notifier #15 [NV3:NV4]
Operation::
    if (!cur_grobj.NOTIFY_VALID) {
        /* DMA notify object not set, or needs to be swapped in by sw */
        throw(INVALID_NOTIFY);
    } else if ((param > 0 && gpu == NV1)
            || (param > 15 && gpu >= NV3 && gpu < NV4)
            || (param > 1 && gpu >= NV4)) {
        /* XXX: what state is changed? */
        throw(INVALID_VALUE);
    } else if (NOTIFY_PENDING) {
        /* tried to do two NOTIFY methods in row */
        /* XXX: what state is changed? */
        throw(DOUBLE_NOTIFY);
    } else {
        NOTIFY_PENDING = 1;
        NOTIFY_TYPE = param;
    }

After every method other than NOTIFY and DMA_NOTIFY, the following is done::

    if (NOTIFY_PENDING) {
        int idx = NOTIFY_TYPE;
        if (idx == 1)
            idx = 0;
        dma_write64(NOTIFY_DMA, idx*0x10+0x0, PTIMER.TIME_HIGH << 32 | PTIMER.TIME_LOW);
        dma_write32(NOTIFY_DMA, idx*0x10+0x8, 0);
        dma_write32(NOTIFY_DMA, idx*0x10+0xc, 0);
        if (NOTIFY_TYPE == 1)
            irq_trigger(NOTIFY);
        NOTIFY_PENDING = 0;
    }

if a subchannel switch or object bind is done while NOTIFY_PENDING is set,
CTXSW_NOTIFY error is raised.

NOTE: NV1 has a 1-bit NOTIFY_PENDING field, allowing it to do notifier writes
with interrupts, but lacks support for setting it via the NOTIFY method. This
functionality thus has to be emulated by the driver if needed.


DMA_NOTIFY method
-----------------

On NV4+, the notifier DMA object can be bound by submitting the DMA_NOTIFY
method. This functionality can be disabled by the driver in PGRAPH settings
registers if not desired.

mthd 0x180: DMA_NOTIFY [all NV4:GF100 graph objects]
  Sets the notifier DMA object. When submitted through PFIFO, this method
  will undergo handle -> address translation via RAMHT.
Operation::
    if (DMA_METHODS_ENABLE) {
        /* XXX: list the validation checks */
        NOTIFY_DMA = param;
    } else {
        throw(INVALID_METHOD);
    }


NOP method
----------

On NV4+ a NOP method was added to enable asking for a notifier write without
having to submit an actual method to the object. The NOP method does nothing,
but still counts as a graph object method and will thus trigger a notifier
write/interrupt if one was previously requested.

mthd 0x100: NOP [all NV4+ graph objects]
  Does nothing.
Operation::
    /* nothing */

.. todo:: figure out if this method can be disabled for NV1 compat
