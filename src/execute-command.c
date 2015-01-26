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
#include <stdlib.h>
extern bool file_error;
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
prepare_profiling (char const *name)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  return open(name, O_WRONLY | O_CREAT | O_TRUNC);
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
    error(1, 0, "We tried to execute a NULL command");
  }
  int stdin_backup = dup(STDIN_FILENO);
  if (stdin_backup == -1)
    error(1, errno, "Failed to dup stdin");
  int stdout_backup = dup(STDOUT_FILENO);
  if (stdout_backup == -1)
    error(1, errno, "Failed to dup stdout");
  if (c->input)
  {
    int fd = open(c->input, O_RDONLY);
    if (fd < 0)
    {
      perror(c->input);
      exit(1);
    }
    if (dup2(fd, STDIN_FILENO) == -1)
      error(1, errno, "Failed to dup2");
    close(fd);
  }
  if (c->output)
  {
    int fd = open(c->output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
      perror(c->output);
      exit(1);
    }
    if (dup2(fd, STDOUT_FILENO) == -1)
      error(1, errno, "Failed to dup2");
    close(fd);
  }
  switch(c->type)
  {
    case IF_COMMAND:
    {
      c->status = 0;
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
      c->status = 0;
      while (true)
      {
        execute_command(c->u.command[0], profiling);
        if (! c->u.command[0]->status) // while command returned 0
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
      c->status = 0;
      while (true)
      {
        execute_command(c->u.command[0], profiling);
        if ((c->status = c->u.command[0]->status)) // until command returned nonzero
        {
          execute_command(c->u.command[1], profiling);
          c->status = c->u.command[1]->status;
        }
        else
          break;
      }
      break;
    }
    case PIPE_COMMAND:
    {
      int pipefd[2];
      if (pipe(pipefd) == -1)
        error(1, errno, "Failed to pipe");
      pid_t left, right;
      int status;
      left = fork();
      if (left == -1)
      {
        error(1, errno, "Failed to fork");
      }
      else if (!left) // child
      {
        if (close(pipefd[0]) == -1) // close read end
        {
          perror("Failed to close read end of pipe");
          _exit(1);
        }
        if (dup2(pipefd[1], STDOUT_FILENO) == -1)
        {
          perror("Failed to dup2");
          _exit(1);
        }
        execute_command(c->u.command[0], profiling);
        _exit(c->u.command[0]->status);
      }
      else
      {
        right = fork();
        if (right == -1)
        {
          error(1, errno, "Failed to fork");
        }
        else if (!right) // child 2
        {
          if (close(pipefd[1]) == -1) // close write end
          {
            perror("Failed to close write end of pipe");
            _exit(1);
          }
          if (dup2(pipefd[0], STDIN_FILENO) == -1)
          {
            perror("Failed to dup2");
            _exit(1);
          }
          execute_command(c->u.command[1], profiling);
          _exit(c->u.command[1]->status);
        }
        else
        {
          if (waitpid(left, &status, 0) == -1)
            error(1, errno, "Failed to waitpid");
          if (close(pipefd[1]) == -1) // After left command has ended, close the write end
            error(1, errno, "Failed to close"); // to signal EOF to the right command
          if (waitpid(right, &status, 0) == -1)
            error(1, errno, "Failed to waitpid");
          if (close(pipefd[0]) == -1)
            error(1, errno, "Failed to close");
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
      if (strcmp(c->u.word[0], "exec") == 0)
      {
        if (c->u.word[1] != NULL)
          execvp(c->u.word[1], &(c->u.word[1]));
        else
          error(1, 0, "`exec' requires a command");
      }
      else if (strcmp(c->u.word[0], ":") == 0)
      {
        free(c->u.word[0]);
        c->u.word[0] = checked_malloc(5 * sizeof(char));
        strcpy(c->u.word[0], "true");
      }
      pid_t p;
      int status = 0;
      p = fork();
      if (p == -1)
      {
        error(1, errno, "Failed to fork");
      }
      else if (!p)
      {
        if(execvp(c->u.word[0], c->u.word))
        {
          fprintf(stderr, "Failed to execute command '%s' with error: %s\n",
                  c->u.word[0], strerror(errno));
          _exit(1);
        }
        
      }
      else
      {
        if (waitpid(p, &status, 0) == -1)
          error(1, errno, "Failed to waitpid");
        if (WIFEXITED(status))
          c->status = WEXITSTATUS(status);
      }
      break;
    }
    case SUBSHELL_COMMAND:
      execute_command(c->u.command[0], profiling);
      c->status = c->u.command[0]->status;
      break;
  }
  if (dup2(stdin_backup, STDIN_FILENO) == -1)
    error(1, errno, "Failed to dup2");
  if (dup2(stdout_backup, STDOUT_FILENO) == -1)
    error(1, errno, "Failed to waitpid");
  close(stdin_backup);
  close(stdout_backup);
}
