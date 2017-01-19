.. _nv1-pgraph-rop:

==============================
NV1 ROP - per-pixel operations
==============================

.. contents::


Introduction
============

Once the rasterizer determines what pixels are to be drawn, it is ROP's task
to actually perform the drawing - that is, read current pixel data from memory
(if needed), perform per-pixel operations, then write the result to memory,
or discard it.

The per-pixel operations are as follows:

1. Determine the working color format.  This is based on source color format,
   framebuffer format, and some configuration bits.
2. Get pixel coordinates from raster.  Both X and Y are 12-bit unsigned
   integers.  If BLIT is performed, also get source X and Y.
3. Compare the coordinates against the :ref:`cliprects <nv1-cliprect>`
   and the clipping rectangle from XY logic.  This may result in discarding
   the pixel.
4. Get the source color:

   - for solids, just take it from (the low bits of) :obj:`SRC_COLOR
     <nv1-pgraph-src-color>`.
   - for IFC, IFM, and textured quads, take it from :obj:`SRC_COLOR
     <nv1-pgraph-src-color>`, selecting the right pixel for < 32bpp input.
   - for BITMAP, take the right bit of :obj:`SRC_COLOR <nv1-pgraph-src-color>`,
     then perform bitmap expansion.
   - for BLIT, read the source pixel from the framebuffer.  If the source
     pixel is outside the clipping rectangle from XY logic, or rejected
     by the cliprects, discard the current pixel.

5. If the source alpha component is 0, discard the pixel.
6. Upconvert the source color to working color format, if necessary.
7. If the operation selected by the current object requires that, read
   the current value of the destination pixel, and (if needed) upconvert it
   to the working color format.
8. If the operation selected by the current object requires it, compute
   the pattern color at the destination coordinates, and (if needed)
   downconvert it to the working color format.
9. If the operation selected by the current object is ``BLEND_*``, calculate
   the blend factor, then perform the blending.
10. If the operation is not ``BLEND_*``:

    1. If color key is enabled on current object: downconvert the color key
       to the working color format (if necessary), compare against the source
       color, discard the pixel if they are equal.
    2. If the operation is not ``SRCCOPY``: perform the bitwise operation.
    3. If plane masking is enabled on current object: downconvert the plane
       mask to the working color format (if necessary), merge the color computed
       so far with the current destination color using the plane mask.

11. If necessary, downconvert the color from the working format to framebuffer
    format, possibly with dithering.
12. Write the final color to the framebuffer.

.. todo:: figure out selecting the right part of SRC_COLOR for IFC/IFM/BITMAP

.. todo:: BLIT and source pixel discards

.. todo:: pseudocode, please


The framebuffer(s)
==================

On NV1, handling framebuffer addressing is PFB's job - see :ref:`nv1-fb`.
PFB exposes 1 or 2 buffers to PGRAPH and handles converting the X, Y coords
to VRAM addresses.  Both X and Y coordinates are 12-bit unsigned integers
once they reach ROP stage.

The pixel size is selected by PFB and exposed to PGRAPH.  It can be:

- 8 bpp: each pixel is a single byte, in Y8 format (single component, color
  index).
- 16 bpp: each pixel is a 16-bit little-endian word.  Depending on
  configuration, it can be in one of two formats:

  - indexed (D1X7Y8):

    - bits 0-7: color index
    - bits 8-14: unused, written as 0
    - bit 15: CLUT bypass - whenever a pixel is written, this will be set
      to the current value of :obj:`CANVAS_CONFIG.CLUT_BYPASS
      <nv1-pgraph-canvas-config>`.  In turn, PDAC will use it to select
      pixel mode.

  - direct (D1R5G5B5):

    - bits 0-4: blue component
    - bits 5-9: green component
    - bits 10-14: red component
    - bit 15: CLUT bypass (see above)

- 32 bpp: each pixel is a 32-bit little-endian word.  Depending on
  configuration, it can be in one of two formats:

  - indexed (D1X23Y8):

    - bits 0-7: color index
    - bits 8-30: unused, written as 0
    - bit 31: CLUT bypass - whenever a pixel is written, this will be set
      to the current value of :obj:`CANVAS_CONFIG.CLUT_BYPASS
      <nv1-pgraph-canvas-config>`.  In turn, PDAC will use it to select
      pixel mode.

  - direct (D1X1R10G10B10):

    - bits 0-9: blue component
    - bits 10-19: green component
    - bits 20-29: red component
    - bit 30: unused, written as 0
    - bit 31: CLUT bypass (see above)

Indexed vs direct color is chosen as follows::

    def is_indexed():
        if PFB.CONFIG.BPP <= 1:
            # If framebuffer is 8bpp, always indexed.
            return True
        if ACCESS.CLASS == BLIT:
            # If doing blit, treat pixels as direct color.
            return False
        if CTX_SWITCH.COLOR_FORMAT_DST.COLOR_FORMAT != Y8_A8Y8:
            # Also, treat as direct color if source color format is anything
            # other than Y8.
            return False
        if CANVAS_CONFIG.Y8_EXPAND:
            # If Y8 expansion is performed, treat as direct color.
            return False
        # Otherwise (not a blit, Y8 source format, and no Y8 expansion),
        # treat as indexed.
        return True


Canvas configuration
====================

There is a register that controls assorted aspects of per-pixel operations:

.. reg:: 32 nv1-pgraph-canvas-config Canvas configuration

   - bit 0: CLUT_BYPASS - for 16bpp and 32bpp framebuffer formats, the value
     of this bit will be copied to the highest bit of the written pixels, ie.
     the "CLUT bypass" bit.
   - bit 4: BUF1_IGNORE_CLIPRECT - if set, cliprects will only affect buffer 0
     in dual-buffer configuration - they will be ignored when writing to
     buffer 1.  If not set, cliprects will apply to both buffers.
   - bit 12: Y8_EXPAND - controls color format in use when source format is Y8
     and framebuffer is 16bpp or 32bpp.  If set, Y8 will be expanded to R5G5B5
     or R10G10B10, by broadcasting the single value into all 3 color
     components.  Otherwise, it will remain as Y8, and written thus to the
     framebuffer.
   - bit 16: DITHER - controls color downconversion to R5G5B5 format when
     writing to the framebuffer.  If set, colors will be dithered.  Otherwise,
     a simple truncation will be used.
   - bit 20: REPLICATE - controls color upconversion from source format to
     R10G10B10.  If set, R5G5B5 source components will be multiplied by 0x21
     to get R10G10B10 components (effectively duplicating the 5-bit values
     to get 10-bit values), and R8G8B8/Y8 source components will be multiplied
     by 0x101 and shifted right by 6 bits (effectively duplicating the high
     2 bits as extra 2 low bits).  If not set, components will be converted
     by a simple shift left.
   - bit 24: SOFTWARE - if set, the desired framebuffer configuration is
     considered too complex for NV1's little mind, and all drawing operations
     will trigger CANVAS_SOFTWARE interrupts instead of performing their
     usual function.

This register cannot be changed by any class method, and must be modified
manually by software, if so desired.


.. _nv1-cliprect:

Cliprects
=========

NV1 supports, as part of per-pixel operations, discarding pixels based on
their relation with up to two clipping rectangles.  This is distinct from
the clipping to canvas and user clip rectangle done by the XY logic, and
also less efficient (since the pixels will be produced by the rasterizer
and then discarded).

Cliprect state cannot be changed by any class method, and must be modified
manually by software, if so desired.  The registers involved are:

.. reg:: 32 nv1-pgraph-cliprect-config Cliprect configuration

   - bits 0-1: COUNT - selects how many cliprects are enabled.  Valid
     values are 0-2.  If this is 0, cliprects are disabled, and will
     pass all pixels.
   - bit 4: MODE - selects which pixels will be rendered, if COUNT is not 0:

     - 0: INCLUDED - pixels that are covered by at least one of the cliprects
       will be rendered, pixels not covered will be discarded.
     - 1: OCCLUDED - pixels that are not covered by any cliprect will be
       rendered, pixels covered by at least one cliprect will be discarded.

   - bit 8: SOFTWARE - if set, the desired cliprects are too complex for NV1's
     little mind, and all drawing operations will trigger CLIP_SOFTWARE
     interrupts instead of performing their usual function.

.. reg:: 32 nv1-pgraph-cliprect-min Cliprect upper-left corner

   - bits 0-11: X - the X coordinate of the left edge of the cliprect
   - bits 16-27: Y - the Y coordinate of the top edge of the cliprect

.. reg:: 32 nv1-pgraph-cliprect-max Cliprect lower-right corner

   Since rectangles on NV1 are represented in right-exclusive fashion, these
   coordinates are actually 1 pixel to the right and 1 pixel down from the
   actual corner of the clipping rectangle.

   - bits 0-11: X - the X coordinate of the right edge of the cliprect plus 1
   - bits 16-27: Y - the Y coordinate of the bottom edge of the cliprect plus 1

If dual-buffer configuration is enabled in PFB, a bit in :obj:`CANVAS_CONFIG
<nv1-pgraph-canvas-config>` selects whether cliprects apply to both buffers,
or just to buffer 0.

The exact operation performed is::

    def cliprect_covered(i, x, y) -> bool:
        if x < CLIPRECT_MIN[i].X:
            return False
        if y < CLIPRECT_MIN[i].Y:
            return False
        if x >= CLIPRECT_MAX[i].X:
            return False
        if y >= CLIPRECT_MAX[i].Y:
            return False
        return True

    def cliprect_pass(buf, x, y) -> bool:
        if buf == 1 and CANVAS_CONFIG.BUF1_IGNORE_CLIPRECT:
            return True
        if CLIPRECT_CONFIG.COUNT == 0:
            return True
        covered = cliprect_covered(0, x, y)
        # COUNT == 3 is treated as if it was 2.
        if CLIPRECT_CONFIG.COUNT >= 2:
            covered |= cliprect_covered(1, x, y)
        if CLIPRECT_CONFIG.MODE == INCLUDED:
            return covered
        else
            return not covered
        # Note: CLIPRECT_CONFIG.SOFTWARE is checked by XY logic
        # before rasterization even starts.


Bitmap expansion
================

.. todo:: write me

Bitmap colors
-------------

.. reg:: 32 nv1-pgraph-bitmap-color Bitmap color

   .. todo:: write me

.. reg:: 32 nv1-mthd-bitmap-color Set bitmap color

   .. todo:: write me


Color key
=========

.. reg:: 32 nv1-pgraph-chroma The color key

   .. todo:: write me

.. reg:: 32 nv1-mthd-chroma Set the color key

   .. todo:: write me


Pattern
=======

.. todo:: write me

Pattern shape
-------------

.. reg:: 32 nv1-pgraph-pattern-shape Pattern shape

   .. todo:: write me

.. reg:: 32 nv1-mthd-pattern-shape Set pattern shape

   .. todo:: write me

Pattern bitmap
--------------

.. reg:: 32 nv1-pgraph-pattern-bitmap Pattern bitmap

   .. todo:: write me

.. reg:: 32 nv1-mthd-pattern-bitmap Set pattern bitmap

   .. todo:: write me

Pattern colors
--------------

.. reg:: 32 nv1-pgraph-pattern-bitmap-color Pattern bitmap color

   .. todo:: write me

.. reg:: 32 nv1-pgraph-pattern-bitmap-alpha Pattern bitmap alpha

   .. todo:: write me

.. reg:: 32 nv1-mthd-pattern-bitmap-color Set pattern bitmap color

   .. todo:: write me


Blending
========

.. todo:: write me


Beta factor
-----------

.. reg:: 32 nv1-pgraph-beta The beta blending factor

   .. todo:: write me

.. reg:: 32 nv1-mthd-beta Set the beta blending factor

   .. todo:: write me


Bitwise operations
==================

.. todo:: write me


ROP selection
-------------

.. reg:: 32 nv1-pgraph-rop The bitwise operation

   .. todo:: write me

.. reg:: 32 nv1-mthd-rop Set the bitwise operation

   .. todo:: write me



Plane mask
==========

.. reg:: 32 nv1-pgraph-plane The plane mask

   .. todo:: write me

.. reg:: 32 nv1-mthd-plane Set the plane mask

   .. todo:: write me
