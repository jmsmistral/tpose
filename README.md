# Welcome to tpose home! #

tpose is a UNIX-based terminal program for transposing delimited text-files.

Read this article ([medium](https://medium.com/@jmsmistral/on-transposing-data-884a04a8c1bf) or [pdf](http://jonathansacramento.com/papers/on_transposing_data.pdf)) to learn more about transposing data in the UNIX environment (and tpose!).

## Building tpose ##

* Dependencies: gcc, make

Most of you will just need to download or clone the source code and do:

```
#!bash

$ cd tpose
$ make && sudo make install
```
In case you get issues, check [here](https://bitbucket.org/jmsmistral/tpose/wiki/Home) for info on how to get set-up and troubleshooting build issues.

## Running tpose ##

tpose usage pattern:
```
#!bash

tpose input-file [output-file] [-IGNdiapsPhv]
```
Get more details on the different options by running:
```
#!bash

$ tpose --help
```

#### Simple transpose ####
```
#!bash

$ cat data_ex1_simple.txt
Quarter	Europe	Asia	US
Q1	2	5	3
Q2	3	4	1
Q3	3	5	2
Q4	4	6	3

$ tpose data_ex1_simple.txt
Quarter	Q1	Q2	Q3	Q4
Europe	2	3	3	4
Asia	5	4	5	6
US	3	1	2	3

```

#### Transpose over GROUP field ####
```
#!bash

$ cat data_ex2_group.txt
Customer_id	Revenue_group	Amount
1	rev_A	2
1	rev_B	3
2	rev_C	6
3	rev_B	7
3	rev_B	2
3	rev_C	9
4	rev_A	8
4	rev_A	3

$ tpose data_ex2_group.txt -Grevenue_group -Namount
rev_A	rev_B	rev_C
13.00	12.00	15.00
```

#### Transpose over GROUP and ID field ####
```
#!bash

$ cat data_ex2_group.txt
Customer_id	Revenue_group	Amount
1	rev_A	2
1	rev_B	3
2	rev_C	6
3	rev_B	7
3	rev_B	2
3	rev_C	9
4	rev_A	8
4	rev_A	3

$ tpose data_ex2_group.txt -Icustomer_id -Grevenue_group -Namount
customer_id	rev_A	rev_B	rev_C
1	2.00	3.00	0.00
2	0.00	0.00	6.00
3	0.00	9.00	9.00
4	11.00	0.00	0.00
```

#### Field indexes instead of names ####
Use the -i or --indexed option.
```
#!bash

$ cat data_ex2_group.txt
Customer_id	Revenue_group	Amount
1	rev_A	2
1	rev_B	3
2	rev_C	6
3	rev_B	7
3	rev_B	2
3	rev_C	9
4	rev_A	8
4	rev_A	3

$ tpose data_ex2_group.txt -i -I1 -G2 -N3
customer_id	rev_A	rev_B	rev_C
1	2.00	3.00	0.00
2	0.00	0.00	6.00
3	0.00	9.00	9.00
4	11.00	0.00	0.00
```

#### Different types of aggregation ####
Use the -a or --aggregate option followed by 'sum' (default), 'count', or 'avg'.

* COUNT
Counts group field instances instead of summing the NUMERICAL field values. 
```
#!bash

$ cat data_ex2_group.txt
Customer_id	Revenue_group	Amount
1	rev_A	2
1	rev_B	3
2	rev_C	6
3	rev_B	7
3	rev_B	2
3	rev_C	9
4	rev_A	8
4	rev_A	3

$ tpose data_ex2_group.txt -i -I1 -G2 -N3 -acount
customer_id	rev_A	rev_B	rev_C
1	1	1	0
2	0	0	1
3	0	2	1
4	2	0	0
```

* AVG
Divides the sum of the NUMERICAL field values by the count of GROUP field instances (division by zero result in 'Not-A-Number' or nans)
```
#!bash

$ cat data_ex2_group.txt
Customer_id	Revenue_group	Amount
1	rev_A	2
1	rev_B	3
2	rev_C	6
3	rev_B	7
3	rev_B	2
3	rev_C	9
4	rev_A	8
4	rev_A	3

$ tpose data_ex2_group.txt -i -I1 -G2 -N3 -aavg
customer_id	rev_A	rev_B	rev_C
1	2.00	3.00	nan
2	nan	nan	6.00
3	nan	4.50	9.00
4	5.50	nan	nan
```

#### Parallel execution ####
Use the -P or --parallel option (only works for files >1GB). This example prints to an output file instead of the screen.
```
#!bash

$ ls -l data_large.txt
-rw-r--r--  1 jonathan  staff    14G 25 Sep 21:28 data_large.txt

$ tpose data_large.txt output_tpose.txt -P -i -I1 -G15 -N32

$ ls -l output_tpose.txt
-rw-r--r--  1 jonathan  staff   110M 25 Sep 21:30 output_tpose.txt
```

#### Changing delimiter ####
Use the -d or --delimiter option.
```
#!bash

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
```
#!bash

$ cat data_ex2_group.txt
Customer_id	Revenue_group	Amount
1	rev_A	2
1	rev_B	3
2	rev_C	6
3	rev_B	7
3	rev_B	2
3	rev_C	9
4	rev_A	8
4	rev_A	3

$ tpose data_ex2_group.txt -i -I1 -G2 -N3 -pxxx_ -syyy_
customer_id	xxx_rev_A_yyy	xxx_rev_B_yyy	xxx_rev_C_yyy
1	2.00	3.00	0.00
2	0.00	0.00	6.00
3	0.00	9.00	9.00
4	11.00	0.00	0.00
```

## Bug reports ##
If you have any bug reports or feature requests, please email me at: **jmsmistral@gmail.com**, with "TPOSE BUG" or "TPOSE FEATURE" somewhere in the subject. 
Alternatively, submit a pull-request.

Remember, tpose is free software (licensed under GPLv3)!