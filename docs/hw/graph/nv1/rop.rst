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

1. Determine the :ref:`working color format <nv1-rop-work-color>`.  This is
   based on source color format, framebuffer format, and some configuration
   bits - it does not depend on the individual pixel.
2. If double buffer mode is active, determine which buffer(s) the pixel should
   be written to, if any.  This is selected by the current object.
3. Get pixel coordinates from raster.  Both X and Y are 12-bit unsigned
   integers.  If BLIT is performed, also get source X and Y.
4. Compare the coordinates against the :ref:`cliprects <nv1-cliprect>`
   and the clipping rectangle from XY logic.  This may result in discarding
   the pixel, or writing it to only one of two buffers.
5. Get the source color:

   - for solids, just take it from (the low bits of) :obj:`SRC_COLOR
     <nv1-pgraph-src-color>`.
   - for IFC, IFM, and textured quads, take it from :obj:`SRC_COLOR
     <nv1-pgraph-src-color>`, selecting the right pixel for < 32bpp input.
   - for BITMAP, take the right bit of :obj:`SRC_COLOR <nv1-pgraph-src-color>`,
     then perform bitmap expansion.
   - for BLIT, read the source pixel from the framebuffer.  If the source
     pixel is outside the clipping rectangle from XY logic, or rejected
     by the cliprects, discard the current pixel.

6. If alpha is enabled, extract the source alpha component according to the
   source color format.  Otherwise, source alpha is assumed to be ``0xff``.
7. If the source alpha component is 0, discard the pixel.
8. Upconvert the source color to working color format, if necessary.
9. If the operation selected by the current object requires that, read
   the current value of the destination pixel, and (if needed) upconvert it
   to the working color format.
10. If the operation selected by the current object requires it, compute
    the pattern color at the destination coordinates, and (if needed)
    downconvert it to the working color format.
11. If the operation selected by the current object is ``BLEND_*``, calculate
    the blend factor, then perform the blending.
12. If the operation is not ``BLEND_*``:

    1. If :ref:`color key <nv1-rop-chroma>` is enabled on current object:
       downconvert the color key to the working color format (if necessary),
       compare against the source color, discard the pixel if they are equal.
    2. If the operation is not ``SRCCOPY``: perform the bitwise operation.
    3. If plane masking is enabled on current object: downconvert the plane
       mask to the working color format (if necessary), merge the color computed
       so far with the current destination color using the plane mask.

13. If necessary, downconvert the color from the working format to framebuffer
    format, possibly with dithering.
14. Write the final color to the framebuffer(s).

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
        if CTX_SWITCH.OP in BLEND_*:
            # Always direct if blending is involved.
            return False
        if PFB.CONFIG.BPP <= 1:
            # If framebuffer is 8bpp, always indexed.
            return True
        if ACCESS.CLASS == BLIT:
            # If doing blit, treat pixels as direct color.
            return False
        if CTX_SWITCH.COLOR_FORMAT_DST.COLOR_FORMAT != A8Y8:
            # Also, treat as direct color if source color format is anything
            # other than Y8.
            return False
        if CANVAS_CONFIG.Y8_EXPAND:
            # If Y8 expansion is performed, treat as direct color.
            return False
        # Otherwise (not a blit, Y8 source format, and no Y8 expansion),
        # treat as indexed.
        return True

.. todo:: weird shit happens if blending is enabled and framebuffer is 8bpp.

If single buffer mode is selected on PFB, rendered pixels will always be
written to buffer 0, unless they are discarded to some reason.  If double
buffer mode is selected, the ``COLOR_FORMAT_DST`` field of the :ref:`current
object <nv1-pgraph-object>` determines which buffer(s) are written to -
``BUF0_*`` will write to buffer 0, ``BUF1_*`` to buffer 1, ``BUF01_*``
will write to both buffers, and ``BUF_NONE_*`` will discard all pixels.

.. note:: If both buffers are enabled, each pixel will be written independently
   to both of them - if the selected operation involves the current value of
   destination pixel (for blending or bitwise operation), they may get written
   with different final colors.


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


Color formats
=============

.. _nv1-rop-work-color:

Working format selection
------------------------

Working format can be ``Y8``, ``R5G5B5``, or ``R10G10B10``. It is selected
as follows::

    def working_format():
        if is_indexed():
            # If framebuffer is indexed, always work on Y8.
            return Y8
        if CTX_SWITCH.OP in BLEND_*:
            # Always R10G10B10 if blending is involved.
            return R10G10B10
        if PFB.CONFIG.BPP == 2 and CTX_SWITCH.COLOR_FORMAT_DST.COLOR_FORMAT == A1R5G5B5:
            # Both formats are R5G5B5, so let's use that.
            return R5G5B5
        # All other cases use R10G10B10.
        return R10G10B10


.. _nv1-rop-format-upconvert:

Color format upconversion and extracting alpha
----------------------------------------------

Color format upconversion is performed on the incoming source pixel data
(if needed), on the current destination pixel data (if needed), and on
colors submitted as parameters to some ROP state-setting methods.  If such
conversion is needed at all, it's always done to A8R10G10B10 format.

Color upconversion is affected by the :obj:`CANVAS_CONFIG.REPLICATE
<nv1-pgraph-canvas-config>` bit: if it's set, color components are multiplied
by the correct factors to cover the ``0-0x3ff`` range uniformly.  Otherwise,
they are simply shifted left.

For ``Y8`` and ``Y16`` formats, the singular component is simply broadcast
to all three components, resulting in grayscale.  Since the destination
format has only 10 bits per component, the low 6 bits of ``Y16`` are simply
discarded.

The exact operation is::

    def upconvert_src(val):
        if CTX_SWITCH.COLOR_FORMAT_DST.COLOR_FORMAT == A1R5G5B5:
            b = val & 0x1f
            g = val >> 5 & 0x1f
            r = val >> 10 & 0x1f
            a = val >> 15 & 1
            if CANVAS_CONFIG.REPLICATE:
                # R, G, B are 5 bits - duplicate to get 10 bits.
                b *= 0x21
                g *= 0x21
                r *= 0x21
            else:
                b <<= 5
                g <<= 5
                r <<= 5
            # A is always either 0 or 0xff.
            a *= 0xff
        elif CTX_SWITCH.COLOR_FORMAT_DST.COLOR_FORMAT == A8R8G8B8:
            b = val & 0xff
            g = val >> 8 & 0xff
            r = val >> 16 & 0xff
            a = val >> 24 & 0xff
            if CANVAS_CONFIG.REPLICATE:
                # R, G, B are 8-bit: duplicate to get 16 bits, then truncate
                # to 10.
                b = (b * 0x101) >> 6
                g = (g * 0x101) >> 6
                r = (r * 0x101) >> 6
            else:
                b <<= 2
                g <<= 2
                r <<= 2
            # A is already 8-bit.
        elif CTX_SWITCH.COLOR_FORMAT_DST.COLOR_FORMAT == A2R10G10B10:
            b = val & 0x3ff
            g = val >> 10 & 0x3ff
            r = val >> 20 & 0x3ff
            a = val >> 30 & 3
            # R, G, B are already 10-bit: nothing to do.
            # A is 2-bit - repeat 4 times to get 8 bits (this is not affected
            # by REPLICATE!).
            a *= 0x55
        elif CTX_SWITCH.COLOR_FORMAT_DST.COLOR_FORMAT == A8Y8:
            y = val & 0xff
            a = val >> 8 & 0xff
            if CANVAS_CONFIG.REPLICATE:
                # Y is 8-bit: duplicate to get 16 bits, then truncate to 10.
                y = (y * 0x101) >> 6
            else:
                y <<= 2
            # Broadcast it.
            r = g = b = y
            # A is already 8-bit.
        elif CTX_SWITCH.COLOR_FORMAT_DST.COLOR_FORMAT == A16Y16:
            y = val & 0xffff
            a = val >> 16 & 0xffff
            # Truncate 16 to 10 and broadcast.
            r = g = b = y >> 6
            # Truncate 16 to 8.
            a >>= 8
        if not CTX_SWITCH.ALPHA:
            # Whatever we determined for alpha, it's invalid if not enabled.
            a = 0xff
        return r, g, b, a

    def upconvert_fb(val):
        # The only possibilities here are R5G5B5 and R10G10B10.
        if PFB.CONFIG.BPP == 2:
            b = val & 0x1f
            g = val >> 5 & 0x1f
            r = val >> 10 & 0x1f
            if CANVAS_CONFIG.REPLICATE:
                # R, G, B are 5 bits - duplicate to get 10 bits.
                b *= 0x21
                g *= 0x21
                r *= 0x21
            else:
                b <<= 5
                g <<= 5
                r <<= 5
        else:
            b = val & 0x3ff
            g = val >> 10 & 0x3ff
            r = val >> 20 & 0x3ff
            # R, G, B are already 10-bit: nothing to do.
        return r, g, b


State color downconversion
--------------------------

Since the colors stored as part of ROP state are always stored in ``R10G10B10``
format, they need to be downconverted to the working format when needed.
This downconversion is done by simple truncation - it is assumed that they
were originally submitted in the working format, but were upconverted for
storage::

    def state_downconvert_r5g5b5(r, g, b):
        return r >> 5, g >> 5, b >> 5

    def state_downconvert_y8(r, g, b):
        return b >> 2

Final color downconversion and dithering
----------------------------------------

.. todo:: write me


Bitmap expansion
================

.. todo:: write me

Bitmap colors
-------------

.. reg:: 32 nv1-pgraph-bitmap-color Bitmap color

   .. todo:: write me

.. reg:: 32 nv1-mthd-bitmap-color Set bitmap color

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


.. _nv1-rop-chroma:

Color key
=========

If enabled by the current object, the ROP will perform color key test on all
incoming pixels: if they match the current color key, they will be discarded.

Color key conflicts with blending - if both are selected, the color key will be
effectively disabled.

The current color key is stored in ``A1R10G10B10`` format in a PGRAPH register:

.. reg:: 32 nv1-pgraph-chroma The color key

   - bits 0-9: B - the blue component
   - bits 10-19: G - the green component
   - bits 20-29: R - the red component
   - bit 30: A - the alpha component

Even though it's stored as ``A1R10G10B10``, the color key will be converted
to the working color for the color key test.

The 1-bit alpha component can be used to effectively enable or disable the
color key operation - if alpha is 0, the color key is considered to never
match, passing all pixels.

The current color key can be set by the following method:

.. reg:: 32 nv1-mthd-chroma Set the color key

   Sets the color key.  The value is interpreted according to the current
   object's color format, and upconverted to ``A1R10G10B10`` for storage.
   The alpha component is converted to 0 if the source alpha is 0, to 1
   if it's any other value::

        r, g, b, a = upconvert_src(val)
        CHROMA.A = 1 if a != 0 else 0
        CHROMA.R = r
        CHROMA.G = g
        CHROMA.B = b

The color key test works as follows::

    def chroma_pass_y8(y):
        if not CTX_SWITCH.CHROMA:
            # Color key disabled - always pass.
            return True
        if not CHROMA.A:
            # Color key alpha is 0 - always pass.
            return True
        cy = state_downconvert_y8(CHROMA.R, CHROMA.G, CHROMA.B)
        if cy == y:
            # Color key matched - kill the pixel.
            return False
        # Otherwise, pass the pixel.
        return True

    def chroma_pass_r5g5b5(r, g, b):
        if not CTX_SWITCH.CHROMA:
            # Color key disabled - always pass.
            return True
        if not CHROMA.A:
            # Color key alpha is 0 - always pass.
            return True
        cr, cg, cb = state_downconvert_r5g5b5(CHROMA.R, CHROMA.G, CHROMA.B)
        if cr == r and cg == g and cb == b:
            # Color key matched - kill the pixel.
            return False
        # Otherwise, pass the pixel.
        return True

    def chroma_pass_r10g10b10(r, g, b):
        if not CTX_SWITCH.CHROMA:
            # Color key disabled - always pass.
            return True
        if not CHROMA.A:
            # Color key alpha is 0 - always pass.
            return True
        if CHROMA.R == r and CHROMA.G == g and CHROMA.B == b:
            # Color key matched - kill the pixel.
            return False
        # Otherwise, pass the pixel.
        return True

.. note:: Color key test is performed in the working format, not in the source
   format - if they are different, color key may fail to match if a different
   REPLICATE setting is in effect when pixel is rendered vs when color key
   was submitted, even though the submitted values themselves were actually
   the same.


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
