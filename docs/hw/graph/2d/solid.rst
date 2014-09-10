========================
2D solid shape rendering
========================

.. contents::


Introduction
============

One of 2d engine functions is drawing solid [single-color] primitives. The
solid drawing functions use the usual 2D pipeline as described in graph/2d.txt
and are available on all cards. The primitives supported are:

- points [NV1:NV4 and NV50+]
- lines [NV1:NV4]
- lins [half-open lines]
- triangles
- upright rectangles [edges parallel to X/Y axes]

The 2d engine is limitted to integer vertex coordinates [ie. all primitive
vertices must lie in pixel centres].

On NV1:NV84 cards, the solid drawing functions are exposed via separate
source object types for each type of primitive. On NV50+, all solid drawing
functionality is exposed via the unified 2d object.


Source objects
==============

Each supported primitive type has its own source object class family on
NV1:NV50. These families are:

- POINT [NV1:NV4]
- LINE [NV1:NV4]
- LIN [NV1:NV84]
- TRI [NV1:NV84]
- RECT [NV1:NV40]


Common methods
--------------

The common methods accepted by all solid source objects are:

0100   NOP [NV4-]				[graph/intro.txt]
0104   NOTIFY					[graph/intro.txt]
010c   PATCH [NV4:?]      			[graph/2d.txt]
0110   WAIT_FOR_IDLE [NV50-]			[graph/intro.txt]
0140   PM_TRIGGER [NV40-?]      		[graph/intro.txt]
0180 N DMA_NOTIFY [NV4-]			[graph/intro.txt]
0184 N NV1_CLIP [NV5-]			[graph/2d.txt]
0188 N NV1_PATTERN [NV5-] [NV1_*]		[graph/2d.txt]
0188 N NV4_PATTERN [NV5-] [NV4_* and up]	[graph/2d.txt]
018c N NV1_ROP [NV5-]				[graph/2d.txt]
0190 N NV1_BETA [NV5-]			[graph/2d.txt]
0194 N NV3_SURFACE [NV5-] [NV1_*]		[graph/2d.txt]
0194 N NV4_BETA4 [NV5-] [NV4_* and up]	[graph/2d.txt]
0198 N NV4_SURFACE [NV5-] [NV4_* and up]	[graph/2d.txt]
02fc N OPERATION [NV5-]			[graph/2d.txt]
0300   COLOR_FORMAT [NV4-]			[graph/solid.txt]
0304   COLOR					[graph/solid.txt]

.. todo:: PM_TRIGGER?

.. todo:: PATCH?

.. todo:: add the patchcord methods

.. todo:: document common methods


.. _obj-point:

POINT
-----

The POINT object family draws single points. The objects are:

- objtype 0x08: NV1_POINT [NV1:NV4]

The methods are:

0100:0400	[common solid rendering methods]
0400+i*4, i<32  POINT_XY
0480+i*8, i<16  POINT32_X
0484+i*8, i<16  POINT32_Y
0500+i*8, i<16  CPOINT_COLOR
0504+i*8, i<16  CPOINT_XY

.. todo:: document point methods


.. _obj-line:
.. _obj-lin:

LINE/LIN
--------

The LINE/LIN object families draw lines/lins, respectively. The objects are:

- objtype 0x09: NV1_LINE [NV1:NV4]
- objtype 0x0a: NV1_LIN [NV1:NV4]
- class 0x001c: NV1_LIN [NV4:NV40]
- class 0x005c: NV4_LIN [NV4:NV50]
- class 0x035c: NV30_LIN [NV30:NV40]
- class 0x305c: NV30_LIN [NV40:NV84]

The methods are:

0100:0400	[common solid rendering methods]
0400+i*8, i<16  LINE_START_XY
0404+i*8, i<16  LINE_END_XY
0480+i*16, i<8  LINE32_START_X
0484+i*16, i<8  LINE32_START_Y
0488+i*16, i<8  LINE32_END_X
048c+i*16, i<8  LINE32_END_Y
0500+i*4, i<32  POLYLINE_XY
0580+i*8, i<16  POLYLINE32_X
0584+i*8, i<16  POLYLINE32_Y
0600+i*8, i<16  CPOLYLINE_COLOR
0604+i*8, i<16  CPOLYLINE_XY

.. todo:: document line methods


.. _obj-tri:

TRI
---

The TRI object family draws triangles. The objects are:

- objtype 0x0b: NV1_TRI [NV1:NV4]
- class 0x001d: NV1_TRI [NV4:NV40]
- class 0x005d: NV4_TRI [NV4:NV84]

The methods are:

0100:0400		[common solid rendering methods]
0310+j*4, j<3		TRIANGLE_XY
0320+j*8, j<3		TRIANGLE32_X
0324+j*8, j<3		TRIANGLE32_Y
0400+i*4, i<32		TRIMESH_XY
0480+i*8, i<16		TRIMESH32_X
0484+i*8, i<16		TRIMESH32_Y
0500+i*16		CTRIANGLE_COLOR
0504+i*16+j*4, j<3	CTRIANGLE_XY
0580+i*8, i<16		CTRIMESH_COLOR
0584+i*8, i<16		CTRIMESH_XY

.. todo:: document tri methods


.. _obj-rect:

RECT
----

The RECT object family draws upright rectangles. Another object family that
can also draw solid rectangles and should be used instead of RECT on cards
that don't have RECT is GDI [graph/nv3-gdi.txt]. The objects are:

- objtype 0x0c: NV1_RECT [NV1:NV3]
- objtype 0x07: NV1_RECT [NV3:NV4]
- class 0x001e: NV1_RECT [NV4:NV40]
- class 0x005e: NV4_RECT [NV4:NV40]

The methods are:

0100:0400	[common solid rendering methods]
0400+i*8, i<16  RECT_POINT
0404+i*8, i<16  RECT_SIZE

.. todo:: document rect methods


Unified 2d object
=================

.. todo:: document solid-related unified 2d object methods


Rasterization rules
===================

This section describes exact rasterization rules for solids, ie. which pixels
are considered to be part of a given solid. The common variables appearing
in the pseudocodes are:

- CLIP_MIN_X - the left bounduary of the final clipping rectangle. If user
  clipping rectangle [see graph/2d.txt] is enabled, this is max(UCLIP_MIN_X,
  CANVAS_MIN_X). Otherwise, this is CANVAS_MIN_X.
- CLIP_MAX_X - the right bounduary of the final clipping rectangle. If user
  clipping rectangle is enabled, this is min(UCLIP_MAX_X, CANVAS_MAX_X).
  Otherwise, this is CANVAS_MAX_X.
- CLIP_MIN_Y - the top bounduary of the final clipping rectangle, defined
  like CLIP_MIN_X
- CLIP_MAX_Y - the bottom bounduary of the final clipping rectangle, defined
  like CLIP_MAX_X

A pixel is considered to be inside the clipping rectangle if:

- CLIP_MIN_X <= x < CLIP_MAX_X and
- CLIP_MIN_Y <= y < CLIP_MAX_Y


Points and rectangles
---------------------

A rectangle is defined through the coordinates of its left-top corner [X, Y]
and its width and height [W, H] in pixels. A rectangle covers pixels that
have x in [X, X+W) and y in [Y, Y+H) ranges.

::

    void SOLID_RECT(int X, int Y, int W, int H) {
        int L = max(X, CLIP_MIN_X);
        int R = min(X+W, CLIP_MAX_X);
        int T = max(Y, CLIP_MIN_Y);
        int B = min(Y+H, CLIP_MAX_Y);
        int x, y;
        for (y = T; y < B; y++)
            for (x = L; x < R; x++)
                DRAW_PIXEL(x, y, SOLID_COLOR);
    }

A point is defined through its X, Y coordinates and is rasterized as if it was
a rectangle with W=H=1.

::

    void SOLID_POINT(int X, int Y) {
        SOLID_RECT(X, Y, 1, 1);
    }


Lines and lins
--------------

Lines and lins are defined through the coordinates of two endpoints [X[2],
Y[2]]. They are rasterized via a variant of Bresenham's line algorithm, with
the following characteristics:

- rasterization proceeds in the direction of increasing x for y-major lines,
  and in the direction of increasing y for x-major lines [ie. in the
  direction of increasing *minor* component]
- when presented with a tie in a decision whether to increase the minor
  coordinate or not, increase it.
- if rasterizing a lin, the X[1], Y[1] pixel is not rasterized, but
  calculations are otherwise unaffected
- pixels outside the clipping rectangle are not rasterized, but calculations
  are otherwise unaffected

Equivalently, the rasterized lines/lins match those constructed via the
diamond-exit rule with the following characteristics:

- a pixel is rasterized if the diamond inside it intersects the line/lin,
  unless it's a lin and the diamond also contains the second endpoint
- pixels outside the clipping rectangle are not rasterized, but calculations
  are otherwise unaffected
- pixel centres are considered to be on integer coordinates
- the following coordinates are considered to be contained in the diamond for
  pixel X, Y:

  - abs(x-X) + abs(x-Y) < 0.5 [ie. the inside of the diamond]
  - x = X-0.5, y = Y [ie. top vertex of the diamond]
  - x = X, y = Y-0.5 [ie. leftmost vertex of the diamond]

  [note that the edges don't matter, other than at the vertices - it's
  impossible to create a line touching them without intersecting them, due
  to integer endpoint coordinates]

::

    void SOLID_LINE_LIN(int X[2], int Y[2], int is_lin) {
        /* determine minor/major direction */
        int xmajor = abs(X[0] - X[1]) > abs(Y[0] - Y[1]);
        int min0, min1, maj0, maj1;
        if (xmajor) {
            maj0 = X[0];
            maj1 = X[1];
            min0 = Y[0];
            min1 = Y[1];
        } else {
            maj0 = Y[0];
            maj1 = Y[1];
            min0 = X[0];
            min1 = X[1];
        }
        if (min1 < min0) {
            /* order by increasing minor */
            swap(min0, min1);
            swap(maj0, maj1);
        }
        /* deltas */
        int dmin = min1 - min0;
        int dmaj = abs(maj1 - maj0);
        /* major step direction */
        int step = maj1 > maj0 ? 1 : -1;
        int min, maj;
        /* scaled error - real error is err/(dmin * dmaj * 2) */
        int err = 0;
        for (min = min0, maj = maj0; maj != maj1 + step; maj += step) {
            if (err >= dmaj) { /* error >= 1/(dmin*2) */
                /* error too large, increase minor */
                min++;
                err -= dmaj * 2; /* error -= 1/dmin */
            }
            int x = xmajor?maj:min;
            int y = xmajor?min:maj;
            /* if not the final pixel of a lin and inside the clipping
               region, draw it */
            if ((!is_lin || x != X[1] || y != Y[1]) && in_clip(x, y))
                DRAW_PIXEL(x, y, SOLID_COLOR);
            error += dmin * 2; /* error += 1/dmaj */
        }
    }


Triangles
---------

Triangles are defined through the coordinates of three vertices [X[3], Y[3]].
A triangle is rasterized as an intersection of three half-planes,
corresponding to the three edges. For the purpose of triangle rasterization,
half-planes are defined as follows:

- the edges are (0, 1), (1, 2) and (2, 0)
- if the two vertices making an edge overlap, the triangle is degenerate and
  is not rasterized
- a pixel is considered to be in a half-plane corresponding to a given edge
  if it's on the same side of that edge as the third vertex of the triangle
  [the one not included in the edge]
- if the third vertex lies on the edge, the triangle is degenerate and will
  not be rasterized
- if the pixel being considered for rasterization lies on the edge, it's
  considered included in the half-plane if the pixel immediately to its right
  is included in the half-plane
- if that pixel also lies on the edge [ie. edge is exactly horizontal], the
  original pixel is instead considered included if the pixel immediately
  below it is included in the half-plane

Equivalently, a triangle will include exactly-horizontal top edges and left
edges, but not exactly-horizontal bottom edges nor right edges.

::

    void SOLID_TRI(int X[3], int Y[3]) {
        int cross = (X[1] - X[0]) * (Y[2] - Y[0]) - (X[2] - X[0]) * (Y[1] - Y[0]);
        if (cross == 0) /* degenerate triangle */
            return;
        /* coordinates in CW order */
        if (cross < 0) {
            swap(X[1], X[2]);
            swap(Y[1], Y[2]);
        }
        int x, y, e;
        for (y = CLIP_MIN_Y; y < CLIP_MAX_Y; y++)
            for (x = CLIP_MIN_X; x < CLIP_MAX_X; x++) {
                for (e = 0; e < 3; e++) {
                    int x0 = X[e];
                    int y0 = Y[e];
                    int x1 = X[(e+1)%3];
                    int y1 = Y[(e+1)%3];
                    /* first attempt */
                    cross = (x1 - x0) * (y - y0) - (x - x0) * (y1 - y0);
                    /* second attempt - pixel to the right */
                    if (cross == 0)
                        cross = (x1 - x0) * (y - y0) - (x + 1 - x0) * (y1 - y0);
                    /* third attempt - pixel below */
                    if (cross == 0)
                        cross = (x1 - x0) * (y + 1 - y0) - (x - x0) * (y1 - y0);
                    if (cross < 0)
                        goto out;
                }
                DRAW_PIXEL(x, y, SOLID_COLOR);
    out:
            }
    }
