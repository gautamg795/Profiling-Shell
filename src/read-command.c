// UCLA CS 111 Lab 1 command reading

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

#include "command.h"
#include "command-internals.h"

#ifdef __APPLE__
#include <err.h>
#define error(x,y,z) errc(x,y,z)
#else
#include <error.h>
#endif
#include "alloc.h"
#include <stdlib.h>
#include <stdio.h>
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
struct command_stream
{
  command_t *commands;
  int command_idx;
  int num_commands;
  int maxsize;
};

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
                     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  command_stream_t stream = (command_stream_t) checked_malloc(sizeof(struct command_stream));
  stream->commands = NULL;
  stream->command_idx = stream->num_commands = stream->maxsize = 0;
  stream->commands = (command_t*) checked_malloc(128 * sizeof(command_t));
  stream->maxsize = 128;
  int byte;
  while ((byte = get_next_byte(get_next_byte_argument)) != EOF)
  {
    // load the commands
  }
//  error (1, 0, "command reading not yet implemented");
  return stream;
}

void
free_command_stream(command_stream_t stream)
{
  if (stream->commands)
    free(stream->commands);
  free(stream);
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}
