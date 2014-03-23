
/* blitwizard game engine - source code file

  Copyright (C) 2011-2014 Jonas Thiem

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

*/

#include "config.h"
#include "os.h"

#if (defined(USE_GRAPHICS) && !defined(USE_SDL_GRAPHICS))

// various standard headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef WINDOWS
#include <windows.h>
#endif
#include <stdarg.h>

#include "os.h"
#include "logging.h"
#include "imgloader.h"
#include "timefuncs.h"
#include "hash.h"
#include "file.h"
#ifdef NOTHREADEDSDLRW
#include "main.h"
#endif

#include "graphicstexture.h"
#include "graphics.h"
#include "graphicstexturelist.h"
#include "graphicsnullconf.h"

static unsigned int nulldevicewidth = 0;
static unsigned int nulldeviceheight = 0;
static unsigned int nulldevicefullscreen = 0;
static char *nulldevicetitle = NULL;
extern int graphicsactive;  // whether any graphics are running or not

int graphics_haveValidWindow() {
    if (nulldevicewidth && nulldeviceheight) {
        return 1;
    }
    return 0;
}

void graphics_invalidateHWTexture(struct graphicstexture *gt) {
    return;
}

void graphics_destroyHWTexture(struct graphicstexture *gt) {
    return;
}

int graphics_init(char **error, int use3dgraphics) {
    *error = strdup("3d graphics not available");
    return 1;
}

int graphics_textureToHW(struct graphicstexture *gt) {
    return 1;
}

void graphics_textureFromHW(struct graphicstexture *gt) {
    return;
}

int graphics_getWindowDimensions(unsigned int *width, unsigned int *height) {
    if (nulldevicewidth) {
        *width = nulldevicewidth;
        *height = nulldeviceheight;
        return 1;
    }
    return 0;
}

void graphics_close(int preservetextures) {
    graphicsactive = 0;
    nulldevicewidth = 0;
    nulldeviceheight = 0;
    if (nulldevicetitle) {
        free(nulldevicetitle);
        nulldevicetitle = NULL;
    }
}

#ifdef ANDROID
void graphics_reopenForAndroid() {

}
#endif

const char *graphics_getWindowTitle() {
    if (!nulldevicewidth && !nulldeviceheight) {
        return NULL;
    }
    return nulldevicetitle;
}

void graphics_quit() {
    graphics_Close(0);
}

static char nullstaticname[] = "nulldevice";
const char *graphics_getCurrentRendererName() {
    return nullstaticname;
}

int graphics_getNumberOfVideoModes() {
    return GRAPHICS_NULL_VIDEOMODES_COUNT;
}

void graphics_getVideoMode(int index, int *x, int *y) {
    *x = GRAPHICS_NULL_VIDEOMODES_X[index];
    *y = GRAPHICS_NULL_VIDEOMODES_Y[index];
}

void graphics_getDesktopVideoMode(int *x, int *y) {
    if (nulldevicefullscreen && nulldevicewidth) {
        *x = nulldevicewidth;
        *y = nulldeviceheight;
    }else{
        *x = GRAPHICS_NULL_DESKTOPWIDTH;
        *y = GRAPHICS_NULL_DESKTOPHEIGHT;
    }
}

void graphics_minimizeWindow() {
    return;
}

int graphics_isFullscreen() {
    if (nulldevicewidth) {
        return nulldevicefullscreen;
    }
    return 0;
}

void graphics_toggleFullscreen() {
    if (!nulldevicewidth) {
        return;
    }
    if (nulldevicefullscreen) {
        nulldevicefullscreen = 0;
    }else{
        nulldevicefullscreen = 1;
    }
}

#ifdef WINDOWS
HWND graphics_getWindowHWND() {
    return NULL;
}
#endif

int graphics_setMode(int width, int height, int fullscreen, int resizable, const char *title, const char *renderer, char **error) {

#if defined(ANDROID)
    if (!fullscreen) {
        // do not use windowed on Android
        *error = strdup("Windowed mode is not supported on Android");
        return 0;
    }
#endif

    if (width < 1 || height < 1) {
        return 0;
    }
    if (fullscreen) {
        nulldevicefullscreen = 1;
    }else{
        nulldevicefullscreen = 0;
    }
    nulldevicewidth = width;
    nulldeviceheight = height;
    if (nulldevicetitle) {
        free(nulldevicetitle);
        nulldevicetitle = NULL;
    }
    nulldevicetitle = strdup(title);
    if (nulldevicetitle) {
        nulldevicetitle = strdup("");
    }
    graphics_calculateUnitToPixels(nulldevicewidth, nulldeviceheight);
    graphicsactive = 1;
    return 1;
}


void graphicsrender_startFrame() {
    return;
}

void graphicsrender_completeFrame() {
    return;
}


void graphics_checkEvents(
        void (*quitevent)(void),
        void (*mousebuttonevent)(int button, int release, int x, int y),
        void (*mousemoveevent)(int x, int y),
        void (*keyboardevent)(const char *button, int release),
        void (*textevent)(const char *text),
        void (*putinbackground)(int background)) {
    return;
}

#endif // ifdef USE_GRAPHICS

