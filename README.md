# Welcome to tpose! #

[![GCC build and tests](https://github.com/jmsmistral/tpose/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/jmsmistral/tpose/actions/workflows/ci.yml?query=branch%3Amaster)

tpose is a UNIX-based terminal program for transposing delimited text-files.

Read this article ([medium](https://medium.com/@jmsmistral/on-transposing-data-884a04a8c1bf) or [pdf](http://jonathansacramento.com/papers/on_transposing_data.pdf)) to learn more about how transposes work (and tpose!).


## Building tpose ##

Dependencies:
- **GNU GCC** with GNU C11 support
- **GNU Make**

#### GNU/Linux
Install GCC and Make using your distribution's package manager if
needed (for example, `sudo apt install gcc make` on Debian/Ubuntu). Then:

```sh
make
make test
./tpose --version
```

#### MacOS
Install Apple's Command Line Tools (`xcode-select --install`) if
needed for Make and the system SDK, and install GNU GCC with
`brew install gcc`. Note: Apple's `/usr/bin/gcc` invokes Clang, so select the
versioned Homebrew executable explicitly. For example, with GCC 16:

```sh
make CC=gcc-16
make test CC=gcc-16
./tpose --version
```

#### Installing

You can run `./tpose` directly without installing it.
Installation defaults to `/usr/local/bin` and builds the executable first:

```sh
make install PREFIX="$HOME/.local"
make uninstall PREFIX="$HOME/.local"
```

Add `$HOME/.local/bin` to your `PATH` to invoke that installation as `tpose`.
For a system-wide installation, use `sudo make install` (and specify `CC` on
macOS). Packaging tools can stage an installation with `DESTDIR`, for example
`make install DESTDIR=/tmp/tpose-package PREFIX=/usr`.

Run `make clean` before switching compilers or flags.
For example:

```sh
make clean
make CC=gcc CFLAGS='-O0 -g -Wall -Wextra'
```


## Running tpose ##

tpose usage pattern:
```bash
tpose input-file [output-file] [-IGNdiapsPhv]
```
Get more details on the different options by running:
```bash
$ tpose --help
```

#### Simple transpose ####
Every row must have the same number of fields as the first row. Empty cells
are allowed, including a final empty cell represented by a trailing delimiter.
Rows with missing or extra fields are rejected with a row-numbered error.

```bash
$ cat data_ex1_simple.txt | column -s$'\t' -t
Quarter  Europe  Asia  US
Q1       2       5     3
Q2       3       4     1
Q3       3       5     2
Q4       4       6     3

$ tpose data_ex1_simple.txt
Quarter  Q1  Q2  Q3  Q4
Europe   2   3   3   4
Asia     5   4   5   6
US       3   1   2   3
```

#### Transpose over GROUP field ####
```bash
$ cat data_ex2_group.txt | column -s$'\t' -t
Customer_id  Revenue_group  Amount
1            rev_A          2
1            rev_B          3
2            rev_C          6
3            rev_B          7
3            rev_B          2
3            rev_C          9
4            rev_A          8
4            rev_A          3

$ tpose data_ex2_group.txt -Grevenue_group -Namount
rev_A	rev_B	rev_C
13.00	12.00	15.00
```

#### Transpose over GROUP and ID field ####
```bash
$ cat data_ex2_group.txt | column -s$'\t' -t
Customer_id  Revenue_group  Amount
1            rev_A          2
1            rev_B          3
2            rev_C          6
3            rev_B          7
3            rev_B          2
3            rev_C          9
4            rev_A          8
4            rev_A          3

$ tpose data_ex2_group.txt -Icustomer_id -Grevenue_group -Namount
customer_id  rev_A  rev_B  rev_C
1            2.00   3.00   0.00
2            0.00   0.00   6.00
3            0.00   9.00   9.00
4            11.00  0.00   0.00
```

#### Field indexes instead of names ####
Use the -i or --indexed option.
```bash
$ cat data_ex2_group.txt | column -s$'\t' -t
Customer_id  Revenue_group  Amount
1            rev_A          2
1            rev_B          3
2            rev_C          6
3            rev_B          7
3            rev_B          2
3            rev_C          9
4            rev_A          8
4            rev_A          3

$ tpose data_ex2_group.txt -i -I1 -G2 -N3
customer_id  rev_A  rev_B  rev_C
1            2.00   3.00   0.00
2            0.00   0.00   6.00
3            0.00   9.00   9.00
4            11.00  0.00   0.00
```

#### Different types of aggregation ####
Use the -a or --aggregate option followed by 'sum' (default), 'count', or 'avg'.

* COUNT
Counts group field instances instead of summing the NUMERICAL field values. 
```bash
$ cat data_ex2_group.txt | column -s$'\t' -t
Customer_id  Revenue_group  Amount
1            rev_A          2
1            rev_B          3
2            rev_C          6
3            rev_B          7
3            rev_B          2
3            rev_C          9
4            rev_A          8
4            rev_A          3

$ tpose data_ex2_group.txt -i -I1 -G2 -N3 -acount
customer_id  rev_A  rev_B  rev_C
1            1      1      0
2            0      0      1
3            0      2      1
4            2      0      0
```

* AVG
Divides the sum of the NUMERICAL field values by the count of GROUP field instances (division by zero result in 'Not-A-Number' or nans)
```bash
$ cat data_ex2_group.txt | column -s$'\t' -t
Customer_id  Revenue_group  Amount
1            rev_A          2
1            rev_B          3
2            rev_C          6
3            rev_B          7
3            rev_B          2
3            rev_C          9
4            rev_A          8
4            rev_A          3

$ tpose data_ex2_group.txt -i -I1 -G2 -N3 -aavg
customer_id  rev_A  rev_B  rev_C
1            2.00   3.00   -nan
2            -nan   -nan   6.00
3            -nan   4.50   9.00
4            5.50   -nan   -nan
```

#### Parallel execution ####
Use the -P or --parallel option (only works for files >1GB). This example prints to an output file instead of the screen.
```bash
$ ls -l data_large.txt
-rw-r--r--  1 jonathan  staff    14G 25 Sep 21:28 data_large.txt

$ tpose data_large.txt output_tpose.txt -P -i -I1 -G15 -N32

$ ls -l output_tpose.txt
-rw-r--r--  1 jonathan  staff   110M 25 Sep 21:30 output_tpose.txt
```

#### Changing delimiter ####
Use the -d or --delimiter option.
```bash
$ cat data.csv
Customer_id,Revenue_group,Amount
1,rev_A,2
1,rev_B,3
2,rev_C,6
3,rev_B,7
3,rev_B,2
3,rev_C,9
4,rev_A,8
4,rev_A,3

$ tpose data.csv -d, -i -I1 -G2 -N3
customer_id,rev_A,rev_B,rev_C
1,2.00,3.00,0.00
2,0.00,0.00,6.00
3,0.00,9.00,9.00
4,11.00,0.00,0.00
```

#### Add a prefix/suffix to output field names ####
Use the -p (or --prefix), and -s (or --suffix) option.
```bash
$ cat data_ex2_group.txt | column -s$'\t' -t
Customer_id  Revenue_group  Amount
1            rev_A          2
1            rev_B          3
2            rev_C          6
3            rev_B          7
3            rev_B          2
3            rev_C          9
4            rev_A          8
4            rev_A          3

$ tpose data_ex2_group.txt -i -I1 -G2 -N3 -pxxx_ -syyy_
customer_id  xxx_rev_A_yyy  xxx_rev_B_yyy  xxx_rev_C_yyy
1            2.00           3.00           0.00
2            0.00           0.00           6.00
3            0.00           9.00           9.00
4            11.00          0.00           0.00
```

#### Note on limits

The current parser limits each processed field to **4,999 bytes**, leaving one
byte for the string terminator. This covers cells (including headers) in simple
transpose and group, ID, and numeric values in aggregation. Grouped output can
contain at most **5,000 distinct groups** across the input; repeated groups do
not count toward this limit again. These limits apply to serial and parallel
processing. Exceeding a limit prints an error to standard error and exits with
status 1.


### License

Remember, tpose is free software (licensed under GPLv3)!
