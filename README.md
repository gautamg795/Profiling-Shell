# Lab 1. Profiling shell
####Dylan Flanders (#504274041) and Gautam Gupta (#304282688)
###### CS111 Section 1A, Winter 2015
* * * 

## Known Issues

If our program finds a syntax error in the input file, it may continue outputting standard format for all commands found before the syntax error and then note the line number of the syntax error and exit the program. However, since this was later deemed unnecessary, other syntax errors may terminate the program immediately with just an error message. We have no additional features that we would like to call to your attention. 

## Project Status
Part 1a: Complete  
Part 1b: Incomplete  
Part 1c: Incomplete  
    
## Compilation
Download and untar the source files, then run `make` in the `src/` directory to generate the `profsh` executable. At this time, it can only be run as `profsh -t <sourcefile.sh>` which will print the command tree structure associated with the input.   
Run `make check` to run the shell against the two test scripts, `test-t-ok.sh` and `test-t-bad.sh`.  
Run `make debug` (only on the SEASnet servers) to compile with AddressSanitizer and UndefinedBehaviorSanitizer.  
  
Compilation warnings regarding unused parameters can be safely ignored, as they are regarding the incomplete portions of this lab.

   

     
    
#Project Specification
------

[Source](http://cs.ucla.edu/classes/winter15/cs111/assign/lab1.html "Permalink to Lab 1. Profiling shell")



## Introduction

You are a programmer for Big Data Systems, Inc., a company that specializes in large backend systems that analyze [big data][1]. Much of BDS's computation occurs in a cloud or a grid. Computational nodes are cheap [SMP][2] hosts with a relatively small number of processors. Nodes typically run simple shell scripts as part of the larger computation, and you've been assigned the job of speeding up these scripts.

Many of the shell scripts have command sequences that look like the following (though the actual commands tend to be more proprietary):

      sort < a | cat b - | tr A-Z a-z > c
      sort -k2 d - < a | uniq -c > e
      diff a c > f

      until cmp -s a b; do
        recalc a b | sort -u -o a
        recalc b a | sort -u -o b
      done

Some of these individual commands run quickly, and some slowly, and it's not easy for your developers (who are not shell experts) to debug where the slownesses are. They can modify their scripts to use the [`time`][3] command, but that's a pain and they'd rather have a method that doesn't require modifying their scripts.

Your goal is to write a prototype for a shell that runs scripts like the above, without modification, while providing performance information that does not unduly interfere with ordinary execution. If this prototype works well, the idea is that you'll later (i.e., after CS 111 is over...) improve the prototype until it is production quality.

Your company's shell scripts all follow some simple rules which should make the prototype easier to write:

* They limit themselves to a small subset of the shell syntax, described in "Shell syntax subset" below.
* They don't care about some properties of your implementation, and will work regardless of how your implementation behaves in these areas, as described in "Don't care behaviors" below.

Your implementation will take three phases:

* In Lab 1a, you'll warm up by implementing just the shell's command reader. This shell will support a test option `-t`, so that the command '`profsh -t script.sh`' will read the shell commands in the file `script.sh` and output them in a standard format that is already supplied by a code skeleton available on CCLE; sample output is in the skeleton's test script `test-t-ok.sh`.
* In Lab 1b, you'll implement the standard execution model for your shell subset. Once this is done, the command '`profsh script.sh`' should behave like the standard command '`sh script.sh`', assuming `script.sh` is in the shell subset described in this assignment.
* In Lab 1c, you'll implement the profiling execution model, which outputs profiling information. Once this is done, the command '`profsh -p f.shp script.sh`' should execute the script while recording profiling information in the file `p.shp`.

## Lab 1c details

In Lab 1c, every process started by the shell or by one of its descendant subshells should generate one line of output in the profile when the process exits. Also, one line of output should be generated for the shell process itself, when it exits. Each line of the profile should look like the following:

    1414117272.94 0.159 0.020 0.003 sleep 0.1547825513s

In this example, "1414117272.94" is the time when the command finished (in seconds since 1970-01-01 00:00:00 UTC, not counting leap seconds), "0.159" is the command's real time in seconds, "0.020" is the command's user CPU time in seconds, "0.003" is the command's system CPU time in seconds, and "sleep 0.1547825513s" is the command name (here, "sleep") followed by a space and then by its arguments separated by spaces (here just one argument, "0.1547825513s"). If a name or argument contains a newline, display the newline as a space; if the entire log line (not counting the trailing newline) would contain more than 1023 bytes, output just the first 1023 bytes.

If the process did not exec a command or is the shell process itself, log the process's numeric ID in square brackets instead, e.g., for process 1321:

    1414117272.98 0.199 0.023 0.004 [1321]

When logging user and system CPU time for a process, include the CPU time consumed by children of the process.

When logging absolute times such as 1414117272.94, use the CLOCK_REALTIME clock; see the clock_gettime system call. When logging real time, use the clock supported by the SEASnet GNU/Linux servers that is most appropriate for measuring real time in a profiling shell; this is not necessarily the CLOCK_REALTIME clock. Your README file should justify your choice of clock.

If you can log times to more precision than shown in the examples above, do so; include trailing zeros in the times only if they are significant. See the clock_getres system call for information about how to get clock resolution.

If the log file cannot be written to for some reason, the shell should continue without further attempts at logging but should eventually exit with nonzero status. For example, the command "profsh -p /dev/full script" should exit with nonzero status after running the script.

Make sure that log lines are not interleaved when two processes finish at about the same time.

Your README file should answer the following questions. In these questions, assume that the log file could always be written to successfully.

* Must the first column of the log output be in nondecreasing order? If not, give a counterexample.
* Can the second column be less than the sum of the third and fourth columns, other than due to rounding errors? If so, explain why.
* Can the second, third, or fourth column ever be negative? Briefly explain.
* How did you test your shell to make sure that log lines are not interleaved? If you used test scripts, submit a copy of the scripts as part of your submission tarball.

## Implementation

A skeleton implementation will be given to you on CCLE. It comes with a makefile that supports the following actions. Your solution should have similar actions in its makefile.

* '`make`' builds the `profsh` program.
* '`make clean`' removes the program and all other temporary files and object files that can be regenerated with '`make`'.
* '`make check`' tests the `profsh` program on the available test cases. The initial test cases are just for Lab 1a, and they fail on the skeleton code because the skeleton code doesn't do anything useful. Your program should succeed on them. For Lab 1b and 1c, you should add two test cases each, in the same style as the existing cases for 1a.
* '`make dist`' makes a software distribution tarball `lab1-yourname.tar.gz` and does some simple testing on it. This tarball is what you should submit via CCLE.

Your solution should be written in the C programming language. Stick with the programming style used in the skeleton, which uses standard [GNU style for C][4]. Your code should be [robust][5], for example, it should not impose an arbitrary limit like 216 bytes on the length of a token. You may use the features of [C11][6] as implemented on the SEASnet GNU/Linux servers. Please prepend the directory `/usr/local/cs/bin` to your PATH, to get the versions of the tools that we will use to test your solution. Your solution should stick to the standard [GNU C library][7] that is installed on SEASnet, and should not rely on other libraries.

You can run your program directly by invoking, for example, `./profsh -t foo`. Eventually, you should put your own test cases into a file `testsomething.sh` so that it is automatically run as part of '`make check`'.

## More details on syntax and semantics of the shell subset

The "time travel" feature of your shell is feasible partly because of the restricted subset of the shell that you need to implement. Also, for this assignment, the shell has been simplified further so as to avoid some work that can be deferred until a production version.

### Shell syntax subset

Your implementation of the shell needs to support only the following small subset of the standard [POSIX shell grammar][8]:

* Words, consisting of a maximal sequence of one or more adjacent characters that are ASCII letters (either upper or lower case), digits, or any of: `! % + , - . / : @ ^ _`
* The following six special tokens: `; | ( ) < >`
* Simple commands, which are sequences of one or more words. The first word is the file to be executed.
* Subshells, which are complete commands surrounded by `( )`.
* Compound commands, which have one of the following forms:
    * `if` A `then` B `fi`
    * `if` A `then` B `else` C `fi`
    * `while` A `do` B `done`
    * `until` A `do` B `done`
Each letter A, B, C represents a complete command. The special words (`if`, etc.) are recognized only if they would otherwise be the first word of a simple command. For example, the simple command '`cat if done`' is valid, and runs the program `cat` with arguments `if` and `done`.
* Commands, which are simple commands or compound commands or subshells, followed by I/O redirections. An I/O redirection is possibly empty, or `<` followed by a word, or `>` followed by a word, or `<` followed by a word followed by `>` followed by a word.
* Pipelines, which are one or more commands separated by `|`.
* Complete commands, which are one or more pipelines each separated by a semicolon or newline, and which are optionally followed by a semicolon. An entire shell script is a complete command.
* Comments, each consisting of a `#` that is not immediately preceded by an ordinary token, followed by characters up to (but not including) the next newline.
* White space consisting of space, tab, and newline. Newline is special: as described above, it can substitute for semicolon. Also, although white space can ordinarily appear before and after any token, the only tokens that newlines can appear before are `(`, `)`, `if`, `then`, `else`, `fi`, `while`, `do`, `done`, `until`, and the first words of simple commands. Newlines may follow any special token other than `<` and `>`.

If your shell's input does not fall within the above subset, your implementation should output to stderr a syntax error message that starts with the line number and a colon, and should then exit.

Your implementation can have undefined behavior if any of the following features are used. In other words, our test cases won't use these features and your program need not diagnose an error if these features are used.

* [Shell reserved words][9] such as `!`, `{`, `if`, and `function`, when used as the first word of a command.
* Commands that invoke [special built-in utilities][10] such as `break`, `.`, and `exit`. Exception: your implementation should support the special-builtin utility [`exec`][11] with a command and optional arguments (you need not support `exec` without a command).
* A token consisting entirely of digits, immediately before `<` or `>` (for example, as in the command '`cat 2>/dev/null`').
* Two adjacent left parentheses `((` – see [Token Recognition][12] for why.

### Don't care behaviors

Similarly, in some cases, your company's scripts don't care how your implementation behaves, and it's OK for it to depart from established semantics when it is run in time-travel mode.

* It is OK if commands behave a bit more slowly because of the profiling overhead. For example, it is OK if '`sleep 1`' sleeps a tiny bit more than 1 second (which it does anyway).
* It is OK if commands modify the file named by `profsh`'s `-p` option.

You can simplify your shell in one other way, regardless of whether it is run in time-travel mode:

* It is OK if your shell attempts to execute the following commands as regular commands, finding them via the PATH environment variable and running them as executables in a separate child process, even if the commands do not exist in the PATH, and even though POSIX does not allow this behavior: `false,fc,fg,getopts,jobs,kill,newgrp,pwd,read,true,umask,unalias,wait `

© 2012–2014 [Paul Eggert][26]. See [copying rules][27].  

[1]: http://en.wikipedia.org/wiki/Big_data
[2]: http://en.wikipedia.org/wiki/Symmetric_multiprocessing
[3]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/time.html
[4]: http://www.gnu.org/prep/standards/html_node/Writing-C.html
[5]: http://www.gnu.org/prep/standards/html_node/Semantics.html
[6]: http://en.wikipedia.org/wiki/C11_%28C_standard_revision%29
[7]: http://www.gnu.org/software/libc/
[8]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_10
[9]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_04
[10]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_14
[11]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#exec
[12]: http://pubs.opengroup.org/onlinepubs/007904875/xrat/xcu_chap02.html#tag_02_02_03
[13]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/false.html
[14]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/fc.html
[15]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/fg.html
[16]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/getopts.html
[17]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/jobs.html
[18]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/kill.html
[19]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/newgrp.html
[20]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/pwd.html
[21]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/read.html
[22]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/true.html
[23]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/umask.html
[24]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/unalias.html
[25]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/wait.html
[26]: ../mail-eggert.html
[27]: ../copyright.html
