
# blitwizard configure.ac - autoconf generated, extended by Jonas Thiem et al
# Process this file with autoreconf to produce a configure script
# (see autogen.sh)

AC_PREREQ([2.60])

AC_INIT([blitwizard], [0.4], [jonasthiem@googlemail.com])
m4_pattern_allow([AC_DEFINE_DIR])
m4_include([m4/ac_define_dir.m4])

# Checks for programs, various options:

AM_INIT_AUTOMAKE([foreign])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
#AC_PROG_STRIP
#MACTEST=`uname -a | grep Darwin`
AC_PROG_OBJC
#_AM_DEPENDENCIES(OBJC)
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])
STRIP="strip"
AC_CONFIG_HEADERS([config.h])

# Evaluate target os:

maybeos=`echo $EXEEXT | grep exe`
AS_IF([test -z "$maybeos"],[
	# Unix
	WINDOWS="no"
],[
	# Windows
	WINDOWS="yes"
])


# Remove -g from default CFLAGS since we control that through --disable-debug
CFLAGS="${CFLAGS/-g}"
CXXFLAGS="${CXXFLAGS/-g}"

# Remove -O2 from default CFLAGS since we automatically detect O3 support
# (and if present, we want to use it)
CFLAGS="${CFLAGS/-O2}"
CXXFLAGS="${CXXFLAGS/-O2}"

# Check for ar:

AC_CHECK_TOOL(AR, ar)
if [ test -z "$AR" ]; then
    AC_MSG_ERROR([No ar available. Please install ar.])
    exit 1
fi

# Enable/disable features:

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--disable-debug],[compile without debugging symbols. Results in smaller blitwizard binary, but you can no longer properly debug a blitwizard crash])],
[case "${enableval}" in
     yes | no ) enable_debug="${enableval}" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --disable-debug) ;;
 esac],
[enable_debug=yes])

AC_ARG_ENABLE([sdl-graphics],
    [AS_HELP_STRING([--disable-sdl-graphics],[disable 2d graphics support through SDL (default is enabled). If you compile with Ogre, you can still use 2d graphics through Ogre])],
[case "${enableval}" in
    yes | no ) enable_sdlgraphics="${enableval}" ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --disable-sdl-graphics) ;;
esac],
[enable_sdlgraphics=yes])

AC_ARG_ENABLE([ogre-graphics],
    [AS_HELP_STRING([--disable-ogre-graphics],[disable 2d/3d graphics support through Ogre (default is enabled)])],
[case "${enableval}" in
    yes | no ) enable_ogregraphics="${enableval}" ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --disable-ogre-graphics) ;;
esac],
[enable_ogregraphics=yes])

AC_ARG_ENABLE([sdl-audio],
    [AS_HELP_STRING([--disable-sdl-audio],[disable audio support through SDL (default is enabled)])],
[case "${enableval}" in
     yes | no ) enable_sdlaudio="${enableval}" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --disable-sdl-audio) ;;
 esac],
[enable_sdlaudio=yes])

AC_ARG_ENABLE([sdl],
    [AS_HELP_STRING([--disable-sdl],[disable SDL completely. This is the same as --disable-sdl-audio, --disable-sdl-graphics combined])],
[case "${enableval}" in
     yes | no ) enable_sdl="${enableval}" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --disable-sdl) ;;
 esac],
[enable_sdl=yes])

AS_IF([test "x$enable_sdl" = xno], [
    enable_sdlaudio="no"
    enable_sdlgraphics="no"
])

AC_ARG_ENABLE([graphics],
    [AS_HELP_STRING([--disable-graphics],[disable graphics support completely, including the null graphics device (default is enabled)])],
[case "${enableval}" in
     yes | no ) enable_graphics="${enableval}" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --disable-graphics) ;;
 esac],
[enable_graphics=yes])

AS_IF([test "x$enable_graphics" = xno], [
    enable_sdlgraphics="no"
    enable_ogregraphics="no"
])

AC_ARG_ENABLE([audio],
    [AS_HELP_STRING([--disable-audio],[disable audio support completely, including the null audio device (default is enabled)])],
[case "${enableval}" in
     yes | no ) enable_audio="${enableval}" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --disable-audio) ;;
 esac],
[enable_audio="yes"])

AS_IF([test "x$enable_audio" = xno], [
    enable_sdlaudio="no"
])

AC_ARG_ENABLE([physics2d],
    [AS_HELP_STRING([--disable-physics2d],[disable physics simulation support based on Box2D for 2d objects (default is enabled)])],
[case "${enableval}" in
     yes | no ) enable_physics2d="${enableval}" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --disable-physics2d) ;;
 esac],
[enable_physics2d=yes])

AC_ARG_ENABLE([physics3d],
    [AS_HELP_STRING([--disable-physics3d],[disable physics simulation support based on bullet physics for 3d objects (default is enabled)])],
[case "${enableval}" in
     yes | no ) enable_physics3d="${enableval}" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --disable-physics3d) ;;
 esac],
[enable_physics3d=yes])

AC_ARG_ENABLE([physics],
    [AS_HELP_STRING([--disable-physics],[disable physics simulation support completely])],
[case "${enableval}" in
    yes | no ) enable_physics="${enableval}" ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --disable-physics) ;;
 esac],
[enable_physics=yes])


AS_IF([test "x$enable_physics" = xno], [
    enable_physics2d="no"
    enable_physics3d="no"
])

AC_ARG_ENABLE([physfs],
    [AS_HELP_STRING([--disable-physfs],[disable optional PhysFS support which enables reading resources from .zip files])],
[case "${enableval}" in
    yes | no ) enable_physfs="${enableval}" ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --disable-physfs) ;;
 esac],
[enable_physfs=yes])

AC_ARG_ENABLE([flac],
    [AS_HELP_STRING([--disable-flac],[disable optional FLAC audio support through libFLAC. If FFmpeg support is enabled and FFmpeg is available at runtime, you might still be able to play FLAC audio])],
[case "${enableval}" in
     yes | no ) enable_flac="${enableval}" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --disable-flac) ;;
 esac],
[enable_flac=yes])

AC_ARG_ENABLE([speex],
    [AS_HELP_STRING([--disable-speex],[disable optional Speex support for proper resampling. Use only if you want to supply all your audio in 48kHz to avoid the need for resampling entirely])],
[case "${enableval}" in
     yes | no ) enable_speex="${enableval}" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --disable-speex) ;;
 esac],
[enable_speex=yes])

# Auto-disable Ogre3d, 3d physics:

AS_IF([test xyes = "x$enable_ogregraphics"],[
    echo "**************************************************"
    echo "  AUTO-DISABLING 3D GRAPHICS (not supported yet)  "
    echo "**************************************************"
    enable_ogregraphics="no"
])
AS_IF([test xyes = "x$enable_physics3d"],[
    echo "*************************************************"
    echo "  AUTO-DISABLING 3D PHYSICS (not supported yet)  "
    echo "*************************************************"
    enable_physics3d="no"
])


# Remember which libs we want to build ourselves for static use:

STATIC_LIBS_USE=""


# Remember which flags we need because we don't use some libs at all

SDL_GRAPHICS_FEATURE_ENABLED="yes"
OGRE_GRAPHICS_FEATURE_ENABLED="yes"
NULL_GRAPHICS_FEATURE_ENABLED="yes"
AUDIO_FEATURE_ENABLED="yes"
AUDIO_FEATURE_ENABLED_TYPE="SDL2"
NULL_AUDIO_FEATURE_ENABLED="yes"
FLAC_FEATURE_ENABLED="yes"
SPEEX_RESAMPLING_FEATURE_ENABLED="yes"
FFMPEG_FEATURE_ENABLED="yes"
PHYSICS2D_FEATURE_ENABLED="yes"
PHYSICS3D_FEATURE_ENABLED="yes"
FAST_FLOAT2INT_FEATURE_ENABLED="yes"
ZIP_FEATURE_ENABLED="yes"


# Lib deps info text for later output:

LIBS_DEP_INFO_TEXT="
*** Dependency scan results ***"


# Check whether ffmpeg is available

AS_IF([test -e "src/ffmpeg/libavcodec/avcodec.h"],[
    AC_MSG_NOTICE([FFmpeg source code appears to be present, will compile with FFmpeg support.])
    FFMPEG_CONFIG_FAILED="no"
    AS_IF([test ! -e "src/ffmpeg/libavutil/avconfig.h"], [
        AC_WARN([FFmpeg is not configured yet. FFmpeg will be now configured (but not built) with recommended options. If you want to do that yourself, please cancel and do it now, then rerun blitwizard ./configure.])
        CONFIGUREFFMPEG="cd src/ffmpeg && ./configure --disable-libgsm --enable-runtime-cpudetect  --enable-version3 --disable-gpl --enable-shared --disable-doc --disable-ffmpeg --disable-avconv --disable-ffplay --disable-ffprobe --disable-ffserver --disable-protocols --disable-devices;"
        eval $CONFIGUREFFMPEG || { FFMPEG_CONFIG_FAILED="yes"; }
        cd ../../
        # Recent versions of FFmpeg no longer know about --disable-avconf:
        AS_IF([test "x$FFMPEG_CONFIG_FAILED" = xyes], [ 
            FFMPEG_CONFIG_FAILED="no"
            CONFIGUREFFMPEG="cd src/ffmpeg && ./configure --disable-libgsm --enable-runtime-cpudetect --enable-version3 --disable-gpl --enable-shared --disable-doc --disable-ffmpeg --disable-ffplay --disable-ffprobe --disable-ffserver --disable-protocols --disable-devices;"
            eval $CONFIGUREFFMPEG || { FFMPEG_CONFIG_FAILED="yes"; }
            cd ../../
        ]) 
    ])
    AS_IF([test "x$FFMPEG_CONFIG_FAILED" = xno],[
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  FFmpeg headers from tarball: yes"
    ],[
        AC_WARN([Failed to configure FFmpeg. Blitwizard will NOT have any FFmpeg support])
        LIBS_DEP_INFO_TEXT="${LIPS_DEP_INFO_TEXT}
  FFmpeg headers from tarball: no"
        FFMPEG_FEATURE_ENABLED="no, FFmpeg configure in tarball failed" 
    ])
],[
    AC_WARN([FFmpeg source code is not present or not properly placed. Blitwizard will NOT have any FFmpeg support.])
    LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  FFmpeg headers from tarball: no"
    FFMPEG_FEATURE_ENABLED="no, tarball not found"
])

# See if we want libSDL or not:
NEEDSDL="no"
AS_IF([test "x$enable_sdlaudio" = xyes], [
    NEEDSDL="yes"
])
AS_IF([test "x$enable_sdlgraphics" = xyes], [
    NEEDSDL="yes"
])
AS_IF([test "x$enable_sdlgraphics" = xno],[
    SDL_GRAPHICS_FEATURE_ENABLED="no, disabled"
])
AS_IF([test "x$enable_sdlaudio" = xno],[
    # Windows can do audio without SDL (WaveOut):
    AS_IF([test "x$WINDOWS" = xno],[
        AUDIO_FEATURE_ENABLED="no, disabled"
    ],[
        AUDIO_FEATURE_ENABLED="yes"
        AUDIO_FEATURE_ENABLED_TYPE="waveOut"
    ])
])

# See if we want Ogre3D:
AS_IF([test "x$enable_ogregraphics" = xno],[
    OGRE_GRAPHICS_FEATURE_ENABLED="no, disabled"
])

# Check FreeType:
COMPILE_OGRE="yes"
AS_IF([test "x$enable_ogregraphics" = xyes],[
    AC_MSG_CHECKING([for static freetype library])
    AS_IF([test ! -f "src/freetype/src/base/ftinit.c"], [
        AC_MSG_RESULT([no])
        AC_CHECK_LIB([freetype], [FT_Init_FreeType],[
            #shared lib is there!
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  freetype: shared"
        ],[
            #nothing there
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  freetype: no"
            OGRE_GRAPHICS_FEATURE_ENABLED="no, need freetype"
            COMPILE_OGRE="no"
        ])

    ],[
        AC_MSG_RESULT([yes])
        STATIC_LIBS_USE="${STATIC_LIBS_USE} freetype"
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  freetype: static"
    ])
],[
    COMPILE_OGRE="no"
])

# Check for various libs we need for graphics:
AS_IF([test "x$enable_graphics" = xyes],[ # START enable_graphics block
    # Check for libpng availability:

    AC_MSG_CHECKING([for static png library])
    AS_IF([test ! -f "src/imgloader/png/png.c"],[
        AC_MSG_RESULT([no])
        AC_CHECK_LIB([png], [png_create_read_struct],[
            #shared lib is there!
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libpng: shared"
        ],[
            #nothing there
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libpng: no"
            SDL_GRAPHICS_FEATURE_ENABLED="no, need libpng"
            OGRE_GRAPHICS_FEATURE_ENABLED="no, need libpng"
            NULL_GRAPHICS_FEATURE_ENABLED="no, need libpng"
            GRAPHICS_FEATURE_ENABLED="no, need libpng"
            COMPILE_OGRE="no"
            NEEDSDL="no"
        ])
    ],[
        AC_MSG_RESULT([yes])
        STATIC_LIBS_USE="${STATIC_LIBS_USE} png"
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libpng: static"
    ])


    # Check for zlib availability:

    AC_MSG_CHECKING([for static zlib library])
    AS_IF([test ! -f "src/imgloader/zlib/gzlib.c"],[
        AC_MSG_RESULT([no])
        AC_CHECK_HEADERS([zlib.h],[
            #shared lib is there!
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  zlib: shared"
        ],[
            #nothing there
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  zlib: no"
            GRAPHICS_FEATURE_ENABLED="no, need zlib"
            NULL_GRAPHICS_FEATURE_ENABLED="no, need zlib"
            SDL_GRAPHICS_FEATURE_ENABLED="no, need zlib"
            OGRE_GRAPHICS_FEATURE_ENABLED="no, need zlib"
            COMPILE_OGRE="no"
            NEEDSDL="no"
        ])
    ],[
        AC_MSG_RESULT([yes])
        STATIC_LIBS_USE="${STATIC_LIBS_USE} zlib"
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  zlib: static"
    ])

],[ # STOP enable_graphics block
    GRAPHICS_FEATURE_ENABLED="no, disabled"
    SDL_GRAPHICS_FEATURE_ENABLED="no, disabled"
    OGRE_GRAPHICS_FEATURE_ENABLED="no, disabled"
    NULL_GRAPHICS_FEATURE_ENABLED="no, disabled"
    COMPILE_OGRE="no"
    NEEDSDL="no"
])



# Check OIS:
AS_IF([test "x$COMPILE_OGRE" = xyes], [
    AC_MSG_CHECKING([for static OIS library])
    AS_IF([test ! -f "src/ois/src/OISKeyboard.cpp"],[
        AC_MSG_RESULT([no])
        AC_LANG_PUSH([C++])
        AC_CHECK_HEADERS([OIS.h],[
            #shared lib is there!
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  OIS: shared"
        ],[
            #nothing there
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  OIS: no"
            OGRE_GRAPHICS_FEATURE_ENABLED="no, need OIS"
            COMPILE_OGRE="no"
        ])
        AC_LANG_POP([C++])
    ],[
        AC_MSG_RESULT([yes])
        STATIC_LIBS_USE="${STATIC_LIBS_USE} OIS"
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  OIS: static"
    ])
])

# Check various X headers (only on Unix):
AS_IF([test -z "$maybeos"],[
    # Check Xaw headers for Ogre3D:
    AS_IF([test "x$COMPILE_OGRE" = xyes], [
        AC_CHECK_HEADERS([X11/Xaw/Form.h],,[
            AC_WARN([Cannot find Xaw header files required for Ogre3D. No Ogre support possible.])
            OGRE_GRAPHICS_FEATURE_ENABLED="no, need Xaw headers"
            COMPILE_OGRE="no"
        ])
    ])

    # Check X11, Xext headers for Ogre3D and SDL:
    NEED_XHEADERS="no"
    AS_IF([test "x$COMPILE_OGRE" = xyes], [
        NEED_XHEADERS="yes"
    ])
    AS_IF([test "x$NULL_GRAPHICS_FEATURE_ENABLED" = xyes],[
        NEED_XHEADERS="yes"
    ])
    AC_CHECK_HEADERS([X11/Xlib.h],,[
        AC_WARN([Cannot find Xlib header files required for Ogre3D and SDL2. No Ogre3D or SDL2 support possible.])
        OGRE_GRAPHICS_FEATURE_ENABLED="no, need X11 headers"
        SDL_GRAPHICS_FEATURE_ENABLED="no, need X11 headers"
        AUDIO_FEATURE_ENABLED="no, need SDL2"
        COMPILE_OGRE="no"
        NEEDSDL="no"
    ])
    AS_IF([test "x$NEED_XHEADERS" = xyes], [
        AC_CHECK_HEADERS([X11/extensions/Xext.h],,[
            AC_WARN([Cannot find Xext header files required for Ogre3D and SDL2. No Ogre3D or SDL2 support possible.])
            OGRE_GRAPHICS_FEATURE_ENABLED="no, need Xext headers"
            SDL_GRAPHICS_FEATURE_ENABLED="no, need Xext headers"
            AUDIO_FEATURE_ENABLED="no, need SDL2"
            COMPILE_OGRE="no"
            NEEDSDL="no"
        ],[
#include <X11/Xlib.h>
        ])
    ])
])

# Check Ogre3D:
AS_IF([test "x$COMPILE_OGRE" = xyes],[
    AC_MSG_CHECKING([for static Ogre3D library])
    AS_IF([test ! -f "src/ogre/OgreMain/src/OgreMath.cpp"],[
        AC_MSG_RESULT([no])
        AC_LANG_PUSH([C++])
        AC_CHECK_HEADERS([OgreRoot.h],[
            #shared lib is there!
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  Ogre3D: shared"
        ],[
            #nothing there
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  Ogre3D: no"
            OGRE_GRAPHICS_FEATURE_ENABLED="no, need Ogre3D"
        ])
        AC_LANG_POP([C++])
    ],[
        AC_MSG_RESULT([yes])
        STATIC_LIBS_USE="${STATIC_LIBS_USE} Ogre3D"
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  Ogre3D: static"
    ])
])

# Check for SDL2 availability:
AS_IF([test "x$NEEDSDL" = xyes],[
    AC_MSG_CHECKING([for static SDL2 library])
    AS_IF([test ! -f "src/sdl/src/SDL.c"],[
        AC_MSG_RESULT([no])
        AC_CHECK_LIB([SDL2], [SDL_Init],[
            #shared lib is there!
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  SDL2: shared"
        ],[
            #nothing there
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  SDL2: no"
            SDL_GRAPHICS_FEATURE_ENABLED="no, need SDL2"
            # Windows can do audio without SDL (WaveOut):
            AS_IF([test "x$WINDOWS" = xno],[
                AUDIO_FEATURE_ENABLED="no, need SDL2"
            ],[
                AUDIO_FEATURE_ENABLED="yes"
                AUDIO_FEATURE_ENABLED_TYPE="waveOut"
            ])
        ])
    ],[
        AC_MSG_RESULT([yes])
        STATIC_LIBS_USE="${STATIC_LIBS_USE} SDL2"
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  SDL2: static"
    ])
])

# Check PhysFS:
AC_MSG_CHECKING([for static PhysFS >=2.1 library])
AS_IF([test ! -f "src/physfs/src/physfs.c"],[
    AC_MSG_RESULT([no])
    AC_CHECK_LIB([physfs], [PHYSFS_mountIo],[
        #shared lib is there!
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libphysfs: shared"
    ],[
        #nothing there
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libphysfs: no"
        ZIP_FEATURE_ENABLED="no, need libphysfs"
    ])
],[
    AC_MSG_RESULT([yes])
    STATIC_LIBS_USE="${STATIC_LIBS_USE} PhysFS"
    LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libphysfs: static"
])

AS_IF([test "x$enable_audio" = xyes],[ # START enable_audio block

# Check for libvorbis availability:

AC_MSG_CHECKING([for static vorbis/vorbisfile library])
AS_IF([test ! -f "src/vorbis/lib/vorbisenc.c"],[
    AC_MSG_RESULT([no])
    AC_CHECK_LIB([vorbisfile], [ov_info],[
        AC_CHECK_LIB([vorbis], [vorbis_block_init],[
            #shared lib is there!
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libvorbis/vorbisfile: shared"
        ],[
            #incomplete
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libvorbis/vorbisfile: no"
            AUDIO_FEATURE_ENABLED="no, need libvorbis/libvorbisfile"
            NULL_AUDIO_FEATURE_ENABLED="no, need libvorbis/libvorbisfile"
            FFMPEG_FEATURE_ENABLED="no, need libvorbis/libvorbisfile"
            SPEEX_RESAMPLING_FEATURE_ENABLED="no, need libvorbis/libvorbisfile"
            FLAC_FEATURE_ENABLED="no, need libvorbis/libvorbisfile"
        ])
    ],[
        #nothing there
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libvorbis/vorbisfile: no"
        AUDIO_FEATURE_ENABLED="no, need libvorbis/libvorbisfile"
        NULL_AUDIO_FEATURE_ENABLED="no, need libvorbis/libvorbisfile"
        FFMPEG_FEATURE_ENABLED="no, need libvorbis/libvorbisfile"
        SPEEX_RESAMPLING_FEATURE_ENABLED="no, need libvorbis/libvorbisfile"
        FLAC_FEATURE_ENABLED="no, need libvorbis/libvorbisfile"
    ])
],[
    AC_MSG_RESULT([yes])
    STATIC_LIBS_USE="${STATIC_LIBS_USE} vorbis"
    LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libvorbis/vorbisfile: static"
])


# Check for libogg availability:

AC_MSG_CHECKING([for static ogg library])
AS_IF([test ! -f "src/ogg/src/framing.c"],[
    AC_MSG_RESULT([no])
    AC_CHECK_LIB([ogg], [oggpack_read],[
        #shared lib is there!
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libogg: shared"
    ],[
        #nothing there
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libogg: no"
        AUDIO_FEATURE_ENABLED="no, need libogg"
        NULL_AUDIO_FEATURE_ENABLED="no, need libogg"
        FFMPEG_FEATURE_ENABLED="no, need libogg"
        SPEEX_RESAMPLING_FEATURE_ENABLED="no, need libogg"
        FLAC_FEATURE_ENABLED="no, need libogg"
    ])
],[
    AC_MSG_RESULT([yes])
    STATIC_LIBS_USE="${STATIC_LIBS_USE} ogg"
    LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libogg: static"
])


# Check for libFLAC availability:

AS_IF([test "x$enable_flac" = xyes],[
    AC_MSG_CHECKING([for static libFLAC library])
    AS_IF([test ! -f "src/flac/src/libFLAC/lpc.c"],[
        AC_MSG_RESULT([no])

        # Check for shared FLAC lib:
        HAVESHAREDFLAC="no"
        AC_CHECK_LIB([FLAC], [FLAC__stream_decoder_new],[
            AC_CHECK_HEADERS([FLAC/format.h], [
                HAVESHAREDFLAC="yes"
                LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libFLAC: shared"
            ])
        ])

        AS_IF([test "x$HAVESHAREDFLAC" = xno],[
            #nothing there
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libFLAC: no"
            FLAC_FEATURE_ENABLED="no, need libFLAC"
        ])
    ],[
        AC_MSG_RESULT([yes])
        STATIC_LIBS_USE="${STATIC_LIBS_USE} FLAC"
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libFLAC: static"
    ])
], [
    FLAC_FEATURE_ENABLED="no, disabled"
])


# Check for libspeex availability:

AS_IF([test "x$enable_speex" = xyes],[
    AC_MSG_CHECKING([for static libspeex library])
    AS_IF([test ! -f "src/speex/src/speexdec.c"],[
        AC_MSG_RESULT([no])

        # Check for shared speex lib:
        HAVESHAREDSPEEX="no"
        AC_CHECK_LIB([speexdsp], [speex_resampler_init],[
            AC_CHECK_LIB([speex], [speex_encoder_init],[
                AC_CHECK_HEADERS([speex/speex_resampler.h], [
                    HAVESHAREDSPEEX="yes"
                    LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libspeex: shared"
                ])
            ])
        ])

        # Deactivate features if necessary:
        AS_IF([test "x$HAVESHAREDSPEEX" = xno],[
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libspeex: no"
            SPEEX_RESAMPLING_FEATURE_ENABLED="no, need libspeex"
        ])
    ],[
        AC_MSG_RESULT([yes])
        STATIC_LIBS_USE="${STATIC_LIBS_USE} speex"
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libspeex: static"
    ])
],[
    SPEEX_RESAMPLING_FEATURE_ENABLED="no, disabled"
])

],[ # STOP enable_audio block
    AUDIO_FEATURE_ENABLED="no, disabled"
    NULL_AUDIO_FEATURE_ENABLED="no, disabled"
    FLAC_FEATURE_ENABLED="no, disabled"
    SPEEX_RESAMPLING_FEATURE_ENABLED="no, disabled"
    FFMPEG_FEATURE_ENABLED="no, disabled"
])


# Check for liblua availability:

AC_MSG_CHECKING([for static lua library])
AS_IF([test ! -f "src/lua/src/lua.h"],[
    AC_MSG_RESULT([no])
    HAVE_LUA_SHARED="no"

    # Check for regular -llua5.2 first
    AC_CHECK_LIB([lua5.2], [lua_close],[
        AC_CHECK_LIB([lua5.2], [lua_rawlen],[
            AC_CHECK_HEADERS([lua5.2/lua.h], [
                #shared lib is there!
                LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  liblua: shared"
                HAVE_LUA_SHARED="yes"
                SHARED_LUA_5_2="yes"
                FAST_FLOAT2INT_FEATURE_ENABLED="no, need static liblua"
            ])
        ])
    ])

    # Check for generic -llua
    AS_IF([test "x$HAVE_LUA_SHARED" = xno],[
        AC_CHECK_LIB([lua], [lua_close],[
            AC_CHECK_LIB([lua], [lua_rawlen],[
                AC_CHECK_HEADERS([lua.h], [
                    #shared lib is there!
                    LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  liblua: shared"
                    HAVE_LUA_SHARED="yes"
                    FAST_FLOAT2INT_FEATURE_ENABLED="no, need static liblua"
                ])
            ])
        ])
    ])

    AS_IF([test "x$HAVE_LUA_SHARED" = xno],[
        #nothing there
        AC_MSG_ERROR([Lua 5.2 is required for blitwizard. Check README-deps.txt for help on how to add it.])
    ])
],[
    AC_MSG_RESULT([yes])
    STATIC_LIBS_USE="${STATIC_LIBS_USE} lua"
    LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  liblua: static"
])

# Check for bullet physics availability:

AS_IF([test "x$enable_physics3d" = xyes],[
    AC_MSG_CHECKING([for static bullet physics library])
    HAVE_STATIC_BULLET="yes"
    AS_IF([test ! -f "src/bullet/src/BulletDynamics/Dynamics/btRigidBody.cpp"],[
        AC_MSG_RESULT([no])
        HAVE_STATIC_BULLET="no"
    ], [
        AC_MSG_RESULT([yes])
    ])
    AS_IF([test "x$HAVE_STATIC_BULLET" = xno], [
        # check for shared lib
        AC_LANG_PUSH([C++])
        AC_CHECK_HEADER([BulletDynamics/btBulletDynamicsCommon.h], [
            # shared lib is there
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  bullet: shared"
        ], [
            # no lib there
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  bullet: no"
            PHYSICS3D_FEATURE_ENABLED="no, need bullet physics"
        ])
        AC_LANG_POP([C++])
    ], [
        # static lib is here
        STATIC_LIBS_USE="${STATIC_LIBS_USE} bullet"
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  bullet: static"
    ])
], [
    PHYSICS3D_FEATURE_ENABLED="no, disabled"
])

# Check for libbox2d availability:

AS_IF([test "x$enable_physics2d" = xyes],[
    AC_MSG_CHECKING([for static Box2D library])
    HAVE_STATIC_BOX2D="yes"
    AS_IF([test ! -f "src/box2d/Box2D/Box2D.h"],[
        AC_MSG_RESULT([no])
        HAVE_STATIC_BOX2D="no"
    ], [
        AC_MSG_RESULT([yes])
    ])
    AS_IF([test "x$HAVE_STATIC_BOX2D" = xno], [
        #check for shared lib
        AC_LANG_PUSH([C++])
        AC_CHECK_HEADERS([Box2D.h],[
            #shared lib is there!
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libbox2d: shared"
        ],[
            #nothing there
            LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libbox2d: no"
            PHYSICS2D_FEATURE_ENABLED="no, need libbox2d"
        ])
        AC_LANG_POP([C++])
    ],[
        #static lib is here
        STATIC_LIBS_USE="${STATIC_LIBS_USE} box2d"
        LIBS_DEP_INFO_TEXT="${LIBS_DEP_INFO_TEXT}
  libbox2d: static"
    ])
], [
    PHYSICS2D_FEATURE_ENABLED="no, disabled"
])

# Compose ld/include/define flags:

FINAL_LD_FLAGS=""
FINAL_INCLUDE_FLAGS=""
FINAL_USE_LIB_FLAGS=""

SDLADDED=no
statictest=`echo $STATIC_LIBS_USE | grep lua`
AS_IF([test -z "$statictest"], [
    FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DNOLLIMITS"
    AC_DEFINE([NOLLIMITS], [], [disable use of llimits-based float2int optimisation])
    AS_IF([test "x$SHARED_LUA_5_2" = xyes],[
        FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DLUA_5_2_HEADER"
        AC_DEFINE([LUA_5_2_HEADER], [], [Enable use of lua 5.2 include folder])
    ])
])
AS_IF([test "x$SDL_GRAPHICS_FEATURE_ENABLED" = xyes],[
    statictest=`echo $STATIC_LIBS_USE | grep SDL2`
    SDLADDED=yes
    AS_IF([test -z "$statictest"], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lSDL2"
    ], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardSDL"
        FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Iinclude/sdl"
    ])
    FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DUSE_SDL_GRAPHICS"
    AC_DEFINE([USE_SDL_GRAPHICS], [], [Enable SDL graphics subsystem])
])
AS_IF([test "x$ZIP_FEATURE_ENABLED" = xyes],[
    statictest=`echo $STATIC_LIBS_USE | grep PhysFS`
    AS_IF([test -z "$statictest"], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lphysfs"
    ], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardPhysFS"
        FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Iphysfs/src"
    ])
    FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DUSE_PHYSFS"
    AC_DEFINE([USE_PHYSFS], [],
    [Enable .zip archive loading support based on PhysFS])
])
AS_IF([test "x$OGRE_GRAPHICS_FEATURE_ENABLED" = xyes],[
    statictest=`echo $STATIC_LIBS_USE | grep Ogre3D`

    # Add ogre to include path:
    AS_IF([test -z "$statictest"], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lOgreMain"
    ], [
        for f in libs/libblitwizardOgre*.a libs/libblitwizardPlugin_*.a libs/libblitwizardRenderSystem_*.a
        do
            FINAL_LD_FLAGS="$FINAL_LD_FLAGS -l`basename $f .a | sed -e 's/^lib//'`"
        done
        FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Iogre/include/ -Iogre/OgreMain/include"
    ])

    # Add OIS to include path:
    AS_IF([test -z "$statictest"], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lOIS"
    ], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardOIS"
        FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Iois/includes"
    ])

    # Add freetype to include path:
    AS_IF([test -z "$statictest"], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lfreetype"
    ], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardfreetype"
        FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Ifreetype/include"
    ])

    FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DUSE_OGRE_GRAPHICS"
    AC_DEFINE([USE_OGRE_GRAPHICS], [], [Enable 3d graphics based on OGRE])
])
AS_IF([test "x$AUDIO_FEATURE_ENABLED" = xyes],[
    AS_IF([test "x$AUDIO_FEATURE_ENABLED_TYPE" = xSDL2],[
        # make sure SDL 2 is linked for audio:
        AS_IF([test "x$SDLADDED" = xno],[
            statictest=`echo $STATIC_LIBS_USE | grep SDL2`
            SDLADDED=yes
            AS_IF([test -z "$statictest"], [
                FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lSDL2"
            ], [
                FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardSDL"
                FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Iinclude/sdl"
            ])
        ])
        
        FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DUSE_SDL_AUDIO"
        AC_DEFINE([USE_SDL_AUDIO], [], [Enable SDL audio subsystem]) 
    ])
])

AS_IF([test "x$NULL_GRAPHICS_FEATURE_ENABLED" = xyes],[
    FINAL_LD_FLAGS="$FINAL_LD_FLAGS -limglib"
    FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Iimgloader/ -Iimgloader/png -Iimgloader/zlib"
    STATIC_LIBS_USE="$STATIC_LIBS_USE imgloader"
    statictest=`echo $STATIC_LIBS_USE | grep png`
    AS_IF([test -z "$statictest"],[
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lpng"
    ], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardpng"
    ])
    AS_IF([test -z "$statictest"],[
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lz"
    ], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardzlib"
    ])
    FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DUSE_GRAPHICS"
    AC_DEFINE([USE_GRAPHICS], [], [Enable basic graphics subsystem])
])

AS_IF([test "x$NULL_AUDIO_FEATURE_ENABLED" = xyes],[
    statictest=`echo $STATIC_LIBS_USE | grep vorbis`
    AS_IF([test -z "$statictest"],[
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lvorbisfile -lvorbis"
    ],[
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardvorbisfile -lblitwizardvorbis"
        FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Ivorbis/include"

    ])
    statictest=`echo $STATIC_LIBS_USE | grep ogg`
    AS_IF([test -z "$statictest"],[
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -logg"
    ], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardogg"
        FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Iogg/include"
    ])
    FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DUSE_AUDIO"
    AC_DEFINE([USE_AUDIO], [], [Enable basic audio subsystem])
])

statictest=`echo $STATIC_LIBS_USE | grep lua`
AS_IF([test -z "$statictest"],[
    AS_IF([test "x$SHARED_LUA_5_2" = xyes],[
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -llua5.2"
    ],[
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -llua"
    ])
], [
    FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardlua"
    FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Ilua/src"
])

AS_IF([test "x$NULL_AUDIO_FEATURE_ENABLED" = xyes],[
    # Checking on FLAC status:
    AS_IF([test "x$FLAC_FEATURE_ENABLED" = xyes],[
        statictest=`echo $STATIC_LIBS_USE | grep FLAC`
        AS_IF([test -z "$statictest"],[
            FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lFLAC"
        ], [
            FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardFLAC"
            FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Iflac/include"
        ])
        FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DUSE_FLAC_AUDIO"
        AC_DEFINE([USE_FLAC_AUDIO], [], [Enable decoding of FLAC audio files])
    ])

    # Checking on Speex status:
    AS_IF([test "x$SPEEX_RESAMPLING_FEATURE_ENABLED" = xyes],[
        statictest=`echo $STATIC_LIBS_USE | grep speex`
        AS_IF([test -z "$statictest"],[
            FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lspeex -lspeexdsp"
        ], [
            FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardspeex -lblitwizardspeexdsp"
            FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Ispeex/include"
        ])
        FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DUSE_SPEEX_RESAMPLING"
        AC_DEFINE([USE_SPEEX_RESAMPLING], [],
        [Enable audio resampling based on libspeex])
    ])

    # Checking on FFmpeg status:
    AS_IF([test "x$FFMPEG_FEATURE_ENABLED" = xyes],[
        FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Iffmpeg/"
        FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DUSE_FFMPEG_AUDIO"
        AC_DEFINE([USE_FFMPEG_AUDIO], [], [Enable audio decoding with FFmpeg])
    ])

]) # AUDIO_FEATURE_ENABLED

AS_IF([test "x$PHYSICS2D_FEATURE_ENABLED" = xyes],[
    statictest=`echo $STATIC_LIBS_USE | grep box2d`
    AS_IF([test -z "$statictest"],[
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lbox2d"
    ], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardbox2d"
        FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Ibox2d/ -Ibox2d/Box2D/"
    ])
    FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DUSE_PHYSICS2D"
    AC_DEFINE([USE_PHYSICS2D], [], [Enable 2d physics based on Box2D])
])
AS_IF([test "x$PHYSICS3D_FEATURE_ENABLED" = xyes],[
    statictest=`echo $STATIC_LIBS_USE | grep bullet`
    AS_IF([test -z "$statictest"],[
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lBulletDynamics -lBulletSoftBody -lBulletCollision -lLinearMath"
    ], [
        FINAL_LD_FLAGS="$FINAL_LD_FLAGS -lblitwizardBulletDynamics -lblitwizardBulletSoftBody -lblitwizardBulletCollision -lblitwizardLinearMath"
        FINAL_INCLUDE_FLAGS="$FINAL_INCLUDE_FLAGS -Ibullet/src/"
    ])
    FINAL_USE_LIB_FLAGS="$FINAL_USE_LIB_FLAGS -DUSE_PHYSICS3D"
    AC_DEFINE([USE_PHYSICS3D], [], [Enable 3d physics based on bullet])
])

# Get the scripts/ dir (absolute path):
cd scripts/
scriptsdir="`pwd`"
cd ..

# Check for -O3 -fno-finite-math-only
OLD_CFLAGS="$CFLAGS"
CFLAGS="-O2 -fno-math-errno -fno-finite-math-only"
EXTRA_SPEED_FLAGS="no"
AC_MSG_CHECKING([whether CC accepts -O2 -fno-math-errno -fno-finite-math-only])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
    [AC_MSG_RESULT([yes])]
    [EXTRA_SPEED_FLAGS="yes"],
    [AC_MSG_RESULT([no])]
)
EXTRA_SPEED_FLAGS="no"
CFLAGS="$OLD_CFLAGS"

# Decide on some additional cflags:
ADDITIONAL_CFLAGS=""
AS_IF([test "x$enable_debug" = xyes],[
    ADDITIONAL_CFLAGS="$ADDITIONAL_CFLAGS -g"
],[
    ADDITIONAL_CFLAGS="$ADDITIONAL_CFLAGS -s"
    AS_IF([test "x$EXTRA_SPEED_FLAGS" = xyes],[
        ADDITIONAL_CFLAGS="$ADDITIONAL_CFLAGS -O2 -fno-math-errno -fno-finite-math-only"
    ],[
        ADDITIONAL_CFLAGS="$ADDITIONAL_CFLAGS -O2"
    ])
])

# OS dependant things:
AS_IF([test -z "$maybeos"],[
    AC_MSG_NOTICE([Trying to compile for a Unix target.]) 
    # >> Unix
    AM_CONDITIONAL([WIN_APP_PATHS], [test 0 = 1])

    MACTEST=`uname -a | grep Darwin`
    AS_IF([test -z "$MACTEST"],[
        # Non-Mac Unix:

        AM_CONDITIONAL([MAC_APP_PATHS], [test 0 = 1])
        LIBS="-pthread -ldl -lm -lrt"
        WRITELINE="\nluatarget=posix\nCXX=$CXX\nCC=$CC\nAR=$AR\nSTATIC_LIBS_USE=$STATIC_LIBS_USE\nHOST=$TGHOST\nUSE_LIB_FLAGS=$FINAL_USE_LIB_FLAGS\nRANLIB=$RANLIB\nMACBUILD=no\nWINDOWSBUILD=no\n"
    ],[
        # Mac OS X:

        # Use this compiler since the default one hates libvorbis:
        CC="clang -arch i386 "
        CXX="clang -arch i386"
        OBJC="clang -arch i386"
        # Also, we want to compile a 32bit bin so we can load FFmpeg

        # Various other flags:
        AM_CONDITIONAL([MAC_APP_PATHS], [test 1 = 1])
        LIBS="$LIBS -lm -liconv -pthread -framework CoreServices -framework Carbon -framework AudioUnit -framework ForceFeedback -framework OpenGL -framework AppKit -framework Cocoa -framework Foundation -framework IOKit -framework CoreAudio"
        WRITELINE="\nEXEEXT=$EXEEXT\nluatarget=posix\nCXX=$CXX\nCC=$CC\nSTATIC_LIBS_USE=$STATIC_LIBS_USE\nAR=$AR\nHOST=$TGHOST\nUSE_LIB_FLAGS=$FINAL_USE_LIB_FLAGS\nRANLIB=$RANLIB\nMACBUILD=yes\nWINDOWSBUILD=no\n"
    ])

    echo -e "$WRITELINE" > scripts/.buildinfo
    CFLAGS="$ADDITIONAL_CFLAGS $CFLAGS `cd $scriptsdir && sh ./checksse.sh`"   
    CXXFLAGS="$ADDITIONAL_CFLAGS $CXXFLAGS `cd $scriptsdir && sh ./checksse.sh`"

    AC_CHECK_LIB([dl], [dlopen],, AC_MSG_ERROR(Cannot find all the necessary functions. Try to install the necessary libraries and headers))
    
    # Checks for header files.
    AC_CHECK_HEADERS([arpa/inet.h dlfcn.h ctype.h fcntl.h netdb.h netinet/in.h stdarg.h sys/stat.h sys/socket.h sys/wait.h],,AC_MSG_ERROR(Cannot find all the necessary headers. Try to install the necessary libraries and headers))
    AC_CHECK_HEADERS([malloc.h])


    # Checks for typedefs, structures, and compiler characteristics.
    #AC_HEADER_STDBOOL

    AC_FUNC_ALLOCA(,,AC_MSG_ERROR(Cannot find all the necessary functions. Try to install the necessary libraries and headers))
    #AC_FUNC_CHOWN(,,AC_MSG_ERROR(Cannot find all the necessary functions. Try to install the necessary libraries and headers))
    #AC_FUNC_FORK(,,AC_MSG_ERROR(Cannot find all the necessary functions. Try to install the necessary libraries and headers))
    AC_CHECK_FUNCS([snprintf strncat],,AC_MSG_ERROR(Cannot find all the necessary functions. Try to install the necessary libraries and headers))
    AC_FUNC_MALLOC(,,AC_MSG_ERROR(Cannot find all the necessary functions. Try to install the necessary libraries and headers))
    #AC_FUNC_MKTIME(,,AC_MSG_ERROR(Cannot find all the necessary functions. Try to install the necessary libraries and headers))
    AC_FUNC_REALLOC(,,AC_MSG_ERROR(Cannot find all the necessary functions. Try to install the necessary libraries and headers))

],[
    AC_MSG_NOTICE([Attempting to compile for a Windows target.]) 
    # >> Windows
    AM_CONDITIONAL([WIN_APP_PATHS], [test 1 = 1])
    AM_CONDITIONAL([MAC_APP_PATHS], [test 0 = 1])

    # - check compiler first
    compileros=`echo $CC | grep mingw`
    AS_IF([test -z "$compileros"], [
    AC_WARN([You have chosen a mingw target but apparently the compiler is not a mingw cross-compiler. Please fix this if you are on Unix trying to cross-compile for Windows (if you are on Windows, everything should be fine).])
    ])

    # Guess strip program:
    STRIP="$host_alias-strip"

    TGHOST="$host_alias"
    AC_SUBST(TGHOST)
    
    WRITELINE="\nEXEEXT=$EXEEXT\nluatarget=mingw\nCXX=$CXX\nCC=$CC\nAR=$AR\nHOST=$TGHOST\nUSE_LIB_FLAGS=$FINAL_USE_LIB_FLAGS\nRANLIB=$RANLIB\nSTATIC_LIBS_USE=$STATIC_LIBS_USE\nMACBUILD=no\nWINDOWSBUILD=yes\n"
    echo -e "$WRITELINE" > scripts/.buildinfo

    CFLAGS="$ADDITIONAL_CFLAGS $CFLAGS -Wl,--large-address-aware -static-libgcc -mwindows -mthreads -L../ -I../ `cd $scriptsdir && sh ./checksse.sh`"
    CXXFLAGS="$ADDITIONAL_CFLAGS $CFLAGS -Wl,--large-address-aware -static-libgcc -static-libstdc++ -mwindows -mthreads -L../ -I../ `cd $scriptsdir && sh ./checksse.sh`"
    LIBS="$LIBS -lshlwapi -lgdi32 -limm32 -ldbghelp -loleaut32 -lws2_32 -lole32 -lwinmm -luuid -lversion"
    
    # -lwbemuuid check:
    AC_MSG_CHECKING([for -lwbemuuid availability])
    echo "int main(int argc, const char** argv) {return 0;}" > ./test.c
    rm -f ./testbin
    $CC -L./ test.c -lwbemuuid -o ./testbin > /dev/null 2>&1
    rm ./test.c
    AS_IF([test ! -f ./testbin],[
        AC_MSG_RESULT([no])
    ],[
        AS_IF([test -f ./testbin.exe ],[
            rm ./testbin.exe
        ],[
            rm -f ./testbin
        ])
        AC_MSG_RESULT([yes])
        LIBS="$LIBS -lwbemuuid"
    ])
    
    FINAL_LD_FLAGS="-lmingw32 $FINAL_LD_FLAGS"
    STATICSTDCXX="-static-libstdc++"
    AC_SUBST(STATICSTDCXX)

    AC_CHECK_HEADERS([windows.h string.h stdio.h unistd.h],,AC_MSG_ERROR(Cannot find all the necessary headers. Try to install the necessary libraries and headers))

])


# Prepare output of the build flags

BUILD_FLAGS_OUTPUT="
Build flags:
  LD flags:      $FINAL_LD_FLAGS
  include flags: $FINAL_INCLUDE_FLAGS
  Feature flags: $FINAL_USE_LIB_FLAGS"


# Add the build flags for the make script

AC_SUBST(FINAL_INCLUDE_FLAGS)
AC_SUBST(FINAL_USE_LIB_FLAGS)
AC_SUBST(FINAL_LD_FLAGS)
AC_DEFINE_DIR([BWDATADIR], [datadir], [Data directory])
AC_DEFINE_UNQUOTED([SYSTEM_TEMPLATE_PATH], "$BWDATADIR/blitwizard/templates/",
[The path to the system-wide installed templates])
AS_IF([test "x$enable_debug" = xyes],[
    AM_CONDITIONAL([ENABLE_DEBUG], [test 1 = 1])
],[
    AM_CONDITIONAL([ENABLE_DEBUG], [test 0 = 1])
])

LIBS="$FINAL_LD_FLAGS $LIBS"


# Export strip program

AC_SUBST(STRIP)


AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile])
AC_OUTPUT

# Prepare final deps info output:

AS_IF([test ! "x$AUDIO_FEATURE_ENABLED" = xyes],[
    AUDIO_FEATURE_ENABLED_TYPE=""
],[
    AUDIO_FEATURE_ENABLED_TYPE="($AUDIO_FEATURE_ENABLED_TYPE)"
])

DEPS_INFO_OUTPUT="
This results in the following feature set:
  SDL Graphics (2d only): ${SDL_GRAPHICS_FEATURE_ENABLED}
  Ogre Graphics (2d or 3d): ${OGRE_GRAPHICS_FEATURE_ENABLED}
  Null device graphics: ${NULL_GRAPHICS_FEATURE_ENABLED}
  Audio: ${AUDIO_FEATURE_ENABLED} ${AUDIO_FEATURE_ENABLED_TYPE}
  Null device audio: ${NULL_AUDIO_FEATURE_ENABLED}
  Flac decoding: ${FLAC_FEATURE_ENABLED}
  Speex-based resampling: ${SPEEX_RESAMPLING_FEATURE_ENABLED}
  Runtime FFmpeg support: ${FFMPEG_FEATURE_ENABLED}
  2D physics simulation support: ${PHYSICS2D_FEATURE_ENABLED}
  3D physics simulation support: ${PHYSICS3D_FEATURE_ENABLED}
  Fast float-to-int conversion: ${FAST_FLOAT2INT_FEATURE_ENABLED}
  Zip archive resource loading: ${ZIP_FEATURE_ENABLED}
"



echo "$LIBS_DEP_INFO_TEXT"
echo "$DEPS_INFO_OUTPUT"

echo " "
echo "*** You can now build the project by typing 'make'."
echo "The resulting binaries will be created inside the bin/ folder."

