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

void
add_semicolon(char *startpos, char *endpos)
{
  for (char *c = startpos; c <= endpos; c++)
  {
    if (*c == ';')
      return;
    if (*c == '\n')
    {
      *c = ';';
      return;
    }
  }
}

bool
word_at_pos(char *startpos, char *endpos, char *word)
{
  size_t len = strlen(word);
  if (endpos - startpos < len) {
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
      // Must have a space or newline or a semicolon following the word to be valid
      return isspace(startpos[len]) || startpos[len] == ';';
    }
  }
  return false;
}

char *
read_script(int (*get_next_byte) (void *), void *arg, size_t *len)
{
  size_t buf_size = 1024;
  size_t cur_size = 0;
  uint local_linenum = 1;
  char *buf = (char *)checked_malloc(buf_size * sizeof(char));
  char *last_nonspace = 0;
  while (true)
  {
    if (cur_size == buf_size - 2)
    {
      buf = checked_grow_alloc(buf, &buf_size);
    }
    int byte = get_next_byte(arg);
    if (byte == '#')
    {
      while ((byte = get_next_byte(arg)) != '\n')
        continue;
    }
    if (byte == '\n')
    {
      if (last_nonspace && *last_nonspace == ';')
        *last_nonspace = ' ';
      local_linenum++;
    }
    if (byte == EOF)
      break;
    if (!isalnum(byte) && !isspace(byte) && !strchr("!%+,-./:@^_;|<>()",byte))
      error(1, 0, "%u: Syntax error. Unexpected character encountered.", local_linenum);
    if (!isspace(byte))
      last_nonspace = &buf[cur_size];
    buf[cur_size++] = byte;
  }
  if (cur_size > 0 && buf[cur_size-1] != '\n')
    buf[cur_size++] = '\n';
  buf[cur_size] = '\0';
  *len = cur_size;
  for (char *c = buf; c < buf + cur_size; c++)
  {
    if (word_at_pos(c, buf + cur_size, "fi"))
    {
      add_semicolon(c, buf + cur_size);
    }
    else if (word_at_pos(c, buf + cur_size, "done"))
    {
      add_semicolon(c, buf + cur_size);
    }
  }
  return buf;
}

// TODO: Free all dynamically allocated memory
void
free_command_stream(command_stream_t stream)
{
  if (!stream)
    return;
  for (int i = 0; i < stream->num_commands; i++)
    free_command(stream->commands[i]);
  free(stream->commands);
  free(stream);
}

void
free_command(command_t cmd)
{
  if (cmd->type == SIMPLE_COMMAND)
  {
    free(cmd->u.word[0]);
    free(cmd->u.word);
  }
  else
  {
    for (int i = 0; i < 3; i++)
      if (cmd->u.command[i] != NULL)
        free_command(cmd->u.command[i]);
  }
  free(cmd);
  return;
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
    if (stream->num_commands == stream->maxsize - 1)
    {
      stream->commands = (command_t *)checked_realloc(stream->commands,
                                                         stream->maxsize * 2 * sizeof(command_t));
      stream->maxsize *= 2;
    }
    command_t cmd = build_command(&start, end);
    if (!cmd)
      break;
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
  
  char *endsearch = strchr(front, '\n');
  if (!endsearch || endsearch > endpos)
    endsearch = endpos; // FIXME: deal with end of file
  char *original_end = endsearch;
  // If semicolon is at the end of the command/search space, ignore it! Decrease the search space.
  do
  {
    if (front == endsearch)
    {
      // no text left
      return NULL;
    }
    endsearch--;
    if (!isspace(*endsearch) && *endsearch != ';')
      break;
  } while (true);
  
  if (*endsearch != ';')
    endsearch++;
  
  // Search for a semicolon, pipe, or redirect in the new search space
  char *semicolon = NULL;
  int internalIf = 0;
  int internalLoops = 0;
  for (char *c = front; c != endsearch; c++)
  {
    if (word_at_pos(c, endsearch, "if"))
      internalIf++;
    else if (word_at_pos(c, endsearch, "while") || word_at_pos(c, endsearch, "until"))
      internalLoops++;
    else if (word_at_pos(c, endsearch, "fi"))
      internalIf--;
    else if (word_at_pos(c, endsearch, "done"))
      internalLoops--;
    else if (*c == ';')
      if (internalLoops == 0 && internalIf == 0)
      {
        semicolon = c;
        break;
      }
  }
  char *pipe = memchr(front, '|', endsearch - front);
  char *left_redir = memchr(front, '<', endsearch - front);
  char *right_redir = memchr(front, '>', endsearch - front);
  
  command_t cmd = (command_t)checked_malloc(sizeof(struct command));
  cmd->status = -1;
  cmd->input = cmd->output = NULL;
  
  // TODO: Deal with freeing memory
  
  memset(cmd->u.command, 0, 3 * sizeof(command_t)); // zero out the command ptrs
  
  // Always deal with semicolon first (sequence commands)!
  if (semicolon)
  {
    cmd->type = SEQUENCE_COMMAND;
    cmd->u.command[0] = build_command(startpos, semicolon);
    *startpos = semicolon+1; // +1 to get rid of semicolon?
    cmd->u.command[1] = build_command(startpos, endsearch);
    return cmd;
  }
  
  if (word_at_pos(front, endpos, "if"))
  {
    front = *startpos = front+2; //+2 to skip the if
    free(cmd); // we don't need the cmd
    return build_if_command(startpos, endpos);
  }
  else if (word_at_pos(front, endpos, "while"))
  {
    front = *startpos = front+5; //+5 to skip the while
    free(cmd); // we don't need the cmd
    return build_loop_command(startpos, endpos, WHILE_COMMAND);
  }
  else if (word_at_pos(front, endpos, "until"))
  {
    front = *startpos = front+5; //+5 to skip the until
    free(cmd); // we don't need the cmd
    return build_loop_command(startpos, endpos, UNTIL_COMMAND);
  }
  
  // Deal with pipe afterwards
  if (pipe)
  {
    cmd->type = PIPE_COMMAND;
    cmd->u.command[0] = build_command(startpos, pipe);
    *startpos = pipe+1; // +1 to get rid of pipe?
    cmd->u.command[1] = build_command(startpos, endsearch);
    return cmd;
  }
  
  // It must be a simple command
  if (!semicolon && !pipe && !left_redir && !right_redir)
  {
    char **cmdstr = (char**)checked_malloc(2 * sizeof(char*));
    cmdstr[1] = 0;
    *cmdstr = (char*)checked_malloc((endsearch - front + 1) * sizeof(char));
    strncpy(*cmdstr, front, endsearch - front);
    cmdstr[0][endsearch-front] = 0; // add the null byte
    cmd->type = SIMPLE_COMMAND;
    cmd->u.word = cmdstr;
    *startpos = original_end;
    return cmd;
  }
  
  error(1, 0, "We should not have made it here");
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
  memset(cmd->u.command, 0, 3 * sizeof(command_t)); // zero out the command ptrs
  
  while (front < endpos) // TODO: Verify < or <= ?
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
      *startpos = front+3;
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
build_loop_command(char **startpos, char *endpos, enum command_type cmdtype)
{
  int numInteriorLoops = 0;
  
  char *front = *startpos;
  
  command_t cmd = (command_t)checked_malloc(sizeof(struct command));
  cmd->type = cmdtype; // Can be WHILE or UNTIL
  cmd->status = -1;
  cmd->input = cmd->output = NULL;
  memset(cmd->u.command, 0, 3 * sizeof(command_t)); // zero out the command ptrs
  
  while (front < endpos) // TODO: Verify < or <= ?
  {
    if (isspace(*front)) {
      front++;
      continue;
    }
    
    // We're done!
    if (word_at_pos(front, endpos, "done") && numInteriorLoops == 0)
    {
      // Build_command on everything between DO and DONE
      // store resulting command in u.command[1]
      cmd->u.command[1] = build_command(startpos, front);
      
      // TODO: How do we update startpos to note that we are done with this while / until ?
      *startpos = front+5;
      break;
    }
    
    if (word_at_pos(front, endpos, "while") || word_at_pos(front, endpos, "until"))
    {
      numInteriorLoops++;
    }
    else if (word_at_pos(front, endpos, "done"))
    {
      numInteriorLoops--;
    }
    else if (word_at_pos(front, endpos, "do") && numInteriorLoops == 0)
    {
      // Build_command on everything before DO
      // store resulting command in u.command[0]
      cmd->u.command[0] = build_command(startpos, front);
      front = *startpos = front+2; // +2 to pass over the do
    }
    
    front++;
  }
  
  return cmd;
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
