
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

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include "blitwizard_opengl.h"

#include "graphicstexture.h"
#include "graphics.h"
#include "graphicssdltexturestruct.h"
#include "graphicssdlglext.h"
#include "threading.h"

// uncomment for no texture upload timing measurements:
#define DEBUGUPLOADTIMING


#ifdef NDEBUG
#undef DEBUGPLOADTIMING
#endif

#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
extern SDL_GLContext *maincontext;
#endif
extern SDL_Window* mainwindow;
extern SDL_Renderer* mainrenderer;


void graphicstexture_destroy(struct graphicstexture *gt) {
    if (!thread_isMainThread()) {
        return;
    }
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    if (maincontext) {
        // delete the OpenGL PBO texture
        if (gt->texid > 0) {
            glDeleteTextures(1, &gt->texid);
            gt->texid = 0;
        }
        if (gt->pboid > 0) {
            glDeleteBuffers(1, &gt->pboid);
            gt->pboid = 0;
        }
    } else {
#endif
        // delete ordinary SDL texture
        if (gt->sdltex) {
            SDL_DestroyTexture(gt->sdltex);
        }
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    }
#endif
    free(gt);
}

static int graphicstexture_SDLFormatToPixelFormat(int format) {
    switch (format) {
    case SDL_PIXELFORMAT_ABGR8888:
        return PIXELFORMAT_32RGBA;
    case SDL_PIXELFORMAT_BGRA8888:
        return PIXELFORMAT_32ARGB;
    case SDL_PIXELFORMAT_RGBA8888:
        return PIXELFORMAT_32ABGR;
    case SDL_PIXELFORMAT_ARGB8888:
        return PIXELFORMAT_32BGRA;
    default:
        return PIXELFORMAT_UNKNOWN;
    }
}

static volatile int infoprinted = 0;
int graphicstexture_getDesiredFormat(void) {
#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    if (maincontext) {
        return PIXELFORMAT_32BGRA_UPSIDEDOWN;
    }
#endif
    SDL_RendererInfo rinfo;
    if (SDL_GetRendererInfo(mainrenderer, &rinfo) == 0) {
        unsigned int i = 0;
        while (i < rinfo.num_texture_formats) {
            int pixelformat = graphicstexture_SDLFormatToPixelFormat(
                rinfo.texture_formats[i]);
            if (pixelformat != PIXELFORMAT_UNKNOWN) {
                if (!infoprinted) {
                    infoprinted = 1;
                    printinfo("[sdltex] Pixel format found: %d\n",
                        pixelformat);
                }
                return pixelformat;
            }
            i++;
        }
        if (!infoprinted) {
            printinfo("[sdltex] no known pixel format in %d "
                "supported formats of renderer", rinfo.
                num_texture_formats);
        } 
    } else {
        printwarning("[sdltex] SDL_GetRendererInfo call failed: %s",
            SDL_GetError());
    }
    if (!infoprinted) {
        infoprinted = 1;
        printwarning("[sdltex] cannot find optimal pixel format!");
    }
    return PIXELFORMAT_32RGBA;
}

static int graphicstexture_pixelFormatToSDLFormat(int format) {
    // blitwizard pixel format to SDL pixel format
    switch (format) {
    case PIXELFORMAT_32RGBA:
        return SDL_PIXELFORMAT_BGRA8888;
    case PIXELFORMAT_32ARGB:
        return SDL_PIXELFORMAT_ARGB8888;
    case PIXELFORMAT_32ABGR:
        return SDL_PIXELFORMAT_ABGR8888;
    case PIXELFORMAT_32BGRA:
        return SDL_PIXELFORMAT_BGRA8888;
    default:
        return -1;
    }
}

#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
int graphicstexture_bindGl(struct graphicstexture *gt, uint64_t time) {
    if (gt->uploadtime + 30 > time) {
        // wait for it to upload first.
        return 0;
    }
    glBindTexture(GL_TEXTURE_2D, gt->texid);
    return 1;
}

struct graphicstexture *graphicstexture_createHWPBO(
        struct graphicstexture *gt,
        void *data,
        size_t width, size_t height, int format, uint64_t time) {
    if (format != PIXELFORMAT_32BGRA_UPSIDEDOWN) {
        printwarning("graphicstexture_createHWPBO: failed; unsupported "
            "pixel format %d requested", format);
        return NULL;
    }
    // clear OpenGL error:
    GLenum err;
    if ((err = glGetError()) != GL_NO_ERROR) {
        printwarning("graphicstexture_createHWPBO: lingering error %s",
            glGetErrorString(err));
        while (glGetError() != GL_NO_ERROR) {
            glGetError();
        }
    }
    
    glGenBuffers(1, &gt->pboid);
    if ((err = glGetError()) != GL_NO_ERROR) {
        gt->pboid = 0;
        printwarning("graphicstexture_createHWPBO: PBO creation: "
            "glGenBuffers failed");
        goto failure;
    }
    // bind buffer and fill
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, gt->pboid);
    if ((err = glGetError()) != GL_NO_ERROR) {
        printwarning("graphicstexture_createHWPBO: PBO CREATION: "
            "glBindBuffer failed");
        goto failure;
    }
    glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, width * height * 4,
        data, GL_STREAM_DRAW_ARB);
    if ((err = glGetError()) != GL_NO_ERROR) {
        printwarning("graphicstexture_createHWPBO: PBO creation: "
            "glBufferData failed");
        goto failure;
    }

    glActiveTexture(GL_TEXTURE0);

    // create a new texture:
    glGenTextures(1, &gt->texid);
    if ((err = glGetError()) != GL_NO_ERROR) {
        gt->texid = 0;
        printwarning("graphicstexture_createHWPBO: texture creation: "
            "glGenTextures failed");
        goto failure;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    if ((err = glGetError()) != GL_NO_ERROR) {
        gt->texid = 0;
        printwarning("graphicstexture_createHWPBO: texture creation: "
        "glTexParameteri failed (GL_TEXTURE_BASE_LEVEL)");
        goto failure;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    if ((err = glGetError()) != GL_NO_ERROR) {
        gt->texid = 0;
        printwarning("graphicstexture_createHWPBO: texture creation: "
            "glTexParameteri failed (GL_TEXTURE_MAX_LEVEL)");
        goto failure;
    }

    // transfer PBO to texture:
    glBindTexture(GL_TEXTURE_2D, gt->texid);
    if ((err = glGetError()) != GL_NO_ERROR) {
        printwarning("graphicstexture_createHWPBO: transfer: "
            "glBindTexture failed");
        goto failure;
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, gt->pboid);
    if ((err = glGetError()) != GL_NO_ERROR) {
        printwarning("graphicstexture_createHWPBO: transfer: "
            "glBindBuffer failed");
        goto failure;
    }
    glTexImage2D(GL_TEXTURE_2D,
        0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8,
        0);
    if ((err = glGetError()) != GL_NO_ERROR) {
        printwarning("graphicstexture_createHWPBO: transfer: "
            "glTexImage2D failed");
        goto failure;
    }
    gt->uploadtime = time;
    return gt;

failure:
    printwarning("graphicstexture_createHWPBO: failed; OpenGL "
        "error string is: %s", glGetErrorString(err));
    // clean up all error bits:
    while (glGetError() != GL_NO_ERROR) {
        glGetError();
    }
    // destroy texture:
    graphicstexture_destroy(gt);
    return NULL;
}
#endif

const char *pixelformattoname(int format);
struct graphicstexture *graphicstexture_createHWSDL(
        struct graphicstexture *gt,
        void *data,
        size_t width, size_t height, int format,
        __attribute__((unused)) uint64_t time) {
    // create hw texture
#ifdef DEBUGUPLOADTIMING
    uint64_t ts1 = time_getMilliseconds();
#endif
    gt->sdltex = SDL_CreateTexture(mainrenderer,
        graphicstexture_pixelFormatToSDLFormat(format),
        SDL_TEXTUREACCESS_STREAMING,
        width, height);
    if (!gt->sdltex) {
        graphicstexture_destroy(gt);
        return NULL;
    }
#ifdef DEBUGUPLOADTIMING
    uint64_t ts2 = time_getMilliseconds();
#endif

    // lock texture
    void *pixels;
    int pitch;
    if (SDL_LockTexture(gt->sdltex, NULL, &pixels, &pitch) != 0) {
        graphicstexture_destroy(gt);
        return NULL;
    }
#ifdef DEBUGUPLOADTIMING
    uint64_t ts3 = time_getMilliseconds();
#endif

    // copy pixels into texture
    memcpy(pixels, data, gt->width * gt->height * 4);
    // FIXME: we probably need to handle pitch here??

#ifdef DEBUGUPLOADTIMING
    uint64_t ts4 = time_getMilliseconds();
#endif

    // unlock texture
    SDL_UnlockTexture(gt->sdltex);

#ifdef DEBUGUPLOADTIMING
    uint64_t ts5 = time_getMilliseconds();
    if (ts5-ts1 > 100) {
        printwarning("[sdltex] texture upload duration: %dms total, "
            "%dms creation, %dms lock, %dms copy, %dms unlock",
            (int)(ts5-ts1), (int)(ts2-ts1), (int)(ts3-ts2),
            (int)(ts4-ts3), (int)(ts5-ts4));
    }
#endif

    // set blend mode
    SDL_SetTextureBlendMode(gt->sdltex, SDL_BLENDMODE_BLEND);

    return gt;
}

struct graphicstexture *graphicstexture_create(void *data,
        size_t width, size_t height, int format, uint64_t time) {
    if (!thread_isMainThread()) {
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
    gt->format = format;

#ifdef USE_SDL_GRAPHICS_OPENGL_EFFECTS
    if (maincontext) {
        return graphicstexture_createHWPBO(gt, data, width, height, format,
            time);
    }
#endif
    return graphicstexture_createHWSDL(gt, data, width, height, format,
        time); 
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
    if (!thread_isMainThread()) {
        return 0;
    }
    // Lock SDL Texture
    void *pixelsptr;
    int pitch;
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

