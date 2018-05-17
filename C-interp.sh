#!/bin/bash

trap safeClean 0 1 2 3 15

## create the directory where compiled files will go if it doesn't exist
DIRNAME=`mktemp -d /tmp/foo.XXXXXXX` 
safeClean(){

	## remove everything within /tmp/1ra
	rm -f $DIRNAME/*

	## remove the directory itself
	rm -d $DIRNAME
	
	echo "safe cleaned completed "
	exit 1
}

## check for -o option, if so, then default to /tmp/1ra
##if echo $@ | grep '\-o' >/dev/null
##	then
##		echo "USAGE: -o option is defaulted to random folder in tmp"
##fi

## compile the c code 
gcc $0.c -o $DIRNAME/$0.o

## execute if the .o file is successfully created
if [ -f $DIRNAME/$0.o ]
	then
		$DIRNAME/$0.o $@
fi

## remove sym link
## rm $0


