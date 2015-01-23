#! /bin/sh

# UCLA CS 111 Lab1b - Test that valid commands are run correctly.
# By Dylan Flanders & Gautam Gupta

tmp=$0-$$.tmp
mkdir "$tmp" || exit

trap "rm -rf $tmp; exit" SIGHUP SIGINT SIGTERM #EXIT

(
cd "$tmp" || exit

cat >test_exec.sh <<'EOF' 

#! /bin/bash
echo hello world > a.txt

cat a.txt

echo seq1; echo seq2

( echo subshell )

if true
then echo withinif
fi

EOF

chmod +x test_exec.sh
./test_exec.sh >test_exec.exp || exit

../profsh test_exec.sh >test_exec.txt 2>err.txt || exit

diff -u test_exec.exp test_exec.txt || exit 1
test ! -s err.txt || {
  cat err.txt
  exit 1
}

) || exit

rm -rf "$tmp"
