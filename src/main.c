// UCLA CS 111 Lab 1 main program

// Copyright 2012-2014 Paul Eggert.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "command.h"

static char const *program_name;
static char const *script_name;
const double NSECS_PER_SEC = 1000000000;
const double USECS_PER_SEC = 1000000;
bool file_error = false;
int precision_realtime = 0;
int precision_monotonic = 0;


static void
usage (void)
{
  error (1, 0, "usage: %s [-p PROF-FILE | -t] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
get_clock_precision(clockid_t clk_id)
{
  struct timespec res;
  if (clock_getres(clk_id, &res) == -1)
  {
    perror(NULL);
    exit(1);
  }
  if (res.tv_sec)
    return 0;
  if (res.tv_nsec >= 100000000)
    return 1;
  if (res.tv_nsec >= 10000000)
    return 2;
  if (res.tv_nsec >= 1000000)
    return 3;
  if (res.tv_nsec >= 100000)
    return 4;
  if (res.tv_nsec >= 10000)
    return 5;
  if (res.tv_nsec >= 1000)
    return 6;
  if (res.tv_nsec >= 100)
    return 7;
  if (res.tv_nsec >= 10)
    return 8;
  return 9;
}

void
total_rusage(double *user, double *system)
{
    struct rusage selfusage, childusage;
    struct timeval utime, stime;
    if (getrusage(RUSAGE_SELF, &selfusage) == -1)
    {
        perror(NULL);
        _exit(1);
    }
    if (getrusage(RUSAGE_CHILDREN, &childusage) == -1)
    {
        perror(NULL);
        _exit(1);
    }
    timeradd(&(selfusage.ru_utime), &(childusage.ru_utime), &utime);
    timeradd(&(selfusage.ru_stime), &(childusage.ru_stime), &stime);
    *user = utime.tv_sec + (double)utime.tv_usec / USECS_PER_SEC;
    *system = stime.tv_sec + (double)stime.tv_usec / USECS_PER_SEC;
}

int
main (int argc, char **argv)
{
  struct timespec begin_time;
  if(clock_gettime(CLOCK_MONOTONIC, &begin_time) == -1)
  {
      perror(NULL);
      exit(1);
  }
  int command_number = 1;
  bool print_tree = false;
  char const *profile_name = 0;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "p:t"))
      {
      case 'p': profile_name = optarg; break;
      case 't': print_tree = true; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);
  fclose(script_stream);
  int profiling = -1;
  if (profile_name)
    {
      profiling = prepare_profiling (profile_name);
      if (profiling < 0)
        error (1, errno, "%s: cannot open", profile_name);
      precision_realtime = get_clock_precision(CLOCK_REALTIME);
      precision_monotonic = get_clock_precision(CLOCK_MONOTONIC);
    }

  command_t last_command = NULL;
  command_t command;
  while ((command = read_command_stream (command_stream)))
    {
      if (print_tree)
	{
	  printf ("# %d\n", command_number++);
	  print_command (command);
	}
      else
	{
	  last_command = command;
	  execute_command (command, profiling);
	}
    }

  int retval = print_tree || !last_command ? 0 : command_status (last_command);
  free_command_stream(command_stream);
  if (profile_name && !file_error)
  {
    char s[1024];
    struct timespec t;
    clock_getres(CLOCK_REALTIME, &t);
    if(clock_gettime(CLOCK_REALTIME, &t) == -1)
    {
      perror(NULL);
      exit(1);
    }
    double endtime = timespec_to_sec(&t);
    clock_gettime(CLOCK_MONOTONIC, &t);
    struct timespec elapsed = diff(begin_time, t);
    double elapsedtime = timespec_to_sec(&elapsed);
    double utime, stime;
    total_rusage(&utime, &stime);
    pid_t shell_pid = getpid();
    snprintf(s, 1023, "%.*f %.*f %.6f %.6f [%d]\n", precision_realtime, endtime, precision_monotonic, elapsedtime, utime, stime, shell_pid);
    write(profiling, s, strlen(s));
    close(profiling);
  }
  return file_error ? 127 : retval;
}
