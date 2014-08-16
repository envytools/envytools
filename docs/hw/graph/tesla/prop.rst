.. _nv50-prop:

Pre-ROP: PROP
=============

.. contents::

.. todo:: write me


PCOUNTER signals
================

- 0x00:

  - 2: rop_busy[0]
  - 3: rop_busy[1]
  - 4: rop_busy[2]
  - 5: rop_busy[3]
  - 6: rop_waits_for_shader[0]
  - 7: rop_waits_for_shader[1]

- 0x03: shaded_pixel_count...?

- 0x15:

  - 0-5: rop_samples_in_count_1
  - 6: rop_samples_in_count_0[0]
  - 7: rop_samples_in_count_0[1]

- 0x16:

  - 0-5: rasterizer_pixels_out_count_1
  - 6: rasterizer_pixels_out_count_0[0]
  - 7: rasterizer_pixels_out_count_0[1]

- 0x1a:

  - 0-5: rop_samples_killed_by_earlyz_count

- 0x1b:

  - 0-5: rop_samples_killed_by_latez_count

- 0x1c: shaded_pixel_count...?
- 0x1d: shaded_pixel_count...?

- 0x1e:

  - 0: CG_IFACE_DISABLE [G80]
  - 0: CG[0] [G84:]
  - 1: CG[1] [G84:]
  - 2: CG[2] [G84:]
