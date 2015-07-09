===========================
Overview of the 2D pipeline
===========================

.. contents::


Introduction
============

On nvidia GPUs, 2d operations are done by PGRAPH engine [see graph/intro.txt].
The 2d engine is rather orthogonal and has the following features:

- various data sources:

  - solid color shapes (points, lines, triangles, rectangles)
  - pixels uploaded directly through command stream, raw or expanded using
    a palette
  - text with in-memory fonts [NV3:G80]
  - rectangles blitted from another area of video memory
  - pixels read by DMA
  - linearly and quadratically textured quads [NV1:NV3]

- color format conversions
- chroma key
- clipping rectangles
- per-pixel operations between source, destination, and pattern:

  - logic operations
  - alpha and beta blending
  - pre-multiplied alpha blending [NV4-]

- plane masking [NV1:NV4]
- dithering
- data output:

  - to the framebuffer [NV1:NV3]
  - to any surface in VRAM [NV3:G84]
  - to arbirary memory [G84-]


The objects
===========

The 2d engine is controlled by the user via PGRAPH objects. On NV1:G84, each
piece of 2d functionality has its own object class - a matching set of objects
needs to be used together to perform an operation. G80+ have a unified 2d
engine object that can be used to control all of the 2d pipeline in one place.

The non-unified objects can be divided into 3 classes:

- source objects: control the drawing operation, choose pixels to draw and
  their colors
- context objects: control various pipeline settings shared by other objects
- operation objects: connect source and context objects together

The source objects are:

- :ref:`POINT <obj-point>`, :ref:`LIN <obj-lin>`, :ref:`LINE <obj-line>`,
  :ref:`TRI <obj-tri>`, :ref:`RECT <obj-rect>`: drawing of solid color shapes
- :ref:`IFC <obj-ifc>`, :ref:`BITMAP <obj-bitmap>`, :ref:`SIFC <obj-sifc>`,
  :ref:`INDEX <obj-index>`, :ref:`TEXTURE <obj-texture>`: drawing of pixel data from CPU
- :ref:`BLIT <obj-blit>`: copying rectangles from another area of video memory
- :ref:`IFM <obj-ifm>`, :ref:`SIFM <obj-sifm>`: drawing pixel data from DMA
- :ref:`GDI <obj-gdi>`: Drawing solid rectangles and text fonts
- :ref:`TEXLIN <obj-texlin>`, :ref:`TEXQUAD <obj-texquad>`, :ref:`TEXLINBETA <obj-texlinbeta>`,
  :ref:`TEXQUADBETA <obj-texquadbeta>`: Drawing textured quads

The context objects are:

- :ref:`BETA <obj-beta>`: blend factor
- :ref:`ROP <obj-rop>`: logic operation
- :ref:`CHROMA <obj-chroma>`: color for chroma key
- :ref:`PLANE <obj-plane>`: color for plane mask
- :ref:`CLIP <obj-clip>`: clipping rectangle
- :ref:`PATTERN <obj-pattern>`: repeating pattern image [graph/pattern.txt]
- :ref:`BETA4 <obj-beta4>`: pre-multiplied blend factor
- :ref:`SURF <obj-surf>`, :ref:`SURF2D <obj-surf2d>`, :ref:`SWZSURF <obj-swzsurf>`:
  destination and blit source surface setup

The operation objects are:

- OP_CLIP: clipping operation
- OP_BLEND_AND: blending
- OP_ROP_AND: logic operation
- OP_CHROMA: color key	
- OP_SRCCOPY_AND: source copy with 0-alpha discard
- OP_SRCCOPY: source copy
- OP_SRCCOPY_PREMULT: pre-multiplying copy
- OP_BLEND_PREMULT: pre-multiplied blending

The unified 2d engine objects are described below.

The objects that, although related to 2d operations, aren't part of the usual
2d pipeline:

 - :ref:`ITM <obj-itm>`: downloading framebuffer data to DMA
 - :ref:`M2MF <obj-m2mf>`: DMA to DMA copies
 - :ref:`DVD_SUBPICTURE <obj-dvd>`: blending of YUV data

Note that, although multiple objects of a single kind may be created, there is
only one copy of pipeline state data in PGRAPH. There are thus two usage
possibilities:

- aliasing: all objects on a channel access common pipeline state, making it
  mostly useless to create several objects of single kind
- swapping: the kernel driver or some other piece of software handles PGRAPH
  interrupts, swapping pipeline configurations as they're needed, and marking
  objects valid/not valid according to currently loaded configuration


Connecting the objects - NV1 style
----------------------------------

The objects were originally intended and designed for connecting with
so-called patchcords. A patchcord is a dummy object that's conceptually
a wire carrying some sort of data. The patchcord types are:

- image patchcord: carries pixel color data
- beta patchcord: carries beta blend factor data
- zeta patchcord: carries pixel depth data
- rop patchcord: carries logic operation data

Each 2d object has patchcord "slots" representing its inputs and outputs.
A slot is represented by an object methods. Objects are connected together
by creating a patchcord of appropriate type and writing its handle to the
input slot method on one object and the output slot method on the other
object. For example:

- source objects have an output image patchcord slot [BLIT also has input
  image slot]
- BETA context object has an output beta slot
- OP_BLEND_AND has two image input slots, one beta input slot, and one
  image output slot

A valid set of objects, called a "patch" is constructed by connecting
patchcords appropriately. Not all possible connections ara valid, though.
Only ones that map to the actual hardware pipeline are allowed: one of
the source objects must be at the beginning, connected via image patchcord
to OP_BLEND_*, OP_ROP_AND, or OP_SRCCOPY_*, optionally connected further
through OP_CLIP and/or OP_CHROMA, then finally connected to a SURF object
representing the destination surface. Each of the OP_* objects and source
objects that needs it must also be connected to the appropriate extra
inputs, like the CLIP rectangle, PATTERN or another SURF, or CHROMA key.

No GPU has ever supported connecting patchcords in hardware - the software
must deal with all required processing and state swapping. However, NV4:NV20
hardware knows of the methods reserved for these purpose, and raises a special
interrupt when they're called. The OP_*, while lacking in any useful hardware
methods, are also supported on NV4:NV5.


Connecting the objects - NV5 style
----------------------------------

A new way of connecting objects was designed for NV5 [but can be used with
earlier cards via software emulation]. Instead of treating a patch as
a freeform set of objects, the patch is centered on the source object. While
context objects are still in use, operation objects are skipped - the set
of operations to perform is specified at the source object, instead of being
implid by the patchcord topology. The context objects are now connected
directly to the source object by writing their handles to appropriate source
object methods. The OP_CLIP and OP_CHROMA functionality is replaced by CLIP
and CHROMA methods on the source objects: enabling clipping/color keying
is done by connecting appropriate context object, while disabling is done
by connecting a NULL object. The remaining operation objects are replaced
by OPERATION method, which takes an enum selecting the operation to perform.

NV5 added support for the NV5-style connections in hardware - all methods
can be processed without software assistance as long as only one object of
each type is in use [or they're allowed to alias]. If swapping is required,
it's the responsibility of software. The new methods can be globally disabled
if NV1-style connections are desired, however. NV5-style connections can
also be implemented for older GPUs simply by handling the relevant methods
in software.


.. _graph-2d-format-config:

Color and monochrome formats
============================

.. todo:: write me


COLOR_FORMAT methods
--------------------

mthd 0x300: COLOR_FORMAT [NV1_CHROMA, NV1_PATTERN] [NV4-]
  Sets the color format using NV1 color enum.
Operation::

    cur_grobj.COLOR_FORMAT = get_nv1_color_format(param);

.. todo:: figure out this enum

mthd 0x300: COLOR_FORMAT [NV4_CHROMA, NV4_PATTERN]
  Sets the color format using NV4 color enum.
Operation::

    cur_grobj.COLOR_FORMAT = get_nv4_color_format(param);

.. todo:: figure out this enum


Color format conversions
------------------------

.. todo:: write me


Monochrome formats
------------------

.. todo:: write me

mthd 0x304: MONO_FORMAT [NV1_PATTERN] [NV4-]
  Sets the monochrome format.
Operation::

    if (param != LE && param != CGA6)
        throw(INVALID_ENUM);
    cur_grobj.MONO_FORMAT = param;

.. todo:: check


.. _graph-2d-pipe-config:

The pipeline
============

The 2d pipeline consists of the following stages, in order:

1. Image source: one of the source objects, or one of the three source types
   on the unified 2d objects [SOLID, SIFC, or BLIT] - see documentation
   of the relevant object
2. Clipping
3. Source color conversion
4. One of:

   1. Bitwise operation subpipeline, soncisting of:

     1. Optionally, an arbitrary bitwise operation done on the source,
            the destination, and the pattern.
     2. Optionally, a color key operation
     3. Optionally, a plane mask operation [NV1:NV4]

   2. Blending operation subpipeline, consisting of:

     1. Blend factor calculation
     2. Blending

5. Dithering
6. Destination write

In addition, the pipeline may be used in RGB mode [treating colors as made of
R, G, B components], or index mode [treating colors as 8-bit palette index].
The pipeline mode is determined automatically by the hardware based on source
and destination formats and some configuration bits.

The pixels are rendered to a destination buffer. On NV1:NV4, more than one
destination buffer may be enabled at a time. If this is the case, the pixel
operations are executed separately for each buffer.


Pipeline configuration: NV1
---------------------------

The pipeline configuration is stored in graph options and other PGRAPH
registers. It cannot be changed by user-visible commands other than via
rebinding objects. The following options are stored in the graph object:

- the operation, one of:

  - RPOP_DS - RPOP(DST, SRC)
  - ROP_SDD - ROP(SRC, DST, DST)
  - ROP_DSD - ROP(DST, SRC, DST)
  - ROP_SSD - ROP(SRC, SRC, DST)
  - ROP_DDS - ROP(DST, DST, SRC)
  - ROP_SDS - ROP(SRC, DST, SRC)
  - ROP_DSS - ROP(DST, SRC, SRC)
  - ROP_SSS - ROP(SRC, SRC, SRC)
  - ROP_SSS_ALT - ROP(SRC, SRC, SRC)
  - ROP_PSS - ROP(PAT, SRC, SRC)
  - ROP_SPS - ROP(SRC, PAT, SRC)
  - ROP_PPS - ROP(PAT, PAT, SRC)
  - ROP_SSP - ROP(SRC, SRC, PAT)
  - ROP_PSP - ROP(PAT, SRC, PAT)
  - ROP_SPP - ROP(SRC, PAT, PAT)
  - RPOP_SP - ROP(SRC, PAT)
  - ROP_DSP - ROP(DST, SRC, PAT)
  - ROP_SDP - ROP(SRC, DST, PAT)
  - ROP_DPS - ROP(DST, PAT, SRC)
  - ROP_PDS - ROP(PAT, DST, SRC)
  - ROP_SPD - ROP(SRC, PAT, DST)
  - ROP_PSD - ROP(PAT, SRC, DST)
  - SRCCOPY - SRC [no operation]
  - BLEND_DS_AA - BLEND(DST, SRC, SRC.ALPHA^2) [XXX check]
  - BLEND_DS_AB - BLEND(DST, SRC, SRC.ALPHA * BETA)
  - BLEND_DS_AIB - BLEND(DST, SRC, SRC.ALPHA * (1-BETA))
  - BLEND_PS_B - BLEND(PAT, SRC, BETA)
  - BLEND_PS_IB - BLEND(SRC, PAT, (1-BETA))

  If the operation is set to one of the BLEND_* values, blending subpipeline
  will be active. Otherwise, the bitwise operation subpipeline will be active.
  For bitwise operation pipeline, RPOP* and ROP* will cause the bitwise
  operation stage to be enabled with the appropriate options, while the
  SRCCOPY setting will cause it to be disabled and bypassed.

- chroma enable: if this is set to 1, and the bitwise operation subpipeline
  is active, the color key stage will be enabled
- plane mask enable: if this is set to 1, and the bitwise operation
  subpipeline is active, the plane mask stage will be enabled
- user clip enable: if set to 1, the user clip rectangle will be enabled in
  the clipping stage
- destination buffer mask: selects which destination buffers will be written

The following options are stored in other PGRAPH registers:

- palette bypass bit: determines the value of the palette bypass bit written
  to the framebuffer
- Y8 expand: determines piepline mode used with Y8 source and non-Y8
  destination - if set, Y8 is upconverted to RGB and the RGB mode is used,
  otherwise the index mode is used
- dither enable: if set, and several conditions are fullfilled, dithering
  stage will be enabled
- software mode: if set, all drawing operations will trap without touching
  the framebuffer, allowing software to perform the operation instead

The pipeline mode is selected as follows:

- if blending subpipeline is used, RGB mode is selected [index blending is
  not supported]
- if bitwise operation subpipeline is used:

  - if destination format is Y8, indexed mode is selected
  - if destination format is D1R5G5B5 or D1X1R10G10B10:

    - if source format is not Y8 or Y8 expand is enabled, RGB mode is selected
    - if source format is Y8 and Y8 expand is not enabled, indexed mode is
      selected

In RGB mode, the pipeline internally uses 10-bit components. In index mode,
8-bit indices are used.

See :ref:`nv1-pgraph` for more information on the configuration registers.


Clipping
--------

.. todo:: write me


Source format conversion
------------------------

Firstly, the source color is converted from its original format to the format
used for operations.

.. todo:: figure out what happens on ITM, IFM, BLIT, TEX*BETA

On NV1, all operations are done on A8R10G10B10 or I8 format internally. In
RGB mode, colors are converted using the standard color expansion formula.
In index mode, the index is taken from the low 8 bits of the color.

::

    src.B = get_color_b10(cur_grobj, color);
    src.G = get_color_g10(cur_grobj, color);
    src.R = get_color_r10(cur_grobj, color);
    src.A = get_color_a8(cur_grobj, color);
    src.I = color[0:7];

In addition, pixels are discarded [all processing is aborted and the
destination buffer is left untouched] if the alpha component is 0 [even in
index mode].

::

    if (!src.A)
        discard;

.. todo:: NV3+


Buffer read
-----------

In some blending and bitwise operation modes, the current contents of the
destination buffer at the drawn pixel location may be used as an input to
the 2d pipeline.

.. todo:: document that and BLIT


Bitwise operation
-----------------

.. todo:: write me


Chroma key
----------

.. todo:: write me


The plane mask
--------------

.. todo:: write me


Blending
--------

.. todo:: write me


Dithering
---------

.. todo:: write me


The framebuffer
---------------

.. todo:: write me


NV1 canvas
~~~~~~~~~~

.. todo:: write me


NV3 surfaces
~~~~~~~~~~~~

.. todo:: write me

Clip rectangles
~~~~~~~~~~~~~~~

.. todo:: write me


.. _obj-op:

NV1-style operation objects
===========================

.. todo:: write me


.. _obj-2d:

Unified 2d objects
==================

.. todo:: write me

0100   NOP					[graph/intro.txt]
0104   NOTIFY [G80_2D]				[graph/intro.txt]
[XXX: GF100 methods]
0110   WAIT_FOR_IDLE				[graph/intro.txt]
0140   PM_TRIGGER				[graph/intro.txt]
0180   DMA_NOTIFY [G80_2D]			[graph/intro.txt]
0184   DMA_SRC [G80_2D]			[XXX]
0188   DMA_DST [G80_2D]			[XXX]
018c   DMA_COND [G80_2D]			[XXX]
[XXX: 0200-02ac]
02b0   PATTERN_OFFSET				[graph/pattern.txt]
02b4   PATTERN_SELECT				[graph/pattern.txt]
02dc   ??? [GF100_2D-]				[XXX]
02e0   ??? [GF100_2D-]				[XXX]
02e8   PATTERN_COLOR_FORMAT			[graph/pattern.txt]
02ec   PATTERN_BITMAP_FORMAT			[graph/pattern.txt]
02f0+i*4, i<2   PATTERN_BITMAP_COLOR		[graph/pattern.txt]
02f8+i*4, i<2   PATTERN_BITMAP			[graph/pattern.txt]
0300+i*4, i<64  PATTERN_X8R8G8B8		[graph/pattern.txt]
0400+i*4, i<32  PATTERN_R5G6B5			[graph/pattern.txt]
0480+i*4, i<32  PATTERN_X1R5G5B5		[graph/pattern.txt]
0500+i*4, i<16  PATTERN_Y8			[graph/pattern.txt]
[XXX: 0540-08dc]
08e0+i*4, i<32  FIRMWARE			[graph/intro.txt]
[XXX: GF100 methods]
