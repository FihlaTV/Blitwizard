
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
 
    // if the request was cancelled, the callbacks
    // are gone and must not be used anymore:
    int canceled;
};
struct texturerequesthandle* textureRequestList = NULL;

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

        mutex_Release(textureReqListMutex);

        // sleep a moment:
        time_Sleep(100);
    }
}

// obtain best gpu texture available right now.
// might be NULL if none is in gpu memory.
struct graphicstexture* texturemanager_ObtainBestGpuTexture(
struct graphicstexturemanaged* gtm, size_t width, size_t height) {
    return NULL;
}

static void texturemanager_InitialLoadingCallback
(struct graphicstexturemanaged* gtm, void* userdata) {

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

struct texturerequesthandle* texturemanager_RequestTexture(
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

    mutex_Lock(textureReqListMutex);

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

    // if texture isn't loaded yet, load it:
    if (!gtm->beinginitiallyloaded) {
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
            gtm->beinginitiallyloaded = 1;

            // fire up the threaded loading process which
            // gives us the texture in the most common sizes
            graphicstextureloader_DoInitialLoading(gtm,
            texturemanager_InitialLoadingCallback,
            request);

            // we are done for now. loading needs to finish first
            mutex_Release(textureReqListMutex);            
        }
    }

    // find a texture matching the requested size:
    size_t width = 0;
    size_t height = 0; // FIXME, have proper size parameters
    texturemanager_ObtainBestGpuTexture(gtm, width, height);

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

void texturemanager_InvalidateTextures() {

}


#endif  // USE_GRAPHICS


