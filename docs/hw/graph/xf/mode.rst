.. _pgraph-xf-mode:

=================
XF mode selection
=================

.. contents::


Introduction
============

This document describes the mode bits controlling XF behavior.  On NV10:NV40,
such mode bits are gathered in a 128-bit vector (or two vectors on Rankine)
called XFMODE.  XFMODE is loaded to XF via the IDX2XF MODE command.
FE3D keeps a MMIO-exposed shadow copy of the XFMODE vector(s), updating it as
mode-affecting methods are processed, and sending a copy to XF every time it
changes.  The shadow copy is also used for context switching.  Due to the
word endianness mismatch between FE shadow copy / IDX2XF addresses and XF
internal commands, keeping track of the word positions can be rather confusing.

On NV40:, XFMODE no longer exists, and XF mode is instead controlled by
state bundles like most other parts of the pipeline.


XFMODE -- Celsius
=================

On Celsius, XFMODE is a single 128-bit vector, with the following fields:

- bits 0-31: ``XFMODE_A``, the low word:

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

- bits 32-63: ``XFMODE_B``, the high word:

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

- bits 64-127: unused.

Where tex gen modes can be one of:

- 0: PASS - input coordinate is passed through.
- 1: EYE_LINEAR
- 2: OBJECT_LINEAR
- 3: SPHERE_MAP (only supported on s and t)
- 4: NORMAL_MAP (only supported on s, t, r)
- 5: REFLECTION_MAP (only supported on s, t, r)
- 6: EMBOSS_MAP (only supported on s of texture 1, but if used affects all
  coordinates)

The FE3D shadow copies are kept at:

- MMIO 0x400f40: ``XFMODE_B``
- MMIO 0x400f44: ``XFMODE_A`` (writing this register causes the MODE command
  to be submitted to XF).


XFMODE -- Kelvin & Rankine
==========================

On Kelvin, XFMODE consists of a single 128-bit vector:

- bits 0-31 aka word 3: ``XFMODE_T[0]`` (textures 0 and 1)
- bits 32-63 aka word 2: ``XFMODE_T[1]`` (textures 2 and 3)
- bits 64-95 aka word 1: ``XFMODE_A``
- bits 96-127 aka word 0: ``XFMODE_B``

On Rankine, XFMODE consists of two 128-bit vectors:

- vector 0:

  - bits 0-31 aka word 3: ``XFMODE_A``
  - bits 32-63 aka word 2: ``XFMODE_B``
  - bits 64-95 aka word 1: ``XFMODE_C``
  - bits 96-127 aka word 0: unused

- vector 1:

  - bits 0-31 aka word 3: ``XFMODE_T[0]`` (texture coordinates 0 and 1)
  - bits 32-63 aka word 2: ``XFMODE_T[1]`` (texture coordinates 2 and 3)
  - bits 64-95 aka word 1: ``XFMODE_T[2]`` (texture coordinates 4 and 5)
  - bits 96-127 aka word 0: ``XFMODE_T[3]`` (texture coordinates 6 and 7)

The words are as follows:

``XFMODE_A``:

  - bits 0-1: LIGHT_MATERIAL_SPECULAR_BACK - one of:

   - 0: NONE
   - 1: COL0
   - 2: COL1

  - bits 2-3: LIGHT_MATERIAL_DIFFUSE_BACK
  - bits 4-5: LIGHT_MATERIAL_AMBIENT_BACK
  - bits 6-7: LIGHT_MATERIAL_EMISSION_BACK
  - bits 8-15: PROGRAM_START_POS - index of the first program to be executed
    in PROGRAM_* modes.
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

``XFMODE_B``:

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
  - bit 16: VIEWPORT_TRANSFORM_SKIP [NV30:] -- if set, the position output
    from vertex program is assumed to already be in screen coordinates, and
    no viewport transform will be performed.  Otherwise, it is assumed to
    be in clip coordinates and will be transformed by fixed-function viewport
    transform.
  - bit 17: ARITH_RULES [NV30:] -- selects how various arithmetic operations
    behave.

    - 0: LEGACY -- semantics as in GL_NV_vertex_program, with various
      idiosyncracies (0 times NaN is 0, -NaN < -Inf < -0 < 0 < Inf < NaN, etc).
    - 1: MODERN -- semantics as in GL_NV_vertex_program2, mostly following
      IEEE 754.

  - bit 18: XFCTX_ACCESS -- determines which XFCTX entries are accessible to
    the running programs:

    - 0: USER_ONLY -- only USER will be accessible by indirect accesses; only
      USER, VIEWPORT_TRANSLATE, and VIEWPORT_SCALE will be accessible by direct
      accesses.
    - 1: FULL -- all XFCTX entries are accessible.

  - bit 19: FOG_ENABLE - if set, XF&LT computes the fog coord.  Otherwise,
    fog computations are not performed.
  - bit 20: ???, set by UNK9CC method.
  - bit 21: FOG_MODE_EXP [NV20:NV30] - if set, one of the EXP fog modes is used.
    Otherwise, one of LINEAR modes is used.
  - bits 22-24: FOG_COORD [NV20:NV30] - selects how fog coordinate is computed.
    One of:

    - 0: SPEC_ALPHA
    - 1: DIST_RADIAL
    - 2: DIST_ORTHOGONAL
    - 3: DIST_ORTHOGONAL_ABS
    - 4: FOG_COORD

  - bits 22-23: FOG_COORD [NV30:] - selects how fog coordinate is computed.
    One of:

    - 0: SPEC_ALPHA
    - 1: DIST_RADIAL
    - 2: DIST_ORTHOGONAL
    - 3: FOG_COORD

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

  - bit 29: XFCTX_WRITE_ENABLE -- if set, vertex programs are allowed to write
    to XFCTX, but will execute serially.  If clear, writes are blocked, but
    vertices can be processed in parallel.
  - bits 30-31: MODE -- selects operating mode, one of:

    - 0: FIXED -- full fixed-function transform and lighting
    - 1: BYPASS [NV20:NV30] -- minimal computations performed
    - 1: PROGRAM_V3 [NV40:] -- vertex program is run, fixed-function
      computations disabled, third-generation ISA features are supported.
    - 2: PROGRAM_V1 -- vertex program is run, fixed-function computations
      disabled, first-generation ISA features are supported.
    - 3: PROGRAM_V2 [NV30:] -- like above, but second-generation ISA features
      are supported.

``XFMODE_C`` (only on Rankine):

  - bits 0-5: CLIP_PLANE_ENABLE_[0-5]

``XFMODE_T`` (two instances on Kelvin, four on Rankine - each describes two
textures):

  - bit 0: TEX_0_ENABLE - if set, coordinates for texture 0/2/4/6 will be
    computed.  Otherwise, texture unit 0/2/4/6 will be ignored.
  - bit 1: TEX_0_MATRIX_ENABLE - if set, enabled transformation of texture 0/2/4/6
    coordinates by texture matrix.
  - bit 2: TEX_0_R_ENABLE - if set, the r coordinate for texture 0/2/4/6 will be
    computed.  Otherwise, it will be ignored.
  - bits 4-6: TEX_0_GEN_S - selects how texture 0/2/4/6 coordinate s is generated.
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

The supported texgen mode are the same as on Celsius.

On Kelvin, the ``FE3D`` shadow copies are kept at:

- MMIO 0x400fb4: ``XFMODE_B``
- MMIO 0x400fb8: ``XFMODE_A``
- MMIO 0x400fbc: ``XFMODE_T[1]``
- MMIO 0x400fc0: ``XFMODE_T[0]``

And on Rankine:

- MMIO 0x400fb4: (dummy 0 word)
- MMIO 0x400fb8: ``XFMODE_C``
- MMIO 0x400fbc: ``XFMODE_B``
- MMIO 0x400fc0: ``XFMODE_A``
- MMIO 0x400fc4: ``XFMODE_T[3]``
- MMIO 0x400fc8: ``XFMODE_T[2]``
- MMIO 0x400fcc: ``XFMODE_T[1]``
- MMIO 0x400fd0: ``XFMODE_T[0]``


Curie XF bundles
================

XF_A:

  - bit 0: ???, set by UNK9CC method [NV40:NV41]
  - bit 2: XFCTX_ACCESS [NV40:NV41]
  - bits 3-4: LIGHT_MATERIAL_EMISSION_FRONT [NV40:NV41]
  - bits 5-6: LIGHT_MATERIAL_AMBIENT_FRONT [NV40:NV41]
  - bits 7-8: LIGHT_MATERIAL_DIFFUSE_FRONT [NV40:NV41]
  - bits 9-10: LIGHT_MATERIAL_SPECULAR_FRONT [NV40:NV41]
  - bits 11-12: LIGHT_MATERIAL_EMISSION_BACK [NV40:NV41]
  - bits 13-14: LIGHT_MATERIAL_AMBIENT_BACK [NV40:NV41]
  - bits 15-16: LIGHT_MATERIAL_DIFFUSE_BACK [NV40:NV41]
  - bits 17-18: LIGHT_MATERIAL_SPECULAR_BACK [NV40:NV41]
  - bits 19-21: FOG_COORD [NV40:NV41]
  - bit 22: LIGHTING_ENABLE [NV40:NV41]
  - bits 23-25: WEIGHT_MODE [NV40:NV41]
  - bit 26: NORMALIZE_ENABLE [NV40:NV41]
  - bit 28: VIEWPORT_TRANSFORM_SKIP

XF_LIGHT [NV40:NV41]:

  - bits 0-1: LIGHT_MODE_0
  - bits 2-3: LIGHT_MODE_1
  - bits 4-5: LIGHT_MODE_2
  - bits 6-7: LIGHT_MODE_3
  - bits 8-9: LIGHT_MODE_4
  - bits 10-11: LIGHT_MODE_5
  - bits 12-13: LIGHT_MODE_6
  - bits 14-15: LIGHT_MODE_7
  - bit 16: LIGHT_MODEL_LOCAL_VIEWER
  - bit 17: ???, Kelvin LIGHT_MODEL bit 17
  - bit 18: LIGHT_MODEL_SEPARATE_SPECULAR - ???

XF_C:

  - bits 0-9: PROGRAM_START_POS
  - bit 27: ARITH_RULES
  - bits 30-31: MODE

XF_D:

  - bits 0-15: TIMEOUT
  - bit 16: ??? set by UNK1EF8 bit 20

XF_TXC:

  - bits 0-2: TEX_GEN_S [NV40:NV41] [only present for first 8 coords]
  - bits 4-6: TEX_GEN_T [NV40:NV41] [only present for first 8 coords]
  - bits 8-10: TEX_GEN_R [NV40:NV41] [only present for first 8 coords]
  - bits 12-14: TEX_GEN_Q [NV40:NV41] [only present for first 8 coords]
  - bit 16: TEX_MATRIX_ENABLE [NV40:NV41] [only present for first 8 coords]
  - bit 17: ???
  - bit 18: ???
  - bit 19: ???

.. todo:: Incomplete list.


Mode setting methods
====================

.. todo:: write me
