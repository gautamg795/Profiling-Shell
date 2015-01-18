// UCLA CS 111 Lab 1 command execution

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
#include <unistd.h>
#include <sys/wait.h>
#include "alloc.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
prepare_profiling (char const *name)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  error (0, 0, "warning: profiling not yet implemented");
  return -1;
}

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c, int profiling)
{
  if(!c)
  {
    error(1, 0, "We tried ot execute a NULL command");
  }
  int stdin_backup = dup(STDIN_FILENO);
  int stdout_backup = dup(STDOUT_FILENO);
  if (c->input)
  {
    int fd = open(c->input, O_RDONLY);
    if (fd < 0)
    {
      perror(c->input);
      _exit(1);
    }
    dup2(fd, STDIN_FILENO);
  }
  if (c->output)
  {
    int fd = open(c->output, O_WRONLY | O_CREAT, 0644);
    if (fd < 0)
    {
      perror(c->output);
      _exit(1);
    }
    dup2(fd, STDOUT_FILENO);
  }
  switch(c->type)
  {
    case IF_COMMAND:
    {
      execute_command(c->u.command[0], profiling);
      if (! c->u.command[0]->status) // if command returned 0
      {
        execute_command(c->u.command[1], profiling);
        c->status = c->u.command[1]->status;
      }
      else if (c->u.command[2])
      {
        execute_command(c->u.command[2], profiling);
        c->status = c->u.command[2]->status;
      }
      break;
    }
    case WHILE_COMMAND:
    {
      while (true)
      {
        execute_command(c->u.command[0], profiling);
        if (! c->u.command[0]->status) // if command returned 0
        {
          execute_command(c->u.command[1], profiling);
          c->status = c->u.command[1]->status;
        }
        else break;
      }
      break;
    }
    case UNTIL_COMMAND:
    {
      while (true)
      {
        execute_command(c->u.command[0], profiling);
        if (c->u.command[0]->status) // if command returned nonzero
        {
          execute_command(c->u.command[1], profiling);
          c->status = c->u.command[1]->status;
        }
        else break;
      }
      break;
    }
    case PIPE_COMMAND:
    {
      int pipefd[2];
      if (pipe(pipefd) == -1)
        perror(NULL);
      pid_t left, right;
      int status;
      left = fork();
      if (!left) // child
      {
        close(pipefd[0]); // close read end
        dup2(pipefd[1], STDOUT_FILENO);
        execute_command(c->u.command[0], profiling);
        _exit(c->u.command[0]->status);
      }
      else
      {
        right = fork();
        if (!right) // child 2
        {
          close(pipefd[1]); // close write end
          dup2(pipefd[0], STDIN_FILENO);
          execute_command(c->u.command[1], profiling);
          _exit(c->u.command[1]->status);
        }
        else
        {
          waitpid(left, &status, 0);
          close(pipefd[1]); // After left command has ended, close the write end
                            // to signal EOF to the right command
          waitpid(right, &status, 0);
          close(pipefd[0]);
          c->status = WEXITSTATUS(status); // We only care about the exit status
                                           // of the right side command
        }
      }
      break;
    }
    case SEQUENCE_COMMAND:
    {
      execute_command(c->u.command[0], profiling);
      execute_command(c->u.command[1], profiling);
      c->status = c->u.command[1]->status;
      break;
    }
    case SIMPLE_COMMAND:
    {
      pid_t p;
      int status = 0;
      p = fork();
      if (!p)
      {
        if(execvp(c->u.word[0], c->u.word))
          fprintf(stderr, "Failed to execute command '%s' with error: %s\n",
                  c->u.word[0], strerror(errno));
      }
      else
      {
        waitpid(p, &status, 0);
        if (WIFEXITED(status))
          c->status = WEXITSTATUS(status);
      }
      break;
    }
    case SUBSHELL_COMMAND:
      execute_command(c->u.command[0], profiling);
      break;
  }
  dup2(stdin_backup, STDIN_FILENO);
  dup2(stdout_backup, STDOUT_FILENO);
}
