#!/bin/bash

# This test confirms blitwizard.object:getMass returns
# the value previously set through blitwizard.object:setMass

source preparetest.sh

# Run blitwizard test:
echo "-- don't spill out log output:
function blitwizard.onLog() end

-- create object with rectangle shape:
local obj = blitwizard.object:new(blitwizard.object.o2d)
obj:enableMovableCollision({
    type='rectangle', width=2, height=2
})

-- set and get mass:
obj:setMass(5)
assert(5 == obj:getMass())

-- set graphics mode so blitwizard keeps running:
blitwizard.graphics.setMode(20, 20, 'test', false)

-- get mass delayed:
function obj:doAlways()
    assert(5 == self:getMass())
    print(\"success\")
    os.exit(0)
end

-- get mass after setting friction:
obj:setFriction(0.7)
assert(5 == obj:getMass())
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


