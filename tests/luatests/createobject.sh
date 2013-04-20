#!/bin/bash

# This test confirms os.ls() (a blitwizard lua api function) works.
# It lists the files in the templates/ directory using blitwizard/os.ls,
# then does the same in bash and compares the results.

source preparetest.sh

# Run blitwizard test:
echo "local obj = blitwizard.object:new(false, nil, function()
    print(\"success: \" .. tostring(self))
end)
obj:setPosition(1, 2)
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


