/*
 * Copyright (C) 2015 Samuel Pitoiset <samuel.pitoiset@gmail.com>.
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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/wait.h>

#include "decode_utils.h"
#include "util.h"

static void
usage()
{
	printf("Usage:\n");
	printf("	lookup_trace -a NVXX -e event\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	char *event = NULL, *chipset = NULL;
	int use_colors = 1;
	int c;

	if (argc < 4) {
		usage();
		return 1;
	}

	/* Arguments parsing */
	while ((c = getopt(argc, argv, "a:c:e:")) != -1) {
		switch (c) {
			case 'a':
				chipset = strdup(optarg);
				break;
			case 'c':
				use_colors = 0;
				break;
			case 'e':
				event = strdup(optarg);
				break;
			default:
				usage();
		}
	}

	if (!chipset || !event)
		usage();

	init_rnnctx(chipset, use_colors);

	if (lookup_trace(chipset, event))
		return 1;

	destroy_rnnctx();

	return 0;
}
