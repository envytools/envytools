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
#include <string.h>

int envy_bios_parse_power_unk14(struct envy_bios *bios);
int envy_bios_parse_power_budget(struct envy_bios *bios);
int envy_bios_parse_power_boost(struct envy_bios *bios);
int envy_bios_parse_power_cstep(struct envy_bios *bios);
int envy_bios_parse_power_unk24(struct envy_bios *bios);
int envy_bios_parse_power_sense(struct envy_bios *bios);
int envy_bios_parse_power_unk38(struct envy_bios *bios);
int envy_bios_parse_power_unk3c(struct envy_bios *bios);
int envy_bios_parse_power_unk40(struct envy_bios *bios);
int envy_bios_parse_power_unk44(struct envy_bios *bios);
int envy_bios_parse_power_unk48(struct envy_bios *bios);
int envy_bios_parse_power_unk4c(struct envy_bios *bios);
int envy_bios_parse_power_unk50(struct envy_bios *bios);
int envy_bios_parse_power_unk54(struct envy_bios *bios);
int envy_bios_parse_power_unk58(struct envy_bios *bios);
int envy_bios_parse_power_unk5c(struct envy_bios *bios);
int envy_bios_parse_power_unk60(struct envy_bios *bios);
int envy_bios_parse_power_unk64(struct envy_bios *bios);

struct P_known_tables {
	uint8_t offset;
	uint16_t *ptr;
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
		{ 0x15, &power->unk.offset, "POWER UNK" }
	};
	struct P_known_tables p2_tbls[] = {
		{ 0x00, &power->perf.offset, "PERFORMANCE" },
		{ 0x04, &power->timing_map.offset, "MEMORY TIMINGS MAPPING" },
		{ 0x08, &power->timing.offset, "MEMORY TIMINGS" },
		{ 0x0c, &power->volt.offset, "VOLTAGE" },
		{ 0x10, &power->therm.offset, "THERMAL"  },
		{ 0x14, &power->unk14.offset, "UNK14"  },
		{ 0x18, &power->unk.offset, "POWER UNK" },
		{ 0x20, &power->volt_map.offset, "VOLT MAPPING" },
		{ 0x24, &power->unk24.offset, "POWER UNK24" },
		{ 0x28, &power->sense.offset, "POWER SENSE" },
		{ 0x2c, &power->budget.offset, "POWER BUDGET" },
		{ 0x30, &power->boost.offset, "BOOST" },
		{ 0x34, &power->cstep.offset, "CSTEP" },
		{ 0x38, &power->unk38.offset, "POWER UNK38" },
		{ 0x3c, &power->unk3c.offset, "POWER UNK3C" },
		{ 0x40, &power->unk40.offset, "POWER UNK40" },
		{ 0x44, &power->unk44.offset, "POWER UNK44" },
		{ 0x48, &power->unk48.offset, "POWER UNK48" },
		{ 0x4c, &power->unk4c.offset, "POWER UNK4C" },
		{ 0x50, &power->unk50.offset, "POWER UNK50" },
		{ 0x54, &power->unk54.offset, "POWER UNK54" },
		{ 0x58, &power->unk58.offset, "POWER UNK58" },
		{ 0x5c, &power->unk5c.offset, "POWER UNK5C" },
		{ 0x60, &power->unk60.offset, "POWER UNK60" },
		{ 0x64, &power->unk64.offset, "POWER UNK64" }
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

	envy_bios_parse_power_unk14(bios);
	envy_bios_parse_power_unk24(bios);
	envy_bios_parse_power_sense(bios);
	envy_bios_parse_power_budget(bios);
	envy_bios_parse_power_boost(bios);
	envy_bios_parse_power_cstep(bios);
	envy_bios_parse_power_unk38(bios);
	envy_bios_parse_power_unk3c(bios);
	envy_bios_parse_power_unk40(bios);
	envy_bios_parse_power_unk44(bios);
	envy_bios_parse_power_unk48(bios);
	envy_bios_parse_power_unk4c(bios);
	envy_bios_parse_power_unk50(bios);
	envy_bios_parse_power_unk54(bios);
	envy_bios_parse_power_unk58(bios);
	envy_bios_parse_power_unk5c(bios);
	envy_bios_parse_power_unk60(bios);
	envy_bios_parse_power_unk64(bios);

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
}

int envy_bios_parse_power_unk14(struct envy_bios *bios) {
	struct envy_bios_power_unk14 *unk14 = &bios->power.unk14;
	int i, err = 0;

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

	fprintf(out, "UNK14 table at 0x%x, version %x\n", unk14->offset, unk14->version);
	envy_bios_dump_hex(bios, out, unk14->offset, unk14->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk14->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk14->entries[i].offset, unk14->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_boost(struct envy_bios *bios) {
	struct envy_bios_power_boost *boost = &bios->power.boost;
	int i, j, err = 0;

	bios_u8(bios, boost->offset + 0x0, &boost->version);
	switch(boost->version) {
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

	for (i = 0; i < boost->entriesnum; i++) {
		uint16_t data = boost->offset + boost->hlen + i * (boost->rlen + (boost->snr * boost->ssz));

		uint16_t tmp;
		err |= bios_u16(bios, data + 0x0, &tmp);
		err |= bios_u16(bios, data + 0x2, &boost->entries[i].min);
		err |= bios_u16(bios, data + 0x4, &boost->entries[i].max);

		boost->entries[i].offset = data;
		boost->entries[i].pstate = (tmp & 0x01e0) >> 5;

		boost->entries[i].entries = malloc(boost->snr * sizeof(struct envy_bios_power_boost_subentry));

		for (j = 0; j < boost->snr; j++) {
			struct envy_bios_power_boost_subentry *sub = &boost->entries[i].entries[j];
			uint16_t sdata = data + boost->rlen + j * boost->ssz;

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

	for (i = 0; i < cstep->entriesnum; i++) {
		uint16_t data = cstep->offset + cstep->hlen + i * cstep->rlen;

		uint16_t tmp;
		err |= bios_u16(bios, data + 0x0, &tmp);

		cstep->ent1[i].offset = data;
		cstep->ent1[i].pstate = (tmp & 0x01e0) >> 5;
		bios_u8(bios, data + 0x3, &cstep->ent1[i].index);
	}

	cstep->ent2 = malloc(cstep->snr * sizeof(struct envy_bios_power_cstep_entry2));
	memset(cstep->ent2, 0x0, cstep->snr * sizeof(struct envy_bios_power_cstep_entry2));
	for (i = 0; i < cstep->snr; i++) {
		uint16_t data = cstep->offset + cstep->hlen + (cstep->entriesnum * cstep->rlen) + (i * cstep->ssz);

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

	bios_u8(bios, budget->offset + 0x0, &budget->version);
	switch(budget->version) {
	case 0x10:
	case 0x20:
		err |= bios_u8(bios, budget->offset + 0x1, &budget->hlen);
		err |= bios_u8(bios, budget->offset + 0x2, &budget->rlen);
		err |= bios_u8(bios, budget->offset + 0x3, &budget->entriesnum);
		budget->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown POWER BUDGET table version 0x%x\n", budget->version);
		return -EINVAL;
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

	fprintf(out, "POWER BUDGET table at 0x%x, version %x\n", budget->offset, budget->version);
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
			err |= bios_u8(bios, data + 0x2, &sense->entries[i].extdev_id);
			err |= bios_u8(bios, data + 0x3, &sense->entries[i].resistor_mohm);
			break;
		case 0x20:
			err |= bios_u8(bios, data + 0x1, &sense->entries[i].extdev_id);
			err |= bios_u8(bios, data + 0x5, &sense->entries[i].resistor_mohm);
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

	fprintf(out, "SENSE table at 0x%x, version %x\n", sense->offset, sense->version);
	envy_bios_dump_hex(bios, out, sense->offset, sense->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < sense->entriesnum; i++) {
		fprintf(out, "power rail %i: extdev_id/power_rail = %u, shunt resistor = %u mOhm\n",
			i, sense->entries[i].extdev_id, sense->entries[i].resistor_mohm);
		envy_bios_dump_hex(bios, out, sense->entries[i].offset, sense->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk38(struct envy_bios *bios) {
	struct envy_bios_power_unk38 *unk38 = &bios->power.unk38;
	int i, err = 0;

	bios_u8(bios, unk38->offset + 0x0, &unk38->version);
	switch(unk38->version) {
	case 0x10:
		err |= bios_u8(bios, unk38->offset + 0x1, &unk38->hlen);
		err |= bios_u8(bios, unk38->offset + 0x2, &unk38->rlen);
		err |= bios_u8(bios, unk38->offset + 0x3, &unk38->entriesnum);
		unk38->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK38 table version 0x%x\n", unk38->version);
		return -EINVAL;
	};

	err = 0;
	unk38->entries = malloc(unk38->entriesnum * sizeof(struct envy_bios_power_unk38_entry));
	for (i = 0; i < unk38->entriesnum; i++) {
		uint16_t data = unk38->offset + unk38->hlen + i * unk38->rlen;

		unk38->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk38(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk38 *unk38 = &bios->power.unk38;
	int i;

	if (!unk38->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;

	fprintf(out, "UNK38 table at 0x%x, version %x\n", unk38->offset, unk38->version);
	envy_bios_dump_hex(bios, out, unk38->offset, unk38->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk38->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk38->entries[i].offset, unk38->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk3c(struct envy_bios *bios) {
	struct envy_bios_power_unk3c *unk3c = &bios->power.unk3c;
	int i, err = 0;

	bios_u8(bios, unk3c->offset + 0x0, &unk3c->version);
	switch(unk3c->version) {
	case 0x10:
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

	fprintf(out, "UNK44 table at 0x%x, version %x\n", unk44->offset, unk44->version);
	envy_bios_dump_hex(bios, out, unk44->offset, unk44->hlen, mask);
	fprintf(out, "\n");
}

int envy_bios_parse_power_unk48(struct envy_bios *bios) {
	struct envy_bios_power_unk48 *unk48 = &bios->power.unk48;
	int i, err = 0;

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
		uint16_t data = unk50->offset + unk50->hlen + i * unk50->rlen;

		unk50->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk50(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk50 *unk50 = &bios->power.unk50;
	int i;

	if (!unk50->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;

	fprintf(out, "UNK50 table at 0x%x, version %x\n", unk50->offset, unk50->version);
	envy_bios_dump_hex(bios, out, unk50->offset, unk50->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk50->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk50->entries[i].offset, unk50->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk54(struct envy_bios *bios) {
	struct envy_bios_power_unk54 *unk54 = &bios->power.unk54;
	int i, err = 0;

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
		uint16_t data = unk54->offset + unk54->hlen + i * unk54->rlen;

		unk54->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk54(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk54 *unk54 = &bios->power.unk54;
	int i;

	if (!unk54->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;

	fprintf(out, "UNK54 table at 0x%x, version %x\n", unk54->offset, unk54->version);
	envy_bios_dump_hex(bios, out, unk54->offset, unk54->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk54->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk54->entries[i].offset, unk54->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk58(struct envy_bios *bios) {
	struct envy_bios_power_unk58 *unk58 = &bios->power.unk58;
	int i, err = 0;

	bios_u8(bios, unk58->offset + 0x0, &unk58->version);
	switch(unk58->version) {
	case 0x10:
		err |= bios_u8(bios, unk58->offset + 0x1, &unk58->hlen);
		err |= bios_u8(bios, unk58->offset + 0x2, &unk58->rlen);
		err |= bios_u8(bios, unk58->offset + 0x3, &unk58->entriesnum);

		/* F*ck logic, right? */
		unk58->entriesnum = unk58->rlen / 5;
		unk58->rlen = 5;

		unk58->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK58 table version 0x%x\n", unk58->version);
		return -EINVAL;
	};

	unk58->entries = malloc(unk58->entriesnum * sizeof(struct envy_bios_power_unk58_entry));
	for (i = 0; i < unk58->entriesnum; i++) {
		uint16_t data = unk58->offset + unk58->hlen + i * unk58->rlen;

		unk58->entries[i].offset = data;
		err |= bios_u8(bios, data, &unk58->entries[i].id);
		err |= bios_u32(bios, data + 1, &unk58->entries[i].value);
	}

	unk58->valid = !err;

	return 0;
}

void envy_bios_print_power_unk58(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk58 *unk58 = &bios->power.unk58;
	int i;

	if (!unk58->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;

	fprintf(out, "UNK58 table at 0x%x, version %x\n", unk58->offset, unk58->version);
	envy_bios_dump_hex(bios, out, unk58->offset, unk58->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk58->entriesnum; i++) {
		fprintf(out, "id = 0x%02x, data = 0x%08x -- ",
			unk58->entries[i].id, unk58->entries[i].value);

		switch(unk58->entries[i].id) {
		case 0x9:
			fprintf(out, "Fan divisor: %u; unkflags: %x\n",
				unk58->entries[i].value & 0xffffff,
				unk58->entries[i].value >> 24);
			break;
		default:
			fprintf(out, "Unknown\n");
		}

		envy_bios_dump_hex(bios, out, unk58->entries[i].offset, unk58->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk5c(struct envy_bios *bios) {
	struct envy_bios_power_unk5c *unk5c = &bios->power.unk5c;
	int i, err = 0;

	bios_u8(bios, unk5c->offset + 0x0, &unk5c->version);
	switch(unk5c->version) {
	case 0x10:
		err |= bios_u8(bios, unk5c->offset + 0x1, &unk5c->hlen);
		err |= bios_u8(bios, unk5c->offset + 0x2, &unk5c->rlen);
		err |= bios_u8(bios, unk5c->offset + 0x3, &unk5c->entriesnum);
		unk5c->valid = !err;
		break;
	default:
		ENVY_BIOS_ERR("Unknown UNK5c table version 0x%x\n", unk5c->version);
		return -EINVAL;
	};

	err = 0;
	unk5c->entries = malloc(unk5c->entriesnum * sizeof(struct envy_bios_power_unk5c_entry));
	for (i = 0; i < unk5c->entriesnum; i++) {
		uint16_t data = unk5c->offset + unk5c->hlen + i * unk5c->rlen;

		unk5c->entries[i].offset = data;
	}

	return 0;
}

void envy_bios_print_power_unk5c(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_power_unk5c *unk5c = &bios->power.unk5c;
	int i;

	if (!unk5c->offset || !(mask & ENVY_BIOS_PRINT_PERF))
		return;

	fprintf(out, "UNK5c table at 0x%x, version %x\n", unk5c->offset, unk5c->version);
	envy_bios_dump_hex(bios, out, unk5c->offset, unk5c->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk5c->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk5c->entries[i].offset, unk5c->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}

int envy_bios_parse_power_unk60(struct envy_bios *bios) {
	struct envy_bios_power_unk60 *unk60 = &bios->power.unk60;
	int i, err = 0;

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

	fprintf(out, "UNK64 table at 0x%x, version %x\n", unk64->offset, unk64->version);
	envy_bios_dump_hex(bios, out, unk64->offset, unk64->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < unk64->entriesnum; i++) {
		envy_bios_dump_hex(bios, out, unk64->entries[i].offset, unk64->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
	}

	fprintf(out, "\n");
}
