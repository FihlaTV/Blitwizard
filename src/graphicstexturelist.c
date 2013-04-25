
/* blitwizard 2d engine - source code file

  Copyright (C) 2011 Jonas Thiem

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

#ifdef USE_GRAPHICS

// various standard headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef WINDOWS
#include <windows.h>
#endif
#include <stdarg.h>

#include "os.h"
#include "logging.h"
#ifdef USE_SDL_GRAPHICS
#include "SDL.h"
#include "SDL_syswm.h"
#endif
#include "graphicstexture.h"
#include "graphics.h"
#include "graphicstexturelist.h"
#include "diskcache.h"
#include "imgloader.h"
#include "timefuncs.h"
#include "hash.h"
#include "file.h"
#ifdef NOTHREADEDSDLRW
#include "main.h"
#endif

static struct graphicstexturemanaged* texlist = NULL;
hashmap* texhashmap = NULL;

void graphicstexturelist_InitializeHashmap(void) {
    if (texhashmap) {
        return;
    }
    texhashmap = hashmap_New(1024 * 1024);
}

struct graphicstexturemanaged* graphicstexturelist_AddTextureToList(
const char* path) {
    struct graphicstexturemanaged* m = malloc(sizeof(*m));
    if (!m) {
        return NULL;
    }
    memset(m, 0, sizeof(*m));
    m->path = strdup(path);
    if (!m->path) {
        free(m);
        return NULL;
    }
    m->origscale = -1;
    m->next = texlist;
    texlist = m;
    return m;
}

void graphicstexturelist_RemoveTextureFromList(
struct graphicstexturemanaged* li, struct graphicstexturemanaged* prev) {
    if (prev) {
        prev->next = li->next;
    } else {
        texlist = li->next;
    }
}

struct graphicstexturemanaged* graphicstexturelist_GetTextureByName(
const char* name) {
    graphicstexturelist_InitializeHashmap();
    uint32_t i = hashmap_GetIndex(texhashmap, name, strlen(name), 1);
    struct graphicstexturemanaged* m =
    (struct graphicstexturemanaged*)(texhashmap->items[i]);
    while (m && !(strcasecmp(m->path, name) == 0)) {
        m = m->hashbucketnext;
    }
    return m;
}

void graphicstexturelist_AddTextureToHashmap(
struct graphicstexturemanaged* m) {
    graphicstexturelist_InitializeHashmap();
    uint32_t i = hashmap_GetIndex(texhashmap, m->path, strlen(m->path), 1);
    
    m->hashbucketnext = (struct graphicstexturemanaged*)(texhashmap->items[i]);
    texhashmap->items[i] = m;
}

void graphicstexturelist_RemoveTextureFromHashmap(
struct graphicstexturemanaged* gt) {
    graphicstexturelist_InitializeHashmap();
    uint32_t i = hashmap_GetIndex(texhashmap, gt->path, strlen(gt->path), 1);
    struct graphicstexturemanaged* gt2 =
    (struct graphicstexturemanaged*)(texhashmap->items[i]);
    struct graphicstexturemanaged* gtprev = NULL;
    while (gt2) {
        if (gt2 == gt) {
            if (gtprev) {
                gtprev->next = gt->hashbucketnext;
            }else{
                texhashmap->items[i] = gt->hashbucketnext;
            }
            gt->hashbucketnext = NULL;
            return;
        }

        gtprev = gt2;
        gt2 = gt2->hashbucketnext;
    }
}

void graphicstexturelist_TransferTextureFromHW(
struct graphicstexturemanaged* gt) {
    int i = 0;
    while (i < gt->scalelistcount) {
        struct graphicstexturescaled* s = &gt->scalelist[i];
        if (!s->pixels) {  // no pixels stored in system memory
            // if we can, download texture from GPU:
            if (s->gt) {
                void* newpixels = malloc(s->width * s->height * 4);
                if (newpixels) {
                    int r = graphicstexture_PixelsFromTexture(s->gt,
                    newpixels);
                    if (r) {
                        s->pixels = newpixels;
                    } else {
                        free(newpixels);
                    }
                }
            }
        }
        if (s->gt) {
            // destroy texture from the GPU
            graphicstexture_Destroy(s->gt);
            s->gt = NULL;
        }
        i++;
    }
}

void graphicstexturelist_InvalidateTextureInHW(
struct graphicstexturemanaged* gt) {
    int i = 0;
    while (i < gt->scalelistcount) {
        struct graphicstexturescaled* s = &gt->scalelist[i];
        if (s->gt) {
            // destroy texture from the GPU
            graphicstexture_Destroy(s->gt);
            s->gt = NULL;
        }
        i++;
    }
}

void graphicstexturelist_TransferTexturesFromHW(void) {
    struct graphicstexturemanaged* gt = texlist;
    while (gt) {
        graphicstexturelist_TransferTextureFromHW(gt);
        gt = gt->next;
    }
}

void graphicstexturelist_InvalidateHWTextures(void) {
    struct graphicstexturemanaged* gt = texlist;
    while (gt) {
        graphicstexturelist_InvalidateTextureInHW(gt);
        gt = gt->next;
    }
}

void graphicstexturelist_TransferTextureToHW(
struct graphicstexturemanaged* gt) {
    int i = 0;
    while (i < gt->scalelistcount) {
        struct graphicstexturescaled* s = &gt->scalelist[i];
        if (!s->pixels) {  // no pixels stored in system memory
            // if we can, download texture from GPU:
            if (s->gt) {
                void* newpixels = malloc(s->width * s->height * 4);
                if (newpixels) {
                    int r = graphicstexture_PixelsFromTexture(s->gt,
                    newpixels);
                    if (r) {
                        s->pixels = newpixels;
                    } else {
                        free(newpixels);
                    }
                }
            }
        }
        if (s->gt) {
            // destroy texture from the GPU
            graphicstexture_Destroy(s->gt);
            s->gt = NULL;
        }
        i++;
    }
}


struct graphicstexturemanaged* graphicstexturelist_GetPreviousTexture(
struct graphicstexturemanaged* gt) {
    struct graphicstexturemanaged* gtprev = texlist;
    while (gtprev && !(gtprev->next == gt)) {
        gtprev = gtprev->next;
    }
    return gtprev;
}

void graphicstexturelist_DestroyTexture(
struct graphicstexturemanaged* gt) {
    int i = 0;
    while (i < gt->scalelistcount) {
        struct graphicstexturescaled* s = &gt->scalelist[i];
        if (s->gt) {
            // destroy texture from the GPU
            graphicstexture_Destroy(s->gt);
        }
        if (s->pixels) {
            // destroy texture in memory
            free(s->pixels);
        }
        if (s->diskcachepath) {
            // destroy texture from disk cache
            diskcache_Delete(s->diskcachepath);
            free(s->diskcachepath);
        }
        i++;
    }
    free(gt->scalelist);
    free(gt);
}

void graphicstexturelist_FreeAllTextures(void) {
    // free all textures
    struct graphicstexturemanaged* gt = texlist;
    while (gt) {
        struct graphicstexturemanaged* gtnext = gt->next;
        graphicstexturelist_DestroyTexture(gt);
        gt = gtnext;
    }
}

void graphicstexturelist_DoForAllTextures(
int (*callback)(struct graphicstexturemanaged* texture,
struct graphicstexturemanaged* previoustexture,
void* userdata), void* userdata) {
    struct graphicstexturemanaged* gt = texlist;
    struct graphicstexturemanaged* gtprev = NULL;
    while (gt) {
        struct graphicstexturemanaged* gtnext = gt->next;
        if (callback(gt, gtprev, userdata)) {
            // entry is still valid (callback return 1), remember it as prev
            gtprev = gt;
        }
        gt = gtnext;
    }
}

#endif // ifdef USE_GRAPHICS

