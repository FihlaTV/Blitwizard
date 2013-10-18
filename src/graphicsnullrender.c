
/* blitwizard game engine - source code file

  Copyright (C) 2013 Jonas Thiem

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

#ifdef USE_NULL_GRAPHICS

//  various standard headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef WINDOWS
#include <windows.h>
#endif
#include <stdarg.h>
#include <assert.h>

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
#include "graphicsnulltexturestruct.h"
#include "graphics2dsprites.h"

extern int graphicsactive;
extern int inbackground;
extern int graphics3d;
extern int mainwindowfullscreen;

void graphicsnullrender_StartFrame(void) {
}

void graphicsnullrender_CompleteFrame(void) {
}

static void graphicsnullrender_spriteCallback(
const struct graphics2dsprite* sprite,
const char* path, struct graphicstexture* tex,
double r, double g, double b, double alpha,
int visible, int cameraId, int textureFiltering) {
    if (!tex) {
        return;
    }
    if (!visible) {
        return;
    }

    double x, y, width, height;
    double sourceX, sourceY, sourceWidth, sourceHeight;
    double angle;
    int horiflip;
    graphics2dsprite_calculateSizeOnScreen(
    sprite, 0, &x, &y, &width, &height, &sourceX, &sourceY,
    &sourceWidth, &sourceHeight, &angle, &horiflip, 0);

    // render:
}

void graphicsrender_Draw(void) {
    graphicsnullrender_StartFrame();

    // render sprites:
    graphics2dsprites_doForAllSprites(
    &graphicsnullrender_spriteCallback);

    graphicsnullrender_CompleteFrame();
}

#endif  // USE_NULL_GRAPHICS


