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
#include <stdbool.h>
#include <string.h>
#include <ctype.h>


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


static int line_num = 0;

char *read_script(int (*get_next_byte) (void *), void *arg, size_t *len)
{
  size_t buf_size = 1024;
  size_t cur_size = 0;
  char *buf = (char *)checked_malloc(buf_size * sizeof(char));
  while (true)
  {
    if (cur_size == buf_size)
    {
      buf = checked_grow_alloc(buf, &buf_size);
    }
    int byte = get_next_byte(arg);
    if (byte == EOF)
      break;
    buf[cur_size++] = byte;
  }
  buf[cur_size] = '\0';
  *len = cur_size;
  return buf;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
                     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  command_stream_t stream;
  size_t script_length;
  char *script = read_script(get_next_byte, get_next_byte_argument, &script_length);
  return stream;
}


command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  //error (1, 0, "command reading not yet implemented");
  
  // No more commands
  if (s->command_idx == s->num_commands) {
    return NULL;
  }
  
  command_t comm = s->commands[s->command_idx];
  s->command_idx++;
  
  // Do we need to free anything?
  return comm;
}
