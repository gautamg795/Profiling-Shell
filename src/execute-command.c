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
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
extern bool file_error;
extern double NSECS_PER_SEC;
extern double USECS_PER_SEC;
static pid_t childpid;
void
handle_sigpipe(int sig)
{
   (void)sig;
   kill(childpid, SIGPIPE);
}

int
prepare_profiling (char const *name)
{
  return open(name, O_WRONLY | O_CREAT | O_APPEND, 0644);
}

struct timespec
diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}


double
timespec_to_sec(struct timespec *ts)
{
  return ts->tv_sec + (double)ts->tv_nsec / NSECS_PER_SEC;
}

double
timeval_to_sec(struct timeval *tv)
{
  return tv->tv_sec + (double)tv->tv_usec / USECS_PER_SEC;
}

int
command_status (command_t c)
{
  return c->status;
}

command_t
find_exec_in_tree(command_t root)
{
    if (!root)
        return NULL;
    switch (root->type)
    {
        case SIMPLE_COMMAND:
            {
                if (strcmp(root->u.word[0], "exec") == 0)
                    return root;
                else
                    return NULL;
            }
        case SEQUENCE_COMMAND:
        case WHILE_COMMAND:
        case UNTIL_COMMAND:
        case PIPE_COMMAND:
            {
                return find_exec_in_tree(root->u.command[0]) ?: find_exec_in_tree(root->u.command[1]);
            }
        case SUBSHELL_COMMAND:
            {
                return find_exec_in_tree(root->u.command[0]);
            }
        case IF_COMMAND:
            {
                return find_exec_in_tree(root->u.command[0]) ?: find_exec_in_tree(root->u.command[1]) ?: find_exec_in_tree(root->u.command[2]);
            }
  
        default:
            return NULL;
    }
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
      struct timespec leftstart, rightstart, leftend, rightend;
      int status;
      clock_gettime(CLOCK_MONOTONIC, &leftstart);
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
        childpid = left;
        signal(SIGPIPE, handle_sigpipe);
        clock_gettime(CLOCK_MONOTONIC, &rightstart);
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
          if (waitpid(left, NULL, WNOHANG))
            kill(left, SIGPIPE);
          _exit(c->u.command[1]->status);
        }
        else
        {
            if (profiling > 0 && !file_error)
            {
                struct rusage usage;
                if (wait4(left, &status, 0, &usage) == -1)
                {
                    error(1, errno, "Failed to waitpid");
                }
                char s[1024];
                if(clock_gettime(CLOCK_REALTIME, &leftend) == -1)
                {
                    perror(NULL);
                    exit(1);
                }
                double endtime = leftend.tv_sec + (double)leftend.tv_nsec / NSECS_PER_SEC;
                if(clock_gettime(CLOCK_MONOTONIC, &leftend) == -1)
                {
                    perror(NULL);
                    exit(1);
                }
                struct timespec elapsed = diff(leftstart, leftend);
                double elapsedtime = elapsed.tv_sec + (double)elapsed.tv_nsec / NSECS_PER_SEC;
                double utime = usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / USECS_PER_SEC;
                double stime = usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / USECS_PER_SEC;
                snprintf(s, 1023, "%.6f %.6f %.3f %.3f [%d]\n", endtime, elapsedtime, utime, stime, left);
                if(write(profiling, s, strlen(s)) == -1)
                    file_error = true;
          }
          else if (waitpid(left, &status, 0) == -1)
            error(1, errno, "Failed to waitpid");
          if (close(pipefd[1]) == -1) // After left command has ended, close the write end
            error(1, errno, "Failed to close"); // to signal EOF to the right command
          childpid = right;
          if (profiling > 0 && !file_error)
            {
                struct rusage usage;
                if (wait4(right, &status, 0, &usage) == -1)
                {
                    error(1, errno, "Failed to waitpid");
                }
                char s[1024];
                if(clock_gettime(CLOCK_REALTIME, &rightend) == -1)
                {
                    perror(NULL);
                    exit(1);
                }
                double endtime = rightend.tv_sec + (double)rightend.tv_nsec / NSECS_PER_SEC;
                if(clock_gettime(CLOCK_MONOTONIC, &rightend) == -1)
                {
                    perror(NULL);
                    exit(1);
                }
                struct timespec elapsed = diff(rightstart, rightend);
                double elapsedtime = elapsed.tv_sec + (double)elapsed.tv_nsec / NSECS_PER_SEC;
                double utime = usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / USECS_PER_SEC;
                double stime = usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / USECS_PER_SEC;
                snprintf(s, 1023, "%.6f %.6f %.3f %.3f [%d]\n", endtime, elapsedtime, utime, stime, right);
                if(write(profiling, s, strlen(s)) == -1)
                    file_error = true;
            }
          else if (waitpid(right, &status, 0) == -1)
            error(1, errno, "Failed to waitpid");
          if (close(pipefd[0]) == -1)
            error(1, errno, "Failed to close");
          c->status = WEXITSTATUS(status); // We only care about the exit status
                                           // of the right side command
          signal(SIGPIPE, SIG_DFL);
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
      struct timespec start_time;
      if(clock_gettime(CLOCK_MONOTONIC, &start_time) == -1)
      {
        perror(NULL);
        exit(1);
      }
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
        childpid = p;
        signal(SIGPIPE, handle_sigpipe);
        if (profiling > 0 && !file_error)
        {
            struct rusage usage;
            if (wait4(p, &status, 0, &usage) == -1)
            {
                error(1, errno, "Failed to waitpid");
            }
            char s[1024];
            struct timespec end_time;
            if(clock_gettime(CLOCK_REALTIME, &end_time) == -1)
            {
                perror(NULL);
                exit(1);
            }
            double endtime = end_time.tv_sec + (double)end_time.tv_nsec / NSECS_PER_SEC;
            if(clock_gettime(CLOCK_MONOTONIC, &end_time) == -1)
            {
                perror(NULL);
                exit(1);
            }
            struct timespec elapsed = diff(start_time, end_time);
            double elapsedtime = elapsed.tv_sec + (double)elapsed.tv_nsec / NSECS_PER_SEC;
            double utime = usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / USECS_PER_SEC;
            double stime = usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / USECS_PER_SEC;
            snprintf(s, 1023, "%.6f %.6f %.3f %.3f", endtime, elapsedtime, utime, stime);
            char** w = c->u.word;
            while (*w != NULL && strlen(s) < 1023)
            {
                snprintf(s + strlen(s), 1023 - strlen(s), " %s", *w);
                w++;
            }
            sprintf(s+strlen(s), "\n");
            if(write(profiling, s, strlen(s)) == -1)
                file_error = true;
        }
        else if (waitpid(p, &status, 0) == -1)
          error(1, errno, "Failed to waitpid");
        c->status = WEXITSTATUS(status);
        signal(SIGPIPE, SIG_DFL);
      }
      break;
    }
    case SUBSHELL_COMMAND:
    {
      pid_t p;
      int status;
      struct timespec start_time, end_time;
      clock_gettime(CLOCK_MONOTONIC, &start_time);
      p = fork();
      if (p < 0)
      {
          perror(NULL);
          _exit(1);
      }
      else if (p == 0)
      {
        execute_command(c->u.command[0], profiling);
        c->status = c->u.command[0]->status;
        _exit(c->status);
      }
      else
      {
          childpid = p;
          signal(SIGPIPE, handle_sigpipe);
          if (profiling > 0 && !file_error)
          {
              struct rusage usage;
              if (wait4(p, &status, 0, &usage) == -1)
              {
                  error(1, errno, "Failed to waitpid");
              }
              char s[1024];
              if(clock_gettime(CLOCK_REALTIME, &end_time) == -1)
              {
                  perror(NULL);
                  exit(1);
              }
              double endtime = end_time.tv_sec + (double)end_time.tv_nsec / NSECS_PER_SEC;
              if(clock_gettime(CLOCK_MONOTONIC, &end_time) == -1)
              {
                  perror(NULL);
                  exit(1);
              }
              struct timespec elapsed = diff(start_time, end_time);
              double elapsedtime = elapsed.tv_sec + (double)elapsed.tv_nsec / NSECS_PER_SEC;
              double utime = usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / USECS_PER_SEC;
              double stime = usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / USECS_PER_SEC;
              command_t exec_cmd = find_exec_in_tree(c->u.command[0]);
              if (exec_cmd)
              {
                  snprintf(s, 1023, "%.6f %.6f %.3f %.3f", endtime, elapsedtime, utime, stime);
                  char **w = exec_cmd->u.word;
                  while (*w != NULL && strlen(s) < 1023)
                  {
                      snprintf(s + strlen(s), 1023 - strlen(s), " %s", *w);
                      w++;
                  }
                  sprintf(s+strlen(s), "\n");
              }
              else
                snprintf(s, 1023, "%.6f %.6f %.3f %.3f [%d]\n", endtime, elapsedtime, utime, stime, p);
              if(write(profiling, s, strlen(s)) == -1)
                  file_error = true;

          }
          else if(waitpid(p, &status, 0) == -1)
          {
              perror(NULL);
              _exit(1);
          }
          c->status = WEXITSTATUS(status);
          signal(SIGPIPE, SIG_DFL);
      }
      break;
    }
  }
  if (dup2(stdin_backup, STDIN_FILENO) == -1)
    error(1, errno, "Failed to dup2");
  if (dup2(stdout_backup, STDOUT_FILENO) == -1)
    error(1, errno, "Failed to waitpid");
  close(stdin_backup);
  close(stdout_backup);
}
