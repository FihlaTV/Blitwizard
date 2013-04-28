
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
//#define DEBUGTEXTUREMANAGER
#endif

#include "os.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifdef USE_GRAPHICS

#include "threading.h"
#include "graphics.h"
#include "graphicstexturemanager.h"
#include "logging.h"
#include "graphicstextureloader.h"
#include "timefuncs.h"
#include "graphicstexturelist.h"
#include "graphicstextureloader.h"
#include "imgloader/imgloader.h"
#include "diskcache.h"

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

void texturemanager_LockForTextureAccess() {
    mutex_Lock(textureReqListMutex);
}

void texturemanager_ReleaseFromTextureAccess() {
    mutex_Release(textureReqListMutex);
}

int texturemanager_SaveGPUMemory(void) {
    return 1;
}

// This returns a scaled texture slot which is to be used
// preferrably right now.
// The use is decided based on the graphicstexturemanaged's
// lastUsed time stamps.
// If you want to save memory and get stuff out, specify savememory = 1.
// If you anxiously want more memory, specify savememory = 2.
int texturemanager_DecideOnPrefferedSize(struct graphicstexturemanaged* gtm,
time_t now, int savememory) {
    return 0;
    int wantsize = 0;  // 0 = full, 1 = tiny, 2 = low, 3 = medium, 4 = high
    // downscaling based on distance:
    if (gtm->lastUsage[USING_AT_VISIBILITY_DETAIL] + SCALEDOWNSECONDS
    < now || savememory == 2) {
        // it hasn't been in detail closeup for a few seconds,
        // go down to high scaling:
        wantsize = 4;  // high
        if (gtm->lastUsage[USING_AT_VISIBILITY_NORMAL] + SCALEDOWNSECONDS
        < now ||
            (savememory &&  // previous detail use is long ago:
            gtm->lastUsage[USING_AT_VISIBILITY_DETAIL] +
            SCALEDOWNSECONDSLONG < now)) {
            // it hasn't been used at normal distance for a few seconds
            // (or detail distance for quite a while and we want memory),
            // go down to medium scaling:
            wantsize = 3;  // medium
            if (gtm->lastUsage[USING_AT_VISIBILITY_NORMAL] +
            SCALEDOWNSECONDSLONG < now &&
            gtm->lastUsage[USING_AT_VISIBILITY_DETAIL]
            < SCALEDOWNSECONDSLONG &&
                // require memory saving plans or
                // recent invisible use:
                (savememory ||
                (gtm->lastUsage[USING_AT_VISIBILITY_INVISIBLE] +
                SCALEDOWNSECONDSLONG > now &&
                gtm->lastUsage[USING_AT_VISIBILITY_INVISIBLE] >
                gtm->lastUsage[USING_AT_VISIBILITY_NORMAL]
                )
            )) {
                // both normal and distant use are quite a while in the past,
                // go down to low scaling:
                wantsize = 2;  // low
            }
        }
    }
    // going down to tiny scaling in some rare cases:
    if (
    // invisible and not recently close up:
    (
        gtm->lastUsage[USING_AT_VISIBILITY_INVISIBLE] +
        SCALEDOWNSECONDS > now &&
        gtm->lastUsage[USING_AT_VISIBILITY_DETAIL] +
        SCALEDOWNSECONDSLONG < now &&
        gtm->lastUsage[USING_AT_VISIBILITY_NORMAL] +
        SCALEDOWNSECONDSLONG < now &&
        gtm->lastUsage[USING_AT_VISIBILITY_DISTANT] <
        gtm->lastUsage[USING_AT_VISIBILITY_INVISIBLE]
    )
    ||
    // all uses including distant use are quite in the past:
    (
        gtm->lastUsage[USING_AT_VISIBILITY_DETAIL] +
        SCALEDOWNSECONDSLONG < now &&
        gtm->lastUsage[USING_AT_VISIBILITY_NORMAL] + 
        SCALEDOWNSECONDSLONG < now &&
        (
            gtm->lastUsage[USING_AT_VISIBILITY_DISTANT] +
            SCALEDOWNSECONDSLONG < now ||
            (gtm->lastUsage[USING_AT_VISIBILITY_DISTANT] +
            SCALEDOWNSECONDS && savememory)
        )
    )
    ) {
        wantsize = 1;  // tiny
        if (savememory == 2) {
            // we anxiously want memory.
            // suggest unloading:
            wantsize = -1;
        }
    }
    return wantsize;
}

// this runs on application start:
__attribute__((constructor)) static void texturemanager_Init(void) {
    textureReqListMutex = mutex_Create();
}

// return just any GPU texture:
struct graphicstexture* texturemanager_GetRandomGPUTexture(
struct graphicstexturemanaged* gtm) {
    int i = 0;
    while (i < gtm->scalelistcount) {
        if (!gtm->scalelist[i].locked) {
            if (gtm->scalelist[i].gt) {
                // this one shall do for now.
                return gtm->scalelist[i].gt;
            }
        }
        i++;
    }
    return NULL;
}

struct scaleonretrievalinfo {
    struct graphicstexturescaled* scaletarget;
    struct graphicstexturescaled* obtainedscale;
};

static void texturemanager_ScaleTextureThread(void* userdata) {
    struct scaleonretrievalinfo* info = userdata;

    // let's do some scaling!
    texturemanager_LockForTextureAccess();
    if (info->obtainedscale->pixels) {
        // allocate new pixel data:
        info->scaletarget->pixels =
        malloc(info->scaletarget->width * info->scaletarget->height * 4);
        if (info->scaletarget->pixels) {
            // scale it:
            img_Scale(4, info->obtainedscale->pixels,
            info->obtainedscale->width,
            info->obtainedscale->height, (char**)&(info->scaletarget->pixels),
            info->scaletarget->width, info->scaletarget->height);
        } else {
            // free so we don't have uninitialised data as texture:
            free(info->scaletarget->pixels);
            info->scaletarget->pixels = NULL;
        }
    }
    // unlock both textures:
    info->scaletarget->locked = 0;
    info->obtainedscale->locked = 0;
    texturemanager_ReleaseFromTextureAccess();
    // done!
    free(info);  // free scale info
}

static void texturemanager_ScaleOnRetrieval(void* data,
size_t datalength, void* userdata) {
    // stuff came out of the disk cache. scale it!
    struct scaleonretrievalinfo* info = userdata;
    info->obtainedscale->pixels = data;
    texturemanager_ScaleTextureThread(info);
}

static void texturemanager_DiskCacheRetrieval(void* data,
size_t datalength, void* userdata) {
    struct graphicstexturescaled* s = userdata;
    texturemanager_LockForTextureAccess();
    if (data) {
        s->pixels = data;
    }
    s->locked = 0;
    texturemanager_ReleaseFromTextureAccess();
}

// obtain best gpu texture available right now.
// might be NULL if none is in gpu memory.
// texture manager texture access needs to be locked!
// (textureReqListMutex)
static struct graphicstexture* texturemanager_ObtainBestGpuTexture(
struct graphicstexturemanaged* gtm, size_t width, size_t height) {
    if (gtm->beingInitiallyLoaded) {
        // texture is being loaded, don't touch.
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN/GPU request] texture still being loaded: %s",
        gtm->path);
#endif
        return NULL;
    }

    int i = texturemanager_DecideOnPrefferedSize(gtm,
    time(NULL), texturemanager_SaveGPUMemory());

    if (i < 0 || i >= gtm->scalelistcount) {
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN/GPU request] preferred size %d invalid for %s",
        i, gtm->path);
#endif
        // it suggests an invisible or nonexistant texture.
        // that is odd.
        return texturemanager_GetRandomGPUTexture(gtm);
    }

    if (gtm->scalelist[i].locked) {
        // we cannot currently use this entry.
        // check if we can find another one that is not locked:
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN/GPU request] texture locked: %s", gtm->path);
#endif
        return texturemanager_GetRandomGPUTexture(gtm);
    } else {
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN] obtaining size %d of %s", i, gtm->path);
#endif
        if (gtm->scalelist[i].gt) {
            return gtm->scalelist[i].gt;
        } else {
            if (gtm->scalelist[i].pixels) {
                // we need to upload this to the gpu.
                // let's do it right now:
                if (!thread_IsMainThread()) {
                    // impossible from this thread.
                    return texturemanager_GetRandomGPUTexture(gtm);
                }
                gtm->scalelist[i].gt = graphicstexture_Create(
                gtm->scalelist[i].pixels, gtm->scalelist[i].width,
                gtm->scalelist[i].height, PIXELFORMAT_32RGBA);
                if (!gtm->scalelist[i].gt) {
                    // creaton failed, return random other size:
                    return texturemanager_GetRandomGPUTexture(gtm);
                }
                return gtm->scalelist[i].gt;
            } else {
                if (gtm->scalelist[i].diskcachepath) {
                    // we need to retrieve this from the disk cache:
                    gtm->scalelist[i].locked = 1;
                    diskcache_Retrieve(gtm->scalelist[i].diskcachepath,
                    texturemanager_DiskCacheRetrieval,
                    (void*)&gtm->scalelist[i]);
                    // for now, return a random size:
                    return texturemanager_GetRandomGPUTexture(gtm);
                } else {
                    // see if we got the texture in original size:
                    if (gtm->origscale >= 0 &&
                    !gtm->scalelist[gtm->origscale].locked) {
                        if (gtm->scalelist[gtm->origscale].pixels) {
                            // it is right there! we only need to scale!
                            // so... let's do this.. LEEROOOOOY - okok
                            struct scaleonretrievalinfo* scaleinfo =
                            malloc(sizeof(*scaleinfo));
                            if (!scaleinfo) {
                                // yow. that sucks.
                                return texturemanager_GetRandomGPUTexture(
                                gtm);
                            }
                            // prepare scale info:
                            scaleinfo->scaletarget = &gtm->scalelist[i];
                            scaleinfo->obtainedscale =
                            &gtm->scalelist[gtm->origscale];
                            // lock textures and prepare thread:
                            scaleinfo->scaletarget->locked = 1;
                            scaleinfo->obtainedscale->locked = 1;
                            threadinfo* t = thread_CreateInfo();
                            if (!t) {
                                // unlock stuff and cancel:
                                scaleinfo->scaletarget->locked = 0;
                                scaleinfo->obtainedscale->locked = 0;
                                free(scaleinfo);
                                return texturemanager_GetRandomGPUTexture(
                                gtm);
                            }
                            thread_Spawn(t,
                            &texturemanager_ScaleTextureThread,
                            scaleinfo);
                            thread_FreeInfo(t);
                        } else if (gtm->scalelist[gtm->origscale].
                        diskcachepath) {
                            // urghs, we need to get it back into memory
                            // first.
                            struct scaleonretrievalinfo* scaleinfo =
                            malloc(sizeof(*scaleinfo));
                            if (!scaleinfo) {
                                // whoops. well, just return anything:
                                return texturemanager_GetRandomGPUTexture(
                                gtm);
                            }
                            // prepare scale info:
                            scaleinfo->scaletarget = &gtm->scalelist[i];
                            scaleinfo->obtainedscale =
                            &gtm->scalelist[gtm->origscale];
                            // lock scale target aswell:
                            scaleinfo->scaletarget->locked = 1;
                            // get original texture from disk cache:
                            gtm->scalelist[gtm->origscale].locked = 1;
                            diskcache_Retrieve(gtm->scalelist[gtm->origscale].
                            diskcachepath, texturemanager_ScaleOnRetrieval,
                            scaleinfo);
                            // for now, return random texture:
                            return texturemanager_GetRandomGPUTexture(gtm);
                        } else {
                            // nothing we could sensefully do here.
                            return texturemanager_GetRandomGPUTexture(gtm);
                        }
                    }
                }
            }
        }
    }
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
        if (gtm->scalelistcount > 0 && gtm->origscale >= 0 &&
        gtm->origscale < gtm->scalelistcount) {
            if (gtm->scalelist[gtm->origscale].diskcachepath
            || gtm->scalelist[gtm->origscale].pixels
            || gtm->scalelist[gtm->origscale].gt) {
                donotload = 1;
            }
        }
        if (!donotload) {
#ifdef DEBUGTEXTUREMANAGER
            printinfo("[TEXMAN] initial texture fetch for %s", gtm->path);
#endif
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
    if (!gt) {
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN] No GPU texture found for %s", gtm->path);
#endif
    }

    if (gt != request->handedTexture && gt) {
        // remember we handed this out:
        successfullyhandled = 1;
        request->handedTexture = gt;

        // hand out texture:
        request->textureDimensionInfoCallback(request,
        gtm->width, gtm->height, request->userdata);
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
            unhandledRequestList = request->unhandledNext;
        }
        request->unhandledNext = unhandledRequestList;
        if (request->unhandledNext) {
            request->unhandledNext->unhandledPrev = request->unhandledPrev;
        }
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
#ifdef DEBUGTEXTUREMANAGER
    printinfo("[TEXMAN] dimensions reported for %s", gtm->path);
#endif
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
    gtm->beingInitiallyLoaded = 0;
    if (!success) {
        gtm->failedToLoad = 1;
        request->textureDimensionInfoCallback(request,
        0, 0, request->userdata);
        mutex_Release(textureReqListMutex);
        return;
    }
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


