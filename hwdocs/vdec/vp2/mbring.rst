=============
MBRING format
=============

.. contents::


Introduction
============

An invocation of SLICE_DATA VLD command writes the decoded data into the
MBRING. The MBRING is a ring buffer located in VM memory, made of 32-bit word
oriented packets. Each packet starts with a header word, whose high 8 bits
signify the packet type.

An invocation of SLICE_DATA command writes the following packets, in order:

- pred weight table [packet type 4] - if PRED_WEIGHT_TABLE command has been
  invoked previously
- for each macroblock [including skipped] in slice, in decoding order:

  - motion vectors [packet type 1] - if macroblock is not skipped and not
    intra coded
  - macroblock info [packet type 0] - always
  - residual data [packet type 2] - if at least one non-zero coefficient
    present
  - coded block mask [packet type 3] - if macroblock is not skipped


Packet type 0: macroblock info
==============================

Packet is made of a header word and 3 or 6 payload words.

- Header word:

  - bits 0-23: number of payload words [3 or 6]
  - bits 24-31: packet type [0]

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

Packet has 3 payload words when macroblock is skipped, 6 when it's not
skipped. This packet type is present for all macroblocks. The mb_type
and sub_mb_type values correspond to values used in CAVLC mode for current
slice_type - thus for example I_NxN is mb_type 0 when decoding I slices,
mb_type 5 when decoding P slices. For I_NxN macroblocks encoded in 4x4
transform mode, rem_intra_pred_mode[i] and pred_intra_pred_mode_flag[i]
correspond to rem_intra4x4_pred_mode[i] and pred_intra4x4_pred_mode_flag[i]
for i = 0..15. For I_NxN macroblocks encoded in 8x8 transform mode,
rem_intra_pred_mode[i] and pred_intra_pred_mode_flag[i] correspond to
rem_intra8x8_pred_mode[i] and pred_intra8x8_pred_mode_flag[i] for i = 0..3,
and are unused for i = 4..15.


Packet type 1: motion vectors
=============================

Packet is made of two header words + 1 word for each motion vector.

- Header word:

  - bits 0-23: number of motion vectors [always 0x20]
  - bits 24-31: packet type [1]

- Second header word:

  - bit i = bit 4 of ref_idx[i]

- Motion vector word i:

  - bits 0-12: mvd[i] Y coord
  - bits 13-27: mvd[i] X coord
  - bits 28-31: bits 0-3 of ref_idx[i]

Indices 0..15 correspond to mvd_l0 and ref_idx_l0, indices 16-31 correspond
to mvd_l1 and ref_idx_l1. Each index corresponds to one 4x4 block, in the
usual scan order for 4x4 blocks. Data is always included for all blocks - if
macroblock/sub-macroblock partition size greater than 4x4 is used, its data
is duplicated for all covered blocks.


Packet type 2: residual data
============================

Packet is made of a header word + 1 halfword for each residual coefficient +
0 or 1 halfwords of padding to the next multiple of word size

- Header word:

  - bits 0-23: number of residual coefficients
  - bits 24-31: packet type [2]

- Payload halfword:

  - bits 0-15: residual coefficient

For I_PCM macroblocks, this packet contains one coefficient for each
pcm_sample_* element present in the bitstream, stored in bitstream order.

For other types of macroblocks, this packet contains data for all blocks
that have at least one non-zero coefficient. If a block has a non-zero
coefficient, all coefficients for this block, including zero ones, are
stored in this packet. Otherwise, The block is entirely skipped. The
coefficients stored in this packet type are dezigzagged - their order
inside a single block corresponds to raster scan order. The blocks are
stored in decoding order. The mask of blocks stored in this packet is
stored in packet type 3. If there are no non-zero coefficients in the whole
macroblock, this packet is not present.


Packet type 3: coded block mask
===============================

Packet is made of a header word and a payload word.

- Header word:

  - bits 0-23: number of payload words [1]
  - bits 24-31: packet type [3]

- Payload word [4x4 mode]:

  - bits 0-15: luma 4x4 blocks 0-15 [16 coords each]
  - bit 16: Cb DC block [4 coords]
  - bit 17: Cr DC block [4 coords]
  - bits 18-21: Cb AC blocks 0-3 [15 coords each]
  - bits 22-25: Cr AC blocks 0-3 [15 coords each]

- Payload word [8x8 mode]:

  - bits 0-3: luma 8x8 blocks 0-3 [64 coords each]
  - bit 4: Cb DC block [4 coords]
  - bit 5: Cr DC block [4 coords]
  - bits 6-9: Cb AC blocks 0-3 [15 coords each]
  - bits 10-13: Cr AC blocks 0-3 [15 coords each]

- Payload word [intra 16x16 mode]:

  - bit 0: luma DC block [16 coords]
  - bits 1-16: luma AC blocks 0-15 [15 coords each]
  - bit 17: Cb DC block [4 coords]
  - bit 18: Cr DC block [4 coords]
  - bits 19-22: Cb AC blocks 0-3 [15 coords each]
  - bits 23-26: Cr AC blocks 0-3 [15 coords each]

- Payload word [PCM mode]: [all 0]

This packet stores the mask of blocks present in preceding packet of type 2
[if any]. The bit corresponding to a block is 1 if the block has at least one
non-zero coefficient and is stored in the residual data packet, 0 if all
its coefficients are zero and it's not stored in the residual data packet.
This packet type is present for all non-skipped macroblocks, including I_PCM
macroblocks - but its payload word is always equal to 0 for I_PCM.


Packet type 4: pred weight table
================================

Packet is made of a header word and a variable number of table write requests,
each request being two words long.

- Header word:

  - bits 0-23: number of write requests
  - bits 24-31: packet type [4]

- Request word 0: table index to write
- Request word 1: data value to write

The pred weight table is treated as an array of 0x81 32-bit numbers. This
packet is made of "write requests" which are supposed to modify the table
entries in the receiver.

The table indices are:

- Index i * 2, 0 <= i <= 0x1f:

  - bits 0-7: luma_offset_l0[i]
  - bits 8-15: luma_weight_l0[i]
  - bit 16: chroma_weight_l0_flag[i]
  - bit 17: luma_weight_l0_flag[i]

- Index i * 2 + 1, 0 <= i <= 0x1f:

  - bits 0-7: chroma_offset_l0[i][1]
  - bits 8-15: chroma_weight_l0[i][1]
  - bits 16-23: chroma_offset_l0[i][0]
  - bits 24-31: chroma_weight_l0[i][0]

- Index 0x40 + i * 2, 0 <= i <= 0x1f:

  - bits 0-7: luma_offset_l1[i]
  - bits 8-15: luma_weight_l1[i]
  - bit 16: chroma_weight_l1_flag[i]
  - bit 17: luma_weight_l1_flag[i]

- Index 0x40 + i * 2 + 1, 0 <= i <= 0x1f:

  - bits 0-7: chroma_offset_l1[i][1]
  - bits 8-15: chroma_weight_l1[i][1]
  - bits 16-23: chroma_offset_l1[i][0]
  - bits 24-31: chroma_weight_l1[i][0]

- Index 0x80:

  - bits 0-2: chroma_log2_weight_denom
  - bits 3-5: luma_log2_weight_denom

The requests are emitted in the following order:

- 0x80
- for 0 <= i <= num_ref_idx_l0_active_minus1: 2*i, 2*i + 1
- for 0 <= i <= num_ref_idx_l1_active_minus1: 0x40 + 2*i, 0x40 + 2*i + 1

The fields corresponding to data not present in the bitstream are set to 0,
they're *not* set to their inferred values.
