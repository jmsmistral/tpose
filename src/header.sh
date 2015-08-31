#!/bin/bash

filename=$1

if [[ -n "$filename" ]]; then
	head -n1 $filename | tr $'\t' $'\n' | cat -n
else
    echo "usage : header <filepath>"
fi
