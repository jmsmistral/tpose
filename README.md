# Welcome to tpose home! #

tpose is a UNIX-based terminal program for transposing delimited text-files.

Read this [article](http://jonathansacramento.com/papers/on_transposing_data.pdf) to learn more about transposing data in the UNIX environment (and tpose!).

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

** Simple transpose **
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

** Transpose over GROUP field **
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

** Transpose over GROUP and ID field **
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