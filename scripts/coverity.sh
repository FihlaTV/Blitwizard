#!/bin/bash

COVERITY_PATH="/home/jonas/Develop/coverity/cov-analysis-linux64-6.6.1/bin"

if [ x$BASH = "x/bin/bash" ];
then
    echo "/bin/bash detected..."
else
    echo "Error: only run this with /bin/bash to avoid breakage. Thanks."
    exit 1
fi

export PATH="$COVERITY_PATH:$PATH"

./configure || { echo "CONFIGURE FAILED."; exit 1; }
rm -rf ./cov-int || { echo "FAILED TO REMOVE INTERMEDIATE DIR"; exit 1; }
rm -f ./blitwizard-cov.tgz
currentdir="`pwd`"
cd src && make clean || { echo "MAKE CLEAN FAILED."; exit 1; }
cd "$currentdir"
cov-build --dir cov-int make || { echo "COVERITY RUN FAILED."; exit 1; }

read -p "Upload build to coverity? [y/N]" -r
if [[ $REPLY =~ ^[Yy]$ ]]
then
    token=""
    while [[ x$token = "x" ]]
    do
        read -p "Enter coverity token:" -r
        token="$REPLY"
    done
    version="`git rev-parse HEAD`"
    tar czvf blitwizard-cov.tgz cov-int/
    read -p "Will upload with version '$version' and token '$token'. Continue? [y/N]" -r
    if [[ $REPLY =~ ^[Yy]$ ]]
    then
        curl --form project=Blitwizard --form token="$token" --form email=jonasthiem@googlemail.com --form file=@blitwizard-cov.tgz --form version="$version" --form description="dev build" http://scan5.coverity.com/cgi-bin/upload.py
    fi
fi
