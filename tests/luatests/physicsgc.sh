#!/bin/bash

# This test confirms os.ls() (a blitwizard lua api function) works.
# It lists the files in the templates/ directory using blitwizard/os.ls,
# then does the same in bash and compares the results.

source preparetest.sh

# Get output from blitwizard
echo "
-- create a static 3d and a movable 2d object
local obj1 = blitwizard.object:new(true)
obj1:enableStaticCollision({type="box",x_size=1,y_size=1,z_size=1})
local obj2 = blitwizard.object:new(false)
obj2:enableMovableCollision({type="rectangle",width=2,height=2})

-- a ray is also generating new references and affecting garbage collection:
blitwizard.physics.ray2d(-10, 0, 0, 0)

-- collect garbage until something happens
print(\"Physics GC test phase 1/2\")
local i = 1
while i < 1000 do
    collectgarbage()
    i = i + 1
end

-- Now nil one reference explicitely,
-- and delete the other object and nil it:
obj1 = nil
obj2:delete()
obj2 = nil

-- collect more garbage
print(\"Physics GC test phase 2/2\")
local i = 1
while i < 1000 do
    collectgarbage()
    i = i + 1
end
os.exit(0)
" > ./test.lua
$RUNBLITWIZARD ./test.lua
RETURNVALUE="$?"
rm ./test.lua

if [ "x$RETURNVALUE" = "x0" ]; then
    exit 0
else
    echo "Error: invalid test return value: $RETURNVALUE"
    exit 1
fi


