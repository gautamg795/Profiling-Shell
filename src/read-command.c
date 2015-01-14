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
__attribute__((noreturn))
extern void error(int,int,const char*, ...);
#endif
#include "alloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

static const char rekd = (char)178;

struct command_stream
{
  command_t *commands;
  size_t command_idx;
  size_t num_commands;
  size_t maxsize;
};

static unsigned int linenum = 1;

void
add_semicolon(char *startpos, char *endpos)
{
  bool found_command = false;
  int internalIf = 0;
  int internalLoops = 0;
  int internalSubshells = 0;

  for (char *c = startpos; c <= endpos; c++)
  {
    if (word_at_pos(c, endpos, "if"))
      internalIf++;
    else if (word_at_pos(c, endpos, "while") || word_at_pos(c, endpos, "until"))
      internalLoops++;
    else if (word_at_pos(c, endpos, "fi"))
      internalIf--;
    else if (word_at_pos(c, endpos, "done"))
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
  int ifnum = 0;
  int parnum = 0;
  int loopnum =0;
  int donum = 0;
  int thennum = 0;
  int elsenum = 0;
  char *last_open_paren = NULL;
  char *last_if = NULL;
  char *last_loop = NULL;
  char *last_do = NULL;
  char *last_then = NULL;
  char *last_else = NULL;
  
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
      last_open_paren = c;
      parnum++;
    }
    else if (*c ==')')
    {
      parnum--;
    }
    else if (word_at_pos(c, endpos, "do"))
    {
      last_do = c;
      donum++;
    }
    else if (word_at_pos(c, endpos, "then"))
    {
      last_then = c;
      thennum++;
    }
    else if (word_at_pos(c, endpos, "else"))
    {
      last_else = c;
      elsenum++;
    }
    else if (word_at_pos(c, endpos, "while") || word_at_pos(c, endpos, "until"))
    {
      last_loop = c;
      loopnum++;
    }
    else if (word_at_pos(c, endpos, "done"))
    {
      donum--;
      loopnum--;
    }
    else if (word_at_pos(c, endpos, "if"))
    {
      last_if = c;
      ifnum++;
    }
    else if (word_at_pos(c, endpos, "fi"))
    {
      elsenum--; // Could be negative
      thennum--;
      ifnum--;
    }
    if (ifnum < 0 || parnum < 0 || loopnum < 0 || donum < 0 || thennum < 0)
    {
      *c = rekd; // dotted rectangle
      return true;
    }
  }
  
  if (parnum)
  {
    *last_open_paren = rekd; // dotted rectangle
    return true;
  }
  else if (ifnum)
  {
    *last_if = rekd; // dotted rectangle
    return true;
  }
  else if (loopnum)
  {
    *last_loop = rekd; // dotted rectangle
    return true;
  }
  else if (donum)
  {
    *last_do = rekd; // dotted rectangle
    return true;
  }
  else if (thennum)
  {
    *last_then = rekd; // dotted rectangle
    return true;
  }
  else if (elsenum > 0)
  {
    *last_else = rekd; // dotted rectangle
    return true;
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
        continue;
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
  return buf;
}

// TODO: Free all dynamically allocated memory
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
    endsearch = endpos; // FIXME: deal with end of file
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
    if (word_at_pos(c, endsearch, "if"))
      internalIf++;
    else if (word_at_pos(c, endsearch, "while") || word_at_pos(c, endsearch, "until"))
      internalLoops++;
    else if (word_at_pos(c, endsearch, "fi"))
      internalIf--;
    else if (word_at_pos(c, endsearch, "done"))
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
    if (word_at_pos(c, endsearch, "if"))
      internalIf++;
    else if (word_at_pos(c, endsearch, "while") || word_at_pos(c, endsearch, "until"))
      internalLoops++;
    else if (word_at_pos(c, endsearch, "fi"))
      internalIf--;
    else if (word_at_pos(c, endsearch, "done"))
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

  char *left_redir = memchr(front, '<', endsearch - front);
  char *right_redir = memchr(front, '>', endsearch - front);
  if (left_redir && right_redir && right_redir < left_redir)
    error(1, 0, "Error in redirection");
  char *left_paren = memchr(front, '(', endsearch - front);
  char *right_paren = NULL;
  
  internalSubshells = 0;
  if (left_paren) {
    for (char *c = left_paren; c != endpos; c++)
    {
      if (*c == '(')
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
  
  // Deal with pipe afterwards
  if (pipe)
  {
    cmd->type = PIPE_COMMAND;
    cmd->u.command[0] = build_command(startpos, pipe);
    *startpos = pipe+1; // +1 to get rid of pipe?
    cmd->u.command[1] = build_command(startpos, endsearch);
    return cmd;
  }
  
  // Deal with subshells next
  if (left_paren)
  {
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
        cmd->input = (char *)checked_malloc((right_redir - left_redir + 1) * sizeof(char));
        char *end = right_redir;
        do
          end--;
        while (isspace(*end));
        memcpy(cmd->input, left_redir, end - left_redir + 1);
        cmd->input[end - left_redir] = '\0';
      }
      else
      {
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
        cmd->input = (char *)checked_malloc((right_redir - left_redir + 1) * sizeof(char));
        char *end = right_redir;
        do
          end--;
        while (isspace(*end));
        memcpy(cmd->input, left_redir, end - left_redir + 1);
        cmd->input[end - left_redir + 1] = '\0';
      }
      else // no right_redir
      {
        cmd->input = (char *)checked_malloc((original_endsearch - left_redir + 2) * sizeof(char));
        memcpy(cmd->input, left_redir, original_endsearch - left_redir);
        cmd->input[original_endsearch - left_redir] = '\0';
      }
    }
    
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
  cmd->syntaxErr = false;
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
        add_semicolon(*startpos, front);
        cmd->u.command[1] = build_command(startpos, front);
      }
      else
      {
        // Build_command on everything between THEN and ELSE
        // store resulting command in u.command[1]
        add_semicolon(*startpos, posOfElse);
        cmd->u.command[1] = build_command(startpos, posOfElse);
        
        // Build_command on everything between ELSE and FI
        // store resulting command in u.command[2]
        *startpos = posOfElse+4; // +4 so that else is not included
        add_semicolon(*startpos, front);
        cmd->u.command[2] = build_command(startpos, front);
      }
      // TODO: How do we update startpos to note that we are done with this if?
      *startpos = front+3;
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
          cmd->input = (char *)checked_malloc((original_endsearch - left_redir + 2) * sizeof(char));
          memcpy(cmd->input, left_redir, original_endsearch - left_redir);
          cmd->input[original_endsearch - left_redir] = '\0';
        }
      }
      *startpos = actual_endsearch;
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
      add_semicolon(*startpos, front);
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
  cmd->syntaxErr = false;
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
      add_semicolon(*startpos, front);
      cmd->u.command[1] = build_command(startpos, front);
      // TODO: How do we update startpos to note that we are done with this while / until ?
      *startpos = front+5;
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
          cmd->input = (char *)checked_malloc((right_redir - left_redir + 1) * sizeof(char));
          char *end = right_redir;
          do
            end--;
          while (isspace(*end));
          memcpy(cmd->input, left_redir, end - left_redir + 1);
          cmd->input[end - left_redir] = '\0';
        }
        else
        {
          cmd->input = (char *)checked_malloc((original_endsearch - left_redir + 2) * sizeof(char));
          memcpy(cmd->input, left_redir, original_endsearch - left_redir);
          cmd->input[original_endsearch - left_redir] = '\0';
        }
      }
      *startpos = actual_endsearch;
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
  
  // TODO: FIX LINENUM!!!
  if (cmd_has_bad_syntax(s->commands[s->command_idx])) {
    error(1, 0, "%u: Syntax error.", linenum);
  }
  
  return s->commands[s->command_idx++];
}
