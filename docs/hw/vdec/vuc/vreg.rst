===============================
VP2/VP3/VP4 vµc video registers
===============================

2. The video MMIO registers
3. $parm register
4. The RPIs and rpitab
5. Macroblock input: mbiread, mbinext
6. Table lookup instruction: lut


Introduction
============

.. todo:: write me


The video special registers
===========================

.. todo:: the following information may only be valid for H.264 mode for now

- $sr0: ??? controls $sr48-$sr58 (bits 0-6 when set separately) [XXX] [VP2
  only]
- $sr1: ??? similar to $sr0 (bits 0-4, probably more) [XXX] [VP2 only]
- $sr2/$spidx: partition and subpartition index, used to select which
  [sub]partitions some other special registers access:

  - bits 0-1: subpartition index
  - bits 2-3: partition index

  Note that, for indexing purposes, each partition index is considered
  to refer to an 8x8 block, and each subpartition index to 4x4 block. If
  partition/subpartition size is bigger than that, the indices will
  alias. Thus, for 16x8 partitioning mode, $spidx equal to 0-7 wil select
  the top partition, $spidx equal to 8-15 will select the bottom one. For
  8x16 partitioning, $spidx equal to 0-3 and 8-11 will select the left
  partition, 4-7 and 12-15 will select the right partition.

- $sr3: ??? bits 0-4 affect $sr32-$sr42 [XXX] [VP2 only]
- $sr3: ??? [XXX] [VP3+ only]
- $sr4/$h2v: a scratch register to pass data from host to vµc [see
  vdec/vuc/intro.txt]
- $sr5/$v2h: a scratch register to pass data from vµc to host [see
  vdec/vuc/intro.txt]
- $sr6/$stat: some sort of control/status reg, writing 0x8000 alternates
  values between 0x8103 and 0 [XXX]

  - bit 10: macroblock input available - set whenever there's a complete
    macroblock available from MBRING, cleared when mbinext instruction
    skips past the last currently available macroblock. Will break out
    of sleep instruction when set.
  - bit 11: $h2v modified - set when host writes H2V, cleared when $h2v
    is read by vµc, will break out of sleep instruction when set.
  - bit 12: watchdog hit - set 1 cycle after $icnt reaches WDCNT and it's
    not equal to 0xffff, cleared when $icnt or WDCNT is modified in any
    way.

- $sr7/$parm: sequence/picture/slice parameters required by vµc hardware
  [see vdec/vuc/intro.txt]
- $sr9/$cspos: call stack position, 0-8. Equal to the number of used entries
  on the stack.
- $sr10/$cstop: call stack top. Writing to this register causes the written
  value to be pushed onto the stack, reading this register pops a value off
  the stack and returns it.
- $sr11/$rpitab: D[] address of refidx -> dpb index translation table [VP2
  only]
- $sr15/$icnt: instruction/cycle counter (?: check nops, effect of delays)
- $sr16/$mvxl0: sign-extended mvd_l0[$spidx][0] [input]
- $sr17/$mvyl0: sign-extended mvd_l0[$spidx][1] [input]
- $sr18/$mvxl1: sign-extended mvd_l1[$spidx][0] [input]
- $sr19/$mvyl1: sign-extended mvd_l1[$spidx][1] [input]
- $sr20/$refl0: ref_idx_l0[$spidx>>2] [input]
- $sr21/$refl1: ref_idx_l1[$spidx>>2] [input]
- $sr22/$rpil0: dpb index of L0 reference picture for $spidx-selected
  partition
- $sr23/$rpil1: dpb index of L1 reference picture for $spidx-selected
  partition
- $sr24/$mbflags:

  - bit 0: mb_field_decoding_flag [RW]
  - bit 1: is intra macroblock [RO]
  - bit 2: is I_NxN macroblock [RO]
  - bit 3: transform_size_8x8_flag [RW]
  - bit 4: ??? [XXX]
  - bit 5: is I_16x16 macroblock [RO]
  - bit 6: partition selected by $spidx uses L0 or Bi prediction [RO]
  - bit 7: partition selected by $spidx uses L1 or Bi prediction [RO]
  - bit 8: mb_field_decoding_flag for next macroblock [only valid if $sr6
    bit 10 is set] [RO]
  - bit 9: mb_skip_flag for next macroblock [only valid if $sr6 bit 10 is
    set] [RO]
  - bit 10: partition selected by $spidx uses Direct prediction [RO]
  - bit 11: any partition of macroblock uses Direct prediction [RO]
  - bit 12: is I_PCM macroblock [RO]
  - bit 13: is P_SKIP macroblock [RO]

- $sr25/$qpy:

  - bits 0-5: mb_qp_delta [input] / QPy [output] [H.264]
  - bits 0-5: quantiser_scale_code [input and output] [MPEG1/MPEG2]
  - bits 8-11: intra_chroma_pred_mode, values:

    - 0: DC [input], DC_??? [output] [XXX]
    - 1: horizontal [input, output]
    - 2: vertical [input, output]
    - 3: plane [input, output]
    - 4: DC_??? [output]
    - 5: DC_??? [output]
    - 6: DC_??? [output]
    - 7: DC_??? [output]
    - 8: DC_??? [output]
    - 9: DC_??? [output]
    - 0xa: DC_??? [output]

- $sr26/$qpc:

  - bits 0-5: QPc for Cb [output] [H.264]
  - bits 8-13: QPc for Cr [output] [H.264]

- $sr27/$mbpart:
  - bits 0-1: macroblock partitioning type

    - 0: 16x16
    - 1: 16x8
    - 2: 8x16
    - 3: 8x8

  - bits 2-3: partition 0 subpartitioning type
  - bits 4-5: partition 0 subpartitioning type
  - bits 6-7: partition 0 subpartitioning type
  - bits 8-9: partition 0 subpartitioning type

    - 0: 8x8
    - 1: 8x4
    - 2: 4x8
    - 3: 4x4

- $sr28/$mbxy:

  - bits 0-7: macroblock Y position
  - bits 8-15: macroblock X position

- $sr29/$mbaddr:

  - bits 0-12: macroblock address
  - bit 15: first macroblock in slice flag

- $sr30/$mbtype: macroblock type, for H.264:

  - 0x00: I_NxN
  - 0x01: I_16x16_0_0_0
  - 0x02: I_16x16_1_0_0
  - 0x03: I_16x16_2_0_0
  - 0x04: I_16x16_3_0_0
  - 0x05: I_16x16_0_1_0
  - 0x06: I_16x16_1_1_0
  - 0x07: I_16x16_2_1_0
  - 0x08: I_16x16_3_1_0
  - 0x09: I_16x16_0_2_0
  - 0x0a: I_16x16_1_2_0
  - 0x0b: I_16x16_2_2_0
  - 0x0c: I_16x16_3_2_0
  - 0x0d: I_16x16_0_0_1
  - 0x0e: I_16x16_1_0_1
  - 0x0f: I_16x16_2_0_1
  - 0x10: I_16x16_3_0_1
  - 0x11: I_16x16_0_1_1
  - 0x12: I_16x16_1_1_1
  - 0x13: I_16x16_2_1_1
  - 0x14: I_16x16_3_1_1
  - 0x15: I_16x16_0_2_1
  - 0x16: I_16x16_1_2_1
  - 0x17: I_16x16_2_2_1
  - 0x18: I_16x16_3_2_1
  - 0x19: I_PCM
  - 0x20: P_L0_16x16
  - 0x21: P_L0_L0_16x8
  - 0x22: P_L0_L0_8x16
  - 0x23: P_8x8
  - 0x24: P_8x8ref0
  - 0x40: B_Direct_16x16
  - 0x41: B_L0_16x16
  - 0x42: B_L1_16x16
  - 0x43: B_Bi_16x16
  - 0x44: B_L0_L0_16x8
  - 0x45: B_L0_L0_8x16
  - 0x46: B_L1_L1_16x8
  - 0x47: B_L1_L1_8x16
  - 0x48: B_L0_L1_16x8
  - 0x49: B_L0_L1_8x16
  - 0x4a: B_L1_L0_16x8
  - 0x4b: B_L1_L0_8x16
  - 0x4c: B_L0_Bi_16x8
  - 0x4d: B_L0_Bi_8x16
  - 0x4e: B_L1_Bi_16x8
  - 0x4f: B_L1_Bi_8x16
  - 0x50: B_Bi_L0_16x8
  - 0x51: B_Bi_L0_8x16
  - 0x52: B_Bi_L1_16x8
  - 0x53: B_Bi_L1_8x16
  - 0x54: B_Bi_Bi_16x8
  - 0x55: B_Bi_Bi_8x16
  - 0x56: B_8x8
  - 0x7e: B_SKIP
  - 0x7f: P_SKIP

- $sr31/$submbtype: [VP2 only]

  - bits 0-3: sub_mb_type[0]
  - bits 4-7: sub_mb_type[1]
  - bits 8-11: sub_mb_type[2]
  - bits 12-15: sub_mb_type[3]

- $sr31: ??? [XXX] [VP3+ only]
- $sr32-$sr40: ??? affected by $sr3, unko21, read only [XXX]
- $sr41-$sr42: ??? affected by $sr3, unko21, read only [XXX] [VP2 only]
- $sr48-$sr58: ??? affected by writing $sr0 and $sr1, unko22, read only [XXX]


Table lookup instruction: lut
=============================

Performs a lookup of src1 in the lookup table selected by low 4 bits of src2.
The tables are codec-specific and generated by hardware from the current
contents of the video special registers.

.. todo:: recheck this instruction on VP3 and other codecs

Tables 0-3 are an alternate way of accessing H.264 inter prediction registers
[$sr16-$sr23]. The table index is 1-bit. Index 0 selects the l0 register,
index 1 selects the l1 register. Table 0 is $mvxl* registers, 1 is $mvyl*, 2
is $refl*, 3 is $rpil*.

Tables 4-7 behave like tables 0-3, except the lookup returns 0 if $mbtype is
equal to 0x7f [P_SKIP].

Table 8, known as pcnt, is used to look up partition and subpartition counts.
The index is 3-bit. Indices 0-3 return the subpartition count of corresponding
partition, while indices 4-7 return the partition count of the macroblock.

Tables 9 and 10 are indexed in a special manner: the index selects a partition
and a subpartition. Bits 0-7 of the index are partition index, bits 8-15 of
the index are subpartition index. The partition and subpartition indices
bahave as in the H.264 spec: valid indices are 0, 0-1, or 0-3 depending on the
partitioning/subpartitioning mode.

Table 9, known as spidx, translates indices of the form given above into
$spidx values. If both partition and subpartition index are valid for the
current partitioning and subpartitioning mode, the value returned is the value
that has to be poked into $spidx to access the selected [sub]partition.
Otherwise, junk may be returned.

Table 10, known as pnext, advances the partition/subpartition index to the
next valid subpartition or partition. The returned value is an index in the
same format as the input index. Additionally, the predicate output is set
if the partition index was not incremented [transition to the next
subpartition of a partition], cleared if the partition index was incremented
[transition to the first subpartition of the next partition].

Table 11, known as pmode, returns the inter prediction mode for a given
partition. The index is 2-bit and selects the partition. If index is less then
pcnt[4] and $mbtype is inter-predicted, returns inter prediction mode,
otherwise returns 0. The prediction modes are:

- 0 direct
- 1 L0
- 2 L1
- 3 Bi

Tables 12-15 are unused and always return 0. [XXX: 12 used for VC-1 on VP3]

Instructions:
    lut pdst, dst, src1, src2   OP=11100
Opcode: base opcode, OP as above
Operation::

    /* helper functions */
    int pcnt() {
        switch ($mbtype) {
            case 0:     /* I_NxN */
            case 0x19:  /* I_PCM */
                return 4;
            case 1..0x18:   /* I_16x16_* */
                return 1;
            case 0x20:  /* P_L0_16x16 */
                return 1;
            case 0x21:  /* P_L0_L0_16x8 */
            case 0x22:  /* P_L0_L0_8x16 */
                return 2;
            case 0x23:  /* P_8x8 */
            case 0x24:  /* P_8x8ref0 */
                return 4;
            case 0x40:  /* B_Direct_16x16 */
            case 0x41:  /* B_L0_16x16 */
            case 0x42:  /* B_L1_16x16 */
            case 0x43:  /* B_Bi_16x16 */
                return 1;
            case 0x44:  /* B_L0_L0_16x8 */
            case 0x45:  /* B_L0_L0_8x16 */
            case 0x46:  /* B_L1_L1_16x8 */
            case 0x47:  /* B_L1_L1_8x16 */
            case 0x48:  /* B_L0_L1_16x8 */
            case 0x49:  /* B_L0_L1_8x16 */
            case 0x4a:  /* B_L1_L0_16x8 */
            case 0x4b:  /* B_L1_L0_8x16 */
            case 0x4c:  /* B_L0_Bi_16x8 */
            case 0x4d:  /* B_L0_Bi_8x16 */
            case 0x4e:  /* B_L1_Bi_16x8 */
            case 0x4f:  /* B_L1_Bi_8x16 */
            case 0x50:  /* B_Bi_L0_16x8 */
            case 0x51:  /* B_Bi_L0_8x16 */
            case 0x52:  /* B_Bi_L1_16x8 */
            case 0x53:  /* B_Bi_L1_8x16 */
            case 0x54:  /* B_Bi_Bi_16x8 */
            case 0x55:  /* B_Bi_Bi_8x16 */
                return 2;
            case 0x56:  /* B_8x8 */
                return 4;
            case 0x7e:  /* B_SKIP */
                return 4;
            case 0x7f:  /* P_SKIP */
                return 1;
            /* in other cases returns junk */
        }
    }
    int spcnt(int idx) {
        if (pcnt() < 4) {
            return 1;
        } else if ($mbtype == 0 || $mbtype == 0x19) { /* I_NxN or I_PCM */
            return ($mbflags[3:3] ? 1 : 4); /* transform_size_8x8_flag */
        } else {
            smt = $submbtype >> (idx * 4)) & 0xf;
            /* XXX */
        }
    }
    int mbpartmode_16x8() {
        switch ($mbtype) {
            case 0x21: /* P_L0_L0_16x8 */
            case 0x44: /* B_L0_L0_16x8 */
            case 0x46: /* B_L1_L1_16x8 */
            case 0x48: /* B_L0_L1_16x8 */
            case 0x4a: /* B_L1_L0_16x8 */
            case 0x4c: /* B_L0_Bi_16x8 */
            case 0x4e: /* B_L1_Bi_16x8 */
            case 0x50: /* B_Bi_L0_16x8 */
            case 0x52: /* B_Bi_L1_16x8 */
            case 0x54: /* B_Bi_Bi_16x8 */
                return 1;
            default:
                return 0;
        }
    }
    int submbpartmode_8x4(int idx) {
        smt = $submbtype >> (idx * 4) & 0xf;
        switch(submbtype) {
            /* XXX */
        }
    }
    int mbpartpredmode(int idx) {
        /* XXX */
    }
    /* end of helper functions */
    table = src2 & 0xf;
    if (table < 8) {
        which = src1 & 1;
        switch (table & 3) {
            case 0: result = (which ? $mvxl1 : $mvxl0); break;
            case 1: result = (which ? $mvyl1 : $mvyl0); break;
            case 2: result = (which ? $refl1 : $refl0); break;
            case 3: result = (which ? $rpil1 : $rpil0); break;
        }
        if ((table & 4) && $mbtype == 0x7f)
            result = 0;
        presult = result & 1;
    } else if (table == 8) {    /* pcnt */
        idx = src1 & 7;
        if (idx < 4) {
            result = spcnt(idx);
        } else {
            result = pcnt();
        }
    } else if (table == 9 || table == 10) {
        pidx = src1 & 7;
        sidx = src1 >> 8 & 3;
        if (table == 9) {   /* spidx */
            if (mbpartmode_16x8())
                resp = (pidx & 1) << 1;
            else
                resp = (pidx & 3);
            if (submbpartmode_8x4(resp >> 2))
                ress = (sidx & 1) << 1;
            else
                ress = (sidx & 3);
            result = resp << 2 | ress;
            presult = result & 1;
        } else {        /* pnext */
            if (pidx < 4) {
                c = spcnt(idx);
            } else {
                c = pcnt();
            }
            ress = sidx + 1;
            if (ress >= c) {
                resp = (pidx & 3) + 1;
                ress = 0;
            } else {
                resp = pidx & 3;
            }
            result = ress << 8 | resp;
            presult = ress != 0;
        }
    } else if (table == 10) {   /* pmode */
        result = mbpartpredmode(src1 & 3);
        presult = result & 1;
    } else {
        result = 0;
        presult = 0;
    }
    dst = result;
    pdst = presult;
Execution time: 1 cycle
Predicate output:
    Tables 0-9 and 11-15: bit 0 of the result
    Table 10: 1 if transition to next subpartition in a partition, 0 if
          transition to next partition


