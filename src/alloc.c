// UCLA CS 111 Lab 1 storage allocation

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

#include "alloc.h"

#include <error.h>
#include <errno.h>
#include <stdlib.h>

static void
memory_exhausted (int errnum)
{
  error (1, errnum, "memory exhausted");
}

static void *
check_nonnull (void *p)
{
  if (! p)
    memory_exhausted (errno);
  return p;
}

void *
checked_malloc (size_t size)
{
  return check_nonnull (malloc (size ? size : 1));
}

void *
checked_realloc (void *ptr, size_t size)
{
  return check_nonnull (realloc (ptr, size ? size : 1));
}

void *
checked_grow_alloc (void *ptr, size_t *size)
{
  size_t max = -1;
  if (*size == max)
    memory_exhausted (0);
  *size = *size < max / 2 ? 2 * *size : max;
  return checked_realloc (ptr, *size);
}
