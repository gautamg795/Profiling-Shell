#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid syntax is processed correctly.

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

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'
true

g++ -c foo.c

: : :

if cat < /etc/passwd | tr a-z A-Z | sort -u; then :; else echo sort failed!; fi

a b<c > d

if cat < /etc/passwd | tr a-z A-Z | sort -u > out
then :
else echo sort failed!
fi

if
  if a;a;a; then b; else :; fi
then

 if c
  then if d | e; then f; fi
 fi
fi

g<h

while
  while
    until :; do echo yoo hoo!; done
    false
  do (a|b)
  done >f
do
  :>g
done

# Another weird example: nobody would ever want to run this.
a<b>c|d<e>f|g<h>i
EOF

cat >test.exp <<'EOF'
# 1
  true
# 2
  g++ -c foo.c
# 3
  : : :
# 4
  if
      cat</etc/passwd \
    |
      tr a-z A-Z \
    |
      sort -u
  then
    :
  else
    echo sort failed!
  fi
# 5
  a b<c>d
# 6
  if
      cat</etc/passwd \
    |
      tr a-z A-Z \
    |
      sort -u>out
  then
    :
  else
    echo sort failed!
  fi
# 7
  if
    if
        a \
      ;
        a \
      ;
        a
    then
      b
    else
      :
    fi
  then
    if
      c
    then
      if
          d \
        |
          e
      then
        f
      fi
    fi
  fi
# 8
  g<h
# 9
  while
    while
        until
          :
        do
          echo yoo hoo!
        done \
      ;
        false
    do
      (
         a \
       |
         b
      )
    done>f
  do
    :>g
  done
# 10
    a<b>c \
  |
    d<e>f \
  |
    g<h>i
EOF

../profsh -t test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
