#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <vdpau/vdpau.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include "util.h"

/* bsp structs */
struct mpeg12_picparm_bsp {
	uint16_t width;
	uint16_t height;
	uint8_t picture_structure;
	uint8_t picture_coding_type;
	uint8_t intra_dc_precision;
	uint8_t frame_pred_frame_dct;
	uint8_t concealment_motion_vectors;
	uint8_t intra_vlc_format;
	uint16_t pad;
	uint8_t f_code[2][2];
};

struct mpeg4_picparm_bsp {
	uint16_t width;
	uint16_t height;
	uint8_t vop_time_increment_size;
	uint8_t interlaced;
	uint8_t resync_marker_disable;
};

struct vc1_picparm_bsp {
	uint16_t width;
	uint16_t height;
	uint8_t profile; // 04 0 simple, 1 main, 2 advanced
	uint8_t postprocflag; // 05
	uint8_t pulldown; // 06
	uint8_t interlaced; // 07
	uint8_t tfcntrflag; // 08
	uint8_t finterpflag; // 09
	uint8_t psf; // 0a
	uint8_t pad; // 0b
	uint8_t multires; // 0c
	uint8_t syncmarker; // 0d
	uint8_t rangered; // 0e
	uint8_t maxbframes; // 0f
	uint8_t dquant; // 10
	uint8_t panscan_flag; // 11
	uint8_t refdist_flag; // 12
	uint8_t quantizer; // 13
	uint8_t extended_mv; // 14
	uint8_t extended_dmv; // 15
	uint8_t overlap; // 16
	uint8_t vstransform; // 17
};

struct h264_picparm_bsp {
	// 00
	uint32_t unk00;
	// 04
	uint32_t log2_max_frame_num_minus4; // 04 checked
	uint32_t pic_order_cnt_type; // 08 checked
	uint32_t log2_max_pic_order_cnt_lsb_minus4; // 0c checked
	uint32_t delta_pic_order_always_zero_flag; // 10, or unknown

	uint32_t frame_mbs_only_flag; // 14, always 1?
	uint32_t direct_8x8_inference_flag; // 18, always 1?
	uint32_t width_mb; // 1c checked
	uint32_t height_mb; // 20 checked
	// 24
	//struct picparm2
		uint32_t entropy_coding_mode_flag; // 00, checked
		uint32_t pic_order_present_flag; // 04 checked
		uint32_t unk; // 08 seems to be 0?
		uint32_t pad1; // 0c seems to be 0?
		uint32_t pad2; // 10 always 0 ?
		uint32_t num_ref_idx_l0_active_minus1; // 14 always 0?
		uint32_t num_ref_idx_l1_active_minus1; // 18 always 0?
		uint32_t weighted_pred_flag; // 1c checked
		uint32_t weighted_bipred_idc; // 20 checked
		uint32_t pic_init_qp_minus26; // 24 checked
		uint32_t deblocking_filter_control_present_flag; // 28 always 1?
		uint32_t redundant_pic_cnt_present_flag; // 2c always 0?
		uint32_t transform_8x8_mode_flag; // 30 checked
		uint32_t mb_adaptive_frame_field_flag; // 34 checked-ish
		uint8_t field_pic_flag; // 38 checked
		uint8_t bottom_field_flag; // 39 checked
		uint8_t real_pad[0x1b]; // XX why?
};

/* vp structs */
struct mpeg12_picparm_vp {
	uint16_t width; // 00 in mb units
	uint16_t height; // 02 in mb units

	uint32_t unk04; // 04 stride for Y?
	uint32_t unk08; // 08 stride for CbCr?

	uint32_t ofs[6]; // 1c..20 ofs
	uint32_t bucket_size; // 24
	uint32_t inter_ring_data_size; // 28
	uint16_t unk2c; // 2c
	uint16_t alternate_scan; // 2e
	uint16_t unk30; // 30 not seen set yet
	uint16_t picture_structure; // 32
	uint16_t pad2[3];
	uint16_t unk3a; // 3a set on I frame?

	uint32_t f_code[4]; // 3c
	uint32_t picture_coding_type; // 4c
	uint32_t intra_dc_precision; // 50
	uint32_t q_scale_type; // 54
	uint32_t top_field_first; // 58
	uint32_t full_pel_forward_vector; // 5c
	uint32_t full_pel_backward_vector; // 60
	uint8_t intra_quantizer_matrix[0x40]; // 64
	uint8_t non_intra_quantizer_matrix[0x40]; // a4
};

struct mpeg4_picparm_vp {
	uint32_t width; // 00 in normal units
	uint32_t height; // 04 in normal units
	uint32_t unk08; // stride 1
	uint32_t unk0c; // stride 2
	uint32_t ofs[6]; // 10..24 ofs
	uint32_t bucket_size; // 28
	uint32_t pad1; // 2c, pad
	uint32_t pad2; // 30
	uint32_t inter_ring_data_size; // 34

	uint32_t trd[2]; // 38, 3c
	uint32_t trb[2]; // 40, 44
	uint32_t u48; // XXX codec selection? Should test with different values of VdpDecoderProfile
	uint16_t f_code_fw; // 4c
	uint16_t f_code_bw; // 4e
	uint8_t interlaced; // 50

	uint8_t quant_type; // bool, written to 528
	uint8_t quarter_sample; // bool, written to 548
	uint8_t short_video_header; // bool, negated written to 528 shifted by 1
	uint8_t u54; // bool, written to 0x740
	uint8_t vop_coding_type; // 55
	uint8_t rounding_control; // 56
	uint8_t alternate_vertical_scan_flag; // 57 bool
	uint8_t top_field_first; // bool, written to vuc

	uint8_t pad4[3]; // 59, 5a, 5b, contains garbage on blob

	uint32_t intra[0x10]; // 5c
	uint32_t non_intra[0x10]; // 9c

	uint32_t pad5[8]; // bc... onwards non-inclusive, but WHY?

	// udc..uff pad?
};

// Full version, with data pumped from BSP
struct vc1_picparm_vp {
	uint32_t bucket_size; // 00
	uint32_t pad; // 04

	uint32_t inter_ring_data_size; // 08
	uint32_t unk0c; // stride 1
	uint32_t unk10; // stride 2
	uint32_t ofs[6]; // 14..28 ofs

	uint16_t width; // 2c
	uint16_t height; // 2e

	uint8_t profile; // 30 0 = simple, 1 = main, 2 = advanced
	uint8_t loopfilter; // 31 written into vuc
	uint8_t u32; // 32, written into vuc
	uint8_t dquant; // 33

	uint8_t overlap; // 34
	uint8_t quantizer; // 35
	uint8_t u36; // 36, bool
	uint8_t pad2; // 37, to align to 0x38
};

struct h264_picparm_vp { // 700..a00
	uint16_t width, height;
	uint32_t stride1, stride2; // 04 08
	uint32_t ofs[6]; // 0c..24 in-image offset

	uint32_t tmp_stride;
	uint32_t bucket_size; // 28 bucket size
	uint32_t inter_ring_data_size; // 2c

	union {
		struct {
			unsigned mb_adaptive_frame_field_flag : 1; // 0
			unsigned direct_8x8_inference_flag : 1; // 1 0x02: into vuc ofs 56
			unsigned weighted_pred_flag : 1; // 2 0x04
			unsigned constrained_intra_pred_flag : 1; // 3 0x08: into vuc ofs 68
			unsigned is_reference : 1; // 4
			unsigned interlace : 1; // 5 field_pic_flag
			unsigned bottom_field_flag : 1; // 6
			unsigned second_field : 1; // 7 0x80: nfi yet

			signed log2_max_frame_num_minus4 : 4; // 31 0..3
			unsigned chroma_format_idc : 2; // 31 4..5
			unsigned pic_order_cnt_type : 2; // 31 6..7
			signed pic_init_qp_minus26 : 6; // 32 0..5
			signed chroma_qp_index_offset : 5; // 32 6..10
			signed second_chroma_qp_index_offset : 5; // 32 11..15
		};
		unsigned f1;
	};

	union {
		struct {
			unsigned weighted_bipred_idc : 2; // 34 0..1
			unsigned fifo_dec_index : 7; // 34 2..8
			unsigned tmp_idx : 5; // 34 9..13
			unsigned frame_number : 16; // 34 14..29
			unsigned u34_3030 : 1; // 34 30..30 pp.u34[30:30]
			unsigned u34_3131 : 1; // 34 31..31 pad?
		};
		unsigned f2;
	};

	uint32_t field_order_cnt[2]; // 38, 3c

	struct { // 40
		union {
			struct {
				unsigned fifo_idx : 7; // 00 0..6
				unsigned tmp_idx : 5; // 00 7..11
				unsigned top_is_reference : 1; // 00 12
				unsigned bottom_is_reference : 1; // 00 13
				unsigned is_long_term : 1; // 00 14
				unsigned notseenyet : 1; // 00 15 pad?
				unsigned field_pic_flag : 1; // 00 16
				unsigned top_field_marking : 4; // 00 17..20
				unsigned bottom_field_marking : 4; // 00 21..24
				unsigned pad : 7; // 00 d25..31
			};
			unsigned f3;
		};

		uint32_t field_order_cnt[2]; // 04,08
		uint32_t frame_idx; // 0c
	} refs[0x10];

	uint8_t m4x4[6][16]; // 140
	uint8_t m8x8[2][64]; // 1a0
	uint32_t u220; // 220 number of extra reorder_list to append?
	uint8_t u224[0x20]; // 224..244 reorder_list append ?
	uint8_t nfi244[0xb0]; // add some pad to make sure nulls are read
};

struct struct_define {
	const char *struct_name;

	struct struct_members {
		const char *member_name;
		unsigned member_ofs;
		unsigned member_sizeof;
		unsigned member_array_length;
		unsigned member_min;
		unsigned member_mask;

		enum struct_define_type {
			PAD = 0,
			BITFIELD,
			NORMAL,
			ARRAY
		} member_type;
	} members[64];
};

#define _PAD(from, x) { #x, offsetof(struct from, x), sizeof(((struct from*)NULL)->x), 1, 1, ~0U, PAD }
#define _(from, x) { #x, offsetof(struct from, x), sizeof(((struct from*)NULL)->x), 1, 1, ~0U, NORMAL }
#define _ARRAY(from, x) { #x, offsetof(struct from, x), sizeof(((struct from*)NULL)->x[0]), \
			  sizeof(((struct from*)NULL)->x)/sizeof(((struct from*)NULL)->x[0]), 1, ~0U, ARRAY }
#define _BF(from, bf, x) { #x, offsetof(struct from, bf), sizeof(((struct from*)NULL)->bf), 1, (struct from){ .x = 1 }.bf, (struct from){ .x = ~0 }.bf, BITFIELD }

static const char *get_member_name(struct struct_members *cur, unsigned j)
{
	static char name[64];
	if (cur->member_type != ARRAY)
		return cur->member_name;
	sprintf(name, "%s[%u]", cur->member_name, j);
	return name;
}

static void compare(char *oldstruct, char *newstruct, unsigned size, struct struct_define *strdata)
{
	int i;
	const char *name = strdata->struct_name;
	struct struct_members *cur = &strdata->members[0];

	if (!cur->member_name) {
		unsigned *olds = (void*)oldstruct;
		unsigned *news = (void*)newstruct;

		/* no data, word sized comparison */
		for (i = 0; i < size/4; ++i, news++, olds++)
			if (*news != *olds) {
				printf("vp ofs %02x differs: %08x now %08x\n", i*4, *olds, *news);
			}

		return;
	}

	for (i = 0; cur->member_name; ++cur, ++i) {
		unsigned ofs = cur->member_ofs;
		int j;

		for (j = 0; j < cur->member_array_length; ++j, ofs += cur->member_sizeof)
			if (memcmp(&oldstruct[ofs], &newstruct[ofs], cur->member_sizeof)) {
				switch (cur->member_sizeof) {
					case 4: {
						uint32_t old, new;

						old = *(uint32_t*)&oldstruct[ofs] & cur->member_mask;
						new = *(uint32_t*)&newstruct[ofs] & cur->member_mask;

						if (old != new) {
							printf("%s member %s differs: %08x now %08x\n",
							       name, get_member_name(cur, j),
							       old / cur->member_min, new / cur->member_min);
						}
						break;
					}
					case 2:
						printf("%s member %s differs: %04x now %04x\n",
						       name, get_member_name(cur, j),
						       *(uint16_t*)&oldstruct[ofs],
						       *(uint16_t*)&newstruct[ofs]);
						break;
					case 1:
						printf("%s member %s differs: %02x now %02x\n",
						       name, get_member_name(cur, j),
						       *(uint8_t*)&oldstruct[ofs],
						       *(uint8_t*)&newstruct[ofs]);
						break;
					default:
						printf("%s member %s differs\n",
						       name, cur->member_name);
						break;
				}
				fflush(stdout);
				assert(cur->member_type != PAD);
			}
	}
}

static void print_def(char *newstruct, unsigned size, struct struct_define *strdata)
{
	int i;
	const char *name = strdata->struct_name;
	struct struct_members *cur = &strdata->members[0];

	if (!cur->member_name) {
		unsigned *news = (void*)newstruct;

		/* no data, word sized comparison */
		for (i = 0; i < size/4; ++i, news++)
			if (*news) {
				printf("vp ofs %02x is set to %08x\n", i*4, *news);
			}

		return;
	}

	for (i = 0; cur->member_name; ++cur, ++i) {
		unsigned ofs = cur->member_ofs;
		int j;

		for (j = 0; j < cur->member_array_length; ++j, ofs += cur->member_sizeof)
			if (cur->member_type != PAD) {
				switch (cur->member_sizeof) {
					case 4: {
						uint32_t new;

						new = *(uint32_t*)&newstruct[ofs] & cur->member_mask;

						if (cur->member_type == ARRAY) {
							printf("%s member %s[%d] is set to %08x\n",
							       name, cur->member_name, j, new / cur->member_min);
						} else if (new) {
							printf("%s member %s is set to %08x\n",
							       name, cur->member_name, new / cur->member_min);
						}
						break;
					}
					case 2:
						printf("%s member %s is set to %04x\n",
						       name, cur->member_name, *(uint16_t*)&newstruct[ofs]);
						break;
					case 1:
						printf("%s member %s is set to %02x\n",
						       name, cur->member_name, *(uint8_t*)&newstruct[ofs]);
						break;
					default:
						break;
				}
			}
	}
}
static char dump[2][0x8000000];
static char save_bsp[0x100], save_vp[0x300], saved;
static unsigned codec, endcode, map, othermap;
static unsigned endcodes[4] = { 0xb7010000, 0x0a010000, 0x0b010000, 0xb1010000 };

static int configure(int argc, char **argv)
{
	const struct option opts[] = {
		{ .name = "codec", .has_arg = 1, NULL, 'c' },
		{ .name = "map", .has_arg = 1, NULL, 'm' },
		{ .name = "othermap", .has_arg = 1, NULL, 'o' },
		{},
	};

	while (1) {
		switch (getopt_long(argc, argv, "c:m:o:", opts, NULL)) {
		default:
			return 0;
		case -1:
			return map;
		case 'c':
			codec = atoi(optarg);
			if (codec < 1 || codec > 4)
				return 0;
			endcode = endcodes[codec - 1];
			break;
		case 'm':
			map = atoi(optarg);
			break;
		case 'o':
			othermap = atoi(optarg);
			break;
		}
	}
}

int from_vdpau(unsigned profile) {
	switch (profile) {
	case VDP_DECODER_PROFILE_MPEG1:
	case VDP_DECODER_PROFILE_MPEG2_SIMPLE:
	case VDP_DECODER_PROFILE_MPEG2_MAIN:
		return 1;

	case VDP_DECODER_PROFILE_H264_BASELINE:
	case VDP_DECODER_PROFILE_H264_MAIN:
	case VDP_DECODER_PROFILE_H264_HIGH:
		return 3;

	case VDP_DECODER_PROFILE_VC1_SIMPLE:
	case VDP_DECODER_PROFILE_VC1_MAIN:
	case VDP_DECODER_PROFILE_VC1_ADVANCED:
		return 2;

	case VDP_DECODER_PROFILE_MPEG4_PART2_SP:
	case VDP_DECODER_PROFILE_MPEG4_PART2_ASP:
		return 4;
	default:
		assert(0);
		exit(1);
	}
}

#define MAX_SEQS 8192

int main(int argc, char **argv)
{
	FILE *f = stdin;
	char *line = NULL;
	size_t n;
	ssize_t ret;
	unsigned was_create = 0, create = 0;

	unsigned start = ~0, end = ~0, written = 0;
	unsigned sizes[MAX_SEQS];
	unsigned offsets[MAX_SEQS];
	unsigned maps[MAX_SEQS];
	unsigned lastmap = -1;


	struct struct_define defines_bsp[] = {
	{ "mpeg1/2 bsp", {
	#define STRUCT mpeg12_picparm_bsp
		_(STRUCT, width),
		_(STRUCT, height),
		_(STRUCT, picture_structure),
		_(STRUCT, picture_coding_type),
		_(STRUCT, intra_dc_precision),
		_(STRUCT, frame_pred_frame_dct),
		_(STRUCT, concealment_motion_vectors),
		_(STRUCT, intra_vlc_format),
		_PAD(STRUCT, pad),
		_ARRAY(STRUCT, f_code[0]),
		_ARRAY(STRUCT, f_code[1]),
	#undef STRUCT
	}},
	{ "vc-1 bsp", {
	#define STRUCT vc1_picparm_bsp
		_(STRUCT, width),
		_(STRUCT, height),
		_(STRUCT, profile),
		_(STRUCT, postprocflag),
		_(STRUCT, pulldown),
		_(STRUCT, interlaced),
		_(STRUCT, tfcntrflag),
		_(STRUCT, finterpflag),
		_(STRUCT, psf),
		_PAD(STRUCT, pad),
		_(STRUCT, multires),
		_(STRUCT, syncmarker),
		_(STRUCT, rangered),
		_(STRUCT, maxbframes),
		_(STRUCT, dquant),
		_(STRUCT, panscan_flag),
		_(STRUCT, refdist_flag),
		_(STRUCT, quantizer),
		_(STRUCT, extended_mv),
		_(STRUCT, extended_dmv),
		_(STRUCT, overlap),
		_(STRUCT, vstransform),

	#undef STRUCT
	}},
	{ "h264 bsp", {
	#define STRUCT h264_picparm_bsp
		_(STRUCT, unk00),
		_(STRUCT, log2_max_frame_num_minus4),
		_(STRUCT, pic_order_cnt_type),
		_(STRUCT, log2_max_pic_order_cnt_lsb_minus4),
		_(STRUCT, delta_pic_order_always_zero_flag),
		_(STRUCT, frame_mbs_only_flag),
		_(STRUCT, direct_8x8_inference_flag),
		_(STRUCT, width_mb),
		_(STRUCT, height_mb),

		/* picparm2 */
		_(STRUCT, entropy_coding_mode_flag),
		_(STRUCT, pic_order_present_flag),
		_PAD(STRUCT, unk),
		_PAD(STRUCT, pad1),
		_PAD(STRUCT, pad2),
		_(STRUCT, num_ref_idx_l0_active_minus1),
		_(STRUCT, num_ref_idx_l1_active_minus1),
		_(STRUCT, weighted_pred_flag),
		_(STRUCT, weighted_bipred_idc),
		_(STRUCT, pic_init_qp_minus26),
		_(STRUCT, deblocking_filter_control_present_flag),
		_(STRUCT, redundant_pic_cnt_present_flag),
		_(STRUCT, transform_8x8_mode_flag),
		_(STRUCT, mb_adaptive_frame_field_flag),
		_(STRUCT, field_pic_flag),
		_(STRUCT, bottom_field_flag),
		_PAD(STRUCT, real_pad),
	#undef STRUCT
	}},
	{ "mpeg4 bsp", {
	#define STRUCT mpeg4_picparm_bsp
		_(STRUCT, width),
		_(STRUCT, height),
		_(STRUCT, vop_time_increment_size),
		_(STRUCT, interlaced),
		_(STRUCT, resync_marker_disable),
	#undef STRUCT
	}},
	};

	struct struct_define defines_vp[] = {
	{ "mpeg1/2 vp", {
	#define STRUCT mpeg12_picparm_vp
	#undef STRUCT
	}},
	{ "vc-1 vp", {
	#define STRUCT vc1_picparm_vp
		_(STRUCT, bucket_size),
		_PAD(STRUCT, pad),
		_(STRUCT, inter_ring_data_size),
		_(STRUCT, unk0c),
		_(STRUCT, unk10),
		_ARRAY(STRUCT, ofs),
		_(STRUCT, width),
		_(STRUCT, height),
		_(STRUCT, profile),
		_(STRUCT, loopfilter),
		_(STRUCT, u32),
		_(STRUCT, dquant),
		_(STRUCT, overlap),
		_(STRUCT, quantizer),
		_(STRUCT, u36),
		_PAD(STRUCT, pad2),
	#undef STRUCT
	}},
	{ "h264 vp", {
	#define STRUCT h264_picparm_vp
		_(STRUCT, width),
		_(STRUCT, height),
		_(STRUCT, stride1),
		_(STRUCT, stride2),
		_ARRAY(STRUCT, ofs),
		_(STRUCT, tmp_stride),
		_(STRUCT, bucket_size),
		_(STRUCT, inter_ring_data_size),
		_BF(STRUCT, f1, mb_adaptive_frame_field_flag),
		_BF(STRUCT, f1, direct_8x8_inference_flag),
		_BF(STRUCT, f1, weighted_pred_flag),
		_BF(STRUCT, f1, constrained_intra_pred_flag),
		_BF(STRUCT, f1, is_reference),
		_BF(STRUCT, f1, interlace),
		_BF(STRUCT, f1, bottom_field_flag),
		_BF(STRUCT, f1, second_field),
		_BF(STRUCT, f1, log2_max_frame_num_minus4),
		_BF(STRUCT, f1, chroma_format_idc),
		_BF(STRUCT, f1, pic_order_cnt_type),
		_BF(STRUCT, f1, pic_init_qp_minus26),
		_BF(STRUCT, f1, chroma_qp_index_offset),
		_BF(STRUCT, f1, second_chroma_qp_index_offset),

		_BF(STRUCT, f2, weighted_bipred_idc),
		_BF(STRUCT, f2, fifo_dec_index),
		_BF(STRUCT, f2, tmp_idx),
		_BF(STRUCT, f2, frame_number),
		_BF(STRUCT, f2, u34_3030),
		_BF(STRUCT, f2, u34_3131),
		_(STRUCT, field_order_cnt[0]),
		_(STRUCT, field_order_cnt[1]),
	#undef STRUCT
	}},
	{ "mpeg4 vp", {
	#define STRUCT mpeg4_picparm_vp
		_(STRUCT, width),
		_(STRUCT, height),
		_(STRUCT, unk08),
		_(STRUCT, unk0c),
		_ARRAY(STRUCT, ofs),
		_(STRUCT, bucket_size),
		_PAD(STRUCT, pad1),
		_PAD(STRUCT, pad2),
		_(STRUCT, inter_ring_data_size),
		_ARRAY(STRUCT, trd),
		_ARRAY(STRUCT, trb),
		_PAD(STRUCT, u48),
		_(STRUCT, f_code_fw),
		_(STRUCT, f_code_bw),
		_(STRUCT, interlaced),
		_(STRUCT, quant_type),
		_(STRUCT, quarter_sample),
		_(STRUCT, short_video_header),
		_PAD(STRUCT, u54),
		_(STRUCT, vop_coding_type),
		_(STRUCT, rounding_control),
		_(STRUCT, alternate_vertical_scan_flag),
		_(STRUCT, top_field_first),
		_ARRAY(STRUCT, pad4),
		_ARRAY(STRUCT, intra),
		_ARRAY(STRUCT, non_intra),
		_ARRAY(STRUCT, pad5),
	#undef STRUCT
	}},
	};

	/* Sanity check, make sure we didn't leave any holes.. */
	for (n = 0; n < sizeof(defines_bsp)/sizeof(*defines_bsp); ++n) {
		struct struct_members *cur;
		int i;
		int prev_end = 0;

		for (i = 0, cur = defines_bsp[n].members; cur->member_name; ++i, ++cur) {
			assert(cur->member_ofs == prev_end);
			prev_end = cur->member_ofs + cur->member_sizeof * cur->member_array_length;
		}
	}

	for (n = 0; n < sizeof(defines_vp)/sizeof(*defines_vp); ++n) {
		struct struct_members *cur;
		int i;
		int prev_end = 0;

		for (i = 0, cur = defines_vp[n].members; cur->member_name; ++i, ++cur) {
			assert(cur->member_ofs == prev_end || cur->member_min > 1);
			prev_end = cur->member_ofs + cur->member_sizeof * cur->member_array_length;
		}
	}

	if (!configure(argc, argv)) {
		fprintf(stderr, "Usage: %s --map <MAP> [--othermap <MAP>] [--codec <NUM>]\n", argv[0]);
		fprintf(stderr, "Where codec number is:\n");
		fprintf(stderr, "\t1 -- mpeg1/2\n");
		fprintf(stderr, "\t2 -- vc-1\n");
		fprintf(stderr, "\t3 -- h264\n");
		fprintf(stderr, "\t4 -- mpeg4\n");
		return 1;
	}

	while ((ret = getline(&line, &n, f)) > 0) {
		unsigned val[4], m, addr, w;
		char c[4], mode;
		int i;

		if (line[ret-1] == '\n')
			line[ret-1] = 0;

		i = sscanf(line, "--%*d-- %c %u:%x, %x%c%x%c%x%c%x",
			   &mode, &m, &addr, &val[0], &c[0], &val[1], &c[1],
			   &val[2], &c[2], &val[3]);
		if (i >= 4 && (m == map || (othermap && m == othermap))) {
			unsigned *ofs = (unsigned*)&dump[m == map][addr];

			if (i == 10) {
				//reorder words (4,3,2,1) => (1,2,3,4)

				ofs[0] = val[3];
				ofs[1] = val[2];
				ofs[2] = val[1];
				ofs[3] = val[0];

				w = 0x10;
			} else if (i == 7 || i == 6) {
				//reorder words (2,1) => (1,2)

				ofs[0] = val[1];
				ofs[1] = val[0];

				w = 0x8;
			} else {
				char t[8];
				assert(i == 5 && c[0] == ' ');
				w = sscanf(line, "--%*d-- %*c %*u:%*x, 0x%c%c%c%c%c%c%c%c%c",
					   &t[0], &t[1], &t[2], &t[3], &t[4], &t[5], &t[6], &t[7], &c[0]);

				switch (w) {
				case 9:
					assert(c[0] == ' ');
				case 8:
					ofs[0] = val[0];
					break;

				case 5:
					assert(t[4] == ' ');
				case 4:
					*(short*)ofs = val[0];
					break;

				case 3:
					assert(t[2] == ' ');
				case 2:
					*(char*)ofs = val[0];
					break;
				default: assert(0); exit(1);
				}
				w /= 2;
			}

			if (m == lastmap && start == addr + w) {
				start = addr;
			} else if (m == lastmap && addr <= end && addr > start) {
				if (addr + w > end)
					end = addr + w;
//				else assert(addr + w > end);
			} else {
				if (written) {
					offsets[written-1] = start;
					sizes[written-1] = end - start;
					maps[written-1] = (lastmap == map);

					assert(written < sizeof(offsets)/sizeof(*offsets));
				}

				start = addr;
				end = addr + w;
				lastmap = m;

				++written;
			}
		} else if (i < 1) {
			int eliminated = 0;
			int j;

			was_create = create;

			i = sscanf(line, "vdp_decoder_create(%*d, %d, %*s)", &w);
			if (i == 1) {
				memset(save_bsp, 0, sizeof(save_bsp));
				memset(save_vp, 0, sizeof(save_vp));
				saved = 0;
				codec = from_vdpau(w);
				create = 1;
			} else {
				if (!strncmp(line, "vdp_imp_device_create_x11(", sizeof("vdp_imp_device_create_x11(")-1))
					create = 2;
				else
					create = 0;
			}

			if (written) {
				offsets[written-1] = start;
				sizes[written-1] = end - start;
				maps[written-1] = (lastmap == map);
				start = end = ~0;

				/* coalesce all writes, where possible */
				for (i = 0; i < written; ++i) {
					start = offsets[i];
					end = offsets[i] + sizes[i];

retry:
					for (j = i + 1; j < written; ++j) {
						unsigned jstart, jend;

						if (offsets[j] == ~0 || maps[i] != maps[j])
							continue;

						jstart = offsets[j];
						jend = jstart + sizes[j];

						if ((jstart >= start && jstart <= end) ||
						    (start >= jstart && start <= jend) ||
						    (jend >= start && jend <= end) ||
						    (end >= jstart && end <= jend))
							goto merge;

						continue;

merge:
						if (jstart < start)
							start = jstart;
						if (end < jend)
							end = jend;
						eliminated++;
						offsets[j] = ~0;
						goto retry;
					}
					offsets[i] = start;
					sizes[i] = end - start;
				}
			}

			if (!was_create && written) {
				void *bsp, *vp;
				unsigned bsp_size, vp_size;

//				printf("Intercepted %u (-%u) blocks on codec %u\n", written, eliminated, codec);

				for (i = j = 0; i < written; ++i) {
					if (offsets[i] == ~0)
						continue;
					if (j == 0) {
						bsp = &dump[maps[i]][offsets[i]];
						bsp_size = sizes[i];
					} else if (sizes[i] <= sizeof(save_vp)) {
						vp = &dump[maps[i]][offsets[i]];
						vp_size = sizes[i];
					}

//					printf("[%i/%u] = [ %x, %x (+%x) )\n", j, maps[i], offsets[i], offsets[i] + sizes[i], sizes[i]);
					j++;
				}
				assert(codec);
				assert(written - eliminated == 4 || written - eliminated == 3);

				if (saved) {
					compare(save_bsp, bsp, bsp_size, &defines_bsp[codec-1]);
					compare(save_vp, vp, vp_size, &defines_vp[codec-1]);
				} else {
					print_def(bsp, bsp_size, &defines_bsp[codec-1]);
					print_def(vp, vp_size, &defines_vp[codec-1]);
				}
				memcpy(save_bsp, bsp, bsp_size);
				memcpy(save_vp, vp, vp_size);
				saved = 1;
			} else if (was_create == 1 && written) {

				for (i = j = 0; i < written; ++i) {
					int fd;
					char fw[32];
					char name[][8] = { "mpeg12", "vc1", "h264", "mpeg4" };
					ssize_t r;

					if (offsets[i] == ~0)
						continue;

					sprintf(fw, "vuc-%s-%u", name[codec-1], j);
					printf("writing %s\n", fw);
					fd = open(fw, O_CREAT | O_WRONLY | O_TRUNC, 0644);
					assert(fd >= 0);
					r = write(fd, &dump[maps[i]][offsets[i]], sizes[i]);
					assert(r == sizes[i]);
					close(fd);
					j++;
				}
			}

			written = 0;

			if ((strncmp(line, "vdp", 3) && strncmp(line, "    ->", 6)) ||
			    !strncmp(line, "vdp_decoder_render", sizeof("vdp_decoder_render")-1) ||
			    !strncmp(line, "vdp_decoder_create", sizeof("vdp_decoder_create")-1))
				printf("%s\n", line);
		}
	}
	return 0;
}

