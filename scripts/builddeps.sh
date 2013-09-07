#!/bin/bash

# blitwizard game engine - dependency build script

cd ..

luatarget=`cat scripts/.buildinfo | grep luatarget | sed -e 's/^luatarget\=//'`
static_libs_use=`cat scripts/.buildinfo | grep STATIC_LIBS_USE | sed -e 's/^.*\=//'`
use_lib_flags=`cat scripts/.buildinfo | grep USE_LIB_FLAGS | sed -e 's/^.*\=//'`

changedhost=""

# Check if our previous build target was a different platform:
REBUILDALL="no"
if [ -f scripts/.depsarebuilt_luatarget ]; then
    depsluatarget=`cat scripts/.depsarebuilt_luatarget`
    if [ "$luatarget" != "$depsluatarget" ]; then
        # target has changed
        REBUILDALL="yes"
    fi
else
    # we had no previous known target at all, so obviously yes!
    REBUILDALL="yes"
fi

# Check if our previous build flags were different:
REBUILDCORE="no"
if [ -f scripts/.depsarebuilt_flags ]; then
    depsuselibflags=`cat scripts/.depsarebuilt_flags`
    if [ "$use_lib_flags" != "$depsuselibflags" ]; then
        # flags have changed
        REBUILDCORE="yes"
    fi
else
    # we had no previous known flags at all, so obviously yes!
    REBUILDCORE="yes"
fi

if [ "$REBUILDALL" = "yes" ]; then
    REBUILDCORE="yes"
    echo "Deps will be rebuilt to match new different target.";
    changedhost="yes";
    rm libs/libblitwizard*.a > /dev/null 2>&1
    rm libs/libimglib.a > /dev/null 2>&1
fi
if [ "$REBUILDCORE" = "yes" ]; then
    echo "Core will be rebuilt to match different build flags or target.";
    rm -f src/*.o > /dev/null 2>&1
fi

CC=`cat scripts/.buildinfo | grep CC | sed -e 's/^.*\=//'`
CXX=`cat scripts/.buildinfo | grep CXX | sed -e 's/^.*\=//'`
RANLIB=`cat scripts/.buildinfo | grep RANLIB | sed -e 's/^.*\=//'`
AR=`cat scripts/.buildinfo | grep AR | sed -e 's/^.*\=//'`
HOST=`cat scripts/.buildinfo | grep HOST | sed -e 's/^.*\=//'`
MACBUILD=`cat scripts/.buildinfo | grep MACBUILD | sed -e 's/^.*\=//'`

export CC="$CC"
export AR="$AR"

dir=`pwd`

echo "Will build static dependencies now."

# Remember for which target we built
echo "$luatarget" > scripts/.depsarebuilt_luatarget
echo "$use_lib_flags" > scripts/.depsarebuilt_flags

# libpng/zlib/imglib:
if [ ! -e libs/libimglib.a ]; then
    if [ -n "`echo $static_libs_use | grep imgloader`" ]; then
        # static png:
        if [ -n "`echo $static_libs_use | grep png`" ]; then
            echo "Compiling libpng..."
            CC="$CC" AR="$AR" cd src/imgloader && make deps-png || { echo "Failed to compile libpng"; exit 1; }
            cd "$dir"
        fi
        #static zlib
        if [ -n "`echo $static_libs_use | grep zlib`" ]; then
            echo "Compiling zlib..."
            CC="$CC" AR="$AR" cd src/imgloader && make deps-zlib || { echo "Failed to compile zlib"; exit 1; }
            cd "$dir"
        fi
        echo "Compiling imgloader..."
        # Our custom threaded image loader wrapper
        CC="$CC" AR="$AR" cd src/imgloader && make || { echo "Failed to compile imgloader"; exit 1; }
    fi
    cd "$dir"
fi

# Copy compiled imglib
if [ ! -e libs/libimglib.a ]; then
    if [ -n "`echo $static_libs_use | grep imgloader`" ]; then
        cp src/imgloader/libimglib.a libs/ || { echo "Failed to copy imglib"; exit 1; }
    fi
fi

# Copy compiled png/zlib
if [ ! -e libs/libblitwizardpng.a ]; then
    if [ -n "`echo $static_libs_use | grep png`" ]; then
        cp src/imgloader/libcustompng.a libs/libblitwizardpng.a || { echo "Failed to copy libpng"; exit 1; }
    fi
fi
if [ ! -e libs/libblitwizardzlib.a ]; then
    if [ -n "`echo $static_libs_use | grep zlib`" ]; then
        cp src/imgloader/libcustomzlib.a libs/libblitwizardzlib.a || { echo "Failed to copy zlib"; exit 1; }
    fi
fi

# Compile OIS:
if [ ! -e libs/libblitwizardOIS.a ]; then
    if [ -n "`echo $static_libs_use | grep OIS`" ]; then
        echo "Compiling OIS..."
        cd src/ois/src
        if [ "x$MACBUILD" = xyes ]; then
            # Linux
            $CXX -c *.cpp -I../includes || { echo "Failed to compile OIS"; exit 1; }
            cd mac
            $CXX -c *.cpp -I../../includes || { echo "Failed to compile OIS"; exit 1; }
            cd ..
            $AR rcs ../libOIS.a ./*.o ./linux/*.o
        else
            if [ "x$WINDOWSBUILD" = xyes ]; then
                # Windows
                $CXX -c *.cpp -I../includes || { echo "Failed to compile OIS"; exit 1; }
                cd win32
                $CXX -c *.cpp -I../../includes || { echo "Failed to compile OIS"; exit 1; }
                cd ..
                $AR rcs ../libOIS.a ./*.o ./linux/*.o
            else
                # Linux
                $CXX -c *.cpp -I../includes || { echo "Failed to compile OIS"; exit 1; }
                cd linux
                $CXX -c *.cpp -I../../includes || { echo "Failed to compile OIS"; exit 1; }
                cd ..
                $AR rcs ../libOIS.a ./*.o ./linux/*.o
            fi
        fi
        cd "$dir"
    fi
fi

# Copy OIS:
if [ ! -e libs/libblitwizardOIS.a ]; then
    if [ -n "`echo $static_libs_use | grep OIS`" ]; then
        cp src/ois/libOIS.a libs/libblitwizardOIS.a || { echo "Failed to copy OIS"; exit 1; }
    fi
fi

# Compile freetype:
if [ ! -e libs/libblitwizardfreetype.a ]; then
    if [ -n "`echo $static_libs_use | grep freetype`" ]; then
        echo "Compiling freetype..."
        cd src/freetype/ && ./configure --host="$host" --disable-shared --enable-static && make || { echo "Failed to compile freetype"; exit 1; }
        cd "$dir"
    fi
fi

# Copy compiled freetype
if [ ! -e libs/libblitwizardfreetype.a ]; then
    if [ -n "`echo $static_libs_use | grep freetype`" ]; then
        cp src/freetype/objs/.libs/libfreetype.a libs/libblitwizardfreetype.a || { echo "Failed to copy freetype"; exit 1; }
    fi
fi

# Ogre3D:
if [ ! -e libs/libblitwizardOgreMainStatic.a ]; then
    if [ -n "`echo $static_libs_use | grep Ogre3D`" ]; then
        cd src/ogre/
        rm ./CMakeCache.txt
        cmake -DOGRE_CONFIG_ENABLE_ZIP=FALSE -DOGRE_BUILD_PLUGIN_BSP=0 -DOGRE_CONFIG_DOUBLE=1 -DOGRE_BUILD_PLUGIN_PFX=0 -DOGRE_BUILD_PLUGIN_PCZ=1 -DOGRE_BUILD_PLUGIN_OCTREE=1 -DOGRE_CONFIG_ENABLE_FREEIMAGE=0 -DOGRE_CONFIG_ENABLE_DDS=0 -DOGRE_CONFIG_THREADS=2 -DOGRE_STATIC=on -DOGRE_BUILD_RENDERSYSTEM_GL=on -DOGRE_BUILD_TESTS=0 -DOGRE_BUILD_TOOLS=0 -DOGRE_BUILD_SAMPLES=0 -DOGRE_BUILD_PLUGIN_CG=off . || { echo "Failed to compile Ogre3D"; exit 1; }
        make || { echo "Failed to compile Ogre3D"; exit 1; }
        cd "$dir"
    fi
fi

# Copy Ogre3D:
if [ ! -e libs/libblitwizardOgreMainStatic.a ]; then
    if [ -n "`echo $static_libs_use | grep Ogre3D`" ]; then
        for f in src/ogre/lib/*
        do
            NEWNAME="libs/libblitwizard`basename $f .a`"
            NEWNAME="`echo $NEWNAME | sed -e 's/libblitwizardlib/libblitwizard/'`.a"
            cp "$f" "$NEWNAME"
        done
    fi
fi

# PhysFS:
if [ ! -e libs/libblitwizardPhysFS.a ]; then
    if [ -n "`echo $static_libs_use | grep PhysFS`" ]; then
        cd src/physfs/
        rm ./CMakeCache.txt
        rm -f *.o
        rm src/archiver_lzma.c
        $CC -c src/*.c -Isrc/ || { echo "Failed to compile PhysFS"; exit 1; }
        $AR rcs "$dir"/libs/libblitwizardPhysFS.a *.o || { echo "Failed to compile PhysFS"; exit 1; }
        rm *.o
        cd "$dir"
    fi
fi

# Copy PhysFS:
if [ ! -e libs/libblitwizardPhysFS.a ]; then
    if [ -n "`echo $static_libs_use | grep PhysFS`" ]; then
        cp src/physfs/libphysfs.a libs/libblitwizardPhysFS.a
    fi
fi

# bullet physics
if [ ! -e libs/libblitwizardBulletDynamics.a ] || [ ! -e libs/libblitwizardBulletCollision.a ] || [ ! -e libs/libblitwizardBulletSoftBody.a ] || [ ! -e libs/libblitwizardLinearMath.a ]; then
    if [ -n "`echo $static_libs_use | grep bullet`" ]; then
        cd src/bullet/
        sh autogen.sh && ./configure --host="$host" --disable-demos --disable-shared --enable-static --disable-debug && make || { echo "Failed to compile bullet physics"; exit 1; }
        cd "$dir"
    fi
fi

# Copy compiled bullet physics:
if [ ! -e libs/libblitwizardBulletDynamics.a ] || [ ! -e libs/libblitwizardBulletCollision.a ] || [ ! -e libs/libblitwizardBulletSoftBody.a ] || [ ! -e libs/libblitwizardLinearMath.a ]; then
    if [ -n "`echo $static_libs_use | grep bullet`" ]; then
        cp src/bullet/src/.libs/libBulletDynamics.a libs/libblitwizardBulletDynamics.a || { echo "Failed to copy bullet physics"; exit 1; }
        cp src/bullet/src/.libs/libBulletCollision.a libs/libblitwizardBulletCollision.a || { echo "Failed to copy bullet physics"; exit 1; }
        cp src/bullet/src/.libs/libBulletSoftBody.a libs/libblitwizardBulletSoftBody.a || { echo "Failed to copy bullet physics"; exit 1; }
        cp src/bullet/src/.libs/libLinearMath.a libs/libblitwizardLinearMath.a || { echo "Failed to copy bullet physics"; exit 1; }
    fi
fi

# libogg:
if [ ! -e libs/libblitwizardogg.a ]; then
    if [ -n "`echo $static_libs_use | grep ogg`" ]; then
        echo "Compiling libogg..."
        # Build ogg
        cd src/ogg && CC="$CC" ./configure --host="$HOST" --disable-shared --enable-static && make clean && make || { echo "Failed to compile libogg"; exit 1; }
        cd "$dir"
    fi
fi

# Copy compiled libogg:
if [ ! -e libs/libblitwizardogg.a ]; then
    if [ -n "`echo $static_libs_use | grep ogg`" ]; then
        cp src/ogg/src/.libs/libogg.a libs/libblitwizardogg.a || { echo "Failed to copy libogg"; exit 1; }
    fi
fi

# libFLAC:
if [ ! -e libs/libblitwizardFLAC.a ]; then
    if [ -n "`echo $static_libs_use | grep FLAC`" ]; then
        echo "Compiling libFLAC..."
        asmoption=""
        if [ "$MACBUILD" = "yes" ]; then
            # This doesn't work on Mac OS X as it seems
            asmoption="--disable-asm-optimizations"
        fi
        if [ ! -e "src/flac/configure" ]; then
            # we need to run autogen.sh first
            cd src/flac
            autoreconf -ivf || { if [ ! e "src/flac/configure" ]; then
                    echo "Failed to compile libFLAC - you might need to install autoconf/automake/libtool"; exit 1;
                fi
            }
            cd "$dir"
        fi
        if [ -n "`echo $static_libs_use | grep ogg`" ]; then
            # Build flac and tell it where ogg is
            oggincludedir="`pwd`/src/ogg/include/"
            ogglibrarydir="`pwd`/src/ogg/src/.libs/"
            cd src/flac && CC="$CC" ./configure --host="$HOST" $asmoption --with-ogg-libraries="$ogglibrarydir" --with-ogg-includes="$oggincludedir" --enable-static --disable-shared --disable-thorough-tests --disable-xmms-plugin --disable-cpplibs --disable-doxygen-docs && make clean && make || { echo "Failed to compile libFLAC"; exit 1; }
        else
            # Build flac and let it guess where ogg is
            cd src/flac && CC="$CC" ./configure --host="$HOST" $asmoption --enable-static --disable-shared --disable-thorough-tests --disable-xmms-plugin --disable-cpplibs --disable-doxygen-docs && make clean && make || { echo "Failed to compile libFLAC"; exit 1; }
        fi
        cd "$dir"
    fi
fi

# Copy compiled libFLAC:
if [ ! -e libs/libblitwizardFLAC.a ]; then
    if [ -n "`echo $static_libs_use | grep FLAC`" ]; then
        cp src/flac/src/libFLAC/.libs/libFLAC.a libs/libblitwizardFLAC.a || { echo "Failed to copy libFLAC"; exit 1; }
    fi
fi

# libspeex:
if [ ! -e libs/libblitwizardspeex.a ]; then
    if [ -n "`echo $static_libs_use | grep speex`" ]; then
        echo "Compiling libspeex..."
        if [ -n "`echo $static_libs_use | grep ogg`" ]; then
            # Build speex and remember to tell it where ogg is
            oggincludedir="`pwd`/src/ogg/include/"
            ogglibrarydir="`pwd`/src/ogg/src/.libs/"
            hostoption="--host=\"$HOST\""
            if [ -z "$HOST" ]; then
                hostoption=""
            fi
            configureline="CC=\"$CC\" ./configure $hostoption --with-ogg-libraries=\"$ogglibrarydir\" --with-ogg-includes=\"$oggincludedir\" --disable-oggtest --disable-shared --enable-static && make clean && make"
            echo "Speex configure line: $configureline"
            cd src/speex && eval $configureline || { echo "Failed to compile libspeex"; exit 1; }
        else
            # Build speex
            cd src/speex && CC="$CC" ./configure --host="$HOST" --disable-oggtest --disable-shared --enable-static && make clean && make || { echo "Failed to compile libspeex"; exit 1; }
        fi
        cd "$dir"
    fi
fi

# Copy compiled libspeex/libspeexdsp:
if [ ! -e libs/libblitwizardspeex.a ]; then
    if [ -n "`echo $static_libs_use | grep speex`" ]; then
        cp src/speex/libspeex/.libs/libspeex.a libs/libblitwizardspeex.a || { echo "Failed to copy libspeex"; exit 1; }
    fi
fi
if [ ! -e libs/libblitwizardspeexdsp.a ]; then
    if [ -n "`echo $static_libs_use | grep speex`" ]; then
        cp src/speex/libspeex/.libs/libspeexdsp.a libs/libblitwizardspeexdsp.a || { echo "Failed to copy libspeexdsp"; exit 1; }
    fi
fi

NOVORBIS="no"
if [ ! -e libs/libblitwizardvorbis.a ]; then
    NOVORBIS="yes"
fi
if [ ! -e libs/libblitwizardvorbisfile.a ]; then
    NOVORBIS="yes"
fi

# libvorbis/libvorbisfile:
if [ "$NOVORBIS" = "yes" ]; then
    if [ -n "`echo $static_libs_use | grep vorbis`" ]; then
        echo "Compiling libvorbis..."
        if [ -n "`echo $static_libs_use | grep ogg`" ]; then
            # Build vorbis and remember to tell it where ogg is
            oggincludedir="`pwd`/src/ogg/include/"
            ogglibrarydir="`pwd`/src/ogg/src/.libs/"
            oggpkgconfigpath="`pwd`/src/ogg/"
            if [ "$MACBUILD" != "yes" ]; then
                echo "./configure line:"
                echo "CC=\"$CC\" PKG_CONFIG_PATH=\"$oggpkgconfigpath\" ./configure --host=\"$HOST\" --with-ogg-libraries=\"$ogglibrarydir\" --with-ogg-includes=\"$oggincludedir\" --disable-oggtest --disable-docs --disable-examples --disable-shared --enable-static"
                cd src/vorbis && CC="$CC" PKG_CONFIG_PATH="$oggpkgconfigpath" ./configure --host="$HOST" --with-ogg-libraries="$ogglibrarydir" --with-ogg-includes="$oggincludedir" --disable-oggtest --disable-docs --disable-examples --disable-shared --enable-static && make clean && make || { echo "Failed to compile libvorbis"; exit 1; }
            else
                cd src/vorbis && CC="$CC" PKG_CONFIG_PATH="$oggpkgconfigpath" ./configure --with-ogg-libraries="$ogglibrarydir" --with-ogg-includes="$oggincludedir" --disable-oggtest --disable-docs --disable-examples --disable-oggtest --disable-shared --enable-static && make clean && make || { echo "Failed to compile libvorbis"; exit 1; }
            fi
        else
            # Build vorbis and let's hope it finds the system ogg
            if [ "$MACBUILD" != "yes" ]; then
                cd src/vorbis && CC="$CC" ./configure --host="$HOST" --disable-oggtest --disable-docs --disable-examples --disable-shared --enable-static && make clean && make || { echo "Failed to compile libvorbis"; exit 1; }
            else
                cd src/vorbis && CC="$CC" ./configure --disable-oggtest --disable-docs --disable-examples --disable-oggtest --disable-shared --enable-static && make clean && make || { echo "Failed to compile libvorbis"; exit 1; }
            fi
        fi
        cd "$dir"
    fi
fi

# Copy compiled libvorbis/libvorbisfile
if [ "$NOVORBIS" = "yes" ]; then
    if [ -n "`echo $static_libs_use | grep vorbis`" ]; then
        cp src/vorbis/lib/.libs/libvorbis.a libs/libblitwizardvorbis.a || { echo "Failed to copy libvorbis"; exit 1; }
        cp src/vorbis/lib/.libs/libvorbisfile.a libs/libblitwizardvorbisfile.a || { echo "Failed to copy libvorbisfile"; exit 1; }
    fi
fi

# libBox2D:
if [ ! -e libs/libblitwizardbox2d.a ]; then
    if [ -n "`echo $static_libs_use | grep box2d`" ]; then
        # Build box2d
        # cmake sucks (at least if you want to cross-compile),
        # therefore we do this manually:
        cd src/box2d
        bflags="-c -O2 -I."
        $CXX $bflags Box2D/Common/*.cpp || { echo "Failed to compile Box2D"; exit 1; }
        $CXX $bflags Box2D/Dynamics/*.cpp || { echo "Failed to compile Box2D"; exit 1; }
        $CXX $bflags Box2D/Dynamics/Contacts/*.cpp || { echo "Failed to compile Box2D"; exit 1; }
        $CXX $bflags Box2D/Dynamics/Joints/*.cpp || { echo "Failed to compile Box2D"; exit 1; }
        $CXX $bflags Box2D/Collision/*.cpp || { echo "Failed to compile Box2D"; exit 1; }
        $CXX $bflags Box2D/Collision/Shapes/*.cpp || { echo "Failed to compile Box2D"; exit 1; }
        $CXX $bflags Box2D/Rope/*.cpp || { echo "Failed to compile Box2D"; exit 1; }
        rm -f Box2D/libBox2D.a
        $AR rcs Box2D/libBox2D.a *.o || { echo "Failed to compile Box2D"; exit 1; }
        cd "$dir"
    fi
fi

# Copy compiled libBox2D:
if [ ! -e libs/libblitwizardbox2d.a ]; then
    if [ -n "`echo $static_libs_use | grep box2d`" ]; then
        BOX2DCOPIED1="true"
        BOX2DCOPIED2="true"
        cp src/box2d/Box2D/libBox2D.a libs/libblitwizardbox2d.a || { BOX2DCOPIED1="false"; }
        cp src/box2d/Box2D/Box2D.lib libs/libblitwizardbox2d.a || { BOX2DCOPIED2="false"; }
        if [ "$BOX2DCOPIED1" = "false" ]; then
            if [ "$BOX2DCOPIED2" = "false" ]; then
                echo "Failed to copy Box2D library"
                exit 1
            fi
        fi
    fi
fi

# libSDL:
if [ ! -e libs/libblitwizardSDL.a ]; then
    if [ -n "`echo $static_libs_use | grep SDL2`" ]; then
        # Build SDL 2
        if [ "$MACBUILD" != "yes" ]; then
            rm -r src/sdl/.hg/
            cd src/sdl && CC="$CC" ./configure --host="$HOST" --enable-assertions=release --enable-ssemath --disable-pulseaudio --enable-sse2 --disable-shared --enable-static || { echo "Failed to compile SDL2"; exit 1; }
        else
            rm -r src/sdl/.hg/
            cd src/sdl && CC="$CC" ./configure --enable-assertions=release --enable-ssemath --disable-pulseaudio --enable-sse2 --disable-mmx --disable-3dnow --disable-shared --enable-static || { echo "Failed to compile SDL2"; exit 1; }
        fi
        cd "$dir"
        cd src/sdl && make clean || { echo "Failed to compile SDL2"; exit 1; }
        cd "$dir"
        cd src/sdl && make || { echo "Failed to compile SDL2"; exit 1; }
        cd "$dir"
        mkdir -p src/include/
        mkdir -p src/include/sdl
        rm -r src/include/sdl/SDL2/
        cp -R src/sdl/include/ src/include/sdl/SDL2/ 
    fi
fi

# Copy compiled SDL library
if [ ! -e libs/libblitwizardSDL.a ]; then
    if [ -n "`echo $static_libs_use | grep SDL2`" ]; then
        cp src/sdl/build/.libs/libSDL2.a libs/libblitwizardSDL.a || { echo "Failed to copy SDL2 library"; exit 1; }
    fi
fi

# liblua:
if [ ! -e libs/libblitwizardlua.a ]; then
    if [ -n "`echo $static_libs_use | grep lua`" ]; then
        # Avoid the overly stupid Lua build script which doesn't even adhere to $CC
        cd src/lua/ && rm -f src/liblua.a && rm -rf build/ || { echo "Failed to compile Lua 5"; exit 1; }
        cd "$dir"
        cd src/lua/ && mkdir -p build/ && cp src/*.c build/ && cp src/*.h build/ || { echo "Failed to compile Lua 5"; exit 1; }
        cd "$dir"
        if [ -z "$AR" ]; then
            AR="ar"
        fi
        cd scripts
        SSEFLAGS=`sh checksse.sh`
        cd "$dir"
        cd src/lua/build && rm lua.c && rm luac.c && $CC -c -O2 $SSEFLAGS *.c && $AR rcs ../src/liblua.a *.o || { echo "Failed to compile Lua 5"; exit 1; }
        cd "$dir"
    fi
fi

# Copy compiled liblua:
if [ ! -e libs/libblitwizardlua.a ]; then
    if [ -n "`echo $static_libs_use | grep lua`" ]; then
        cp src/lua/src/liblua.a libs/libblitwizardlua.a
    fi
fi

# Wipe out the object files of blitwizard if we need to
if [ "$changedhost" = "yes" ]; then
    rm -f src/*.o
fi

# Remember for which target we built
echo "$luatarget" > scripts/.depsarebuilt_luatarget
echo "$use_lib_flags" > scripts/.depsarebuilt_flags

