#!/bin/bash

# This test checks if the physics objects get garbage collected.
# Mainly, it just verifies blitwizard doesn't crash or outright error.

source luatests/preparetest.sh

# Lua test code:
echo "
-- create a static 3d and a movable 2d object
local obj1 = blitwizard.object:new(blitwizard.object.o3d)
local success = pcall(function()
    -- attempt to activate 3d collision:
    obj1:enableStaticCollision({type="box",x_size=1,y_size=1,z_size=1})
end)
if not success then
    -- apparently, 3d collision is disabled
    print(\"WARNING: This build has no 3d collision, test cannot run meaningfully.\")
    os.exit(0)
end
local obj2 = blitwizard.object:new(blitwizard.object.o2d)
local success = pcall(function()
    obj2:enableMovableCollision({type="rectangle",width=2,height=2})
end)
if not success then
    -- apparently, 3d collision is disabled
    print(\"WARNING: This build has no 2d collision, test cannot run meaningfully.\")
    os.exit(0)
end

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


