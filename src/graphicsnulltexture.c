
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
#include "graphicsnulltexturestruct.h"
#include "threading.h"


void graphicstexture_destroy(struct graphicstexture *gt) {
    if (!thread_IsMainThread()) {
        return;
    }
    if (gt->pixdata) {
        free(gt->pixdata);
    }
    free(gt);
}

int graphicstexture_getDesiredFormat(void) {
    return PIXELFORMAT_32RGBA;
}

struct graphicstexture *graphicstexture_create(void *data,
        size_t width, size_t height, size_t paddedWidth, size_t paddedHeight,
        int format) {
    if (!thread_IsMainThread()) {
        return NULL;
    }
    // create basic texture struct:
    struct graphicstexture *gt = malloc(sizeof(*gt));
    if (!gt) {
        return NULL;
    }
    memset(gt, 0, sizeof(*gt));
    gt->width = width;
    gt->height = height;
    gt->paddedWidth = paddedWidth;
    gt->paddedHeight = paddedHeight;

    // format conversion:
    switch (format) {
    case PIXELFORMAT_32RGBA:
        // we can process this as usual.
        break;
    default:
        // not known to us!
        graphicstexture_Destroy(gt);
        return NULL;
    }
    gt->format = format;

    // create hw texture
    gt->pixdata = malloc(sizeof(uint8_t) * 4 * gt->width * gt->height);
    if (!gt->pixdata) {
        graphicstexture_Destroy(gt);
        return NULL;
    }
    memcpy(gt->pixdata, data, sizeof(uint8_t) * 4 * gt->width * gt->height);

    return gt;
}

void graphics_getTextureDimensions(struct graphicstexture *texture,
        size_t *width, size_t *height) {
    *width = texture->width;
    *height = texture->height;
}

int graphics_getTextureFormat(struct graphicstexture *gt) {
    return gt->format;
}

int graphicstexture_pixelsFromTexture(
        struct graphicstexture *gt, void *pixels) {
    if (!thread_IsMainThread()) {
        return 0;
    }
    memcpy(pixels, gt->pixdata, gt->width * gt->height * 4);
    return 1;
}


#endif  // USE_NULL_GRAPHICS

