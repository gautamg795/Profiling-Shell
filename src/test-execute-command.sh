#! /bin/sh

# UCLA CS 111 Lab1b - Test that valid commands are run correctly.
# By Dylan Flanders & Gautam Gupta

tmp=$0-$$.tmp
mkdir "$tmp" || exit

trap "rm -rf $tmp; exit" SIGHUP SIGINT SIGTERM

(
cd "$tmp" || exit

cat >test_exec.sh <<'EOF' 

echo hello
EOF

chmod +x test_exec.sh
./test_exec.sh >test_exec.exp 2>exp_err.txt || exit

../profsh test_exec.sh >test_exec.txt 2>act_err.txt || exit

diff -u test_exec.exp test_exec.txt || exit 1
test ! -s actl_err.txt || {
  cat act_err.txt
  exit 1
}

) || exit

rm -rf "$tmp"
