#!/bin/sh

# UCLA CS 111 Lab 1c - Test that errors in profiling are caught.
# By Dylan Flanders & Gautam Gupta

tmp=$0-$$.tmp
mkdir "$tmp" || exit

trap "rm -rf $tmp; exit" SIGHUP SIGINT SIGTERM #EXIT

(
cd "$tmp" || exit

cat >test1.sh <<EOF

#! /usr/bin/env bash
echo hello world
EOF

../profsh -p "/" test1.sh 1>/dev/null 2>err1.txt
if [ $? -eq 0 -o "$(wc -l <err1.txt)" != "1" ]
then
	echo "Failed bad profiling test 1"
	exit 1
fi

../profsh -p /dev/full test1.sh 1>/dev/null 2>err2.txt
if [ $? -eq 0 -o "$(wc -l <err2.txt)" != "0" ]
then
	echo "Failed bad profiling test 2"
	exit 2
fi
    
cat >test3.sh <<EOF

#! /usr/bin/env bash
echo hello world | rev | cat | cat
EOF

( ulimit -u 8
  ../profsh -p test3.out test3.sh 1>/dev/null 2>err3.txt
  if [ $? -eq 0 -o "$(wc -l <err3.txt)" == "0" ]
  then
	  echo "Failed bad profiling test 3"
	  exit 3
  fi )

) || exit

rm -rf "$tmp"
