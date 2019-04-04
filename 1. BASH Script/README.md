## Program 1 - Matrices
This assignment asks you to write bash shell scripts to compute matrix operations. The purpose is to get you familiar with the Unix shell, shell programming, Unix utilities, standard input, output, and error, pipelines, process ids, exit values, and signals (at a basic level).

What you're going to submit is your script, called simply "matrix".

### Overview
In this assignment, you will write a bash shell script that calculates basic matrix operations using input from either a file or stdin. The input will be whole number values separated by tabs into a rectangular matrix. Your script should be able to print the dimensions of a matrix, transpose a matrix, calculate the mean vector of a matrix, add two matrices, and multiply two matrices.

You will be using bash builtins and Unix utilities to complete the assignment. Some commands to read up on are `while`, `cat`,  `read`, `expr`, `cut`, `head`,  `tail`, `wc`, and `sort`.

Your script must be called simply "matrix". The general format of the matrix command is:
```bash
matrix OPERATION [ARGUMENT]
```
Refer to `man(1)` for an explanation of the conventional notation regarding command syntax, to understand the line above. Note that many terminals render italic font style as an underline.

### Specifications
Your program must perform the following operations: dims, transpose, mean, add, and multiply. Usage is as follows:
```bash
matrix dims [MATRIX]
matrix transpose [MATRIX]
matrix mean [MATRIX]
matrix add MATRIX_LEFT MATRIX_RIGHT
matrix multiply MATRIX_LEFT MATRIX_RIGHT
```
The dims, transpose, and mean operations should either perform their respective operations on the file named MATRIX, or on a matrix provided via stdin. The add and multiply operations do not need to process input via stdin.
 - dims should print the dimensions of the matrix as the number of rows, followed by a space, then the number of columns.
 - transpose should reflect the elements of the matrix along the main diagonal. Thus, an MxN matrix will become an NxM matrix and the values along the main diagonal will remain unchanged.
 - mean should take an MxN matrix and return an 1xN row vector, where the first element is the mean of column one, the second element is the mean of column two, and so on.
 - add should take two MxN matrices and add them together element-wise to produce an MxN matrix. add should return an error if the matrices do not have the same dimensions.
 - multiply should take an MxN and NxP matrix and produce an MxP matrix. Note that, unlike addition, matrix multiplication is not commutative. That is A*B != B*A.

Here is a brief example of what the output should look like.

```bash
$ cat m1
1	2	3	4
5	6	7	8
$ cat m2
1	2
3	4
5	6
7	8
$ ./matrix dims m1
2	4
$ cat m2 | ./matrix dims
4	2
$ ./matrix add m1 m1
2	4	6	8
10	12	14	16
$ ./matrix add m2 m2
2	4
6	8
10	12
14	16
$ ./matrix mean m1
3	4	5	6
$ ./matrix transpose m1
1	5
2	6
3	7
4	8
$ ./matrix multiply m1 m2
50	60
114	140
```

You must check for the right number and format of arguments to matrix. This means that, for example, you must check that a given input file is readable, before attempting to read it. You are not required to test if the input file itself is valid. In other words, the behavior of matrix is undefined when the matrix input is not a valid matrix. for the purposes of this assignment, a valid matrix is a tab-delimited table containing at least one element, where each element is a signed integer, every entry is defined, and the table is rectangular.

The following are examples of invalid matrices and will not be tested against your code, and may not be output by your program:

   - An empty matrix.
   - A matrix where the final entry on a row is followed by a tab character.
   - A matrix with empty lines.
   - A matrix with any element that is blank or not an integer.

Here is a valid matrix file, m1:
```bash
cat m1
8	5	6
3	2	2
1	6	7
5	0	7
2	2	4
$ cat -A m1   # The '-A' flag shows tabs as '^I' and newlines as '$'. This is a good way to check correctness.
8^I5^I6$
3^I2^I2$
1^I6^I7$
5^I0^I7$
2^I2^I4$
$
```
If the inputs are valid -- your program should output only to stdout, and nothing to stderr. The return value should be 0.

If the inputs are invalid -- your program should output only to stderr, and nothing to stdout. The return value should be any number except 0. The error message you print is up to you; you will receive points as long as you print something to stderr and return a non-zero value.

Your program will be tested with matrices up to dimension 100 x 100.

Though optional, I do recommend that you use temporary files; arrays are not recommended. For this assignment, anytemporary files you use should be put in the current working directory. (A more standard place for temporary files is in /tmp but don't do that for this assignment; it makes grading easier if they are in the current directory.) Be sure any temporary file you create uses the process id as part of its name, so that there will not be conflicts if the program is running more than once. Be sure you remove any temporary files when your program is done. You should also use the trap command to catch interrupt, hangup, and terminate signals to remove the temporary files if the program is terminated unexpectedly.

All values and results are and must be integers. You may use the `expr`  command to do your calculations, or any other bash shell scripting method, such as `((expr))`. Do not use any other languages other than bash shell scripting: this means that, among others, awk, sed, tcl, bc, perl, & the python languages and tools are off-limits for this assignment. Note that `expr` only works with whole numbers. When you calculate the average you must round to the nearest integer, where half values round away from 0 (i.e. 7.5 rounds up to 8, but -7.5 rounds down to -8). This is the most common form of rounding. When doing truncating integer division (as bash does), this formula works to divide two numbers and end up with the proper rounding:
```bash
(a + (b/2)*( (a>0)*2-1 )) / b
```
You can learn more about rounding methods here:
[Rounding - Wikipedia](https://en.wikipedia.org/wiki/Rounding#Round_half_away_from_zero)

### Grading With a Script
To make it easy to see how you're doing, you can download the actual grading script here:
[p1gradingscript](./p1gradingscript)
This script is very close to the one that will be used to assign your script a grade. To use the script, just place it in the same directory as your matrix script and run it like this:
```bash
$ ./p1gradingscript
```
Be aware that this script might take a minute or two to run, depending on how speedy your algorithms and code are, since there's a lot of matrix math in there. You're not penalized for having a long-running script, though the TAs may decide that it's just stuck in a loop and is actually broken if it goes too much longer than that (which is a penalizing problem). If you have any doubt about your running time being acceptable, just add a note into your code as a comment at the top to warn the TA. When we run your script for grading, we will do this to put your results into a file we can examine more easily:
```bash
$ ./p1gradingscript > grading_result.username
```
To compare yours to a perfect solution, you can download here a completely correct run of the script that shows what you should get if everything is working correctly:

[p1perfectoutput](https://github.com/IvanHalim/operating-systems/blob/master/1.%20BASH%20Script/p1perfectoutput)

The p1gradingscript itself is a good resource for seeing how some of the more complex shell scripting commands work, too.