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
#define error(x,y,z) errc(x,y,z)
#else
#include <error.h>
#endif
#include <unistd.h>
#include <sys/wait.h>
#include "alloc.h"
#include <string.h>
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
  (void)profiling; // We're not doing anything with this flag yet.
  if(!c)
  {
    error(1, 0, "We tried ot execute a NULL command");
  }
  pid_t p;
  int status;
  switch(c->type)
  {
    case IF_COMMAND:
    case WHILE_COMMAND:
    case UNTIL_COMMAND:
    case PIPE_COMMAND:
    case SEQUENCE_COMMAND:
    case SIMPLE_COMMAND:
    {
      p = fork();
      if (!p)
      {
        int word_count = 0;
        for (;c->u.word[word_count] != NULL; word_count++)
          ;
        char **args = checked_malloc((word_count + 1) * sizeof(char*));
        args[0] = c->u.word[0];
        memcpy(&args[1], c->u.word, word_count * sizeof(char *));
        execvp(args[0], args);
      }
      else
      {
        waitpid(p, &status, 0);
        if (WIFEXITED(status))
          c->status = WEXITSTATUS(status);
        else
          error(1, 0, "Child process did not exit");
      }
    }
    case SUBSHELL_COMMAND:
      ;
  }
}
