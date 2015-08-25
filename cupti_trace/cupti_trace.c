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

#include "cupti_trace_path.h"
#include "decode_utils.h"
#include "util.h"

static char *
get_cupti_sample_path(const char *name)
{
	const char *cupti_samples_path = getenv("CUPTI_SAMPLES_PATH");
	char forig[1024];
	char *fname;
	FILE *f;

	if (!cupti_samples_path)
		cupti_samples_path = CUPTI_SAMPLES_DEF_PATH;

	snprintf(forig, sizeof(forig), "%s/%s", name, name);
	f = find_in_path(forig, cupti_samples_path, &fname);
	if (!f) {
		fprintf(stderr, "Couldn't find %s CUPTI sample file!\n"
				"Please, make sure it is compiled, or use "
				"the env var CUPTI_SAMPLES_PATH.\n", name);
		return NULL;
	}
	fclose(f);

	return fname;
}

static char *
get_callback_event_path()
{
	return get_cupti_sample_path("callback_event");
}

static char *
get_cupti_query_path()
{
	return get_cupti_sample_path("cupti_query");
}

static int
trace_event(int device_id, const char *chipset, const char *event)
{
	char trace_log[1024], device_str[32];
	char *callback_event_path;
	pid_t pid;
	FILE *f;

	callback_event_path = get_callback_event_path();
	if (!callback_event_path)
		return 1;

	sprintf(trace_log, "%s.trace", event);
	sprintf(device_str, "%d", device_id);

	if ((pid = fork()) < 0) {
		perror("fork");
		return 1;
	}

	if (!(f = fopen(trace_log, "w+"))) {
		perror("fopen");
		return 1;
	}

	if (pid == 0) {
		dup2(fileno(f), 2);

		execlp("valgrind", "valgrind",
		       "--tool=mmt",
		       "--mmt-trace-file=/dev/nvidia0",
		       "--mmt-trace-file=/dev/nvidiactl",
		       "--mmt-trace-nvidia-ioctls",
		       callback_event_path,
		       device_str,
		       event,
		       NULL);

		if (errno == ENOENT) {
			fprintf(stderr, "No such file or directory.\n");
			exit(EXIT_FAILURE);
		}
	} else {
		int status;
		if (waitpid(pid, &status, 0) < 0) {
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
				fprintf(stderr,
					"Failed to execute child process.\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	if (lookup_trace(chipset, event))
		return 1;

	if (fclose(f) < 0) {
		perror("fclose");
		return 1;
	}

	printf("Trace of '%s' saved in file '%s'\n\n", event, trace_log);
	fflush(stdout);

	return 0;
}

static int
get_domains(int **pdomains, int *num_domains)
{
	char cmd[1024], buf[1024];
	char *cupti_query_path;
	int *domains = NULL;
	FILE *f;

	cupti_query_path = get_cupti_query_path();
	if (!cupti_query_path)
		return 1;

	snprintf(cmd, sizeof(cmd), "%s -getdomains", cupti_query_path);

	f = popen(cmd, "r");
	if (!f) {
		perror("popen");
		return 1;
	}

	*num_domains = 0;
	while (fgets(buf, sizeof(buf), f) != NULL) {
		int domain_id;
		char *token;

		token = strstr(buf, "Id         = ");
		if (token) {
			(*num_domains)++;
			domains = realloc(domains, *num_domains * sizeof(int));
			assert(domains != NULL);

			sscanf(token + 13, "%d", &domain_id);
			domains[*num_domains - 1] = domain_id;
		}
	}

	if (pclose(f) < 0) {
		perror("pclose");
		return 1;
	}

	*pdomains = domains;
	return 0;
}

static int
get_events_by_domain(int domain, char ***pevents, int *num_events)
{
	char cmd[1024], buf[1024];
	char *cupti_query_path;
	char **events = NULL;
	FILE *f;

	cupti_query_path = get_cupti_query_path();
	if (!cupti_query_path)
		return 1;

	snprintf(cmd, sizeof(cmd), "%s -domain %d -getevents",
		 cupti_query_path, domain);

	f = popen(cmd, "r");
	if (!f) {
		perror("popen");
		return 1;
	}

	*num_events = 0;
	while (fgets(buf, sizeof(buf), f) != NULL) {
		char event_name[1024];
		char *token;

		token = strstr(buf, "Name      = ");
		if (token) {
			(*num_events)++;
			events = realloc(events, *num_events * sizeof(char *));
			assert(events != NULL);

			sscanf(token + 12, "%s", event_name);

			events[*num_events - 1] = strdup(event_name);
			assert(events[*num_events - 1] != NULL);
		}
	}

	if (pclose(f) < 0) {
		perror("pclose");
		return 1;
	}

	*pevents = events;
	return 0;
}

static int
trace_all_events(int device_id, const char *chipset)
{
	int *domains = NULL;
	int num_domains = 0;
	int i, j;

	if (get_domains(&domains, &num_domains)) {
		fprintf(stderr, "Failed to get list of domains!\n");
		return 1;
	}

	for (i = 0; i < num_domains; i++) {
		char **events = NULL;
		int num_events;

		if (!get_events_by_domain(domains[i], &events, &num_events)) {
			for (j = 0; j < num_events; j++) {
				trace_event(device_id, chipset, events[j]);
				free(events[j]);
			}
			free(events);
		}
	}
	free(domains);
	return 0;
}

static void
usage()
{
	printf("Usage:\n");
	printf("	cupti_trace -a NVXX [-d device_id] [-e event-name]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	char *event = NULL, *chipset = NULL;
	int device_id = 0, use_colors = 1;
	int c;

	if (argc < 3) {
		usage();
		return 1;
	}

	/* Arguments parsing */
	while ((c = getopt(argc, argv, "a:d:e:c")) != -1) {
		switch (c) {
			case 'a':
				chipset = strdup(optarg);
				break;
			case 'c':
				use_colors = 0;
				break;
			case 'd':
				device_id = strtoul(optarg, NULL, 10);
				break;
			case 'e':
				event = strdup(optarg);
				break;
			default:
				usage();
		}
	}

	if (!chipset)
		usage();

	init_rnnctx(chipset, use_colors);

	if (!event) {
		/* Trace all events. */
		if (trace_all_events(device_id, chipset))
			return 1;
	} else {
		/* Trace the specified event. */
		if (trace_event(device_id, chipset, event))
			return 1;
	}

	destroy_rnnctx();

	return 0;
}
