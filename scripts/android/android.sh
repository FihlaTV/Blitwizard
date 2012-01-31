#!/bin/bash

# Android build script. Do not call directly, use create-release-archive.sh instead!

# Deal with overly curious users first:
if [ -z "$1" ]; then
    echo "Please don't run this directly. Run create-release-archive.sh instead."
    exit 1;
fi
if [ "$1" = "-h" ]; then
    echo "Please don't run this directly. Run create-release-archive.sh instead."
    exit 1;
fi
if [ "$1" = "-help" ]; then
    echo "Please don't run this directly. Run create-release-archive.sh instead."
    exit 1;
fi
if [ "$1" = "--help" ]; then
    echo "Please don't run this directly. Run create-release-archive.sh instead."
    exit 1;
fi

ANDROID_SDK_PATH="$1"
ANDROID_NDK_PATH="$2"

# Copy the project and SDL
rm -rf ./blitwizard-android/
cp -R blitwizard/src/sdl/android-project ./blitwizard-android/ || { echo "Failed to copy android-project"; exit 1; }
cp -R blitwizard/src/sdl/ ./blitwizard-android/jni/SDL/ || { echo "Failed to copy SDL"; exit 1; }
echo "APP_STL := stlport_shared" | cat - blitwizard-android/jni/Android.mk > /tmp/androidmk && mv /tmp/androidmk blitwizard-android/jni/Android.mk

# Copy android config
cp blitwizard-android/jni/SDL/include/SDL_config_android.h blitwizard-android/jni/SDL/include/SDL_config.h

# Ask the user some things:
echo "Type intended app name (or nothing) and then press [ENTER]:"
read app_name
if [ -z "$app_name" ]; then
	app_name="Blitwizard App"
fi
echo "App name will be $app_name"
echo "You can include a blitwizard game which blitwizard will run."
echo "For this, you can provide the path of a directory containing it."
echo "Please note !!ONLY THE GAME FILES AND templates/ FOLDER!!"
echo "should be in that directory, not any other files like blitwizard"
echo "binaries, examples folder or anything."
echo "Type a folder path or nothing and press [ENTER]:"
read game_files_path

if [ -n "$game_files_path" ]; then
	echo "Copying game files..."
	cp -R "$game_files_path"/* blitwizard-android/res/
	echo "Game files copied."
else
	echo "No game file path given, not integrating any resources."
fi

# Get the blitwizard version
blitwizard_version=`grep AC_INIT blitwizard/configure.ac | sed -e "s/AC_INIT[(][[]blitwizard[]], [[]//g" | sed -e "s/[]])//g"`

# Prepare vorbis
cp -R blitwizard/src/vorbis/ ./blitwizard-android/jni/vorbis/
cp android/Android-vorbis.mk ./blitwizard-android/jni/vorbis/Android.mk
rm blitwizard-android/jni/vorbis/lib/psytune.c # dead code (see comments inside)
rm blitwizard-android/jni/vorbis/lib/tone.c # program with main()
rm blitwizard-android/jni/vorbis/lib/barkmel.c # program with main()

# Vorbis define for ogg_types.h hack
cat blitwizard-android/jni/vorbis/lib/os.h | sed "s/\#define _OS_H/#define _OS_H\n#define VORBIS_HACK/g" > blitwizard-android/jni/vorbis/lib/os.h.2

# Prepare ogg
cp -R blitwizard/src/ogg/ ./blitwizard-android/jni/ogg/
cp android/Android-ogg.mk ./blitwizard-android/jni/ogg/Android.mk
cp android/ogg_config_types.h blitwizard-android/jni/ogg/include/ogg/config_types.h

# Prepare Box2D
cp -R blitwizard/src/box2d/ ./blitwizard-android/jni/box2d/
cp android/Android-box2d.mk ./blitwizard-android/jni/box2d/Android.mk

# Prepare png
cp -R blitwizard/src/imgloader/png/ ./blitwizard-android/jni/png/
rm blitwizard-android/jni/png/pngtest.c # program with main()
rm blitwizard-android/jni/png/pngvalid.c # we do not need this
cp android/Android-png.mk ./blitwizard-android/jni/png/Android.mk
cp blitwizard-android/jni/png/scripts/pnglibconf.h.prebuilt blitwizard-android/jni/png/pnglibconf.h

# Prepare zlib
cp -R blitwizard/src/imgloader/zlib/ ./blitwizard-android/jni/zlib/
rm blitwizard-android/jni/zlib/example.c # program with main()
cp android/Android-zlib.mk ./blitwizard-android/jni/zlib/Android.mk

mkdir blitwizard-android/jni/imgloader/
cp blitwizard/src/imgloader/*.c blitwizard-android/jni/imgloader/
cp blitwizard/src/imgloader/*.h blitwizard-android/jni/imgloader/
cp android/Android-imgloader.mk ./blitwizard-android/jni/imgloader/

# Prepare Lua
mkdir blitwizard-android/jni/lua/
cp blitwizard/src/lua/src/*.c blitwizard-android/jni/lua/
cp blitwizard/src/lua/src/*.h blitwizard-android/jni/lua/
rm blitwizard-android/jni/lua/lua.c
rm blitwizard-android/jni/lua/luac.c

# Blitwizard Android.mk:
source_file_list="`cat ../src/Makefile.am | grep blitwizard_SOURCES | sed -e 's/^blitwizard_SOURCES \= //'`"
cp blitwizard/src/*.c blitwizard-android/jni/src
cp blitwizard/src/*.h blitwizard-android/jni/src
cp android/Android-blitwizard.mk blitwizard-android/jni/src/Android.mk
cat blitwizard-android/jni/src/Android.mk | sed -e "s/SOURCEFILELIST/${source_file_list}/g" > blitwizard-android/jni/src/Android.mk
cat blitwizard-android/jni/src/Android.mk | sed -e "s/VERSIONINSERT/${blitwizard_version}/g" > blitwizard-android/jni/src/Android.mk

# Use the Android NDK/SDK to complete our project:
cd blitwizard-android
export HOST_AWK="awk"

# NDK build:
mv "$ANDROID_NDK_PATH/prebuilt/linux-x86/bin/awk" "$ANDROID_NDK_PATH/prebuilt/linux-x86/bin/awk_"
"$ANDROID_NDK_PATH/ndk-build" APP_STL=stlport_shared || { echo "NDK build failed."; exit 1; }

# Regenerate build.xml (SDL build.xml is outdated) and prepare some strings:
"$ANDROID_SDK_PATH/android" update project -p . --target android-10
cat blitwizard-android/build.xml | sed -e "s/SDLActivity/${app_name}/g" > blitwizard-android/build.xml
cat blitwizard-android/res/values/strings.xml  | sed -e "s/SDL App/${app_name}/g" > blitwizard-android/res/values/strings.xml

# Do final SDK build
echo "sdk.dir=$ANDROID_SDK_PATH" > local.properties
ant debug || { echo "ant failed."; exit 1; }
cd ..

# Success!
echo "Android .apk should be complete."
exit 0

