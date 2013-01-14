/*
 * copying a 64-bit value to a 32-bit struct with 2 members is frowned upon, apparently..
 * DIE MEMCPY
 */
#undef _FORTIFY_SOURCE
#define _FORTIFY_SOURCE 0

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "nva.h"
#include "vstream.h"
#include "h262.h"
#include "h264.h"
#include "util.h"

VdpGetProcAddress * vdp_get_proc_address;
VdpGetErrorString * vdp_get_error_string;
VdpGetApiVersion * vdp_get_api_version;
VdpGetInformationString * vdp_get_information_string;
VdpDeviceDestroy * vdp_device_destroy;
VdpGenerateCSCMatrix * vdp_generate_csc_matrix;
VdpVideoSurfaceQueryCapabilities * vdp_video_surface_query_capabilities;
VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities * vdp_video_surface_query_get_put_bits_y_cb_cr_capabilities;
VdpVideoSurfaceCreate * vdp_video_surface_create;
VdpVideoSurfaceDestroy * vdp_video_surface_destroy;
VdpVideoSurfaceGetParameters * vdp_video_surface_get_parameters;
VdpVideoSurfaceGetBitsYCbCr * vdp_video_surface_get_bits_y_cb_cr;
VdpVideoSurfacePutBitsYCbCr * vdp_video_surface_put_bits_y_cb_cr;
VdpOutputSurfaceQueryCapabilities * vdp_output_surface_query_capabilities;
VdpOutputSurfaceQueryGetPutBitsNativeCapabilities * vdp_output_surface_query_get_put_bits_native_capabilities;
VdpOutputSurfaceQueryPutBitsIndexedCapabilities * vdp_output_surface_query_put_bits_indexed_capabilities;
VdpOutputSurfaceQueryPutBitsYCbCrCapabilities * vdp_output_surface_query_put_bits_y_cb_cr_capabilities;
VdpOutputSurfaceCreate * vdp_output_surface_create;
VdpOutputSurfaceDestroy * vdp_output_surface_destroy;
VdpOutputSurfaceGetParameters * vdp_output_surface_get_parameters;
VdpOutputSurfaceGetBitsNative * vdp_output_surface_get_bits_native;
VdpOutputSurfacePutBitsNative * vdp_output_surface_put_bits_native;
VdpOutputSurfacePutBitsIndexed * vdp_output_surface_put_bits_indexed;
VdpOutputSurfacePutBitsYCbCr * vdp_output_surface_put_bits_y_cb_cr;
VdpBitmapSurfaceQueryCapabilities * vdp_bitmap_surface_query_capabilities;
VdpBitmapSurfaceCreate * vdp_bitmap_surface_create;
VdpBitmapSurfaceDestroy * vdp_bitmap_surface_destroy;
VdpBitmapSurfaceGetParameters * vdp_bitmap_surface_get_parameters;
VdpBitmapSurfacePutBitsNative * vdp_bitmap_surface_put_bits_native;
VdpOutputSurfaceRenderOutputSurface * vdp_output_surface_render_output_surface;
VdpOutputSurfaceRenderBitmapSurface * vdp_output_surface_render_bitmap_surface;
VdpDecoderQueryCapabilities * vdp_decoder_query_capabilities;
VdpDecoderCreate * vdp_decoder_create;
VdpDecoderDestroy * vdp_decoder_destroy;
VdpDecoderGetParameters * vdp_decoder_get_parameters;
VdpDecoderRender * vdp_decoder_render;
VdpVideoMixerQueryFeatureSupport * vdp_video_mixer_query_feature_support;
VdpVideoMixerQueryParameterSupport * vdp_video_mixer_query_parameter_support;
VdpVideoMixerQueryAttributeSupport * vdp_video_mixer_query_attribute_support;
VdpVideoMixerQueryParameterValueRange * vdp_video_mixer_query_parameter_value_range;
VdpVideoMixerQueryAttributeValueRange * vdp_video_mixer_query_attribute_value_range;
VdpVideoMixerCreate * vdp_video_mixer_create;
VdpVideoMixerSetFeatureEnables * vdp_video_mixer_set_feature_enables;
VdpVideoMixerSetAttributeValues * vdp_video_mixer_set_attribute_values;
VdpVideoMixerGetFeatureSupport * vdp_video_mixer_get_feature_support;
VdpVideoMixerGetFeatureEnables * vdp_video_mixer_get_feature_enables;
VdpVideoMixerGetParameterValues * vdp_video_mixer_get_parameter_values;
VdpVideoMixerGetAttributeValues * vdp_video_mixer_get_attribute_values;
VdpVideoMixerDestroy * vdp_video_mixer_destroy;
VdpVideoMixerRender * vdp_video_mixer_render;
VdpPresentationQueueTargetDestroy * vdp_presentation_queue_target_destroy;
VdpPresentationQueueCreate * vdp_presentation_queue_create;
VdpPresentationQueueDestroy * vdp_presentation_queue_destroy;
VdpPresentationQueueSetBackgroundColor * vdp_presentation_queue_set_background_color;
VdpPresentationQueueGetBackgroundColor * vdp_presentation_queue_get_background_color;
VdpPresentationQueueGetTime * vdp_presentation_queue_get_time;
VdpPresentationQueueDisplay * vdp_presentation_queue_display;
VdpPresentationQueueBlockUntilSurfaceIdle * vdp_presentation_queue_block_until_surface_idle;
VdpPresentationQueueQuerySurfaceStatus * vdp_presentation_queue_query_surface_status;
VdpPreemptionCallbackRegister * vdp_preemption_callback_register;
VdpPresentationQueueTargetCreateX11 * vdp_presentation_queue_target_create_x11;

static void load_vdpau(VdpDevice dev);

static const int input_width = 1440;
static const int input_height = 1152;
static const int output_width = 1440;
static const int output_height = 1152;

VdpDevice dev;
static uint32_t cnum = 0;

static int open_map(void)
{
	if (nva_init()) {
		fprintf(stderr, "NVA init failed\n");
		return 0;
	}
	if (!nva_cardsnum) {
		fprintf(stderr, "Cannot find any valid card!\n");
		return 0;
	}
	if (cnum > nva_cardsnum) {
		fprintf(stderr, "Only %u cards found, invalid card %u selected\n", nva_cardsnum, cnum);
		return 0;
	}
	return 1;
}

#define ok(a) do { ret = a; if (ret != VDP_STATUS_OK) { fprintf(stderr, "%s:%u %u %s\n", __FUNCTION__, __LINE__, ret, vdp_get_error_string(ret)); exit(1); } } while (0)

struct snap {
	uint32_t bsp_done, data_done, pvp_done, tries;
	uint32_t bsp[0x180/4]; // IO registers 0x400...0x580, ignoring indexed bucket at 580..590
	uint16_t dump[2048]; // PVP data section (400/440 indexed io)
	uint32_t pvp[0x3c0/4]; // PVP registers 0x440...0x800
};

struct fuzz {
	int offset;
	int size_bytes;
	const char *name;
	int single_values;
	int64_t values[8]; // range if single_values is not set

	// Some values will only make sense in certain combinations
	// so add support to override which template to use
	int template_idx;
};

#if 0
static void memcpy4(uint32_t *out, uint32_t in, uint32_t size)
{
	for (size /= 4; size; size--, in += 4, ++out)
		*out = nva_rd32(cnum, in);
}
#endif

#define BSP(x) (0x84000 + (x))
#define BSP_OFS(x) BSP(0x400 + (x))

#define PVP(x) (0x85000 + (x))
#define PVP_OFS(x) PVP(0x440 + (x))
#define VUC(x) PVP(0x400 + (x % 0x40))

static void clear_data(void)
{
#if 0 // Crashes
	int i, j;
	for (i = 0; i < sizeof(((struct snap*)0)->bsp); i += 4)
		nva_wr32(cnum, BSP_OFS(i), 0);
	for (i = 0; i < 0x40; ++i) {
		nva_wr32(cnum, PVP(0xffc), i);
		for (j = 0; j < 0x40; j += 4)
			nva_wr32(cnum, VUC(j), 0);
	}
	nva_wr32(cnum, PVP(0xffc), 0);
	  
	for (i = 0; i < sizeof(((struct snap*)0)->pvp); i += 4)
		nva_wr32(cnum, PVP_OFS(i), 0);
#endif
}

static uint64_t gettime(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000ULL;
}

static uint32_t mask_bsp(enum vs_type codec, uint32_t idx) {
	switch (idx & 0xfff) {
	case 0x420:
		if (codec == VS_H262)
			return ~0x01fffffc; // Clear MB_IDX|SLICE_START_LINE
		else if (codec == VS_H264)
			return ~0x00007ffc; // Clear SLICE_IDX
		else
			return ~0U;

	case 0x424:
	case 0x440:
		return ~0x1fffffff; // Top bits undocumented?

	case 0x448: // BSP.STATUS
	case 0x454: // BSP.INTR
	case 0x480 ... 0x4c4: // stream setup
	case 0x500 ... 0x510: // mbring setup
		return 0;

	default:
		return ~0U;
	}
}

static uint32_t mask_vuc(enum vs_type codec, uint32_t idx) {
	switch (codec) {
	case VS_H262:
		switch (idx) {
		case 52 ... 243: // Presumably addresses for Y Cb Cr
		case 246 ... 2047: // Trash?
			return 0U;
		default:
			return ~0U;
		}
	default:
		return ~0U;
	}
}

static uint32_t mask_pvp(enum vs_type codec, uint32_t idx) {
	switch (idx & 0xfff) {
	case 0x4a0:
	case 0x500:
	case 0x688 ... 0x69c:
		return 0U;
	case 0x440 ... 0x444:
		return nva_cards[cnum].chipset < 0xd0 ? 0U : ~0U;
	default:
		return ~0U;
	}
}

/* registers we use to detect work being done */
#define WORK_BSP BSP(0x420)
#define WORK_PVP PVP(0x4b0) /*0x608->only works on VP5*/

static int save_data(enum vs_type codec, struct snap *cur)
{
	uint32_t addr, mask, *dump = (uint32_t*)cur->dump;
	++cur->tries;
	if (cur->bsp_done < ARRAY_SIZE(cur->bsp)) {
		uint64_t start = gettime();
		while (!nva_rd32(cnum, WORK_BSP))
			if (gettime() - start >= 100) {
				fprintf(stderr, "Timeout in bsp!\n");
				return -EIO;
			}
		for (; cur->bsp_done < ARRAY_SIZE(cur->bsp); ++cur->bsp_done) {
			addr = BSP_OFS(cur->bsp_done * 4);
			if (!(mask = mask_bsp(codec, addr)))
				continue;
			cur->bsp[cur->bsp_done] = nva_rd32(cnum, addr) & mask;
			if (!nva_rd32(cnum, WORK_BSP))
				return -EAGAIN;
		}
		return -EAGAIN;
	}
	if (cur->data_done < 0x40) {
		uint64_t start = gettime();
		while (!nva_rd32(cnum, WORK_PVP))
			if (gettime() - start >= 100) {
				fprintf(stderr, "Timeout in vp!\n");
				return -EIO;
			}

		for (; cur->data_done < 0x40; ++cur->data_done) {
			uint32_t j;
			nva_wr32(cnum, PVP(0xffc), cur->data_done);
			for (j = 0; j < 0x40; j += 4) {
				addr = cur->data_done + j * 0x10;
				if (!(mask = mask_vuc(codec, addr * 2)))
					continue;
				dump[addr] = nva_rd32(cnum, VUC(j)) & mask;
			}
			if (!nva_rd32(cnum, WORK_PVP))
				return -EAGAIN;
		}
		nva_wr32(cnum, PVP(0xffc), 0);
		goto skipspin;
	}
	if (cur->data_done == 0x40) {
		uint64_t start = gettime();
		while (!nva_rd32(cnum, WORK_PVP))
			if (gettime() - start >= 100)
				return -EIO;

skipspin:
		for (; cur->pvp_done < ARRAY_SIZE(cur->pvp); ++cur->pvp_done) {
			addr = PVP_OFS(cur->pvp_done * 4);
			if (!(mask = mask_pvp(codec, addr)))
				continue;
			cur->pvp[cur->pvp_done] = nva_rd32(cnum, addr) & mask;
			if (!nva_rd32(cnum, WORK_PVP))
				return -EAGAIN;
		}
	}
	return 0;
}

static void compare_data(struct snap *ref, struct snap *cur, struct fuzz *f, uint32_t oldval, uint32_t newval)
{
	uint32_t i, idx;
	fprintf(stderr, "End result for %s %u -> %u\n", f->name, oldval, newval);
	for (i = 0; i < ARRAY_SIZE(cur->bsp); ++i) {
		idx = BSP_OFS(i * 4) & 0xfff;
		if (ref->bsp[i] != cur->bsp[i])
			fprintf(stderr, "BSP.%x = %x -> %x\n", idx, ref->bsp[i], cur->bsp[i]);
	}

	for (i = 0; i < ARRAY_SIZE(cur->dump); ++i) {
		if (ref->dump[i] != cur->dump[i])
			fprintf(stderr, "PVP.VUC[%i] = %x -> %x\n", i, ref->dump[i], cur->dump[i]);
	}
	for (i = 0; i < ARRAY_SIZE(cur->pvp); ++i) {
		idx = PVP_OFS(i * 4) & 0xfff;
		if (ref->pvp[i] != cur->pvp[i])
			fprintf(stderr, "PVP.%x = %x -> %x\n", idx, ref->pvp[i], cur->pvp[i]);
	}
}

static void fill_mpeg_picparm(const VdpPictureInfoMPEG1Or2 *in, struct h262_picparm *pp)
{
	int i;
	pp->picture_structure = in->picture_structure;
	pp->picture_coding_type = in->picture_coding_type;
	pp->intra_dc_precision = in->intra_dc_precision;
	pp->frame_pred_frame_dct = in->frame_pred_frame_dct;
	pp->concealment_motion_vectors = in->concealment_motion_vectors;
	pp->intra_vlc_format = in->intra_vlc_format;
	pp->alternate_scan = in->alternate_scan;
	pp->q_scale_type = in->q_scale_type;
	pp->top_field_first = in->top_field_first;
	pp->full_pel_forward_vector = in->full_pel_forward_vector;
	pp->full_pel_backward_vector = in->full_pel_backward_vector;
	pp->f_code[0][0] = in->f_code[0][0];
	pp->f_code[0][1] = in->f_code[0][1];
	pp->f_code[1][0] = in->f_code[1][0];
	pp->f_code[1][1] = in->f_code[1][1];
	pp->pic_width_in_mbs = input_width / 16;
	pp->pic_height_in_mbs = input_height / 16;
	pp->pic_size_in_mbs = pp->pic_width_in_mbs * pp->pic_height_in_mbs;
	for (i = 0; i < 64; ++i) {
		pp->intra_quantiser_matrix[i] = i * 4;
		pp->non_intra_quantiser_matrix[i] = i * 4;
	}
}

static void fill_mpeg_mb(const struct h262_picparm *pp, const struct h262_slice *slice, struct h262_macroblock *mb)
{
	int vfs = pp->picture_structure == H262_PIC_STRUCT_FIELD_BOTTOM;
	if (pp->picture_structure == H262_PIC_STRUCT_FRAME &&
	    !pp->frame_pred_frame_dct &&
	    (mb->macroblock_intra || mb->macroblock_pattern))
		mb->dct_type = 1; // field decoding
	else
		mb->dct_type = 0;
	mb->quantiser_scale_code = slice->quantiser_scale_code;
	switch (pp->picture_coding_type) {
	case H262_PIC_TYPE_I:
	case H262_PIC_TYPE_D:
		mb->macroblock_intra = 1;
		mb->coded_block_pattern = 0x3f;
		mb->block[0][0] = 0x80;
		mb->block[1][0] = 0x80;
		mb->block[2][0] = 0x80;
		mb->block[3][0] = 0x80;
		mb->block[4][0] = 0x80;
		mb->block[5][0] = 0x80;
		break;
	// Because motion vectors are fun
	case H262_PIC_TYPE_B:
		mb->macroblock_motion_backward = 1;
	case H262_PIC_TYPE_P:
		mb->macroblock_motion_forward = 1;
		break;
	}

	if (pp->picture_structure == H262_PIC_STRUCT_FRAME)
		mb->frame_motion_type = H262_FRAME_MOTION_FRAME;
	else
		mb->field_motion_type = H262_FIELD_MOTION_FIELD;

	mb->motion_vertical_field_select[0][0] = vfs;
	mb->motion_vertical_field_select[0][1] = vfs;
	mb->motion_vertical_field_select[1][0] = vfs;
	mb->motion_vertical_field_select[1][1] = vfs;
}

static void test_mpeg(void *bytes, uint32_t len, struct h262_picparm *picparm, struct h262_seqparm *seqparm)
{
#if 1
	struct bitstream *str = vs_new_decode(VS_H262, bytes, len);
	int nslice = 0;

	while (str->bytepos < str->bytesnum) {
		unsigned start_code;

		if (vs_start(str, &start_code)) goto err;

		if (start_code >= H262_START_CODE_SLICE_BASE && start_code <= H262_START_CODE_SLICE_LAST) {
			struct h262_slice *slice;

			nslice++;

			slice = calloc (sizeof *slice, 1);
			slice->mbs = calloc (sizeof *slice->mbs, picparm->pic_size_in_mbs);
			slice->slice_vertical_position = start_code - H262_START_CODE_SLICE_BASE;
			if (seqparm->vertical_size > 2800) {
				uint32_t svp_ext;
				if (vs_u(str, &svp_ext, 3)) {
					h262_del_slice(slice);
					goto err;
				}
				if (slice->slice_vertical_position >= 0x80) {
					fprintf(stderr, "Invalid slice start code for large picture\n");
					goto err;
				}
				slice->slice_vertical_position += svp_ext * 0x80;
			}
			if (slice->slice_vertical_position >= picparm->pic_height_in_mbs) {
				fprintf(stderr, "slice_vertical_position too large\n");
				goto err;
			}
			if (h262_slice(str, seqparm, picparm, slice)) {
//				h262_print_slice(seqparm, picparm, slice);
				h262_del_slice(slice);
				goto err;
			}
//			h262_print_slice(seqparm, picparm, slice);
			if (vs_end(str)) {
				h262_del_slice(slice);
				goto err;
			}
			h262_del_slice(slice);
		} else {
			fprintf(stderr, "Unknown start code %08x\n", start_code);
			goto err;
		}
	}
	return;

err:
	assert(0);
	exit(1);
#endif
}

static void generate_mpeg(const VdpPictureInfoMPEG1Or2 *info, int mpeg2, const void **byte, uint32_t *len)
{
	struct bitstream *str = vs_new_encode(VS_H262);
	struct h262_seqparm seqparm;
	struct h262_picparm pp;
	uint32_t x, y, val = H262_START_CODE_SLICE_BASE;
	struct h262_macroblock *mbs;
	struct h262_slice slice;

	seqparm.is_ext = mpeg2;
	seqparm.chroma_format = 1; // 420
	seqparm.vertical_size = input_height;
	pp.is_ext = mpeg2;
	fill_mpeg_picparm(info, &pp);
	mbs = calloc(sizeof(*mbs), pp.pic_size_in_mbs);

	slice.slice_vertical_position = val;
	slice.quantiser_scale_code = 1;
	slice.intra_slice_flag = 0;
	slice.first_mb_in_slice = 0;
	slice.last_mb_in_slice = pp.pic_width_in_mbs;
	slice.mbs = mbs;

	for (x = 0; x < pp.pic_size_in_mbs; ++x)
		fill_mpeg_mb(&pp, &slice, &mbs[x]);

	for (y = 0; y < pp.pic_size_in_mbs; y += pp.pic_width_in_mbs, ++val) {
		vs_start(str, &val);
		if (h262_slice(str, &seqparm, &pp, &slice) || vs_end(str)) {
			fprintf(stderr, "Failed to encode slice!\n");
			return;
		}
	}
	*byte = str->bytes;
	*len = str->bytesnum;

	test_mpeg(str->bytes, str->bytesnum, &pp, &seqparm);
	free(mbs);
	free(str);
}

static void action_mpeg(VdpDecoder dec, VdpVideoSurface surf, VdpPictureInfoMPEG1Or2 *info, struct snap *ref,
struct fuzz *f, uint32_t oldval, uint32_t newval)
{
	VdpStatus ret;
	int debug = 1;

	struct snap *cur = f ? calloc(1, sizeof(*cur)) : ref;
	int save = 0, failed_tries = 0;

	VdpBitstreamBuffer buffer;
	buffer.struct_version = VDP_BITSTREAM_BUFFER_VERSION;
	generate_mpeg(info, 1, &buffer.bitstream, &buffer.bitstream_bytes);

	if (f)
		fprintf(stderr, "Starting %s %u -> %u\n", f->name, oldval, newval);

	if (!debug) {
		uint32_t pitches[3] = { input_width, input_width };

		void *data[3];

		data[0] = calloc(input_height, input_width);
		data[1] = calloc(input_height/2, input_width);
		data[2] = NULL;

		do {
			if (save)
				ok(vdp_video_surface_get_bits_y_cb_cr(surf, VDP_YCBCR_FORMAT_NV12, data, pitches));
			ok(vdp_decoder_render(dec, surf, (void*)info, 1, &buffer));
		} while ((save = save_data(VS_H262, cur)) == -EAGAIN || (save == -EIO && ++failed_tries < 3));

		ok(vdp_video_surface_get_bits_y_cb_cr(surf, VDP_YCBCR_FORMAT_NV12, data, pitches));
		free(data[0]);
		free(data[1]);

		if (save < 0) {
			if (!f || oldval == newval) {
				// Reference streams shouldn't be failing
				assert(!!!"WTF?");
				return;
			}
			fprintf(stderr, "Failed on %s %u->%u, stream was likely invalid\n", f->name, oldval, newval);
			free((void*)buffer.bitstream);
			free(cur);
			return;
		}
	} else
		ok(vdp_decoder_render(dec, surf, (void*)info, 1, &buffer));

	free((void*)buffer.bitstream);
	if (f) {
		compare_data(ref, cur, f, oldval, newval);
		free(cur);
	}
}

static void fuzz_mpeg(VdpVideoMixer mix, VdpVideoSurface *surf, VdpOutputSurface *osurf)
{
	VdpPictureInfoMPEG1Or2 info, template[1] = { { surf[1], surf[2], input_height/16, 1, 1 } }; // Empty top field B frame
	uint32_t i, j;
	VdpDecoder dec;
	VdpStatus ret;
	struct snap *ref = calloc(ARRAY_SIZE(template), sizeof(*ref));

	for (i = 0; i < ARRAY_SIZE(template); ++i) {
		template[i].f_code[0][0] = 0xf;
		template[i].f_code[0][1] = 0xf;
		template[i].f_code[1][0] = 0xf;
		template[i].f_code[1][1] = 0xf;
	}

	struct fuzz fuzzy[] = {
	  { 8, 4, "slice_count", 1, { input_height / 16 + 1 } },
	  { 12, 1, "picture_structure", 0, { 1, 3 } },
	  { 13, 1, "picture_coding_type", 0, { 1, 4 } },
	  { 14, 1, "intra_dc_precision", 0, { 0, 3 } },
	  { 15, 1, "frame_pred_frame_dct", 0, { 0, 1 } },
	  { 16, 1, "concealment_motion_vectors", 0, { 0, 1 } },
	  { 17, 1, "intra_vlc_format", 0, { 0, 1 } },
	  { 18, 1, "alternate_scan", 0, { 0, 1 } },
	  { 19, 1, "q_scale_type", 0, { 0, 1 } },
	  { 20, 1, "top_field_first", 0, { 0, 1 } },
	  { 21, 1, "full_pel_forward_vector", 0, { 0, 1 } },
	  { 22, 1, "full_pel_backward_vector", 0, { 0, 1 } },
	};

//	ok(vdp_decoder_create(dev, VDP_DECODER_PROFILE_MPEG1, input_width, input_height, 2, &dec));
//	vdp_decoder_destroy(dec);
	ok(vdp_decoder_create(dev, VDP_DECODER_PROFILE_MPEG2_MAIN, input_width, input_height, 2, &dec));

	clear_data();

	// Render first, then retrieve bits to force serialization
	for (i = 0; i < ARRAY_SIZE(template); ++i)
		action_mpeg(dec, surf[0], &template[i], &ref[i], NULL, 0, 0);

	for (i = 0; i < ARRAY_SIZE(fuzzy); ++i) {
		int oldval = 0, tidx = fuzzy[i].template_idx;
		memcpy(&info, &template[tidx], sizeof(template[0]));
		memcpy(&oldval, (char*)&template[tidx] + fuzzy[i].offset, fuzzy[i].size_bytes);
		if (fuzzy[i].single_values) {
			for (j = 0; j < fuzzy[i].single_values; ++j) {
				memcpy((char*)&info + fuzzy[i].offset, &fuzzy[i].values[j], fuzzy[i].size_bytes);
				action_mpeg(dec, surf[0], &info, &ref[tidx], &fuzzy[i], oldval, fuzzy[i].values[j]);
			}
		} else {
			for (j = fuzzy[i].values[0]; j <= fuzzy[i].values[1]; ++j) {
				memcpy((char*)&info + fuzzy[i].offset, &j, fuzzy[i].size_bytes);
				action_mpeg(dec, surf[0], &info, &ref[tidx], &fuzzy[i], oldval, j);
			}
		}
	}
	fprintf(stderr, "Done, destroying renderer\n");
	free(ref);
	vdp_decoder_destroy(dec);
}

static void action_h263(VdpDecoder dec, VdpVideoSurface surf, VdpPictureInfoMPEG4Part2 *info, struct snap *ref,
struct fuzz *f, uint32_t oldval, uint32_t newval)
{
	struct snap *cur = f ? calloc(1, sizeof(*cur)) : ref;
	VdpStatus ret;
	int debug = 1;
	char bitstream[0] = {};
	int save = 0, failed_tries = 0;
	VdpBitstreamBuffer buffer;

	buffer.struct_version = VDP_BITSTREAM_BUFFER_VERSION;
	buffer.bitstream = bitstream;
	buffer.bitstream_bytes = sizeof(bitstream);

	if (f)
		fprintf(stderr, "Starting %s %u -> %u\n", f->name, oldval, newval);

	if (!debug) {
		uint32_t pitches[3] = { input_width, input_width };

		void *data[3];

		data[0] = calloc(input_height, input_width);
		data[1] = calloc(input_height/2, input_width);
		data[2] = NULL;

		do {
			if (save)
				ok(vdp_video_surface_get_bits_y_cb_cr(surf, VDP_YCBCR_FORMAT_NV12, data, pitches));
			ok(vdp_decoder_render(dec, surf, (void*)info, 1, &buffer));
		} while ((save = save_data(VS_H263, cur)) == -EAGAIN || (save == -EIO && ++failed_tries < 3));

		ok(vdp_video_surface_get_bits_y_cb_cr(surf, VDP_YCBCR_FORMAT_NV12, data, pitches));
		free(data[0]);
		free(data[1]);

		if (save < 0) {
			if (!f || oldval == newval) {
				// Reference streams shouldn't be failing
				assert(!!!"WTF?");
				return;
			}
			fprintf(stderr, "Failed on %s %u->%u, stream was likely invalid\n", f->name, oldval, newval);
//			free((void*)buffer.bitstream);
			free(cur);
			return;
		}
	} else
		ok(vdp_decoder_render(dec, surf, (void*)info, 1, &buffer));

//	free((void*)buffer.bitstream);
	if (f) {
		compare_data(ref, cur, f, oldval, newval);
		free(cur);
	}
}

static void fuzz_h263(VdpVideoMixer mix, VdpVideoSurface *surf, VdpOutputSurface *osurf)
{
	VdpPictureInfoMPEG4Part2 info, template[1] = { { } };
	uint32_t i, j;
	VdpDecoder dec;
	VdpStatus ret;
	struct snap *ref = calloc(ARRAY_SIZE(template), sizeof(*ref));

	for (i = 0; i < ARRAY_SIZE(template); ++i) {
		template[i] = (VdpPictureInfoMPEG4Part2){
			.forward_reference = VDP_INVALID_HANDLE,
			.backward_reference = VDP_INVALID_HANDLE,
			.trd = { 0, 0 },
			.trb = { 0, 0 },
			.vop_time_increment_resolution = 1,
			.vop_coding_type = 2,
			.vop_fcode_backward = 1,
			.vop_fcode_forward = 1,
			.resync_marker_disable = 0,
			.interlaced = 0,
			.quant_type = 0,
			.quarter_sample = 0,
			.short_video_header = 0,
			.rounding_control = 0,
			.alternate_vertical_scan_flag = 0,
			.top_field_first = 0,
		};
		for (j = 0; j < 64; ++j) {
			template[i].intra_quantizer_matrix[j] = j;
			template[i].non_intra_quantizer_matrix[j] = j;
		}
	}

	struct fuzz fuzzy[] = {
#define entry(type, rest...) { \
	__builtin_offsetof(VdpPictureInfoMPEG4Part2, type), \
	sizeof(((VdpPictureInfoMPEG4Part2*)NULL)->type), \
	#type, rest \
}
#define entry_bool(type) entry(type, 0, { 0, 1 })

		entry(trd[0], 2, { 0, -1 }),
		entry(trd[1], 2, { 0, -1 }),
		entry(trb[0], 2, { 0, -1 }),
		entry(trb[1], 2, { 0, -1 }),
		entry(vop_time_increment_resolution, 2, { 1, 0xffff }),
		entry(vop_coding_type, 3, { 0, 1, 2 }),
		entry(vop_fcode_backward, 2, { 1, 7 }),
		entry(vop_fcode_forward, 2, { 1, 7 }),
		entry_bool(resync_marker_disable),
		entry_bool(interlaced),
		entry_bool(quant_type),
		entry_bool(quarter_sample),
		entry_bool(short_video_header),
		entry_bool(rounding_control),
		entry_bool(alternate_vertical_scan_flag),
		entry_bool(top_field_first),
	};
#undef entry
#undef entry_bool

	ok(vdp_decoder_create(dev, VDP_DECODER_PROFILE_MPEG4_PART2_ASP, input_width, input_height, 2, &dec));

	clear_data();
	// Render first, then retrieve bits to force serialization
	for (i = 0; i < ARRAY_SIZE(template); ++i)
		action_h263(dec, surf[0], &template[i], &ref[i], NULL, 0, 0);

	fprintf(stderr, "Reference rendering done\n");

	for (i = 0; i < ARRAY_SIZE(fuzzy); ++i) {
		int oldval = 0, tidx = fuzzy[i].template_idx;
		memcpy(&info, &template[tidx], sizeof(template[0]));
		memcpy(&oldval, (char*)&template[tidx] + fuzzy[i].offset, fuzzy[i].size_bytes);
		if (fuzzy[i].single_values) {
			for (j = 0; j < fuzzy[i].single_values; ++j) {
				memcpy((char*)&info + fuzzy[i].offset, &fuzzy[i].values[j], fuzzy[i].size_bytes);
				action_h263(dec, surf[0], &info, &ref[tidx], &fuzzy[i], oldval, fuzzy[i].values[j]);
			}
		} else {
			for (j = fuzzy[i].values[0]; j <= fuzzy[i].values[1]; ++j) {
				memcpy((char*)&info + fuzzy[i].offset, &j, fuzzy[i].size_bytes);
				action_h263(dec, surf[0], &info, &ref[tidx], &fuzzy[i], oldval, j);
			}
		}
	}
	fprintf(stderr, "Done, destroying renderer\n");
	free(ref);
	vdp_decoder_destroy(dec);
}

static void action_h264(VdpDecoder dec, VdpVideoSurface surf, VdpPictureInfoH264 *info, struct snap *ref,
struct fuzz *f, uint32_t oldval, uint32_t newval)
{
	struct snap *cur = f ? calloc(1, sizeof(*cur)) : ref;
	VdpStatus ret;
	int debug = 1;
	char bitstream[0] = {};
	int save = 0, failed_tries = 0;
	VdpBitstreamBuffer buffer;

	buffer.struct_version = VDP_BITSTREAM_BUFFER_VERSION;
	buffer.bitstream = bitstream;
	buffer.bitstream_bytes = sizeof(bitstream);

	if (f)
		fprintf(stderr, "Starting %s %u -> %u\n", f->name, oldval, newval);

	if (!debug) {
		uint32_t pitches[3] = { input_width, input_width };

		void *data[3];

		data[0] = calloc(input_height, input_width);
		data[1] = calloc(input_height/2, input_width);
		data[2] = NULL;

		do {
			if (save)
				ok(vdp_video_surface_get_bits_y_cb_cr(surf, VDP_YCBCR_FORMAT_NV12, data, pitches));
			ok(vdp_decoder_render(dec, surf, (void*)info, 1, &buffer));
		} while ((save = save_data(VS_H264, cur)) == -EAGAIN || (save == -EIO && ++failed_tries < 3));

		ok(vdp_video_surface_get_bits_y_cb_cr(surf, VDP_YCBCR_FORMAT_NV12, data, pitches));
		free(data[0]);
		free(data[1]);

		if (save < 0) {
			if (!f || oldval == newval) {
				// Reference streams shouldn't be failing
				assert(!!!"WTF?");
				return;
			}
			fprintf(stderr, "Failed on %s %u->%u, stream was likely invalid\n", f->name, oldval, newval);
//			free((void*)buffer.bitstream);
			free(cur);
			return;
		}
	} else
		ok(vdp_decoder_render(dec, surf, (void*)info, 1, &buffer));

//	free((void*)buffer.bitstream);
	if (f) {
		compare_data(ref, cur, f, oldval, newval);
		free(cur);
	}
}

static void fuzz_h264(VdpVideoMixer mix, VdpVideoSurface *surf, VdpOutputSurface *osurf)
{
	VdpPictureInfoH264 info, template[1] = { { } };
	uint32_t i, j;
	VdpDecoder dec;
	VdpStatus ret;
	struct snap *ref = calloc(ARRAY_SIZE(template), sizeof(*ref));

	for (i = 0; i < ARRAY_SIZE(template); ++i) {
		template[i] = (VdpPictureInfoH264){
			.slice_count = 1,
			.field_order_cnt = { 0, 1 },
			.is_reference = 0,
			.frame_num = 0,
			.field_pic_flag = 1,
			.bottom_field_flag = 0,
			.num_ref_frames = 0,
			.mb_adaptive_frame_field_flag = 0,
			.constrained_intra_pred_flag = 0,
			.weighted_pred_flag = 0,
			.weighted_bipred_idc = 0,
			.frame_mbs_only_flag = 0,
			.transform_8x8_mode_flag = 0,
			.chroma_qp_index_offset = 0,
			.second_chroma_qp_index_offset = 0,
			.pic_init_qp_minus26 = 0,
			.num_ref_idx_l0_active_minus1 = 0,
			.num_ref_idx_l1_active_minus1 = 0,
			.log2_max_frame_num_minus4 = 0,
			.delta_pic_order_always_zero_flag = 0,
			.direct_8x8_inference_flag = 0,
			.entropy_coding_mode_flag = 0,
			.pic_order_present_flag = 0,
			.deblocking_filter_control_present_flag = 0,
			.redundant_pic_cnt_present_flag = 0,
		};
		for (j = 0; j < 6; ++j) {
			int k;

			for (k = 0; k < 16; ++k)
				template[i].scaling_lists_4x4[j][k] = k << 4;
		}

		for (j = 0; j < 2; ++j) {
			int k;

			for (k = 0; k < 64; ++k)
				template[i].scaling_lists_8x8[j][k] = k << 2;
		}

		for (j = 0; j < 16; ++j)
			template[i].referenceFrames[j].surface = VDP_INVALID_HANDLE;
	}

	struct fuzz fuzzy[] = {
#define entry(type, rest...) { \
	__builtin_offsetof(VdpPictureInfoH264, type), \
	sizeof(((VdpPictureInfoH264*)NULL)->type), \
	#type, rest \
}
#define entry_bool(type) entry(type, 0, { 0, 1 })

		entry(slice_count, 1, { input_height/16 }),
		entry(field_order_cnt, 2, { 1LL << 32, 1LL }),
		entry_bool(is_reference),
		entry(frame_num, 2, { 0, 0xffff }),
		entry_bool(field_pic_flag),
		entry_bool(bottom_field_flag),
		entry(num_ref_frames, 0, { 0, 1 }),
		entry_bool(mb_adaptive_frame_field_flag),
		entry_bool(constrained_intra_pred_flag),
		entry_bool(weighted_pred_flag),
		entry(weighted_bipred_idc, 0, { 0, 2 }),
		entry_bool(frame_mbs_only_flag),
		entry_bool(transform_8x8_mode_flag),
		entry(chroma_qp_index_offset, 2, { 0, -1 }),
		entry(second_chroma_qp_index_offset, 2, { 0, -1 }),
		entry(pic_init_qp_minus26, 2, { 0, -1 }),
		entry(pic_order_cnt_type, 0, { 0, 2 }),
		entry(log2_max_pic_order_cnt_lsb_minus4, 2, { 0, 12 }),

		entry(num_ref_idx_l0_active_minus1, 2, { 0, 1 }),
		entry(num_ref_idx_l1_active_minus1, 2, { 0, 1 }),
		entry(log2_max_frame_num_minus4, 2, { 0, 1 }),

		entry_bool(delta_pic_order_always_zero_flag),
		entry_bool(direct_8x8_inference_flag),
		entry_bool(entropy_coding_mode_flag),
		entry_bool(pic_order_present_flag),
		entry_bool(deblocking_filter_control_present_flag),
		entry_bool(redundant_pic_cnt_present_flag),
	};
#undef entry
#undef entry_bool

	ok(vdp_decoder_create(dev, VDP_DECODER_PROFILE_H264_HIGH, input_width, input_height, 16, &dec));

	clear_data();
	// Render first, then retrieve bits to force serialization
	for (i = 0; i < ARRAY_SIZE(template); ++i)
		action_h264(dec, surf[0], &template[i], &ref[i], NULL, 0, 0);

	fprintf(stderr, "Reference rendering done\n");

	for (i = 0; i < ARRAY_SIZE(fuzzy); ++i) {
		int oldval = 0, tidx = fuzzy[i].template_idx;
		memcpy(&info, &template[tidx], sizeof(template[0]));
		memcpy(&oldval, (char*)&template[tidx] + fuzzy[i].offset, fuzzy[i].size_bytes);
		if (fuzzy[i].single_values) {
			for (j = 0; j < fuzzy[i].single_values; ++j) {
				memcpy((char*)&info + fuzzy[i].offset, &fuzzy[i].values[j], fuzzy[i].size_bytes);
				action_h264(dec, surf[0], &info, &ref[tidx], &fuzzy[i], oldval, fuzzy[i].values[j]);
			}
		} else {
			for (j = fuzzy[i].values[0]; j <= fuzzy[i].values[1]; ++j) {
				memcpy((char*)&info + fuzzy[i].offset, &j, fuzzy[i].size_bytes);
				action_h264(dec, surf[0], &info, &ref[tidx], &fuzzy[i], oldval, j);
			}
		}
	}
	fprintf(stderr, "Done, destroying renderer\n");
	free(ref);
	vdp_decoder_destroy(dec);
}

static void action_vc1(VdpDecoder dec, VdpVideoSurface surf, VdpPictureInfoVC1 *info, struct snap *ref,
struct fuzz *f, uint32_t oldval, uint32_t newval)
{
	struct snap *cur = f ? calloc(1, sizeof(*cur)) : ref;
	VdpStatus ret;
	int debug = 1;
	char bitstream[0] = {};
	int save = 0, failed_tries = 0;
	VdpBitstreamBuffer buffer;

	buffer.struct_version = VDP_BITSTREAM_BUFFER_VERSION;
	buffer.bitstream = bitstream;
	buffer.bitstream_bytes = sizeof(bitstream);

	if (f)
		fprintf(stderr, "Starting %s %u -> %u\n", f->name, oldval, newval);

	if (!debug) {
		uint32_t pitches[3] = { input_width, input_width };

		void *data[3];

		data[0] = calloc(input_height, input_width);
		data[1] = calloc(input_height/2, input_width);
		data[2] = NULL;

		do {
			if (save)
				ok(vdp_video_surface_get_bits_y_cb_cr(surf, VDP_YCBCR_FORMAT_NV12, data, pitches));
			ok(vdp_decoder_render(dec, surf, (void*)info, 1, &buffer));
		} while ((save = save_data(VS_H263, cur)) == -EAGAIN || (save == -EIO && ++failed_tries < 3));

		ok(vdp_video_surface_get_bits_y_cb_cr(surf, VDP_YCBCR_FORMAT_NV12, data, pitches));
		free(data[0]);
		free(data[1]);

		if (save < 0) {
			if (!f || oldval == newval) {
				// Reference streams shouldn't be failing
				assert(!!!"WTF?");
				return;
			}
			fprintf(stderr, "Failed on %s %u->%u, stream was likely invalid\n", f->name, oldval, newval);
//			free((void*)buffer.bitstream);
			free(cur);
			return;
		}
	} else
		ok(vdp_decoder_render(dec, surf, (void*)info, 1, &buffer));

//	free((void*)buffer.bitstream);
	if (f) {
		compare_data(ref, cur, f, oldval, newval);
		free(cur);
	}
}

static void fuzz_vc1(VdpVideoMixer mix, VdpVideoSurface *surf, VdpOutputSurface *osurf)
{
	VdpPictureInfoVC1 info, template[1] = { { } };
	uint32_t i, j;
	VdpDecoder dec;
	VdpStatus ret;
	struct snap *ref = calloc(ARRAY_SIZE(template), sizeof(*ref));

	for (i = 0; i < ARRAY_SIZE(template); ++i) {
		template[i] = (VdpPictureInfoVC1){
			.forward_reference = VDP_INVALID_HANDLE,
			.backward_reference = VDP_INVALID_HANDLE,
			.slice_count = 1,
			.picture_type = 3,
			.frame_coding_mode = 3,

			/* VC-1 chapter 6.1 */
			.postprocflag = 0,
			.pulldown = 0,
			.interlace = 1,
			.tfcntrflag = 0,
			.finterpflag = 0,
			.psf = 0,

			/* VC-1 chapter 6.2 */
			.dquant = 0,
			.panscan_flag = 0,
			.refdist_flag = 0,
			.quantizer = 0,
			.extended_mv = 0,
			.extended_dmv = 0,
			.overlap = 0,
			.vstransform = 0,
			.loopfilter = 0,
			.fastuvmc = 0,
			.range_mapy_flag = 1,
			.range_mapy = 0,
			.range_mapuv_flag = 1,
			.range_mapuv = 0,

			/* simple/main only, VC-1 appendix J */
			.multires = 0,
			.syncmarker = 0,
			.rangered = 0,
			.maxbframes = 1,

			/* VC-1 chapter 7.1.1 */
			.deblockEnable = 0,
			.pquant = 0,
		};
	}

	struct fuzz fuzzy[] = {
#define entry(type, rest...) { \
	__builtin_offsetof(VdpPictureInfoVC1, type), \
	sizeof(((VdpPictureInfoVC1*)NULL)->type), \
	#type, rest \
}
#define entry_bool(type) entry(type, 0, { 0, 1 })

		entry(slice_count, 2, { 1, input_height/16 }),
		entry(picture_type, 4, { 0, 1, 3, 4 }),
		entry(frame_coding_mode, 3, { 0, 2, 3 }),

		entry_bool(postprocflag),
		entry_bool(pulldown),
		entry_bool(interlace),
		entry_bool(tfcntrflag),
		entry_bool(finterpflag),
		entry_bool(psf),

		entry(dquant, 2, { 0, 3 }),
		entry_bool(panscan_flag),
		entry_bool(refdist_flag),
		entry(quantizer, 2, {0, 3}),
		entry_bool(extended_mv),
		entry_bool(extended_dmv),
		entry_bool(overlap),
		entry_bool(vstransform),
		entry_bool(loopfilter),
		entry_bool(fastuvmc),
		entry_bool(range_mapy_flag),
		entry(range_mapy, 2, {0, 7}),
		entry_bool(range_mapuv_flag),
		entry(range_mapuv, 2, {0, 7}),

		entry_bool(multires),
		entry_bool(syncmarker),
		entry_bool(rangered),
		entry(maxbframes, 2, { 0, 7 }),
		entry_bool(deblockEnable),
		entry(pquant, 0, { 0, 12 }),
	};
#undef entry
#undef entry_bool

	ok(vdp_decoder_create(dev, VDP_DECODER_PROFILE_VC1_ADVANCED, input_width, input_height, 2, &dec));

	clear_data();
	// Render first, then retrieve bits to force serialization
	for (i = 0; i < ARRAY_SIZE(template); ++i)
		action_vc1(dec, surf[0], &template[i], &ref[i], NULL, 0, 0);

	fprintf(stderr, "Reference rendering done\n");

	for (i = 0; i < ARRAY_SIZE(fuzzy); ++i) {
		int oldval = 0, tidx = fuzzy[i].template_idx;
		memcpy(&info, &template[tidx], sizeof(template[0]));
		memcpy(&oldval, (char*)&template[tidx] + fuzzy[i].offset, fuzzy[i].size_bytes);
		if (fuzzy[i].single_values) {
			for (j = 0; j < fuzzy[i].single_values; ++j) {
				memcpy((char*)&info + fuzzy[i].offset, &fuzzy[i].values[j], fuzzy[i].size_bytes);
				action_vc1(dec, surf[0], &info, &ref[tidx], &fuzzy[i], oldval, fuzzy[i].values[j]);
			}
		} else {
			for (j = fuzzy[i].values[0]; j <= fuzzy[i].values[1]; ++j) {
				memcpy((char*)&info + fuzzy[i].offset, &j, fuzzy[i].size_bytes);
				action_vc1(dec, surf[0], &info, &ref[tidx], &fuzzy[i], oldval, j);
			}
		}
	}
	fprintf(stderr, "Done, destroying renderer\n");
	free(ref);
	vdp_decoder_destroy(dec);
}

int main(int argc, char *argv[]) {
   Display *display;
   Window root, window;
   VdpStatus ret;
   VdpPresentationQueueTarget target;
   VdpVideoSurface surf[8];
   VdpOutputSurface osurf[8];
   VdpVideoMixer mixer;
   VdpProcamp camp = { VDP_PROCAMP_VERSION, 0., 1., 1., 0. };
   VdpCSCMatrix mat;
   int i;

   VdpVideoMixerFeature mixer_features[] = {
   };
   VdpVideoMixerParameter mixer_parameters[] = {
      VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
      VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT
   };
   const void *mixer_values[ARRAY_SIZE(mixer_parameters)] = {
      &input_width,
      &input_height
   };

   if (!open_map())
      return 1;

   /* This requires libvdpau_trace, which is available in libvdpau.git */
   setenv("VDPAU_TRACE", "1", 0);

   display = XOpenDisplay(NULL);
   root = XDefaultRootWindow(display);
   window = XCreateSimpleWindow(display, root, 0, 0, output_width, output_height, 0, 0, 0);
   XSelectInput(display, window, ExposureMask | KeyPressMask);
   XMapWindow(display, window);
   XSync(display, 0);

   ret = vdp_device_create_x11(display, 0, &dev, &vdp_get_proc_address);
   assert(ret == VDP_STATUS_OK);

   load_vdpau(dev);
   vdp_generate_csc_matrix(&camp, VDP_COLOR_STANDARD_SMPTE_240M, &mat);

   ok(vdp_presentation_queue_target_create_x11(dev, window, &target));
   for (i = 0; i < ARRAY_SIZE(surf); ++i) {
      ok(vdp_video_surface_create(dev, VDP_CHROMA_TYPE_420, input_width, input_height, &surf[i]));
      ok(vdp_output_surface_create(dev, VDP_COLOR_TABLE_FORMAT_B8G8R8X8, output_width, output_height, &osurf[i]));
   }
   ok(vdp_video_mixer_create(dev, ARRAY_SIZE(mixer_features), mixer_features,
                             ARRAY_SIZE(mixer_parameters), mixer_parameters, mixer_values, &mixer));

   fuzz_vc1(mixer, surf, osurf);
   fuzz_mpeg(mixer, surf, osurf);
   fuzz_h263(mixer, surf, osurf);
   fuzz_h264(mixer, surf, osurf);

   vdp_video_mixer_destroy(mixer);
   for (i = ARRAY_SIZE(surf); i > 0; i--) {
      vdp_output_surface_destroy(osurf[i - 1]);
      vdp_video_surface_destroy(surf[i - 1]);
   }
   vdp_presentation_queue_target_destroy(target);
   vdp_device_destroy(dev);
   XDestroyWindow(display, window);
   XCloseDisplay(display);
   return 0;
}

static void load_vdpau(VdpDevice dev) {
   VdpStatus ret;
#define GET_POINTER(fid, fn) ret = vdp_get_proc_address(dev, fid, (void**)&fn); assert(ret == VDP_STATUS_OK);
   GET_POINTER(VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER,                          vdp_preemption_callback_register);
   GET_POINTER(VDP_FUNC_ID_GET_ERROR_STRING,                                      vdp_get_error_string);
   GET_POINTER(VDP_FUNC_ID_GET_API_VERSION,                                       vdp_get_api_version);
   GET_POINTER(VDP_FUNC_ID_GET_INFORMATION_STRING,                                vdp_get_information_string);
   GET_POINTER(VDP_FUNC_ID_DEVICE_DESTROY,                                        vdp_device_destroy);
   GET_POINTER(VDP_FUNC_ID_GENERATE_CSC_MATRIX,                                   vdp_generate_csc_matrix);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES,                      vdp_video_surface_query_capabilities);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES, vdp_video_surface_query_get_put_bits_y_cb_cr_capabilities);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_CREATE,                                  vdp_video_surface_create);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_DESTROY,                                 vdp_video_surface_destroy);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS,                          vdp_video_surface_get_parameters);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR,                        vdp_video_surface_get_bits_y_cb_cr);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR,                        vdp_video_surface_put_bits_y_cb_cr);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES,                     vdp_output_surface_query_capabilities);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES, vdp_output_surface_query_get_put_bits_native_capabilities);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES,    vdp_output_surface_query_put_bits_indexed_capabilities);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES,    vdp_output_surface_query_put_bits_y_cb_cr_capabilities);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_CREATE,                                 vdp_output_surface_create);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY,                                vdp_output_surface_destroy);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS,                         vdp_output_surface_get_parameters);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE,                        vdp_output_surface_get_bits_native);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE,                        vdp_output_surface_put_bits_native);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED,                       vdp_output_surface_put_bits_indexed);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR,                       vdp_output_surface_put_bits_y_cb_cr);
   GET_POINTER(VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES,                     vdp_bitmap_surface_query_capabilities);
   GET_POINTER(VDP_FUNC_ID_BITMAP_SURFACE_CREATE,                                 vdp_bitmap_surface_create);
   GET_POINTER(VDP_FUNC_ID_BITMAP_SURFACE_DESTROY,                                vdp_bitmap_surface_destroy);
   GET_POINTER(VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS,                         vdp_bitmap_surface_get_parameters);
   GET_POINTER(VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE,                        vdp_bitmap_surface_put_bits_native);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE,                  vdp_output_surface_render_output_surface);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE,                  vdp_output_surface_render_bitmap_surface);
   GET_POINTER(VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES,                            vdp_decoder_query_capabilities);
   GET_POINTER(VDP_FUNC_ID_DECODER_CREATE,                                        vdp_decoder_create);
   GET_POINTER(VDP_FUNC_ID_DECODER_DESTROY,                                       vdp_decoder_destroy);
   GET_POINTER(VDP_FUNC_ID_DECODER_GET_PARAMETERS,                                vdp_decoder_get_parameters);
   GET_POINTER(VDP_FUNC_ID_DECODER_RENDER,                                        vdp_decoder_render);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT,                     vdp_video_mixer_query_feature_support);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT,                   vdp_video_mixer_query_parameter_support);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT,                   vdp_video_mixer_query_attribute_support);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE,               vdp_video_mixer_query_parameter_value_range);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE,               vdp_video_mixer_query_attribute_value_range);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_CREATE,                                    vdp_video_mixer_create);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES,                       vdp_video_mixer_set_feature_enables);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES,                      vdp_video_mixer_set_attribute_values);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT,                       vdp_video_mixer_get_feature_support);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES,                       vdp_video_mixer_get_feature_enables);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES,                      vdp_video_mixer_get_parameter_values);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES,                      vdp_video_mixer_get_attribute_values);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_DESTROY,                                   vdp_video_mixer_destroy);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_RENDER,                                    vdp_video_mixer_render);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY,                     vdp_presentation_queue_target_destroy);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE,                             vdp_presentation_queue_create);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY,                            vdp_presentation_queue_destroy);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR,               vdp_presentation_queue_set_background_color);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR,               vdp_presentation_queue_get_background_color);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME,                           vdp_presentation_queue_get_time);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY,                            vdp_presentation_queue_display);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE,           vdp_presentation_queue_block_until_surface_idle);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS,               vdp_presentation_queue_query_surface_status);
   GET_POINTER(VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER,                          vdp_preemption_callback_register);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11,                  vdp_presentation_queue_target_create_x11);
}
