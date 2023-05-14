# SWISH-Extension
A version of SWISH that supports pipelined command execution and redirection. Does not support background process management like the first part did.
In order to keep the focus of this repository on the pipeline features, some repeated source code is replaced with the compiled version.

#### Original SWISH: https://github.com/JacksonKary/SWISH

## This project uses a number of important systems programming topics:

- Text file input and parsing
- Creating pipes with the pipe() system call
- Using pipe file descriptors to coordinate among several processes
- Executing program pipelines using pipe() and dup2() along with the usual fork() and exec()

## All of the following are examples of valid command pipelines that this shell handles:
<ul>
  <li> <code>cat file.txt | wc -l</code>
  <li> <code>cat < file.txt | wc -l</code>
  <li> <code>cat file.txt | wc -l > out.txt</code>
  <li> <code>cat < file.txt | wc -l > out.txt</code>
  <li> <code>cat file.txt | wc -l >> out.txt</code>
  <li> <code>cat < file.txt | wc -l >> out.txt</code>
</ul>
    
For example, consider the pipeline <code>cat gatsby.txt | tr -cs A-Za-z \n | tr A-Z a-z | sort | uniq -c | sort -n | tail -n 12</code>, which counts the most frequently appearing words in the file gatsby.txt. This pipeline consists of seven commands and therefore requires six pipes, as shown in the diagram below:
![image](https://github.com/JacksonKary/SWISH-Extension/assets/117691954/fc7d3a47-c7b4-45f2-9867-4a7ccda221f8)
    
## What is in this directory?
<ul>
  <li>  <code>swish_funcs_provided.o</code> : Binary file with implementations of some shell helper functions.
  <li>  <code>swish_funcs.c</code> : Implementations of swish helper functions - **Bulk of the extension is here.**
  <li>  <code>swish_funcs.h</code> : Header file for swish helper functions.
  <li>  <code>swish.c</code> : Implements the simplified command-line interface for the swish shell.
  <li>  <code>string_vector.h</code> : Header file for a vector data structure to store strings.
  <li>  <code>string_vector.c</code> : Implementation of the string vector data structure - includes more functions than the original SWISH.
  <li>  <code>Makefile</code> : Build file to compile and run test cases.
  <li>  <code>test_cases</code> Folder, which contains:
  <ul>
    <li>  <code>input</code> : Input files used in automated testing cases.
    <li>  <code>resources</code> : More input files.
    <li>  <code>output</code> : Expected output.
  </ul>
  <li>  <code>testius</code> : Python script that runs the tests.
</ul>

## Running Tests

A Makefile is provided as part of this project. This file supports the following commands:

<ul>
  <li>  <code>make</code> : Compile all code, produce an executable program.
  <li>  <code>make clean</code> : Remove all compiled items. Useful if you want to recompile everything from scratch.
  <li>  <code>make clean-tests</code> : Remove all files produced during execution of the tests.
  <li>  <code>make test</code> : Run all test cases.
  <li>  <code>make test testnum=5</code> : Run test case #5 only.
</ul>

