.. _pbsp-vld:

=============================
VLD: variable length decoding
=============================

.. contents::


Introduction
============

The VLD is the first stage of the VP2 decoding pipeline. It is part of
PBSP and deals with decoding the H.264 bitstream into syntax elements.

The input to the VLD is the raw H.264 bitstream. The output of VLD is MBRING,
a ring buffer structure storing the decoded syntax elements in the form
of word-aligned packets.

The VLD only deals with parsing the NALs containing the slice data - the
remaining NAL types are supposed to be parsed by the host. Further, the
hardware can only parse pred_weight_table and slice_data elements efficiently
- the remaining parts of the slice NAL are supposed to be parsed by the
firmware controlling the VLD in a semi-manual manner: the VLD provides
commands that parse single syntax elements.

The following H.264 profiles are supported:

- Constrained Baseline
- Baseline [only in single-macroblock mode if FMO used - see below]
- Main
- Progressive High
- High
- Multiview High
- Stereo High

The limitations are:

- max picture width and height: 128 macroblocks
- max macroblocks in picture: 8192

.. todo:: width/height max may be 255?

There are two modes of operation that VLD can be used with: single-macroblock
mode and whole-slice mode. In the single-macroblock mode, parsing for each
macroblock has to be manually triggered by the firmware. In whole-slice mode,
the firmware triggers processing of a whole slice, and the hardware
automatically iterates over all macroblocks in the slice. However, whole-slice
mode doesn't support Flexible Macroblock Ordering aka. slice groups. Thus,
single-macroblock mode has to be used for sequences with non-zero value of
num_slice_groups_minus1.

The VLD keeps extensive hidden internal state, including:

- pred_weight_table data, to be prepended to the next emitted macroblock
- bitstream position, zero byte count [for escaping], and lookahead buffer
- CABAC valMPS, pStateIdx, codIOFfset, codIRange state
- previously decoded parts of macroblock data, used for CABAC and CAVLC
  context selection algorithms
- already queued but not yet written MBRING output data


.. _pbsp-io-vld:

The registers
=============

The VLD registers are located in PBSP XLMI space at addresses 0x00000:0x08000
[BAR0 addresses 0x103000:0x103200]. They are:

=============== ============ =========== ==============
XLMI            MMIO         Name        Description
=============== ============ =========== ==============
0x00000         0x103000     PARM_0      :ref:`parameters from sequence/picture parameter structs and the slice header <pbsp-io-vld-parm>`
0x00100         0x103004     PARM_1      :ref:`parameters from sequence/picture parameter structs and the slice header <pbsp-io-vld-parm>`
0x00200         0x103008     MB_POS      :ref:`position of the current macroblock <pbsp-io-vld-mb-pos>`
0x00300         0x10300c     COMMAND     :ref:`writing executes a VLD command <pbsp-io-vld-command>`
0x00400         0x103010     STATUS      :ref:`shows busy status of various parts of the VLD <pbsp-io-vld-status>`
0x00500         0x103014     RESULT      :ref:`result of a command <pbsp-io-vld-result>`
0x00700         0x10301c     INTR_EN     :ref:`interrupt enable mask <pbsp-io-vld-intr>`
0x00800         0x103020     ???         ???
0x00900         0x103024     INTR        :ref:`interrupt status <pbsp-io-vld-intr>`
0x00a00         0x103028     RESET       :ref:`resets the VLD and its registers to initial state <pbsp-io-vld-reset>`
0x01000+i*0x100 0x103040+i*4 CONF[0:8]   :ref:`length and enable bit of stream buffer i <pbsp-io-vld-stream>`
0x01100+i*0x100 0x103044+i*4 OFFSET[0:8] :ref:`offset of stream buffer i <pbsp-io-vld-stream>`
0x02000         0x103080     BITPOS      :ref:`the bit position in the stream <pbsp-io-vld-stream>`
0x04000         0x103100     OFFSET      :ref:`the MBRING offset <pbsp-io-vld-mbring>`
0x04100         0x103104     HALT_POS    :ref:`the MBRING halt position <pbsp-io-vld-mbring>`
0x04200         0x103108     WRITE_POS   :ref:`the MBRING write position <pbsp-io-vld-mbring>`
0x04300         0x10310c     SIZE        :ref:`the MBRING size <pbsp-io-vld-mbring>`
0x04400         0x103110     TRIGGER     :ref:`writing executes MBRING commands <pbsp-io-vld-mbring>`
=============== ============ =========== ==============

.. todo:: reg 0x00800


.. _pbsp-io-vld-reset:

Reset
=====

The engine may be reset at any time by poking the RESET register.

BAR0 0x103028 / XLMI 0x00a00: RESET
  Any write will cause the VLD to be reset. All internal state is reset to
  default values. All user-writable registers are set to 0, except UNK8
  which is set to 0xffffffff.


.. _pbsp-io-vld-parm:
.. _pbsp-io-vld-mb-pos:

Parameter and position registers
================================

The values of these registers are used by some of the VLD commands. PARM
registers should be initialised with values derived from sequence parameters,
picture parameters, and slice header. MB_POS should be set to the address of
currently processed macroblock [for single-macroblock operation] or the first
macroblock of the slice [for whole-slice operation]. In whole-slice operation,
MB_POS is updated by the hardware to the position of the last macroblock in
the parsed slice.

For details on use of this information by specific commands, see their
documentation.

BAR0 0x103000 / XLMI 0x00000: PARM_0
  - bit 0: entropy_coding_mode_flag - set as in picture parameters
  - bits 1-8: width_mbs - set to pic_width_in_mbs_minus1 + 1
  - bit 9: mbaff_frame_flag - set to (mb_adaptive_frame_field_flag && !field_pic_flag)
  - bits 10-11: picture_structure - one of:

    - 0: frame - set if !field_pic_flag
    - 1: top field - set if field_pic_flag && !bottom_field_flag
    - 2: bottom field - set if field_pic_flag && bottom_field_flag

  - bits 12-16: nal_unit_type - set as in the slice NAL header [XXX: check]
  - bit 17: constrained_intra_pred - set as in picture parameters [XXX: check]
  - bits 18-19: cabac_init_idc - set as in slice header, for P and B slices
  - bits 20-21: chroma_format_idc - if parsing auxiliary coded picture, set to 0,
    otherwise set as in sequence parameters
  - bit 22: direct_8x8_inference_flag - set as in sequence parameters
  - bit 23: transform_8x8_mode_flag - set as in picture parameters

BAR0 0x103004 / XLMI 0x00100: PARM_1
  - bits 0-1: slice_type - set as in slice header
  - bits 2-14: slice_tag - used to tag macroblocks in internal state with their
    slices, for determining availability status in CABAC/CAVLC context
    selection algorithms. See command description.
  - bits 15-19: num_ref_idx_l0_active_minus1 - set as in slice header, for P
    and B slices
  - bits 20-24: num_ref_idx_l1_active_minus1 - set as in slice header, for B
    slices
  - bits 25-30: sliceqpy - set to (pic_init_qp_minus26 + 26 + slice_qp_delta)

BAR0 0x103008 / XLMI 0x00200: MB_POS
  - bits 0-12: addr - address of the macroblock
  - bits 13-20: x - x coordinate of the macroblock in macroblock units
  - bits 21-28: y - y coordinate of the macroblock in macroblock units
  - bit 29: first - 1 if the described macroblock is the first macroblock in its
    slice, 0 otherwise


Internal state for context selection
====================================

Both CAVLC and CABAC sometimes use decoded data of previous macroblocks in the
slice to determine the decoding algorithm for syntax elements of the current
macroblock. The VLD thus stores this data in its internal hidden memory.

.. todo:: what macroblocks are stored, indexing, tagging, reset state

For each macroblock, the following data is stored:

- slice_tag
- mb_field_decoding_flag
- mb_skip_flag
- mb_type
- coded_block_pattern
- transform_size_8x8_flag
- intra_chroma_pred_mode
- ref_idx_lX[i]
- mvd_lX[i][j]
- coded_block_flag for each block
- total_coeffs for each luma 4x4 / luma AC block

.. todo:: and availability status?

Additionally, the following data of the previous decoded macroblock [not
indexed by macroblock address] is stored:

- mb_qp_delta


.. _pbsp-intr-vld:
.. _pbsp-io-vld-intr:

Interrupts
==========

.. todo:: write me

BAR0 0x10301c / XLMI 0x00700: INTR_EN
  - bit 0: UNK_INPUT_1
  - bit 1: END_OF_STREAM
  - bit 2: UNK_INPUT_3
  - bit 3: MBRING_HALT
  - bit 4: SLICE_DATA_DONE

BAR0 0x103024 / XLMI 0x00900: INTR
  - bits 0-3: INPUT
    - 0: no interrupt pending
    - 1: UNK_INPUT_1
    - 2: END_OF_STREAM
    - 3: UNK_INPUT_3
    - 4: SLICE_DATA_DONE
  - bit 4: MBRING_FULL


.. _pbsp-io-vld-stream:

Stream input
============

.. todo:: RE and write me


.. _pbsp-io-vld-mbring:

MBRING output
=============

.. todo:: write me


.. _pbsp-io-vld-command:
.. _pbsp-io-vld-status:
.. _pbsp-io-vld-result:

Command and status registers
============================

.. todo:: write me


Command 0: GET_UE
-----------------

Parameter:
    none
Result:
    the decoded value of parsed bitfield, or 0xffffffff if out of range

Parses one ue(v) element as defined in H.264 spec. Only elements in range
0..0xfffe [up to 31 bits in the bitstream] are supported by this command.
If the next bits of the bitstream are a valid ue(v) element in supported
range, the element is parsed, the bitstream pointer advances past it, and
its parsed value is returned as the result. Otherwise, bitstream pointer
is not modified and 0xffffffff is returned.

Operation::

    if (nextbits(16) != 0) {
        int bitcnt = 0;
        while (getbits(1) == 0)
            bitcnt++;
        return (1 << bitcnt) - 1 + getbits(bitcnt);
    } else {
        return 0xffffffff;
    }


Command 1: GET_SE
-----------------

Parameter:
    none
Result:
    the decoded value of parsed bitfield, or 0x80000000 if out of range

Parses one se(v) element as defined in H.264 spec. Only elements in range
-0x7fff..0x7fff [up to 31 bits in the bitstream] are supported by this command.
If the next bits of the bitstream are a valid se(v) element in supported
range, the element is parsed, the bitstream pointer advances past it, and
its parsed value is returned as the result. Otherwise, bitstream pointer
is not modified and 0x80000000 is returned.

Operation::

    if (nextbits(16) != 0) {
        int bitcnt = 0;
        while (getbits(1) == 0)
            bitcnt++;
        int tmp = (1 << bitcnt) - 1 + getbits(bitcnt);
        if (tmp & 1)
            return (tmp+1) >> 1;
        else
            return -(tmp >> 1);
    } else {
        return 0x80000000;
    }


Command 2: GETBITS
------------------

Parameter:
    number of bits to read, or 0 to read 32 bits [5 bits]
Result:
    the bits from the bitstream

Given parameter n, returns the next (n?n:32) bits from the bitstream as an
unsigned integer.

Operation:: 

    return getbits(n?n:32);


Command 3: NEXT_START_CODE
--------------------------

Parameter:
    none
Result:
    the next start code found

Skips bytes in the raw bitstream until the start code [00 00 01] is found.
Then, read the byte after the start code and return it as the result. The
bitstream pointer is advanced to point after the returned byte.

Operation::

    byte_align();
    while (nextbytes_raw(3) != 1)
        getbits_raw(8);
    getbits_raw(24);
    return getbits_raw(8);


Command 4: CABAC_START
----------------------

Parameter:
    none
Result:
    none

Skips bits in the bitstream until the current bit position is byte-aligned,
then initialises the arithmetic decoding engine registers codIRange and
codIOffset, as per H.264.9.3.1.2.

Oprtation::

    byte_align();
    cabac_init_engine();


Command 5: MORE_RBSP_DATA
-------------------------

Parameter:
    none
Result:
    1 if there's more data in RBSP, 0 otherwise

Returns 0 if there's a valid RBSP trailing bits element at the current bit
position, 1 otherwise. Does not modify the bitstream pointer.

Operation::

    return more_rbsp_data();


Command 6: MB_SKIP_FLAG
-----------------------

Parameter:
    none
Result:
    value of parsed mb_skip_flag

Parses the CABAC mb_skip_flag element. The SLICE_POS has to be set to the
address of the macroblock to which this element applies.

Operation::

    return cabac_mb_skip_flag();


Command 7: END_OF_SLICE_FLAG
----------------------------

Parameter:
    none
Result:
    value of parsed end_of_slice_flag

Parses the CABAC end_of_slice_flag element.

Operation::

    return cabac_terminate();


Command 8: CABAC_INIT_CTX
-------------------------

Parameter:
    none
Result:
    none

Initialises the CABAC context variables, as per H.264.9.3.1.1. slice_type,
cabac_init_idc [for P/B slices], and sliceqpy have to be set in the PARM
registers for this command to work properly.

Operation::

    cabac_init_ctx();


Command 9: MACROBLOCK_SKIP_MBFDF
--------------------------------

Parameter:
    mb_field_decoding_flag presence [1 bit]
Result
    none

If parameter is 1, mb_field_decoding_flag syntax element is parsed. Otherwise,
the value of mb_field_decoding_flag is inferred from preceding macroblocks.
A skipped macroblock with thus determined value of mb_field_decoding_flag
is emitted into the MBRING, and its data stored into internal state. SLICE_POS
has to be set to the address of this macroblock.

Operation::

    if (param) {
        if (entropy_coding_mode_flag)
            this_mb.mb_field_decoding_flag = cabac_mb_field_decoding_flag();
        else
            this_mb.mb_field_decoding_flag = getbits(1);
    } else {
        this_mb.mb_field_decoding_flag = mb_field_decoding_flag_infer();
    }
    this_mb.mb_skip_flag = 1;
    this_mb.slice_tag = slice_tag;
    mbring_emit_macroblock();

.. todo:: more inferred crap


Command 0xa: MACROBLOCK_LAYER_MBFDF
-----------------------------------

Parameter:
    mb_field_decoding_flag presence [1 bit]
Result:
    none

If parameter is 1, mb_field_decoding_flag syntax element is parsed. Otherwise,
the value of mb_field_decoding_flag is inferred from preceding macroblocks.
A macroblock_layer syntax structure is parsed from the bitstream, data for the
decoded macroblock is emitted into the MBRING, and stored into internal state.
SLICE_POS has to be set to the address of this macroblock.

Operation::

    if (param) {
        if (entropy_coding_mode_flag)
            this_mb.mb_field_decoding_flag = cabac_mb_field_decoding_flag();
        else
            this_mb.mb_field_decoding_flag = getbits(1);
    } else {
        this_mb.mb_field_decoding_flag = mb_field_decoding_flag_infer();
    }
    this_mb.mb_skip_flag = 0;
    this_mb.slice_tag = slice_tag;
    macroblock_layer();


Command 0xb: PRED_WEIGHT_TABLE
------------------------------

Parameter:
    none
Result:
    none

Parses the pred_weight_table element, stores its contents in internal memory,
and advances the bitstream to the end of the element.

Operation:

.. todo:: write me


Command 0xc: SLICE_DATA
-----------------------

Parameter:
    none
Result: 
    none

Writes the stored pred_weight_table data to MBRING, parses the slice_data
element, storing decoded data into MBRING, halting when the RBSP trailing bit
sequence is encountered. When done, raises the MACROBLOCKS_DONE interrupt.
Bitstream pointer is updated to point to the RBSP traling bits. SLICE_POS has
to be set to the address of the first macroblock on slice before this command
is called. When this command finishes, SLICE_POS is updated to the address of
the last macroblock in the parsed slice.

Operation::

    if (entropy_coding_mode_flag) {
        cabac_init_ctx();
        byte_align();
        cabac_init_engine();
    }
    mb_pos.first = 1;
    first = 1;
    skip_pending = 0;
    end = 0;
    bottom = 0;
    while (1) {
        if (slice_type == P || slice_type == B) {
            if (entropy_coding_mode_flag) {
                while (1) {
                    tmp = cabac_mb_skip_flag();
                    if (!tmp)
                        break;
                    skip_pending++;
                    if (!mbaff_frame_flag || bottom) {
                        end = cabac_terminate();
                        if (end)
                            break;
                    }
                    bottom = !bottom;
                }
            } else {
                skip_pending = get_ue();
                end = !more_rbsp_data();
                bottom ^= skip_pending & 1;
            }
        } else {
            skip_pending = 0;
        }
        while (1) {
            if (!skip_pending)
                break;
            if (mbaff_frame_flag && bottom && skip_pending < 2)
                break;
            if (first) {
                first = 0;
            } else {
                mb_pos_advance();
            }
            macroblock_skip_mbfdf(0);
            skip_pending--;
        }
        if (end)
            break;
        if (first) {
            first = 0;
        } else {
            mb_pos_advance();
        }
        if (mbaff_frame_flag) {
            if (skip_pending) {
                macroblock_skip_mbfdf(1);
                mb_pos_advance();
                macroblock_layer_mbfdf(0);
                skip_pending = 0;
            } else {
                if (bottom) {
                    macroblock_layer_mbfdf(0);
                } else {
                    macroblock_layer_mbfdf(1);
                }
            }
            bottom = !bottom;
        } else {
            macroblock_layer_mbfdf(0);
        }
        if (entropy_coding_mode) {
            if (mbaff_frame_flag && bottom)) {
                end = 0;
            } else {
                end = cabac_terminate();
            }
        } else {
            end = !more_rbsp_data();
        }
        if (end) break;
    }
    trigger_intr(SLICE_DATA_DONE);
