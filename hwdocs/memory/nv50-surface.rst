.. _nv50-surface:

=====================
NV50- surface formats
=====================

.. contents::


Introduction
============

This file deals with nv50+ cards only. For older cards, see :ref:`nv01-surface`.

A "surface" is a 2d or 3d array of elements. Surfaces are used for image
storage, and can be bound to at least the following slots on the engines:

- m2mf input and output buffers
- 2d source and destination surfaces
- 3d/compute texture units: the textures
- 3d color render targets
- 3d zeta render target
- compute g[] spaces [NV50:NVC0]
- 3d/compute image units [NVC0+]
- PCOPY input and output buffers
- PDISPLAY: the framebuffer

.. todo:: vdec stuff
.. todo:: NVC0 ZCULL?

Surfaces on nv50+ cards come in two types: linear and tiled. Linear surfaces
have a simple format, but they're are limitted to 2 dimensions only, don't
support arrays nor mipmapping when used as textures, cannot be used for zeta
buffers, and have lower performance than tiled textures. Tiled surfaces
can have up to three dimensions, can be put into arrays and be mipmapped,
and use custom element arrangement in memory. However, tiled surfaces need to
be placed in memory area with special storage type, depending on the surface
format.

Tiled surfaces have two main levels of element rearrangement: high-level and
low-level. Low-level rearrangement is quite complicated, depends on surface's
storage type, and is hidden by the VM subsystem - if the surface is accessed
through VM with properly set storage type, only the high-level rearrangement
is visible. Thus the low-level rearrangement can only be seen when accessing
tiled system RAM directly from CPU, or accessing tiled VRAM with storage type
set to 0. Also, low-level rearrangement for VRAM uses several tricks to
distribute load evenly across memory partitions, while rearrangement for
system RAM skips them and merely reorders elements inside a roptile. High-level
rearrangement, otoh, is relatively simple, and always visible to the user -
its knowledge is needed to calculate address of a given element, or to
calculate the memory size of a surface.


Surface elements
================

A basic unit of surface is an "element", which can be 1, 2, 4, 8, or 16 bytes
long. element type is vital in selecting the proper compressed storage type
for a surface. For most surface formats, an element means simply a sample. This
is different for surfaces storing compressed textures - the elements are
compressed blocks. Also, it's different for bitmap textures - in these, an
element is a 64-bit word containing 8x8 block of samples.

While texture, RT, and 2d bindings deal only with surface elements, they're
ignored by some other binding points, like PCOPY and m2mf - in these, the
element size is ignored, and the surface is treated as an array of bytes. That
is, a 16x16 surface of 4-byte elements is treated as a 64x16 surface of bytes.


Linear surfaces
===============

A linear surface is a 2d array of elements, where each row is contiguous in
memory, and each row starts at a fixed distance from start of the previous
one. This distance is the surface's "pitch". Linear surfaces always use
storage type 0 [linear].

The attributes defining a linear surface are:

- address: 40-bit VM address, aligned to 64 bytes
- pitch: distance between subsequent rows in bytes - needs to be a multiple
  of 64
- element size: implied by format, or defaulting to 1 if binding point is
  byte-oriented
- width: surface width in elements, only used when bounds checking / size
  information is needed
- height: surface height in elements, only used when bounds checking / size
  information is needed

.. todo:: check pitch, width, height min/max values. this may depend on binding
   point. check if 64 byte alignment still holds on NVC0.

The address of element (x,y) is::

    address + pitch * y + elem_size * x

Or, alternatively, the address of byte (x,y) is::

    address + pitch * y + x


Tiled surfaces
==============

A tiled surface is a 3d array of elements, stored in memory in units called
"tiles". There are two levels of tiling. The lower-level tile is called
a "roptile" and has a fixed size. This size is 64 bytes × 4 × 1 on NV50:NVC0
cards, 64 bytes × 8 × 1 for NVC0+ cards. The higher-level tile is called
a bigtile, and is of variable size between 1×1×1 and 32×32×32 roptiles.

The attributes defining a tiled surface are:

- address: 40-bit VM address, aligned to roptile size [0x100 bytes on
  NV50:NVC0, 0x200 bytes on NVC0]
- tile size x: 0-5, log2 of roptiles per bigtile in x dimension
- tile size y: 0-5, log2 of roptiles per bigtile in y dimension
- tile size z: 0-5, log2 of roptiles per bigtile in z dimension
- element size: implied by format, or defaulting to 1 if the binding point
  is byte-oriented
- width: surface width [size in x dimension] in elements
- height: surface height [size in y dimension] in elements
- depth: surface depth [size in z dimension] in elements

.. todo:: check bounduaries on them all, check tiling on NVC0.
.. todo:: PCOPY surfaces with weird tile size

It should be noted that some limits on these parameters are to some extent
specific to the binding point. In particular, x tile size greater than 0 is
only supported by the render targets and texture units, with render targets
only supporting 0 and 1. y tile sizes 0-5 can be safely used with all tiled
surface binding points, and z tile sizes 0-5 can be used with binding points
other than NV50 g[] spaces, which only support 0.

The tiled format works as follows:

First, the big tile size is computed. This computation depends on the binding
point: some binding points clamp the effective bigtile size in a given
dimension to the smallest size that would cover the whole surfaces, some do
not. The ones that do are called "auto-sizing" binding points. One of such
binding ports where it's important is the texture unit: since all mipmap
levels of a texture use a single "tile size" field in TIC, the auto-sizing is
needed to ensure that small mipmaps of a large surface don't use needlessly
large tiles. Pseudocode::

    bytes_per_roptile_x = 64;
    if (chipset < NVC0)
        bytes_per_roptile_y = 4;
    else
        bytes_per_roptile_y = 8;
    bytes_per_roptile = 1;
    eff_tile_size_x = tile_size_x;
    eff_tile_size_y = tile_size_y;
    eff_tile_size_z = tile_size_z;
    if (auto_sizing) {
        while (eff_tile_size_x > 0 && (bytes_per_roptile_x << (eff_tile_size_x - 1)) >= width * element_size)
            eff_tile_size_x--;
        while (eff_tile_size_y > 0 && (bytes_per_roptile_y << (eff_tile_size_y - 1)) >= height)
            eff_tile_size_y--;
        while (eff_tile_size_z > 0 && (bytes_per_roptile_z << (eff_tile_size_z - 1)) >= depth)
            eff_tile_size_z--;
    }
    roptiles_per_bigtile_x = 1 << eff_tile_size_x;
    roptiles_per_bigtile_y = 1 << eff_tile_size_y;
    roptiles_per_bigtile_z = 1 << eff_tile_size_z;
    bytes_per_bigtile_x = bytes_per_roptile_x * roptiles_per_bigtile_x;
    bytes_per_bigtile_y = bytes_per_roptile_y * roptiles_per_bigtile_y;
    bytes_per_bigtile_z = bytes_per_roptile_z * roptiles_per_bigtile_z;
    elements_per_bigtile_x = bytes_per_bigtile_x / element_size;
    roptile_bytes = bytes_per_roptile_x * bytes_per_roptile_y * bytes_per_roptile_z;
    bigtile_roptiles = roptiles_per_bigtils_x * roptiles_per_bigtile_y * roptiles_per_bigtile_z;
    bigtile_bytes = roptile_bytes * bigtile_roptiles;

Due to the auto-sizing being present on some binding points, it's a bad idea
to use surfaces that have bigtile size at least two times bigger than the
actual surface - they'll be unusable on these binding points [and waste a lot
of memory anyway].

Once bigtile size is known, the geometry and size of the surface can be
determined. A surface is first broken down into bigtiles. Each bigtile convers
a contiguous elements_per_bigtile_x × bytes_per_bigtile_y × bytes_per_bigtile_z
aligned subarea of the surface. If the surface size is not a multiple of the
bigtile size in any dimension, the size is aligned up for surface layout
purposes and the remaining space is unused. The bigtiles making up a surface
are stored sequentially in memory first in x direction, then in y direction,
then in z direction::

    bigtiles_per_surface_x = ceil(width * element_size / bytes_per_bigtile_x);
    bigtiles_per_surface_y = ceil(height / bytes_per_bigtile_y);
    bigtiles_per_surface_z = ceil(depth / bytes_per_bigtile_z);
    surface_bigtiles = bigtiles_per_surface_x * bigtiles_per_surface_y * bigtiles_per_surface_z;
    // total bytes in surface - surface resides at addresses [address, address+surface_bytes)
    surface_bytes = surface_bigtiles * bigtile_bytes;
    bigtile_address = address + floor(x_coord * element_size / bytes_per_bigtile_x) * bigtile_bytes
                + floor(y_coord / bytes_per_bigtile_y) * bigtile_bytes * bigtiles_per_surface_x;
                + floor(z_coord / bytes_per_bigtile_z) * bigtile_bytes * bigtiles_per_surface_x * bigtiles_per_surface_z;
    x_coord_in_bigtile = (x_coord * element_size) % bytes_per_bigtile_x;
    y_coord_in_bigtile = y_coord % bytes_per_bigtile_y;
    z_coord_in_bigtile = z_coord % bytes_per_bigtile_z;

Like bigtiles in the surface, roptiles inside a bigtile are stored ordered first by x coord, then by y coord, then by z coord::

    roptile_address = bigtile_address
            + floor(x_coord_in_bigtile / bytes_per_roptile_x) * roptile_bytes
            + floor(y_coord_in_bigtile / bytes_per_roptile_y) * roptile_bytes * roptiles_per_bigtile_x
            + z_coord_in_bigtile * roptile_bytes * roptiles_per_bigtile_x * roptiles_per_bigtile_y; // bytes_per_roptile_z always 1.
    x_coord_in_roptile = x_coord_in_bigtile % bytes_per_roptile_x;
    y_coord_in_roptile = y_coord_in_bigtile % bytes_per_roptile_y;

The elements inside a roptile are likewise stored ordered first by x coordinate, and then by y::

    element_address = roptile_address + x_coord_in_roptile + y_coord_in_roptile * bytes_per_roptile_x;

Note that the above is the higher-level rearrangement only - the element
address resulting from the above pseudocode is the address that user would see
by looking through the card's VM subsystem. The lower-level rearrangement is
storage type dependent, invisible to the user, and will be covered below.

As an example, let's take a 13 × 17 × 3 surface with element size of 16
bytes, tile size x of 1, tile size y of 1, and tile size z of 1. Further,
the card is assumed to be nv50. The surface will be located in memory the
following way:

- bigtile size in bytes = 0x800 bytes
- bigtile width: 128 bytes / 8 elements
- bigtile height: 8
- bigtile depth: 2
- surface width in bigtiles: 2
- surface height in bigtiles: 3
- surface depth in bigtiles: 2
- surface memory size: 0x6000 bytes

::

    | - x element bounduary
    || - x roptile bounduary
    ||| - x bigtile bounduary
    [no line] - y element bounduary
    --- - y roptile bounduary
    === - y bigtile bounduary

    z == 0:
     x -->
    y+--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
    ||  |  0 |  1 |  2 |  3 ||  4 |  5 |  6 |  7 |||  8 |  9 | 10 | 11 || 12 |
    |+--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
    V| 0|0000|0010|0020|0030||0100|0110|0120|0130|||0800|0810|0820|0830||0900|
     | 1|0040|0050|0060|0070||0140|0150|0160|0170|||0840|0850|0860|0870||0940|
     | 2|0080|0090|00a0|00b0||0180|0190|01a0|01b0|||0880|0890|08a0|08b0||0980|
     | 3|00c0|00d0|00e0|00f0||01c0|01d0|01e0|01f0|||08c0|08d0|08e0|08f0||09c0|
     +--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
     | 4|0200|0210|0220|0230||0300|0310|0320|0330|||0a00|0a10|0a20|0a30||0b00|
     | 5|0240|0250|0260|0270||0340|0350|0360|0370|||0a40|0a50|0a60|0a70||0b40|
     | 6|0280|0290|02a0|02b0||0380|0390|03a0|03b0|||0a80|0a90|0aa0|0ab0||0b80|
     | 7|02c0|02d0|02e0|02f0||03c0|03d0|03e0|03f0|||0ac0|0ad0|0ae0|0af0||0bc0|
     +==+====+====+====+====++====+====+====+====+++====+====+====+====++====+
     | 8|1000|1010|1020|1030||1100|1110|1120|1130|||1800|1810|1820|1830||1900|
     | 9|1040|1050|1060|1070||1140|1150|1160|1170|||1840|1850|1860|1870||1940|
     |10|1080|1090|10a0|10b0||1180|1190|11a0|11b0|||1880|1890|18a0|18b0||1980|
     |11|10c0|10d0|10e0|10f0||11c0|11d0|11e0|11f0|||18c0|18d0|18e0|18f0||19c0|
     +--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
     |12|1200|1210|1220|1230||1300|1310|1320|1330|||1a00|1a10|1a20|1a30||1b00|
     |13|1240|1250|1260|1270||1340|1350|1360|1370|||1a40|1a50|1a60|1a70||1b40|
     |14|1280|1290|12a0|12b0||1380|1390|13a0|13b0|||1a80|1a90|1aa0|1ab0||1b80|
     |15|12c0|12d0|12e0|12f0||13c0|13d0|13e0|13f0|||1ac0|1ad0|1ae0|1af0||1bc0|
     +==+====+====+====+====++====+====+====+====+++====+====+====+====++====+
     |16|2000|2010|2020|2030||2100|2110|2120|2130|||2800|2810|2820|2830||2900|
     +--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
    z == 1:
     x -->
    y+--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
    ||  |  0 |  1 |  2 |  3 ||  4 |  5 |  6 |  7 |||  8 |  9 | 10 | 11 || 12 |
    |+--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
    V| 0|0400|0410|0420|0430||0500|0510|0520|0530|||0c00|0c10|0c20|0c30||0d00|
     | 1|0440|0450|0460|0470||0540|0550|0560|0570|||0c40|0c50|0c60|0c70||0d40|
     | 2|0480|0490|04a0|04b0||0580|0590|05a0|05b0|||0c80|0c90|0ca0|0cb0||0d80|
     | 3|04c0|04d0|04e0|04f0||05c0|05d0|05e0|05f0|||0cc0|0cd0|0ce0|0cf0||0dc0|
     +--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
     | 4|0600|0610|0620|0630||0700|0710|0720|0730|||0e00|0a10|0e20|0a30||0f00|
     | 5|0640|0650|0660|0670||0740|0750|0760|0770|||0e40|0a50|0e60|0a70||0f40|
     | 6|0680|0690|06a0|06b0||0780|0790|07a0|07b0|||0e80|0a90|0ea0|0ab0||0f80|
     | 7|06c0|06d0|06e0|06f0||07c0|07d0|07e0|07f0|||0ec0|0ad0|0ee0|0af0||0fc0|
     +==+====+====+====+====++====+====+====+====+++====+====+====+====++====+
     | 8|1400|1410|1420|1430||1500|1510|1520|1530|||1c00|1c10|1c20|1c30||1d00|
     | 9|1440|1450|1460|1470||1540|1550|1560|1570|||1c40|1c50|1c60|1c70||1d40|
     |10|1480|1490|14a0|14b0||1580|1590|15a0|15b0|||1c80|1c90|1ca0|1cb0||1d80|
     |11|14c0|14d0|14e0|14f0||15c0|15d0|15e0|15f0|||1cc0|1cd0|1ce0|1cf0||1dc0|
     +--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
     |12|1600|1610|1620|1630||1700|1710|1720|1730|||1e00|1e10|1e20|1e30||1f00|
     |13|1640|1650|1660|1670||1740|1750|1760|1770|||1e40|1e50|1e60|1e70||1f40|
     |14|1680|1690|16a0|16b0||1780|1790|17a0|17b0|||1e80|1e90|1ea0|1eb0||1f80|
     |15|16c0|16d0|16e0|16f0||17c0|17d0|17e0|17f0|||1ec0|1ed0|1ee0|1ef0||1fc0|
     +==+====+====+====+====++====+====+====+====+++====+====+====+====++====+
     |16|2400|2410|2420|2430||2500|2510|2520|2530|||2c00|2c10|2c20|2c30||2d00|
     +--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
    [z bigtile bounduary here]
    z == 2:
     x -->
    y+--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
    ||  |  0 |  1 |  2 |  3 ||  4 |  5 |  6 |  7 |||  8 |  9 | 10 | 11 || 12 |
    |+--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
    V| 0|3000|3010|3020|3030||3100|3110|3120|3130|||3800|3810|3820|3830||3900|
     | 1|3040|3050|3060|3070||3140|3150|3160|3170|||3840|3850|3860|3870||3940|
     | 2|3080|3090|30a0|30b0||3180|3190|31a0|31b0|||3880|3890|38a0|38b0||3980|
     | 3|30c0|30d0|30e0|30f0||31c0|31d0|31e0|31f0|||38c0|38d0|38e0|38f0||39c0|
     +--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
     | 4|3200|3210|3220|3230||3300|3310|3320|3330|||3a00|3a10|3a20|3a30||3b00|
     | 5|3240|3250|3260|3270||3340|3350|3360|3370|||3a40|3a50|3a60|3a70||3b40|
     | 6|3280|3290|32a0|32b0||3380|3390|33a0|33b0|||3a80|3a90|3aa0|3ab0||3b80|
     | 7|32c0|32d0|32e0|32f0||33c0|33d0|33e0|33f0|||3ac0|3ad0|3ae0|3af0||3bc0|
     +==+====+====+====+====++====+====+====+====+++====+====+====+====++====+
     | 8|4000|4010|4020|4030||4100|4110|4120|4130|||4800|4810|4820|4830||4900|
     | 9|4040|4050|4060|4070||4140|4150|4160|4170|||4840|4850|4860|4870||4940|
     |10|4080|4090|40a0|40b0||4180|4190|41a0|41b0|||4880|4890|48a0|48b0||4980|
     |11|40c0|40d0|40e0|40f0||41c0|41d0|41e0|41f0|||48c0|48d0|48e0|48f0||49c0|
     +--+----+----+----+----++----+----+----+----+++----+----+----+----++----+
     |12|4200|4210|4220|4230||4300|4310|4320|4330|||4a00|4a10|4a20|4a30||4b00|
     |13|4240|4250|4260|4270||4340|4350|4360|4370|||4a40|4a50|4a60|4a70||4b40|
     |14|4280|4290|42a0|42b0||4380|4390|43a0|43b0|||4a80|4a90|4aa0|4ab0||4b80|
     |15|42c0|42d0|42e0|42f0||43c0|43d0|43e0|43f0|||4ac0|4ad0|4ae0|4af0||4bc0|
     +==+====+====+====+====++====+====+====+====+++====+====+====+====++====+
     |16|5000|5010|5020|2030||5100|5110|5120|5130|||5800|5810|5820|5830||5900|
     +--+----+----+----+----++----+----+----+----+++----+----+----+----++----+


Textures, mipmapping and arrays
===============================

A texture on NV50/NVC0 can have one of 9 types:

- 1D: made of 1 or more mip levels, each mip level is a tiled surface with
  height and depth forced to 1
- 2D: made of 1 or more mip levels, each mip level is a tiled surface with
  depth forced to 1
- 3D: made of 1 or more mip levels, each mip level is a tiled surface
- 1D_ARRAY: made of some number of subtextures, each subtexture is like
  a single 1D texture
- 2D_ARRAY: made of some number of subtextures, each subtexture is like
  a single 2D texture
- CUBE: made of 6 subtextures, each subtexture is like a single 2D texture -
  has the same layout as a 2D_ARRAY with 6 subtextures, but different
  semantics
- BUFFER: a simple packed 1D array of elements - not a surface
- RECT: a single linear surface, or a single tiled surface with depth forced
  to 1
- CUBE_ARRAY [NVA3+]: like 2D_ARRAY, but subtexture count has to be divisible
  by 6, and groups of 6 subtextures behave like CUBE textures

Types other than BUFFER and RECT are made of subtextures, which are in turn
made of mip levels, which are tiled surfaces. For such textures, only the
parameters of the first mip level of the first subtexture are specified -
parameters of the following mip levels and subtextures are calculated
automatically.

Each mip level has each dimension 2 times smaller than the corresponding
dimension of previous mip level, rounding down unless it would result in size
of 0. Since texture units use auto-sizing for the tile size, the bigtile sizes
will be different between mip levels. The surface for each mip level starts
right after the previous one ends. Also, the total size of the subtexture is
rounded up to the size of the 0th mip level's bigtile size::

    mip_address[0] = subtexture_address;
    mip_width[0] = texture_width;
    mip_height[0] = texture_height;
    mip_depth[0] = texture_depth;
    mip_bytes[0] = calc_surface_bytes(mip[0]);
    subtexture_bytes = mip_bytes[0];
    for (i = 1; i <= max_mip_level; i++) {
        mip_address[i] = mip_address[i-1] + mip_bytes[i-1];
        mip_width[i] = max(1, floor(mip_width[i-1] / 2));
        mip_height[i] = max(1, floor(mip_height[i-1] / 2));
        mip_depth[i] = max(1, floor(mip_depth[i-1] / 2));
        mip_bytes[i] = calc_surface_bytes(mip[1]);
        subtexture_bytes += mip_bytes[i];
    }
    subtexture_bytes = alignup(subtexture_bytes, calc_surface_bigtile_bytes(mip[0]));

For 1D_ARRAY, 2D_ARRAY, CUBE and CUBE_ARRAY textures, the subtextures are
stored sequentially::

    for (i = 0; i < subtexture_count; i++) {
        subtexture_address[i] = texture_address + i * subtexture_bytes;
    }

For more information about textures, see graph/nv50-texture.txt


Multisampled surfaces
=====================

Some surfaces are used as multisampled surfaces. This includes surfaces bound
as color and zeta render targets when multisampling type is other than 1X, as
well as multisampled textures on nvc0+.

A multisampled surface contains several samples per pixel. A "sample" is
a single set of RGBA or depth/stencil values [depending on surface type].
These samples correspond to various points inside the pixel, called sample
positions. When a multisample surface has to be displayed, it is downsampled
to a normal surface by an operation called "resolving".

nv50+ GPUs also support a variant of multisampling called "coverage sampling"
or CSAA. When CSAA is used, there are two sample types: full samples and
coverage samples. Full samples behave as in normal multisampling. Coverage
samples have assigned positions inside a pixel, but their values are not
stored in the render target surfaces when rendering. Instead, a special
component, called C or coverage, is added to the zeta surface, and for each
coverage sample, a bitmask of full samples with the same value is stored.
During the resolve process, this bitmask is used to assign different weights
to the full samples depending on the count of coverage samples with matching
values, thus improving picture quality. Note that the C component conceptually
belongs to a whole pixel, not to individual samples. However, for surface
layout purposes, its value is split into several parts, and each of the parts
is stored together with one of the samples.

For the most part, multisampling mode does not affect surface layout - in
fact, a multisampled render target is bound as a non-multisampled texture for
the resolving process. However, multisampling mode is vital for CSAA zeta
surface layout, and for render target storage type selection if compression is
to be used - the compression schema used is directly tied to multisampling
mode.

The following multisample modes exist:

- mode 0x0: MS1 [1×1] - no multisampling

  - sample 0: (0x0.8, 0x0.8) [0,0]

- mode 0x1: MS2 [2×1]

  - sample 0: (0x0.4, 0x0.4) [0,0]
  - sample 1: (0x0.c, 0x0.c) [1,0]

- mode 0x2: MS4 [2×2]

  - sample 0: (0x0.6, 0x0.2) [0,0]
  - sample 1: (0x0.e, 0x0.6) [1,0]
  - sample 2: (0x0.2, 0x0.a) [0,1]
  - sample 3: (0x0.a, 0x0.e) [1,1]

- mode 0x3: MS8 [4×2]

  - sample 0: (0x0.1, 0x0.7) [0,0]
  - sample 1: (0x0.5, 0x0.3) [1,0]
  - sample 2: (0x0.3, 0x0.d) [0,1]
  - sample 3: (0x0.7, 0x0.b) [1,1]
  - sample 4: (0x0.9, 0x0.5) [2,0]
  - sample 5: (0x0.f, 0x0.1) [3,0]
  - sample 6: (0x0.b, 0x0.f) [2,1]
  - sample 7: (0x0.d, 0x0.9) [3,1]

- mode 0x4: MS2_ALT [2×1] [NVA3-]

  - sample 0: (0x0.c, 0x0.c) [1,0]
  - sample 1: (0x0.4, 0x0.4) [0,0]

- mode 0x5: MS8_ALT [4×2] [NVA3-]

  - sample 0: (0x0.9, 0x0.5) [2,0]
  - sample 1: (0x0.7, 0x0.b) [1,1]
  - sample 2: (0x0.d, 0x0.9) [3,1]
  - sample 3: (0x0.5, 0x0.3) [1,0]
  - sample 4: (0x0.3, 0x0.d) [0,1]
  - sample 5: (0x0.1, 0x0.7) [0,0]
  - sample 6: (0x0.b, 0x0.f) [2,1]
  - sample 7: (0x0.f, 0x0.1) [3,0]

- mode 0x6: ??? [NVC0-] [XXX]
- mode 0x8: MS4_CS4 [2×2]

  - sample 0: (0x0.6, 0x0.2) [0,0]
  - sample 1: (0x0.e, 0x0.6) [1,0]
  - sample 2: (0x0.2, 0x0.a) [0,1]
  - sample 3: (0x0.a, 0x0.e) [1,1]
  - coverage sample 4: (0x0.5, 0x0.7), belongs to 1, 3, 0, 2
  - coverage sample 5: (0x0.9, 0x0.4), belongs to 3, 2, 1, 0
  - coverage sample 6: (0x0.7, 0x0.c), belongs to 0, 1, 2, 3
  - coverage sample 7: (0x0.b, 0x0.9), belongs to 2, 0, 3, 1

  C component is 16 bits per pixel, bitfields:

  - 0-3: sample 4 associations: 0, 1, 2, 3
  - 4-7: sample 5 associations: 0, 1, 2, 3
  - 8-11: sample 6 associations: 0, 1, 2, 3
  - 12-15: sample 7 associations: 0, 1, 2, 3

- mode 0x9: MS4_CS12 [2×2]
  - sample 0: (0x0.6, 0x0.1) [0,0]
  - sample 1: (0x0.f, 0x0.6) [1,0]
  - sample 2: (0x0.1, 0x0.a) [0,1]
  - sample 3: (0x0.a, 0x0.f) [1,1]
  - coverage sample 4: (0x0.4, 0x0.e), belongs to 2, 3
  - coverage sample 5: (0x0.c, 0x0.3), belongs to 1, 0
  - coverage sample 6: (0x0.d, 0x0.d), belongs to 3, 1
  - coverage sample 7: (0x0.4, 0x0.4), belongs to 0, 2
  - coverage sample 8: (0x0.9, 0x0.5), belongs to 0, 1, 2
  - coverage sample 9: (0x0.7, 0x0.7), belongs to 0, 2, 1, 3
  - coverage sample a: (0x0.b, 0x0.8), belongs to 1, 3, 0
  - coverage sample b: (0x0.3, 0x0.8), belongs to 2, 0, 3
  - coverage sample c: (0x0.8, 0x0.c), belongs to 3, 2, 1
  - coverage sample d: (0x0.2, 0x0.2), belongs to 0, 2
  - coverage sample e: (0x0.5, 0x0.b), belongs to 2, 3, 0, 1
  - coverage sample f: (0x0.e, 0x0.9), belongs to 1, 3

  C component is 32 bits per pixel, bitfields:

  - 0-1: sample 4 associations: 2, 3
  - 2-3: sample 5 associations: 0, 1
  - 4-5: sample 6 associations: 1, 3
  - 6-7: sample 7 associations: 0, 2
  - 8-10: sample 8 associations: 0, 1, 2
  - 11-14: sample 9 associations: 0, 1, 2, 3
  - 15-17: sample a associations: 0, 1, 3
  - 18-20: sample b associations: 0, 2, 3
  - 31-23: sample c associations: 1, 2, 3
  - 24-25: sample d associations: 0, 2
  - 26-29: sample e associations: 0, 1, 2, 3
  - 30-31: sample f associations: 1, 3

- mode 0xa: MS8_CS8 [4×2]

  - sample 0: (0x0.1, 0x0.3) [0,0]
  - sample 1: (0x0.6, 0x0.4) [1,0]
  - sample 2: (0x0.3, 0x0.f) [0,1]
  - sample 3: (0x0.4, 0x0.b) [1,1]
  - sample 4: (0x0.c, 0x0.1) [2,0]
  - sample 5: (0x0.e, 0x0.7) [3,0]
  - sample 6: (0x0.8, 0x0.8) [2,1]
  - sample 7: (0x0.f, 0x0.d) [3,1]
  - coverage sample 8: (0x0.5, 0x0.7), belongs to 1, 6, 3, 0
  - coverage sample 9: (0x0.7, 0x0.2), belongs to 1, 0, 4, 6
  - coverage sample a: (0x0.b, 0x0.6), belongs to 5, 6, 1, 4
  - coverage sample b: (0x0.d, 0x0.3), belongs to 4, 5, 6, 1
  - coverage sample c: (0x0.2, 0x0.9), belongs to 3, 0, 2, 1
  - coverage sample d: (0x0.7, 0x0.c), belongs to 3, 2, 6, 7
  - coverage sample e: (0x0.a, 0x0.e), belongs to 7, 3, 2, 6
  - coverage sample f: (0x0.c, 0x0.a), belongs to 5, 6, 7, 3

  C component is 32 bits per pixel, bitfields:

  - 0-3: sample 8 associations: 0, 1, 3, 6
  - 4-7: sample 8 associations: 0, 1, 4, 6
  - 8-11: sample 8 associations: 1, 4, 5, 6
  - 12-15: sample 8 associations: 1, 4, 5, 6
  - 16-19: sample 8 associations: 0, 1, 2, 3
  - 20-23: sample 8 associations: 2, 3, 6, 7
  - 24-27: sample 8 associations: 2, 3, 6, 7
  - 28-31: sample 8 associations: 3, 5, 6, 7

- mode 0xb: MS8_CS24 [NVC0-]

.. todo:: wtf is up with modes 4 and 5?
.. todo:: nail down MS8_CS24 sample positions
.. todo:: figure out mode 6
.. todo:: figure out MS8_CS24 C component

Note that MS8 and MS8_C* modes cannot be used with surfaces that have 16-byte
element size due to a hardware limitation. Also, multisampling is only
possible with tiled surfaces.

.. todo:: check MS8/128bpp on NVC0.

The sample ids are, for full samples, the values appearing in the sampleid
register. The numbers in () are the geometric coordinates of the sample
inside a pixel, as used by the rasterization process. The dimensions in []
are dimensions of a block represents a pixel in the surface - if it's 4×2,
each pixel is represented in the surface as a block 4 elements wide and 2
elements tall. The numbers in [] after each full sample are the coordinates
inside this block.

Each coverage sample "belongs to" several full samples. For every such pair
of coverage sample and full sample, the C component contains a bit that tells
if the coverage sample's value is the same as the full one's, ie. if the last
rendered primitive that covered the full sample also covered the coverage
sample. When the surface is resolved, each sample will "contribute" to exactly
one full sample. The full samples always contribute to themselves, while
coverage sample will contribute to the first full sample that they belong to,
in order listed above, that has the relevant bit set in C component of the
zeta surface. If none of the C bits for a given coverage sample are set, the
sample will default to contributing to the first sample in its belongs list.
Then, for each full sample, the number of samples contributing to it is
counted, and used as its weight when performing the downsample calculation.

Note that, while the belongs list orderings are carefully chosen based on
sample locations and to even the weights, the bits in C component don't use
this ordering and are sorted by sample id instead.

The C component is 16 or 32 bits per pixel, depending on the format. It is
then split into 8-bit chunks, starting from LSB, and each chunk is assigned
to one of the full samples. For MS4_CS4 and MS8_CS8, only samples in the top
line of each block get a chunk assigned, for MS4_CS12 all samples get a chunk.
The chunks are assigned to samples ordered first by x coordinate of the
sample, then by its y coordinate.


Surface formats
===============

A surface's format determines the type of information it stores in its
elements, the element size, and the element layout. Not all binding points
care about the format - m2mf and PCOPY treat all surfaces as arrays of bytes.
Also, format specification differs a lot between the binding points that make
use of it - 2d engine and render targets use a big enum of valid formats,
with values specifying both the layout and components, while texture
units decouple layout specification from component assignment and type
selection, allowing arbitrary swizzles.

There are 3 main enums used for specifying surface formats:

- texture format: used for textures, epecifies element size and layout, but
  not the component assignments nor type
- color format: used for color RTs and the 2d engine, specifies the full
  format
- zeta format: used for zeta RTs, specifies the full format, except the
  specific coverage sampling mode, if applicable

The surface formats can be broadly divided into the following categories:

- simple color formats: elements correspond directly to samples. Each element
  has 1 to 4 bitfields corresponding to R, G, B, A components. Usable for
  texturing, color RTs, and 2d engine.
- shared exponent color format: like above, but the components are floats
  sharing the exponent bitfield. Usable for texturing only.
- YUV color formats: element corresponds to two pixels lying in the same
  horizontal line. The pixels have three components, conventionally labeled
  as Y, U, V. U and V components are common for the two pixels making up an
  element, but Y components are separate. Usable for texturing only.
- zeta formats: elements correspond to samples. There is a per-sample depth
  component, optionally a per-sample stencil component, and optionally a
  per-pixel coverage value for CSAA surfaces. Usable for texturing and ZETA
  RT.
- compressed texture formats: elements correspond to blocks of samples, and
  are decoded to RGBA color values on the fly. Can be used only for
  texturing.
- bitmap texture format: each element corresponds to 8x8 block of samples,
  with 1 bit per sample. Has to be used with a special texture sampler.
  Usable for texturing and 2d engine.

.. todo:: wtf is color format 0x1d?


Simple color surface formats
----------------------------

A simple color surface is a surface where each element corresponds directly to
a sample, each sample has 4 components known as R, G, B, A [in that order], and
the bitfields in element correspond directly to components. There can be less
bitfields than components - the remaining components will be ignored on write,
and get a default value on read, which is 0 for R, G, B and 1 for A.

When bound to texture unit, the simple color formats are specified in three
parts. First, the format is specified, which is an enumerated value shared
with other format types. This format specifies the format type and, for simple
color formats, element size, and location of bitfields inside the element.
Then, the type [float/sint/uint/unorm/snorm] of each element component is
specified. Finally, a swizzle is specified: each of the 4 component outputs
[R, G, B, A] from the texture unit can be mapped to any of the components
present in the element [called C0-C3], constant 0, integer constant 1, or
float constant 1.

Thanks to the swizzle capability, there's no need to support multiple
orderings in the format itself, and all simple color texture formats have
C0 bitfield starting at LSB of the first byte, C1 [if present] at the first
bit after C0, and so on. Thus it's enough to specify bitfield lengths to
uniquely identify a texture type: for example 5_5_6 is a format with 3
components and element size of 2 bytes, C0 at bits 0-4, C1 at bits 5-9,
and C2 at bits 10-15. The element is always treated as a little-endian word
of the proper size, and bitfields are listed from LSB side. Also, in some
cases the texture format has bitfields used only for padding, and not usable
as components: these will be listed in the name as X<size>. For example,
32_8_X24 is a format with element size of 8 bytes, where bits 0-31 are C0,
32-39 are C1, and 40-63 are unusable.
[XXX: what exactly happens to element layout in big-endian mode?]

However, when bound to RTs or the 2d engine, all of the format, including
element size, element layout, component types, component assignment, and SRGB
flag, is specified by a single enumerated value. These formats have
a many-to-one relationship to texture formats, and are listed here below the
corresponding one. The information listed here for a format is C0-C3
assignments to actual components and component type, plus SRGB flag where
applicable. The components can be R, G, B, A, representing a bitfield
corresponding directly to a single component, X representing an unused
bitfield, or Y representing a bitfield copied to all components on read,
and filled with the R value on write.

The formats are:

Element size 16:

- texture format 0x01: 32_32_32_32

  - color format 0xc0: RGBA, float
  - color format 0xc1: RGBA, sint
  - color format 0xc2: RGBA, uint
  - color format 0xc3: RGBX, float
  - color format 0xc4: RGBX, sint
  - color format 0xc5: RGBX, uint

Element size 8:

- texture format 0x03: 16_16_16_16

  - color format 0xc6: RGBA, unorm
  - color format 0xc7: RGBA, snorm
  - color format 0xc8: RGBA, sint
  - color format 0xc9: RGBA, uint
  - color format 0xca: RGBA, float
  - color format 0xce: RGBX, float

- texture format 0x04: 32_32

  - color format 0xcb: RG, float
  - color format 0xcc: RG, sint
  - color format 0xcd: RG, uint

- texture format 0x05: 32_8_X24

Element size 4:

- texture format 0x07: 8_8_8_X8

.. todo:: htf do I determine if a surface format counts as 0x07 or 0x08?

- texture format 0x08: 8_8_8_8

  - color format 0xcf: BGRA, unorm
  - color format 0xd0: BGRA, unorm, SRGB
  - color format 0xd5: RGBA, unorm
  - color format 0xd6: RGBA, unorm, SRGB
  - color format 0xd7: RGBA, snorm
  - color format 0xd8: RGBA, sint
  - color format 0xd9: RGBA, uint
  - color format 0xe6: BGRX, unorm
  - color format 0xe7: BGRX, unorm, SRGB
  - color format 0xf9: RGBX, unorm
  - color format 0xfa: RGBX, unorm, SRGB
  - color format 0xfd: BGRX, unorm [XXX]
  - color format 0xfe: BGRX, unorm [XXX]

- texture format 0x09: 10_10_10_2

  - color format 0xd1: RGBA, unorm
  - color format 0xd2: RGBA, uint
  - color format 0xdf: BGRA, unorm

- texture format 0x0c: 16_16

  - color format 0xda: RG, unorm
  - color format 0xdb: RG, snorm
  - color format 0xdc: RG, sint
  - color format 0xdd: RG, uint
  - color format 0xde: RG, float

- texture format 0x0d: 24_8
- texture format 0x0e: 8_24
- texture format 0x0f: 32

  - color format 0xe3: R, sint
  - color format 0xe4: R, uint
  - color format 0xe5: R, float
  - color format 0xff: Y, uint [XXX]

- texture format 0x21: 11_11_10

  - color format 0xe0: RGB, float

Element size 2:

- texture format 0x12: 4_4_4_4
- texture format 0x13: 1_5_5_5
- texture format 0x14: 5_5_5_1

  - color format 0xe9: BGRA, unorm
  - color format 0xf8: BGRX, unorm
  - color format 0xfb: BGRX, unorm [XXX]
  - color format 0xfc: BGRX, unorm [XXX]

- texture format 0x15: 5_6_5

  - color format 0xe8: BGR, unorm

- texture format 0x16: 5_5_6
- texture format 0x18: 8_8

  - color format 0xea: RG, unorm
  - color format 0xeb: RG, snorm
  - color format 0xec: RG, uint
  - color format 0xed: RG, sint

- texture format 0x1b: 16

  - color format 0xee: R, unorm
  - color format 0xef: R, snorm
  - color format 0xf0: R, sint
  - color format 0xf1: R, uint
  - color format 0xf2: R, float

Element size 1:

- texture format 0x1d: 8

  - color format 0xf3: R, unorm
  - color format 0xf4: R, snorm
  - color format 0xf5: R, sint
  - color format 0xf6: R, uint
  - color format 0xf7: A, unorm

- texture format 0x1e: 4_4

.. todo:: which component types are valid for a given bitfield size?
.. todo:: clarify float encoding for weird sizes


Shared exponent color format
----------------------------

A shared exponent color format is like a simple color format, but there's
an extra bitfield, called E, that's used as a shared exponent for C0-C2. The
remaining three bitfields correspond to the mantissas of C0-C2, respectively.
They can be swizzled arbitrarily, but they have to use the float type.

Element size 4:

- texture format 0x20: 9_9_9_E5


YUV color formats
-----------------

These formats are also similar to color formats. However, The components are
conventionally called Y, U, V: C0 is known as U, C1 is known as Y, and C2 is
known as V. An element represents two pixels, and has 4 bitfields: YA
representing Y value for first pixel, YB representing Y value for second
pixel, U representing U value for both pixels, and V representing V value
of both pixels. There are two YUV formats, differing in bitfield order:

Element size 4:

- texture format 0x21: U8_YA8_V8_YB8
- texture format 0x22: YA8_U8_YB8_V8

.. todo:: verify I haven't screwed up the ordering here


Zeta surface format
-------------------

A zeta surface, like a simple color surface, has one element per sample.
It contains up to three components: the depth component [called Z], optionally
the stencil component [called S], and if coverage sampling is in use, the
coverage component [called C].

The Z component can be a 32-bit float, a 24-bit normalized unsinged integer, or
[on NVA0+] a 16-bit normalized unsigned integer. The S component, if present,
is always an 8-bit raw integer.

The C component is special: if present, it's an 8-bit bitfield in each sample.
However, semantically it is a per-pixel value, and the values of the samples'
C components are stitched together to obtain a per-pixel value. This stitching
process depends on the multisample mode, thus it needs to be specified to
bind a coverage sampled zeta surface as a texture. It's not allowed to use
a coverage sampling mode with a zeta format without C component, or the other
way around.

Like with color formats, there are two different enums that specify zeta
formats: texture formats and zeta formats. However, this time the zeta formats
have one-to-many relationship with texture formats: Texture format contains
information about the specific coverage sampling mode used, while zeta format
merely says whether coverage sampling is in use, and the mode is taken from
RT multisample configuration.

For textures, Z corresponds to C0, S to C1, and C to C2. However, C cannot
be used together with Z and/or S in a single sampler. Z and S sampling works
normally, but when C is sampled, the sampler returns preprocessed weights
instead of the raw value - see graph/nv50-texture.txt for more information
about the sampling process.

The formats are:

Element size 2:

- zeta format 0x13: Z16 [NVA0+ only]

  - texture format 0x3a: Z16 [NVA0+ only]

Element size 4:

- zeta format 0x0a: Z32

  - texture format 0x2f

- zeta format 0x14: S8_Z24

  - texture format 0x29

- zeta format 0x15: Z24_X8

  - texture format 0x2b

- zeta format 0x16: Z24_S8

  - texture format 0x2a

- zeta format 0x18: Z24_C8

  - texture format 0x2c: MS4_CS4
  - texture format 0x2d: MS8_CS8
  - texture format 0x2e: MS4_CS12

Element size 8:

- zeta format 0x19: Z32_S8_X24

  - texture format 0x30

- zeta format 0x1d: Z24_X8_S8_C8_X16

  - texture format 0x31: MS4_CS4
  - texture format 0x32: MS8_CS8
  - texture format 0x37: MS4_CS12

- zeta format 0x1e: Z32_X8_C8_X16

  - texture format 0x33: MS4_CS4
  - texture format 0x34: MS8_CS8
  - texture format 0x38: MS4_CS12

- zeta format 0x1f: Z32_S8_C8_X16

  - texture format 0x35: MS4_CS4
  - texture format 0x36: MS8_CS8
  - texture format 0x39: MS4_CS12

.. todo:: figure out the MS8_CS24 formats


Compressed texture formats
--------------------------

.. todo:: write me


Bitmap surface format
---------------------

A bitmap surface has only one component, and the component has 1 bit per sample
- that is, the component's value can be either 0 or 1 for each sample in the
surface. The surface is made of 8-byte elements, with each element
representing 8×8 block of samples. The element is treated as a 64-bit word,
with each sample taking 1 bit. The bits start from LSB and are ordered first
by x coordinate of the sample, then by its y coordinate.

This format can be used for 2d engine and texturing. When used for texturing,
it forces using a special "box" filter: result of sampling is a percentage of
"lit" area in WxH rectangle centered on the sampled location. See
graph/nv50-texture.txt for more details.

.. todo:: figure out more. Check how it works with 2d engine.

The formats are:

Element size 8:

- texture format 0x1f: BITMAP

  - color format 0x1c: BITMAP


NV50 storage types
==================

On nv50, the storage type is made of two parts: the storage type itself, and
the compression mode. The storage type is a 7-bit enum, the compression mode
is a 2-bit enum.

The compression modes are:

- 0: NONE - no compression
- 1: SINGLE - 2 compression tag bits per roptile, 1 tag cell per 64kB page
- 2: DOUBLE - 4 compression tag bits per roptile, 2 tag cells per 64kB page

.. todo:: verify somehow.

The set of valid compression modes varies with the storage type. NONE is
always valid.

As mentioned before, the low-level rearrangement is further split into two
sublevels: short range reordering, rearranging bytes in a single roptile,
and long range reordering, rearranging roptiles. Short range reordering
is performed for both VRAM and system RAM, and is highly dependent on the
storage type. Long range reordering is done only for VRAM, and has only three
types:

- none [NONE] - no reordering, only used for storage type 0 [linear]
- small scale [SSR] - roptiles rearranged inside a single 4kB page, used for
  non-0 storage types
- large scale [LSR] - large blocks of memory rearranged, based on internal
  VRAM geometry. Bounduaries between VRAM areas using NONE/SSR and LSR need
  to be properly aligned in physical space to prevent conflicts.

Long range reordering is described in detail in :ref:`nv50-vram`.

The storage types can be roughly split into the following groups:

- linear storage type: used for linear surfaces and non-surface buffers
- tiled color storage types: used for non-zeta tiled surfaces
- zeta storage types: used for zeta surfaces

On the original nv50, non-0 storage types can only be used on VRAM, on NV84
and later cards they can also be used on system RAM. Compression modes other
than NONE can only be used on VRAM. However, due to the nv50 limitation, tiled
surfaces stored in system RAM are allowed to use storage type 0, and will work
correctly for texturing and m2mf source/destination - rendering to them with
2d or 3d engine is impossible, though.

Correct storage types are only enforced by texture units and ROPs [ie. 2d and
3d engine render targets + CUDA global/local/stack spaces], which have
dedicated paths to memory and depend on the storage types for performance. The
other engines have storage type handling done by the common memory controller
logic, and will accept any storage type.

The linear storage type is:

storage type 0x00: LINEAR
  long range reordering: NONE
  valid compression modes: NONE
  There's no short range reordering on this storage type - the offset inside
  a roptile is identical between the virtual and physical addresses.


Tiled color storage types
-------------------------

.. todo:: reformat

The following tiled color storage types exist:

storage type 0x70: TILED
  long range reordering: SSR
  valid compression modes: NONE
  valid surface formats: any non-zeta with element size of 1, 2, 4, or 8 bytes
  valid multisampling modes: any
storage type 0x72: TILED_LSR
  long range reordering: LSR
  valid compression modes: NONE
  valid surface formats: any non-zeta with element size of 1, 2, 4, or 8 bytes
  valid multisampling modes: any
storage type 0x76: TILED_128_LSR
  long range reordering: LSR
  valid compression modes: NONE
  valid surface formats: any non-zeta with element size of 16 bytes
  valid multisampling modes: any

[XXX]

storage type 0x74: TILED_128
  long range reordering: SSR
  valid compression modes: NONE
  valid surface formats: any non-zeta with element size of 16 bytes
  valid multisampling modes: any

[XXX]

storage type 0x78: TILED_32_MS4
  long range reordering: SSR
  valid compression modes: NONE, SINGLE
  valid surface formats: any non-zeta with element size of 4 bytes
  valid multisampling modes: MS1, MS2*, MS4*
storage type 0x79: TILED_32_MS8
  long range reordering: SSR
  valid compression modes: NONE, SINGLE
  valid surface formats: any non-zeta with element size of 4 bytes
  valid multisampling modes: MS8*
storage type 0x7a: TILED_32_MS4_LSR
  long range reordering: LSR
  valid compression modes: NONE, SINGLE
  valid surface formats: any non-zeta with element size of 4 bytes
  valid multisampling modes: MS1, MS2*, MS4*
storage type 0x7b: TILED_32_MS8_LSR
  long range reordering: LSR
  valid compression modes: NONE, SINGLE
  valid surface formats: any non-zeta with element size of 4 bytes
  valid multisampling modes: MS8*

[XXX]

storage type 0x7c: TILED_64_MS4
  long range reordering: SSR
  valid compression modes: NONE, SINGLE
  valid surface formats: any non-zeta with element size of 8 bytes
  valid multisampling modes: MS1, MS2*, MS4*
storage type 0x7d: TILED_64_MS8
  long range reordering: SSR
  valid compression modes: NONE, SINGLE
  valid surface formats: any non-zeta with element size of 8 bytes
  valid multisampling modes: MS8*

[XXX]

storage type 0x44: TILED_24
  long range reordering: SSR
  valid compression modes: NONE
  valid surface formats: texture format 8_8_8_X8 and corresponding color formats
  valid multisampling modes: any
storage type 0x45: TILED_24_MS4
  long range reordering: SSR
  valid compression modes: NONE, SINGLE
  valid surface formats: texture format 8_8_8_X8 and corresponding color formats
  valid multisampling modes: MS1, MS2*, MS4*
storage type 0x46: TILED_24_MS8
  long range reordering: SSR
  valid compression modes: NONE, SINGLE
  valid surface formats: texture format 8_8_8_X8 and corresponding color formats
  valid multisampling modes: MS8*
storage type 0x4b: TILED_24_LSR
  long range reordering: LSR
  valid compression modes: NONE
  valid surface formats: texture format 8_8_8_X8 and corresponding color formats
  valid multisampling modes: any
storage type 0x4c: TILED_24_MS4_LSR
  long range reordering: LSR
  valid compression modes: NONE, SINGLE
  valid surface formats: texture format 8_8_8_X8 and corresponding color formats
  valid multisampling modes: MS1, MS2*, MS4*
storage type 0x4d: TILED_24_MS8_LSR
  long range reordering: LSR
  valid compression modes: NONE, SINGLE
  valid surface formats: texture format 8_8_8_X8 and corresponding color formats
  valid multisampling modes: MS8*

[XXX]


Zeta storage types
------------------

.. todo:: write me


NVC0 storage types
==================

.. todo:: write me
