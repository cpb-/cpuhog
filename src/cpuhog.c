/*
 * cpuhog
 *
 * (c) 2020-2025 Christophe BLAESS <christophe.blaess@logilin.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#define _GNU_SOURCE

#include <getopt.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static char *Version       = "1.0.0";
static int   Duration      = 60;
static int   Fifo_priority = 0;
static int   RR_priority   = 0;
static int   Always_yes    = 0;

static int  *Core_list = NULL;
static int   Nb_cores  = 0;

static pthread_barrier_t Pthread_barrier;



static void display_usage(const char *name);
static void display_version(void);
static int  initialize_core_list(const char *name);
static void loop(void);
static int  read_options(int argc, const char *argv[]);
static int  run_loops(const char *name);
static int  run_realtime(const char *name);
static int  scan_core_list(const char *name, const char *pattern);
static void *thread_loop(void *arg);



int main(int argc, const char *argv[])
{
	int ret;

	if (initialize_core_list(argv[0]) != 0)
		return EXIT_FAILURE;

	ret = read_options(argc, argv);
	if (ret < 0)
		return EXIT_FAILURE;
	if (ret > 0)
		return EXIT_SUCCESS;

	ret = run_loops(argv[0]);
	if (ret < 0)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}



static void display_usage(const char *name)
{
	fprintf(stderr, "Usage: %s [options]\n", name);
	fprintf(stderr, " Options:\n");
	fprintf(stderr, "  -c --core <list>          list of core where to run active loops (default: all)\n");
	fprintf(stderr, "  -d --duration <seconds>   duration of the active loops (default: 60 seconds)\n");
	fprintf(stderr, "  -f --fifo <priority>      run loops with realtime Fifo pxriority (dangerous)\n");
	fprintf(stderr, "  -h --help                 this help\n");
	fprintf(stderr, "  -r --rr <priority>        run loops with realtime Round-Robin priority (dangerous)\n");
	fprintf(stderr, "  -v --version              display cpuhog version\n");
	fprintf(stderr, "  -y --yes                  answer 'yes' to all questions\n");
	fprintf(stderr, " \n");
	fprintf(stderr, " Examples:\n");
	fprintf(stderr, " %s -c 0-3,7,11-13  -d 3600\n", name);
	fprintf(stderr, " %s -f 30  -d 10 -y\n", name);
}



static void display_version(void)
{
	fprintf(stderr, "cpuhog - %s\n(c) 2020-2025 Christophe BLAESS\n", Version);
}



static int initialize_core_list(const char *name)
{
	Nb_cores = sysconf(_SC_NPROCESSORS_ONLN);
	Core_list = calloc(Nb_cores, sizeof(int));
	if (Core_list == NULL) {
		fprintf(stderr, "%s: unable to allocate table for %d cores.\n", name, Nb_cores);
		return -1;
	}
	for (int i = 0; i < Nb_cores; i++)
		Core_list[i] = 1;

	return 0;
}



static void loop(void)
{
	volatile long int l = 0;

	time_t end = time(NULL) + Duration;

	while (time(NULL) < end) {
		l = 0;
		while (l < 100000000)
			l ++;
	}
}



static int read_options(int argc, const char *argv[])
{
	static struct option long_options[] = {
		{ "core",     required_argument, NULL, 'c' },
		{ "duration", required_argument, NULL, 'd' },
		{ "fifo",     required_argument, NULL, 'f' },
		{ "help",     no_argument,       NULL, 'h' },
		{ "rr",       required_argument, NULL, 'r' },
		{ "version",  no_argument,       NULL, 'v' },
		{ "yes",      no_argument,       NULL, 'y' },
	};
	int opt;

	while ((opt = getopt_long(argc, (char **)argv, "c:d:f:hrr:vy", long_options, NULL)) >= 0) {
		switch(opt) {
			case 'c':
				if (scan_core_list(argv[0], optarg) != 0)
					return -1;
				break;
			case 'd':
				if ((sscanf(optarg, "%d", &Duration) != 1)
				 || (Duration < 1)) {
					fprintf(stderr, "%s: invalid duration.\n", argv[0]);
					return -1;
				}
				break;
			case 'f':
				if ((sscanf(optarg, "%d", &Fifo_priority) != 1)
				 || (Fifo_priority < sched_get_priority_min(SCHED_FIFO))
				 || (Fifo_priority > sched_get_priority_max(SCHED_FIFO))) {
					fprintf(stderr, "%s: invalid Fifo priority.\n", argv[0]);
					return -1;
				}
				if (RR_priority > 0) {
					fprintf(stderr, "%s can't set Fifo and RR priorities at the same time.\n", argv[0]);
					return -1;
				}
				break;
			case 'h':
				display_usage(argv[0]);
				return 1;
			case 'r':
				if ((sscanf(optarg, "%d", &RR_priority) != 1)
				 || (RR_priority < sched_get_priority_min(SCHED_RR))
				 || (RR_priority > sched_get_priority_max(SCHED_RR))) {
					fprintf(stderr, "%s: invalid RR priority.\n", argv[0]);
					return -1;
				}
				if (Fifo_priority > 0) {
					fprintf(stderr, "%s can't set Fifo and RR priorities at the same time.\n", argv[0]);
					return -1;
				}
				break;
			case 'v':
				display_version();
				return 1;
			case 'y':
				Always_yes = 1;
				break;
			case '?':
				fprintf(stderr, "%s: missing option value.\n", argv[0]);
				return -1;
			default:
				fprintf(stderr, "%s: invalid option.\n", argv[0]);
				return -1;
		}
	}

	if (optind < argc) {
		fprintf(stderr, "%s: unexpected argument after options.\n", argv[0]);
		return -1;
	}

	return 0;
}



static int run_loops(const char *name)
{
	if ((Fifo_priority > 0) || (RR_priority > 0))
		if (run_realtime(name) != 0)
			return -1;

	pthread_t *threads = NULL;
	threads = calloc(Nb_cores, sizeof(pthread_t));
	if (threads == NULL) {
		fprintf(stderr, "%s: unable to allocate %d thread identifiers\n", name, Nb_cores);
		return -1;
	}
	int nb_threads = 0;
	for (long int i = 0; i < Nb_cores; i ++) {
		if (Core_list[i])
			nb_threads ++;
	}
	if (nb_threads == 0)
		return 0;

	pthread_barrier_init(&Pthread_barrier, NULL, nb_threads);

	for (long int i = 0; i < Nb_cores; i ++) {
		if (Core_list[i]) {
			if (pthread_create(&(threads[i]), NULL, thread_loop, (void *)i) != 0) {
				fprintf(stderr, "%s: unable to run thread #%ld\n", name, i);
				return -1;
			}
		}
	}

	for (long int i = 0; i < Nb_cores; i ++) {
		if (Core_list[i]) {
			pthread_join(threads[i], NULL);
		}
	}

	return 0;
}



static int run_realtime(const char *name)
{
	while (! Always_yes) {
		char answer[32];
		fprintf(stderr, "Running a realtime loop may freeze the whole system. Are you sure you want to continue (y/n)? [N] ");
		if (fgets(answer, 31, stdin) == NULL)
			return -1;
		if ((answer[0] == 'Y') || (answer[0] == 'y'))
			break;
		if ((answer[0] == 'N') || (answer[0] == 'n'))
			return -1;
	}

	int ret = 0;

	if (Fifo_priority > 0) {
		struct sched_param param;
		param.sched_priority = Fifo_priority;
		ret = sched_setscheduler(0, SCHED_FIFO, &param);
	} else if (RR_priority > 0) {
		struct sched_param param;
                param.sched_priority = RR_priority;
                ret = sched_setscheduler(0, SCHED_RR, &param);
	}
	if (ret < 0)
		fprintf(stderr, "%s: unable to get realtime priority. Are you root (or did you use `sudo`) ?\n", name);

	return ret;
}



static int scan_core_list(const char *name, const char *pattern)
{
	for (int i = 0; i < Nb_cores; i++)
		Core_list[i] = 0;

	int offset = 0;

	while (pattern[offset] != '\0') {
		int nb;
		int core, core_start, core_end;
		if (sscanf(&(pattern[offset]), "%d-%d%n", &core_start, &core_end, &nb) == 2) {
			offset += nb;
			if ((core_start < 0) || (core_start >= Nb_cores)) {
				fprintf(stderr, "%s: invalid core number: %d\n", name, core_start);
				return -1;
			}
			if ((core_end < 0) || (core_end >= Nb_cores)) {
				fprintf(stderr, "%s: invalid core number: %d\n", name, core_end);
				return -1;
			}
			if (core_end < core_start) {
				core_end   = core_start - core_end;
				core_start = core_start - core_end;
				core_end   = core_start + core_end;
				// ;-)
			}
			for (int i = core_start; i <= core_end; i++)
				Core_list[i] = 1;
			continue;
		} else if (sscanf(&(pattern[offset]), "%d%n", &core, &nb) == 1) {
			offset += nb;
			if ((core < 0) || (core >= Nb_cores)) {
				fprintf(stderr, "%s: invalid core number: %d\n", name, core);
				return -1;
			}
			Core_list[core] = 1;
		} else if (pattern[offset] == ',') {
			offset ++;
		} else {
			fprintf(stderr, "%s: invalid character in core list: %c\n", name, pattern[offset]);
			return -1;
		}
	}

	return 0;
}



static void *thread_loop(void *arg)
{
	cpu_set_t cpu_set;

	CPU_ZERO(& cpu_set);
	CPU_SET((long int)arg, & cpu_set);

	if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set) != 0) {
		fprintf(stderr, "Unable to put a thread on CPU #%ld\n", (long int) arg);
		exit(EXIT_FAILURE);
	}
	pthread_barrier_wait(&Pthread_barrier);
	loop();
	return NULL;
}


