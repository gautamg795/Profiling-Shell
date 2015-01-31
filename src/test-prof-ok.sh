#! /usr/bin/env bash

# UCLA CS 111 Lab 1c - Test that valid commands are run correctly.
# By Dylan Flanders & Gautam Gupta

tmp=$0-$$.tmp
mkdir "$tmp" || exit

trap "rm -rf $tmp; exit" SIGHUP SIGINT SIGTERM #EXIT

(
cd "$tmp" || exit
cat >test1.sh << 'EOF'

#! /usr/bin/env bash
echo hello world

EOF
../profsh -p test1.out test1.sh 1>/dev/null 2>err1.txt
if [ "$(wc -l <test1.out)" != "2" -o "$(wc -l <err1.txt)" != "0" ]
then
    echo "Failed test 1"
    exit 1
fi

cat >test2.sh << 'EOF'

#! /usr/bin/env bash
(echo hello world)

EOF
../profsh -p test2.out test2.sh 1>/dev/null 2>err2.txt
if [ "$(wc -l <test2.out)" != "3" -o "$(wc -l <err2.txt)" != "0" ]
then
    echo "Failed test 2"
    exit 2
fi


cat >test3.sh << 'EOF'

#! /usr/bin/env bash
echo hello world | rev

EOF
../profsh -p test3.out test3.sh 1>/dev/null 2>err3.txt
if [ "$(wc -l <test3.out)" != "5" -o "$(wc -l <err3.txt)" != "0" ]
then
    echo "Failed test 3"
    exit 3
fi


cat >test4.sh << 'EOF'

#! /usr/bin/env bash
echo hello world | rev | cat

EOF
../profsh -p test4.out test4.sh 1>/dev/null 2>err4.txt
if [ "$(wc -l <test4.out)" != "8" -o "$(wc -l <err4.txt)" != "0" ]
then
    echo "Failed test 4"
    exit 4
fi


cat >test5.sh << 'EOF'

#! /usr/bin/env bash
exec /bin/ls

EOF
../profsh -p test5.out test5.sh 1>/dev/null 2>err5.txt
if [ "$(wc -l <test5.out)" != "0" -o "$(wc -l <err5.txt)" != "0" ]
then
    echo "Failed test 5"
    exit 5
fi


cat >test6.sh << 'EOF'

#! /usr/bin/env bash
( exec /bin/ls )
EOF
../profsh -p test6.out test6.sh 1>/dev/null 2>err6.txt
if [ "$(wc -l <test6.out)" != "2" -o "$(wc -l <err6.txt)" != "0" ] || ! grep -sq "exec" test6.out
then
    echo "Failed test 6"
    exit 6
fi

) || exit

rm -rf "$tmp"
