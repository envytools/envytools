.. _g80-vfetch:

Vertex fetch: VFETCH
====================

.. contents::

.. todo:: write me


PCOUNTER signals
================

Mux 0:

- 0x0e: geom_vertex_in_count[0]
- 0x0f: geom_vertex_in_count[1]
- 0x10: geom_vertex_in_count[2]

- 0x19: CG_IFACE_DISABLE [G80]

Mux 1:

- 0x02: input_assembler_busy[0]
- 0x03: input_assembler_busy[1]
- 0x08: geom_primitive_in_count
- 0x0b: input_assembler_waits_for_fb [G200:]
- 0x0e: input_assembler_waits_for_fb [G80:G200]
- 0x14: input_assembler_busy[2] [G200:]
- 0x15: input_assembler_busy[3] [G200:]
- 0x17: input_assembler_busy[2] [G80:G200]
- 0x18: input_assembler_busy[3] [G80:G200]

Mux 2 [G84:]:

- 0x00: CG[0]
- 0x01: CG[1]
- 0x02: CG[2]
