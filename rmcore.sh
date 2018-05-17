#!/bin/bash

## short script to remove some core dump files in the current directory
OUTPUT=`ls | grep "^core\.[0-9]*$"`
echo $OUTPUT
rm $OUTPUT

