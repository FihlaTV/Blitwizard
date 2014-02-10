#!/bin/bash

# This test checks if dofile works on a file in a script.

source luatests/preparetest.sh

# prepare test game:
mkdir -p ./dofiletest
echo "print(\"success\")" > ./dofiletest/file.lua
echo "
dofile(\"dofiletest/file.lua\")
os.exit(0)" > ./game.lua
zip -r -9 ./dofiletest.zip ./dofiletest/ ./game.lua
rm -r ./dofiletest/

# execute test:
cat $BINARYPATH ./dofiletest.zip > ./zipdofilebinary$EXEEXT
chmod +x ./zipdofilebinary$EXEEXT
./zipdofilebinary$EXEEXT
RETURNVALUE="$?"

# removing temporary files
rm ./game.lua
rm -r ./dofiletest.zip
rm -f ./zipdofilebinary$EXEEXT

# return value:
if [ "x$RETURNVALUE" = "x0" ]; then
    exit 0
else
    echo "Error: invalid test return value: $RETURNVALUE"
    exit 1
fi


