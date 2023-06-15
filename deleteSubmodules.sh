#!/bin/bash
File="submodules.txt"
Lines=$(cat $File)
for Line in $Lines
do
	git rm -r --cached "$Line"
done
