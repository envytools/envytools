/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "vstream.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>

int vs_byte(struct bitstream *str) {
	if (str->dir == VS_ENCODE) {
		switch (str->type) {
			case VS_MPEG12:
				if (str->curbyte < 2 && str->zero_bytes >= 2) {
					fprintf(stderr, "00 00 0%d emitted!\n", str->curbyte);
					return 1;
				}
				break;
			case VS_H264:
				if (str->curbyte < 4 && str->zero_bytes == 2) {
					/* escape */
					ADDARRAY(str->bytes, 3);
					str->zero_bytes = 0;
				}
				break;
			default:
				abort();
		}
		ADDARRAY(str->bytes, str->curbyte);
		if (!str->curbyte)
			str->zero_bytes++;
		else
			str->zero_bytes = 0;
		str->curbyte = 0;
	} else {
		if (str->bytepos >= str->bytesnum) {
			fprintf(stderr, "End of bitstream in a NAL!\n");
			return 1;
		}
		str->curbyte = str->bytes[str->bytepos++];
		switch (str->type) {
			case VS_MPEG12:
				if (str->curbyte < 2 && str->zero_bytes >= 2) {
					fprintf(stderr, "00 00 0%d read in a NAL!\n", str->curbyte);
					return 1;
				}
				break;
			case VS_H264:
				if (str->zero_bytes == 2) {
					switch (str->curbyte) {
						case 0:
						case 1:
						case 2:
							fprintf(stderr, "00 00 0%d read in a NAL!\n", str->curbyte);
							return 1;
						case 3:
							if (str->bytepos >= str->bytesnum) {
								fprintf(stderr, "End of bitstream in a NAL!\n");
								return 1;
							}
							str->zero_bytes = 0;
							str->curbyte = str->bytes[str->bytepos++];
							if (str->curbyte > 3) {
								fprintf(stderr, "Invalid escape sequence: 00 00 03 %02x!\n", str->curbyte);
								return 1;
							}
							break;
					}
				}
				break;
			default:
				abort();
		}
		if (!str->curbyte)
			str->zero_bytes++;
		else
			str->zero_bytes = 0;
		str->hasbyte = 1;
	}
	str->bitpos = 7;
	return 0;
}

int vs_u(struct bitstream *str, uint32_t *val, int size) {
	int i;
	if (str->dir == VS_ENCODE) {
		for (i = 0; i < size; i++) {
			int bit = *val >> (size - 1 - i) & 1;
			str->curbyte |= bit << str->bitpos;
			if (!str->bitpos) {
				if (vs_byte(str))
					return 1;
			} else {
				str->bitpos--;
			}
		}
		return 0;
	} else {
		*val = 0;
		for (i = 0; i < size; i++) {
			if (!str->hasbyte) {
				if (vs_byte(str))
					return 1;
			}
			int bit = str->curbyte >> str->bitpos & 1;
			*val |= bit << (size - 1 - i);
			if (!str->bitpos) {
				str->hasbyte = 0;
				str->bitpos = 7;
			} else {
				str->bitpos--;
			}
		}
		return 0;
	}
}

int vs_ue(struct bitstream *str, uint32_t *val) {
	int lzb = 0;
	uint32_t tmp;
	if (str->dir == VS_ENCODE) {
		tmp = 0;
		if (*val >= (uint32_t)0xffffffff) {
			fprintf (stderr, "Exp-Golomb number larger than 2^32-2\n");
			return 1;
		}
		while (*val >= (uint32_t)(1u << (lzb + 1)) - 1) {
			if (vs_u(str, &tmp, 1))
				return 1;
			lzb++;
		}
		tmp = 1;
		if (vs_u(str, &tmp, 1))
			return 1;
		tmp = *val - ((1 << lzb) - 1);
		if (vs_u(str, &tmp, lzb))
			return 1;
		return 0;
	} else {
		do {
			if (vs_u(str, &tmp, 1))
				return 1;
			lzb++;
		} while (!tmp);
		lzb--;
		if (lzb > 31) {
			fprintf (stderr, "Exp-Golomb number larger than 2^32-2\n");
			return 1;
		}
		if (vs_u(str, &tmp, lzb))
			return 1;
		*val = tmp + (1u << lzb) - 1;
		return 0;
	}
}

int vs_se(struct bitstream *str, int32_t *val) {
	uint32_t tmp;
	if (str->dir == VS_ENCODE) {
		if (*val == (int32_t)(-0x7fffffff-1)) {
			fprintf (stderr, "Exp-Golomb signed number equal to -2^31\n");
			return 1;
		}
		if (*val > 0) {
			tmp = *val * 2 - 1;
		} else {
			tmp = -*val * 2;
		}
		return vs_ue(str, &tmp);
	} else {
		if (vs_ue(str, &tmp))
			return 1;
		if (tmp & 1) {
			*val = (tmp + 1) >> 1;
		} else {
			*val = -(tmp >> 1);
		}
		return 0;
	}
}

int vs_vlc(struct bitstream *str, uint32_t *val, const struct vs_vlc_val *tab) {
	if (str->dir == VS_ENCODE) {
		int i, j;
		for (i = 0; tab[i].blen; i++) {
			if (*val == tab[i].val) {
				uint32_t bit;
				for (j = 0; j < tab[i].blen; j++) {
					bit = tab[i].bits[j];
					if (vs_u(str, &bit, 1))
						return 1;
				}
				return 0;
			}
		}
		fprintf(stderr, "No VLC code for a value\n");
		return 1;
	} else {
		int i, j;
		uint32_t bit[32];
		int n = 0;
		for (i = 0; tab[i].blen; i++) {
			for (j = 0; j < tab[i].blen; j++) {
				if (j == n) {
					if (vs_u(str, &bit[j], 1)) return 1;
					n = j + 1;
				}
				if (bit[j] != tab[i].bits[j])
					break;
			}
			if (j == tab[i].blen) {
				*val = tab[i].val;
				return 0;
			}
		}
		fprintf(stderr, "Invalid VLC code\n");
		return 1;
	}
}

int vs_start(struct bitstream *str, uint32_t *val) {
	if (str->bitpos != 7) {
		fprintf (stderr, "Start code attempted at non-bytealigned position\n");
		return 1;
	}
	if (str->dir == VS_ENCODE) {
		ADDARRAY(str->bytes, 0);
		ADDARRAY(str->bytes, 0);
		ADDARRAY(str->bytes, 1);
		ADDARRAY(str->bytes, *val);
	} else {
		str->zero_bytes--;
		do {
			str->zero_bytes++;
			if (str->bytepos >= str->bytesnum) {
				fprintf(stderr, "End of bitstream when searching for a start code!\n");
				return 1;
			}
			str->curbyte = str->bytes[str->bytepos++];
		} while (str->curbyte == 0);
		if (str->curbyte != 1) {
			fprintf(stderr, "Found byte %08x when searching for a start code!\n", str->curbyte);
			return 1;
		}
		if (str->zero_bytes < 2) {
			fprintf(stderr, "Found premature byte %08x when searching for a start code!\n", str->curbyte);
			return 1;
		}
		if (str->bytepos >= str->bytesnum) {
			fprintf(stderr, "End of bitstream when searching for a start code!\n");
			return 1;
		}
		str->curbyte = str->bytes[str->bytepos++];
		str->zero_bytes = 0;
		*val = str->curbyte;
	}
	return 0;
}

int vs_search_start(struct bitstream *str) {
	if (str->dir != VS_DECODE) {
		fprintf (stderr, "vs_search_start called in encode mode!\n");
		return -1;
	}
	str->hasbyte = 0;
	str->bitpos = 7;
	while (1) {
		if (str->bytepos >= str->bytesnum)
			return 0;
		if (str->zero_bytes == 2 && str->bytes[str->bytepos] == 1)
			return 1;
		if (str->bytes[str->bytepos] == 0) {
			if (str->zero_bytes != 2)
				str->zero_bytes++;
		} else {
			str->zero_bytes = 0;
		}
		str->bytepos++;
	}
}

int vs_align_byte(struct bitstream *str, enum vs_align_byte_mode mode) {
	uint32_t pad;
	switch (mode) {
		case VS_ALIGN_0:
			pad = 0;
			break;
		case VS_ALIGN_1:
			pad = (1 << (str->bitpos + 1)) - 1;
			break;
		case VS_ALIGN_10:
			pad = 1 << str->bitpos;
			break;
	}
	if (str->dir == VS_ENCODE) {
		if (str->bitpos != 7) {
			str->curbyte |= pad;
			if (vs_byte(str))
				return 1;
		}
	} else {
		if (str->hasbyte) {
			uint32_t tmp;
			if (vs_u(str, &tmp, str->bitpos + 1))
				return 1;
			if (tmp != pad) {
				fprintf(stderr, "Byte alignment bits don't match!\n");
				return 1;
			}
		}
		str->hasbyte = 0;
		str->bitpos = 7;
	}
	return 0;
}

int vs_end(struct bitstream *str) {
	uint32_t one = 1;
	switch (str->type) {
		case VS_MPEG12:
			return vs_align_byte(str, VS_ALIGN_0);
		case VS_H264:
			if (vs_u(str, &one, 1))
				return 1;
			if (one != 1) {
				fprintf (stderr, "Wrong RBSP trailing bit!\n");
				return 1;
			}
			return vs_align_byte(str, VS_ALIGN_0);
			/* XXX: add the cabac special case */
	}
}

int vs_has_more_data(struct bitstream *str) {
	if (str->dir == VS_ENCODE) {
		fprintf (stderr, "vs_has_more_data called in encode mode\n");
		return -1;
	}
	int byte;
	int offs = 0;
	switch (str->type) {
		case VS_H264:
			if (!str->hasbyte) {
				if (str->bytepos == str->bytesnum) {
					fprintf (stderr, "no RBSP trailer byte\n");
					return -1;
				}
				offs = 1;
				byte = str->bytes[str->bytepos];
			} else {
				byte = str->curbyte;
				offs = 0;
			}
			byte &= (1 << str->bitpos+1) - 1;
			if (byte != (1 << str->bitpos))
				return 1;
			break;
		case VS_MPEG12:
			break;
	}
	offs += str->bytepos;
	if (offs < str->bytesnum && str->bytes[offs])
		return 1;
	if (offs+1 < str->bytesnum && str->bytes[offs+1])
		return 1;
	if (offs+2 < str->bytesnum && str->bytes[offs+2] > 2)
		return 1;
	/* followed by 00 00 00 or 00 00 01 or 00* EOF */
	return 0;

}

struct bitstream *vs_new_encode(enum vs_type type) {
	struct bitstream *res = calloc(sizeof *res, 1);
	res->dir = VS_ENCODE;
	res->type = type;
	res->bitpos = 7;
	return res;
}

struct bitstream *vs_new_decode(enum vs_type type, uint8_t *bytes, int bytesnum) {
	struct bitstream *res = calloc(sizeof *res, 1);
	res->dir = VS_DECODE;
	res->type = type;
	res->bytes = bytes;
	res->bytesnum = bytesnum;
	res->bitpos = 7;
	return res;
}

int vs_infer(struct bitstream *str, uint32_t *val, uint32_t ival) {
	if (str->dir == VS_ENCODE) {
		if (*val != ival) {
			fprintf (stderr, "Wrong infered value: %d != %d\n", *val, ival);
			return 1;
		}
	} else {
		*val = ival;
	}
	return 0;
}

void vs_destroy(struct bitstream *str) {
	free(str->bytes);
	free(str);
}
