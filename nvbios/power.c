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
#include <assert.h>
#include <string.h>

/* nvbios.c */
void printscript (uint16_t soff);

int envy_bios_parse_power_unk14(struct envy_bios *bios);
int envy_bios_parse_power_fan_calib(struct envy_bios *bios);
int envy_bios_parse_power_unk1c(struct envy_bios *bios);
int envy_bios_parse_power_budget(struct envy_bios *bios);
int envy_bios_parse_power_boost(struct envy_bios *bios);
int envy_bios_parse_power_cstep(struct envy_bios *bios);
int envy_bios_parse_power_unk24(struct envy_bios *bios);
int envy_bios_parse_power_sense(struct envy_bios *bios);
int envy_bios_parse_power_base_clock(struct envy_bios *bios);
int envy_bios_parse_power_unk3c(struct envy_bios *bios);
int envy_bios_parse_power_unk40(struct envy_bios *bios);
int envy_bios_parse_power_unk44(struct envy_bios *bios);
int envy_bios_parse_power_unk48(struct envy_bios *bios);
int envy_bios_parse_power_unk4c(struct envy_bios *bios);
int envy_bios_parse_power_unk50(struct envy_bios *bios);
int envy_bios_parse_power_unk54(struct envy_bios *bios);
int envy_bios_parse_power_fan(struct envy_bios *bios);
int envy_bios_parse_power_fan_mgmt(struct envy_bios *bios);
int envy_bios_parse_power_unk60(struct envy_bios *bios);
int envy_bios_parse_power_unk64(struct envy_bios *bios);
int envy_bios_parse_power_unk68(struct envy_bios *bios);
int envy_bios_parse_power_unk6c(struct envy_bios *bios);
int envy_bios_parse_power_unk70(struct envy_bios *bios);
int envy_bios_parse_power_unk74(struct envy_bios *bios);
int envy_bios_parse_power_unk78(struct envy_bios *bios);
int envy_bios_parse_power_unk7c(struct envy_bios *bios);
int envy_bios_parse_power_unk80(struct envy_bios *bios);
int envy_bios_parse_power_unk84(struct envy_bios *bios);
int envy_bios_parse_power_unk88(struct envy_bios *bios);
int envy_bios_parse_power_unk8c(struct envy_bios *bios);
int envy_bios_parse_power_unk90(struct envy_bios *bios);
int envy_bios_parse_power_unk94(struct envy_bios *bios);
int envy_bios_parse_power_unk98(struct envy_bios *bios);

struct P_known_tables {
	uint8_t offset;
	uint32_t *ptr;
	const char *name;
};

static int parse_at(struct envy_bios *bios, struct envy_bios_power *power,
	     int idx, int offset, const char ** name)
{
	struct P_known_tables p1_tbls[] = {
		{ 0x00, &power->perf.offset, "PERFORMANCE" },
		{ 0x04, &power->timing.offset, "MEMORY TIMINGS" },
		{ 0x0c, &power->therm.offset, "THERMAL" },
		{ 0x10, &power->volt.offset, "VOLTAGE" },
		{ 0x15, &power->fan_calib.offset, "POWER UNK18" }
	};
	struct P_known_tables p2_tbls[] = {
		{ 0x00, &power->perf.offset, "PERFORMANCE" },
		{ 0x04, &power->timing_map.offset, "MEMORY TIMINGS MAPPING" },
		{ 0x08, &power->timing.offset, "MEMORY TIMINGS" },
		{ 0x0c, &power->volt.offset, "VOLTAGE" },
		{ 0x10, &power->therm.offset, "THERMAL"  },
		{ 0x14, &power->unk14.offset, "UNK14"  },
		{ 0x18, &power->fan_calib.offset, "FAN CALIBRATION" },
		{ 0x1c, &power->unk1c.offset, "POWER UNK1C" },
		{ 0x20, &power->volt_map.offset, "VOLT MAPPING" },
		{ 0x24, &power->unk24.offset, "POWER UNK24" },
		{ 0x28, &power->sense.offset, "POWER SENSE" },
		{ 0x2c, &power->budget.offset, "POWER BUDGET" },
		{ 0x30, &power->boost.offset, "BOOST" },
		{ 0x34, &power->cstep.offset, "CSTEP" },
		{ 0x38, &power->base_clock.offset, "POWER BASE CLOCK" },
		{ 0x3c, &power->unk3c.offset, "POWER UNK3C" },
		{ 0x40, &power->unk40.offset, "POWER UNK40" },
		{ 0x44, &power->unk44.offset, "POWER UNK44" },
		{ 0x48, &power->unk48.offset, "POWER UNK48" },
		{ 0x4c, &power->unk4c.offset, "POWER UNK4C" },
		{ 0x50, &power->unk50.offset, "POWER UNK50" },
		{ 0x54, &power->unk54.offset, "POWER UNK54" },
		{ 0x58, &power->fan.offset, "POWER FAN" },
		{ 0x5c, &power->fan_mgmt.offset, "POWER FAN_MGMT" },
		{ 0x60, &power->unk60.offset, "POWER UNK60" },
		{ 0x64, &power->unk64.offset, "POWER UNK64" },
		{ 0x68, &power->unk68.offset, "POWER UNK68" },
		{ 0x6c, &power->unk6c.offset, "POWER UNK6C" },
		{ 0x70, &power->unk70.offset, "POWER UNK70" },
		{ 0x74, &power->unk74.offset, "POWER UNK74" },
		{ 0x78, &power->unk78.offset, "POWER UNK78" },
		{ 0x7c, &power->unk7c.offset, "POWER UNK7C" },
		{ 0x80, &power->unk80.offset, "POWER UNK80" },
		{ 0x84, &power->unk84.offset, "POWER UNK84" },
		{ 0x88, &power->unk88.offset, "POWER UNK88" },
		{ 0x8c, &power->unk8c.offset, "POWER UNK8C" },
		{ 0x90, &power->unk90.offset, "POWER UNK90" },
		{ 0x94, &power->unk94.offset, "POWER UNK94" },
		{ 0x98, &power->unk98.offset, "POWER UNK98" },
	};
	struct P_known_tables *tbls;
	int entries_count = 0;
	int ret;

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

	uint32_t tblOffset;
	ret = bios_u32(bios, power->bit->t_offset + tbls[idx].offset, &tblOffset);
	*tbls[idx].ptr = tblOffset;
	return ret;
}

int envy_bios_parse_bit_P (struct envy_bios *bios, struct envy_bios_bit_entry *bit) {
	struct envy_bios_power *power = &bios->power;
	int idx = 0;

	power->bit = bit;
	while (!parse_at(bios, power, idx, -1, NULL))
		idx++;

	envy_bios_parse_power_unk14(bios);
	envy_bios_parse_power_fan_calib(bios);
	envy_bios_parse_power_unk1c(bios);
	envy_bios_parse_power_unk24(bios);
	envy_bios_parse_power_sense(bios);
	envy_bios_parse_power_budget(bios);
	envy_bios_parse_power_boost(bios);
	envy_bios_parse_power_cstep(bios);
	envy_bios_parse_power_base_clock(bios);
	envy_bios_parse_power_unk3c(bios);
	envy_bios_parse_power_unk40(bios);
	envy_bios_parse_power_unk44(bios);
	envy_bios_parse_power_unk48(bios);
	envy_bios_parse_power_unk4c(bios);
	envy_bios_parse_power_unk50(bios);
	envy_bios_parse_power_unk54(bios);
	envy_bios_parse_power_fan(bios);
	envy_bios_parse_power_fan_mgmt(bios);
	envy_bios_parse_power_unk60(bios);
	envy_bios_parse_power_unk64(bios);
	envy_bios_parse_power_unk68(bios);
	envy_bios_parse_power_unk6c(bios);
	envy_bios_parse_power_unk70(bios);
	envy_bios_parse_power_unk74(bios);
	envy_bios_parse_power_unk78(bios);
	envy_bios_parse_power_unk7c(bios);
	envy_bios_parse_power_unk80(bios);
	envy_bios_parse_power_unk84(bios);
	envy_bios_parse_power_unk88(bios);
	envy_bios_parse_power_unk8c(bios);
	envy_bios_parse_power_unk90(bios);
	envy_bios_parse_power_unk94(bios);
	envy_bios_parse_power_unk98(bios);

	return 0;
}

void envy_bios_print_bit_P (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power *power = &bios->power;
	const char *name;
	uint32_t addr;
	int ret = 0, i = 0;
	
	if (!power->bit || !(mask & ENVY_BIOS_PRINT_PERF))
		return;

	fprintf(out, "BIT table 'P' at 0x%x, version %i\n", 
		power->bit->offset, power->bit->version);

	for (i = 0; i < power->bit->t_len; i+=4) {
		ret = bios_u32(bios, power->bit->t_offset + i, &addr);
		if (!ret && addr) {
			name = "UNKNOWN";
			ret = parse_at(bios, power, -1, i, &name);
			fprintf(out, "0x%02x: 0x%x => %s TABLE\n", i, addr, name);
		}
	}
	
	fprintf(out, "\n");
}

int envy_bios_parse_power_unk14(struct envy_bios *bios) {
	struct envy_bios_power_unk14 *unk14 = &bios->power.unk14;
	int i, err = 0;

	if (!unk14->offset)
		return -EINVAL;

	bios_u8(bios, unk14->offset + 0x0, &unk14->version);
	switch(unk14->version) {
	case 0x10:
		err |= bios_u8(bios, unk14->offset + 0x1, &unk14->hlen);
		err |= bios_u8(bios, unk14->offset + 0x2, &unk14->rlen);
		err |= bios_u8(bios, unk14->offset + 0x3, &unk14->entriesnum);
		unk14->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK14 table version 0x%x\n", unk14->version);
		return -EINVAL;
	};

	err = 0;
	unk14->entries = malloc(unk14->entriesnum * sizeof(struct envy_bios_power_unk14_entry));
	for (i = 0; i < unk14->entriesnum; i++) {
		uint16_t data = unk14->offset + unk14->hlen + i * unk14->rlen;

		unk14->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk14(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk14 *unk14 = &bios->power.unk14;
	int i;

	if (!unk14->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk14->valid) {
		fprintf(out, "Failed to parse UNK14 table at 0x%x, version %x\n", unk14->offset, unk14->version);
		return;
	}

	fprintf(out, "UNK14 table at 0x%x, version %x\n", unk14->offset, unk14->version);
	envy_bios_dump_hex(bios, out, unk14->offset, unk14->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk14->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk14->entries[i].offset, unk14->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_fan_calib(struct envy_bios *bios) {
	struct envy_bios_power_fan_calib *fan_calib = &bios->power.fan_calib;
	int i, err = 0;

	if (!fan_calib->offset)
		return -EINVAL;

	bios_u8(bios, fan_calib->offset + 0x0, &fan_calib->version);
	switch(fan_calib->version) {
	case 0x10:
		err |= bios_u8(bios, fan_calib->offset + 0x1, &fan_calib->hlen);
		err |= bios_u8(bios, fan_calib->offset + 0x2, &fan_calib->rlen);
		err |= bios_u8(bios, fan_calib->offset + 0x3, &fan_calib->entriesnum);
		fan_calib->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown FAN CALIBRATION table version 0x%x\n", fan_calib->version);
		return -EINVAL;
	};

	err = 0;
	fan_calib->entries = calloc(fan_calib->entriesnum, sizeof(struct envy_bios_power_fan_calib_entry));
	for (i = 0; i < fan_calib->entriesnum; i++) {
		uint16_t data = fan_calib->offset + fan_calib->hlen + i * fan_calib->rlen;

		fan_calib->entries[i].offset = data;
		bios_u8(bios, data + 0x00, &fan_calib->entries[i].enable);
		bios_u8(bios, data + 0x01, &fan_calib->entries[i].mode);
		bios_u16(bios, data + 0x02, &fan_calib->entries[i].unk02);
		bios_u16(bios, data + 0x04, &fan_calib->entries[i].unk04);
		bios_u16(bios, data + 0x06, &fan_calib->entries[i].unk06);
		bios_u16(bios, data + 0x08, &fan_calib->entries[i].pwm_freq); /* not used by the blob */
		bios_u16(bios, data + 0x0a, &fan_calib->entries[i].calib_full_pwr);
		bios_u16(bios, data + 0x0c, &fan_calib->entries[i].calib_no_pwr);
		bios_u16(bios, data + 0x0e, &fan_calib->entries[i].unk0e);

		if (fan_calib->rlen >= 0x12)
			bios_u16(bios, data + 0x10, &fan_calib->entries[i].unk10);

		if (fan_calib->rlen >= 0x14)
			bios_u16(bios, data + 0x12, &fan_calib->entries[i].unk12);
	}

	return 0;
}

void envy_bios_print_power_fan_calib(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_fan_calib *fan_calib = &bios->power.fan_calib;
	int i;

	if (!fan_calib->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!fan_calib->valid) {
		fprintf(out, "Failed to parse FAN CALIBRATION table at 0x%x, version %x\n", fan_calib->offset, fan_calib->version);
		return;
	}

	fprintf(out, "FAN CALIBRATION table at 0x%x, version %x\n", fan_calib->offset, fan_calib->version);
	envy_bios_dump_hex(bios, out, fan_calib->offset, fan_calib->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < fan_calib->entriesnum; i++) {
		struct envy_bios_power_fan_calib_entry *e = &fan_calib->entries[i];
		if (e->enable == 1 && (e->mode & 0x7) == 1) {
			fprintf(out, "-- %i: mode_high %u mode_low %u unk02 %u unk04 %u "
					"unk06 %u PWM freq %d Hz full_power %d no_pwr %d "
					"unk0e %u unk10 %u unk12 %u --\n",
					i, e->mode >> 4, e->mode & 0x7, e->unk02, e->unk04, e->unk06,
					e->pwm_freq, e->calib_full_pwr, e->calib_no_pwr,
					e->unk0e, e->unk10, e->unk12);
		}

		envy_bios_dump_hex(bios, out, fan_calib->entries[i].offset, fan_calib->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk1c(struct envy_bios *bios) {
	struct envy_bios_power_unk1c *unk1c = &bios->power.unk1c;

	if (!unk1c->offset)
		return -EINVAL;

	/* Script, no version */

	return 0;
}

void envy_bios_print_power_unk1c(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk1c *unk1c = &bios->power.unk1c;

	if (!unk1c->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;

	fprintf(out, "UNK1C script at 0x%x\n", unk1c->offset);
	printscript(unk1c->offset);
	fprintf(out, "\n");
}

int envy_bios_parse_power_boost(struct envy_bios *bios) {
	struct envy_bios_power_boost *boost = &bios->power.boost;
	int i, j, err = 0;

	if (!boost->offset)
		return -EINVAL;

	bios_u8(bios, boost->offset + 0x0, &boost->version);
	switch(boost->version) {
	case 0x10:
		err |= bios_u8(bios, boost->offset + 0x1, &boost->hlen);
		err |= bios_u8(bios, boost->offset + 0x2, &boost->rlen);
		err |= bios_u8(bios, boost->offset + 0x3, &boost->entriesnum);
		boost->ssz = 0;
		boost->snr = 0;
		boost->valid = !err;
		break;
	case 0x11:
		err |= bios_u8(bios, boost->offset + 0x1, &boost->hlen);
		err |= bios_u8(bios, boost->offset + 0x2, &boost->rlen);
		err |= bios_u8(bios, boost->offset + 0x3, &boost->ssz);
		err |= bios_u8(bios, boost->offset + 0x4, &boost->snr);
		err |= bios_u8(bios, boost->offset + 0x5, &boost->entriesnum);
		boost->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown BOOST table version 0x%x\n", boost->version);
		return -EINVAL;
	};

	boost->entries = malloc(boost->entriesnum * sizeof(struct envy_bios_power_boost_entry));

	for (i = 0; i < boost->entriesnum; i++) {
		uint32_t data = boost->offset + boost->hlen + i * (boost->rlen + (boost->snr * boost->ssz));

		uint16_t tmp;
		err |= bios_u16(bios, data + 0x0, &tmp);
		err |= bios_u16(bios, data + 0x2, &boost->entries[i].min);
		err |= bios_u16(bios, data + 0x4, &boost->entries[i].max);

		boost->entries[i].offset = data;
		boost->entries[i].pstate = (tmp & 0x01e0) >> 5;

		boost->entries[i].entries = malloc(boost->snr * sizeof(struct envy_bios_power_boost_subentry));

		for (j = 0; j < boost->snr; j++) {
			struct envy_bios_power_boost_subentry *sub = &boost->entries[i].entries[j];
			uint32_t sdata = data + boost->rlen + j * boost->ssz;

			sub->offset = sdata;
			bios_u8(bios, sdata + 0x0, &sub->domain);
			bios_u8(bios, sdata + 0x1, &sub->percent);
			bios_u16(bios, sdata + 0x2, &sub->min);
			bios_u16(bios, sdata + 0x4, &sub->max);
		}
	}

	return 0;
}

void envy_bios_print_power_boost(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_boost *boost = &bios->power.boost;
	int i, j;

	if (!boost->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!boost->valid) {
		fprintf(out, "Failed to parse BOOST table at 0x%x, version %x\n", boost->offset, boost->version);
		return;
	}

	fprintf(out, "BOOST table at 0x%x, version %x\n", boost->offset, boost->version);
	envy_bios_dump_hex(bios, out, boost->offset, boost->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < boost->entriesnum; i++) {
		fprintf(out, "	%i: pstate %x min %d MHz max %d MHz\n", i,
			boost->entries[i].pstate, boost->entries[i].min,
			boost->entries[i].max);
		envy_bios_dump_hex(bios, out, boost->entries[i].offset, boost->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");


		for (j = 0; j < boost->snr; j++) {
			struct envy_bios_power_boost_subentry *sub = &boost->entries[i].entries[j];
			fprintf(stdout, "		%i: domain %x percent %d min %d max %d\n",
				j, sub->domain, sub->percent, sub->min, sub->max);
			envy_bios_dump_hex(bios, out, sub->offset, boost->ssz, mask);
			if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
		}
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_cstep(struct envy_bios *bios) {
	struct envy_bios_power_cstep *cstep = &bios->power.cstep;
	int i, err = 0;

	if (!cstep->offset)
		return -EINVAL;

	bios_u8(bios, cstep->offset + 0x0, &cstep->version);
	switch(cstep->version) {
	case 0x10:
		err |= bios_u8(bios, cstep->offset + 0x1, &cstep->hlen);
		err |= bios_u8(bios, cstep->offset + 0x2, &cstep->rlen);
		err |= bios_u8(bios, cstep->offset + 0x3, &cstep->entriesnum);
		err |= bios_u8(bios, cstep->offset + 0x4, &cstep->ssz);
		err |= bios_u8(bios, cstep->offset + 0x5, &cstep->snr);
		cstep->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown CSTEP table version 0x%x\n", cstep->version);
		return -EINVAL;
	};

	assert(cstep->entriesnum <= (sizeof(cstep->ent1) / sizeof(struct envy_bios_power_cstep_entry1)));
	for (i = 0; i < cstep->entriesnum; i++) {
		uint32_t data = cstep->offset + cstep->hlen + i * cstep->rlen;

		uint16_t tmp;
		err |= bios_u16(bios, data + 0x0, &tmp);

		cstep->ent1[i].offset = data;
		cstep->ent1[i].pstate = (tmp & 0x01e0) >> 5;
		bios_u8(bios, data + 0x3, &cstep->ent1[i].index);
	}

	cstep->ent2 = malloc(cstep->snr * sizeof(struct envy_bios_power_cstep_entry2));
	memset(cstep->ent2, 0x0, cstep->snr * sizeof(struct envy_bios_power_cstep_entry2));
	for (i = 0; i < cstep->snr; i++) {
		uint32_t data = cstep->offset + cstep->hlen + (cstep->entriesnum * cstep->rlen) + (i * cstep->ssz);

		cstep->ent2[i].offset = data;
		bios_u16(bios, data + 0x0, &cstep->ent2[i].freq);
		bios_u8(bios, data + 0x2, &cstep->ent2[i].unkn[0]);
		bios_u8(bios, data + 0x3, &cstep->ent2[i].unkn[1]);
		bios_u8(bios, data + 0x4, &cstep->ent2[i].voltage);
		cstep->ent2[i].valid = (cstep->ent2[i].freq > 0);
	}

	return 0;
}

void envy_bios_print_power_cstep(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_cstep *cstep = &bios->power.cstep;
	int i;

	if (!cstep->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!cstep->valid) {
		fprintf(out, "Failed to parse CSTEP table at 0x%x, version %x\n", cstep->offset, cstep->version);
		return;
	}

	fprintf(out, "CSTEP table at 0x%x, version %x\n", cstep->offset, cstep->version);
	envy_bios_dump_hex(bios, out, cstep->offset, cstep->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < cstep->entriesnum; i++) {
		fprintf(out, "	%i: pstate %x index %d\n", i,
			cstep->ent1[i].pstate, cstep->ent1[i].index);
		envy_bios_dump_hex(bios, out, cstep->ent1[i].offset, cstep->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}
	fprintf(out, "---\n");
	for (i = 0; i < cstep->snr; i++) {
		if (!cstep->ent2[i].valid)
			continue;

		fprintf(out, "	%i: freq %d MHz unkn[0] %x unkn[1] %x voltage %d\n",
			i, cstep->ent2[i].freq, cstep->ent2[i].unkn[0], cstep->ent2[i].unkn[1], cstep->ent2[i].voltage);
		envy_bios_dump_hex(bios, out, cstep->ent2[i].offset, cstep->ssz, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_budget(struct envy_bios *bios) {
	struct envy_bios_power_budget *budget = &bios->power.budget;
	int i, err = 0;

	if (!budget->offset)
		return -EINVAL;

	bios_u8(bios, budget->offset + 0x0, &budget->version);

	switch(budget->version) {
	case 0x10:
	case 0x20:
	case 0x30:
		err |= bios_u8(bios, budget->offset + 0x1, &budget->hlen);
		err |= bios_u8(bios, budget->offset + 0x2, &budget->rlen);
		err |= bios_u8(bios, budget->offset + 0x3, &budget->entriesnum);
		budget->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown POWER BUDGET table version 0x%x\n", budget->version);
		return -EINVAL;
	};

	switch(budget->version) {
	case 0x20:
		err |= bios_u8(bios, budget->offset + 0x9, &budget->cap_entry);
		break;
	case 0x30:
		err |= bios_u8(bios, budget->offset + 0xa, &budget->cap_entry);
		break;
	};

	err = 0;
	budget->entries = malloc(budget->entriesnum * sizeof(struct envy_bios_power_budget_entry));
	memset(budget->entries, 0x0, budget->entriesnum * sizeof(struct envy_bios_power_budget_entry));
	for (i = 0; i < budget->entriesnum; i++) {
		uint16_t data = budget->offset + budget->hlen + i * budget->rlen;

		budget->entries[i].offset = data;

		if (budget->rlen == 0x6) {
			err |= bios_u32(bios, data + 0x02, &budget->entries[i].avg);
		} else {
			err |= bios_u32(bios, data + 0x02, &budget->entries[i].min);
			err |= bios_u32(bios, data + 0x06, &budget->entries[i].avg);
			err |= bios_u32(bios, data + 0x0a, &budget->entries[i].peak);
			err |= bios_u32(bios, data + 0x12, &budget->entries[i].unkn12);
		}

		budget->entries[i].valid = !err;
	}

	return 0;
}

void envy_bios_print_power_budget(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_budget *budget = &bios->power.budget;
	int i;

	extern uint32_t strap;
	uint8_t ram_cfg = strap?(strap & 0x1c) >> 2:0xff;


	if (!budget->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!budget->valid) {
		fprintf(out, "Failed to parse BUDGET table at 0x%x, version %x\n", budget->offset, budget->version);
		return;
	}

	fprintf(out, "POWER BUDGET table at 0x%x, version %x\n", budget->offset, budget->version);
	switch(budget->version) {
	case 0x20:
	case 0x30:
		fprintf(out, "nvidia-smi cap entry: %i\n", budget->cap_entry);
		break;
	}
	envy_bios_dump_hex(bios, out, budget->offset, budget->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < budget->entriesnum; i++) {
		fprintf(out, "%s %i: min = %u mW, avg = %u mW, peak = %u mW (unkn12 = %u)\n",
			ram_cfg == i?"*":" ", i, budget->entries[i].min,
			budget->entries[i].avg, budget->entries[i].peak,
			budget->entries[i].unkn12
       		);

		envy_bios_dump_hex(bios, out, budget->entries[i].offset, budget->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk24(struct envy_bios *bios) {
	struct envy_bios_power_unk24 *unk24 = &bios->power.unk24;
	int i, err = 0;

	if (!unk24->offset)
		return -EINVAL;

	bios_u8(bios, unk24->offset + 0x0, &unk24->version);
	switch(unk24->version) {
	case 0x10:
		err |= bios_u8(bios, unk24->offset + 0x1, &unk24->hlen);
		err |= bios_u8(bios, unk24->offset + 0x2, &unk24->rlen);
		err |= bios_u8(bios, unk24->offset + 0x3, &unk24->entriesnum);
		unk24->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK24 table version 0x%x\n", unk24->version);
		return -EINVAL;
	};

	err = 0;
	unk24->entries = malloc(unk24->entriesnum * sizeof(struct envy_bios_power_unk24_entry));
	for (i = 0; i < unk24->entriesnum; i++) {
		uint16_t data = unk24->offset + unk24->hlen + i * unk24->rlen;

		unk24->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk24(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk24 *unk24 = &bios->power.unk24;
	int i;

	if (!unk24->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk24->valid) {
		fprintf(out, "Failed to parse UNK24 table at 0x%x, version %x\n", unk24->offset, unk24->version);
		return;
	}

	fprintf(out, "UNK24 table at 0x%x, version %x\n", unk24->offset, unk24->version);
	envy_bios_dump_hex(bios, out, unk24->offset, unk24->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk24->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk24->entries[i].offset, unk24->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_sense(struct envy_bios *bios) {
	struct envy_bios_power_sense *sense = &bios->power.sense;
	int i, err = 0;

	if (!sense->offset)
		return -EINVAL;

	bios_u8(bios, sense->offset + 0x0, &sense->version);
	switch(sense->version) {
	case 0x10:
	case 0x20:
		err |= bios_u8(bios, sense->offset + 0x1, &sense->hlen);
		err |= bios_u8(bios, sense->offset + 0x2, &sense->rlen);
		err |= bios_u8(bios, sense->offset + 0x3, &sense->entriesnum);
		sense->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown SENSE table version 0x%x\n", sense->version);
		return -EINVAL;
	};

	err = 0;
	sense->entries = malloc(sense->entriesnum * sizeof(struct envy_bios_power_sense_entry));
	for (i = 0; i < sense->entriesnum; i++) {
		uint16_t data = sense->offset + sense->hlen + i * sense->rlen;

		sense->entries[i].offset = data;

		switch(sense->version) {
		case 0x10:
			err |= bios_u8(bios, data + 0x1, &sense->entries[i].mode);
			err |= bios_u8(bios, data + 0x2, &sense->entries[i].extdev_id);
			if (sense->rlen - 0x3 > 0x10)
				ENVY_BIOS_ERR("SENSE table entry bigger than expected\n");
			for (int j = 0; j < sense->rlen - 0x3 && j < 0x10; ++j)
				err |= bios_u8(bios, data + 0x3 + j, &sense->entries[i].d.raw[j]);
			break;
		case 0x20:
			err |= bios_u8(bios, data + 0x0, &sense->entries[i].mode);
			err |= bios_u8(bios, data + 0x1, &sense->entries[i].extdev_id);
			if (sense->rlen - 0x5 > 0x10)
				ENVY_BIOS_ERR("SENSE table entry bigger than expected\n");
			for (int j = 0; j < sense->rlen - 0x5 && j < 0x10; ++j)
				err |= bios_u8(bios, data + 0x5 + j, &sense->entries[i].d.raw[j]);
			break;
		};
	}

	return 0;
}

void envy_bios_print_power_sense(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_sense *sense = &bios->power.sense;
	int i;

	if (!sense->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!sense->valid) {
		fprintf(out, "Failed to parse SENSE table at 0x%x, version %x\n", sense->offset, sense->version);
		return;
	}

	fprintf(out, "SENSE table at 0x%x, version %x\n", sense->offset, sense->version);
	envy_bios_dump_hex(bios, out, sense->offset, sense->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < sense->entriesnum; i++) {
		struct envy_bios_power_sense_entry *e = &sense->entries[i];

		if (bios->extdev.entriesnum > e->extdev_id) {
			switch (bios->extdev.entries[e->extdev_id].type) {
			case ENVY_BIOS_EXTDEV_INA209:
			case ENVY_BIOS_EXTDEV_INA219:
				fprintf(out, "power rail %i: unk0 = 0x%x, extdev_id = %u, shunt resistor = %u mOhm (unk %x), config = 0x%04x\n",
					i, e->mode, e->extdev_id, e->d.ina219.res.mohm, e->d.ina219.res.unk, e->d.ina219.config);
				break;
			case ENVY_BIOS_EXTDEV_INA3221:
				fprintf(out, "power rail %i: unk0 = 0x%x, extdev_id = %u, shunt resistors = {%u mOhm (unk %x), %u mOhm (unk %x), %u mOhm (unk %x)}, config = 0x%04x\n",
					i, e->mode, e->extdev_id, e->d.ina3221.res[0].mohm, e->d.ina3221.res[0].unk, e->d.ina3221.res[1].mohm, e->d.ina3221.res[1].unk, e->d.ina3221.res[2].mohm, e->d.ina3221.res[2].unk, e->d.ina3221.config);
				break;
			default:
				fprintf(out, "power rail %i: unk0 = 0x%x, extdev_id = %u\n",
					i, e->mode, e->extdev_id);
			}
		}
		envy_bios_dump_hex(bios, out, sense->entries[i].offset, sense->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_base_clock(struct envy_bios *bios) {
	struct envy_bios_power_base_clock *bc = &bios->power.base_clock;
	int i, j, err = 0;

	if (!bc->offset)
		return -EINVAL;

	bios_u8(bios, bc->offset + 0x0, &bc->version);
	switch(bc->version) {
	case 0x10:
		err |= bios_u8(bios, bc->offset + 0x1, &bc->hlen);
		err |= bios_u8(bios, bc->offset + 0x2, &bc->rlen);
		err |= bios_u8(bios, bc->offset + 0x3, &bc->selen);
		err |= bios_u8(bios, bc->offset + 0x4, &bc->secount);
		err |= bios_u8(bios, bc->offset + 0x5, &bc->entriesnum);
		bc->valid = !err;

		if (bc->valid) {
			bios_u8(bios, bc->offset + 0x06, &bc->d2_entry);
			bios_u8(bios, bc->offset + 0x07, &bc->d3_entry);
			bios_u8(bios, bc->offset + 0x08, &bc->d4_entry);
			bios_u8(bios, bc->offset + 0x09, &bc->d5_entry);
			bios_u8(bios, bc->offset + 0x0a, &bc->over_current_entry);
			bios_u8(bios, bc->offset + 0x0b, &bc->vrhot_entry);
			bios_u8(bios, bc->offset + 0x0c, &bc->max_batt_entry);
			bios_u8(bios, bc->offset + 0x0d, &bc->max_sli_entry);
			bios_u8(bios, bc->offset + 0x0e, &bc->max_therm_sustain_entry);
			bios_u8(bios, bc->offset + 0x0f, &bc->boost_entry);
			if (bc->hlen > 0x10)
				bios_u8(bios, bc->offset + 0x10, &bc->turbo_boost_entry);
			else
				bc->turbo_boost_entry = 0xff;
			if (bc->hlen > 0x11)
				bios_u8(bios, bc->offset + 0x11, &bc->rated_tdp_entry);
			else
				bc->rated_tdp_entry = 0xff;
			if (bc->hlen > 0x12)
				bios_u8(bios, bc->offset + 0x12, &bc->slowdown_pwr_entry);
			else
				bc->slowdown_pwr_entry = 0xff;
			if (bc->hlen > 0x13)
				bios_u8(bios, bc->offset + 0x13, &bc->mid_point_entry);
			else
				bc->mid_point_entry = 0xff;
			if (bc->hlen > 0x15)
				bios_u8(bios, bc->offset + 0x15, &bc->unk15_entry);
			else
				bc->unk15_entry = 0xff;
			if (bc->hlen > 0x16)
				bios_u8(bios, bc->offset + 0x16, &bc->unk16_entry);
			else
				bc->unk16_entry = 0xff;
		} else {
			return -EINVAL;
		}

		break;
	default:
		ENVY_BIOS_ERR("BASE CLOCKS table version 0x%x\n", bc->version);
		return -EINVAL;
	};

	err = 0;
	bc->entries = malloc(bc->entriesnum * sizeof(struct envy_bios_power_base_clock_entry));
	for (i = 0; i < bc->entriesnum; i++) {
		struct envy_bios_power_base_clock_entry *bce = &bc->entries[i];

		bce->offset = bc->offset + bc->hlen + i * (bc->rlen + (bc->selen * bc->secount));
		bios_u8(bios, bce->offset, &bce->pstate);
		bios_u16(bios, bce->offset + 0x1, &bce->reqPower);
		bios_u16(bios, bce->offset + 0x3, &bce->reqSlowdownPower);
		bce->clock = malloc(bc->secount * sizeof(uint16_t));
		for (j = 0; j < bc->secount; j++)
			bios_u16(bios, bce->offset + 0x5 + j, &bce->clock[j]);
	}

	return 0;
}

void envy_bios_print_power_base_clock(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_base_clock *bc = &bios->power.base_clock;
	int i, j;

	if (!bc->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!bc->valid) {
		fprintf(out, "Failed to parse BASE CLOCK table at 0x%x, version %x\n", bc->offset, bc->version);
		return;
	}

	fprintf(out, "BASE CLOCK table at 0x%x, version %x\n", bc->offset, bc->version);
	if (bc->entriesnum) {
		if (bc->d2_entry != 0xff)
			fprintf(out, "d2 entry: %i\n", bc->d2_entry);
		if (bc->d3_entry != 0xff)
			fprintf(out, "d3 entry: %i\n", bc->d3_entry);
		if (bc->d4_entry != 0xff)
			fprintf(out, "d4 entry: %i\n", bc->d4_entry);
		if (bc->d5_entry != 0xff)
			fprintf(out, "d5 entry: %i\n", bc->d5_entry);
		if (bc->over_current_entry != 0xff)
			fprintf(out, "over current entry: %i\n", bc->over_current_entry);
		if (bc->vrhot_entry != 0xff)
			fprintf(out, "vrhot entry: %i\n", bc->vrhot_entry);
		if (bc->max_batt_entry != 0xff)
			fprintf(out, "max batt entry: %i\n", bc->max_batt_entry);
		if (bc->max_sli_entry != 0xff)
			fprintf(out, "max sli entry: %i\n", bc->max_sli_entry);
		if (bc->max_therm_sustain_entry != 0xff)
			fprintf(out, "max therm sustain entry: %i\n", bc->max_therm_sustain_entry);
		if (bc->boost_entry != 0xff)
			fprintf(out, "boost entry: %i\n", bc->boost_entry);
		if (bc->turbo_boost_entry != 0xff)
			fprintf(out, "turbo boost entry: %i\n", bc->turbo_boost_entry);
		if (bc->rated_tdp_entry != 0xff)
			fprintf(out, "rated tdp entry: %i\n", bc->rated_tdp_entry);
		if (bc->slowdown_pwr_entry != 0xff)
			fprintf(out, "slowdown pwr entry: %i\n", bc->slowdown_pwr_entry);
		if (bc->mid_point_entry != 0xff)
			fprintf(out, "mid point entry: %i\n", bc->mid_point_entry);
		if (bc->unk15_entry != 0xff)
			fprintf(out, "unk15 entry: %i\n", bc->unk15_entry);
		if (bc->unk16_entry != 0xff)
			fprintf(out, "unk16 entry: %i\n", bc->unk16_entry);
	}

	envy_bios_dump_hex(bios, out, bc->offset, bc->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < bc->entriesnum; i++) {
		struct envy_bios_power_base_clock_entry *bce = &bc->entries[i];

		if (bce->pstate == 0x0 || bce->pstate == 0xff)
			continue;

		fprintf(out, "-- entry %i, pstate = %x, reqPower = %i mW, reqSlowdownPower = %i mW",
		        i, bce->pstate, bce->reqPower * 10, bce->reqSlowdownPower * 10);
		for (j = 0; j < bc->secount; j++)
			fprintf(out, ", clock%i = %i MHz", j, bce->clock[j]);
		fprintf(out, "\n");
		envy_bios_dump_hex(bios, out, bce->offset, bc->rlen + (bc->selen * bc->secount), mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk3c(struct envy_bios *bios) {
	struct envy_bios_power_unk3c *unk3c = &bios->power.unk3c;
	int i, err = 0;

	if (!unk3c->offset)
		return -EINVAL;

	bios_u8(bios, unk3c->offset + 0x0, &unk3c->version);
	switch(unk3c->version) {
	case 0x10:
	case 0x20:
		err |= bios_u8(bios, unk3c->offset + 0x1, &unk3c->hlen);
		err |= bios_u8(bios, unk3c->offset + 0x2, &unk3c->rlen);
		err |= bios_u8(bios, unk3c->offset + 0x3, &unk3c->entriesnum);
		unk3c->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK3C table version 0x%x\n", unk3c->version);
		return -EINVAL;
	};

	err = 0;
	unk3c->entries = malloc(unk3c->entriesnum * sizeof(struct envy_bios_power_unk3c_entry));
	for (i = 0; i < unk3c->entriesnum; i++) {
		uint16_t data = unk3c->offset + unk3c->hlen + i * unk3c->rlen;

		unk3c->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk3c(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk3c *unk3c = &bios->power.unk3c;
	int i;

	if (!unk3c->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk3c->valid) {
		fprintf(out, "Failed to parse UNK3C table at 0x%x, version %x\n", unk3c->offset, unk3c->version);
		return;
	}

	fprintf(out, "UNK3C table at 0x%x, version %x\n", unk3c->offset, unk3c->version);
	envy_bios_dump_hex(bios, out, unk3c->offset, unk3c->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk3c->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk3c->entries[i].offset, unk3c->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk40(struct envy_bios *bios) {
	struct envy_bios_power_unk40 *unk40 = &bios->power.unk40;
	int i, err = 0;

	if (!unk40->offset)
		return -EINVAL;

	bios_u8(bios, unk40->offset + 0x0, &unk40->version);
	switch(unk40->version) {
	case 0x10:
		err |= bios_u8(bios, unk40->offset + 0x1, &unk40->hlen);
		err |= bios_u8(bios, unk40->offset + 0x2, &unk40->rlen);
		err |= bios_u8(bios, unk40->offset + 0x3, &unk40->entriesnum);
		unk40->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK40 table version 0x%x\n", unk40->version);
		return -EINVAL;
	};

	err = 0;
	unk40->entries = malloc(unk40->entriesnum * sizeof(struct envy_bios_power_unk40_entry));
	for (i = 0; i < unk40->entriesnum; i++) {
		uint16_t data = unk40->offset + unk40->hlen + i * unk40->rlen;

		unk40->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk40(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk40 *unk40 = &bios->power.unk40;
	int i;

	if (!unk40->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk40->valid) {
		fprintf(out, "Failed to parse UNK40 table at 0x%x, version %x\n", unk40->offset, unk40->version);
		return;
	}

	fprintf(out, "UNK40 table at 0x%x, version %x\n", unk40->offset, unk40->version);
	envy_bios_dump_hex(bios, out, unk40->offset, unk40->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk40->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk40->entries[i].offset, unk40->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk44(struct envy_bios *bios) {
	struct envy_bios_power_unk44 *unk44 = &bios->power.unk44;
	int err = 0;

	if (!unk44->offset)
		return -EINVAL;

	bios_u8(bios, unk44->offset + 0x0, &unk44->version);
	switch(unk44->version) {
	case 0x10:
		err |= bios_u8(bios, unk44->offset + 0x1, &unk44->hlen);

		/* it doesn't appear as if this table has entries at all */
		unk44->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK44 table version 0x%x\n", unk44->version);
		return -EINVAL;
	};

	return 0;
}

void envy_bios_print_power_unk44(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk44 *unk44 = &bios->power.unk44;

	if (!unk44->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk44->valid) {
		fprintf(out, "Failed to parse UNK44 table at 0x%x, version %x\n", unk44->offset, unk44->version);
		return;
	}

	fprintf(out, "UNK44 table at 0x%x, version %x\n", unk44->offset, unk44->version);
	envy_bios_dump_hex(bios, out, unk44->offset, unk44->hlen, mask);
	fprintf(out, "\n");
}

int envy_bios_parse_power_unk48(struct envy_bios *bios) {
	struct envy_bios_power_unk48 *unk48 = &bios->power.unk48;
	int i, err = 0;

	if (!unk48->offset)
		return -EINVAL;

	bios_u8(bios, unk48->offset + 0x0, &unk48->version);
	switch(unk48->version) {
	case 0x10:
		err |= bios_u8(bios, unk48->offset + 0x1, &unk48->hlen);
		err |= bios_u8(bios, unk48->offset + 0x2, &unk48->rlen);
		err |= bios_u8(bios, unk48->offset + 0x3, &unk48->entriesnum);
		unk48->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK48 table version 0x%x\n", unk48->version);
		return -EINVAL;
	};

	err = 0;
	unk48->entries = malloc(unk48->entriesnum * sizeof(struct envy_bios_power_unk48_entry));
	for (i = 0; i < unk48->entriesnum; i++) {
		uint16_t data = unk48->offset + unk48->hlen + i * unk48->rlen;

		unk48->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk48(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk48 *unk48 = &bios->power.unk48;
	int i;

	if (!unk48->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk48->valid) {
		fprintf(out, "Failed to parse UNK48 table at 0x%x, version %x\n", unk48->offset, unk48->version);
		return;
	}

	fprintf(out, "UNK48 table at 0x%x, version %x\n", unk48->offset, unk48->version);
	envy_bios_dump_hex(bios, out, unk48->offset, unk48->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk48->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk48->entries[i].offset, unk48->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk4c(struct envy_bios *bios) {
	struct envy_bios_power_unk4c *unk4c = &bios->power.unk4c;
	int i, err = 0;

	if (!unk4c->offset)
		return -EINVAL;

	bios_u8(bios, unk4c->offset + 0x0, &unk4c->version);
	switch(unk4c->version) {
	case 0x10:
		err |= bios_u8(bios, unk4c->offset + 0x1, &unk4c->hlen);
		err |= bios_u8(bios, unk4c->offset + 0x2, &unk4c->rlen);
		err |= bios_u8(bios, unk4c->offset + 0x3, &unk4c->entriesnum);
		unk4c->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK4c table version 0x%x\n", unk4c->version);
		return -EINVAL;
	};

	err = 0;
	unk4c->entries = malloc(unk4c->entriesnum * sizeof(struct envy_bios_power_unk4c_entry));
	for (i = 0; i < unk4c->entriesnum; i++) {
		uint16_t data = unk4c->offset + unk4c->hlen + i * unk4c->rlen;

		unk4c->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk4c(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk4c *unk4c = &bios->power.unk4c;
	int i;

	if (!unk4c->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk4c->valid) {
		fprintf(out, "Failed to parse UNK4C table at 0x%x, version %x\n", unk4c->offset, unk4c->version);
		return;
	}

	fprintf(out, "UNK4c table at 0x%x, version %x\n", unk4c->offset, unk4c->version);
	envy_bios_dump_hex(bios, out, unk4c->offset, unk4c->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk4c->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk4c->entries[i].offset, unk4c->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk50(struct envy_bios *bios) {
	struct envy_bios_power_unk50 *unk50 = &bios->power.unk50;
	int i, err = 0;

	if (!unk50->offset)
		return -EINVAL;

	bios_u8(bios, unk50->offset + 0x0, &unk50->version);
	switch(unk50->version) {
	case 0x10:
		err |= bios_u8(bios, unk50->offset + 0x1, &unk50->hlen);
		err |= bios_u8(bios, unk50->offset + 0x2, &unk50->rlen);
		err |= bios_u8(bios, unk50->offset + 0x3, &unk50->entriesnum);
		unk50->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK50 table version 0x%x\n", unk50->version);
		return -EINVAL;
	};

	err = 0;
	unk50->entries = malloc(unk50->entriesnum * sizeof(struct envy_bios_power_unk50_entry));
	for (i = 0; i < unk50->entriesnum; i++) {
		struct envy_bios_power_unk50_entry *e = &unk50->entries[i];

		e->offset = unk50->offset + unk50->hlen + i * unk50->rlen;
		bios_u8 (bios, e->offset + 0x0, &e->mode);
		bios_u16(bios, e->offset + 0x2, &e->downclock_t);
		bios_u16(bios, e->offset + 0x4, &e->t1);
		bios_u16(bios, e->offset + 0x6, &e->t2);
		bios_u16(bios, e->offset + 0x8, &e->interval_us);
	}

	return 0;
}

void envy_bios_print_power_unk50(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk50 *unk50 = &bios->power.unk50;
	int i;

	if (!unk50->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk50->valid) {
		fprintf(out, "Failed to parse UNK50 table at 0x%x, version %x\n", unk50->offset, unk50->version);
		return;
	}

	fprintf(out, "UNK50 table at 0x%x, version %x\n", unk50->offset, unk50->version);
	envy_bios_dump_hex(bios, out, unk50->offset, unk50->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk50->entriesnum; i++) {
		struct envy_bios_power_unk50_entry *e = &unk50->entries[i];

		if (e->downclock_t == 0)
			continue;

		fprintf(out, "-- entry %i, mode %i, donwclock_t %.2f, t1 %.2f, t2 %.2f, interval %i µs\n",
			i, e->mode, (double)e->downclock_t / 32, (double)e->t1 / 32, (double)e->t2 / 32, e->interval_us);

		envy_bios_dump_hex(bios, out, unk50->entries[i].offset, unk50->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk54(struct envy_bios *bios) {
	struct envy_bios_power_unk54 *unk54 = &bios->power.unk54;
	int i, err = 0;

	if (!unk54->offset)
		return -EINVAL;

	bios_u8(bios, unk54->offset + 0x0, &unk54->version);
	switch(unk54->version) {
	case 0x10:
		err |= bios_u8(bios, unk54->offset + 0x1, &unk54->hlen);
		err |= bios_u8(bios, unk54->offset + 0x2, &unk54->rlen);
		err |= bios_u8(bios, unk54->offset + 0x3, &unk54->entriesnum);
		unk54->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK54 table version 0x%x\n", unk54->version);
		return -EINVAL;
	};

	err = 0;
	unk54->entries = malloc(unk54->entriesnum * sizeof(struct envy_bios_power_unk54_entry));
	for (i = 0; i < unk54->entriesnum; i++) {
		uint32_t data = unk54->offset + unk54->hlen + i * unk54->rlen;

		unk54->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk54(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk54 *unk54 = &bios->power.unk54;
	int i;

	if (!unk54->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk54->valid) {
		fprintf(out, "Failed to parse UNK54 table at 0x%x, version %x\n", unk54->offset, unk54->version);
		return;
	}

	fprintf(out, "UNK54 table at 0x%x, version %x\n", unk54->offset, unk54->version);
	envy_bios_dump_hex(bios, out, unk54->offset, unk54->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk54->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk54->entries[i].offset, unk54->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_fan(struct envy_bios *bios) {
	struct envy_bios_power_fan *fan = &bios->power.fan;
	uint16_t data;
	int err = 0;

	if (!fan->offset)
		return -EINVAL;

	bios_u8(bios, fan->offset + 0x0, &fan->version);
	switch(fan->version) {
	case 0x10:
		err |= bios_u8(bios, fan->offset + 0x1, &fan->hlen);
		err |= bios_u8(bios, fan->offset + 0x2, &fan->rlen);
		err |= bios_u8(bios, fan->offset + 0x3, &fan->entriesnum);

		fan->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown FAN table version 0x%x\n", fan->version);
		return -EINVAL;
	};

	/* go to the first entry */
	data = fan->offset + fan->hlen;

	bios_u8(bios, data + 0x00, &fan->type);
	bios_u8(bios, data + 0x02, &fan->duty_min);
	bios_u8(bios, data + 0x03, &fan->duty_max);
	/* 0x10 == constant to 9? */
	bios_u32(bios, data + 0x0b, &fan->divisor); fan->divisor &= 0xffffff;
	bios_u16(bios, data + 0x0e, &fan->unk0e); /* looks like the fan bump delay */
	bios_u16(bios, data + 0x10, &fan->unk10); /* looks like the fan slow down delay */
	bios_u16(bios, data + 0x14, &fan->unboost_unboost_ms);
	bios_u8(bios, data + 0x17, &fan->duty_boosted); /* threshold = 96 °C */

	/* temp fan bump min = 45°C */
	/* temp fan max = 95°C */

	return 0;
}

void envy_bios_print_power_fan(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_fan *fan = &bios->power.fan;
	const char *fan_type_s = "UNKNOWN";

	if (!fan->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!fan->valid) {
		fprintf(out, "Failed to parse FAN table at 0x%x, version %x\n", fan->offset, fan->version);
		return;
	}

	fprintf(out, "FAN table at 0x%x, version %x\n", fan->offset, fan->version);
	envy_bios_dump_hex(bios, out, fan->offset, fan->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	if (fan->type == 0)
		fan_type_s = "TOGGLE";
	else if (fan->type == 1)
		fan_type_s = "PWM";

	fprintf(out, "-- type: %s, duty_range: [%u:%u]%%, fan_div: %u --\n",
		fan_type_s, fan->duty_min, fan->duty_max, fan->divisor);
	fprintf(out, "-- unk0e: %u, unk10: %u, unboost delay: %u ms, boosted_duty: %u%% --\n",
		fan->unk0e, fan->unk10, fan->unboost_unboost_ms, fan->duty_boosted);

	/* fan boost threshold is set to 96°C but seems to be hardcoded */

	envy_bios_dump_hex(bios, out, fan->offset + fan->hlen, fan->rlen, mask);
	fprintf(out, "\n");
}

int envy_bios_parse_power_fan_mgmt(struct envy_bios *bios) {
	struct envy_bios_power_fan_mgmt *fan_mgmt = &bios->power.fan_mgmt;
	int i, err = 0;

	if (!fan_mgmt->offset)
		return -EINVAL;

	bios_u8(bios, fan_mgmt->offset + 0x0, &fan_mgmt->version);
	switch(fan_mgmt->version) {
	case 0x10:
		err |= bios_u8(bios, fan_mgmt->offset + 0x1, &fan_mgmt->hlen);
		err |= bios_u8(bios, fan_mgmt->offset + 0x2, &fan_mgmt->rlen);
		err |= bios_u8(bios, fan_mgmt->offset + 0x3, &fan_mgmt->entriesnum);
		fan_mgmt->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown FAN_MGMT table version 0x%x\n", fan_mgmt->version);
		return -EINVAL;
	};

	err = 0;
	fan_mgmt->entries = malloc(fan_mgmt->entriesnum * sizeof(struct envy_bios_power_fan_mgmt_entry));
	for (i = 0; i < fan_mgmt->entriesnum; i++) {
		struct envy_bios_power_fan_mgmt_entry *e = &fan_mgmt->entries[i];

		e->offset = fan_mgmt->offset + fan_mgmt->hlen + i * fan_mgmt->rlen;
		bios_u8(bios, e->offset + 0x11, &e->duty0);
		bios_u16(bios, e->offset + 0x15, &e->temp0);
		bios_u16(bios, e->offset + 0x17, &e->speed0);

		bios_u8(bios, e->offset + 0x12, &e->duty1);
		bios_u16(bios, e->offset + 0x19, &e->temp1);
		bios_u16(bios, e->offset + 0x1b, &e->speed1);

		bios_u8(bios, e->offset + 0x13, &e->duty2);
		bios_u16(bios, e->offset + 0x1d, &e->temp2);
		bios_u16(bios, e->offset + 0x1f, &e->speed2);
	}

	return 0;
}

void envy_bios_print_power_fan_mgmt(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_fan_mgmt *fan_mgmt = &bios->power.fan_mgmt;
	int i;

	if (!fan_mgmt->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!fan_mgmt->valid) {
		fprintf(out, "Failed to parse FAN_MGMT table at 0x%x, version %x\n", fan_mgmt->offset, fan_mgmt->version);
		return;
	}

	fprintf(out, "FAN_MGMT table at 0x%x, version %x\n", fan_mgmt->offset, fan_mgmt->version);
	envy_bios_dump_hex(bios, out, fan_mgmt->offset, fan_mgmt->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < fan_mgmt->entriesnum; i++) {
		struct envy_bios_power_fan_mgmt_entry *e = &fan_mgmt->entries[i];

		fprintf(out, "-- entry %i\n", i);
		fprintf(out, "\t 0: { %.2f °C: duty %i => %i rpm }\n", (double)e->temp0 / 32, e->duty0, e->speed0);
		fprintf(out, "\t 1: { %.2f °C: duty %i => %i rpm }\n", (double)e->temp1 / 32, e->duty1, e->speed1);
		fprintf(out, "\t 2: { %.2f °C: duty %i => %i rpm }\n", (double)e->temp2 / 32, e->duty2, e->speed2);

		envy_bios_dump_hex(bios, out, fan_mgmt->entries[i].offset, fan_mgmt->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk60(struct envy_bios *bios) {
	struct envy_bios_power_unk60 *unk60 = &bios->power.unk60;
	int i, err = 0;

	if (!unk60->offset)
		return -EINVAL;

	bios_u8(bios, unk60->offset + 0x0, &unk60->version);
	switch(unk60->version) {
	case 0x10:
		err |= bios_u8(bios, unk60->offset + 0x1, &unk60->hlen);
		err |= bios_u8(bios, unk60->offset + 0x2, &unk60->rlen);
		err |= bios_u8(bios, unk60->offset + 0x3, &unk60->entriesnum);
		unk60->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK60 table version 0x%x\n", unk60->version);
		return -EINVAL;
	};

	err = 0;
	unk60->entries = malloc(unk60->entriesnum * sizeof(struct envy_bios_power_unk60_entry));
	for (i = 0; i < unk60->entriesnum; i++) {
		uint16_t data = unk60->offset + unk60->hlen + i * unk60->rlen;

		unk60->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk60(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk60 *unk60 = &bios->power.unk60;
	int i;

	if (!unk60->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk60->valid) {
		fprintf(out, "Failed to parse UNK60 table at 0x%x, version %x\n", unk60->offset, unk60->version);
		return;
	}

	fprintf(out, "UNK60 table at 0x%x, version %x\n", unk60->offset, unk60->version);
	envy_bios_dump_hex(bios, out, unk60->offset, unk60->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk60->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk60->entries[i].offset, unk60->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk64(struct envy_bios *bios) {
	struct envy_bios_power_unk64 *unk64 = &bios->power.unk64;
	int i, err = 0;

	if (!unk64->offset)
		return -EINVAL;

	bios_u8(bios, unk64->offset + 0x0, &unk64->version);
	switch(unk64->version) {
	case 0x10:
		err |= bios_u8(bios, unk64->offset + 0x1, &unk64->hlen);
		err |= bios_u8(bios, unk64->offset + 0x2, &unk64->rlen);
		err |= bios_u8(bios, unk64->offset + 0x3, &unk64->entriesnum);
		unk64->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK64 table version 0x%x\n", unk64->version);
		return -EINVAL;
	};

	err = 0;
	unk64->entries = malloc(unk64->entriesnum * sizeof(struct envy_bios_power_unk64_entry));
	for (i = 0; i < unk64->entriesnum; i++) {
		uint16_t data = unk64->offset + unk64->hlen + i * unk64->rlen;

		unk64->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk64(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk64 *unk64 = &bios->power.unk64;
	int i;

	if (!unk64->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk64->valid) {
		fprintf(out, "Failed to parse UNK64 table at 0x%x, version %x\n", unk64->offset, unk64->version);
		return;
	}

	fprintf(out, "UNK64 table at 0x%x, version %x\n", unk64->offset, unk64->version);
	envy_bios_dump_hex(bios, out, unk64->offset, unk64->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk64->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk64->entries[i].offset, unk64->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk68(struct envy_bios *bios) {
	struct envy_bios_power_unk68 *unk68 = &bios->power.unk68;
	int i, err = 0;

	if (!unk68->offset)
		return -EINVAL;

	bios_u8(bios, unk68->offset + 0x0, &unk68->version);
	switch(unk68->version) {
	case 0x10:
		err |= bios_u8(bios, unk68->offset + 0x1, &unk68->hlen);
		err |= bios_u8(bios, unk68->offset + 0x2, &unk68->rlen);
		err |= bios_u8(bios, unk68->offset + 0x3, &unk68->entriesnum);
		unk68->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK68 table version 0x%x\n", unk68->version);
		return -EINVAL;
	};

	err = 0;
	unk68->entries = malloc(unk68->entriesnum * sizeof(struct envy_bios_power_unk68_entry));
	for (i = 0; i < unk68->entriesnum; i++) {
		uint16_t data = unk68->offset + unk68->hlen + i * unk68->rlen;

		unk68->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk68(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk68 *unk68 = &bios->power.unk68;
	int i;

	if (!unk68->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk68->valid) {
		fprintf(out, "Failed to parse UNK68 table at 0x%x, version %x\n", unk68->offset, unk68->version);
		return;
	}

	fprintf(out, "UNK68 table at 0x%x, version %x\n", unk68->offset, unk68->version);
	envy_bios_dump_hex(bios, out, unk68->offset, unk68->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk68->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk68->entries[i].offset, unk68->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk6c(struct envy_bios *bios) {
	struct envy_bios_power_unk6c *unk6c = &bios->power.unk6c;
	int i, err = 0;

	if (!unk6c->offset)
		return -EINVAL;

	bios_u8(bios, unk6c->offset + 0x0, &unk6c->version);
	switch(unk6c->version) {
	case 0x10:
		err |= bios_u8(bios, unk6c->offset + 0x1, &unk6c->hlen);
		err |= bios_u8(bios, unk6c->offset + 0x2, &unk6c->rlen);
		err |= bios_u8(bios, unk6c->offset + 0x3, &unk6c->entriesnum);
		unk6c->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK6C table version 0x%x\n", unk6c->version);
		return -EINVAL;
	};

	err = 0;
	unk6c->entries = malloc(unk6c->entriesnum * sizeof(struct envy_bios_power_unk6c_entry));
	for (i = 0; i < unk6c->entriesnum; i++) {
		uint16_t data = unk6c->offset + unk6c->hlen + i * unk6c->rlen;

		unk6c->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk6c(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk6c *unk6c = &bios->power.unk6c;
	int i;

	if (!unk6c->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk6c->valid) {
		fprintf(out, "Failed to parse UNK6C table at 0x%x, version %x\n", unk6c->offset, unk6c->version);
		return;
	}

	fprintf(out, "UNK6C table at 0x%x, version %x\n", unk6c->offset, unk6c->version);
	envy_bios_dump_hex(bios, out, unk6c->offset, unk6c->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk6c->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk6c->entries[i].offset, unk6c->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk70(struct envy_bios *bios) {
	struct envy_bios_power_unk70 *unk70 = &bios->power.unk70;
	int i, err = 0;

	if (!unk70->offset)
		return -EINVAL;

	bios_u8(bios, unk70->offset + 0x0, &unk70->version);
	switch(unk70->version) {
	case 0x10:
		err |= bios_u8(bios, unk70->offset + 0x1, &unk70->hlen);
		err |= bios_u8(bios, unk70->offset + 0x2, &unk70->rlen);
		err |= bios_u8(bios, unk70->offset + 0x3, &unk70->entriesnum);
		unk70->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK70 table version 0x%x\n", unk70->version);
		return -EINVAL;
	};

	err = 0;
	unk70->entries = malloc(unk70->entriesnum * sizeof(struct envy_bios_power_unk70_entry));
	for (i = 0; i < unk70->entriesnum; i++) {
		uint16_t data = unk70->offset + unk70->hlen + i * unk70->rlen;

		unk70->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk70(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk70 *unk70 = &bios->power.unk70;
	int i;

	if (!unk70->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk70->valid) {
		fprintf(out, "Failed to parse UNK70 table at 0x%x, version %x\n", unk70->offset, unk70->version);
		return;
	}

	fprintf(out, "UNK70 table at 0x%x, version %x\n", unk70->offset, unk70->version);
	envy_bios_dump_hex(bios, out, unk70->offset, unk70->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk70->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk70->entries[i].offset, unk70->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk74(struct envy_bios *bios) {
	struct envy_bios_power_unk74 *unk74 = &bios->power.unk74;
	int i, err = 0;

	if (!unk74->offset)
		return -EINVAL;

	bios_u8(bios, unk74->offset + 0x0, &unk74->version);
	switch(unk74->version) {
	case 0x10:
		err |= bios_u8(bios, unk74->offset + 0x1, &unk74->hlen);
		err |= bios_u8(bios, unk74->offset + 0x2, &unk74->rlen);
		err |= bios_u8(bios, unk74->offset + 0x3, &unk74->entriesnum);
		unk74->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK74 table version 0x%x\n", unk74->version);
		return -EINVAL;
	};

	err = 0;
	unk74->entries = malloc(unk74->entriesnum * sizeof(struct envy_bios_power_unk74_entry));
	for (i = 0; i < unk74->entriesnum; i++) {
		uint16_t data = unk74->offset + unk74->hlen + i * unk74->rlen;

		unk74->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk74(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk74 *unk74 = &bios->power.unk74;
	int i;

	if (!unk74->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk74->valid) {
		fprintf(out, "Failed to parse UNK74 table at 0x%x, version %x\n", unk74->offset, unk74->version);
		return;
	}

	fprintf(out, "UNK74 table at 0x%x, version %x\n", unk74->offset, unk74->version);
	envy_bios_dump_hex(bios, out, unk74->offset, unk74->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk74->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk74->entries[i].offset, unk74->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk78(struct envy_bios *bios) {
	struct envy_bios_power_unk78 *unk78 = &bios->power.unk78;
	int i, err = 0;

	if (!unk78->offset)
		return -EINVAL;

	bios_u8(bios, unk78->offset + 0x0, &unk78->version);
	switch(unk78->version) {
	case 0x10:
		err |= bios_u8(bios, unk78->offset + 0x1, &unk78->hlen);
		err |= bios_u8(bios, unk78->offset + 0x2, &unk78->rlen);
		err |= bios_u8(bios, unk78->offset + 0x3, &unk78->entriesnum);
		unk78->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK78 table version 0x%x\n", unk78->version);
		return -EINVAL;
	};

	err = 0;
	unk78->entries = malloc(unk78->entriesnum * sizeof(struct envy_bios_power_unk78_entry));
	for (i = 0; i < unk78->entriesnum; i++) {
		uint16_t data = unk78->offset + unk78->hlen + i * unk78->rlen;

		unk78->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk78(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk78 *unk78 = &bios->power.unk78;
	int i;

	if (!unk78->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk78->valid) {
		fprintf(out, "Failed to parse UNK78 table at 0x%x, version %x\n", unk78->offset, unk78->version);
		return;
	}

	fprintf(out, "UNK78 table at 0x%x, version %x\n", unk78->offset, unk78->version);
	envy_bios_dump_hex(bios, out, unk78->offset, unk78->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk78->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk78->entries[i].offset, unk78->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk7c(struct envy_bios *bios) {
	struct envy_bios_power_unk7c *unk7c = &bios->power.unk7c;
	int i, err = 0;

	if (!unk7c->offset)
		return -EINVAL;

	bios_u8(bios, unk7c->offset + 0x0, &unk7c->version);
	switch(unk7c->version) {
	case 0x10:
		err |= bios_u8(bios, unk7c->offset + 0x1, &unk7c->hlen);
		err |= bios_u8(bios, unk7c->offset + 0x2, &unk7c->rlen);
		err |= bios_u8(bios, unk7c->offset + 0x3, &unk7c->entriesnum);
		unk7c->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK7C table version 0x%x\n", unk7c->version);
		return -EINVAL;
	};

	err = 0;
	unk7c->entries = malloc(unk7c->entriesnum * sizeof(struct envy_bios_power_unk7c_entry));
	for (i = 0; i < unk7c->entriesnum; i++) {
		uint16_t data = unk7c->offset + unk7c->hlen + i * unk7c->rlen;

		unk7c->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk7c(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk7c *unk7c = &bios->power.unk7c;
	int i;

	if (!unk7c->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk7c->valid) {
		fprintf(out, "Failed to parse UNK7C table at 0x%x, version %x\n", unk7c->offset, unk7c->version);
		return;
	}

	fprintf(out, "UNK7C table at 0x%x, version %x\n", unk7c->offset, unk7c->version);
	envy_bios_dump_hex(bios, out, unk7c->offset, unk7c->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk7c->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk7c->entries[i].offset, unk7c->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk80(struct envy_bios *bios) {
	struct envy_bios_power_unk80 *unk80 = &bios->power.unk80;
	int i, err = 0;

	if (!unk80->offset)
		return -EINVAL;

	bios_u8(bios, unk80->offset + 0x0, &unk80->version);
	switch(unk80->version) {
	case 0x10:
		err |= bios_u8(bios, unk80->offset + 0x1, &unk80->hlen);
		err |= bios_u8(bios, unk80->offset + 0x2, &unk80->rlen);
		err |= bios_u8(bios, unk80->offset + 0x3, &unk80->entriesnum);
		unk80->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK80 table version 0x%x\n", unk80->version);
		return -EINVAL;
	};

	err = 0;
	unk80->entries = malloc(unk80->entriesnum * sizeof(struct envy_bios_power_unk80_entry));
	for (i = 0; i < unk80->entriesnum; i++) {
		uint16_t data = unk80->offset + unk80->hlen + i * unk80->rlen;

		unk80->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk80(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk80 *unk80 = &bios->power.unk80;
	int i;

	if (!unk80->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk80->valid) {
		fprintf(out, "Failed to parse UNK80 table at 0x%x, version %x\n", unk80->offset, unk80->version);
		return;
	}

	fprintf(out, "UNK80 table at 0x%x, version %x\n", unk80->offset, unk80->version);
	envy_bios_dump_hex(bios, out, unk80->offset, unk80->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk80->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk80->entries[i].offset, unk80->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk84(struct envy_bios *bios) {
	struct envy_bios_power_unk84 *unk84 = &bios->power.unk84;
	int i, err = 0;

	if (!unk84->offset)
		return -EINVAL;

	bios_u8(bios, unk84->offset + 0x0, &unk84->version);
	switch(unk84->version) {
	case 0x10:
		err |= bios_u8(bios, unk84->offset + 0x1, &unk84->hlen);
		err |= bios_u8(bios, unk84->offset + 0x2, &unk84->rlen);
		err |= bios_u8(bios, unk84->offset + 0x3, &unk84->entriesnum);
		unk84->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK84 table version 0x%x\n", unk84->version);
		return -EINVAL;
	};

	err = 0;
	unk84->entries = malloc(unk84->entriesnum * sizeof(struct envy_bios_power_unk84_entry));
	for (i = 0; i < unk84->entriesnum; i++) {
		uint16_t data = unk84->offset + unk84->hlen + i * unk84->rlen;

		unk84->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk84(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk84 *unk84 = &bios->power.unk84;
	int i;

	if (!unk84->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk84->valid) {
		fprintf(out, "Failed to parse UNK84 table at 0x%x, version %x\n", unk84->offset, unk84->version);
		return;
	}

	fprintf(out, "UNK84 table at 0x%x, version %x\n", unk84->offset, unk84->version);
	envy_bios_dump_hex(bios, out, unk84->offset, unk84->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk84->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk84->entries[i].offset, unk84->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk88(struct envy_bios *bios) {
	struct envy_bios_power_unk88 *unk88 = &bios->power.unk88;
	int i, err = 0;

	if (!unk88->offset)
		return -EINVAL;

	bios_u8(bios, unk88->offset + 0x0, &unk88->version);
	switch(unk88->version) {
	case 0x10:
		err |= bios_u8(bios, unk88->offset + 0x1, &unk88->hlen);
		err |= bios_u8(bios, unk88->offset + 0x2, &unk88->rlen);
		err |= bios_u8(bios, unk88->offset + 0x3, &unk88->entriesnum);
		unk88->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK88 table version 0x%x\n", unk88->version);
		return -EINVAL;
	};

	err = 0;
	unk88->entries = malloc(unk88->entriesnum * sizeof(struct envy_bios_power_unk88_entry));
	for (i = 0; i < unk88->entriesnum; i++) {
		uint16_t data = unk88->offset + unk88->hlen + i * unk88->rlen;

		unk88->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk88(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk88 *unk88 = &bios->power.unk88;
	int i;

	if (!unk88->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk88->valid) {
		fprintf(out, "Failed to parse UNK88 table at 0x%x, version %x\n", unk88->offset, unk88->version);
		return;
	}

	fprintf(out, "UNK88 table at 0x%x, version %x\n", unk88->offset, unk88->version);
	envy_bios_dump_hex(bios, out, unk88->offset, unk88->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk88->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk88->entries[i].offset, unk88->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk8c(struct envy_bios *bios) {
	struct envy_bios_power_unk8c *unk8c = &bios->power.unk8c;
	int i, err = 0;

	if (!unk8c->offset)
		return -EINVAL;

	bios_u8(bios, unk8c->offset + 0x0, &unk8c->version);
	switch(unk8c->version) {
	case 0x10:
		err |= bios_u8(bios, unk8c->offset + 0x1, &unk8c->hlen);
		err |= bios_u8(bios, unk8c->offset + 0x2, &unk8c->rlen);
		err |= bios_u8(bios, unk8c->offset + 0x3, &unk8c->entriesnum);
		unk8c->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK8C table version 0x%x\n", unk8c->version);
		return -EINVAL;
	};

	err = 0;
	unk8c->entries = malloc(unk8c->entriesnum * sizeof(struct envy_bios_power_unk8c_entry));
	for (i = 0; i < unk8c->entriesnum; i++) {
		uint16_t data = unk8c->offset + unk8c->hlen + i * unk8c->rlen;

		unk8c->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk8c(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk8c *unk8c = &bios->power.unk8c;
	int i;

	if (!unk8c->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk8c->valid) {
		fprintf(out, "Failed to parse UNK8C table at 0x%x, version %x\n", unk8c->offset, unk8c->version);
		return;
	}

	fprintf(out, "UNK8C table at 0x%x, version %x\n", unk8c->offset, unk8c->version);
	envy_bios_dump_hex(bios, out, unk8c->offset, unk8c->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk8c->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk8c->entries[i].offset, unk8c->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk90(struct envy_bios *bios) {
	struct envy_bios_power_unk90 *unk90 = &bios->power.unk90;
	int i, err = 0;

	if (!unk90->offset)
		return -EINVAL;

	bios_u8(bios, unk90->offset + 0x0, &unk90->version);
	switch(unk90->version) {
	case 0x10:
		err |= bios_u8(bios, unk90->offset + 0x1, &unk90->hlen);
		err |= bios_u8(bios, unk90->offset + 0x2, &unk90->rlen);
		err |= bios_u8(bios, unk90->offset + 0x3, &unk90->entriesnum);
		unk90->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK90 table version 0x%x\n", unk90->version);
		return -EINVAL;
	};

	err = 0;
	unk90->entries = malloc(unk90->entriesnum * sizeof(struct envy_bios_power_unk90_entry));
	for (i = 0; i < unk90->entriesnum; i++) {
		uint16_t data = unk90->offset + unk90->hlen + i * unk90->rlen;

		unk90->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk90(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk90 *unk90 = &bios->power.unk90;
	int i;

	if (!unk90->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk90->valid) {
		fprintf(out, "Failed to parse UNK90 table at 0x%x, version %x\n", unk90->offset, unk90->version);
		return;
	}

	fprintf(out, "UNK90 table at 0x%x, version %x\n", unk90->offset, unk90->version);
	envy_bios_dump_hex(bios, out, unk90->offset, unk90->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk90->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk90->entries[i].offset, unk90->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk94(struct envy_bios *bios) {
	struct envy_bios_power_unk94 *unk94 = &bios->power.unk94;
	int i, err = 0;

	if (!unk94->offset)
		return -EINVAL;

	bios_u8(bios, unk94->offset + 0x0, &unk94->version);
	switch(unk94->version) {
	case 0x10:
		err |= bios_u8(bios, unk94->offset + 0x1, &unk94->hlen);
		err |= bios_u8(bios, unk94->offset + 0x2, &unk94->rlen);
		err |= bios_u8(bios, unk94->offset + 0x3, &unk94->entriesnum);
		unk94->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK94 table version 0x%x\n", unk94->version);
		return -EINVAL;
	};

	err = 0;
	unk94->entries = malloc(unk94->entriesnum * sizeof(struct envy_bios_power_unk94_entry));
	for (i = 0; i < unk94->entriesnum; i++) {
		uint16_t data = unk94->offset + unk94->hlen + i * unk94->rlen;

		unk94->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk94(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk94 *unk94 = &bios->power.unk94;
	int i;

	if (!unk94->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk94->valid) {
		fprintf(out, "Failed to parse UNK94 table at 0x%x, version %x\n", unk94->offset, unk94->version);
		return;
	}

	fprintf(out, "UNK94 table at 0x%x, version %x\n", unk94->offset, unk94->version);
	envy_bios_dump_hex(bios, out, unk94->offset, unk94->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk94->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk94->entries[i].offset, unk94->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk98(struct envy_bios *bios) {
	struct envy_bios_power_unk98 *unk98 = &bios->power.unk98;
	int i, err = 0;

	if (!unk98->offset)
		return -EINVAL;

	bios_u8(bios, unk98->offset + 0x0, &unk98->version);
	switch(unk98->version) {
	case 0x10:
		err |= bios_u8(bios, unk98->offset + 0x1, &unk98->hlen);
		err |= bios_u8(bios, unk98->offset + 0x2, &unk98->rlen);
		err |= bios_u8(bios, unk98->offset + 0x3, &unk98->entriesnum);
		unk98->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK98 table version 0x%x\n", unk98->version);
		return -EINVAL;
	};

	err = 0;
	unk98->entries = malloc(unk98->entriesnum * sizeof(struct envy_bios_power_unk98_entry));
	for (i = 0; i < unk98->entriesnum; i++) {
		uint16_t data = unk98->offset + unk98->hlen + i * unk98->rlen;

		unk98->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk98(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk98 *unk98 = &bios->power.unk98;
	int i;

	if (!unk98->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;
	if (!unk98->valid) {
		fprintf(out, "Failed to parse UNK98 table at 0x%x, version %x\n", unk98->offset, unk98->version);
		return;
	}

	fprintf(out, "UNK98 table at 0x%x, version %x\n", unk98->offset, unk98->version);
	envy_bios_dump_hex(bios, out, unk98->offset, unk98->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk98->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk98->entries[i].offset, unk98->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}
