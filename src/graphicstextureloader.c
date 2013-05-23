
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
//#define DEBUGTEXTURELOADER
#endif

#include "config.h"
#include "os.h"

#ifdef USE_GRAPHICS

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "zipfile.h"
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
        texturemanager_lockForTextureAccess();
        info->gtm->width = info->width;
        info->gtm->height = info->height;
        if (!info->gtm->scalelist) {
            int scalecount = 1;

            // for smaller textures, divide the sizes we use for downscaling:
            unsigned int sizedivisor = 1;
            if (info->width < 64 || info->height < 64) {
                sizedivisor = 8;
            } else if (info->width < 128 || info->height < 128) {
                sizedivisor = 4;
            } else if (info->width < 256 || info->height < 256) {
                sizedivisor = 2;
            }

            // now we only want to have those sizes that are actually
            // smaller than our current image:

            // check for tiny size:
            if (info->width > TEXSIZE_TINY/sizedivisor ||
            info->height > TEXSIZE_TINY/sizedivisor) {
                scalecount++;
            }

            // check for low size:
            if (info->width > TEXSIZE_LOW/sizedivisor ||
            info->height > TEXSIZE_LOW/sizedivisor) {
                scalecount++;
            }

            // check if this texture needs a medium size:
            if (info->width > TEXSIZE_MEDIUM/sizedivisor ||
            info->height > TEXSIZE_MEDIUM/sizedivisor) {
                scalecount++;
            }

            // check if this texture needs a scaled down high size:
            if (info->width > TEXSIZE_HIGH/sizedivisor ||
            info->height > TEXSIZE_HIGH/sizedivisor) {
                scalecount++;
            }

            // allocate scaled texture list:
            info->gtm->scalelist = malloc(
            sizeof(struct graphicstexturescaled) * scalecount);
            if (!info->gtm->scalelist) {
                // allocation failed.
                free(imgdata);
                texturemanager_releaseFromTextureAccess();
                info->callbackData(info->gtm, (imgdata != NULL), info->userdata);
                return;
            }
            // initialise list:
            info->gtm->scalelistcount = scalecount;
            memset(info->gtm->scalelist, 0,
            sizeof(struct graphicstexturescaled) * scalecount);

            // fill scaled texture list:
            int i = 0;
            while (i < scalecount) {
                info->gtm->scalelist[i].parent = info->gtm;
                if (i == 0) {
                    // original size
                    info->gtm->scalelist[i].pixels = imgdata;
                    info->gtm->scalelist[i].width = info->width;
                    info->gtm->scalelist[i].height = info->height;
                    info->gtm->origscale = i;
#ifdef DEBUGTEXTURELOADER
                    printinfo("[TEXLOAD] texture has now been loaded: %s "
                    "(%d, %d)",
                    info->path, info->width, info->height);
#endif
                } else {
                    // see what would be the intended side size:
                    int intendedsize = 0;
                    switch (i) {
                    case 1:
                        intendedsize = TEXSIZE_TINY/sizedivisor;
                        break;
                    case 2:
                        intendedsize = TEXSIZE_LOW/sizedivisor;
                        break;
                    case 3:
                        intendedsize = TEXSIZE_MEDIUM/sizedivisor;
                        break;
                    case 4:
                        intendedsize = TEXSIZE_HIGH/sizedivisor;
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
#ifdef DEBUGTEXTURELOADER
            printinfo("[TEXLOAD] imgloader is confused about: %s",
            info->path);
#endif
            // scale list already there. shwoot
            free(imgdata);
            imgdata = NULL;
        }
        texturemanager_releaseFromTextureAccess();
    } else {
#ifdef DEBUGTEXTURELOADER
        printinfo("[TEXLOAD] imgloader reported failure for: %s",
        info->path);
#endif
    }

    info->callbackData(info->gtm, (imgdata != NULL), info->userdata);
}

struct loaderfuncinfo {
    struct graphicstextureloader_initialLoadingThreadInfo* info;
    struct zipfilereader* file;
    struct zipfile* archive;
};

static int graphicstextureloader_ImageReadFunc(void* buffer,
size_t bytes, void* userdata) {
    struct loaderfuncinfo* lfi = userdata;
    
    if (!lfi->file) {
        lfi->file = zipfile_FileOpen(lfi->archive, lfi->info->path);
        if (!lfi->file) {
            free(lfi);
            return -1;  // error: cannot open file
        }
    }
    int i = zipfile_FileRead(lfi->file, buffer, bytes);
    if (i <= 0) {
        zipfile_FileClose(lfi->file);
        free(lfi);
        return 0;
    }
    return i;
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
        // use standard disk file image loader:
        void* handle = img_LoadImageThreadedFromFile(info->path,
        4096, 4096, "rgba", graphicstextureloader_callbackSize,
        graphicstextureloader_callbackData, info);
    } else if (loc.type == LOCATION_TYPE_ZIP) {
        // prepare image reader info struct:
        struct loaderfuncinfo* lfi = malloc(sizeof(*lfi));
        if (!lfi) {
            printwarning("[TEXLOAD] memory allocation failed (1)");
            free(info->path);
            free(info);
            return;
        }
        memset(lfi, 0, sizeof(*lfi));
        lfi->info = info;
        lfi->archive = loc.location.ziplocation.archive;

        // prompt image loader with our own image reader:
        void* handle = img_LoadImageThreadedFromFunction(
        graphicstextureloader_ImageReadFunc, lfi,
        4096, 4096, "rgba", graphicstextureloader_callbackSize,
        graphicstextureloader_callbackData, info);
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

    // give texture initial normal usage to start with:
    info->gtm->lastUsage[USING_AT_VISIBILITY_NORMAL] = time(NULL);

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

#endif  // USE_GRAPHICS

