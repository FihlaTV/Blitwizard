
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

#ifndef _NDEBUG
// uncomment if you want debug output:
#define DEBUGTEXTURELOADER
#endif

#include "os.h"

#include <stdio.h>
#include <string.h>

#include "graphicstexture.h"
#include "graphicstextureloader.h"
#include "graphicstexturelist.h"
#include "threading.h"
#include "resources.h"
#include "logging.h"
#include "imgloader/imgloader.h"

struct graphicstextureloader_initialLoadingThreadInfo {
    void (*callbackDimensions)(struct graphicstexturemanaged* gtm,
    size_t width, size_t height, int success, void* userdata);
    void (*callbackData)(struct graphicstexturemanaged* gtm,
    int success, void* userdata);
    void* userdata;
    char* path;

    // remember size temporarily:
    size_t width, height;

    // this texture pointer must be treated as black box
    // by the loader thread, because other threads might
    // be messing around with it:
    struct graphicstexturemanaged* gtm;
};

void graphicstextureloader_callbackSize(void* handle,
int width, int height, void* userdata) {
    struct graphicstextureloader_initialLoadingThreadInfo* info =
    userdata;
    if (width > 0 && height > 0) {
        info->callbackDimensions(info->gtm, width, height,
        1, info->userdata);
        info->width = width;
        info->height = height;
    } else {
        info->callbackDimensions(info->gtm, 0, 0, 0, info->userdata);
    }
}

void graphicstextureloader_callbackData(void* handle,
char* imgdata, unsigned int imgdatasize, void* userdata) {
    struct graphicstextureloader_initialLoadingThreadInfo* info =
    userdata;

    if (imgdata) {
        texturemanager_LockForTextureAccess();
        if (!info->gtm->scalelist) {
            int scalecount = 1;

            // check for tiny size:
            if (info->width > TEXSIZE_TINY || info->height > TEXSIZE_TINY) {
                scalecount++;
            }

            // check for low size:
            if (info->width > TEXSIZE_LOW || info->height > TEXSIZE_LOW) {
                scalecount++;
            }

            // check if this texture needs a medium size:
            if (info->width > TEXSIZE_MEDIUM ||
            info->height > TEXSIZE_MEDIUM) {
                scalecount++;
            }

            // check if this texture needs a scaled down high size:
            if (info->width > TEXSIZE_HIGH || info->height > TEXSIZE_HIGH) {
                scalecount++;
            }

            // allocate scaled texture list:
            info->gtm->scalelist = malloc(
            sizeof(struct graphicstexturescaled)*scalecount);
            if (!info->gtm->scalelist) {
                // allocation failed.
                free(imgdata);
                texturemanager_ReleaseFromTextureAccess();
                info->callbackData(info->gtm, (imgdata != NULL), info->userdata);
                return;
            }
            // initialise list:
            info->gtm->scalelistcount = scalecount;
            memset(info->gtm->scalelist, 0,
            sizeof(struct graphicstexturescaled)*scalecount);

            // fill scaled texture list:
            int i = 0;
            while (i < scalecount) {
                if (i == 0) {
                    // original size
                    info->gtm->scalelist[i].pixels = imgdata;
                    info->gtm->scalelist[i].width = info->width;
                    info->gtm->scalelist[i].height = info->height;
                    info->gtm->origscale = i;
#ifdef DEBUGTEXTURELOADER
                    printinfo("[TEXLOAD] texture has now been loaded: %s",
                    info->path);
#endif
                } else {
                    // see what would be the intended side size:
                    int intendedsize;
                    switch (i) {
                    case 1:
                        intendedsize = TEXSIZE_TINY;
                        break;
                    case 2:
                        intendedsize = TEXSIZE_LOW;
                        break;
                    case 3:
                        intendedsize = TEXSIZE_MEDIUM;
                        break;
                    case 4:
                        intendedsize = TEXSIZE_HIGH;
                        break;
                    }

                    // check what actual side lengths are senseful:
                    size_t sidelength_x = intendedsize;
                    size_t sidelength_y = intendedsize;
                    while (sidelength_x > info->width) {
                        sidelength_x /= 2;
                    }
                    while (sidelength_y > info->height) {
                        sidelength_y /= 2;
                    }

                    // set size info:
                    info->gtm->scalelist[i].width = sidelength_x;
                    info->gtm->scalelist[i].height = sidelength_y;
                }
                i++;
            }
        } else {
            // scale list already there. shwoot
            free(imgdata);
            imgdata = NULL;
        }
        texturemanager_ReleaseFromTextureAccess();
    }

    info->callbackData(info->gtm, (imgdata != NULL), info->userdata);
}

void graphicstextureloader_InitialLoaderThread(void* userdata) {
    struct graphicstextureloader_initialLoadingThreadInfo* info =
    userdata;
#ifdef DEBUGTEXTURELOADER
    printinfo("[TEXLOAD] starting initial loading of %s", info->path);
#endif

    struct resourcelocation loc;
    if (!resources_LocateResource(info->path, &loc)) {
#ifdef DEBUGTEXTURELOADER
        printinfo("[TEXLOAD] resource not found: %s", info->path);
#endif
        info->callbackDimensions(info->gtm, 0, 0, 0, info->userdata);
        free(info->path);
        free(info);
        return;
    }

    if (loc.type == LOCATION_TYPE_DISK) {
        void* handle = img_LoadImageThreadedFromFile(info->path,
        4096, 4096, "rgba", graphicstextureloader_callbackSize,
        graphicstextureloader_callbackData, info);
//    } else if (loc.type == LOCATION_TYPE_ZIP) {

    } else {
        printwarning("[TEXLOAD] unsupported resource location");
        info->callbackDimensions(info->gtm, 0, 0, 0, info->userdata);
        free(info->path);
        free(info);
        return;
    }
}

void graphicstextureloader_DoInitialLoading(struct graphicstexturemanaged* gtm,
void (*callbackDimensions)(struct graphicstexturemanaged* gtm,
size_t width, size_t height, int success,
void* userdata),
void (*callbackData)(struct graphicstexturemanaged* gtm, int success,
void* userdata),
void* userdata) {
    // create loader thread info:
    struct graphicstextureloader_initialLoadingThreadInfo* info =
    malloc(sizeof(*info));
    if (!info) {
        callbackDimensions(gtm, 0, 0, 0, userdata);
        return;
    }

    // duplicate path:
    info->path = strdup(gtm->path);
    if (!info->path) {
        free(info);
        callbackDimensions(gtm, 0, 0, 0, userdata);
        return;
    }

    // store various info:
    info->gtm = gtm;
    info->callbackDimensions = callbackDimensions;
    info->callbackData = callbackData;
    info->userdata = userdata;

    // spawn our loader thread:
    threadinfo* ti = thread_CreateInfo();
    if (!ti) {
        free(info->path);
        free(info);
        callbackDimensions(gtm, 0, 0, 0, userdata);
        return;
    }
    thread_Spawn(ti, graphicstextureloader_InitialLoaderThread, info);
    thread_FreeInfo(ti);
}

