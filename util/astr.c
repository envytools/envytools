/*
 * Copyright (C) 2012 Marcelina Kościelnicka <mwk@0x04.net>
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

#include "util.h"

void print_escaped_astr(FILE *out, struct astr *astr) {
	int i;
	fprintf(out, "\"");
	for (i = 0; i < astr->len; i++) {
		unsigned char c = astr->str[i];
		switch (c) {
			case '\\':
				fprintf(out, "\\\\");
				break;
			case '\"':
				fprintf(out, "\\\"");
				break;
			case '\n':
				fprintf(out, "\\n");
				break;
			case '\f':
				fprintf(out, "\\f");
				break;
			case '\t':
				fprintf(out, "\\t");
				break;
			case '\a':
				fprintf(out, "\\a");
				break;
			case '\v':
				fprintf(out, "\\v");
				break;
			case '\r':
				fprintf(out, "\\r");
				break;
			default:
				if (c >= 0x20 && c <= 0x7e) {
					fprintf(out, "%c", c);
				} else {
					fprintf(out, "\\x%02x", c);
				}
				break;
		}
	}
	fprintf(out, "\"");
}
