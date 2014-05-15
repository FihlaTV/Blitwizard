
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

#ifndef _NDEBUG
// uncomment if you want debug output:
//#define DEBUGTEXTURELOADER
#endif

#define MAXLOADWIDTH 10000
#define MAXLOADHEIGHT 10000

#include "config.h"
#include "os.h"

#ifdef USE_GRAPHICS

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef USE_PHYSFS
#include "zipfile.h"
#endif
#include "graphicstexture.h"
#include "graphicstextureloader.h"
#include "graphicstexturelist.h"
#include "threading.h"
#include "resources.h"
#include "logging.h"
#include "imgloader/imgloader.h"

struct loaderfuncinfo;
// the main info struct for our texture loader thread:
struct graphicstextureloader_initialLoadingThreadInfo {
    void (*callbackDimensions)(struct graphicstexturemanaged *gtm,
    size_t width, size_t height, int success, void *userdata);
    void (*callbackData)(struct graphicstexturemanaged *gtm,
        int success, void *userdata);
    void *userdata;
    char *path;
    int padnpot;
    int failed;

    // texture format we decided on:
    int format;

    // present if loading from memory:
    struct loaderfuncinfo *linfo;

    // remember size temporarily:
    size_t width, height;
    size_t paddedWidth, paddedHeight;

    // this texture pointer must be treated as black box
    // by the loader thread, because other threads might
    // be messing around with it:
    struct graphicstexturemanaged *gtm;
};

// the info struct for the file I/O function that parses from memory
struct loaderfuncinfo {
#ifdef USE_PHYSFS
    struct graphicstextureloader_initialLoadingThreadInfo *info;
    struct zipfilereader *file;
    struct zipfile *archive;
#endif
};

static void freeinitialloadinginfo(
        struct graphicstextureloader_initialLoadingThreadInfo *info) {
    if (info->linfo) {
#ifdef USE_PHYSFS
        if (info->linfo->file) {
            zipfile_FileClose(info->linfo->file);
            info->linfo->file = NULL;
        }
#endif
        free(info->linfo);
    }
    free(info);
}

void graphicstextureloader_callbackSize(void *handle,
int width, int height, void *userdata) {
    struct graphicstextureloader_initialLoadingThreadInfo *info =
    userdata;
    if (width > 0 && height > 0) {
        // store size, padded size:
        if (info->padnpot) {
            info->paddedWidth = imgloader_getPaddedSize(width);
            info->paddedHeight = imgloader_getPaddedSize(height);
        } else {
            info->paddedWidth = width;
            info->paddedHeight = height;
        }
        info->width = width;
        info->height = height;

        // set the size to the gtm texture struct:
        texturemanager_lockForTextureAccess();
        info->gtm->width = width;
        info->gtm->height = height;
        texturemanager_releaseFromTextureAccess();

        // report back the size:
        info->callbackDimensions(info->gtm, width, height,
        1, info->userdata);
    } else {
        // report failure:
        info->callbackDimensions(info->gtm, 0, 0, 0, info->userdata);
        info->failed = 1;
    }
}

void graphicstextureloader_callbackData(void *handle,
        char *imgdata, unsigned int imgdatasize, void *userdata) {
    struct graphicstextureloader_initialLoadingThreadInfo *info =
    userdata;

    if (info->failed) {
        // we don't care to process any of this.
        if (imgdata) {
            free(imgdata);
        }
        freeinitialloadinginfo(info);
        return;
    }

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
                info->callbackData(info->gtm, 0, info->userdata);
                freeinitialloadinginfo(info);
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
                info->gtm->scalelist[i].format = info->format;
                if (i == 0) {
                    // original size
                    info->gtm->scalelist[i].pixels = imgdata;
                    info->gtm->scalelist[i].width = info->width;
                    info->gtm->scalelist[i].height = info->height;
                    info->gtm->scalelist[i].paddedWidth = info->paddedWidth;
                    info->gtm->scalelist[i].paddedHeight = info->paddedHeight;
                    assert(info->paddedWidth >= info->width);
#ifndef NDEBUG
                    if (info->padnpot) {
                        assert(info->paddedWidth ==
                            imgloader_getPaddedSize(info->width));
                        assert(info->paddedHeight ==
                            imgloader_getPaddedSize(info->height)); 
                    }
#endif
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
                    info->gtm->scalelist[i].paddedWidth = sidelength_x;
                    info->gtm->scalelist[i].paddedHeight = sidelength_y;
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
    freeinitialloadinginfo(info);
}

#ifdef USE_PHYSFS
static int graphicstextureloader_imageReadFunc(void *buffer,
size_t bytes, void *userdata) {
    struct loaderfuncinfo *lfi = userdata;
    if (bytes == 0) {
       return 0;
    }

    if (!lfi->file) {
        lfi->file = zipfile_FileOpen(lfi->archive, lfi->info->path);
        if (!lfi->file) {
            return -1;  // error: cannot open file
        }
    }
    int i = zipfile_FileRead(lfi->file, buffer, bytes);
    if (i <= 0) {
        zipfile_FileClose(lfi->file);
        lfi->file = NULL;
        return 0;
    }
    return i;
}
#endif

const char *pixelformattoname(int format) {
    switch (format) {
    case PIXELFORMAT_32RGBA:
        return "rgba";
    case PIXELFORMAT_32ARGB:
        return "argb";
    case PIXELFORMAT_32ABGR:
        return "abgr";
    case PIXELFORMAT_32BGRA:
        return "bgra";
    case PIXELFORMAT_32RGBA_UPSIDEDOWN:
        return "rgba_upsidedown";
    case PIXELFORMAT_32ARGB_UPSIDEDOWN:
        return "argb_upsidedown";
    case PIXELFORMAT_32ABGR_UPSIDEDOWN:
        return "abgr_upsidedown";
    case PIXELFORMAT_32BGRA_UPSIDEDOWN:
        return "bgra_upsidedown";
    default:
        return "unknown";
    }
}

void graphicstextureloader_initialLoaderThread(void *userdata) {
    struct graphicstextureloader_initialLoadingThreadInfo *info =
    userdata;
#ifdef DEBUGTEXTURELOADER
    printinfo("[TEXLOAD] starting initial loading of %s", info->path);
#endif

    struct resourcelocation loc;
    if (!resources_locateResource(info->path, &loc)) {
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
        info->format = graphicstexture_getDesiredFormat();
        void *handle = img_loadImageThreadedFromFile(info->path,
            MAXLOADWIDTH, MAXLOADHEIGHT, info->padnpot,
            pixelformattoname(info->format),
            graphicstextureloader_callbackSize,
            graphicstextureloader_callbackData, info);
        if (!handle) {
            info->callbackDimensions(info->gtm, 0, 0, 0,
                info->userdata);
            free(info->path);
            free(info);
            return;
        }
        img_freeHandle(handle);        
#ifdef USE_PHYSFS
    } else if (loc.type == LOCATION_TYPE_ZIP) {
        // prepare image reader info struct:
        struct loaderfuncinfo *lfi = malloc(sizeof(*lfi));
        if (!lfi) {
            printwarning("[TEXLOAD] memory allocation failed (1)");
            free(info->path);
            free(info);
            return;
        }
        memset(lfi, 0, sizeof(*lfi));
        info->linfo = lfi;
        info->format = graphicstexture_getDesiredFormat();
        lfi->info = info;
        lfi->archive = loc.location.ziplocation.archive;

        // prompt image loader with our own image reader:
        void *handle = img_loadImageThreadedFromFunction(
            graphicstextureloader_imageReadFunc, lfi,
            MAXLOADWIDTH, MAXLOADHEIGHT, info->padnpot,
            pixelformattoname(info->format),
            graphicstextureloader_callbackSize,
            graphicstextureloader_callbackData, info);
        if (!handle) {
            info->callbackDimensions(info->gtm, 0, 0, 0,
                info->userdata);
            free(info->path);
            free(info);
            return;           
        }
        img_freeHandle(handle);
#endif
    } else {
        printwarning("[TEXLOAD] unsupported resource location");
        info->callbackDimensions(info->gtm, 0, 0, 0, info->userdata);
        free(info->path);
        free(info);
        return;
    }
}

void graphicstextureloader_doInitialLoading(struct graphicstexturemanaged *gtm,
        void (*callbackDimensions)(struct graphicstexturemanaged *gtm,
        size_t width, size_t height, int success,
        void *userdata),
        void (*callbackData)(struct graphicstexturemanaged *gtm, int success,
        void *userdata),
        void *userdata) {
    // create loader thread info:
    struct graphicstextureloader_initialLoadingThreadInfo *info =
    malloc(sizeof(*info));
    if (!info) {
        callbackDimensions(gtm, 0, 0, 0, userdata);
        return;
    }
    memset(info, 0, sizeof(*info));

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
    info->padnpot = 1;

    // give texture initial normal usage to start with:
    info->gtm->lastUsage[USING_AT_VISIBILITY_NORMAL] = time(NULL);

    // spawn our loader thread:
    threadinfo* ti = thread_createInfo();
    if (!ti) {
        free(info->path);
        free(info);
        callbackDimensions(gtm, 0, 0, 0, userdata);
        return;
    }
    thread_spawnWithPriority(ti, 0,
        graphicstextureloader_initialLoaderThread, info);
    thread_freeInfo(ti);
}

#endif  // USE_GRAPHICS

