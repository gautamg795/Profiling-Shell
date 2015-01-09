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

char *get_one_line(int (*getbyte) (void *), void *arg)
{
  char* str = checked_malloc(32 * sizeof(char));
  int curLen = 0;
  int maxLen = 32;
  int byte;
  while(true)
  {
    if (curLen == maxLen)
    {
      str = (char*)checked_realloc(str, maxLen * 2 * sizeof(char));
      maxLen *= 2;
    }
    byte = getbyte(arg);
    if (byte == EOF || byte == '\n')
    {
      if (byte == EOF && curLen == 0)
        return NULL;
      str[curLen] = 0;
      break;
    }
    if (curLen == 0 && isspace(byte)) // discard leading whitespace
      continue;
    str[curLen++] = byte;
  }
  return str;
}

bool
words_left_on_line(char *line)
{
  for (int i = 0; i < strlen(line); i++)
    if (!isspace(line[i]))
      return true;
  return false;
}

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
  while(1)
  {
    if (stream->num_commands == stream->maxsize)
    {
      stream->commands = (command_t*) checked_realloc(stream->commands,
                                                      (stream->maxsize + 128) * sizeof(command_t));
      stream->maxsize += 128;
    }
    command_t cmd = build_command(get_next_byte, get_next_byte_argument, UNPARSED,
                                  NULL);
    if (cmd == NULL) /* Done reading commands */
    {
      break; //??
    }
      // TODO: Add the command to the stream after we get it
    stream->commands[stream->command_idx] = cmd;
    stream->num_commands++;
  }
  return stream;
}

void
free_command_stream(command_stream_t stream)
{
  if (stream->commands)
    free(stream->commands);
  free(stream);
    // !!!: This isn't good enough.
}

command_t
build_command(int (*getbyte) (void *), void *arg, command_tokenization_state state,
              char *line)
{
  command_t cmd = NULL;
  char *word = NULL;
  if (!line)
  {
    while(true)
    {
      line = get_one_line(getbyte, arg);
      if (!line)
        return NULL;
      if (strlen(line) > 0)
        line_num++; // For syntax error reporting
        break;
    }
  }
  cmd = (command_t)checked_malloc(sizeof(struct command));
  
  
  if (state == UNPARSED)
  {
    word = strtok(line, " ");
    if (!word) // TODO: This shouldn't happen.
      error(1, 0, "attempted parsing empty line in build_command()");
    if (strcmp(word, "if") == 0)
    {
      cmd->type = IF_COMMAND;
      if (words_left_on_line(line+3)) // 3 because if\0
      {
        // No more words on the line, get a new line
        cmd->u.command[0] = build_command(getbyte, arg, THEN, NULL); // DF Changed UNPARSED to THEN
      }
      else // word is a pointer to the next word
      {
        char *newline = (char*)checked_malloc(strlen(line + 3) * sizeof(char));
        strcpy(newline, line+3);
        free(line);
        cmd->u.command[0] = build_command(getbyte, arg, THEN, newline);
      }
      
    }
    else if (strcmp(word, "while") == 0)
    {
      cmd->type = WHILE_COMMAND;
    }
    else if (strcmp(word, "until") == 0)
    {
      cmd->type = UNTIL_COMMAND;
    }
    else
    {
      cmd->type = SIMPLE_COMMAND;
      cmd->u.word = &line;
      return cmd;
    }
  }
  
  else if (state == THEN) {
    // ..what about if you have if if 1 && if 2 then echo hello fi.. bad syntax
    int ct = 0;
    while (ct <= strlen(line) - 4) {
      // Find the then
      if (line[ct] == 't' && line[ct+1] == 'h' && line[ct+2] == 'e' && line[ct+3] == 'n') {
        // Call build_command on everything before the then (the condition)
        // And call build_command on everything after the then
      }
      
    }
  }
  return cmd;
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
