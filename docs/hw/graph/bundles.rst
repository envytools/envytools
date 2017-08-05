.. _pgraph-bundles:

================
Pipeline Bundles
================

.. contents::


Introduction
============

By its nature, every stage of the graphics pipeline processes a different kind
of data -- the format of packets sent between pipeline units varies greatly.
However, there is a kind of data that is supported on most unit
interconnections: pipeline bundles.  Bundles are used for data that needs to
be passed unchanged through many stages of the pipeline -- most of them
directly from the FE.  Every unit in the pipeline will only recognize and
act on a small subset of the bundles, and pass through all other bundles.

Bundles have first appeared on Celsius, where they consist of a 6-bit bundle
type and 32-bit bundle data.  On Kelvin, the bundle type space has been
reorganized and extended to 9 bits.  On Tesla, bundle types have been
reorganized again and extended to 16 bits.

Most bundles are so-called "state bundles" -- their purpose is to pass
pipeline configuration data from the FE to all interested pipeline units.
The pipeline units that need to know a particular piece of configuration
data will watch for the corresponding state bundle, updating its internal
configuration registers when such a bundle passes through.  In some cases,
units will recognize that no further units in the pipeline need a given
state bundle and won't pass it any further, but usually state bundles
travel unchanged from the FE right until the ROPs.

Before Tesla, state bundles usually contained packed state -- many pieces
of configuration affecting related units were collected into a single bundle.
The FE thus keeps a copy of the last value sent for every state bundle, which
is visible through MMIO.  Whenever a method is processed that changes
a piece of configuration, the relevant bits in the correponding state bundle
shadow register are updated, and the entire bundle is resubmitted through
the pipeline.  The shadow registers are also used for context-switching --
to save pipeline configuration, it's enough to just dump the shadow registers.
On restore, writing the shadow registers will automatically submit the given
bundle down the pipeline, thus restoring the state of every unit involved.

Since Tesla, state bundles usually correspond directly to class methods, and
the FE doesn't need to keep track of most of them (though some are tracked
in shadow registers for pre-launch state validation purposes).  Instead,
state bundles are context-switched by saving and restoring their copies
kept on every involved pipeline unit.

Other bundles are used to trigger some kind of action in a pipeline unit
that is different from the main mode of operation (ie. rendering primitives):
buffer clears, queries, and so on.  These are called trigger bundles.


Celsius/Kelvin/Rankine/Curie bundles
====================================

======== ========= ============= ========= ========= ===============
Celsius  Kelvin    Rankine/Curie Type      Used by   Name
======== ========= ============= ========= ========= ===============
\-       100[20]   000[20]       state-ish RASTER?   POLYGON_STIPPLE
14       020[8]    020[8]        state     RC?       RC_FACTOR_A
15       028[8]    028[8]        state     RC?       RC_FACTOR_B
10[2]    030[8]    030[8]        state     RC?       RC_IN_ALPHA
16[2]    038[8]    038[8]        state     RC?       RC_OUT_ALPHA
12[2]    040[8]    040[8]        state     RC?       RC_IN_COLOR
18[2]    048[8]    048[8]        state     RC?       RC_OUT_COLOR
\-       050       050           state     RC?       RC_CONFIG
1a       051       051           state     RC?       RC_FINAL_A
1b       052       052           state     RC?       RC_FINAL_B
1c       053       053           state     ROP?      CONFIG_A
1d       054       054           state     ROP?      STENCIL_A
1e       055       055           state     ROP?      STENCIL_B
1f       056       056           state     ASSM,ROP? CONFIG_B
\-       \-        057           state     RASTER?   VIEWPORT_OFFSET
\-       \-        058           state     SHADER?   PS_OFFSET
35*      059*      059           state     ZCULL     CLIPID_ID
31*      05a*      05a           state     ZCULL     CLIPID_BASE
32*      05b*      05b           state     ZCULL     CLIPID_LIMIT
33*      05c*      05c           state     ZCULL     CLIPID_OFFSET
34*      05d*      05d           state     ZCULL     CLIPID_PITCH
\-       05e*      05e           state     RASTER?   LINE_STIPPLE
\-       05f?      05f           state     ROP?      RT_ENABLE
23       060       060           state     RC?       FOG_COLOR
\-       061[2]    061[2]        state     ????      FOG_COEFF
2a       063       063           state     ASSM      POINT_SIZE
22       064       064           state     RASTER?   RASTER
\-       065       065           state     SHADER?   TEX_SHADER_CULL_MODE
\-       066       066           state     SHADER?   TEX_SHADER_MISC
\-       067       067           state     SHADER?   TEX_SHADER_OP
\-       068       068           state     ???       FENCE_OFFSET
\-       069       \-            state     TEX?      TEX_ZCOMP
\-       \-        069           state     ????      ????
\-       06a       06a           state     ????      UNK1E68
\-       06b[2]    06b[2]        state     RC?       RC_FINAL_FACTOR
\-       06d[2]    06d[2]        state     RASTER?   CLIP_HV
\-       000       06f           state     ROP?      MULTISAMPLE
\-       003[3]    070[3]        state     SHADER?   TEX_UNK10
\-       006[3]    073[3]        state     SHADER?   TEX_UNK11
\-       009[3]    076[3]        state     SHADER?   TEX_UNK13
\-       00c[3]    079[3]        state     SHADER?   TEX_UNK12
\-       00f[3]    07c[3]        state     SHADER?   TEX_UNK15
\-       012[3]    07f[3]        state     SHADER?   TEX_UNK14
20       001       082           state     ROP?      BLEND
21       002       083           state     ROP?      BLEND_COLOR
2b[2]    019[2]    084[2]        state     RASTER?   CLEAR_HV
\-       01b       086           state     RASTER?   CLEAR_COLOR
\-       \-        087           state     ROP?      STENCIL_C
\-       \-        088           state     ROP?      STENCIL_D
\-       \-        089           state     RASTER?   CLIP_PLANE_ENABLE
\-       \-        08b[2]        state     RASTER?   VIEWPORT_HV
\-       \-        08d[2]        state     RASTER?   SCISSOR_HV
\-       091[8]    091[8]        state     RASTER?   CLIP_RECT_HORIZ
\-       099[8]    099[8]        state     RASTER?   CLIP_RECT_VERT
36       0a1       0a1           state     ZCULL?    Z_CONFIG
37       0a2       0a2           state     ZCULL?    CLEAR_ZETA
38       \-        \-            state     ZCULL?    UNK3FC
27       0a3       0a3           state     RASTER?   DEPTH_RANGE_FAR
26       0a4       0a4           state     RASTER?   DEPTH_RANGE_NEAR
\-       0a5[2]    0a5[2]        state     TEX?      DMA_TEX
\-       0a7[2]    0a7[2]        state     IDX       DMA_VTX
25       0a9       0a9           state     RASTER?   POLYGON_OFFSET_UNITS
24       0aa       0aa           state     RASTER?   POLYGON_OFFSET_FACTOR
\-       0ab[3]    0ab[3]        state     SHADER?   TEX_SHADER_CONST_EYE
\-       0ae*      \-            state     ????      ????
\-       \-        0af           state     ????      RANKINE_UNK0A40
2d*      0b0*      0b0           state     ZCULL     ZCULL_BASE
2e*      0b1*      0b1           state     ZCULL     ZCULL_LIMIT
2f*      0b2*      0b2           state     ZCULL     ZCULL_OFFSET
30*      0b3*      0b3           state     ZCULL     ZCULL_PITCH
\-       0b4[4]*   0b4[4]        state     ????      KELVIN_UNK1DC0
\-       0b8*      0b8           state     ????      KELVIN_UNK1DBC
\-       \-        0b9           state     IDX       PRIMITIVE_RESTART_ENABLE
\-       \-        0ba           state     IDX       PRIMITIVE_RESTART_INDEX
\-       \-        0bb           state     RASTER?   TXC_CYLWRAP
\-       \-        0bc[8]        state-ish SHADER?   PS_PREFETCH_DATA
\-       \-        0c4           state     SHADER?   PS_CONTROL
\-       \-        0c5           state     RASTER?   TXC_ENABLE
\-       \-        0c6           state?    ????      ???? apparently involved in clears
\-       \-        0c7           state     RASTER?   WINDOW_OFFSET
00[2]    089[4]    100[10]       state     TEX?      TEX_OFFSET
04[2]    081[4]    110[10]       state     TEX?      TEX_FORMAT
\-       06f[4]    120[10]       state     TEX?      TEX_WRAP
06[2]    073[4]    130[10]       state     TEX?      TEX_CONTROL
08[2]    077[4]    140[10]       state     TEX?      TEX_PITCH
0a[2]    07b[2]    \-            state     TEX?      TEX_UNK238
0e[2]    07d[4]    150[10]       state     TEX?      TEX_FILTER
0c[2]    085[4]    160[10]       state     TEX?      TEX_RECT
\-       003[4]    170[10]       state     TEX?      TEX_BORDER_COLOR
02[2]    08d[4]    180[10]       state     TEX?      TEX_PALETTE
28[2]    01c[4]    190[10]       state     TEX?      TEX_COLOR_KEY
\-       \-        1dc           trigger?  ????      ???? apparently involved in clears
\-       \-        1f7           trigger?  ????      UNKA08
\-       \-        1f8           trigger   IDX       PS_PREFETCH_TRIGGER
3f*      1f9*      1f9           trigger   ZCULL     INVALIDATE_ZCULL
\-       1fb       1fb           trigger   ?         FENCE_WRITE_B
\-       1fc       1fc           trigger   ROP?      ZPASS_COUNTER_READ
\-       1fd       1fd           trigger   ROP?      ZPASS_COUNTER_RESET
3e*      1fe*      1fe           trigger   ZCULL     CLEAR_CLIPID_TRIGGER
3d*      ?         ?             trigger   ZCULL     CLEAR_ZCULL_TRIGGER
======== ========= ============= ========= ========= ===============


Texture bundles
===============

.. todo:: write me

TEX_OFFSET: A simple 32-bit texture offset.  Should be aligned to 0x80 bytes.

TEX_FORMAT [NV10:NV20]:

  - bit 1: DMA

    - 0: A
    - 1: B

  - bit 2: CUBE_MAP
  - bit 3: CELSIUS_MTHD_TEX_UNK258 [NV17:NV20]
  - bit 4: ORIGIN_ZOH

    - 0: CENTER
    - 1: CORNER

  - bit 6: ORIGIN_FOH
  - bits 7-11: FORMAT
  - bits 12-15: MIPS - number of mipmap levels
  - bits 16-19: SIZE_S - log2 of texture width, if not RECT
  - bits 20-23: SIZE_T - log2 of texture height, if not RECT
  - bits 24-26: WRAP_S
  - bit 27: WRAP_S_CYL
  - bits 28-32: WRAP_T
  - bit 31: WRAP_T_CYL

On NV20, WRAP_* have been moved to a new TEX_WRAP bundle.

TEX_FORMAT [NV20:]:

  - bit 1: DMA

    - 0: A
    - 1: B

  - bit 2: CUBE_MAP
  - bit 3: BORDER_TYPE [NV20:]

    - 0: INCLUDED
    - 1: CONST

  - bit 4: ORIGIN_ZOH [NV20:NV30]
  - bit 5: ORIGIN_FOH [NV20:NV30]
  - bits 6-7: MODE [NV20:NV30]

    - 1: 1D
    - 2: 2D [also used for CUBE]
    - 3: 3D

  - bits 8-14: FORMAT
  - bits 16-19: MIPS - number of mipmap levels [NV20:]
  - bits 20-23: SIZE_S - log2 of texture width, if not RECT
  - bits 24-27: SIZE_T - log2 of texture height, if not RECT
  - bits 28-31: SIZE_R - log2 of texture depth, if 3D

FORMAT can be one of:

- 0x00: ???
- 0x01: ???
- 0x02: ???
- 0x03: ???
- 0x04: ???
- 0x05: ???
- 0x06: ???
- 0x07: ???
- 0x08: ??? [:NV30]
- 0x09: ??? [:NV30]
- 0x0a: ??? [:NV30]
- 0x0b: ???
- 0x0c: ???_DXT
- 0x0e: ???_DXT
- 0x0f: ???_DXT
- 0x10: ???_RECT
- 0x11: ???_RECT
- 0x12: ???_RECT
- 0x13: ???_RECT
- 0x14: ???_RECT
- 0x15: ???_RECT
- 0x16: ???_RECT
- 0x17: ???_RECT
- 0x18: ???_RECT
- 0x19: ???_RECT [NV17:]
- 0x1a: ???_RECT [NV17:]
- 0x1b: ???_RECT [NV17:]
- 0x1c: ???_RECT [NV17:]
- 0x19: ??? [NV20:]
- 0x1a: ??? [NV20:]
- 0x1b: ???_RECT [NV20:]
- 0x1c: ???_RECT [NV20:]
- 0x1d: ???_RECT [NV20:]
- 0x1e: ???_RECT [NV20:]
- 0x1f: ???_RECT [NV20:]
- 0x20: ???_RECT [NV20:]
- 0x24: ???_RECT_DXT [NV20:]
- 0x25: ???_RECT_DXT [NV20:]
- 0x26: ???_RECT [NV20:]
- 0x27: ??? [NV20:]
- 0x28: ??? [NV20:]
- 0x29: ??? [NV20:]
- 0x2a: ???_ZCOMP [NV20:]
- 0x2b: ???_ZCOMP [NV20:]
- 0x2c: ???_ZCOMP [NV20:]
- 0x2d: ???_ZCOMP [NV20:]
- 0x2e: ???_RECT_ZCOMP [NV20:]
- 0x2f: ???_RECT_ZCOMP [NV20:]
- 0x30: ???_RECT_ZCOMP [NV20:]
- 0x31: ???_RECT_ZCOMP [NV20:]
- 0x32: ??? [NV20:]
- 0x33: ??? [NV20:]
- 0x34: ???_RECT_DXT [NV20:]
- 0x35: ???_RECT [NV20:]
- 0x36: ???_RECT [NV20:]
- 0x37: ???_RECT [NV20:]
- 0x38: ??? [NV20:]
- 0x39: ??? [NV20:]
- 0x3a: ??? [NV20:]
- 0x3b: ??? [NV20:]
- 0x3c: ??? [NV20:]
- 0x3d: ???_RECT [NV20:]
- 0x3e: ???_RECT [NV20:]
- 0x3f: ???_RECT [NV20:]
- 0x40: ???_RECT [NV20:]
- 0x41: ???_RECT [NV20:]
- 0x42: ??? [NV25:]
- 0x43: ???_RECT [NV25:]
- 0x44: ??? [NV25:]
- 0x45: ??? [NV25:]
- 0x46: ???_RECT [NV25:]
- 0x47: ???_RECT [NV25:]
- 0x48: ???_RECT [NV25:]
- 0x49: ??? [NV25:]
- 0x4a: ???_RECT [NV30:]
- 0x4b: ???_RECT [NV30:]
- 0x4c: ???_RECT [NV30:]
- 0x4d: ???_RECT [NV30:]
- 0x4e: ??? [NV30:]

TEX_WRAP [NV20:]:

- bits 0-2: WRAP_S
- bit 4: WRAP_S_CYL [NV20:NV30]
- bits 4-7: ANISO_MIP_FILTER_OPTIMIZATION? [NV30:]
- bits 8-10: WRAP_T
- bit 12: WRAP_T_CYL [NV20:NV30]
- bit 12: EXPAND_NORMAL [NV30:]
- bits 13-14: RANKINE_TEX_WRAP_UNK24 [NV30:]

  - 0: ???
  - 1: ???
  - 2: ???

- bits 16-18: WRAP_R
- bits 19-23: FILTER_OPT_TRILINEAR [NV30:]
- bits 24-27: GAMMA_DECREASE_FILTER? [NV30:]
- bits 28-31: ZCOMP [NV30:] -- on NV20, this was a separate bundle instead.
- bit 20: WRAP_R_CYL [NV20:NV30]
- bit 24: WRAP_Q_CYL [NV20:NV30]

On Rankine, WRAP_*_CYL have been moved to a new TXC_CYLWRAP bundle.

WRAP can be one of:

- 1: REPEAT
- 2: MIRRORED_REPEAT
- 3: CLAMP_TO_EDGE
- 4: CLAMP_TO_BORDER
- 5: CLAMP

TEX_CONTROL:

  - bit 0: COLOR_KEY_ENABLE?
  - bits 1-3: ???
  - bits 4-5: ANISOTROPY
  - bits 6-17: MAX_LOD, in 4.8 fixed-point format
  - bits 18-29: MIN_LOD, in 4.8 fixed-point format
  - bit 30: ENABLE - if set, this texture is active

TEX_PITCH:

  - bits 0-1: S1_W [NV30:]

    - 0: W
    - 1: Z
    - 2: Y
    - 3: X

  - bits 2-3: S1_Z [NV30:]
  - bits 4-5: S1_Y [NV30:]
  - bits 6-7: S1_X [NV30:]
  - bits 8-9: S0_W [NV30:]
  - bits 10-11: S0_Z [NV30:]
  - bits 12-13: S0_Y [NV30:]
  - bits 14-15: S0_X [NV30:]
  - bits 16-31: PITCH

TEX_UNK238 (on Kelvin, only applies for first 2 textures) [:NV30]:

  - bits 0-31: ???

TEX_FILTER:

  - bits 0-12: LOD_BIAS, signed number in 5.8 fixed-point format
  - bits 13-15: TEX_FILTER_UNK13 [NV20:]

   - 0: UNK0
   - 1: UNK1
   - 2: UNK2
   - 3: UNK3 [NV25:]

  - bits 16-21: MINIFY [NV20:]
  - bits 24-27: MAGNIFY [NV20:]
  - bit 28: SIGNED_B [NV20:]
  - bit 29: SIGNED_G [NV20:]
  - bit 30: SIGNED_R [NV20:]
  - bit 31: SIGNED_A [NV20:]
  - bits 24-26: MINIFY [:NV20]
  - bits 28-30: MAGNIFY [:NV20]


MINIFY can be one of:

- 1: NEAREST
- 2: LINEAR
- 3: NEAREST_MIPMAP_NEAREST
- 4: LINEAR_MIPMAP_NEAREST
- 5: NEAREST_MIPMAP_LINEAR
- 6: LINEAR_MIPMAP_LINEAR
- 7: ??? [NV20:]

And MAGNIFY can be:

- 1: NEAREST
- 2: LINEAR
- 4: ??? [NV20:]

TEX_RECT:

  - bits 0-10: WIDTH [:NV20]
  - bits 0-12: WIDTH [NV20:]
  - btis 16-26: HEIGHT [:NV20]
  - btis 16-28: HEIGHT [NV20:]

TEX_PALETTE:

  - bit 0: DMA

    - 0: A
    - 1: B

  - bits 2-3: ??? [NV20:]
  - bits 6-31: OFFSET >> 6

TEX_ZCOMP [NV20:NV25]:

  - bits 0-2: MODE -- common for all textures, same values as ALPHA_FUNC

TEX_ZCOMP [NV25:NV30]:

  - bits 0-2: TEX0_MODE
  - bits 3-5: TEX1_MODE
  - bits 6-8: TEX2_MODE
  - bits 9-11: TEX3_MODE

On NV30, this bundle is gone and ZCOMP mode is in TEX_WRAP instead.


Register combiner bundles
=========================

.. todo:: write me

- RC_FACTOR_A
- RC_FACTOR_B
- RC_IN_ALPHA
- RC_OUT_ALPHA
- RC_IN_COLOR
- RC_OUT_COLOR
- RC_CONFIG
- RC_FINAL_A
- RC_FINAL_B
- FOG_COLOR
- RC_FINAL_FACTOR


ROP bundles
===========

Note: CONFIG_A, STENCIL_A and STENCIL_B predate bundles -- they first
appeared on NV4 as plain MMIO registers.  These early versions are described
here as well.

CONFIG_A:

  - bits 0-7: ALPHA_REF
  - bits 8-11: ALPHA_FUNC

    On NV4:NV10, the values are:

    - 1: NEVER
    - 2: LESS
    - 3: EQUAL
    - 4: LEQUAL
    - 5: GREATER
    - 6: NOTEQUAL
    - 7: GEQUAL
    - 8: ALWAYS

    On NV10 and up, they are:

    - 0: NEVER
    - 1: LESS
    - 2: EQUAL
    - 3: LEQUAL
    - 4: GREATER
    - 5: NOTEQUAL
    - 6: GEQUAL
    - 7: ALWAYS

  - bit 12: ALPHA_FUNC_ENABLE
  - bit 14: DEPTH_TEST_ENABLE
  - bits 16-19: DEPTH_FUNC -- has same values as ALPHA_FUNC
  - bits 20-21: CULL_FACE [NV4:NV10]

    - 1: NONE
    - 2: FRONT
    - 3: BACK

    Since there is no FRONT_FACE setting on NV4, FRONT is always CW.
    This was moved to RASTER bundle on Celsius.

  - bit 22: DITHER_ENABLE
  - bit 23: DEPTH_PERSPECTIVE_ENABLE
  - bit 24: DEPTH_WRITE_ENABLE
  - bit 25: STENCIL_WRITE_ENABLE
  - bit 26: COLOR_MASK_A
  - bit 27: COLOR_MASK_R
  - bit 28: COLOR_MASK_G
  - bit 29: COLOR_MASK_B
  - bits 30-21: Z_FORMAT [NV4:NV10]

    - 1: FIXED
    - 2: FLOAT

    This was moved to RASTER bundle on Celsius.

  - bits 30-31: KELVIN_CONFIG_UNK28 [NV20:NV25]

    - 0: ???
    - 1: ???
    - 2: ???

    This was moved to CONFIG_B on NV25.

  - bit 31: CELSIUS_UNK3F8 [NV17:NV20, NV34]

STENCIL_A:

  - bit 0: STENCIL_ENABLE
  - bit 1: STENCIL_BACK_ENABLE [NV30:]
  - bits 4-7: STENCIL_FUNC -- has same values as ALPHA_FUNC
  - bits 8-15: STENCIL_FUNC_REF
  - bits 16-23: STENCIL_FUNC_MASK
  - bits 24-31: STENCIL_MASK

STENCIL_B:

  - bits 0-3: STENCIL_OP_FAIL

    - 1: KEEP
    - 2: ZERO
    - 3: REPLACE
    - 4: INCR
    - 5: DECR
    - 6: INVERT
    - 7: INCR_WRAP
    - 8: DECR_WRAP

  - bits 4-7: STENCIL_OP_ZFAIL
  - bits 8-11: STENCIL_OP_ZPASS
  - bits 12-15: ??? [NV34]

STENCIL_C [NV30:]:

  - bits 0-7: STENCIL_BACK_MASK
  - bits 8-11: STENCIL_BACK_OP_ZPASS
  - bits 12-15: STENCIL_BACK_OP_ZFAIL
  - bits 16-19: STENCIL_BACK_OP_FAIL

STENCIL_D [NV30:]:

  - bits 0-7: STENCIL_BACK_FUNC_REF
  - bits 8-15: STENCIL_BACK_FUNC_MASK
  - bits 16-19: STENCIL_BACK_FUNC

CONFIG_B:

  - bit 0: PROVOKING_VERTEX

    - 0: LAST
    - 1: FIRST

  - bit 1: POINT_SPRITE_ENABLE [NV25:]

    .. todo:: why is POINT_SMOOTH_ENABLE aliased here?

  - bit 2: CELSIUS_CONFIG_UNK24
  - bits 3-4: POINT_SPRITE_R_MODE [NV25:]

    - 0: ZERO
    - 1: R
    - 2: S

  - bit 4: ??? [NV10:NV20] -- no method appears to affect this bit
  - bit 5: SPECULAR_ENABLE -- this is also stored in XF_MODE.
  - bit 6: TEXTURE_PERSPECTIVE_ENABLE
  - bit 7: SHADE_MODE

    - 0: FLAT
    - 1: SMOOTH

  - bit 8: FOG_ENABLE -- this is also stored in XF_MODE.
  - bit 9: POINT_PARAMS_ENABLE -- this is also stored in XF_MODE.
  - bits 10-13: CELSIUS_CONFIG_UNK8 [NV10:NV30]

    - 0: ???
    - 1: ???

  - bits 14-15: CELSIUS_CONFIG_UNK28 [NV17:NV20]
  - bits 16-18: FOG_MODE [NV20:]

    - 0: LINEAR
    - 1: EXP
    - 3: EXP2
    - 4: UNK_0804
    - 5: UNK_0802
    - 7: UNK_0803

    The low bit of this is also stored in XF_MODE.
    On Celsius, fog mode was stored in FE3D_MISC instead.

  - bit 20: ZPASS_COUNTER_ENABLE [NV20:]
  - bits 24-27: POINT_SPRITE_COORD_REPLACE [NV25:]
  - bits 28-30: KELVIN_CONFIG_UNK28 [NV25:]

    - 0: ???
    - 1: ???
    - 2: ???
    - 3: ???

    This was moved from CONFIG_A.

  - bit 31: KELVIN_UNKA0C [NV25:]

BLEND:

  - bits 0-2: BLEND_EQUATION

    - 0: SUBTRACT
    - 1: REVERSE_SUBTRACT
    - 2: ADD
    - 3: MIN
    - 4: MAX
    - 5: UNKF005 [NV20:]
    - 6: UNKF006 [NV20:]
    - 7: UNKF007 [NV25:]

  - bit 3: BLEND_FUNC_ENABLE
  - bits 4-7: BLEND_FACTOR_SRC_0

    - 0x0: ZERO
    - 0x1: ONE
    - 0x2: SRC_COLOR
    - 0x3: ONE_MINUS_SRC_COLOR
    - 0x4: SRC_ALPHA
    - 0x5: ONE_MINUS_SRC_ALPHA
    - 0x6: DST_ALPHA
    - 0x7: ONE_MINUS_DST_ALPHA
    - 0x8: DST_COLOR
    - 0x9: ONE_MINUS_DST_COLOR
    - 0xa: SRC_ALPHA_SATURATE
    - 0xc: CONSTANT_COLOR
    - 0xd: ONE_MINUS_CONSTANT_COLOR
    - 0xe: CONSTANT_ALPHA
    - 0xf: ONE_MINUS_CONSTANT_ALPHA

  - bits 8-11: BLEND_FACTOR_DST_0
  - bits 12-15: COLOR_LOGIC_OP_OP [NV15:]

    - 0x0: CLEAR
    - 0x1: AND
    - 0x2: AND_REVERSE
    - 0x3: COPY
    - 0x4: AND_INVERSE
    - 0x5: NOOP
    - 0x6: XOR
    - 0x7: OR
    - 0x8: NOR
    - 0x9: EQUIV
    - 0xa: INVERT
    - 0xb: OR_REVERSE
    - 0xc: COPY_INVERTED
    - 0xd: OR_INVERTED
    - 0xe: NAND
    - 0xf: SET

  - bit 16: COLOR_LOGIC_OP_ENABLE [NV15:]
  - bits 20-23: BLEND_FACTOR_SRC_1 [NV30:]
  - bits 24-27: BLEND_FACTOR_DST_1 [NV30:]

BLEND_COLOR:

  - bits 0-7: B
  - bits 8-15: G
  - bits 16-23: R
  - bits 24-31: A

MULTISAMPLE:

  - bit 0: MULTISAMPLE_ENABLE
  - bit 4: ALPHA_TO_COVERAGE
  - bit 8: ALPHA_TO_ONE
  - bits 16-31: SAMPLE_COVERAGE


RASTER bundles
==============

RASTER:

  - bits 0-1: POLYGON_MODE_FRONT

    - 0: FILL
    - 1: POINT
    - 2: LINE

  - bits 2-3: POLYGON_MODE_BACK
  - bit 4: POLYGON_STIPPLE_ENABLE [NV20:NV25]

    On NV25, this was moved to LINE_STIPPLE bundle.

  - bit 4: ??? [NV25:NV30]
  - bit 4: RANKINE_UNK1450_UNK31 [NV30:]
  - bit 5: DEPTH_CLAMP_UNK8 [NV20:]
  - bit 6: POLYGON_OFFSET_POINT_ENABLE
  - bit 7: POLYGON_OFFSET_LINE_ENABLE
  - bit 8: POLYGON_OFFSET_FILL_ENABLE
  - bit 9: POINT_SMOOTH_ENABLE [:NV30]
  - bit 10: LINE_SMOOTH_ENABLE
  - bit 11: POLYGON_SMOOTH_ENABLE
  - bits 12-20: LINE_WIDTH
  - bits 21-22: CULL_FACE

    - 1: FRONT
    - 2: BACK
    - 3: FRONT_AND_BACK

  - bit 23: FRONT_FACE

    - 0: CW
    - 1: CCW

  - bit 24: LIGHT_TWO_SIDE_ENABLE [NV20:] -- also stored in XF_MODE
  - bits 25-27: CELSIUS_MTHD_UNK3F0 [NV20:]

    - 0: UNK0
    - 1: UNK1
    - 2: UNK2
    - 3: UNK3
    - 4: UNK4
    - 7: UNK0F

  - bits 26-27: CELSIUS_MTHD_UNK3F0 [NV10:NV20]

    - 0: UNK0
    - 1: UNK1
    - 2: UNK2
    - 3: UNK3

  - bit 28: CULL_FACE_ENABLE
  - bit 29: Z_FORMAT

    - 0: FIXED
    - 1: FLOAT

  - bits 30-31: CELSIUS_MTHD_UNK3F8 [NV10:NV20]
  - bit 30: DEPTH_CLAMP_UNK0 [NV20:]
  - bit 31: CLIP_RECT_MODE [NV20:]

    - 0: ???
    - 1: ???

    Before NV20, this was stored in FE3D_MISC.

LINE_STIPPLE:

  - bit 0: POLYGON_STIPPLE_ENABLE
  - bit 1: LINE_STIPPLE_ENABLE
  - bits 8-15: LINE_STIPPLE_FACTOR
  - bits 16-31: LINE_STIPPLE_PATTERN


Misc bundles
============

POINT_SIZE:

  On NV10:NV25, this is a 9-bit fixed-point number -- 6 integer bits and 3
  fractional bits.  On NV25:, it is a float32.
