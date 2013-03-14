/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
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

int envy_bios_parse_bit_2 (struct envy_bios *bios, struct envy_bios_bit_entry *bit) {
	struct envy_bios_i2cscript *i2cscript = &bios->i2cscript;
	i2cscript->bit = bit;
	int wantlen = 4;
	if (bit->t_len < wantlen) {
		ENVY_BIOS_ERR("I2C script table too short: %d < %d\n", bit->t_len, wantlen);
		return -EINVAL;
	}
	if (bit->t_len > wantlen)
		ENVY_BIOS_WARN("I2C script table longer than expected: %d > %d\n", bit->t_len, wantlen);
	int err = 0;
	err |= bios_u16(bios, bit->t_offset+0, &i2cscript->unk00);
	err |= bios_u16(bios, bit->t_offset+2, &i2cscript->script);
	if (err)
		return -EFAULT;
	i2cscript->valid = 1;
	return 0;
}

void printscript (uint16_t soff);

void envy_bios_print_i2cscript (struct envy_bios *bios, FILE *out, unsigned mask) {
	struct envy_bios_i2cscript *i2cscript = &bios->i2cscript;
	if (!i2cscript->bit || !(mask & ENVY_BIOS_PRINT_I2CSCRIPT))
		return;
	if (!i2cscript->valid) {
		fprintf(out, "Failed to parse BIT table '2' at 0x%04x version 1\n\n", i2cscript->bit->t_offset);
		return;
	}
	if ((mask & ENVY_BIOS_PRINT_VERBOSE) || i2cscript->unk00 || i2cscript->script) {
		fprintf(out, "I2C script");
		if (i2cscript->unk00)
			fprintf(out, " unk00 0x%04x", i2cscript->unk00);
		if (i2cscript->script)
			fprintf(out, ":");
		fprintf(out, "\n");
		envy_bios_dump_hex(bios, out, i2cscript->bit->t_offset, i2cscript->bit->t_len, mask);
		if (i2cscript->script)
			printscript(i2cscript->script);
		fprintf(out, "\n");
	}
}
