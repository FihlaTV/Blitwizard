
# blitwizard Makefile.am

SUBDIRS = .
ACLOCAL_AMFLAGS = -I m4

# -------------
# listing of non-os dependant blitwizard object files:
# -------------
source_code_files = audio.c audiomixer.c audiosourcefadepanvol.c audiosourceffmpeg.c audiosourceflac.c audiosourcefile.c audiosourceformatconvert.c audiosourceloop.c audiosourceogg.c audiosourceprereadcache.c audiosourceresample.c audiosourceresourcefile.c audiosourcewave.c avl-tree/avl-tree.c avl-tree-helpers.c connections.c file.c filelist.c diskcache.c graphics.c graphics2dsprites.c graphics2dspriteslist.c graphics2dspritestree.c graphicscamera.c graphicsnull.c graphicsnullrender.c graphicsnulltexture.c graphicsogre.cpp graphicsogrerender.cpp graphicssdl.c graphicssdlglext.c graphicssdlrender.c graphicssdltexture.c graphicstexturelist.c graphicstextureloader.c graphicstexturemanager.c graphicstexturemanagermembudget.c graphicstexturemanagertexturedecide.c hash.c hostresolver.c ipcheck.c library.c listeners.c logging.c luaerror.c luafuncs.c luafuncs_debug.c luafuncs_graphics.c luafuncs_graphics_camera.c luafuncs_media_object.c luafuncs_net.c luafuncs_object.c luafuncs_objectgraphics.c luafuncs_objectphysics.c luafuncs_os.c luafuncs_physics.c luafuncs_rundelayed.c luafuncs_string.c luafuncs_vector.c luastate.c luastate_functionTables.c main.c mathhelpers.c orderedExecution.c osinfo.c physics.cpp physicsinternal.cpp poolAllocator.c signalhandling.c threading.c timefuncs.c win32console.c resources.c sockets.c zipdecryptionnone.c zipfile.c

# -------------
# OS dependant object files:
# -------------
if MAC_APP_PATHS
source_code_files += macapppaths.m
endif
if WIN_APP_PATHS
source_code_files += win32apppaths.c
endif

# -------------
# blitwizard program target:
# -------------
bin_PROGRAMS = blitwizard
blitwizard_SOURCES = $(source_code_files)
blitwizard_LDADD = 
blitwizard_LDFLAGS = $(FINAL_LD_FLAGS)
if ENABLE_DEBUG
blitwizard_STRIP=
else
blitwizard_STRIP=strip blitwizard$(EXEEXT)
endif

# -------------
# CFLAGS/CXXFLAGS/OBJCFLAGS:
# -------------
# Note: -fno-PIC has been added purely for some additional speed.
AM_CFLAGS = $(FINAL_INCLUDE_FLAGS) -fno-PIC -finline-functions -fno-strict-aliasing -Werror-implicit-function-declaration -Wstrict-prototypes -Wall -Wextra -fno-omit-frame-pointer -L../libs/ -std=gnu99
AM_CXXFLAGS = $(FINAL_INCLUDE_FLAGS) $(STATICSTDCXX) -fno-PIC -finline-functions -fno-strict-aliasing -Wall -Wextra -fexceptions -fno-omit-frame-pointer -L../libs/
AM_OBJCFLAGS = -fno-PIC -Wall -fno-exceptions -fno-omit-frame-pointer

# -------------
# Unit test section
# -------------
TESTS=
testd = ctests
TEST_CFLAGS= $(AM_CFLAGS) -DUNITTEST

# -------------
# C tests
# These tests test the internal C code and they are generally more
# fine-grained and detailed than the lua tests (see below) and they're
# testing smaller components.
# -------------
check_PROGRAMS = $(testd)/test-imgloader-basic $(testd)/test-texman-2dsprites $(testd)/test-imgloader-colors $(testd)/test-texman-availability
__testd__test_imgloader_basic_SOURCES = $(testd)/test-imgloader-basic.c $(source_code_files)
__testd__test_imgloader_basic_LDFLAGS = $(FINAL_LD_FLAGS)
__testd__test_imgloader_basic_CFLAGS = $(TEST_CFLAGS)
__testd__test_imgloader_colors_SOURCES = $(testd)/test-imgloader-colors.c $(source_code_files)
__testd__test_imgloader_colors_LDFLAGS = $(FINAL_LD_FLAGS)
__testd__test_imgloader_colors_CFLAGS = $(TEST_CFLAGS)
__testd__test_texman_2dsprites_SOURCES = $(testd)/test-texman-2dsprites.c $(source_code_files)
__testd__test_texman_2dsprites_LDFLAGS = $(FINAL_LD_FLAGS)
__testd__test_texman_2dsprites_CFLAGS = $(TEST_CFLAGS)
__testd__test_texman_availability_SOURCES = $(testd)/test-texman-availability.c $(source_code_files)
__testd__test_texman_availability_LDFLAGS = $(FINAL_LD_FLAGS)
__testd__test_texman_availability_CFLAGS = $(TEST_CFLAGS)
TESTS += $(testd)/test-imgloader-basic $(testd)/test-texman-2dsprites $(testd)/test-imgloader-colors $(testd)/test-texman-availability

# -------------
# Lua tests
# The lua tests test the final api from the outside (unlike the C tests that
# usually test inner components). Naturally, they operate a bit less
# fine-grained than the C tests and they usually have a slightly larger scope.
# -------------
TESTS += luatests/createobject.sh \
         luatests/filelist.sh \
         luatests/getvisiblegetzindex.sh \
         luatests/movecamera.sh \
         luatests/physicsgc.sh \
         luatests/setgetmass.sh \
         luatests/setmode.sh \
         luatests/textureusagereport.sh \
         luatests/zipls.sh \
		 luatests/zipdofile.sh

# -------------
# Makefile targets:
# -------------
all-local: mytarget
mytarget: blitwizard$(EXEEXT)
	$(blitwizard_STRIP)
	sh ../scripts/copybin.sh
install:
	mkdir -p ${datadir}/blitwizard
	mkdir -p ${datadir}/blitwizard/templates/
	rm -rf ${datadir}/blitwizard/templates/*
	cp -R ../templates/* ${datadir}/blitwizard/templates/
	chmod -R 775 ${datadir}/blitwizard
	cp ../bin/blitwizard${EXEEXT} ${bindir}
	chmod 755 ${bindir}/blitwizard${EXEEXT}
	chmod +x ${bindir}/blitwizard${EXEEXT}
count:
	cloc *.c *.h *.cpp imgloader/*.c imgloader/*.h *.cpp ../templates/init.lua ../templates/console/console.lua ../templates/font/font.lua ../templates/http/http.lua ../templates/irc/irc.lua ../templates/debug/debug.lua
