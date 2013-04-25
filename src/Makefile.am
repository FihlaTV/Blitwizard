
# blitwizard Makefile.am

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = blitwizard

blitwizard_SOURCES = audio.c audiomixer.c audiosourcefadepanvol.c audiosourceffmpeg.c audiosourceflac.c audiosourcefile.c audiosourceformatconvert.c audiosourceloop.c audiosourceogg.c audiosourceprereadcache.c audiosourceresample.c audiosourceresourcefile.c audiosourcewave.c connections.c file.c filelist.c diskcache.c graphics.c graphics2dsprites.c graphicsnull.c graphicsogre.cpp graphicsogrerender.cpp graphicssdl.c graphicssdlrender.c graphicssdltexture.c graphicstexturelist.c graphicstextureloader.c graphicstexturemanager.c hash.c hashtable.c hostresolver.c ipcheck.c library.c listeners.c logging.c luaerror.c luafuncs.c luafuncs_graphics.c luafuncs_graphics_camera.c luafuncs_media_object.c luafuncs_net.c luafuncs_object.c luafuncs_objectgraphics.c luafuncs_objectphysics.c luafuncs_os.c luafuncs_string.c luastate.c luastate_functionTables.c main.c mathhelpers.c osinfo.c physics2d.cpp signalhandling.c threading.c timefuncs.c win32console.c resources.c sockets.c zipdecryptionnone.c zipfile.c
blitwizard_LDADD = 
blitwizard_LDFLAGS = $(FINAL_LD_FLAGS)

if MAC_APP_PATHS
blitwizard_SOURCES += macapppaths.m
endif
if WIN_APP_PATHS
blitwizard_SOURCES += win32apppaths.c
endif
if ENABLE_DEBUG
blitwizard_STRIP=
else
blitwizard_STRIP=strip blitwizard$(EXEEXT)
endif

AM_CFLAGS = $(FINAL_INCLUDE_FLAGS) -fno-PIC -DSYSTEM_TEMPLATE_PATH="\"${datadir}/blitwizard/templates/\"" -DUSE_LIB_FLAGS="\"$(FINAL_USE_LIB_FLAGS)\"" -finline-functions -fno-strict-aliasing -Werror-implicit-function-declaration -Wstrict-prototypes -Wall -Wextra -fno-omit-frame-pointer -L../libs/ $(FINAL_USE_LIB_FLAGS)
AM_CXXFLAGS = $(FINAL_INCLUDE_FLAGS) $(STATICSTDCXX) -fno-PIC -DSYSTEM_TEMPLATE_PATH="\"${datadir}/blitwizard/templates/\"" -DUSE_LIB_FLAGS="\"$(FINAL_USE_LIB_FLAGS)\"" -finline-functions -fno-strict-aliasing -Wall -Wextra -fexceptions -fno-omit-frame-pointer -L../libs/ $(FINAL_USE_LIB_FLAGS)
AM_OBJCFLAGS = -fno-PIC -Wall -fno-exceptions -fno-omit-frame-pointer

# -fno-PIC is only added for some extra speed, same as -O3.

all-local: mytarget
mytarget: blitwizard$(EXEEXT)
	$(blitwizard_STRIP)
	sh ../scripts/copybin.sh
install:
	mkdir -p ${datadir}/blitwizard
#	rm -rf ${datadir}/blitwizard/templates/
	mkdir -p ${datadir}/blitwizard/templates/
	cp -R ../templates/* ${datadir}/blitwizard/templates/
	chmod -R 775 ${datadir}/blitwizard
	cp ../bin/blitwizard${EXEEXT} ${bindir}
	chmod 755 ${bindir}/blitwizard${EXEEXT}
count:
	cloc *.c *.h imgloader/*.c imgloader/*.h *.cpp
