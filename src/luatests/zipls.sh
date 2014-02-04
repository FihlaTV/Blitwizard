#!/bin/bash

# This test checks if blitwizard.debug.getTextureUsageInfo
# returns the correct value in a few basic cases.

source luatests/preparetest.sh

# prepare test game:
mkdir -p ./lstest
touch ./lstest/file1
touch ./lstest/file2
echo "
filelist = os.ls(\"lstest/\")
if #filelist ~= 2 then
    print(\"that didn't work\")
    os.exit(1)
end
print(\"success\")
os.exit(0)" > ./game.lua
zip -r -9 ./lstest.zip ./lstest/ ./game.lua
rm -r ./lstest/

# execute test:
cat $BINARYPATH ./lstest.zip > ./binarytest$EXEEXT
chmod +x ./binarytest$EXEEXT
./binarytest$EXEEXT
RETURNVALUE="$?"

# removing temporary files
rm ./game.lua
rm -r ./lstest.zip
rm ./binarytest$EXEEXT

# return value:
if [ "x$RETURNVALUE" = "x0" ]; then
    exit 0
else
    echo "Error: invalid test return value: $RETURNVALUE"
    exit 1
fi


