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
#include "nvamemtiming.h"

#define	NOUVEAU_TIME_WAIT 60

int vbios_upload_pramin(int cnum, uint8_t *vbios, int length);

enum color {
	NO_COLOR = 0,
	COLOR = 1
};

enum value_type {
	EMPTY,
	VALUE,
	BITFIELD
};

enum value_type nv40_timing_value_types[] = {
	VALUE, VALUE, VALUE, VALUE, EMPTY,
	VALUE, EMPTY, VALUE, EMPTY, VALUE,
	VALUE, VALUE, VALUE, VALUE, BITFIELD,
	VALUE, BITFIELD, VALUE, VALUE, VALUE,
	VALUE, VALUE, VALUE, VALUE, VALUE
};

enum value_type nvc0_timing_value_types[] = {
	VALUE, VALUE, VALUE, VALUE, EMPTY,
	VALUE, EMPTY, VALUE, EMPTY, VALUE,
	VALUE, VALUE, VALUE, VALUE, BITFIELD,
	EMPTY, EMPTY, EMPTY, VALUE, VALUE,
	VALUE, VALUE, EMPTY, EMPTY, VALUE
};

enum value_type *timing_value_types = NULL;

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

static void
mmiotrace_start(int entry)
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

static int
upclock_card(Display *dpy)
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


static int
wait_for_perflvl(Display *dpy, uint8_t wanted_perflvl)
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

static int
force_timing_changes(uint8_t wanted_perflvl, int mmiotrace)
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

static void
print_reg_diff(FILE* outf, uint32_t orig, uint32_t dest, enum color diff)
{
	uint8_t *o = (uint8_t*)&orig;
	uint8_t *d = (uint8_t*)&dest;
	int i;

	for (i = 3; i >= 0; i--) {
		const char* colour = "\e[0;31m";

		if (diff == NO_COLOR || o[i] == d[i])
                        colour = NULL;  

		fprintf(outf, "%s%02x%s", colour?colour:"", d[i], colour?"\e[0m":"");
	}
	fprintf(outf, " ");
}

static void
dump_regs(int cnum, FILE* outf, uint32_t ref_val[], int ref_exist, uint32_t reg, uint32_t regs_len, enum color color)
{
	uint32_t val;
	int r;

	for (r = 0; r < regs_len; r+=4)
        {
                if (r % 0x10 == 0)
                        fprintf(outf, "%08x: ", reg + r);

                val = nva_rd32(cnum, reg + r);

                print_reg_diff(outf, ref_val[r], val, color);

                /* if the value wasn't initialized before, store it as a reference */
                if (ref_exist == 0)
                        ref_val[r] = val;

                if (r % 0x10 == 0xc)
                        fprintf(outf, "\n");
        }
}

static void
dump_timings(struct nvamemtiming_conf *conf, FILE* outf,
			 int8_t progression, enum color color)
{
	static uint32_t ref_val1[0xa0] = { 0 };
	static uint32_t ref_val2[0x10] = { 0 };
	static uint32_t ref_val3[0x10] = { 0 };
	static int ref_exist = 0;
	uint8_t *vbios_entry = conf->vbios.data + conf->vbios.timing_entry_offset;
	int i, r;

	fprintf(outf, "timing entry [%u/%zu]: ", progression, conf->vbios.timing_entry_length);
	for (i = 0; i < conf->vbios.timing_entry_length; i++) {
		if (i != progression - 1)
			fprintf(outf, "%02x ", vbios_entry[i]);
		else
			fprintf(outf, "\e[0;31m%02x\e[0m ", vbios_entry[i]);
	}
	fprintf(outf, "\n");

	if (nva_cards[conf->cnum].card_type >= 0xc0) {
		dump_regs(conf->cnum, outf, ref_val1, ref_exist, 0x10f290, 0xa0, color && progression > 0);
	} else  {
		dump_regs(conf->cnum, outf, ref_val1, ref_exist, 0x100220, 0xd0, color && progression > 0);
		dump_regs(conf->cnum, outf, ref_val2, ref_exist, 0x100510, 0x10, color && progression > 0);
		dump_regs(conf->cnum, outf, ref_val3, ref_exist, 0x100710, 0x10, color && progression > 0);
	}

	fprintf(outf, "\n");
	fflush(outf);

	ref_exist = 1;
}

static int
launch(struct nvamemtiming_conf *conf, FILE *outf, uint8_t progression, enum color color)
{
	printf("--- Start sequence number %u\n", progression);

	if (system("rmmod nvidia") == 127)
	{
		fprintf(stderr, "rmmod not found. Abort\n");
		exit (2);
	}
	
	if (vbios_upload_pramin(conf->cnum, conf->vbios.data, conf->vbios.length) != 1)
	{
		fprintf(stderr, "upload failed. Abort\n");
		exit(3);
	}

	/* modify our env so as we can launch apps on X */
	setenv("DISPLAY", ":0", 1);

	/* monitor downclocking */
	if (conf->mmiotrace)
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
	if (force_timing_changes(conf->timing.perflvl, conf->mmiotrace))
		return 1;

	if (conf->mmiotrace)
                mmiotrace_stop();

	dump_timings(conf, outf, progression, color);

	system("killall X");
	sleep(5);

	return 0;
}

static void
iterate_bitfield(struct nvamemtiming_conf *conf, FILE *outf, uint8_t index, enum color color)
{
	int b;
	for (b = 0; b < 8; b++) {
		conf->vbios.data[conf->vbios.timing_entry_offset + index] = (1 << b);
		launch(conf, outf, index + 1 , COLOR);
	}
}


int
complete_dump(struct nvamemtiming_conf *conf)
{
	int i;

	/* TODO: get this filename from the command line */
	FILE *outf = fopen("regs_timing", "wb");
	if (!outf) {
		perror("Open regs_timing");
		return 1;
	}

	if (nva_cards[conf->cnum].card_type >= 0xc0)
		timing_value_types = nvc0_timing_value_types;
	else
		timing_value_types = nv40_timing_value_types;

	launch(conf, outf, 0, NO_COLOR);

	/* iterate through the vbios timing values */
	for (i = 0; i < conf->vbios.timing_entry_length; i++) {
		uint8_t orig = conf->vbios.data[conf->vbios.timing_entry_offset + i];

		if (timing_value_types[i] == VALUE ||
			(timing_value_types[i] == EMPTY && orig > 0))
		{
			if (timing_value_types[i] == EMPTY && orig > 0)
				fprintf(outf, "WARNING: The following entry was supposed to be unused!\n");

			conf->vbios.data[conf->vbios.timing_entry_offset + i]++;
			launch(conf, outf, i + 1, COLOR);
		} else if (timing_value_types[i] == BITFIELD) {
			iterate_bitfield(conf, outf, i + 1, COLOR);
		} else if (timing_value_types[i] == EMPTY) {
			fprintf(outf, "timing entry [%u/%zu] is supposed empty\n\n",
				i + 1, conf->vbios.timing_entry_length);
		}

		conf->vbios.data[conf->vbios.timing_entry_offset + i] = orig;
	}

	fclose(outf);

	return 0;
}

int bitfield_check(struct nvamemtiming_conf *conf)
{
	char outfile[21];
	int i;

	snprintf(outfile, 20, "entry_b_%i", conf->bitfield.index);

	/* TODO: get this filename from the command line */
	FILE *outf = fopen(outfile, "wb");
	if (!outf) {
		perror("Open the output file");
		return 1;
	}

	launch(conf, outf, 0, NO_COLOR);
	iterate_bitfield(conf, outf, conf->bitfield.index, COLOR);

	fclose(outf);

	return 0;
}

int manual_check(struct nvamemtiming_conf *conf)
{
	char outfile[21];
	int i;

	snprintf(outfile, 20, "entry_%i_%i", conf->manual.index, conf->manual.value);

	/* TODO: get this filename from the command line */
	FILE *outf = fopen(outfile, "wb");
	if (!outf) {
		perror("Open the output file");
		return 1;
	}

	conf->vbios.data[conf->vbios.timing_entry_offset + conf->timing.entry]++;
	launch(conf, outf, 0, NO_COLOR);

	fclose(outf);

	return 0;
}
