
/* blitwizard game engine - source code file

  Copyright (C) 2013-2014 Jonas Thiem

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

#ifdef USE_GRAPHICS
#ifndef USE_SDL_GRAPHICS

//  various standard headers
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef WINDOWS
#include <windows.h>
#endif
#include <stdarg.h>

#include "graphics.h"
#include "graphicstexturelist.h"
#include "graphicscamerastruct.h"

#define MAXCAMERAS 16
struct cameraentry* camentry[MAXCAMERAS];

static void addFirstCamera(void);
__attribute__((constructor)) void clearCameraEntries(void) {
    int i = 0;
    while (i < MAXCAMERAS) {
        camentry[i] = NULL;
        i++;
    }
    addFirstCamera();
}

static void addFirstCamera(void) {
    if (camentry[0] == NULL) {
        assert(graphics_AddCamera() == 0);
    }
}

int graphics_getCameraCount(void) {
    int c = 0;
    int i = 0;
    while (i < MAXCAMERAS) {
        if (camentry[i] != NULL) {
            c++;
        } else {
            break;
        }
        i++;
    }
    return c;
}

int graphics_addCamera(void) {
    int i = 0;
    while (i < MAXCAMERAS) {
        if (camentry[i] == NULL) {
            camentry[i] = malloc(sizeof(*camentry[i]));
            if (!camentry[i]) {
                return -1;
            }
            memset(camentry[i], 0, sizeof(*camentry[i]));
            camentry[i]->width = 100;
            camentry[i]->height = 100;
            camentry[i]->i2d.zoom = 1.0f;
            return i;
        }
        i++;
    }
    return -1;
}

void graphics_deleteCamera(int index) {
    if (index < 0 || index >= MAXCAMERAS ||
    !camentry[index]) {
        return;
    }
    free(camentry[index]);
#ifdef USE_OGRE_GRAPHICS
#error "add cleanup code here"
#endif
    // move down all followup cameras:
    int i = index;
    while (i < MAXCAMERAS - 1) {
        camentry[i] = camentry[i + 1];
        i++;
    }
    camentry[MAXCAMERAS - 1] = NULL;
}

int graphics_getCameraAt(int x, int y) {
    int i = 0;
    while (i < MAXCAMERAS) {
        if (!camentry[i]) {
             continue;
        }
        int cx = graphics_GetCameraX(i);
        int cy = graphics_GetCameraY(i);
        int cw = graphics_GetCameraWidth(i);
        int ch = graphics_GetCameraHeight(i);
        if (x >= cx && cx < cx + cw && cy >= cy
        && cy < cy + ch) {
            return i;
        }
        i++;
    }
    return -1;
}

int graphics_getCameraWidth(int index) {
    if (index < 0 || index >= MAXCAMERAS
    || !camentry[index]) {
        return -1;
    }
    return camentry[index]->width;
}

int graphics_getCameraHeight(int index) {
    if (index < 0 || index >= MAXCAMERAS
    || !camentry[index]) {
        return -1;
    }
    return camentry[index]->height;
}

double graphics_getCamera2DZoom(int index) {
    if (index < 0 || index >= MAXCAMERAS
    || !camentry[index]) {
        return 0;
    }
    assert(camentry[index]->i2d.zoom > 0);
    return camentry[index]->i2d.zoom;
}

void graphics_setCamera2DZoom(int index, double zoom) {
    if (index < 0 || index >= MAXCAMERAS
    || !camentry[index]) {
        return;
    }
    assert(zoom > 0);
    camentry[index]->i2d.zoom = zoom;
}

void graphics_setCameraSize(int index, int w, int h) {
    if (index < 0 || index >= MAXCAMERAS
    || !camentry[index]) {
        return;
    }
    if (w < 1) {w = 1;}
    if (h < 1) {h = 1;}
    camentry[index]->width = w;
    camentry[index]->height = h;
}

double graphics_getCamera2DAspectRatio(int index) {
    if (index < 0 || index >= MAXCAMERAS
    || !camentry[index]) {
        return 1;
    }
    return camentry[index]->i2d.aspectratio;
}

double graphics_getCamera2DCenterX(int index) {
    if (index < 0 || index >= MAXCAMERAS
    || !camentry[index]) {
        return 0;
    }
    return camentry[index]->i2d.centerx;
}

double graphics_getCamera2DCenterY(int index) {
    if (index < 0 || index >= MAXCAMERAS
    || !camentry[index]) {
        return 0;
    }
    return camentry[index]->i2d.centery;
}

void graphics_setCamera2DCenterXY(int index, double x, double y) {
    if (index < 0 || index >= MAXCAMERAS
    || !camentry[index]) {
        return;
    }
    camentry[index]->i2d.centerx = x;
    camentry[index]->i2d.centery = y;
}

int graphics_getCameraX(int index) {
    if (index < 0 || index >= MAXCAMERAS
    || !camentry[index]) {
        return 0;
    }
    return camentry[index]->x;
}

int graphics_getCameraY(int index) {
    if (index < 0 || index >= MAXCAMERAS
    || !camentry[index]) {
        return 0;
    }
    return camentry[index]->y;
}

#endif // not(USE_SDL_GRAPHICS)
#endif // USE_GRAPHICS

