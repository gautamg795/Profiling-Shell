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
#define error(args...) errc(args)
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
  size_t command_idx;
  size_t num_commands;
  size_t maxsize;
};

static int linenum = 1;

char *
read_script(int (*get_next_byte) (void *), void *arg, size_t *len)
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
  command_stream_t stream = (command_stream_t)checked_malloc(sizeof(struct command_stream));
  stream->command_idx = stream->num_commands = 0;
  stream->commands = (command_t*)checked_malloc(128 * sizeof(command_t));
  stream->maxsize = 128;
  size_t script_length;
  char *script = read_script(get_next_byte, get_next_byte_argument, &script_length);
  char *start = script;
  char *end = start + script_length;
  while (start < end)
  {
    if (stream->num_commands == stream->maxsize)
    {
      stream->commands = (command_t *)checked_grow_alloc(stream->commands,
                                                         &stream->maxsize);
    }
    command_t cmd = build_command(&start, end);
    if (!cmd)
      continue;
    stream->commands[stream->num_commands++] = cmd;
  }
  return stream;
}

command_t
build_command(char **startpos, char *endpos)
{
  char *front = *startpos;
  while (isspace(*front))
  {
    if (*front == '\n')
      linenum++;
    front++;
  }
  if (front == endpos)
  {
    // no text left
    *startpos = front;
    return NULL;
  }
  if ((endpos - front) >= 2 && front[0] == 'i' && front[1] == 'f' &&
      front[2] == ' ')
  {
    return build_if_command(startpos, endpos);
  }
  else if ((endpos - front) >= 5 && front[0] == 'w' && front[1] == 'h'
           && front[2] == 'i' && front[3] == 'l' && front[4] == 'e'
           && front[5] == ' ')
  {
    return build_while_command(startpos, endpos);
  }
  else if ((endpos - front) >= 5 && front[0] == 'u' && front[1] == 'n'
           && front[2] == 't' && front[3] == 'i' && front[4] == 'l'
           && front[5] == ' ')
  {
    return build_until_command(startpos, endpos);
  }
  char *next_newline = strchr(front, '\n');
  if (!next_newline)
    error(1, 0, "didn't find a newline"); // FIXME: deal with end of file
  // Search for a pipe
  char *pipe = memchr(front, '|', next_newline - front);
  char *left_redir = memchr(front, '<', next_newline - front);
  char *right_redir = memchr(front, '>', next_newline - front);
  
  // TODO: Deal with this shit
  
  if (!pipe && !left_redir && !right_redir)
  {
    for (char* c = front; c != next_newline; c++)
      if (!isalnum(*c) && !strchr("!%+,-./:@^_ ", *c))
        error(1, 0, "Invalid character read on line %d", linenum);
    command_t cmd = (command_t)checked_malloc(sizeof(struct command));
    char **cmdstr = (char**)checked_malloc(sizeof(char*));
    *cmdstr = (char*)checked_malloc((next_newline - front + 1) * sizeof(char));
    strncpy(*cmdstr, front, next_newline - front);
    cmd->type = SIMPLE_COMMAND;
    cmd->u.word = cmdstr;
    cmd->status = -1;
    *startpos = next_newline;
    return cmd;
  }
  err(1, 0, "we should not have made it here");
}

command_t
build_if_command(char **startpos, char *endpos)
{
  return 0;
}

command_t
build_while_command(char **startpos, char *endpos)
{
  return 0;
}

command_t
build_until_command(char **startpos, char *endpos)
{
  return 0;
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
  
  return s->commands[s->command_idx++];
}
