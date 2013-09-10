=================
VP3 MBRING format
=================

.. contents::

Introduction
============

The macroblock ring outputted from VLD is packet based, and aligned on
32-bit word size.

A packet has the header type in bits [24..31] and length in bits [0..23].
The data length is in words, and doesn't include the header itself.


type 00: Macro block header
===========================

MPEG2
-----

The macro block header contains 4 data words:

- Word 0:

  - [0:15] Absolute address in macroblock units, 0 based

- Word 1:

  - [0:7] Y coord in macroblock units, 0 based
  - [8:15] X coord in macroblock units, 0 based

- Word 2:

  - [0] not_coded[??]
  - [1] skipped[??]
  - [3] quant
  - [4] motion_forward
  - [5] motion_backward
  - [6] coded_block_pattern
  - [7] intra
  - [26:26] dct_type
  - [27:28] motion_type

    - 0: field motion
    - 1: frame-based motion
    - 2: 16x8 field
    - 3: dual prime motion

- Word 3:

  - [6:7] motion_vector_count
  - [8:12] quantiser_scale_code


H.264
-----

- Payload word 0:

  - bits 0-12: macroblock address

- Payload word 1:

  - bits 0-7: y coord in macroblock units
  - bits 8-15: x coord in macroblock units

- Payload word 2:

  - bit 0: first macroblock of a slice flag
  - bit 1: mb_skip_flag
  - bit 2: mb_field_coding_flag
  - bits 3-8: mb_type
  - bits 9+i*4 - 12+i*4, i < 4: sub_mb_type[i]
  - bit 25: transform_size_8x8_flag

- Payload word 3:

  - bits 0-5: mb_qp_delta
  - bits 6-7: intra_chroma_pred_mode

- Payload word 4:

  - bits i*4+0 - i*4+2, i < 8: rem_intra_pred_mode[i]
  - bit i*4+3, i < 8: prev_intra_pred_mode_flag[i]

- Payload word 5:

  - bits i*4+0 - i*4+2, i < 8: rem_intra_pred_mode[i+8]
  - bit i*4+3, i < 8: prev_intra_pred_mode_flag[i+8]


Error
-----

The macro block header contains 3 data words:

- Word 0:

  - [0:15] Absolute address in macroblock units, 0 based
  - [16] error flag, always set

- Word 1:

  - [0:7] Y coord in macroblock units, 0 based
  - [8:15] X coord in macroblock units, 0 based

- Word 2: all 0


type 01: Motion vector
======================

MPEG2
-----

.. todo:: Verify whether X or Y is in the lowest 16 bits. I assume X

The motion vector has a length of 4 data words, and contains a total of
8 PMVs with a size of 16 bits each. The motion vectors are likely
encoded in order of the spec with PMV[r][s][t].

The layout of each 16 bit PMV:

- [0:5] motion code
- [6:13] residual
- [14] motion_vertical_field_select
- [14:15] dmvector (0, 1, or 3)

motion_vertical_field_select and dmvector occupy same bits, but the mpeg
spec makes them mutually exclusive, so they don't conflict.


H.264
-----

Payload like VP2, except length is in 32-bit words.


type 02: DCT coordinates
========================

A packet of this type is created for each pattern enabled in
coded_block_pattern. This packet type is byte oriented, rather than word
oriented. It splits the coordinates up in chunks of 4 coordinates each,
so 0..3 becomes 0, 4..7 becomes 1, 60..63 becomes chunk 15. The first 2
bytes contain a 16-bit bitmask indicating the presence of each chunk. If
a chunks bit is set it will be encoded further.

For each present chunk a 8-bit bitmask will be created, which contains the
size of each coordinate in that chunk. 2 bits are used for each coordinate,
indicating the size (0 = not present, 1 = 1 byte, 2 = 2 bytes).
This is followed by all coordinates present in this chunk, the last chunk
is padded with 0s to align to word size.

For example: 0x10 0x00 0x40 0xff

Chunk 4 (0x0010>>4)&1 has pos 3 (0x40 >> (2*3))&3 set to -1


type 03: PCM data
=================

Payload length is 0x60 words. Packet is byte oriented, instead of word
oriented. Payload is raw PCM data from bitstream.


type 04: Coded block pattern
============================

MPEG2
-----
This packet puts coded_block_pattern in 1 data word.


H.264
-----

Payload like VP2.


type 05: Pred weight table
==========================

Payload like VP2, except length is in 32-bit words.


type 06: End of stream
======================

This header has no length, and signals the parser it's done.


Macroblock
==========

A macroblock is created in this order:

- motion vector (optional)
- macro block header
- DCT coordinates / PCM samples (optional, and repeated as many times as needed)
- coded_block_pattern (optional)

'optional' is relative to the MPEG spec. For example intra frames always
require a coded_block_pattern.
