#! /bin/sh

# UCLA CS 111 Lab 1c - Test that errors in profiling are caught.
# By Dylan Flanders & Gautam Gupta

tmp=$0-$$.tmp
mkdir "$tmp" || exit

trap "rm -rf $tmp; exit" SIGHUP SIGINT SIGTERM #EXIT

(
cd "$tmp" || exit
status=

cat >test1.sh << 'EOF'

#! /usr/bin/env bash
echo hello world
EOF

../profsh -p "/" test1.sh 1>/dev/null 2>err1.txt 
if [ $? -eq 0 -o "$(wc -l <err1.txt)" != "1" ]
then
   	echo "Failed bad profiling test 1"
    exit 1
fi
) || exit

rm -rf "$tmp"
