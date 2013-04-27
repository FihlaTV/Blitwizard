
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

#ifndef NDEBUG
// comment this line if you don't want debug output:
#define DEBUGTEXTUREMANAGER
#endif

#include "os.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef USE_GRAPHICS

#include "threading.h"
#include "graphics.h"
#include "graphicstexturemanager.h"
#include "logging.h"
#include "graphicstextureloader.h"
#include "timefuncs.h"
#include "graphicstexturelist.h"
#include "graphicstextureloader.h"

// texture system memory budget in megabyte:
int textureSysMemoryBudget = 10;

// texture gpu memory budget in megabyte:
int textureGpuMemoryBudget = 10;

//

mutex* textureReqListMutex = NULL;
struct texturerequesthandle {
    // the respective texture list entry:
    struct graphicstexturemanaged* gtm;

    // callback information:
    void (*textureDimensionInfoCallback)(
    struct texturerequesthandle* request,
    size_t width, size_t height, void* userdata);
    void (*textureSwitchCallback)(
    struct texturerequesthandle* request,
    struct graphicstexture* texture, void* userdata);
    void* userdata;

    // remember if we already handed a texture to this request:
    struct graphicstexture* handedTexture;
 
    // if the request was cancelled, the callbacks
    // are gone and must not be used anymore:
    int canceled;

    // prev, next for regular request list:
    struct texturerequesthandle* prev, *next;

    // prev, next for unhandled request list:
    struct texturerequesthandle* unhandledPrev, *unhandledNext;
};

struct texturerequesthandle* textureRequestList = NULL;
struct texturerequesthandle* unhandledRequestList = NULL;

// manager thread which unloads textures when memory budget is used up:
void texturemanager_Watchdog(__attribute__((unused)) void* unused) {
    // this is the core decider place that makes sure
    // things are moved in/out of caches at the right time.
    //
    // it runs in a separate thread so it can act any time,
    // no matter what the main application is doing at this point.

    // to ensure the disk cache is initialised, we will wait
    // a short moment:
    time_Sleep(500);

    while (1) {
        mutex_Lock(textureReqListMutex);
        // unload textures :P (FIXME)
        mutex_Release(textureReqListMutex);

        // sleep a moment:
        time_Sleep(100);
    }
}

// this runs on application start:
__attribute__((constructor)) static void texturemanager_Init(void) {
    textureReqListMutex = mutex_Create();

    threadinfo* tinfo = thread_CreateInfo();
    if (!tinfo) {
        printerror("Failed to allocate thread info for texture manager"
        " watch dog");
        exit(0);
        return;
    }

    // spawn watch dog:
    thread_Spawn(tinfo, texturemanager_Watchdog, NULL);
    thread_FreeInfo(tinfo);
}


// obtain best gpu texture available right now.
// might be NULL if none is in gpu memory.
struct graphicstexture* texturemanager_ObtainBestGpuTexture(
struct graphicstexturemanaged* gtm, size_t width, size_t height) {
    return NULL;
}

static void texturemanager_InitialLoadingDimensionsCallback
(struct graphicstexturemanaged* gtm, size_t width, size_t height,
int success, void* userdata);

static void texturemanager_InitialLoadingDataCallback
(struct graphicstexturemanaged* gtm, int success, void* userdata);

static void texturemanager_ProcessRequest(struct texturerequesthandle*
request, int listLocked) {
    if (!listLocked) {
        mutex_Lock(textureReqListMutex);
    }
    // handle a request which has been waiting for a texture.
    int successfullyhandled = 0;

    struct graphicstexturemanaged* gtm =
    request->gtm;

    // if texture isn't loaded yet, load it:
    if (!gtm->beingInitiallyLoaded && !gtm->failedToLoad) {
        int donotload = 0;
        // check if texture is available in original size:
        if (gtm->scalelistcount > 0 && gtm->origscale >= 0) {
            if (gtm->scalelist[gtm->scalelistcount].diskcachepath
            || gtm->scalelist[gtm->scalelistcount].pixels
            || gtm->scalelist[gtm->scalelistcount].gt) {
                donotload = 1;
            }
        }
        if (!donotload) {
            printinfo("[TEXMAN] initial texture fetch for %s", gtm->path);
            gtm->beingInitiallyLoaded = 1;

            // fire up the threaded loading process which
            // gives us the texture in the most common sizes
            graphicstextureloader_DoInitialLoading(gtm,
            texturemanager_InitialLoadingDimensionsCallback,
            texturemanager_InitialLoadingDataCallback,
            request);
        }
    }

    // find a texture matching the requested size:
    size_t width = 0;
    size_t height = 0; // FIXME, have proper size parameters
    struct graphicstexture* gt = texturemanager_ObtainBestGpuTexture(
    gtm, width, height);

    if (gt != request->handedTexture) {
        // remember we handed this out:
        successfullyhandled = 1;
        request->handedTexture = gt;

        // hand out texture:
        request->textureSwitchCallback(request, gt, request->userdata);
    }

    // if it was successfully handled, move it to the regular request list:
    if (successfullyhandled && (request->unhandledPrev
    || request->unhandledNext || unhandledRequestList == request)) {
        if (!request->prev && !request->next &&
        !(textureRequestList == request)) {
            // add it to regular list:
            request->next = textureRequestList;
            if (request->next) {
                request->next->prev = request;
            }
            request->prev = NULL;
            textureRequestList = request;
        }
        // remove from unhandled request list:
        if (request->unhandledPrev) {
            request->unhandledPrev->unhandledNext = request->unhandledNext;
        } else {
            textureRequestList = request->unhandledNext;
        }
        request->unhandledNext->unhandledPrev = request->unhandledPrev;
    }
    if (!listLocked) {
        mutex_Release(textureReqListMutex);
    }
}

// callback with early dimension/size info:
static void texturemanager_InitialLoadingDimensionsCallback
(struct graphicstexturemanaged* gtm, size_t width, size_t height,
int success, void* userdata) {
    struct texturerequesthandle* request = userdata;
    mutex_Lock(textureReqListMutex);
    printinfo("[TEXMAN] initial loading done for %s", gtm->path);
    if (!success) {
        gtm->failedToLoad = 1;
        width = 0;
        height = 0;
        gtm->beingInitiallyLoaded = 0;
    }
    request->textureDimensionInfoCallback(request,
    width, height, request->userdata);
    mutex_Release(textureReqListMutex);
}

// callback when texture was loaded fully:
static void texturemanager_InitialLoadingDataCallback
(struct graphicstexturemanaged* gtm, int success, void* userdata) {
    mutex_Lock(textureReqListMutex);
    struct texturerequesthandle* request = userdata;
    if (!success) {
        gtm->failedToLoad = 1;
        gtm->beingInitiallyLoaded = 0;
        request->textureDimensionInfoCallback(request,
        0, 0, request->userdata);
        mutex_Release(textureReqListMutex);
        return;
    }
    texturemanager_ProcessRequest(request, 1);
    mutex_Release(textureReqListMutex);
}

struct texturerequesthandle* texturemanager_RequestTexture(
const char* path,
void (*textureDimensionInfo)(struct texturerequesthandle* request,
size_t width, size_t height, void* userdata),
void (*textureSwitch)(struct texturerequesthandle* request,
struct graphicstexture* texture, void* userdata),
void* userdata) {
#ifdef DEBUGTEXTUREMANAGER
    printinfo("[TEXMAN] texture %s requested", path);
#endif

    // allocate request:
    struct texturerequesthandle* request = malloc(sizeof(*request));
    if (!request) {
        return NULL;
    }
    memset(request, 0, sizeof(*request));
    request->textureSwitchCallback = textureSwitch;
    request->textureDimensionInfoCallback = textureDimensionInfo;
    request->userdata = userdata;

    // locate according texture entry:
    struct graphicstexturemanaged* gtm =
    graphicstexturelist_GetTextureByName(path);
    if (!gtm) {
        // add new texture entry:
        gtm = graphicstexturelist_AddTextureToList(
        path);
        if (!gtm) {
            free(request);
            return NULL;
        }
        graphicstexturelist_AddTextureToHashmap(
        gtm);
    }
    request->gtm = gtm;

    mutex_Lock(textureReqListMutex);
    // add to unhandled texture request list:
    request->unhandledNext = unhandledRequestList;
    if (request->unhandledNext) {
        request->unhandledNext->unhandledPrev = request;
    }
    unhandledRequestList = request;

    mutex_Release(textureReqListMutex);

    return request;
}

void texturemanager_UsingRequest(
struct texturerequesthandle* request, int visibility) {
    
}

void texturemanager_DestroyRequest(
struct texturerequesthandle* request) {
    if (!request) {
        return;
    }

    free(request);
}

static void texturemanager_ProcessUnhandledRequests(void) {
    struct texturerequesthandle* handle = unhandledRequestList;
    while (handle) {
        texturemanager_ProcessRequest(handle, 1);
        handle = handle->unhandledNext;
    }
}

void texturemanager_Tick(void) {
    mutex_Lock(textureReqListMutex);
    texturemanager_ProcessUnhandledRequests();
    mutex_Release(textureReqListMutex);
}


#endif  // USE_GRAPHICS


