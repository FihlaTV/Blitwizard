#!/bin/bash

# This test checks if blitwizard.debug.getTextureUsageInfo
# returns the correct value in a few basic cases.

source preparetest.sh

# Run blitwizard test:
echo "-- don't spill out log output:
function blitwizard.onLog() end

-- if there is no console present, we cannot run this test:
if blitwizard.console == nil then
    -- try to load console
    cwd = os.getcwd()
    local result,errmsg = pcall(function()
        os.chdir(\"../\")
        dofile(\"templates/init.lua\")
    end)
    if result ~= true then
        os.chdir(cwd)
        error \"cannot run this test without templates/console\"
    end
    os.chdir(cwd)
end

-- open up graphics
blitwizard.graphics.setMode(640, 480, \"Test\", false)

blitwizard.runDelayed(function()
    -- verify console.png is not visible
    if pcall(function()
        assert(blitwizard.debug.getTextureUsageInfo(os.templatedir() ..
        \"/font/default.png\") < 0)
    end) ~= true then
        print(\"console image is reported as used and shouldn't be, at: \" ..
        os.time())
        os.exit(1)
    end

    -- open up console:
    blitwizard.console.open()

    blitwizard.runDelayed(function()
        -- verify console.png is now visible:
        if pcall(function()
            assert(blitwizard.debug.getTextureUsageInfo(os.templatedir() ..
            \"/font/default.png\") >= 0)
        end) ~= true then
            print(\"console image isn't reported as used and should be\")
            os.exit(1)
        end

        -- done!
        print(\"success\")
        os.exit(0)
    end, 500)
end, 5000)
" > ./test.lua
$RUNBLITWIZARD ./test.lua &> ./testoutput
echo "Lua script output: `cat ./testoutput`"
rm ./test.lua

# Get output which we want to check for \"success\"
testoutput="`cat ./testoutput | grep success | sed 's/[ \n\r]*$//g'`"

rm ./testoutput

if [ "x$testoutput" = "xsuccess" ]; then
    exit 0
else
    echo "Error: invalid test output, doesn't contain success."
    exit 1
fi


