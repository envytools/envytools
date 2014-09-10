===============
Context objects
===============

.. contents::


Introducton
===========

.. todo:: write m


.. _obj-beta:

BETA
====

The BETA object family deals with setting the beta factor for the BLEND
operation. The objects in this family are:

- objtype 0x01: NV1_BETA [NV1:NV4]
- class 0x0012: NV1_BETA [NV4:G84]

The methods are:

0100   NOP [NV4-]
0104   NOTIFY
0110   WAIT_FOR_IDLE [NV50-]
0140   PM_TRIGGER [NV40-?] [XXX]
0180 N DMA_NOTIFY [NV4-]
0200 O PATCH_BETA_OUTPUT [NV4:NV20]
0300   BETA

mthd 0x300: BETA [NV1_BETA]
  Sets the beta factor. The parameter is a signed fixed-point number with
  a sign bit and 31 fractional bits. Note that negative values are clamped
  to 0, and only 8 fractional bits are actually implemented in hardware.
Operation::
	if (param & 0x80000000) /* signed < 0 */
		BETA = 0;
	else
		BETA = param & 0x7f800000;

mthd 0x200: PATCH_BETA_OUTPUT [NV1_BETA] [NV4:NV20]
  Reserved for plugging a beta patchcord to output beta factor into.
Operation::
	throw(UNIMPLEMENTED_MTHD);


.. _obj-rop:

ROP
===

The ROP object family deals with setting the ROP [raster operation]. The ROP
value thus set is only used in the ROP_* operation modes. The objects in this
family are:

- objtype 0x02: NV1_ROP [NV1:NV4]
- class 0x0043: NV1_ROP [NV4:G84]

The methods are:

0100   NOP [NV4-]
0104   NOTIFY
0110   WAIT_FOR_IDLE [NV50-]
0140   PM_TRIGGER [NV40-?] [XXX]
0180 N DMA_NOTIFY [NV4-]
0200 O PATCH_ROP_OUTPUT [NV4:NV20]
0300   ROP

mthd 0x300: ROP [NV1_ROP]
  Sets the raster operation.
Operation:
	if (param & ~0xff)
		throw(INVALID_VALUE);
	ROP = param;

mthd 0x200: PATCH_ROP_OUTPUT [NV1_ROP] [NV4:NV20]
  Reserved for plugging a ROP patchcord to output the ROP into.
Operation:
	throw(UNIMPLEMENTED_MTHD);


.. _obj-chroma:
.. _obj-plane:

CHROMA and PLANE
================

The CHROMA object family deals with setting the color for the color key. The
color key is only used when enabled in options for a given graph object. The
objects in this family are:

- objtype 0x03: NV1_CHROMA [NV1:NV4]
- class 0x0017: NV1_CHROMA [NV4:NV50]
- class 0x0057: NV4_CHROMA [NV4:G84]

The PLANE object family deals with setting the color for plane masking. The
plane mask operation is only done when enabled in options for a given graph
object. The objects in this family are:

- objtype 0x04: NV1_PLANE [NV1:NV4]

For both objects, colors are internally stored in A1R10G10B10 format. [XXX:
check NV4+]

The methods for these families are:

0100   NOP [NV4-]
0104   NOTIFY
0110   WAIT_FOR_IDLE [NV50-]
0140   PM_TRIGGER [NV40-?] [XXX]
0180 N DMA_NOTIFY [NV4-]
0200 O PATCH_IMAGE_OUTPUT [NV4:NV20]
0300   COLOR_FORMAT [NV4-]
0304   COLOR

mthd 0x304: COLOR [\*_CHROMA, NV1_PLANE]
  Sets the color.
Operation::
	struct {
		int B : 10;
		int G : 10;
		int R : 10;
		int A : 1;
	} tmp;
	tmp.B = get_color_b10(cur_grobj, param);
	tmp.G = get_color_g10(cur_grobj, param);
	tmp.R = get_color_r10(cur_grobj, param);
	tmp.A = get_color_a1(cur_grobj, param);
	if (cur_grobj.type == NV1_PLANE)
		PLANE = tmp;
	else
		CHROMA = tmp;

.. todo:: check NV3+

mthd 0x200: PATCH_IMAGE_OUTPUT [\*_CHROMA, NV1_PLANE] [NV4:NV20]
  Reserved for plugging an image patchcord to output the color into.
Operation::
	throw(UNIMPLEMENTED_MTHD);


.. _obj-clip:

CLIP
====

The CLIP object family deals with setting up the user clip rectangle. The user
clip rectangle is only used when enabled in options for a given graph object.
The objects in this family are:

- objtype 0x05: NV1_CLIP [NV1:NV4]
- class 0x0019: NV1_CLIP [NV4:G84]

The methods for this family are:

0100   NOP [NV4-]
0104   NOTIFY
0110   WAIT_FOR_IDLE [NV50-]
0140   PM_TRIGGER [NV40-?] [XXX]
0180 N DMA_NOTIFY [NV4-]
0200 O PATCH_IMAGE_OUTPUT [NV4:NV20]
0300   CORNER
0304   SIZE

The clip rectangle state can be loaded in two ways:

- submit CORNER method twice, with upper-left and bottom-right corners
- submit CORNER method with upper-right corner, then SIZE method

To enable that, clip rectangle method operation is a bit unusual.

.. todo:: check if still applies on NV3+

Note that the clip rectangle state is internally stored relative to the
absolute top-left corner of the framebuffer, while coordinates used in
methods are relative to top-left corner of the canvas.

mthd 0x300: CORNER [NV1_CLIP]
  Sets a corner of the clipping rectangle.
  bits 0-15: X coordinate
  bits 16-31: Y coordinate
Operation::
	ABS_UCLIP_XMIN = ABS_UCLIP_XMAX;
	ABS_UCLIP_YMIN = ABS_UCLIP_YMAX;
	ABS_UCLIP_XMAX = CANVAS_MIN.X + param.X;
	ABS_UCLIP_YMAX = CANVAS_MIN.Y + param.Y;

.. todo:: check NV3+

mthd 0x304: SIZE [NV1_CLIP]
  Sets the size of the clipping rectangle.
  bits 0-15: width
  bits 16-31: height
Operation::
	ABS_UCLIP_XMIN = ABS_UCLIP_XMAX;
	ABS_UCLIP_YMIN = ABS_UCLIP_YMAX;
	ABS_UCLIP_XMAX += param.X;
	ABS_UCLIP_YMAX += param.Y;

.. todo:: check NV3+

mthd 0x200: PATCH_IMAGE_OUTPUT [NV1_CLIP] [NV4:NV20]
  Reserved for plugging an image patchcord to output the rectangle into.
Operation::
	throw(UNIMPLEMENTED_MTHD);


.. _obj-beta4:

BETA4
=====

The BETA4 object family deals with setting the per-component beta factors for
the BLEND_PREMULT and SRCCOPY_PREMULT operations. The objects in this family
are:

- class 0x0072: NV4_BETA4 [NV4:G84]

The methods are:

0100   NOP [NV4-]
0104   NOTIFY
0110   WAIT_FOR_IDLE [NV50-]
0140   PM_TRIGGER [NV40-?] [XXX]
0180 N DMA_NOTIFY [NV4-]
0200 O PATCH_BETA_OUTPUT [NV4:NV20]
0300   BETA4

mthd 0x300: BETA4 [NV4_BETA4]
  Sets the per-component beta factors.
  bits 0-7: B
  bits 8-15: G
  bits 16-23: R
  bits 24-31: A
Operation::
	/* XXX: figure it out */

mthd 0x200: PATCH_BETA_OUTPUT [NV4_BETA4] [NV4:NV20]
  Reserved for plugging a beta patchcord to output beta factors into.
Operation::
	throw(UNIMPLEMENTED_MTHD);


Surface setup
=============

.. todo:: write me


.. _obj-surf:

SURF
----

.. todo:: write me


.. _obj-surf2d:

SURF2D
------

.. todo:: write me


.. _obj-surf3d:

SURF3D
------

.. todo:: write me


.. _obj-swzsurf:

SWZSURF
-------

.. todo:: write me
