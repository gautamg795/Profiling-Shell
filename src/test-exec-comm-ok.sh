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

find . -maxdepth 1 -name a.txt -print0

cat a.txt | grep world | grep hello > b.txt
diff a.txt b.txt

cat < b.txt > c.txt
head c.txt | tr a-z A-Z > d.txt
diff c.txt d.txt

(cat) < d.txt
(cat d.txt) > e.txt
(cat e.txt) | cat
cat e.txt | (cat)

echo seq1; echo seq2; echo seq3

( echo subshell; ( echo subsubshell ) )

:

if true
then echo withinthen
fi

if true
then
  if true;
  then echo withininnerthen; fi
fi

if false
then true
else echo withinelse
fi

if cat < a.txt | tr a-z A-Z | sort -u; then echo sort succeeded!;
else echo sort failed!; fi

while false
do nothing
done

until true
do nothing
done

# while cat a.txt; do rm a.txt; done
# Correctly reports error on second run

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
