
/* blitwizard game engine - source code file

  Copyright (C) 2014 Jonas Thiem

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

extern mutex* textureReqListMutex;

// list of unhandled and regularly handled texture requests:
extern struct texturerequesthandle* textureRequestList;
extern struct texturerequesthandle* unhandledRequestList;

#include "graphicstexturemanagerinternalhelpers.h"

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
    int wantsize = 0;
    time_t usageDetail = gtm->lastUsage[USING_AT_VISIBILITY_DETAIL];
    time_t usageNormal = gtm->lastUsage[USING_AT_VISIBILITY_NORMAL];
    if (usageDetail > usageNormal) {
        usageNormal = usageDetail;
    }
    time_t usageDistant = gtm->lastUsage[USING_AT_VISIBILITY_DISTANT];
    if (usageNormal > usageDistant) {
        usageDistant = usageNormal;
    }
    time_t usageInvisible = gtm->lastUsage[USING_AT_VISIBILITY_INVISIBLE];
    if (usageDetail > usageInvisible || usageNormal > usageInvisible
            || usageDistant > usageInvisible) {
        usageInvisible = 0;
    }
    // downscaling based on distance:
    if (usageDetail + SCALEDOWNSECONDS < now || savememory == 2
            || texturemanager_badTextureDimensions(gtm)
            || (usageInvisible > usageDetail && usageDetail < 5000)) {
        // it hasn't been in detail closeup for a few seconds,
        // -> go down to high scaling:
        wantsize = 4;  // high
        if (usageNormal + SCALEDOWNSECONDSLONG < now
            || (usageNormal + SCALEDOWNSECONDS < now && 
                usageInvisible > usageNormal)
            ||
            (savememory &&
            usageDetail + SCALEDOWNSECONDSLONG < now &&
            usageNormal + SCALEDOWNSECONDS < now)) {
            // it hasn't been used at normal distance for a long time
            // (or detail distance for quite a while and normal for a short
            // time and we want memory).
            // -> go down to medium scaling:
            wantsize = 3;  // medium
            if (usageNormal + SCALEDOWNSECONDSVERYLONG < now &&
            gtm->lastUsage[USING_AT_VISIBILITY_DETAIL]
            < SCALEDOWNSECONDSVERYLONG &&
                // require memory saving plans or
                // recent invisible use to go to low:
                (savememory ||
                (usageInvisible + SCALEDOWNSECONDSLONG > now &&
                usageInvisible > usageNormal
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
        usageInvisible > usageDistant &&
        usageDetail + SCALEDOWNSECONDSLONG < now &&
        usageNormal + SCALEDOWNSECONDSVERYLONG < now
    )
    ||
    // all uses including distant use are quite in the past:
    (
        usageDetail + SCALEDOWNSECONDSVERYVERYLONG < now &&
        usageNormal + SCALEDOWNSECONDSVERYVERYLONG < now &&
        (
            usageDistant + SCALEDOWNSECONDSLONG < now ||
            (usageDistant + SCALEDOWNSECONDS < now && savememory)
        )
    )
    ) {
        wantsize = 1;  // tiny
        if (savememory == 2 &&
                usageDistant +
                SCALEDOWNSECONDSVERYLONG < now) {
            // suggest unloading entirely:
            wantsize = -1;
        }
    }
    // small textures might not have the larger scaled versions:
    if (wantsize >= gtm->scalelistcount && wantsize > 0) {
        if (wantsize < 4) {
            wantsize = gtm->scalelistcount - 1;
            if (wantsize < 1) {
                wantsize = 1;
            }
        } else {
            // better default to original full size here:
            wantsize = 0;
        }
    }
    assert(savememory > 0 || wantsize >= 0);
    return wantsize;
}


#endif  // USE_GRAPHICS


