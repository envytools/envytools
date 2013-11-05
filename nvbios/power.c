/*
 * Copyright (C) 2013 Martin Peres
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "bios.h"

struct P_known_tables {
	uint8_t offset;
	uint16_t *ptr;
	const char *name;
};

int parse_at(struct envy_bios *bios, struct envy_bios_power *power, 
	     int idx, int offset, const char ** name)
{
	struct P_known_tables p1_tbls[] = {
		{ 0x00, &power->perf.offset, "PERFORMANCE" },
		{ 0x04, &power->timing.offset, "MEMORY TIMINGS" },
		{ 0x0c, &power->therm.offset, "THERMAL" },
		{ 0x10, &power->volt.offset, "VOLTAGE" },
		{ 0x15, &power->unk.offset, "UNK" }
	};
	struct P_known_tables p2_tbls[] = {
		{ 0x00, &power->perf.offset, "PERFORMANCE" },
		{ 0x04, &power->timing_map.offset, "MEMORY TIMINGS MAPPING" },
		{ 0x08, &power->timing.offset, "MEMORY TIMINGS" },
		{ 0x0c, &power->volt.offset, "VOLTAGE" },
		{ 0x10, &power->therm.offset, "THERMAL"  },
		{ 0x18, &power->unk.offset, "UNK" },
		{ 0x20, &power->volt_map.offset, "VOLT MAPPING" }
	};
	struct P_known_tables *tbls;
	int entries_count = 0;

	if (power->bit->version == 0x1) {
		tbls = p1_tbls;
		entries_count = (sizeof(p1_tbls) / sizeof(struct P_known_tables));
	} else if (power->bit->version == 0x2) {
		tbls = p2_tbls;
		entries_count = (sizeof(p2_tbls) / sizeof(struct P_known_tables));
	} else
		return -EINVAL;

	/* either we address by offset or idx */
	if (idx != -1 && offset != -1)
		return -EINVAL;

	/* lookup the index by the table's offset */
	if (offset > -1) {
		idx = 0;
		while (idx < entries_count && tbls[idx].offset != offset)
			idx++;
	}

	/* check the index */
	if (idx < 0 || idx >= entries_count)
		return -ENOENT;
	
	/* check the table has the right size */
	if (tbls[idx].offset + 2 > power->bit->t_len)
		return -ENOENT;
	
	if (name)
		*name = tbls[idx].name;
	
	return bios_u16(bios, 
			power->bit->t_offset + tbls[idx].offset, 
			tbls[idx].ptr);
}

int envy_bios_parse_bit_P (struct envy_bios *bios, struct envy_bios_bit_entry *bit) {
	struct envy_bios_power *power = &bios->power;
	int idx = 0;

	power->bit = bit;
	while (!parse_at(bios, power, idx, -1, NULL))
		idx++;
	return 0;
}

void envy_bios_print_bit_P (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power *power = &bios->power;
	const char *name;
	uint16_t addr;
	int ret = 0, i = 0;
	
	if (!power->bit || !(mask & ENVY_BIOS_PRINT_PERF))
		return;

	fprintf(out, "BIT table 'P' at 0x%x, version %i\n", 
		power->bit->offset, power->bit->version);

	for (i = 0; i < power->bit->t_len; i+=2) {
		ret = bios_u16(bios, power->bit->t_offset + i, &addr);
		if (!ret && addr) {
			name = "UNKNOWN";
			ret = parse_at(bios, power, -1, i, &name);
			fprintf(out, "0x%02x: 0x%x => %s TABLE\n", i, addr, name);
		}
	}
	
	fprintf(out, "\n");

	return 0;
}
