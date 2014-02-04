#!/bin/bash

# This test checks whether obj:isVisible() and obj:getZIndex() work.

source luatests/preparetest.sh

# Get output from blitwizard
echo "
-- check visibility with 3d object
local obj1 = blitwizard.object:new(blitwizard.object.o3d)
if obj1:getVisible() ~= true then
    error(\"object should be initially visible\")
end
obj1:setVisible(false)
if obj1:getVisible() == true then
    error(\"object should remember being set to invisible\")
end
local result = pcall(function()
    obj1:setZIndex(5)
end)
if result ~= false then
    error(\"3d objects should not allow setting a z index\")
end

-- check zindex with 2d object:
local obj2 = blitwizard.object:new(blitwizard.object.o2d, \"orb.png\")
obj2:setZIndex(555)
local result = obj2:getZIndex()
if result ~= 555 then
    error(\"object should remember its zindex (expected z index: 555, actual z index: \" .. tostring(result) .. \")\")
end

print(\"Test successful.\")
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


