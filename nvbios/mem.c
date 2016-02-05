/*
 * Copyright (C) 2014 Roy Spliet <rspliet@eclipso.eu>
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

extern uint8_t ram_restrict_group_count;
extern uint32_t strap;

struct M_known_tables {
	uint8_t offset;
	uint16_t *ptr;
	const char *name;
};

extern const char * mem_type(uint8_t version, uint16_t start);

static int
parse_at(struct envy_bios *bios, struct envy_bios_mem *mem,
	     int idx, int offset, const char ** name)
{
	struct M_known_tables m1_tbls[] = {
		{ 0x03, &mem->trestrict, "RESTRICT" },
	};
	struct M_known_tables m2_tbls[] = {
		{ 0x01, &mem->trestrict, "RESTRICT" },
		{ 0x03, &mem->type.offset, "TYPE" },
		{ 0x05, &mem->train.offset, "TRAIN" }
	};
	struct M_known_tables *tbls;
	int entries_count = 0;

	if (mem->bit->version == 0x1) {
		tbls = m1_tbls;
		entries_count = (sizeof(m1_tbls) / sizeof(struct M_known_tables));
	} else if (mem->bit->version == 0x2) {
		tbls = m2_tbls;
		entries_count = (sizeof(m2_tbls) / sizeof(struct M_known_tables));
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
	if (tbls[idx].offset + 2 > mem->bit->t_len)
		return -ENOENT;

	if (name)
		*name = tbls[idx].name;

	return bios_u16(bios,
			mem->bit->t_offset + tbls[idx].offset,
			tbls[idx].ptr);
}

int
envy_bios_parse_mem_train (struct envy_bios *bios) {
	struct envy_bios_mem_train *mt = &bios->mem.train;
	if (!mt->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, mt->offset, &mt->version);
	err |= bios_u8(bios, mt->offset+1, &mt->hlen);
	err |= bios_u8(bios, mt->offset+2, &mt->entriesnum);
	err |= bios_u8(bios, mt->offset+3, &mt->rlen);
	err |= bios_u8(bios, mt->offset+4, &mt->subentries);
	err |= bios_u8(bios, mt->offset+5, &mt->subentrylen);
	if (err)
		return -EFAULT;
	bios_u16(bios, mt->offset+6, &mt->mclk);

	envy_bios_block(bios, mt->offset, mt->hlen + mt->rlen * mt->entriesnum, "MEM TRAIN", -1);
	mt->entries = calloc(mt->entriesnum, sizeof *mt->entries);
	if (!mt->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < mt->entriesnum; i++) {
		struct envy_bios_mem_train_entry *entry = &mt->entries[i];
		entry->offset = mt->offset + mt->hlen + ((mt->rlen + mt->subentries * mt->subentrylen) * i);
		err |= bios_u8(bios, entry->offset, &entry->u00);
		if (mt->subentries > sizeof(entry->subentry)) {
			mt->subentries = sizeof(entry->subentry);
			ENVY_BIOS_ERR("Error when parsing mem train: subentries = %d > %zu\n", mt->subentries, sizeof(entry->subentry));
			return -EFAULT;
		}
		int j;
		for (j = 0; j < mt->subentries; j++) {
			err |= bios_u8(bios, entry->offset+j+1, &entry->subentry[j]);
		}
		if (err)
			return -EFAULT;
	}
	mt->valid = 1;
	return 0;
}

void
envy_bios_print_mem_train(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_mem_train *mt = &bios->mem.train;
	uint8_t ram_cfg = strap?(strap & 0x1c) >> 2:0xff;
	int i, j;

	if (!mt->offset || !(mask & ENVY_BIOS_PRINT_MEM))
		return;

	fprintf(out, "MEM TRAIN table at 0x%x, version %x\n", mt->offset, mt->version);
	fprintf(out, "Training clock: %huMHz\n", mt->mclk);
	envy_bios_dump_hex(bios, out, mt->offset, mt->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < mt->entriesnum; i++) {
		fprintf(out, "    %i: %02hhx\n", i,
			mt->entries[i].u00);
		envy_bios_dump_hex(bios, out, mt->entries[i].offset, mt->rlen, mask);
		if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

		for (j = 0; j < mt->subentries; j++) {
			if(j == ram_cfg) {
				fprintf(out, "     *");
			} else {
				fprintf(out, "      ");
			}
			fprintf(out, " %02i: %02hhx\n",
				j, mt->entries[i].subentry[j]);
			envy_bios_dump_hex(bios, out, mt->entries[i].offset+j+1, mt->subentrylen, mask);
			if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");
		}
	}

	fprintf(out, "\n");
}

int
envy_bios_parse_mem_type (struct envy_bios *bios) {
	struct envy_bios_mem_type *mt = &bios->mem.type;
	if (!mt->offset)
		return 0;
	int err = 0;
	err |= bios_u8(bios, mt->offset, &mt->version);
	err |= bios_u8(bios, mt->offset+1, &mt->hlen);
	err |= bios_u8(bios, mt->offset+2, &mt->rlen);
	err |= bios_u8(bios, mt->offset+3, &mt->entriesnum);
	if (err)
		return -EFAULT;

	envy_bios_block(bios, mt->offset, mt->hlen + mt->rlen * mt->entriesnum, "MEM TYPE", -1);
	mt->entries = calloc(mt->entriesnum, sizeof *mt->entries);
	if (!mt->entries)
		return -ENOMEM;
	int i;
	for (i = 0; i < mt->entriesnum; i++) {
		const char **entry = &mt->entries[i];
		*entry = mem_type(mt->version, mt->offset + mt->hlen + (mt->rlen * i));
		if (err)
			return -EFAULT;
	}
	mt->valid = 1;
	return 0;
}

void
envy_bios_print_mem_type(struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_mem_type *mt = &bios->mem.type;
	uint8_t ram_cfg = strap?(strap & 0x1c) >> 2:0xff;
	int i;

	if (!mt->offset || !(mask & ENVY_BIOS_PRINT_MEM))
		return;

	fprintf(out, "MEM TYPE table at 0x%x, version %x, %u entries\n", mt->offset, mt->version, mt->entriesnum);
	if(mt->version == 0x10) {
		printf("Detected ram type: %s\n",
		       mem_type(mt->version, mt->offset + mt->hlen + (ram_cfg*mt->rlen)));
	}

	envy_bios_dump_hex(bios, out, mt->offset, mt->hlen, mask);
	if (mask & ENVY_BIOS_PRINT_VERBOSE) fprintf(out, "\n");

	for (i = 0; i < mt->entriesnum; i++) {
		if(i == ram_cfg) {
			fprintf(out, "   *");
		} else {
			fprintf(out,"    ");
		}
		fprintf(out,"%02u: Ram type: %s\n", i, mt->entries[i]);
		envy_bios_dump_hex(bios, out, mt->offset + mt->rlen * i, mt->rlen, mask);
	}

	fprintf(out, "\n");
}

int
envy_bios_parse_bit_M (struct envy_bios *bios, struct envy_bios_bit_entry *bit) {
	struct envy_bios_mem *mem = &bios->mem;
	int idx = 0;

	mem->bit = bit;
	while (!parse_at(bios, mem, idx, -1, NULL))
		idx++;

	if (bit->version == 1) {
		if (bit->t_len >= 5) {
			bios_u8(bios, bit->t_offset+2, &ram_restrict_group_count);
		}
	} else if (bit->version == 2) {
		if (bit->t_len >= 3) {
			bios_u8(bios, bit->t_offset, &ram_restrict_group_count);
		}
	}

	envy_bios_parse_mem_train(bios);
	envy_bios_parse_mem_type(bios);

	return 0;
}

void
envy_bios_print_bit_M (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_mem *mem = &bios->mem;
	const char *name;
	uint16_t addr;
	int ret = 0, i = 0;

	if (!mem->bit || !(mask & ENVY_BIOS_PRINT_MEM))
		return;

	fprintf(out, "BIT table 'M' at 0x%x, version %i\n",
			mem->bit->offset, mem->bit->version);

	for (i = 1; i < mem->bit->t_len; i+=2) {
		ret = bios_u16(bios, mem->bit->t_offset + i, &addr);
		if (!ret && addr) {
			name = "UNKNOWN";
			ret = parse_at(bios, mem, -1, i, &name);
			fprintf(out, "0x%02x: 0x%x => %s TABLE\n", i, addr, name);
		}
	}

	fprintf(out, "\n");
}
