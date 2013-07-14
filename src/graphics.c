
/* blitwizard game engine - source code file

  Copyright (C) 2011-2013 Jonas Thiem

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

//  various standard headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef WINDOWS
#include <windows.h>
#endif
#include <stdarg.h>

#include "logging.h"
#include "imgloader.h"
#include "timefuncs.h"
#include "hash.h"
#include "file.h"
#ifdef NOTHREADEDSDLRW
#include "main.h"
#endif

#ifdef USE_SDL_GRAPHICS
#include "SDL.h"
#endif

#include "graphicstexture.h"
#include "graphics.h"
#include "graphicstexturelist.h"

int graphicsactive = 0;

int graphics_AreGraphicsRunning() {
    return graphicsactive;
}

double UNIT_TO_PIXELS = 50;
int unittopixelsset = 0;

void graphics_calculateUnitToPixels(int screenWidth, int screenHeight) {
    unittopixelsset = 1;
    double sw = screenWidth;
    double sh = screenHeight;
    double size = (screenWidth + screenHeight) / 2;
    double unittopixels = 50.0 * (size / 700);
    UNIT_TO_PIXELS = (int)(unittopixels + 0.5);
}

#endif // USE_GRAPHICS

