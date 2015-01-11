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

bool
word_at_pos(char *startpos, char *endpos, char *word)
{
  size_t len = strlen(word);
  if (endpos - startpos < len + 1) {
    return false;
  }
  
  for (int i = 0; i < len; i++)
  {
    if (startpos[i] != word[i])
    {
      return false;
    }
    if (i == len - 1)
    {
      // Must have a space or newline following the word to be valid
      return isspace(startpos[len]);
    }
  }
  return false;
}

char *
read_script(int (*get_next_byte) (void *), void *arg, size_t *len)
{
  size_t buf_size = 1024;
  size_t cur_size = 0;
  char *buf = (char *)checked_malloc(buf_size * sizeof(char));
  while (true)
  {
    if (cur_size == buf_size - 2)
    {
      buf = checked_grow_alloc(buf, &buf_size);
    }
    int byte = get_next_byte(arg);
    if (byte == EOF)
      break;
    buf[cur_size++] = byte;
  }
  if (cur_size > 0 && buf[cur_size-1] != '\n')
    buf[cur_size++] = '\n';
  buf[cur_size] = '\0';
  *len = cur_size;
  return buf;
}

// TODO: Free all dynamically allocated memory

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
    if (stream->num_commands == stream->maxsize - 1)
    {
      stream->commands = (command_t *)checked_realloc(stream->commands,
                                                         stream->maxsize * 2 * sizeof(command_t));
      stream->maxsize *= 2;
    }
    command_t cmd = build_command(&start, end);
    if (!cmd)
      continue;
    stream->commands[stream->num_commands++] = cmd;
  }
  free(script);
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
  *startpos = front;

  if (front == endpos)
  {
    // no text left
    return NULL;
  }
  if (word_at_pos(front, endpos, "if"))
  {
    front = *startpos = front+2; //+2 to skip the if
    return build_if_command(startpos, endpos);
  }
  else if (word_at_pos(front, endpos, "while"))
  {
    return build_while_command(startpos, endpos);
  }
  else if (word_at_pos(front, endpos, "until"))
  {
    return build_until_command(startpos, endpos);
  }
  char *endsearch = strchr(front, '\n');
  if (!endsearch || endsearch > endpos)
    endsearch = endpos; // FIXME: deal with end of file
  // Search for a pipe or redirect
  char *pipe = memchr(front, '|', endsearch - front);
  char *left_redir = memchr(front, '<', endsearch - front);
  char *right_redir = memchr(front, '>', endsearch - front);
  
  // TODO: Deal with this shit
  
  // It must be a simple command
  if (!pipe && !left_redir && !right_redir)
  {
    for (char* c = front; c != endsearch; c++)
      if (!isalnum(*c) && !strchr(";!%+,-./:@^_ ", *c))
        error(1, 0, "Invalid character read on line %d", linenum);
    command_t cmd = (command_t)checked_malloc(sizeof(struct command));
    char **cmdstr = (char**)checked_malloc(2 * sizeof(char*));
    cmdstr[1] = 0;
    *cmdstr = (char*)checked_malloc((endsearch - front + 1) * sizeof(char));
    strncpy(*cmdstr, front, endsearch - front);
    (*cmdstr)[endsearch-front] = 0;
    cmd->type = SIMPLE_COMMAND;
    cmd->u.word = cmdstr;
    cmd->status = -1;
    cmd->input = cmd->output = NULL;
    *startpos = endsearch;
    return cmd;
  }
  error(1, 0, "we should not have made it here");
}

command_t
build_if_command(char **startpos, char *endpos)
{
  int numInteriorIfs = 0;
  char *posOfElse = NULL;
  
  char *front = *startpos;
  
  command_t cmd = (command_t)checked_malloc(sizeof(struct command));
  cmd->type = IF_COMMAND;
  cmd->status = -1;
  cmd->input = cmd->output = NULL;
  
  while (front < endpos) // TODO: Veryify < or <= ?
  {
    if (isspace(*front)) {
      front++;
      continue;
    }
    
    // We're done!
    if (word_at_pos(front, endpos, "fi") && numInteriorIfs == 0)
    {
      // No else statement
      if (posOfElse == NULL)
      {
        // Build_command on everything between THEN and FI
        // store resulting command in u.command[1]
        cmd->u.command[1] = build_command(startpos, front);
      }
      else
      {
        // Build_command on everything between THEN and ELSE
        // store resulting command in u.command[1]
        cmd->u.command[1] = build_command(startpos, posOfElse);
        
        // Build_command on everything between ELSE and FI
        // store resulting command in u.command[2]
        *startpos = posOfElse+4; // +4 so that else is not included
        cmd->u.command[2] = build_command(startpos, front);
      }
      // TODO: How do we update startpos to note that we are done with this if?
      *startpos = front+2;
      break;
    }
    
    if (word_at_pos(front, endpos, "if"))
    {
      numInteriorIfs++;
    }
    else if (word_at_pos(front, endpos, "fi"))
    {
      numInteriorIfs--;
    }
    else if (word_at_pos(front, endpos, "then") && numInteriorIfs == 0)
    {
      // Build_command on everything before THEN
      // store resulting command in u.command[0]
      cmd->u.command[0] = build_command(startpos, front);
      front = *startpos = front+4; // +4 to pass over the then
    }
    else if (word_at_pos(front, endpos, "else") && numInteriorIfs == 0)
    {
      posOfElse = front;
      front = front+4; // pass over the else but don't update startpos
    }
    
    front++;
  }
  
  return cmd;
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
  // No more commands
  if (s->command_idx == s->num_commands) {
    return NULL;
  }
  
  return s->commands[s->command_idx++];
}
