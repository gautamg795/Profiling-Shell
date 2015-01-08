# CS 111 Lab 1 Makefile

# Copyright 2012-2014 Paul Eggert.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


# Build with 'make WERROR_CFLAGS=' to keep going despite warnings.
# However, don't rest until it's warning-free!

CC = gcc
WERROR_CFLAGS = -Werror
CFLAGS = -g -Wall -Wextra $(WERROR_CFLAGS)
LAB = 1
DISTDIR = lab1-$(USER)
CHECK_DIST = ./check-dist
TAR = tar
TAR_FLAGS = --numeric-owner --owner=0 --group=0 --mode=go+u,u+w,go-w

all: profsh

TESTS = $(wildcard test*.sh)
TEST_BASES = $(subst .sh,,$(TESTS))

PROFSH_SOURCES = \
  alloc.c \
  execute-command.c \
  main.c \
  read-command.c \
  print-command.c
PROFSH_OBJECTS = $(subst .c,.o,$(PROFSH_SOURCES))

DIST_SOURCES = \
  $(PROFSH_SOURCES) alloc.h command.h command-internals.h Makefile \
  $(TESTS) check-dist COPYING README

profsh: $(PROFSH_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(PROFSH_OBJECTS)

alloc.o: alloc.h
execute-command.o main.o print-command.o read-command.o: command.h
execute-command.o print-command.o read-command.o: command-internals.h

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES) check-dist
	rm -fr $(DISTDIR)
	$(TAR) $(TAR_FLAGS) -czf $@.tmp --transform='s,^,$(DISTDIR)/,' \
	  $(DIST_SOURCES)
	$(CHECK_DIST) $(DISTDIR)
	mv $@.tmp $@

Skeleton: $(DIST_SOURCES)
	$(MAKE) CHECK_DIST=: USER=$@ lab1-$@.tar.gz

check: $(TEST_BASES)

$(TEST_BASES): profsh
	./$@.sh

clean:
	rm -fr *.o *~ *.bak *.tar.gz core *.core *.tmp profsh $(DISTDIR)

.PHONY: all dist check $(TEST_BASES) clean Skeleton
