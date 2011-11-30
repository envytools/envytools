/*
 * Copyright 2011 Martin Peres (inspired from Emil Velikov's work)
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <X11/Xlib.h>
#include "libXNVCtrl/NVCtrl.h"
#include "libXNVCtrl/NVCtrlLib.h"

#include <sys/types.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include "nva.h"

#define	NOUVEAU_TIME_WAIT 60

int vbios_upload_pramin(int cnum, uint8_t *vbios, int length);

enum value_type {
	EMPTY,
	VALUE,
	BITFIELD
};

enum value_type timing_value_types[25] = {
	VALUE, VALUE, VALUE, VALUE, EMPTY,
	VALUE, EMPTY, VALUE, EMPTY, VALUE,
	VALUE, VALUE, VALUE, VALUE, BITFIELD,
	VALUE, BITFIELD, VALUE, VALUE, VALUE,
	VALUE, VALUE, VALUE, VALUE, VALUE
};

static int
write_to_file(const char *path, const char *data)
{
	FILE *file = fopen(path, "wb");
	if (!file)
		return 1;
	fputs(data, file);
	fclose(file);
	return 0;
}

static void mmiotrace_start(int entry)
{
	int ret;

	ret = mount("debugfs", "/sys/kernel/debug", "debugfs", 0, 0);
	if (ret)
		fprintf(stderr, "mounting debugfs returned error %i: %s\n", ret, strerror(ret));

	if (write_to_file("/sys/kernel/debug/tracing/buffer_size_kb", "128000"))
		fprintf(stderr, "Increasing the buffer size returned error %i: %s\n", ret, strerror(ret));

	if (write_to_file("/sys/kernel/debug/tracing/current_tracer", "mmiotrace"))
		fprintf(stderr, "Changing the tracer returned error %i: %s\n", ret, strerror(ret));

	if (write_to_file("/sys/kernel/debug/tracing/current_tracer", "mmiotrace"))
		fprintf(stderr, "Changing the tracer returned error %i: %s\n", ret, strerror(ret));

	pid_t pid = fork();
        if (pid == 0) {
                char cmd[101];
                snprintf(cmd, 100, "cat /sys/kernel/debug/tracing/trace_pipe > mmt_entry_%i", entry);
		exit(system(cmd));
	} else if (pid < 0) {
		perror("fork");
	}

	printf("mmiotrace started\n");
}

static void
mmiotrace_stop()
{
	if (write_to_file("/sys/kernel/debug/tracing/current_tracer", "nop"))
		fprintf(stderr, "Copying the trace to a file failed\n");

	printf("mmiotrace stopped\n");	
}

int upclock_card(Display *dpy)
{
	unsigned long start_time, get_time;
	struct timeval tv;
	int cur_perflvl = -1, tmp;

	gettimeofday(&tv, NULL);
	start_time = tv.tv_sec;

	XNVCTRLQueryTargetAttribute (dpy, NV_CTRL_TARGET_TYPE_X_SCREEN, 0,
					0, NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL, &cur_perflvl);

	pid_t pid = fork();
	if (pid == 0) {
		exit (system("timeout 3 glxgears"));
	} else if (pid == -1) {
		fprintf(stderr, "Fork failed! Abort\n");
		exit (4);
	}

	do
	{
		usleep(10000); /* 10 ms */

		gettimeofday(&tv, NULL);
		get_time = tv.tv_sec;
		if ((start_time + 5) < get_time ) {
			fprintf(stderr, "ERROR - Timeout\n");
			return 4;
		}

		XNVCTRLQueryTargetAttribute (dpy, NV_CTRL_TARGET_TYPE_X_SCREEN, 0,
						0, NV_CTRL_GPU_CURRENT_PERFORMANCE_MODE, &tmp);

	} while ( tmp != NV_CTRL_GPU_CURRENT_PERFORMANCE_MODE_MAXPERF);

	kill(pid, SIGTERM);

	return 0;
}


int wait_for_perflvl(Display *dpy, uint8_t wanted_perflvl)
{
	unsigned long start_time, get_time;
	struct timeval tv;
	int cur_perflvl = -1, tmp;

	gettimeofday(&tv, NULL);
	start_time = tv.tv_sec;

	XNVCTRLQueryTargetAttribute (dpy, NV_CTRL_TARGET_TYPE_GPU, 0, 0, NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE , &tmp);
	if (tmp != NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE_ENABLED) {
		fprintf(stderr, "ERROR - Driver not in Adaptive state\n");
		return 1;
	}

	do
	{
		gettimeofday(&tv, NULL);
		get_time = tv.tv_sec;
		if ((start_time + NOUVEAU_TIME_WAIT) < get_time ) {
			fprintf(stderr, "ERROR - Timeout\n");
			return 2;
		}

		XNVCTRLQueryTargetAttribute (dpy, NV_CTRL_TARGET_TYPE_X_SCREEN, 0,
						0, NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL, &tmp);
		if (cur_perflvl < 0) {
			cur_perflvl = tmp;
			fprintf(stderr, "NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL = %d\n", cur_perflvl);
		}
		else if (tmp != cur_perflvl) {
			cur_perflvl = tmp;
			fprintf(stderr, "Changed NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL = %d\n", cur_perflvl);
		}

	} while ( cur_perflvl != wanted_perflvl );

	return 0;
}

int force_timing_changes(uint8_t wanted_perflvl, int mmiotrace)
{
	Display *dpy;
	int ret, i = 0;

	/* wait for X to come up */
	do
	{
		usleep(10000); /* 10 ms */
		dpy = XOpenDisplay(NULL);
		i++;
	} while(!dpy && i < 200 ); /* wait 2 seconds */
	
	if (!dpy) {
		fprintf(stderr, "ERROR - XOpenDisplay failed\n");
		return 1;
	}

	if (!upclock_card(dpy)) {
		if (write_to_file("/sys/kernel/debug/tracing/trace_marker", "Start downclocking\n"))
			fprintf(stderr, "Addind a marker to the mmiotrace returned error %i: %s\n", ret, strerror(ret));

		if (!wait_for_perflvl(dpy, wanted_perflvl)) {
			fprintf(stderr, "Downclock monitoring finished\n");
			ret = 0;
		}
	}

	/* wait for PDAEMON to actually change the memory clock */
	sleep(1);
	
	XCloseDisplay(dpy);

	return ret;
}

static void print_reg_diff(FILE* outf, uint32_t orig, uint32_t dest, int no_diff)
{
	uint8_t *o = (uint8_t*)&orig;
	uint8_t *d = (uint8_t*)&dest;
	int i;

	for (i = 3; i >= 0; i--) {
		const char* colour = "\e[0;31m";

		if (no_diff || o[i] == d[i])
                        colour = NULL;  

		fprintf(outf, "%s%02x%s", colour?colour:"", d[i], colour?"\e[0m":"");
	}
	fprintf(outf, " ");
}

static void dump_timings(int cnum, FILE* outf, uint8_t *vbios_entry,
		  size_t vbios_entry_size, int8_t progression, int color)
{
	static uint32_t ref_val[0xd0] = { 0 };
	static int ref_exist = 0;
	uint32_t reg, regs_len, val;
	int i, r;

	fprintf(outf, "timing entry [%u/%zu]: ", progression, vbios_entry_size);
	for (i = 0; i < vbios_entry_size; i++) {
		if (i != progression - 1)
			fprintf(outf, "%02x ", vbios_entry[i]);
		else
			fprintf(outf, "\e[0;31m%02x\e[0m ", vbios_entry[i]);
	}
	fprintf(outf, "\n");

	if (nva_cards[cnum].card_type >= 0xc0) {
		reg = 0x10f290;
		regs_len = 0x70;
	} else  {
		reg = 0x100220;
		regs_len = 0xd0;
	}

	for (r = 0; r < regs_len; r+=4)
	{
		if (r % 0x10 == 0)
			fprintf(outf, "%08x: ", reg + r);

		val = nva_rd32(cnum, reg + r);

		print_reg_diff(outf, ref_val[r], val, !color || progression == 0);

		/* if the value wasn't initialized before, store it as a reference */
		if (ref_exist == 0)
			ref_val[r] = val;

		if (r % 0x10 == 0xc)
			fprintf(outf, "\n");
	}

	fprintf(outf, "\n");
	fflush(outf);

	ref_exist = 1;
}

static int launch(int cnum, FILE* outf, uint8_t *vbios_data, uint16_t vbios_length, uint8_t perflvl,
	uint16_t timing_entry_offset, uint8_t entry_length, int mmiotrace, uint8_t progression, int color)
{
	printf("--- Start sequence number %u\n", progression);

	if (system("rmmod nvidia") == 127)
	{
		fprintf(stderr, "rmmod not found. Abort\n");
		exit (2);
	}
	
	if (vbios_upload_pramin(cnum, vbios_data, vbios_length) != 1)
	{
		fprintf(stderr, "upload failed. Abort\n");
		exit(3);
	}

	/* modify our env so as we can launch apps on X */
	setenv("DISPLAY", ":0", 1);

	/* monitor downclocking */
	if (mmiotrace)
		mmiotrace_start(progression);

	pid_t pid = fork();
	if (pid == 0) {
		system("killall X 2> /dev/null > /dev/null");
		exit(system("X > /dev/null 2> /dev/null"));
	} else if (pid == -1) {
		fprintf(stderr, "Fork failed! Abort\n");
		exit (4);
	}

	/* X runs, wait for the right perflvl to be selected */
	if (force_timing_changes(perflvl, mmiotrace))
		return 1;

	if (mmiotrace)
                mmiotrace_stop();

	dump_timings(cnum, outf, vbios_data + timing_entry_offset, entry_length, progression, color);

	system("killall X");
	sleep(5);

	return 0;
}

int complete_dump(int cnum, uint8_t *vbios_data, uint16_t vbios_length,
	      uint16_t timing_entry_offset, uint8_t entry_length,
	      uint8_t perflvl, int do_mmiotrace)
{
	int i, b;

	/* TODO: get this filename from the command line */
	FILE *outf = fopen("regs_timing", "wb");
	if (!outf) {
		perror("Open regs_timing");
		return 1;
	}

	launch(cnum, outf, vbios_data, vbios_length, perflvl,
		       timing_entry_offset, entry_length, do_mmiotrace, 0, 0);


	/* iterate through the vbios timing values */
	for (i = 0; i < entry_length; i++) {
		uint8_t orig = vbios_data[timing_entry_offset + i];

		switch (timing_value_types[i]) {
		case EMPTY:
			if (orig == 0) {
				fprintf(outf, "timing entry [%u/%zu] is supposed empty\n\n", i + 1, entry_length);
				break;
			} else
				fprintf(outf, "WARNING: The following entry was supposed to be unused!\n");
		case VALUE:
			vbios_data[timing_entry_offset + i]++;
			launch(cnum, outf, vbios_data, vbios_length, perflvl,
				timing_entry_offset, entry_length, do_mmiotrace, i + 1, 1);
			break;
		case BITFIELD:
			for (b = 0; b < 8; b++) {
				vbios_data[timing_entry_offset + i] = (1 << b);
				launch(cnum, outf, vbios_data, vbios_length, perflvl,
					timing_entry_offset, entry_length, do_mmiotrace, i + 1, 1);
			}
			break;
		}

		vbios_data[timing_entry_offset + i] = orig;
	}

	fclose(outf);

	return 0;
}

int manual_check(int cnum, int manual_entry, int manual_value,
		 uint8_t *vbios_data, uint16_t vbios_length,
		 uint16_t timing_entry_offset, uint8_t entry_length,
		 uint8_t perflvl, int do_mmiotrace)
{
	char outfile[11];
	int i;

	snprintf(outfile, 10, "entry_%i", manual_entry);

	/* TODO: get this filename from the command line */
	FILE *outf = fopen(outfile, "wb");
	if (!outf) {
		perror("Open the output file");
		return 1;
	}
	
	vbios_data[timing_entry_offset + manual_entry]++;
	launch(cnum, outf, vbios_data, vbios_length, perflvl,
		timing_entry_offset, entry_length, do_mmiotrace, manual_entry + 1, 0);

	fclose(outf);

	return 0;
}
