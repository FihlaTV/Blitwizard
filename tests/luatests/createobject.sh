#!/bin/bash

# This test confirms blitiwzard.object:new works.
# It will also test the init event function.

source preparetest.sh

# Run blitwizard test:
echo "local obj = blitwizard.object:new(false)
obj:setPosition(1, 2)
print(\"success\")
" > ./test.lua
$RUNBLITWIZARD ./test.lua > ./testoutput
rm ./test.lua

# Get output which we want to check for \"success\"
testoutput="`cat ./testoutput | grep success | sed 's/[ \n\r]*$//g'`"

rm ./testoutput

if [ "x$testoutput" = "xsuccess" ]; then
    exit 0
else
    echo "Error: invalid test output: '$testoutput'"
    exit 1
fi


