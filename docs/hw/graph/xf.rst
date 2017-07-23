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
