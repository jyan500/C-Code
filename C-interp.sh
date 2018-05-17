#!/bin/bash

trap safeClean 0 1 2 3 15

## Perform ln -s C-interp C-File, 
## where C-File is the name of your .c file without the ".c"
## the symbolic link C-File will compile and run the code

## create the directory where compiled files will go if it doesn't exist
DIRNAME=`mktemp -d /tmp/foo.XXXXXXX` 
safeClean(){

	## remove everything within /tmp/1ra
	rm -f $DIRNAME/*

	## remove the directory itself
	rm -d $DIRNAME
	
	exit 1
}

## compile the c code 
gcc $0.c -o $DIRNAME/$0.o

## execute if the .o file is successfully created
if [ -f $DIRNAME/$0.o ]
	then
		$DIRNAME/$0.o $@
fi

## remove sym link
## rm $0


