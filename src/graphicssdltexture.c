
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

#ifdef USE_SDL_GRAPHICS

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

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include "graphicstexture.h"
#include "graphics.h"
#include "graphicssdltexturestruct.h"
#include "threading.h"

extern SDL_Window* mainwindow;
extern SDL_Renderer* mainrenderer;


void graphicstexture_Destroy(struct graphicstexture* gt) {
    if (!thread_IsMainThread()) {
        return;
    }
    if (gt->sdltex) {
        SDL_DestroyTexture(gt->sdltex);
    }
    free(gt);
}


struct graphicstexture* graphicstexture_Create(void* data,
size_t width, size_t height, int format) {
    if (!thread_IsMainThread()) {
        return NULL;
    }
    // create basic texture struct:
    struct graphicstexture* gt = malloc(sizeof(*gt));
    if (!gt) {
        return NULL;
    }
    memset(gt, 0, sizeof(*gt));
    gt->width = width;
    gt->height = height;

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
    gt->sdltex = SDL_CreateTexture(mainrenderer,
    SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
    gt->width, gt->height);
    if (!gt->sdltex) {
        graphicstexture_Destroy(gt);
        return NULL;
    }

    // lock texture
    void* pixels; int pitch;
    if (SDL_LockTexture(gt->sdltex, NULL, &pixels, &pitch) != 0) {
        graphicstexture_Destroy(gt);
        return NULL;
    }

    // copy pixels into texture
    memcpy(pixels, data, gt->width * gt->height * 4);
    // FIXME: we probably need to handle pitch here??

    // unlock texture
    SDL_UnlockTexture(gt->sdltex);

    // set blend mode
    SDL_SetTextureBlendMode(gt->sdltex, SDL_BLENDMODE_BLEND);

    return gt;
}

void graphics_GetTextureDimensions(struct graphicstexture* texture,
size_t* width, size_t* height) {
    *width = texture->width;
    *height = texture->height;
}

int graphics_GetTextureFormat(struct graphicstexture* gt) {
    return gt->format;
}

int graphicstexture_PixelsFromTexture(
struct graphicstexture* gt, void* pixels) {
    if (!thread_IsMainThread()) {
        return 0;
    }
    // Lock SDL Texture
    void* pixelsptr;int pitch;
    if (SDL_LockTexture(gt->sdltex, NULL, &pixelsptr, &pitch) != 0) {
        // locking failed, we cannot extract pixels
        return 0;
    } else {
        // Copy texture to provided pixels pointer
        memcpy(pixels, pixelsptr, gt->width * gt->height * 4);

        // unlock texture again
        SDL_UnlockTexture(gt->sdltex);
        return 1;
    }
}


#endif  // USE_SDL_GRAPHICS

