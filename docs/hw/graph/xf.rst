.. _pgraph-xf:

==========================================
XF: The vertex transform & lighting engine
==========================================

.. contents::


Introduction
============

XF is a PGRAPH unit responsible for processing vertices before they are sent
to the rasterizer.  It first appeared on NV10 -- before that, there was no
transform engine, and the user supplied raw vertex data directly to the
rasterizer.  On G80, it has been replaced with unified shader architecture.

The following versions of XF exist:

1. NV10: the original incarnation of XF.  It is accompanied by the lighting
   engine, LT.  Together, they perform fixed-function transform & lighting
   on incoming vertices.

   Vertices on input have the following attributes:

   - position (4×float32)
   - primary color (4×float32)
   - secondary color (3×float32)
   - fog coordinate (1×float32)
   - 2 sets of texture coordinates (2×4×float32)
   - normal vector (3×float32)
   - weight factor (1×float32)

   After transformation, they have the following attributes:

   - position (4×float32) - computed by XF
   - primary color (4×uint8) - computed by LT or passed through by XF
   - secondary color (3×uint8) - computed by LT or passed through by XF
   - fog coordinate (1×float21) - computed by LT
   - 2 sets of texture coordinates (2×3×float32) - computed by XF
   - point size (1×uint9) - computed by LT

2. NV20.  This version introduces support for user-defined vertex programs,
   which can be used instead of the fixed-function transforms.  If a user
   defined program is used, all fixed-function hardware is disabled, including
   all of LT.  To support user-defined programs, the input vertices can have
   16 arbitrarily used attributes.

   Other new functionality includes:

   - two-sided lighting is supported -- all lighting calculations can be
     performed twice, with different parameters, outputing two sets of
     primary and secondary colors.
   - 4 weights are supported, instead of 1.
   - 4 sets of output texture coordinates are supported, and each set now
     includes 4 components.

3. NV25.  Includes two XF units on GPU, for double processing power.

   .. todo:: more?

4. NV30.  A new version of program ISA is supported, and many limits have
   been bumped.

   .. todo:: write more about it.

   .. todo:: anything else?

   .. todo:: any more revisions between NV3x GPUs?

5. NV40.  Fixed-function mode is no longer supported, and the LT engine
   has been removed as a result.  A new version of program ISA is supported
   as well.  The XF units on the GPU can now be individually enabled or
   disabled.

   .. todo:: write more about it.

6. NV41.  ???

   .. todo:: figure this out

Starting with NV25, GPUs may have multiple XF&LT units.  Every XF unit
and every LT unit can process three vertices in parallel, in SMT fashion.

XF and LT have the following state:

- VAB: made of 4-element vectors of 32-bit data, the input buffer to the XF.
  All incoming data is stored there.  All but the last tow contain the input
  attributes of vertex to be processed, while the last row is used to store
  non-vertex data in transit.  Stores persistent data and should be context
  switched.  This is shared among all XF units on the GPU.

- XFMODE [:NV40]: a few words with lots of bitfields controlling the operation
  of XF & LT.

- XFCTX: made of 4-element vectors of float32 data.  Contains numeric
  parameters to the transform process.  Used by XF for both fixed-function
  and programmable transformation.  Likewise persistent.

- LTCTX [:NV40]: made of 3-element vectors of float22 data.  Contains numeric
  parameters to the lighting process.  Used by LT vector computations
  for fixed-function lighting.  Persistent.

- LTC0, LTC1, LTC2, LTC3 [:NV40]: made of scalar float22 data.  Contains
  numeric parameters to the lighting process.  Used by LT scalar computations
  for fixed-function lighting.  Persistent.

- XFPR [NV20:]: stores the vertex programs.  Persistent.

- IBUF: Contains input attributes for a given vertex in flight.  There are 6
  instances of IBUF per XF unit.

- TBUF: Contains XF output attributes for a given vertex in flight.  There are
  6 instances of TBUF per XF unit.

- WBUF and VBUF [:NV40]: Contain XF-to-LT data for a given vertex in flight.
  There are 6 instances of each per XF&LT unit pair.

- XFREG: Contains intermediate data for XF processing.  There are 3 instances
  of XFREG per XF unit.

- LTREG [:NV40]: Contains intermediate data for LT processing.  There are 3
  instances of LTREG per LT unit.

.. todo:: PC, address reg, cond reg, ...

.. todo:: write me


XFMODE - Celsius
================

On Celsius, XFMODE consists of two 32-bit words.  They are:

``XFMODE_A``:

  - bits 0-1: LIGHT_MODE_0 - Selects how light 0 behaves.  One of:

    - 0: NONE - light is disabled.  Note that if a light is disabled, all
      subsequent lights must be disabled as well.
    - 1: INFINITE
    - 2: LOCAL
    - 3: SPOTLIGHT

  - bits 2-3: LIGHT_MODE_1 - Likewise for light 1.
  - bits 4-5: LIGHT_MODE_2
  - bits 6-7: LIGHT_MODE_3
  - bits 8-9: LIGHT_MODE_4
  - bits 10-11: LIGHT_MODE_5
  - bits 12-13: LIGHT_MODE_6
  - bits 14-15: LIGHT_MODE_7
  - bits 16-17: FOG_COORD - Selects how fog coordinate is computed.  One of:

    - 0: PASS
    - 1: DIST_RADIAL
    - 2: DIST_ORTHOGONAL
    - 3: DIST_ORTHOGONAL_ABS

  - bit 18: LIGHT_MODEL_UNK2 - ???
  - bit 19: LIGHT_MODEL_VERTEX_SPECULAR - ???
  - bit 20: LIGHT_MODEL_SEPARATE_SPECULAR - ???
  - bits 21-24: LIGHT_MATERIAL - ???
  - bit 25: POINT_PARAMS_ENABLE - if set, XF&LT compute point size.
    Otherwise, constant point size is used.
  - bit 27: WEIGHT_ENABLE - if set, eye space transformation matrices will
    be blended together using the input weight.
  - bit 28: BYPASS - if set, XF&LT are in bypass mode, and only a small set
    of computations will be performed.  Otherwise, full transform and lighting
    is enabled.
  - bit 29: ORIGIN - selects viewport offset used in bypass mode.  One of:

    - 0: CORNER
    - 1: CENTER

``XFMODE_B``:

  - bit 0: TEX_0_ENABLE - if set, coordinates for texture 0 will be
    computed.  Otherwise, texture unit 0 will be ignored.
  - bit 1: TEX_0_MATRIX_ENABLE - if set, enabled transformation of texture 0
    coordinates by texture matrix.  This must be set if texgen is used, or
    if perspective is disabled.
  - bit 2: TEX_0_PERSPECTIVE - if set, the final texture 0 coordinates will
    be multiplied by the final 1/w.
  - bits 3-5: TEX_0_GEN_S - selects how texture 0 coordinate s is generated.
  - bits 6-8: TEX_0_GEN_T
  - bits 9-11: TEX_0_GEN_R
  - bits 12-13: TEX_0_GEN_Q
  - bit 14: TEX_1_ENABLE
  - bit 15: TEX_1_MATRIX_ENABLE
  - bit 16: TEX_1_PERSPECTIVE
  - bits 17-19: TEX_1_GEN_S
  - bits 20-22: TEX_1_GEN_T
  - bits 23-25: TEX_1_GEN_R
  - bits 26-27: TEX_1_GEN_Q
  - bit 28: LIGHT_MODEL_LOCAL_VIEWER
  - bit 29: LIGHTING_ENABLE
  - bit 30: NORMALIZE_ENABLE
  - bit 31: FOG_ENABLE

Where tex gen modes can be one of:

- 0: PASS - input coordinate is passed through.
- 1: EYE_LINEAR
- 2: OBJECT_LINEAR
- 3: SPHERE_MAP (only supported on s and t)
- 4: NORMAL_MAP (only supported on s, t, r)
- 5: REFLECTION_MAP (only supported on s, t, r)
- 6: EMBOSS_MAP (only supported on s of texture 1, but if used affects all
  coordinates)


XFMODE - Kelvin
===============

On Kelvin, XFMODE consists of 4 32-bit words.

``XFMODE_A``:

  - bits 0-1: LIGHT_MODE_0 - Selects how light 0 behaves.  One of:

    - 0: NONE - light is disabled.  Note that if a light is disabled, all
      subsequent lights must be disabled as well.
    - 1: INFINITE
    - 2: LOCAL
    - 3: SPOTLIGHT

  - bits 2-3: LIGHT_MODE_1 - Likewise for light 1.
  - bits 4-5: LIGHT_MODE_2
  - bits 6-7: LIGHT_MODE_3
  - bits 8-9: LIGHT_MODE_4
  - bits 10-11: LIGHT_MODE_5
  - bits 12-13: LIGHT_MODE_6
  - bits 14-15: LIGHT_MODE_7
  - bit 18: ???, set by TL_MODE method.
  - bit 19: FOG_ENABLE - if set, XF&LT computes the fog coord.  Otherwise,
    fog computations are not performed.
  - bit 20: ???, set by UNK9CC method.
  - bit 21: FOG_MODE_EXP - if set, one of the EXP fog modes is used.
    Otherwise, one of LINEAR modes is used.
  - bits 22-24: FOG_COORD - selects how fog coordinate is computed.  One of:

    - 0: SPEC_ALPHA
    - 1: DIST_RADIAL
    - 2: DIST_ORTHOGONAL
    - 3: DIST_ORTHOGONAL_ABS
    - 4: FOG_COORD

  - bit 25: POINT_PARAMS_ENABLE - if set, XF&LT compute point size.
    Otherwise, constant point size is used.
  - bits 26-28: WEIGHT_MODE - selects how weighting works.  One of:

    - 0: NONE
    - 1: 1
    - 2: ???
    - 3: ???
    - 4: ???
    - 5: ???
    - 6: ???

  - bit 29: ???, set by UNK1E98 method.
  - bits 30-31: MODE - selects operating mode, one of:

    - 0: FIXED - full fixed-function transform and lighting
    - 1: BYPASS - minimal computations performed
    - 2: PROGRAM - vertex program is run, fixed-function computations
      disabled.

``XFMODE_B``:

  - bits 0-1: LIGHT_MATERIAL_SPECULAR_BACK - one of:

   - 0: NONE
   - 1: COL0
   - 2: COL1

  - bits 2-3: LIGHT_MATERIAL_DIFFUSE_BACK
  - bits 4-5: LIGHT_MATERIAL_AMBIENT_BACK
  - bits 6-7: LIGHT_MATERIAL_EMISSION_BACK
  - bits 8-15: PROGRAM_START_POS - index of the first program to be executed
    in PROGRAM mode.
  - bit 16: SPECULAR_ENABLE - ???
  - bit 17: ???, Kelvin LIGHT_MODEL bit 17
  - bit 18: LIGHT_MODEL_SEPARATE_SPECULAR - ???
  - bits 19-20: LIGHT_MATERIAL_SPECULAR_FRONT
  - bits 21-22: LIGHT_MATERIAL_DIFFUSE_FRONT
  - bits 23-24: LIGHT_MATERIAL_AMBIENT_FRONT
  - bits 25-26: LIGHT_MATERIAL_EMISSION_FRONT
  - bit 27: NORMALIZE_ENABLE
  - bit 28: LIGHT_MODEL_UNK2 - ???
  - bit 29: LIGHT_TWO_SIDE_ENABLE
  - bit 30: LIGHT_MODEL_LOCAL_VIEWER
  - bit 31: LIGHTING_ENABLE

``XFMODE_C`` (two instances - first describes textures 2 and 3, second
describes textures 0 and 1):

  - bit 0: TEX_0_ENABLE - if set, coordinates for texture 0/2 will be
    computed.  Otherwise, texture unit 0/2 will be ignored.
  - bit 1: TEX_0_MATRIX_ENABLE - if set, enabled transformation of texture 0/2
    coordinates by texture matrix.
  - bit 2: TEX_0_R_ENABLE - if set, the r coordinate for texture 0/2 will be
    computed.  Otherwise, it will be ignored.
  - bits 4-6: TEX_0_GEN_S - selects how texture 0 coordinate s is generated.
  - bits 7-9: TEX_0_GEN_T
  - bits 10-12: TEX_0_GEN_R
  - bits 13-15: TEX_0_GEN_Q
  - bit 16: TEX_1_ENABLE
  - bit 17: TEX_1_MATRIX_ENABLE
  - bit 18: TEX_1_R_ENABLE
  - bits 20-22: TEX_1_GEN_S
  - bits 23-25: TEX_1_GEN_T
  - bits 26-28: TEX_1_GEN_R
  - bits 29-31: TEX_1_GEN_Q


XFMODE - Rankine
================

On Rankine, XFMODE consists of 7 32-bit words.

.. todo:: write me


XFCTX
=====

.. todo:: intro?

===== ===== ===== ========================
NV10  NV20  NV30  Name
===== ===== ===== ========================
0x08+ 0x00+ 0x3c+ MATRIX_PROJ
\-    0x04+ 0x40+ MATRIX_UNK440
0x00+ 0x08+ 0x44+ MATRIX_MV0
0x04+ 0x0c+ 0x48+ MATRIX_IMV0
0x0c+ 0x10+ 0x4c+ MATRIX_MV1
0x10+ 0x14+ 0x50+ MATRIX_IMV1
\-    0x18+ 0x54+ MATRIX_MV2
\-    0x1c+ 0x58+ MATRIX_IMV2
\-    0x20+ 0x5c+ MATRIX_MV3
\-    0x24+ 0x60+ MATRIX_IMV3
0x24  0x28  0x64  LIGHT_0_POSITION
0x25  0x29  0x65  LIGHT_1_POSITION
0x26  0x2a  0x66  LIGHT_2_POSITION
0x27  0x2b  0x67  LIGHT_3_POSITION
0x28  0x2c  0x68  LIGHT_4_POSITION
0x29  0x2d  0x69  LIGHT_5_POSITION
0x2a  0x2e  0x6a  LIGHT_6_POSITION
0x2b  0x2f  0x6b  LIGHT_7_POSITION
0x2c  0x30  0x6c  LIGHT_0_SPOT_DIRECTION
0x2d  0x31  0x6d  LIGHT_1_SPOT_DIRECTION
0x2e  0x32  0x6e  LIGHT_2_SPOT_DIRECTION
0x2f  0x33  0x6f  LIGHT_3_SPOT_DIRECTION
0x30  0x34  0x70  LIGHT_4_SPOT_DIRECTION
0x31  0x35  0x71  LIGHT_5_SPOT_DIRECTION
0x32  0x36  0x72  LIGHT_6_SPOT_DIRECTION
0x33  0x37  0x73  LIGHT_7_SPOT_DIRECTION
0x34  0x38  0x74  LIGHT_EYE_POSITION
0x35  \-    \-    CONST_REFLECT_TWO
0x36  \-    \-    CONST_SPHERE_Z_ONE
0x37  \-    \-    CONST_SPHERE_XY_HALF
0x38  0x39  0x75  FOG_PLANE
\-    0x3a  0x76  VIEWPORT_SCALE
0x39  0x3b  0x77  VIEWPORT_TRANSLATE
0x3a  \-    \-    CONST_WEIGHT_ONE
\-    0x3c  0x78  KELVIN_UNK16E0
\-    0x3d  0x79  KELVIN_UNK16F0
\-    0x3e  0x7a  KELVIN_UNK1700
\-    0x3f  0x7b  KELVIN_UNK16D0
0x14  0x40  0x7c  TEX_0_GEN_S
0x15  0x41  0x7d  TEX_0_GEN_T
0x16  0x42  0x7e  TEX_0_GEN_R
0x17  0x43  0x7f  TEX_0_GEN_Q
0x18+ 0x44+ 0x80+ MATRIX_TX0
0x1c  0x48  0x84  TEX_1_GEN_S
0x1d  0x49  0x85  TEX_1_GEN_T
0x1e  0x4a  0x86  TEX_1_GEN_R
0x1f  0x4b  0x87  TEX_1_GEN_Q
0x20+ 0x4c+ 0x88+ MATRIX_TX1
\-    0x50  0x8c  TEX_2_GEN_S
\-    0x51  0x8d  TEX_2_GEN_T
\-    0x52  0x8e  TEX_2_GEN_R
\-    0x53  0x8f  TEX_2_GEN_Q
\-    0x54+ 0x90+ MATRIX_TX2
\-    0x58  0x94  TEX_3_GEN_S
\-    0x59  0x95  TEX_3_GEN_T
\-    0x5a  0x96  TEX_3_GEN_R
\-    0x5b  0x97  TEX_3_GEN_Q
\-    0x5c+ 0x98+ MATRIX_TX3
\-    \-    0x00  TEX_4_GEN_S
\-    \-    0x01  TEX_4_GEN_T
\-    \-    0x02  TEX_4_GEN_R
\-    \-    0x03  TEX_4_GEN_Q
\-    \-    0x04+ MATRIX_TX4
\-    \-    0x08  TEX_5_GEN_S
\-    \-    0x09  TEX_5_GEN_T
\-    \-    0x0a  TEX_5_GEN_R
\-    \-    0x0b  TEX_5_GEN_Q
\-    \-    0x0c+ MATRIX_TX5
\-    \-    0x10  TEX_6_GEN_S
\-    \-    0x11  TEX_6_GEN_T
\-    \-    0x12  TEX_6_GEN_R
\-    \-    0x13  TEX_6_GEN_Q
\-    \-    0x14+ MATRIX_TX6
\-    \-    0x18  TEX_7_GEN_S
\-    \-    0x19  TEX_7_GEN_T
\-    \-    0x1a  TEX_7_GEN_R
\-    \-    0x1b  TEX_7_GEN_Q
\-    \-    0x1c+ MATRIX_TX7
\-    \-    0x20  USER_CLIP_PLANE_0
\-    \-    0x21  USER_CLIP_PLANE_1
\-    \-    0x22  USER_CLIP_PLANE_2
\-    \-    0x23  USER_CLIP_PLANE_3
\-    \-    0x24  USER_CLIP_PLANE_4
\-    \-    0x25  USER_CLIP_PLANE_5
\-    \-    0x26  ???
\-    \-    0x27  ???
\-    \-    0x28  ???
\-    \-    0x29  ???
\-    \-    0x2a  ???
\-    \-    0x2b  ???
\-    \-    0x2c  ???
\-    \-    0x2d  ???
\-    \-    0x2e  ???
\-    \-    0x2f  ???
\-    \-    0x30  ???
\-    \-    0x31  ???
\-    \-    0x32  ???
\-    \-    0x33  ???
\-    \-    0x34  ???
\-    \-    0x35  ???
\-    \-    0x36  ???
\-    \-    0x37  ???
\-    \-    0x38  ???
\-    \-    0x39  ???
\-    \-    0x3a  ???
\-    \-    0x3b  ???
0x3b  \-    \-    [unused]
===== ===== ===== ========================


LTCTX
=====

.. todo:: intro?

==== ==== ==== ========================
NV10 NV20 NV30 Name
==== ==== ==== ========================
0x00 0x00 ?    LIGHT_0_AMBIENT_COLOR
0x01 0x01 ?    LIGHT_0_DIFFUSE_COLOR
0x02 0x02 ?    LIGHT_0_SPECULAR_COLOR
0x03 0x03 ?    LIGHT_0_HALF_VECTOR
0x04 0x04 ?    LIGHT_0_DIRECTION
\-   0x05 ?    LIGHT_0_BACK_AMBIENT_COLOR
\-   0x06 ?    LIGHT_0_BACK_DIFFUSE_COLOR
\-   0x07 ?    LIGHT_0_BACK_SPECULAR_COLOR
0x05 0x08 ?    LIGHT_1_AMBIENT_COLOR
0x06 0x09 ?    LIGHT_1_DIFFUSE_COLOR
0x07 0x0a ?    LIGHT_1_SPECULAR_COLOR
0x08 0x0b ?    LIGHT_1_HALF_VECTOR
0x09 0x0c ?    LIGHT_1_DIRECTION
\-   0x0d ?    LIGHT_1_BACK_AMBIENT_COLOR
\-   0x0e ?    LIGHT_1_BACK_DIFFUSE_COLOR
\-   0x0f ?    LIGHT_1_BACK_SPECULAR_COLOR
0x0a 0x10 ?    LIGHT_2_AMBIENT_COLOR
0x0b 0x11 ?    LIGHT_2_DIFFUSE_COLOR
0x0c 0x12 ?    LIGHT_2_SPECULAR_COLOR
0x0d 0x13 ?    LIGHT_2_HALF_VECTOR
0x0e 0x14 ?    LIGHT_2_DIRECTION
\-   0x15 ?    LIGHT_2_BACK_AMBIENT_COLOR
\-   0x16 ?    LIGHT_2_BACK_DIFFUSE_COLOR
\-   0x17 ?    LIGHT_2_BACK_SPECULAR_COLOR
0x0f 0x18 ?    LIGHT_3_AMBIENT_COLOR
0x10 0x19 ?    LIGHT_3_DIFFUSE_COLOR
0x11 0x1a ?    LIGHT_3_SPECULAR_COLOR
0x12 0x1b ?    LIGHT_3_HALF_VECTOR
0x13 0x1c ?    LIGHT_3_DIRECTION
\-   0x1d ?    LIGHT_3_BACK_AMBIENT_COLOR
\-   0x1e ?    LIGHT_3_BACK_DIFFUSE_COLOR
\-   0x1f ?    LIGHT_3_BACK_SPECULAR_COLOR
0x14 0x20 ?    LIGHT_4_AMBIENT_COLOR
0x15 0x21 ?    LIGHT_4_DIFFUSE_COLOR
0x16 0x22 ?    LIGHT_4_SPECULAR_COLOR
0x17 0x23 ?    LIGHT_4_HALF_VECTOR
0x18 0x24 ?    LIGHT_4_DIRECTION
\-   0x25 ?    LIGHT_4_BACK_AMBIENT_COLOR
\-   0x26 ?    LIGHT_4_BACK_DIFFUSE_COLOR
\-   0x27 ?    LIGHT_4_BACK_SPECULAR_COLOR
0x19 0x28 ?    LIGHT_5_AMBIENT_COLOR
0x1a 0x29 ?    LIGHT_5_DIFFUSE_COLOR
0x1b 0x2a ?    LIGHT_5_SPECULAR_COLOR
0x1c 0x2b ?    LIGHT_5_HALF_VECTOR
0x1d 0x2c ?    LIGHT_5_DIRECTION
\-   0x2d ?    LIGHT_5_BACK_AMBIENT_COLOR
\-   0x2e ?    LIGHT_5_BACK_DIFFUSE_COLOR
\-   0x2f ?    LIGHT_5_BACK_SPECULAR_COLOR
0x1e 0x30 ?    LIGHT_6_AMBIENT_COLOR
0x1f 0x31 ?    LIGHT_6_DIFFUSE_COLOR
0x20 0x32 ?    LIGHT_6_SPECULAR_COLOR
0x21 0x33 ?    LIGHT_6_HALF_VECTOR
0x22 0x34 ?    LIGHT_6_DIRECTION
\-   0x35 ?    LIGHT_6_BACK_AMBIENT_COLOR
\-   0x36 ?    LIGHT_6_BACK_DIFFUSE_COLOR
\-   0x37 ?    LIGHT_6_BACK_SPECULAR_COLOR
0x23 0x38 ?    LIGHT_7_AMBIENT_COLOR
0x24 0x39 ?    LIGHT_7_DIFFUSE_COLOR
0x25 0x3a ?    LIGHT_7_SPECULAR_COLOR
0x26 0x3b ?    LIGHT_7_HALF_VECTOR
0x27 0x3c ?    LIGHT_7_DIRECTION
\-   0x3d ?    LIGHT_7_BACK_AMBIENT_COLOR
\-   0x3e ?    LIGHT_7_BACK_DIFFUSE_COLOR
\-   0x3f ?    LIGHT_7_BACK_SPECULAR_COLOR
0x28 \-   ?    ???
\-   0x40 ?    LT_UNK17E0
0x29 0x41 ?    LIGHT_MODEL_AMBIENT_COLOR
\-   0x42 ?    LIGHT_MODEL_BACK_AMBIENT_COLOR
0x2a 0x43 ?    MATERIAL_FACTOR_RGB
\-   0x44 ?    MATERIAL_FACTOR_BACK_RGB
0x2b 0x45 ?    FOG_COEFF
0x2c \-   ?    CONST_ZERO
\-   0x46 ?    LT_UNK17D4
0x2d 0x47 ?    POINT_PARAMS_012
0x2e 0x48 ?    POINT_PARAMS_345
0x2f \-   ?    [unused]
\-   0x49 ?    LT_UNK17EC
\-   \-   0x36 VIEWPORT_TRANSLATE
\-   \-   0x37 VIEWPORT_SCALE
==== ==== ==== ========================


LTC
===

.. todo:: intro?

====== ====== ====================
NV10   NV20   Name
====== ====== ====================
0.0x00 \-     [const 1.0]
0.0x01 \-     CONST_???
\-     0.0x00 ???
\-     0.0x01 ???
0.0x02 0.0x02 MATERIAL_SHININESS_3
\-     0.0x03 MATERIAL_BACK_SHININESS_3
1.0x00 \-     [const 0.0]
\-     1.0x00 ???
1.0x01 1.0x01 MATERIAL_SHININESS_0
\-     1.0x02 MATERIAL_BACK_SHININESS_0
1.0x02 1.0x03 POINT_PARAMS_6
1.0x03 1.0x04 LIGHT_0_LOCAL_RANGE
1.0x04 1.0x05 LIGHT_1_LOCAL_RANGE
1.0x05 1.0x06 LIGHT_2_LOCAL_RANGE
1.0x06 1.0x07 LIGHT_3_LOCAL_RANGE
1.0x07 1.0x08 LIGHT_4_LOCAL_RANGE
1.0x08 1.0x09 LIGHT_5_LOCAL_RANGE
1.0x09 1.0x0a LIGHT_6_LOCAL_RANGE
1.0x0a 1.0x0b LIGHT_7_LOCAL_RANGE
1.0x0b 1.0x0c LIGHT_0_SPOT_CUTOFF_0
1.0x0c 1.0x0d LIGHT_1_SPOT_CUTOFF_0
1.0x0d 1.0x0e LIGHT_2_SPOT_CUTOFF_0
1.0x0e 1.0x0f LIGHT_3_SPOT_CUTOFF_0
1.0x0f 1.0x10 LIGHT_4_SPOT_CUTOFF_0
1.0x10 1.0x11 LIGHT_5_SPOT_CUTOFF_0
1.0x11 1.0x12 LIGHT_6_SPOT_CUTOFF_0
1.0x12 1.0x13 LIGHT_7_SPOT_CUTOFF_0
2.0x00 \-     [const 1.0]
\-     2.0x00 ???
2.0x01 2.0x01 MATERIAL_SHININESS_1
\-     2.0x02 MATERIAL_BACK_SHININESS_1
2.0x02 2.0x03 MATERIAL_SHININESS_4
\-     2.0x04 MATERIAL_BACK_SHININESS_4
2.0x03 2.0x05 MATERIAL_SHININESS_5
\-     2.0x06 MATERIAL_BACK_SHININESS_5
2.0x04 2.0x07 LIGHT_0_SPOT_CUTOFF_1
2.0x05 2.0x08 LIGHT_1_SPOT_CUTOFF_1
2.0x06 2.0x09 LIGHT_2_SPOT_CUTOFF_1
2.0x07 2.0x0a LIGHT_3_SPOT_CUTOFF_1
2.0x08 2.0x0b LIGHT_4_SPOT_CUTOFF_1
2.0x09 2.0x0c LIGHT_5_SPOT_CUTOFF_1
2.0x0a 2.0x0d LIGHT_6_SPOT_CUTOFF_1
2.0x0b 2.0x0e LIGHT_7_SPOT_CUTOFF_1
3.0x00 \-     [const 0.0]
\-     3.0x00 ???
3.0x01 3.0x01 POINT_PARAMS_7
3.0x02 3.0x02 MATERIAL_SHININESS_2
\-     3.0x03 MATERIAL_BACK_SHININESS_2
3.0x03 3.0x04 LIGHT_0_SPOT_CUTOFF_2
3.0x04 3.0x05 LIGHT_1_SPOT_CUTOFF_2
3.0x05 3.0x06 LIGHT_2_SPOT_CUTOFF_2
3.0x06 3.0x07 LIGHT_3_SPOT_CUTOFF_2
3.0x07 3.0x08 LIGHT_4_SPOT_CUTOFF_2
3.0x08 3.0x09 LIGHT_5_SPOT_CUTOFF_2
3.0x09 3.0x0a LIGHT_6_SPOT_CUTOFF_2
3.0x0a 3.0x0b LIGHT_7_SPOT_CUTOFF_2
3.0x0b 3.0x0c MATERIAL_FACTOR_A
\-     3.0x0d MATERIAL_FACTOR_BACK_A
====== ====== ====================
