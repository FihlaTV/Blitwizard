
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

#ifndef NDEBUG
// comment those lines if you don't want debug output:
#define DEBUGTEXTUREMANAGER
#define DEBUGTEXTUREMANAGERTHREADS
#endif

#include "config.h"
#include "os.h"

#include <assert.h>
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
#include "poolAllocator.h"
#include "imgloader/imgloader.h"
#include "diskcache.h"
#include "file.h"
#include "graphicstexturerequeststruct.h"
#include "graphicstexturemanagermembudget.h"
#include "graphicstexturemanagertexturedecide.h"
#include "graphicstexturemanagerinternalhelpers.h"

// global texture manager timestamp,
// since keeping that for each frame/tick is faster
// than constantly spamming calls to time_getMilliseconds()
static uint64_t texmants = 0;

// no texture uploads (e.g. device is currently lost)
int noTextureUploads = 0;

// max gpu uploads per tick:
int maxuploads = 1;
int currentuploads = 0;

mutex* textureReqListMutex = NULL;

// list of unhandled and regularly handled texture requests:
struct texturerequesthandle* textureRequestList = NULL;
struct texturerequesthandle* unhandledRequestList = NULL;

// list of deleted requests:
struct deletedtexturerequest {
    struct texturerequesthandle *handle;
    struct deletedtexturerequest *next;
};
struct deletedtexturerequest* deletedRequestList = NULL;

// pool allocator for texture requests:
struct poolAllocator* textureReqBlockAlloc = NULL;



// get global lock for accessing any texture request or the list:
void texturemanager_lockForTextureAccess() {
    mutex_lock(textureReqListMutex);
}
// release lock:
void texturemanager_releaseFromTextureAccess() {
    mutex_release(textureReqListMutex);
}

static size_t texturemanager_getRequestCount_internal(void) {
    size_t count = 0;
    struct texturerequesthandle* req = textureRequestList;
    while (req) {
        count++;
        req = req->next;
    }
    req = unhandledRequestList;
    while (req) {
        count++;
        req = req->unhandledNext;
    }
    return count;
}

static int texturemanager_requestSafeToDelete(
        __attribute__ ((unused)) struct texturerequesthandle *request) {
    return 1;
}

// this runs on application start:
__attribute__((constructor)) static void texturemanager_init(void) {
    textureReqListMutex = mutex_create();
    textureReqBlockAlloc = poolAllocator_create(
    sizeof(struct texturerequesthandle), 1);
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
        malloc(info->scaletarget->width *
        info->scaletarget->height * 4);
#ifdef DEBUGTEXTUREMANAGER
        //printinfo("[TEXMAN] scaling %s to %u, %u",
        //info->scaletarget->parent->path,
        //info->scaletarget->width, info->scaletarget->height);
#endif

        if (info->scaletarget->pixels) {
            // scale it:
            int pitch = (info->obtainedscale->paddedWidth -
                info->obtainedscale->width);
#ifdef DEBUGTEXTUREMANAGER
            printinfo("[TEXMAN] scale pitch: %d, (%d, %d)\n", pitch,
                (int)info->obtainedscale->paddedWidth,
                (int)info->obtainedscale->width);
#endif
            img_scale(4, info->obtainedscale->pixels,
            info->obtainedscale->width,
            info->obtainedscale->height,
            pitch,
            (char**)&(info->scaletarget->pixels),
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
        //printinfo("[TEXMAN/GPU request] texture still being loaded: %s",
        //gtm->path);
#endif
        return NULL;
    }
    if (gtm->failedToLoad) {
        return NULL;
    }

    int i = texturemanager_decideOnPreferredSize(gtm,
    time(NULL), texturemanager_saveGPUMemory());
    if (i == -1) {
        // we aren't supposed to offer a texture right now.
        return NULL;
    }

    if (i < 0 || i >= gtm->scalelistcount) {
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN/GPU request] preferred size %d invalid for %s",
        i, gtm->path);
#endif
        // it suggests a nonexistant texture.
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
        printinfo("[TEXMAN/GPU request] texture locked (size %d): %s", i,
            gtm->path);
#endif
        return texturemanager_getRandomGPUTexture(gtm);
    } else {
        if (gtm->scalelist[i].gt) {
            return gtm->scalelist[i].gt;
        } else {
            if (gtm->scalelist[i].pixels) {
                if (noTextureUploads || !unittopixelsset) {
#ifdef DEBUGTEXTUREMANAGER
                    //printinfo("[TEXMAN] GPU upload of %s size %d DELAYED "
                    //"(no device)", gtm->path, i);
#endif
                    return texturemanager_getRandomGPUTexture(gtm);
                }
                // check if this texture has a trivial size:
                int trivialsize = 0;
                if (gtm->scalelist[i].width * gtm->scalelist[i].height <=
                        256 * 256) {
                    trivialsize = 1;
                }
                if (!trivialsize &&
                        (currentuploads >= maxuploads && maxuploads > 0)) {
#ifdef DEBUGTEXTUREMANAGER
                    //printinfo("[TEXMAN] GPU upload of %s size %d DELAYED "
                    //"(max uploads)", gtm->path, i);
#endif
                    return texturemanager_getRandomGPUTexture(gtm);
                }
                currentuploads++;

#ifdef DEBUGTEXTUREMANAGER
                printinfo("[TEXMAN] GPU upload texture size %d of %s", i, gtm->path);
#endif
                // we need to upload this to the gpu.
                // let's do it right now:
                if (!thread_isMainThread()) {
                    // impossible from this thread.
                    return texturemanager_getRandomGPUTexture(gtm);
                }
                gtm->scalelist[i].gt = graphicstexture_create(
                    gtm->scalelist[i].pixels, gtm->scalelist[i].paddedWidth,
                    gtm->scalelist[i].paddedHeight, gtm->scalelist[i].format,
                    texmants);
                gpuMemUse += 4 * gtm->scalelist[i].width * gtm->scalelist[i].height;
                if (!gtm->scalelist[i].gt) {
                    // creaton failed, return random other size:
                    return texturemanager_getRandomGPUTexture(gtm);
                }
                return gtm->scalelist[i].gt;
            } else {
                if (gtm->scalelist[i].diskcachepath) {
#ifdef DEBUGTEXTUREMANAGER
                    printinfo("[TEXMAN] disk cache retrieval "
                    "texture size %d of %s", i, gtm->path);
#endif
                    // we need to retrieve this from the disk cache:
                    gtm->scalelist[i].locked = 1;
                    diskcache_retrieve(gtm->scalelist[i].diskcachepath,
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
                            printinfo("[TEXMAN] downscaling texture "
                            "for size %d of %s", i, gtm->path);
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
                            threadinfo* t = thread_createInfo();
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
                            thread_spawnWithPriority(t, 0,
                            &texturemanager_scaleTextureThread,
                            scaleinfo);
                            thread_freeInfo(t);
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
                            diskcache_retrieve(gtm->scalelist[gtm->origscale].
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
        mutex_lock(textureReqListMutex);
    }

    if (request->canceled) {
        mutex_release(textureReqListMutex);
        return;
    }

    // handle a request which has been waiting for a texture.
    int successfullyhandled = 0;

    struct graphicstexturemanaged* gtm =
    request->gtm;

    if (!gtm) {
        // request has no texture.
        if (!listLocked) {
            mutex_release(textureReqListMutex);
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

    // for textures of unknown sizes, we really would love to
    // know the size:
    if (gtm->width == 0 && gtm->height == 0) {
        relevant = 1;
    }

    if (!relevant) {  // not used for ages, do not process
        if (!request->textureHandlingDoneIssued) {
            // the requester really should know we don't care
            // to process this.
            request->textureHandlingDoneCallback(
                request, request->userdata);
            request->textureHandlingDoneIssued = 1;
        }
        if (!listLocked) {
            mutex_release(textureReqListMutex);
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
        /*if (texturemanager_saveGPUMemory() == 2) {
            donotload = 1;
        }*/  // this is not a good solution since no texture
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
            graphicstextureloader_doInitialLoading(gtm,
                texturemanager_initialLoadingDimensionsCallback,
                texturemanager_initialLoadingDataCallback,
                NULL);
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
        //printinfo("[TEXMAN] No GPU texture found for %s", gtm->path);
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
        mutex_release(textureReqListMutex);
    }
}

// callback with early dimension/size info:
static void texturemanager_initialLoadingDimensionsCallback
        (struct graphicstexturemanaged* gtm, size_t width, size_t height,
        int success, __attribute__ ((unused)) void* userdata) {
    mutex_lock(textureReqListMutex);
#ifdef DEBUGTEXTUREMANAGER
    printinfo("[TEXMAN] dimensions reported for %s", gtm->path);
#endif
    if (!success) {
        gtm->failedToLoad = 1;
        gtm->failedToLoadTime = time(NULL);
        width = 0;
        height = 0;
        gtm->beingInitiallyLoaded = 0;
        gtm->initialLoadDone = 1;
    }
    struct texturerequesthandle *req = unhandledRequestList;
    while (req) {
        if (req->gtm == gtm) {
            req->textureDimensionInfoCallback(req,
                width, height, req->userdata);
        }
        req = req->next;
    }
    mutex_release(textureReqListMutex);
}

// callback when texture was loaded fully:
static void texturemanager_initialLoadingDataCallback
(struct graphicstexturemanaged* gtm, int success,
        __attribute__((unused)) void* userdata) {
    mutex_lock(textureReqListMutex);
    struct texturerequesthandle* request = userdata;
    gtm->beingInitiallyLoaded = 0;
    gtm->initialLoadDone = 1;
    if (!success) {
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN] texture loading fail: %s", gtm->path);
#endif
        gtm->failedToLoad = 1;
        gtm->failedToLoadTime = time(NULL);

        struct texturerequesthandle *req = unhandledRequestList;
        while (req) {
            if (req->gtm == gtm) {
                request->textureDimensionInfoCallback(request,
                    0, 0, request->userdata);
            }
            req = req->next;
        }

        mutex_release(textureReqListMutex);
        return;
    } else {
#ifdef DEBUGTEXTUREMANAGER
        printinfo("[TEXMAN] successful loading for: %s", gtm->path);
#endif
    }
    mutex_release(textureReqListMutex);
}

struct texturerequesthandle* texturemanager_requestTexture(
const char* path,
void (*textureDimensionInfo)(struct texturerequesthandle* request,
    size_t width, size_t height, void* userdata),
void (*textureSwitch)(struct texturerequesthandle* request,
    struct graphicstexture* texture, void* userdata),
void (*textureHandlingDone)(struct texturerequesthandle* request,
    void* userdata),
void* userdata) {
    char* canonicalPath = strdup(path);
    file_makeSlashesCrossplatform(canonicalPath);
#ifdef DEBUGTEXTUREMANAGER
    //printf("[TEXMAN] [REQUEST] new request for %s, new total "
    //"count: %llu\n", path, texturemanager_getRequestCount());
#endif
    // allocate request:
    struct texturerequesthandle* request =
    poolAllocator_alloc(textureReqBlockAlloc);
    if (!request) {
        free(canonicalPath);
        return NULL;
    }
    memset(request, 0, sizeof(*request));
    request->textureSwitchCallback = textureSwitch;
    request->textureDimensionInfoCallback = textureDimensionInfo;
    request->textureHandlingDoneCallback = textureHandlingDone;
    request->userdata = userdata;

    // locate according texture entry:
    struct graphicstexturemanaged* gtm =
    graphicstexturelist_getTextureByName(canonicalPath);
    if (!gtm) {
        // add new texture entry:
        gtm = graphicstexturelist_addTextureToList(
            canonicalPath);
        if (!gtm) {
            poolAllocator_free(textureReqBlockAlloc, request);
            free(canonicalPath);
            return NULL;
        }
        graphicstexturelist_addTextureToHashmap(
            gtm);
    }
    request->gtm = gtm;

    // from now on, lock out other texture manager code:
    mutex_lock(textureReqListMutex);

    // if it failed to load and this was long ago,
    // schedule reload:
    if (gtm->failedToLoad && gtm->failedToLoadTime + 10 < time(NULL)) {
        gtm->failedToLoad = 0;
        gtm->initialLoadDone = 1;
    }

    int isUnhandled = 1;

    // if it is definitely in a failed state right now,
    // report back failure:
    if (gtm->failedToLoad) {
        request->textureDimensionInfoCallback(
            request, 0, 0, request->userdata);
        isUnhandled = 0;
    }

#if (!defined(NDEBUG) && defined(EXTRADEBUG))
    size_t c = texturemanager_getRequestCount_internal();
#endif

    if (isUnhandled) {
        // add to unhandled texture request list:
        request->unhandledNext = unhandledRequestList;
        if (request->unhandledNext) {
            request->unhandledNext->unhandledPrev = request;
        }
        unhandledRequestList = request;
    } else {
        // add it to regular list:
        request->next = textureRequestList;
        if (request->next) {
            request->next->prev = request;
        }
        request->prev = NULL;
        textureRequestList = request; 
    }

#if (!defined(NDEBUG) && defined(EXTRADEBUG))
    assert(texturemanager_getRequestCount_internal() == c + 1);
#endif

    mutex_release(textureReqListMutex);

    free(canonicalPath);
    return request;
}

void texturemanager_destroyRequest(
struct texturerequesthandle* request) {
    if (!request) {
        return;
    }

    mutex_lock(textureReqListMutex);

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

    // mark request as cancelled:
    request->canceled = 1;

    // move into deleted list or delete instantly if safe:
    if (texturemanager_requestSafeToDelete(request)) {
        poolAllocator_free(textureReqBlockAlloc, request);
    } else {
        struct deletedtexturerequest* dtex = malloc(sizeof(*dtex));
        if (!dtex) {
            // oops, that's not good. not much we can do about it though?
        } else {
            dtex->next = deletedRequestList;
            deletedRequestList = dtex;
        }
    }

    mutex_release(textureReqListMutex);
}

static void texturemanager_processUnhandledRequests(void) {
    // we need to process all unhandled requests here.
    // we cannot just proceed a subset because then unservable ones
    // will hold off servable onces. we really need to look at all of them!
    struct texturerequesthandle* handle = unhandledRequestList;
    while (handle) {
        struct texturerequesthandle* handlenext = handle->unhandledNext;
        texturemanager_processRequest(handle, 1);
        handle = handlenext;
    }
}

static void texturemanager_moveRequestToUnhandled(
struct texturerequesthandle* request) {
#if (!defined(NDEBUG) && defined(EXTRADEBUG))
    size_t c = texturemanager_getRequestCount_internal();
#endif
    if ((request->prev || request->next
    || textureRequestList == request)) {
        assert(!request->unhandledPrev && !request->unhandledNext &&
        !(unhandledRequestList == request));
        // add it to unhandled list:
        request->unhandledNext = unhandledRequestList;
        if (request->unhandledNext) {
            request->unhandledNext->unhandledPrev = request;
        }
        request->unhandledPrev = NULL;
        unhandledRequestList = request;

        // remove from regular request list:
        if (request->prev) {
            request->prev->next = request->next;
        } else {
            textureRequestList = request->next;
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
#if (!defined(NDEBUG) && defined(EXTRADEBUG))
    assert(request->unhandledNext || request->unhandledPrev
    || unhandledRequestList == request);
    assert(texturemanager_getRequestCount_internal() == c);
#endif
}


void texturemanager_usingRequest(
struct texturerequesthandle* request, int visibility) {
    if (!request) {
        return;
    }
    request->gtm->lastUsage[visibility] = time(NULL);

    // see if the texture has been unloaded by now:
    // (in which case we want to be sure to dump this to
    // the unhandled request list)
    int available = 0;
    int i = 0;
    while (i < request->gtm->scalelistcount) {
        if (request->gtm->scalelist[i].gt) {
            available = 1;
            break;
        }
        i++;
    }

    if (!available) {
        texturemanager_moveRequestToUnhandled(request);
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
        if (!request->textureHandlingDoneIssued) {
            request->textureHandlingDoneCallback(
                request, request->userdata);
            request->textureHandlingDoneIssued = 1;
        }
    }
    if (newsize < 0) {
        // move request to unhandled request list:
        // (so it will try to re-obtain the texture if it's now
        // without any)
        texturemanager_moveRequestToUnhandled(request);
        // notify the requester that we're not going to load their texture:
        if (!request->textureHandlingDoneIssued) {
            request->textureHandlingDoneCallback(
                request, request->userdata);
            request->textureHandlingDoneIssued = 1;
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
    }
#endif
    gtm->preferredSize = newsize;

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
                    graphicstexture_destroy(gtm->scalelist[i].gt);
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

static int texturemanager_textureSafeToDelete(
        struct graphicstexturemanaged* gtm) {
    int i = 0;
    while (i < gtm->scalelistcount) {
        if (gtm->scalelist[i].locked || gtm->scalelist[i].writelock) {
            return 0;
        }
        i++;
    }
    return 1;
}

static int texturemanager_checkTextureForScaling(
        struct graphicstexturemanaged* gtm,
        struct graphicstexturemanaged* prev,
        void* userdata) {
    if (gtm->beingInitiallyLoaded) {
        return 1;
    }
    // check specific texture for usage and possible downscaling:
    int i = texturemanager_decideOnPreferredSize(gtm,
    time(NULL), texturemanager_saveGPUMemory());
    texturemanager_findAndForceAllRequestsToDifferentSize(gtm, i);
    if (texturemanager_saveGPUMemory() == 0 &&
            gtm->lastUsage[USING_AT_VISIBILITY_DETAIL] + 20 > time(NULL)) {
        assert(i == 0 || i == gtm->scalelistcount - 1);
    }
    texturemanager_unloadUnneededVersions(gtm, i);
    return 1;
}

static time_t lastAdaptCheck = 0;
static void texturemanager_adaptTextures(void) {
    if (lastAdaptCheck + ADAPTINTERVAL < time(NULL)) {
        lastAdaptCheck = time(NULL);
    } else {
        return;
    }
    // this will check all textures on their usage and
    // downscale/upscale on-the-fly.
    graphicstexturelist_doForAllTextures(texturemanager_checkTextureForScaling,
    NULL);
}

uint64_t lastUploadReset = 0;
void texturemanager_tick(void) {
    texmants = time_getMilliseconds();

    // measure when we started:
    uint64_t start = texmants;

    // lock global mutex:
    mutex_lock(textureReqListMutex);

    // reset the texture uploads to 0 every 100ms:
    if (lastUploadReset + 100 < start) {
        currentuploads = 0;
        lastUploadReset = start;
    }
    // process unhandled requests to get them
    // the currently loaded textures,
    // and start loading new ones in the first place:
    texturemanager_processUnhandledRequests();

    // process currently loaded textures for
    // being downscaled or upscaled based on usage:
    texturemanager_adaptTextures();

    // release global mutex:
    mutex_release(textureReqListMutex);

    // measure end and emit warning if this took too long:
    uint64_t end = time_getMilliseconds();
    uint64_t delta = end - start;
    if (delta > 500) {
        printwarning("[TEXMAN] huge hang (%d ms) detected. This shouldn't"
            " happen. If you can reproduce this, please notify blitwizard"
            " developers.", (int)delta);
    }
}

uint64_t texturemanager_getGpuMemoryUse() {
    mutex_lock(textureReqListMutex);
    uint64_t v = gpuMemUse;
    mutex_release(textureReqListMutex);
    return v;
}

static int needtowait = 0;
static int texturemanager_deviceLostTexListCallback(
struct graphicstexturemanaged* gtm,
struct graphicstexturemanaged* gtm2,
void* userdata) {
    if (!texturemanager_textureSafeToDelete(gtm)) {
        // coverity[missing_lock] - explanation:
        // this is called indirectly from texturemanager_deviceLost(),
        // and the call is surrounded by mutex locks over there.
        needtowait = 1;
    }
    return 1;
}

void texturemanager_deviceLost(void) {
#ifdef DEBUGTEXTUREMANAGER
    printinfo("[TEXMAN] [DEVICE] Graphics device was closed.");
#endif
    mutex_lock(textureReqListMutex);

    // if any sort of scaling process or caching process is underway,
    // sadly we will need to wait for it to finish:
    needtowait = 1;
    while (needtowait) {
        needtowait = 0;
        graphicstexturelist_doForAllTextures(
        &texturemanager_deviceLostTexListCallback, NULL);
        if (!needtowait) {
            break;
        }
        mutex_release(textureReqListMutex);
        time_sleep(500);
        mutex_lock(textureReqListMutex);
        needtowait = 1;
    }

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
    graphicstexturelist_invalidateHWTextures();

    // move all regular requests to unhandled requests:
    while (textureRequestList) {
        texturemanager_moveRequestToUnhandled(textureRequestList);
    }

    mutex_release(textureReqListMutex);
}

void texturemanager_wipeTexture(const char* tex) {
    mutex_lock(textureReqListMutex); 
    
    // find relevant texture entry:
    struct graphicstexturemanaged* gtm =
    graphicstexturelist_getTextureByName(tex);

    if (!gtm) {
        // nothing to be reloaded here.
        mutex_release(textureReqListMutex);
        return;
    }

    while (!texturemanager_textureSafeToDelete(gtm)) {
        // we need to wait. (this sucks, yes.)
        mutex_release(textureReqListMutex);
        time_sleep(500);
        mutex_lock(textureReqListMutex);
    }

    // find all affected requests and remove the texture from them:
    struct texturerequesthandle* req = textureRequestList;
    while (req) {
        if (req->gtm == gtm) {
            // steal its texture copy again:
            if (req->handedTexture) {
                req->textureSwitchCallback(req, NULL, req->userdata);
                req->handedTexture = NULL;
                req->handedTextureScaledEntry = NULL;
            }

            // move the request to unhandled so it reloads its texture:
            texturemanager_moveRequestToUnhandled(req);
        }
        req = req->next;
    }
    // same for all unhandled requests:
    req = unhandledRequestList;
    while (req) {
        if (req->gtm == gtm) {
            // steal its texture copy again:
            if (req->handedTexture) {
                req->textureSwitchCallback(req, NULL, req->userdata);
                req->handedTexture = NULL;
                req->handedTextureScaledEntry = NULL;
            }

            // move the request to unhandled so it reloads its texture:
            texturemanager_moveRequestToUnhandled(req);
        }
        req = req->next;
    }

    // wipe texture (it should be reloaded again):
    graphicstexturelist_invalidateTextureInHW(gtm);
    int i = 0;
    while (i < gtm->scalelistcount) {
        struct graphicstexturescaled* st = &(gtm->scalelist[i]);
        if (st->pixels) {
            free(st->pixels);
            st->pixels = NULL;
        }
        if (st->diskcachepath) {
            // FIXME: we probably care to delete the file?
            free(st->diskcachepath);
            st->diskcachepath = NULL;
        }
        i++;
    }

    mutex_release(textureReqListMutex);
}

void texturemanager_deviceRestored(void) {
#ifdef DEBUGTEXTUREMANAGER
    printinfo("[TEXMAN] [DEVICE] Graphics device was opened.");
#endif
    mutex_lock(textureReqListMutex);

    // re-enable GPU uploads:
    noTextureUploads = 0;

    mutex_release(textureReqListMutex);
}

int texturemanager_getTextureUsageInfo(const char* texture) {
    mutex_lock(textureReqListMutex);
    struct graphicstexturemanaged* gtm =
    graphicstexturelist_getTextureByName(texture);
    if (!gtm) {
        mutex_release(textureReqListMutex);
        return -1;
    }
    int usage = -1;
    int i = USING_AT_COUNT-1;
    while (i >= 0) {
        assert(gtm->lastUsage[i] <= time(NULL));
        if (gtm->lastUsage[i] + 1 >= time(NULL)) {
            usage = i;
            // lower is better, so continue the loop:
        }
        i--;
    }
    mutex_release(textureReqListMutex);
    return usage;
}

int texturemanager_getTextureGpuSizeInfo(const char* texture) {
    mutex_lock(textureReqListMutex);
    struct graphicstexturemanaged* gtm =
    graphicstexturelist_getTextureByName(texture);
    if (!gtm) {
        mutex_release(textureReqListMutex);
        return -1;
    }
    int loaded = -1;
    int i = 4;
    if (i >= gtm->scalelistcount) {
        i = gtm->scalelistcount-1;
    }
    while (i >= 0) {
        if (!gtm->scalelist[i].locked) {
            if (gtm->scalelist[i].gt && (i == 0 || loaded < 0)) {
                loaded = i;
            }
        }
        i--;
    }
    mutex_release(textureReqListMutex);
    return loaded;
}

int texturemanager_getTextureRamSizeInfo(const char* texture) {
    mutex_lock(textureReqListMutex);
    struct graphicstexturemanaged* gtm =
    graphicstexturelist_getTextureByName(texture);
    if (!gtm) {
        mutex_release(textureReqListMutex);
        return -1;
    }
    int loaded = -1;
    int i = 4;
    if (i >= gtm->scalelistcount) {
        i = gtm->scalelistcount-1;
    }
    while (i >= 0) {
        if (!gtm->scalelist[i].locked) {
            if (gtm->scalelist[i].pixels && (i == 0 || loaded < 0)) {
                loaded = i;
            }
        }
        i--;
    }
    mutex_release(textureReqListMutex);
    return loaded;
}


size_t texturemanager_getRequestCount(void) {
    mutex_lock(textureReqListMutex);
    size_t c = texturemanager_getRequestCount_internal();
    mutex_release(textureReqListMutex);
    return c;
}

int texturemanager_getWaitingTextureRequests(const char* texture) {
    mutex_lock(textureReqListMutex);
    struct graphicstexturemanaged* gtm =
    graphicstexturelist_getTextureByName(texture);
    if (!gtm) {
        mutex_release(textureReqListMutex);
        return 0;
    }
    int requests = 0;
    struct texturerequesthandle* req = textureRequestList;
    while (req) {
        if (req->gtm == gtm) {
            if (!req->handedTexture) {
                requests++;
            }
        }
        req = req->next;
    }
    req = unhandledRequestList;
    while (req) {
        if (req->gtm == gtm) {
            if (!req->handedTexture) {
                requests++;
            }
        }
        req = req->next;
    }
    mutex_release(textureReqListMutex);
    return requests;
}

int texturemanager_isInitialTextureLoadDone(const char* texture) {
    mutex_lock(textureReqListMutex);
    struct graphicstexturemanaged* gtm =
    graphicstexturelist_getTextureByName(texture);
    if (!gtm) {
        mutex_release(textureReqListMutex);
        return 0;
    }
    int returnvalue = 0;
    if (gtm->initialLoadDone) {
        returnvalue = 1;
    }
    mutex_release(textureReqListMutex);
    return returnvalue;
}

int texturemanager_getServedTextureRequests(const char* texture) {
    mutex_lock(textureReqListMutex);
    struct graphicstexturemanaged* gtm =
    graphicstexturelist_getTextureByName(texture);
    if (!gtm) {
        mutex_release(textureReqListMutex);
        return 0;
    }
    int requests = 0;
    struct texturerequesthandle* req = textureRequestList;
    while (req) {
        if (req->gtm == gtm) {
            if (req->handedTexture) {
                requests++;
            }
        }
        req = req->next;
    }
    req = unhandledRequestList;
    while (req) {
        if (req->gtm == gtm) {
            if (req->handedTexture) {
                requests++;
            }
        }
        req = req->next;
    }
    mutex_release(textureReqListMutex);
    return requests;
}

#endif  // USE_GRAPHICS


