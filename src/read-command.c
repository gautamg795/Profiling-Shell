 // UCLA CS 111 Lab 1 command reading

// Copyright 2012-2014 Gautam Gupta, Dylan Flanders

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

#include <error.h>
#include "alloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>


__attribute__((noreturn))
extern void error(int,int,const char*, ...);

static const char rekd = (char)178;
static unsigned int linenum = 1;

struct command_stream
{
  command_t *commands;
  size_t command_idx;
  size_t num_commands;
  size_t maxsize;
};


void
add_semicolon(char *startpos, char *endpos)
{
  bool found_command = false;
  int internalIf = 0;
  int internalLoops = 0;
  int internalSubshells = 0;

  for (char *c = startpos; c <= endpos; c++)
  {
    if (word_at_pos(c, endpos, "if") && (c == startpos || OK_before_struct(c-1, startpos)))
      internalIf++;
    else if ((word_at_pos(c, endpos, "while") || word_at_pos(c, endpos, "until")) && (c == startpos || OK_before_struct(c-1, startpos)))
      internalLoops++;
    else if (word_at_pos(c, endpos, "fi") && (c == startpos || OK_before_struct(c-1, startpos)))
      internalIf--;
    else if (word_at_pos(c, endpos, "done") && (c == startpos || OK_before_struct(c-1, startpos)))
      internalLoops--;
    else if (*c == '(')
      internalSubshells++;
    else if (*c == ')')
      internalSubshells--;
    
    if (internalLoops == 0 && internalIf == 0 && internalSubshells == 0)
    {
      if (!isspace(*c) && *c != ';')
      {
        found_command = true;
      }
      else if (*c == ';')
      {
        found_command = false;
      }
      else if (*c == '\n' && found_command)
      {
        *c = ';';
      }
    }
  }
  return;
}

bool
word_at_pos(char *startpos, char *endpos, char *word)
{
  long len = strlen(word);
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

void
errorline(char *startpos, char *endpos)
{
  int lines = 0;
  for (char *c = startpos; c <= endpos; c++)
  {
    if (*c == '\n')
      lines++;
    if (*c == rekd)
    {
      linenum = lines;
      return;
    }
  }
}

bool
OK_before_struct(char *front, char *back)
{
  if (front < back)
    return false;
  do
  {
    if (strchr("\n;<>|(",*front))
    {
      return true;
    }
    if (front-2 >= back)
    {
      if (word_at_pos(front-2, front, "do"))
        return true;
    }
    if (front-4 >= back)
    {
      if (word_at_pos(front-4, front, "then") || word_at_pos(front-4, front, "else"))
        return true;
    }
    if (front-5 >= back)
    {
      if (word_at_pos(front-5, front, "while") || word_at_pos(front-5, front, "until"))
        return true;
    }
    if (!isspace(*front))
    {
      return false;
    }
    front--;
  }
  while (front >= back);
  return true;
}

void
check_after_struct(char *startpos, char *endpos)
{
  if (startpos >= endpos)
    error(1, 0, "Syntax error.");
  do
  {
    if (strchr("\n;<>|",*startpos))
      return;
    else if (!isspace(*startpos))
      error(1, 0, "Syntax error.");
    startpos++;
  }
  while (startpos < endpos);
  return;
}

void
check_good_char(char *startpos, char *endpos)
{
  if (startpos >= endpos)
     error(1, 0, "Syntax error.");
  do
  {
    if (isalnum(*startpos) || strchr("!%+,-./:@^_",*startpos))
      return;
    startpos++;
  }
  while (startpos < endpos);
  error(1, 0, "Syntax error.");
}

char *
bad_next_char(char *startpos, char *endpos)
{
  if (startpos >= endpos)
    return NULL;
  
  do
    startpos++;
  while (isspace(*startpos) && startpos <= endpos);
  
  // Found a nullbyte, EOF
  if (!*startpos)
    return startpos;
  
  if (strchr(";|<>",*startpos))
    return startpos;
  
  return NULL;
}

bool
syntax_error(char *startpos, char *endpos)
{

  int parnum = 0;
  int ifnum = 0;
  int loopnum =0;
  int donum = 0;
  int thennum = 0;
  int elsenum = 0;
  
  for (char *c = startpos; c <= endpos; c++)
  {
    if (!isalnum(*c) && !isspace(*c) && !strchr("!%+,-./:@^_;|<>()",*c))
    {
      *c = rekd; // dotted rectangle
      return true;
    }
    
    if (strchr("\n;|<>(",*c))
    {
      char *bad = bad_next_char(c, endpos);
      if (*c != '\n' && *c != ';' && bad == endpos) // Found EOF
      {
        *c = rekd; // dotted rectangle
        return true;
      }
      else if (bad && bad != endpos)
      {
        *bad = rekd; // dotted rectangle
        return true;
      }
    }
    
    if (*c == '(')
    {
      parnum++;
    }
    else if (*c ==')')
    {
      parnum--;
    }
    else if (word_at_pos(c, endpos, "do"))
    {
      if (c == startpos || OK_before_struct(c-1, startpos))
      {
        donum++;
      }
    }
    else if (word_at_pos(c, endpos, "then"))
    {
      if (c == startpos || OK_before_struct(c-1, startpos))
      {
        thennum++;
      }
    }
    else if (word_at_pos(c, endpos, "else"))
    {
      if (c == startpos || OK_before_struct(c-1, startpos))
      {
        elsenum++;
      }
    }
    else if (word_at_pos(c, endpos, "while") || word_at_pos(c, endpos, "until"))
    {
      if (c == startpos || OK_before_struct(c-1, startpos))
      {
        loopnum++;
      }
    }
    else if (word_at_pos(c, endpos, "done"))
    {
      if (c == startpos || OK_before_struct(c-1, startpos))
      {
        check_after_struct(c+4, endpos);
        donum--;
        loopnum--;
      }
    }
    else if (word_at_pos(c, endpos, "if"))
    {
      if (c == startpos || OK_before_struct(c-1, startpos))
      {
        ifnum++;
      }
    }
    else if (word_at_pos(c, endpos, "fi"))
    {
      if (c == startpos || OK_before_struct(c-1, startpos))
      {
        check_after_struct(c+2, endpos);
        elsenum--; // Could be negative
        thennum--;
        ifnum--;
      }
    }
    if (ifnum < 0 || parnum < 0 || loopnum < 0 || donum < 0 || thennum < 0)
    {
      error(1, 0, "Syntax error.");
    }
    if (parnum < 0)
    {
      error(1, 0, "Syntax error.");
    }
  }
  
  if (parnum)
  {
    error(1, 0, "Syntax error: parenthesis");
  }
  else if (ifnum)
  {
    error(1, 0, "Syntax error: too many ifs");
  }
  else if (loopnum)
  {
    error(1, 0, "Syntax error: too many loops");
  }
  else if (donum)
  {
    error(1, 0, "Syntax error: too many dos");
  }
  else if (thennum)
  {
    error(1, 0, "Syntax error: too many thens");
  }
  else if (elsenum > 0)
  {
    error(1, 0, "Syntax error: too many elses");
  }
  
  return false;
}

bool
cmd_has_bad_syntax(command_t cmd)
{
  if (!cmd)
    return false;
  bool bad = false;
  bad = cmd->syntaxErr || bad;
  if (cmd->type != SIMPLE_COMMAND)
  {
    for (int i = 0; i < 3; i++)
    {
      bad = cmd_has_bad_syntax(cmd->u.command[i]) || bad;
    }
  }
  return bad;
}

char *
read_script(int (*get_next_byte) (void *), void *arg, size_t *len)
{
  size_t buf_size = 1024;
  size_t cur_size = 0;
  char *buf = (char *)checked_malloc(buf_size * sizeof(char));
  buf[cur_size++] = '\n';
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
      {
        if (byte == EOF)
          break;
        continue;
      }
    }
    if (byte == EOF)
      break;
    buf[cur_size++] = byte;
  }
  if (cur_size > 0 && buf[cur_size-1] != '\n')
    buf[cur_size++] = '\n';
  buf[cur_size] = '\0';
  if (syntax_error(buf, &buf[cur_size]))
    errorline(buf, &buf[cur_size]);
  *len = cur_size;
  for (char *c = buf; c < &buf[cur_size]; c++)
  {
    if (!isspace(*c))
      last_nonspace = c;
    else if (*c == '\n')
    {
      if (last_nonspace && *last_nonspace == ';')
        *last_nonspace = ' ';
    }
  }
  for (char *c = buf; c < buf + cur_size; c++)
  {
    if (*c == '|')
    {
      char *newline = strchr(c, '\n');
      do
        c++;
      while (isspace(*c)  && c != buf + cur_size);
      if (newline < c)
        *newline = ' ';
    }
    
  }
  return buf;
}

void
free_command_stream(command_stream_t stream)
{
  if (!stream)
    return;
  for (size_t i = 0; i < stream->num_commands; i++)
    free_command(stream->commands[i]);
  free(stream->commands);
  free(stream);
}

void
free_command(command_t cmd)
{
  if (cmd->input)
    free(cmd->input);
  if (cmd->output)
    free(cmd->output);
  if (cmd->type == SIMPLE_COMMAND)
  {
    for (int i = 0; ; i++)
    {
      if (cmd->u.word[i] != NULL)
        free(cmd->u.word[i]);
      else
        break;
    }
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
    if (!start)
      break;
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
    endsearch = endpos; 
  char *original_end = endsearch;
  
  char *rect = memchr(front, rekd, original_end - front);
  if (rect)
  {
    command_t cmd = (command_t)checked_malloc(sizeof(struct command));
    cmd->type = IF_COMMAND;
    memset(cmd->u.command, 0, 3 * sizeof(command_t)); // zero out the command ptrs
    cmd->syntaxErr = true;
    cmd->status = -1;
    cmd->input = cmd->output = NULL;
    *startpos = NULL;
    return cmd;
  }
  
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
  int internalSubshells = 0;
  for (char *c = front; c != endsearch; c++)
  {
    if (word_at_pos(c, endsearch, "if") && (c == front || OK_before_struct(c-1, front)))
      internalIf++;
    else if ((word_at_pos(c, endsearch, "while") || word_at_pos(c, endsearch, "until")) && (c == front || OK_before_struct(c-1, front)))
      internalLoops++;
    else if (word_at_pos(c, endsearch, "fi") && (c == front || OK_before_struct(c-1, front)))
      internalIf--;
    else if (word_at_pos(c, endsearch, "done") && (c == front || OK_before_struct(c-1, front)))
      internalLoops--;
    else if (*c == '(')
      internalSubshells++;
    else if (*c == ')')
      internalSubshells--;
    else if (*c == ';')
      if (internalLoops == 0 && internalIf == 0 && internalSubshells == 0)
      {
        semicolon = c;
        break;
      }
  }
  char *pipe = NULL;
  internalIf = internalLoops = internalSubshells = 0;
  for (char *c = front; c != endsearch; c++)
  {
    if (word_at_pos(c, endsearch, "if") && (c == front || OK_before_struct(c-1, front)))
      internalIf++;
    else if ((word_at_pos(c, endsearch, "while") || word_at_pos(c, endsearch, "until")) && (c == front || OK_before_struct(c-1, front)))
      internalLoops++;
    else if (word_at_pos(c, endsearch, "fi") && (c == front || OK_before_struct(c-1, front)))
      internalIf--;
    else if (word_at_pos(c, endsearch, "done") && (c == front || OK_before_struct(c-1, front)))
      internalLoops--;
    else if (*c == '(')
      internalSubshells++;
    else if (*c == ')')
      internalSubshells--;
    else if (*c == '|')
      if (internalLoops == 0 && internalIf == 0 && internalSubshells == 0)
      {
        pipe = c;
        break;
      }
  }
  
  char *left_redir = NULL;
  internalIf = internalLoops = internalSubshells = 0;
  for (char *c = front; c != endsearch; c++)
  {
    if (word_at_pos(c, endsearch, "if") && (c == front || OK_before_struct(c-1, front)))
      internalIf++;
    else if ((word_at_pos(c, endsearch, "while") || word_at_pos(c, endsearch, "until")) && (c == front || OK_before_struct(c-1, front)))
      internalLoops++;
    else if (word_at_pos(c, endsearch, "fi") && (c == front || OK_before_struct(c-1, front)))
      internalIf--;
    else if (word_at_pos(c, endsearch, "done") && (c == front || OK_before_struct(c-1, front)))
      internalLoops--;
    else if (*c == '(')
      internalSubshells++;
    else if (*c == ')')
      internalSubshells--;
    else if (*c == '<')
      if (internalLoops == 0 && internalIf == 0 && internalSubshells == 0)
      {
        left_redir = c;
        break;
      }
  }

  internalIf = internalLoops = internalSubshells = 0;
  char *right_redir = NULL;
  for (char *c = front; c != endsearch; c++)
  {
    if (word_at_pos(c, endsearch, "if") && (c == front || OK_before_struct(c-1, front)))
    internalIf++;
    else if ((word_at_pos(c, endsearch, "while") || word_at_pos(c, endsearch, "until")) && (c == front || OK_before_struct(c-1, front)))
      internalLoops++;
    else if (word_at_pos(c, endsearch, "fi") && (c == front || OK_before_struct(c-1, front)))
      internalIf--;
    else if (word_at_pos(c, endsearch, "done") && (c == front || OK_before_struct(c-1, front)))
      internalLoops--;
    else if (*c == '(')
      internalSubshells++;
    else if (*c == ')')
      internalSubshells--;
    else if (*c == '>')
      if (internalLoops == 0 && internalIf == 0 && internalSubshells == 0)
      {
        right_redir = c;
        break;
      }
  }
  
  if (left_redir && right_redir && right_redir < left_redir)
    error(1, 0, "Syntax error: redirect operators in wrong order");
  
  internalIf = internalLoops = internalSubshells = 0;
  char *left_paren = NULL;
  for (char *c = front; c != endsearch; c++)
  {
    if (word_at_pos(c, endsearch, "if") && (c == front || OK_before_struct(c-1, front)))
      internalIf++;
    else if ((word_at_pos(c, endsearch, "while") || word_at_pos(c, endsearch, "until")) && (c == front || OK_before_struct(c-1, front)))
      internalLoops++;
    else if (word_at_pos(c, endsearch, "fi") && (c == front || OK_before_struct(c-1, front)))
      internalIf--;
    else if (word_at_pos(c, endsearch, "done") && (c == front || OK_before_struct(c-1, front)))
      internalLoops--;
    else if (*c == '(')
      if (internalLoops == 0 && internalIf == 0)
      {
        left_paren = c;
        break;
      }
  }
  
  char *right_paren = NULL;
  internalIf = internalLoops = internalSubshells = 0;
  if (left_paren) {
    for (char *c = left_paren; c != endpos; c++)
    {
      if (word_at_pos(c, endsearch, "if") && (c == front || OK_before_struct(c-1, front)))
        internalIf++;
      else if ((word_at_pos(c, endsearch, "while") || word_at_pos(c, endsearch, "until")) && (c == front || OK_before_struct(c-1, front)))
        internalLoops++;
      else if (word_at_pos(c, endsearch, "fi") && (c == front || OK_before_struct(c-1, front)))
        internalIf--;
      else if (word_at_pos(c, endsearch, "done") && (c == front || OK_before_struct(c-1, front)))
        internalLoops--;
      else if (*c == '(')
        internalSubshells++;
      else if (*c == ')')
        internalSubshells--;
      
      if (*c == ')' && internalSubshells == 0)
      {
        right_paren = c;
        break;
      }
    }
  }
  
  command_t cmd = (command_t)checked_malloc(sizeof(struct command));
  cmd->syntaxErr = false;
  cmd->status = -1;
  cmd->input = cmd->output = NULL;
  
  memset(cmd->u.command, 0, 3 * sizeof(command_t)); // zero out the command ptrs
  
  // Always deal with semicolon first (sequence commands)!
  if (semicolon)
  {
    cmd->type = SEQUENCE_COMMAND;
    cmd->u.command[0] = build_command(startpos, semicolon);
    *startpos = semicolon+1; // +1 to get rid of semicolon?
    cmd->u.command[1] = build_command(startpos, endpos);
    return cmd;
  }
  
  // Deal with pipe afterwards
  if (pipe)
  {
    cmd->type = PIPE_COMMAND;
    cmd->u.command[0] = build_command(startpos, pipe);
    *startpos = pipe+1; // +1 to get rid of pipe?
    cmd->u.command[1] = build_command(startpos, endpos);
    return cmd;
  }
  
  // Deal with subshells next
  if (left_paren)
  {
    if (right_paren - left_paren < 2)
      error(1, 0, "Syntax error: adjacent parentheses");
    cmd->type = SUBSHELL_COMMAND;
    *startpos = left_paren+1;
    add_semicolon(*startpos, right_paren);
    cmd->u.command[0] = build_command(startpos, right_paren);
    *startpos = right_paren + 1;
    char *left_redir = 0, *right_redir = 0;
    char *endsearch;
    for (endsearch = *startpos; ; endsearch++)
    {
      if (*endsearch == '\0' || *endsearch == ';' || *endsearch == '\n')
        break;
      if (*endsearch == '>')
      {
        right_redir = endsearch;
      }
      if (*endsearch == '<')
        left_redir = endsearch;
    }
    
    char *actual_endsearch = endsearch;
    if (right_redir)
    {
      char *original_endsearch = endsearch;
      endsearch = right_redir;
      do
        endsearch--;
      while (isspace(*endsearch));
      endsearch++;
      char *redir_pos = right_redir;
      do
        redir_pos++;
      while (isspace(*redir_pos));
      if (original_endsearch - redir_pos < 0)
        error(1, 0, "Syntax error: bad redirect");
      cmd->output = (char *)checked_malloc((original_endsearch - redir_pos + 1) * sizeof(char));
      memcpy(cmd->output, redir_pos, original_endsearch - redir_pos);
      cmd->output[original_endsearch - redir_pos] = '\0';
    }
    if (left_redir)
    {
      char *original_endsearch = endsearch;
      endsearch = left_redir;
      do
        endsearch--;
      while (isspace(*endsearch));
      endsearch++;
      do
        left_redir++;
      while (isspace(*left_redir));
      if (right_redir)
      {
        if (right_redir - left_redir < 0)
          error(1, 0, "Syntax error: bad redirect");
        cmd->input = (char *)checked_malloc((right_redir - left_redir + 1) * sizeof(char));
        char *end = right_redir;
        do
          end--;
        while (isspace(*end));
        if (end - left_redir + 1 < 0)
          error(1, 0, "Syntax error: bad redirect");
        memcpy(cmd->input, left_redir, end - left_redir + 1);
        cmd->input[end - left_redir] = '\0';
      }
      else
      {
        if (original_endsearch - left_redir < 0)
          error(1, 0, "Syntax error: bad redirect");
        cmd->input = (char *)checked_malloc((original_endsearch - left_redir + 2) * sizeof(char));
        memcpy(cmd->input, left_redir, original_endsearch - left_redir);
        cmd->input[original_endsearch - left_redir] = '\0';
      }
    }
    *startpos = actual_endsearch;
    return cmd;
  }
  
  if (word_at_pos(front, endpos, "if"))
  {
    *startpos = front+2; //+2 to skip the if
    free(cmd); // we don't need the cmd
    cmd = build_if_command(startpos, endpos);
    return cmd;
  }
  else if (word_at_pos(front, endpos, "while"))
  {
    *startpos = front+5; //+5 to skip the while
    free(cmd); // we don't need the cmd
    cmd = build_loop_command(startpos, endpos, WHILE_COMMAND);
    return cmd;
  }
  else if (word_at_pos(front, endpos, "until"))
  {
    *startpos = front+5; //+5 to skip the until
    free(cmd); // we don't need the cmd
    cmd = build_loop_command(startpos, endpos, UNTIL_COMMAND);
    return cmd;
  }
  
  
  // It must be a simple command
  if (!semicolon && !pipe && !left_paren)
  {
    if (right_redir)
    {
      char *original_endsearch = endsearch;
      endsearch = right_redir;
      do
        endsearch--;
      while (isspace(*endsearch));
      endsearch++;
      char *redir_pos = right_redir;
      do
        redir_pos++;
      while (isspace(*redir_pos));
      if (original_endsearch - redir_pos < 0)
        error(1, 0, "Syntax error: bad redirect");
      cmd->output = (char *)checked_malloc((original_endsearch - redir_pos + 1) * sizeof(char));
      memcpy(cmd->output, redir_pos, original_endsearch - redir_pos);
      cmd->output[original_endsearch - redir_pos] = '\0';
    }
    if (left_redir)
    {
      char *original_endsearch = endsearch;
      endsearch = left_redir;
      do
        endsearch--;
      while (isspace(*endsearch));
      endsearch++;
      do
        left_redir++;
      while (isspace(*left_redir));
      if (right_redir)
      {
        if (right_redir - left_redir < 0)
          error(1, 0, "Syntax error: bad redirect");
        cmd->input = (char *)checked_malloc((right_redir - left_redir + 1) * sizeof(char));
        char *end = right_redir;
        do
          end--;
        while (isspace(*end));
        if (end - left_redir + 1 < 0)
          error(1, 0, "Syntax error: bad redirect");
        memcpy(cmd->input, left_redir, end - left_redir + 1);
        cmd->input[end - left_redir + 1] = '\0';
      }
      else // no right_redir
      {
        if (original_endsearch - left_redir < 0)
          error(1, 0, "Syntax error: bad redirect");
        cmd->input = (char *)checked_malloc((original_endsearch - left_redir + 2) * sizeof(char));
        memcpy(cmd->input, left_redir, original_endsearch - left_redir);
        cmd->input[original_endsearch - left_redir] = '\0';
      }
    }
    if (endsearch - front <= 0)
      error(1, 0, "We tried to create a simple command with < 1 character");
    char *untokenized = (char *)checked_malloc((endsearch - front + 1) * sizeof(char));
    strncpy(untokenized, front, endsearch - front);
    untokenized[endsearch-front] = '\0';
    size_t maxWords = 8;
    size_t numWords = 0;
    char **cmdstr = (char**)checked_malloc(maxWords * sizeof(char*));
    char *tok = strtok(untokenized, " ");
    while (tok != NULL)
    {
      if (numWords == maxWords - 1)
      {
        maxWords *= 2;
        cmdstr = (char **)checked_realloc(cmdstr, maxWords * sizeof(char *));
      }
      cmdstr[numWords] = (char *)checked_malloc((strlen(tok) + 1) * sizeof(char));
      strcpy(cmdstr[numWords++], tok);
      tok = strtok(NULL, " ");
    }
    free(untokenized);
    cmdstr[numWords] = NULL;
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
  bool foundThen = false;
  char *posOfElse = NULL;
  
  char *front = *startpos;
  
  command_t cmd = (command_t)checked_malloc(sizeof(struct command));
  cmd->type = IF_COMMAND;
  cmd->syntaxErr = false;
  cmd->status = -1;
  cmd->input = cmd->output = NULL;
  memset(cmd->u.command, 0, 3 * sizeof(command_t)); // zero out the command ptrs
  
  while (front < endpos) 
  {
    if (isspace(*front)) {
      front++;
      continue;
    }
    
    // We're done!
    if (word_at_pos(front, endpos, "fi") && numInteriorIfs == 0 && (front == *startpos || OK_before_struct(front-1, *startpos)))
    {
      // No else statement
      if (posOfElse == NULL)
      {
        check_good_char(*startpos, front);
        // Build_command on everything between THEN and FI
        // store resulting command in u.command[1]
        add_semicolon(*startpos, front);
        cmd->u.command[1] = build_command(startpos, front);
      }
      else
      {
        check_good_char(*startpos, posOfElse);
        // Build_command on everything between THEN and ELSE
        // store resulting command in u.command[1]
        add_semicolon(*startpos, posOfElse);
        cmd->u.command[1] = build_command(startpos, posOfElse);
        
        // Build_command on everything between ELSE and FI
        // store resulting command in u.command[2]
        *startpos = posOfElse+4; // +4 so that else is not included
        check_good_char(*startpos, front);
        add_semicolon(*startpos, front);
        cmd->u.command[2] = build_command(startpos, front);
      }
      *startpos = front+2;
      char *left_redir = 0, *right_redir = 0;
      char *actual_endsearch;
      char *endsearch;
      for (endsearch = *startpos; ; endsearch++)
      {
        if (*endsearch == '\0' || *endsearch == ';' || *endsearch == '\n')
          break;
        if (*endsearch == '>')
        {
          right_redir = endsearch;
        }
        if (*endsearch == '<')
          left_redir = endsearch;
      }
      actual_endsearch = endsearch;
      char *rect = memchr(front, rekd, actual_endsearch - front);
      if (rect)
      {
        cmd->syntaxErr = true;
        *startpos = NULL;
        return cmd;
      }
      if (left_redir && right_redir && right_redir < left_redir)
        error(1, 0, "Syntax error: redirect operators in wrong order");
      if (right_redir)
      {
        char *original_endsearch = endsearch;
        endsearch = right_redir;
        do
          endsearch--;
        while (isspace(*endsearch));
        endsearch++;
        char *redir_pos = right_redir;
        do
          redir_pos++;
        while (isspace(*redir_pos));
        if (original_endsearch - redir_pos < 0)
          error(1, 0, "Syntax error: bad redirect");
        cmd->output = (char *)checked_malloc((original_endsearch - redir_pos + 1) * sizeof(char));
        memcpy(cmd->output, redir_pos, original_endsearch - redir_pos);
        cmd->output[original_endsearch - redir_pos] = '\0';
      }
      if (left_redir)
      {
        char *original_endsearch = endsearch;
        endsearch = left_redir;
        do
          endsearch--;
        while (isspace(*endsearch));
        endsearch++;
        do
          left_redir++;
        while (isspace(*left_redir));
        if (right_redir)
        {
          if (right_redir - left_redir < 0)
            error(1, 0, "Syntax error: bad redirect");
          cmd->input = (char *)checked_malloc((right_redir - left_redir + 1) * sizeof(char));
          char *end = right_redir;
          do
            end--;
          while (isspace(*end));
          memcpy(cmd->input, left_redir, end - left_redir + 1);
          cmd->input[end - left_redir + 1] = '\0';
        }
        else
        {
          if (original_endsearch - left_redir < 0)
            error(1, 0, "Syntax error: bad redirect");
          cmd->input = (char *)checked_malloc((original_endsearch - left_redir + 2) * sizeof(char));
          memcpy(cmd->input, left_redir, original_endsearch - left_redir);
          cmd->input[original_endsearch - left_redir] = '\0';
        }
      }
      if (*actual_endsearch == ';')
        actual_endsearch++;
      *startpos = actual_endsearch;
      break;
    }
    
    if (word_at_pos(front, endpos, "if") && (front == *startpos || OK_before_struct(front-1, *startpos)))
    {
      numInteriorIfs++;
    }
    else if (word_at_pos(front, endpos, "fi") && (front == *startpos || OK_before_struct(front-1, *startpos)))
    {
      numInteriorIfs--;
    }
    else if (numInteriorIfs == 0 && word_at_pos(front, endpos, "then") && (front == *startpos || OK_before_struct(front-1, *startpos)))
    {
      foundThen = true;
      check_good_char(*startpos, front);
      // Build_command on everything before THEN
      // store resulting command in u.command[0]
      add_semicolon(*startpos, front);
      cmd->u.command[0] = build_command(startpos, front);
      front = *startpos = front+4; // +4 to pass over the then
    }
    else if (numInteriorIfs == 0 && word_at_pos(front, endpos, "else") && (front == *startpos || OK_before_struct(front-1, *startpos)))
    {
      if (!foundThen)
        error(1, 0, "Error in if statement.");
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
  cmd->syntaxErr = false;
  cmd->status = -1;
  cmd->input = cmd->output = NULL;
  memset(cmd->u.command, 0, 3 * sizeof(command_t)); // zero out the command ptrs
  
  while (front < endpos) 
  {
    if (isspace(*front)) {
      front++;
      continue;
    }
    
    // We're done!
    if (word_at_pos(front, endpos, "done") && numInteriorLoops == 0 && (front == *startpos || OK_before_struct(front-1, *startpos)))
    {
      check_good_char(*startpos, front);
      // Build_command on everything between DO and DONE
      // store resulting command in u.command[1]
      add_semicolon(*startpos, front);
      cmd->u.command[1] = build_command(startpos, front);
      *startpos = front+4;
      char *left_redir = 0, *right_redir = 0;
      char *endsearch;
      for (endsearch = *startpos; ; endsearch++)
      {
        if (*endsearch == '\0' || *endsearch == ';' || *endsearch == '\n')
          break;
         if (*endsearch == '>')
         {
           right_redir = endsearch;
         }
        if (*endsearch == '<')
          left_redir = endsearch;
      }
      char *actual_endsearch = endsearch;
      if (left_redir && right_redir && right_redir < left_redir)
        error(1, 0, "Syntax error: redirect operators in wrong order");
      if (right_redir)
      {
        for (char *c = right_redir; c <= actual_endsearch; c++)
          if (*c == rekd)
            error(1, 0, "Syntax error: bad redirect");
        char *original_endsearch = endsearch;
        endsearch = right_redir;
        do
          endsearch--;
        while (isspace(*endsearch));
        endsearch++;
        char *redir_pos = right_redir;
        do
          redir_pos++;
        while (isspace(*redir_pos));
        if (original_endsearch - redir_pos < 0)
          error(1, 0, "Syntax error: bad redirect");
        cmd->output = (char *)checked_malloc((original_endsearch - redir_pos + 1) * sizeof(char));
        memcpy(cmd->output, redir_pos, original_endsearch - redir_pos);
        cmd->output[original_endsearch - redir_pos] = '\0';
      }
      if (left_redir)
      {
        for (char *c = left_redir; c <= endsearch; c++)
          if (*c == rekd)
            error(1, 0, "Syntax error: bad redirect");
        char *original_endsearch = endsearch;
        endsearch = left_redir;
        do
          endsearch--;
        while (isspace(*endsearch));
        endsearch++;
        do
          left_redir++;
        while (isspace(*left_redir));
        if (right_redir)
        {
          for (char *c = left_redir; c <= right_redir; c++)
            if (*c == rekd)
              error(1, 0, "Syntax error: bad redirect");
          if (right_redir - left_redir < 0)
            error(1, 0, "Syntax error: bad redirect");
          cmd->input = (char *)checked_malloc((right_redir - left_redir + 1) * sizeof(char));
          char *end = right_redir;
          do
            end--;
          while (isspace(*end));
          if (end - left_redir + 1 < 0)
            error(1, 0, "Syntax error: bad redirect");
          memcpy(cmd->input, left_redir, end - left_redir + 1);
          cmd->input[end - left_redir + 1] = '\0';
        }
        else
        {
          if (original_endsearch - left_redir < 0)
            error(1, 0, "Syntax error: bad redirect");
          cmd->input = (char *)checked_malloc((original_endsearch - left_redir + 2) * sizeof(char));
          memcpy(cmd->input, left_redir, original_endsearch - left_redir);
          cmd->input[original_endsearch - left_redir] = '\0';
        }
      }
      if (*actual_endsearch == ';')
        actual_endsearch++;
      *startpos = actual_endsearch;
      break;
    }
    
    if ((word_at_pos(front, endpos, "while") || word_at_pos(front, endpos, "until")) && (front == *startpos || OK_before_struct(front-1, *startpos)))
    {
      numInteriorLoops++;
    }
    else if (word_at_pos(front, endpos, "done") && (front == *startpos || OK_before_struct(front-1, *startpos)))
    {
      numInteriorLoops--;
    }
    else if (word_at_pos(front, endpos, "do") && numInteriorLoops == 0 && (front == *startpos || OK_before_struct(front-1, *startpos)))
    {
      check_good_char(*startpos, front);
      // Build_command on everything before DO
      // store resulting command in u.command[0]
      add_semicolon(*startpos, front);
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
  
  if (cmd_has_bad_syntax(s->commands[s->command_idx])) {
    error(1, 0, "%u: Syntax error.", linenum);
  }
  
  return s->commands[s->command_idx++];
}
