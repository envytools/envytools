/*
 * Copyright (C) 2011-2012 Martin Peres <martin.peres@labri.fr>
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

#ifndef NVAMEMTIMING_H
#define NVAMEMTIMING_H
	#include <stdint.h>

	typedef enum { false = 0, true = 1 } bool;

	struct nvamemtiming_conf {
		int cnum;
		bool mmiotrace;
		uint16_t counter;

		enum { MODE_AUTO = 0, MODE_BITFIELD = 1, MODE_MANUAL = 2, MODE_DEEP = 3 } mode;
		union {
			struct {
				uint8_t index;
				uint8_t value;
			} manual;

			struct {
				uint8_t index;
			} bitfield;

			struct {
				uint8_t entry;
				uint16_t timing_entry_offset;
			} deep;
		};

		struct {
			const char *file;
			uint8_t *data;
			uint16_t length;

			uint16_t timing_table_offset;
			uint16_t timing_entry_offset;
			uint16_t timing_entry_length;
		} vbios;

		struct {
			uint8_t entry;
			uint8_t perflvl;
		} timing;
	};

	int vbios_read(const char *filename, uint8_t **vbios, uint16_t *length);

	int deep_dump(struct nvamemtiming_conf *conf);
	int shallow_dump(struct nvamemtiming_conf *conf);
	int bitfield_check(struct nvamemtiming_conf *conf);
	int manual_check(struct nvamemtiming_conf *conf);

#endif
