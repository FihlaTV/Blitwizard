
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

#include <string.h>

#include "graphicstexture.h"
#include "graphicstextureloader.h"
#include "graphicstexturelist.h"
#include "threading.h"
#include "resources.h"
#include "logging.h"

struct graphicstextureloader_initialLoadingThreadInfo {
    void (*callbackDimensions)(struct graphicstexturemanaged* gtm,
    size_t width, size_t height, int success, void* userdata);
    void (*callbackData)(struct graphicstexturemanaged* gtm,
    int success, void* userdata);
    void* userdata;
    char* path;

    // this texture pointer must be treated as black box
    // by the loader thread, because other threads might
    // be messing around with it:
    struct graphicstexturemanaged* gtm;
};

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
        
    } else if (loc.type == LOCATION_TYPE_ZIP) {

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
        return NULL;
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

