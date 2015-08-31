#!/bin/bash

filename=$1
fieldnum=$2
field=$3

if [[ -n "$filename" ]]; then
	echo "* Removing previous results..."
	rm test*
	echo "* Compiling tpose..."
	bash Makefile
	echo "* Running tpose..."
	time ./tpose -d$'\t' $filename --group="$field" > test_init.txt
	echo
	echo "* Processing results..."
	sort -u test_init.txt > test_sorted.txt
	cut -f2 test_sorted.txt | sort -n > testUHash.txt
	cut -f1 test_sorted.txt | sort > testUGroup.txt
	echo
	echo "* Running unix tools to get unique groups... "
	time cut -f$fieldnum $filename | sort -u > testUnixUGroup.txt
	echo
	echo "* Calculating differences between the two..."
	echo "* Tpose Lines: " && wc -l testUGroup.txt
	echo "* Unix tools Lines: " && wc -l testUnixUGroup.txt
	echo "* Diff of files..."
	diff testUGroup.txt testUnixUGroup.txt
else
    echo "usage : hashTest <filepath> <field number> <field value>"
fi
