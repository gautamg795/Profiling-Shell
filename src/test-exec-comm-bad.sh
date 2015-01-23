#! /bin/sh

# UCLA CS 111 Lab1b - Test that invalid commands are caught.
# By Dylan Flanders & Gautam Gupta

tmp=$0-$$.tmp
mkdir "$tmp" || exit

trap "rm -rf $tmp; exit" SIGHUP SIGINT SIGTERM #EXIT

(
cd "$tmp" || exit
status=

n=1
for bad in \
  'bsadfhjk' \
  'cat meow' \
  'echo < qlwkrp932.txt' \
  'echo < qlwkrp932.txt > tmp.txt' \
  'uiptyn | grep i' \
  'echo o | uiptyn' \
  'echo hello | grep o | uiptyn'
do
  echo "$bad" >exec_test$n.sh || exit
  ../profsh exec_test$n.sh >exec_test$n.txt 2>exec_err$n.txt && {
    echo >&2 "test$n: unexpectedly succeeded for: $bad"
    status=1
  }
  test -s exec_err$n.txt || {
    echo >&2 "test$n: no error message for: $bad"
    status=1
  }
  n=$((n+1))
done

exit $status
) || exit

rm -rf "$tmp"
