
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
#define DEBUGTEXTUREMANAGERTHREADS
#endif

#include "config.h"
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
uint64_t textureSysMemoryBudgetMin = 100;
uint64_t texturesysMemoryBudgetMax = 200;

// texture gpu memory budget in megabyte:
uint64_t textureGpuMemoryBudgetMin = 50;
uint64_t textureGpuMemoryBudgetMax = 100;

// actual resource use:
uint64_t sysMemUse = 0;
uint64_t gpuMemUse = 0;

// no texture uploads (e.g. device is currently lost)
int noTextureUploads = 0;

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
    struct graphicstexturescaled* handedTextureScaledEntry;
 
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

void texturemanager_lockForTextureAccess() {
    mutex_Lock(textureReqListMutex);
}

void texturemanager_releaseFromTextureAccess() {
    mutex_Release(textureReqListMutex);
}

int texturemanager_saveGPUMemory(void) {
    return 0;
    double current = (gpuMemUse / (1024 * 1024));
    double m1 = textureGpuMemoryBudgetMin;
    double m2 = textureGpuMemoryBudgetMax;
    if (current > m1 * 0.8) {
        if (current > m1 + (m2-m1)/2) {
            // we should save memory more radically,
            // approaching the upper limit
            return 2;
        }
        return 1;
    }
    return 0;
}

int texturemanager_saveSystemMemory(void) {
    return 1;
}

// This returns a scaled texture slot (NOT the texture)
// which is to be used preferrably right now.
//
// It doesn't load any texture. It just recommends on
// which one to use.
//
// The use is decided based on the graphicstexturemanaged's
// lastUsed time stamps.
//
// If you want to save memory and get stuff out, specify savememory = 1.
// If you anxiously want more memory, specify savememory = 2.
int texturemanager_decideOnPreferredSize(struct graphicstexturemanaged* gtm,
time_t now, int savememory) {
    int wantsize = 0;  // 0 = full, 1 = tiny, 2 = low, 3 = medium, 4 = high
    // downscaling based on distance:
    if (gtm->lastUsage[USING_AT_VISIBILITY_DETAIL] + SCALEDOWNSECONDS
    < now || savememory == 2) {
        // it hasn't been in detail closeup for a few seconds,
        // go down to high scaling:
        wantsize = 4;  // high
        if (gtm->lastUsage[USING_AT_VISIBILITY_NORMAL] + SCALEDOWNSECONDSLONG
        < now ||
            (savememory &&  // previous detail use is long ago:
            gtm->lastUsage[USING_AT_VISIBILITY_DETAIL] +
            SCALEDOWNSECONDSLONG < now &&
            gtm->lastUsage[USING_AT_VISIBILITY_NORMAL] + SCALEDOWNSECONDS
            < now)) {
            // it hasn't been used at normal distance for a long time
            // (or detail distance for quite a while and normal for a short
            // time and we want memory).
            // go down to medium scaling:
            return -1;
            wantsize = 3;  // medium
            if (gtm->lastUsage[USING_AT_VISIBILITY_NORMAL] +
            SCALEDOWNSECONDSVERYLONG < now &&
            gtm->lastUsage[USING_AT_VISIBILITY_DETAIL]
            < SCALEDOWNSECONDSVERYLONG &&
                // require memory saving plans or
                // recent invisible use to go to low:
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
    // invisible and not recently close up, and very long not normal:
    (
        gtm->lastUsage[USING_AT_VISIBILITY_INVISIBLE] +
        SCALEDOWNSECONDS > now &&
        gtm->lastUsage[USING_AT_VISIBILITY_DETAIL] +
        SCALEDOWNSECONDSLONG < now &&
        gtm->lastUsage[USING_AT_VISIBILITY_NORMAL] +
        SCALEDOWNSECONDSVERYLONG < now &&
        gtm->lastUsage[USING_AT_VISIBILITY_DISTANT] <
        gtm->lastUsage[USING_AT_VISIBILITY_INVISIBLE]
    )
    ||
    // all uses including distant use are quite in the past:
    (
        gtm->lastUsage[USING_AT_VISIBILITY_DETAIL] +
        SCALEDOWNSECONDSVERYVERYLONG < now &&
        gtm->lastUsage[USING_AT_VISIBILITY_NORMAL] + 
        SCALEDOWNSECONDSVERYVERYLONG < now &&
        (
            gtm->lastUsage[USING_AT_VISIBILITY_DISTANT] +
            SCALEDOWNSECONDSLONG < now ||
            (gtm->lastUsage[USING_AT_VISIBILITY_DISTANT] +
            SCALEDOWNSECONDS < now && savememory)
        )
    )
    ) {
        wantsize = 1;  // tiny
        if (savememory == 2 &&
        gtm->lastUsage[USING_AT_VISIBILITY_DISTANT] +
        SCALEDOWNSECONDSVERYLONG < now) {
            // we anxiously want memory.
            // suggest unloading:
            wantsize = -1;
        }
    }
    // small textures might not have the larger scaled versions:
    if (wantsize >= gtm->scalelistcount && wantsize > 0) {
        if (wantsize < 4) {
            wantsize = gtm->scalelistcount-1;
        } else {
            // better default to original full size here:
            wantsize = 0;
        }
    }
    return wantsize;
}

// this runs on application start:
__attribute__((constructor)) static void texturemanager_init(void) {
    textureReqListMutex = mutex_Create();
}

// return just any GPU texture:
struct graphicstexture* texturemanager_getRandomGPUTexture(
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

static void texturemanager_scaleTextureThread(void* userdata) {
    struct scaleonretrievalinfo* info = userdata;

    // let's do some scaling!
    texturemanager_lockForTextureAccess();
    if (info->obtainedscale->pixels) {
        // allocate new pixel data:
        info->scaletarget->pixels =
        malloc(info->scaletarget->width * info->scaletarget->height * 4);
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN] scaling %s to %u, %u",
        info->scaletarget->parent->path,
        info->scaletarget->width, info->scaletarget->height);
#endif

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
    info->obtainedscale->writelock--;
    texturemanager_releaseFromTextureAccess();
    // done!
    free(info);  // free scale info
}

static void texturemanager_scaleOnRetrieval(void* data,
size_t datalength, void* userdata) {
    texturemanager_lockForTextureAccess();
    // stuff came out of the disk cache. scale it!
    struct scaleonretrievalinfo* info = userdata;
    info->obtainedscale->pixels = data;
    info->obtainedscale->locked = 0;
    info->obtainedscale->writelock++;
    texturemanager_releaseFromTextureAccess();
    texturemanager_scaleTextureThread(info);
}

static void texturemanager_diskCacheRetrieval(void* data,
size_t datalength, void* userdata) {
    // obtained scaled texture from disk cache:
    struct graphicstexturescaled* s = userdata;
    texturemanager_lockForTextureAccess();
    if (data) {
        s->pixels = data;
    }
    s->locked = 0;
    texturemanager_releaseFromTextureAccess();
}

static struct graphicstexture* texturemanager_getTextureSizeOnGPU(
struct graphicstexturemanaged* gtm, int slot);

// obtain best gpu texture available right now.
// might be NULL if none is in gpu memory.
// texture manager texture access needs to be locked!
// (textureReqListMutex)
static struct graphicstexture* texturemanager_obtainBestGpuTexture(
struct graphicstexturemanaged* gtm, size_t width, size_t height,
int* index) {
    if (gtm->beingInitiallyLoaded) {
        // texture is being loaded, don't touch.
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN/GPU request] texture still being loaded: %s",
        gtm->path);
#endif
        return NULL;
    }

    int i = texturemanager_decideOnPreferredSize(gtm,
    time(NULL), texturemanager_saveGPUMemory());

    if (i < 0 || i >= gtm->scalelistcount) {
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN/GPU request] preferred size %d invalid for %s",
        i, gtm->path);
#endif
        // it suggests an invisible or nonexistant texture.
        // that is odd.
        return texturemanager_getRandomGPUTexture(gtm);
    }

    *index = i;
    return texturemanager_getTextureSizeOnGPU(gtm, i);
}

// Request a specific texture size be on the GPU.
// Returns any GPU texture if possible, might also be another size
// (while the requested one will be loaded and available later).
static struct graphicstexture* texturemanager_getTextureSizeOnGPU(
struct graphicstexturemanaged* gtm, int slot) {
    int i = slot;
    if (gtm->scalelist[i].locked) {
        // we cannot currently use this entry.
        // check if we can find another one that is not locked:
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN/GPU request] texture locked: %s", gtm->path);
#endif
        return texturemanager_getRandomGPUTexture(gtm);
    } else {
        if (gtm->scalelist[i].gt) {
            return gtm->scalelist[i].gt;
        } else {
            if (gtm->scalelist[i].pixels) {
                if (noTextureUploads) {
#ifdef DEBUGTEXTUREMANAGER
                    printinfo("[TEXMAN] GPU upload of %s size %d DELAYED "
                    "(no device)", gtm->path, i);
#endif
                    return texturemanager_getRandomGPUTexture(gtm);
                }
#ifdef DEBUGTEXTUREMANAGER
                printinfo("[TEXMAN] GPU upload texture size %d of %s", i, gtm->path);
#endif
                // we need to upload this to the gpu.
                // let's do it right now:
                if (!thread_IsMainThread()) {
                    // impossible from this thread.
                    return texturemanager_getRandomGPUTexture(gtm);
                }
                gtm->scalelist[i].gt = graphicstexture_Create(
                gtm->scalelist[i].pixels, gtm->scalelist[i].width,
                gtm->scalelist[i].height, PIXELFORMAT_32RGBA);
                gpuMemUse += 4 * gtm->scalelist[i].width * gtm->scalelist[i].height;
                if (!gtm->scalelist[i].gt) {
                    // creaton failed, return random other size:
                    return texturemanager_getRandomGPUTexture(gtm);
                }
                return gtm->scalelist[i].gt;
            } else {
                if (gtm->scalelist[i].diskcachepath) {
#ifdef DEBUGTEXTUREMANAGER
                    printinfo("[TEXMAN] disk cache retrieval texture size %d of %s", i, gtm->path);
#endif
                    // we need to retrieve this from the disk cache:
                    gtm->scalelist[i].locked = 1;
                    diskcache_Retrieve(gtm->scalelist[i].diskcachepath,
                    texturemanager_diskCacheRetrieval,
                    (void*)&gtm->scalelist[i]);
                    // for now, return a random size:
                    return texturemanager_getRandomGPUTexture(gtm);
                } else {
                    // see if we got the texture in original size:
                    if (gtm->origscale >= 0 &&
                    !gtm->scalelist[gtm->origscale].locked) {
                        if (gtm->scalelist[gtm->origscale].pixels) {
#ifdef DEBUGTEXTUREMANAGER
                            printinfo("[TEXMAN] downscaling texture for size %d of %s", i, gtm->path);
#endif
                            // it is right there! we only need to scale!
                            // so... let's do this.. LEEROOOOOY - okok
                            struct scaleonretrievalinfo* scaleinfo =
                            malloc(sizeof(*scaleinfo));
                            if (!scaleinfo) {
                                // yow. that sucks.
                                return texturemanager_getRandomGPUTexture(
                                gtm);
                            }
                            // prepare scale info:
                            scaleinfo->scaletarget = &gtm->scalelist[i];
                            scaleinfo->obtainedscale =
                            &gtm->scalelist[gtm->origscale];
                            // lock textures and prepare thread:
                            scaleinfo->scaletarget->locked = 1;
                            scaleinfo->obtainedscale->writelock++;
                            threadinfo* t = thread_CreateInfo();
                            if (!t) {
                                // unlock stuff and cancel:
                                scaleinfo->scaletarget->locked = 0;
                                scaleinfo->obtainedscale->locked = 0;
                                free(scaleinfo);
                                return texturemanager_getRandomGPUTexture(
                                gtm);
                            }
#ifdef DEBUGTEXTUREMANAGERTHREADS
                            printinfo("[TEXMAN-THREADS] spawn");
#endif
                            thread_Spawn(t,
                            &texturemanager_scaleTextureThread,
                            scaleinfo);
                            thread_FreeInfo(t);
                            return texturemanager_getRandomGPUTexture(gtm);
                        } else if (gtm->scalelist[gtm->origscale].
                        diskcachepath) {
                            // urghs, we need to get the original size
                            // back into memory first.
                            if (gtm->scalelist[gtm->origscale].writelock > 0) {
                                // it is locked from write access,
                                // so we can't.
                                return texturemanager_getRandomGPUTexture(gtm); 
                            }
#ifdef DEBUGTEXTUREMANAGER
                            printinfo("[TEXMAN] disk cache retrieval of "
                            "ORIGINAL for size %d of %s",
                            i, gtm->path);
#endif
                            // urghs, we need to get it back into memory
                            // first.
                            struct scaleonretrievalinfo* scaleinfo =
                            malloc(sizeof(*scaleinfo));
                            if (!scaleinfo) {
                                // whoops. well, just return anything:
                                return texturemanager_getRandomGPUTexture(
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
                            diskcachepath, texturemanager_scaleOnRetrieval,
                            scaleinfo);
                            // for now, return random texture:
                            return texturemanager_getRandomGPUTexture(gtm);
                        } else {
                            // nothing we could sensefully do here.
                            return texturemanager_getRandomGPUTexture(gtm);
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

static void texturemanager_initialLoadingDimensionsCallback
(struct graphicstexturemanaged* gtm, size_t width, size_t height,
int success, void* userdata);

static void texturemanager_initialLoadingDataCallback
(struct graphicstexturemanaged* gtm, int success, void* userdata);

static void texturemanager_processRequest(struct texturerequesthandle*
request, int listLocked) {
    if (!listLocked) {
        mutex_Lock(textureReqListMutex);
    }

    // handle a request which has been waiting for a texture.
    int successfullyhandled = 0;

    struct graphicstexturemanaged* gtm =
    request->gtm;

    if (!gtm) {
        // request has no texture.
        if (!listLocked) {
            mutex_Release(textureReqListMutex);
        }
        return;
    }

    // if request has been used ages ago, do not care:
    int relevant = 0;
    int j = 0;
    while (j < USING_AT_COUNT) {
        if (gtm->lastUsage[j] +
        SCALEDOWNSECONDSVERYVERYLONG/2 > time(NULL)) {
            relevant = 1;
        }
        j++;
    }
    if (!relevant) {  // not used for ages, do not process
        if (!listLocked) {
            mutex_Release(textureReqListMutex);
        }
        return;
    }

    // if texture isn't loaded yet, load it:
    if (!gtm->beingInitiallyLoaded && !gtm->failedToLoad) {
        int donotload = 0;
        // check if texture is available in original size:
        if (gtm->scalelistcount > 0 && gtm->origscale >= 0 &&
        gtm->origscale < gtm->scalelistcount) {
            if (gtm->scalelist[gtm->origscale].locked == 0) {
                if (gtm->scalelist[gtm->origscale].diskcachepath
                || gtm->scalelist[gtm->origscale].pixels
                || gtm->scalelist[gtm->origscale].gt) {
                    donotload = 1;
                }
            }
        }
        if (texturemanager_saveGPUMemory() == 2) {
            donotload = 1;
        }
        if (!donotload) {
#ifdef DEBUGTEXTUREMANAGER
            printinfo("[TEXMAN] initial texture fetch for %s", gtm->path);
#endif
#ifdef DEBUGTEXTUREMANAGERTHREADS
            printinfo("[TEXMAN-THREADS] spawn");
#endif
            gtm->beingInitiallyLoaded = 1;

            // fire up the threaded loading process which
            // gives us the texture in the most common sizes
            graphicstextureloader_DoInitialLoading(gtm,
            texturemanager_initialLoadingDimensionsCallback,
            texturemanager_initialLoadingDataCallback,
            request);
        }
    }

    // find a texture matching the requested size:
    size_t width = 0;
    size_t height = 0; // FIXME, have proper size parameters
    int index = -1;
    struct graphicstexture* gt = texturemanager_obtainBestGpuTexture(
    gtm, width, height, &index);
    if (!gt) {
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN] No GPU texture found for %s", gtm->path);
#endif
    }

    if (gt != request->handedTexture && gt) {
        // forget about old texture again:
        if (request->handedTextureScaledEntry) {
            request->handedTextureScaledEntry->refcount--;
        }
        // remember we handed this out:
        successfullyhandled = 1;
        request->handedTexture = gt;
        request->handedTextureScaledEntry = &gtm->scalelist[index];
        request->handedTextureScaledEntry->refcount++;

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
            if (unhandledRequestList) {
                unhandledRequestList->unhandledPrev = NULL;
            }
        }
        if (request->unhandledNext) {
            request->unhandledNext->unhandledPrev = request->unhandledPrev;
        } 
        request->unhandledNext = NULL;
        request->unhandledPrev = NULL;
    }
    if (!listLocked) {
        mutex_Release(textureReqListMutex);
    }
}

// callback with early dimension/size info:
static void texturemanager_initialLoadingDimensionsCallback
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
static void texturemanager_initialLoadingDataCallback
(struct graphicstexturemanaged* gtm, int success, void* userdata) {
    mutex_Lock(textureReqListMutex);
    struct texturerequesthandle* request = userdata;
    gtm->beingInitiallyLoaded = 0;
    if (!success) {
#ifdef DEBUGTEXTUREMANAGER
    printinfo("[TEXMAN] texture loading fail: %s", gtm->path);
#endif
        gtm->failedToLoad = 1;
        request->textureDimensionInfoCallback(request,
        0, 0, request->userdata);
        mutex_Release(textureReqListMutex);
        return;
    } else {
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN] successful loading for: %s", gtm->path);
#endif
    }
    mutex_Release(textureReqListMutex);
}

struct texturerequesthandle* texturemanager_requestTexture(
const char* path,
void (*textureDimensionInfo)(struct texturerequesthandle* request,
size_t width, size_t height, void* userdata),
void (*textureSwitch)(struct texturerequesthandle* request,
struct graphicstexture* texture, void* userdata),
void* userdata) {

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

    // assume initial usage:
    request->gtm->lastUsage[USING_AT_VISIBILITY_NORMAL] =
    time(NULL);

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

void texturemanager_usingRequest(
struct texturerequesthandle* request, int visibility) {
    if (!request->gtm) {
        return;
    }
    mutex_Lock(textureReqListMutex);
    request->gtm->lastUsage[visibility] = time(NULL);
    mutex_Release(textureReqListMutex); 
}

void texturemanager_destroyRequest(
struct texturerequesthandle* request) {
    if (!request) {
        return;
    }

    mutex_Lock(textureReqListMutex);

    // remove request from regular list
    if (request->prev || request->next || request == textureRequestList) {
        if (request->prev) {
            request->prev->next = request->next;
        } else {
            textureRequestList = request->next;
        }
        if (request->next) {
            request->next->prev = request->prev;
        }
    }

    // remove from unhandled request list:
    if (request->unhandledPrev || request->unhandledNext ||
    unhandledRequestList == request) {
        if (request->unhandledPrev) {
            request->unhandledPrev->unhandledNext = request->unhandledNext;
        } else {
            unhandledRequestList = request->unhandledNext;
        }
        if (request->unhandledNext) {
            request->unhandledNext->unhandledPrev = request->unhandledPrev;
        }
    }

    // free request:
    free(request);

    mutex_Release(textureReqListMutex);
}

static void texturemanager_processUnhandledRequests(void) {
    struct texturerequesthandle* handle = unhandledRequestList;
    while (handle) {
        struct texturerequesthandle* handlenext = handle->unhandledNext;
        texturemanager_processRequest(handle, 1);
        handle = handlenext;
    }
}

static void texturemanager_forceRequestToDifferentSize(
struct texturerequesthandle* request, struct graphicstexturemanaged* gtm,
int newsize) {
    if (request->canceled || request->gtm != gtm) {
        return;
    }
    if (request->handedTexture) {
        if (newsize < 0) {
            // we want the request to drop using the texture:
            if (request->handedTextureScaledEntry) {
                request->handedTextureScaledEntry->refcount--;
            }
            request->textureSwitchCallback(request, NULL, request->userdata);
            request->handedTexture = NULL;
            request->handedTextureScaledEntry = NULL;
        } else if (request->handedTexture != gtm->scalelist[newsize].gt) {
            // this request needs to be set to the new size:
            if (request->handedTextureScaledEntry) {
                request->handedTextureScaledEntry->refcount--;
            }
            request->textureSwitchCallback(request,
            gtm->scalelist[newsize].gt, request->userdata);
            request->handedTexture = gtm->scalelist[newsize].gt;
            request->handedTextureScaledEntry = &gtm->scalelist[newsize];
            request->handedTextureScaledEntry->refcount++;
        }
    }
    if (newsize < 0) {
        // move request to unhandled request list:
        // (so it will try to re-obtain the texture if it's now
        // without any)
        if ((request->prev || request->next
        || textureRequestList == request)) {
            if (!request->unhandledPrev && !request->unhandledNext &&
            !(unhandledRequestList == request)) {
                // add it to unhandled list:
                request->unhandledNext = unhandledRequestList;
                if (request->unhandledNext) {
                    request->unhandledNext->unhandledPrev = request;
                }
                request->unhandledPrev = NULL;
                unhandledRequestList = request;
            }
            // remove from regular request list:
            if (request->prev) {
                request->prev->next = request->next;
            } else {
                textureRequestList = request->unhandledNext;
                if (textureRequestList) {
                    textureRequestList->prev = NULL;
                }
            }
            if (request->next) {
                request->next->prev = request->prev;
            }
            request->next = NULL;
            request->prev = NULL;
        }
    }
}

static void texturemanager_findAndForceAllRequestsToDifferentSize(
struct graphicstexturemanaged* gtm, int newsize) {
    // a texture should be downscaled/upscaled.
    // find all using requests and force them to use the new version.

    // check a few obvious error conditions:
    if (!gtm->scalelist) {
        // texture not loaded yet. there is nothing to do here.
        return;
    }
#ifdef DEBUGTEXTUREMANAGER
    if (gtm->preferredSize != newsize) {
        printinfo("[TEXMAN] new size for %s: %d", gtm->path, newsize);
        gtm->preferredSize = newsize;
    }
#endif
    if (newsize >= 0) {
        if (gtm->scalelist[newsize].locked) {
            // texture is locked, cannot use.
            return;
        }
        if (!gtm->scalelist[newsize].gt) {
            // this texture size is not available. load it:
            texturemanager_getTextureSizeOnGPU(gtm, newsize);
            return;
        }
    }

    // force all requests to the new size:
    struct texturerequesthandle* handle = unhandledRequestList;
    while (handle) {
        texturemanager_forceRequestToDifferentSize(handle, gtm, newsize);
        handle = handle->unhandledNext;
    }
    handle = textureRequestList;
    while (handle) {
        texturemanager_forceRequestToDifferentSize(handle, gtm, newsize);
        handle = handle->next;
    }
}

static void texturemanager_unloadUnneededVersions(
struct graphicstexturemanaged* gtm, int neededversion) {
    int i = 0;
    while (i < gtm->scalelistcount) {
        if (!gtm->scalelist[i].locked && !gtm->scalelist[i].writelock) {
            if (gtm->scalelist[i].refcount <= 0 && i != neededversion) {
                // unload texture from GPU
                if (gtm->scalelist[i].gt) {
                    graphicstexture_Destroy(gtm->scalelist[i].gt);
                    gtm->scalelist[i].gt = NULL;
                    gpuMemUse -= 4 * gtm->scalelist[i].width *
                    gtm->scalelist[i].height;
#ifdef DEBUGTEXTUREMANAGER
                    printinfo("[TEXMAN] Unloading %s size %d from GPU",
                    gtm->path, i);
#endif
                }
                
            }
        }
        i++;
    }     
}

static int texturemanager_checkTextureForScaling(
struct graphicstexturemanaged* gtm, struct graphicstexturemanaged* prev,
void* userdata) {
    // check specific texture for usage and possible downscaling:
    int i = texturemanager_decideOnPreferredSize(gtm,
    time(NULL), texturemanager_saveGPUMemory());
    texturemanager_findAndForceAllRequestsToDifferentSize(gtm, i);
    texturemanager_unloadUnneededVersions(gtm, i);
    return 1;
}

static time_t lastAdoptCheck = 0;
static void texturemanager_adoptTextures(void) {
    if (lastAdoptCheck + ADOPTINTERVAL < time(NULL)) {
        lastAdoptCheck = time(NULL);
    } else {
        return;
    }
    // this will check all textures on their usage and
    // downscale/upscale on-the-fly.
    graphicstexturelist_doForAllTextures(texturemanager_checkTextureForScaling,
    NULL);
}

void texturemanager_tick(void) {
    mutex_Lock(textureReqListMutex);
    texturemanager_processUnhandledRequests();
    texturemanager_adoptTextures();
    mutex_Release(textureReqListMutex);
}

uint64_t texturemanager_getGpuMemoryUse() {
    mutex_Lock(textureReqListMutex);
    uint64_t v = gpuMemUse;
    mutex_Release(textureReqListMutex);
    return v;
}

void texturemanager_deviceLost(void) {
#ifdef DEBUGTEXTUREMANAGER
    printinfo("[TEXMAN] [DEVICE] Graphics device was closed.");
#endif
    mutex_Lock(textureReqListMutex);

    // forbid further GPU uploads:
    noTextureUploads = 1;

    // now, strip all requests of their textures:

    // regular requests:
    struct texturerequesthandle* req = textureRequestList; 
    while (req) {
        if (req->handedTexture) {
            req->textureSwitchCallback(req, NULL, req->userdata);
            req->handedTexture = NULL;
            req->handedTextureScaledEntry = NULL;
        }
        req = req->next;
    }

    // unhandled requests:
    req = unhandledRequestList;
    while (req) {
        if (req->handedTexture) {
            req->textureSwitchCallback(req, NULL, req->userdata);
            req->handedTexture = NULL;
            req->handedTextureScaledEntry = NULL;
        }
        req = req->unhandledNext;
    }

    // now throw all GPU textures away:
    graphicstexturelist_InvalidateHWTextures();

    mutex_Release(textureReqListMutex);
}

void texturemanager_deviceRestored(void) {
#ifdef DEBUGTEXTUREMANAGER
    printinfo("[TEXMAN] [DEVICE] Graphics device was opened.");
#endif
    mutex_Lock(textureReqListMutex);

    // re-enable GPU uploads:
    noTextureUploads = 0;

    mutex_Release(textureReqListMutex);
}

int texturemanager_getTextureUsageInfo(const char* texture) {
    mutex_Lock(textureReqListMutex);
    struct graphicstexturemanaged* gtm =
    graphicstexturelist_GetTextureByName(texture);
    if (!gtm) {
        mutex_Release(textureReqListMutex);
        return -1;
    }
    int usage = -1;
    int i = USING_AT_COUNT-1;
    while (i >= 0) {
        if (gtm->lastUsage[i] + 5 >= time(NULL)) {
            usage = i;
            // lower is better, so continue the loop:
        }
        i--;
    }
    mutex_Release(textureReqListMutex);
    return usage;
}

int texturemanager_getTextureGpuSizeInfo(const char* texture) {
    mutex_Lock(textureReqListMutex);
    struct graphicstexturemanaged* gtm =
    graphicstexturelist_GetTextureByName(texture);
    if (!gtm) {
        mutex_Release(textureReqListMutex);
        return -1;
    }
    int loaded = -1;
    int i = 4;
    if (i >= gtm->scalelistcount) {
        i = gtm->scalelistcount-1;
    }
    while (i >= 0) {
        if (!gtm->scalelist[i].locked) {
            if (gtm->scalelist[i].gt) {
                loaded = i;
                // higher is better, so we're done:
                break;
            }
        }
        i--;
    }
    mutex_Release(textureReqListMutex);
    return loaded;
}

size_t texturemanager_getRequestCount() {
    size_t count = 0;
    mutex_Lock(textureReqListMutex);
    struct texturerequesthandle* req = textureRequestList;
    while (req) {
        count++;
        req = req->next;
    }
    req = unhandledRequestList;
    while (req) {
        count++;
        req = req->next;
    }
    mutex_Release(textureReqListMutex);
    return count;
}

#endif  // USE_GRAPHICS


