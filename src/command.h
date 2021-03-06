// UCLA CS 111 Lab 1 command interface

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


#include <stdbool.h>

 // Forward declarations
enum command_type;
struct timeval;
struct timespec;

typedef struct command *command_t;
typedef struct command_stream *command_stream_t;

/* Read the entire script with GETBYTE and ARG. Return a pointer to the array
 * of chars, and store the length of the array in *LEN */
char *read_script(int (*get_next_byte) (void *), void *arg, unsigned long *len);

/* Build a command between *STARTPOS and ENDPOS, and update STARTPOS to the end
 * of the just-build command */
command_t build_command(char **startpos, char *endpos);
command_t build_if_command(char **startpos, char *endpos);
command_t build_loop_command(char **startpos, char *endpos, enum command_type cmdtype);

/* Functions used for syntax checking */
bool word_at_pos(char *startpos, char *endpos, char *word);
void add_semicolon(char *startpos, char *endpos);
bool syntax_error(char *startpos, char *endpos);
void errorline(char *startpos, char *endpos);
bool cmd_has_bad_syntax(command_t cmd);
char *bad_next_char(char *startpos, char *endpos);
void check_good_char(char *startpos, char *endpos);
void check_after_struct(char *startpos, char *endpos);
bool OK_before_struct(char *front, char *back);


/* Create a command stream from GETBYTE and ARG.  A reader of
   the command stream will invoke GETBYTE (ARG) to get the next byte.
   GETBYTE will return the next input byte, or a negative number
   (setting errno) on failure.  */
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);

/* Used to free a command stream and all associated commands recursively.
 * Users should only call free_command_stream, which uses free_command as
 * a helper function. */
void free_command_stream(command_stream_t stream);
void free_command(command_t cmd);


/* Prepare for profiling to the file FILENAME.  If FILENAME is null or
   cannot be written to, set errno and return -1.  Otherwise, return a
   nonnegative integer flag useful as an argument to
   execute_command.  */
int prepare_profiling (char const *filename);

/* Helper functions used in profiling to keep main code clean */
struct timespec diff(struct timespec first, struct timespec second);
void total_rusage(double *user, double *system);
double timespec_to_sec(struct timespec *ts);
double timeval_to_sec(struct timeval *tv);


/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t);

/* Execute a command.  Use profiling according to the flag; do not profile
   if the flag is negative.  */
void execute_command (command_t, int);

/* Return the exit status of a command, which must have previously
   been executed.  Wait for the command, if it is not already finished.  */
int command_status (command_t);
