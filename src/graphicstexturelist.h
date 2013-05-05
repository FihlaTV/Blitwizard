
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

#ifndef BLITWIZARD_GRAPHICSTEXTURELIST_H_
#define BLITWIZARD_GRAPHICSTEXTURELIST_H_

#include "os.h"

// This file manages a linear texture list,
// and a hash map with texture file name -> texture list entry lookup.
//
// For loading a texture by path, you will want to do a hash map
// search by path using graphicstexturelist_GetTextureByName.
// 
// In case it isn't found, create it on the linear list using
// graphicstexturelist_AddTextureToList, then add it to the hash map
// using graphicstexturelist_AddTextureToHashmap.
//
// To iterate through all textures, use graphicstexturelist_DoForAllTextures.
//
// To delete all textures completely, use graphicstexturelist_FreeAllTextures.
// It calls graphicstexturelist_Destroy on all textures.

#ifdef USE_GRAPHICS

#include "graphics.h"
#include "graphicstexturemanager.h"

// This contains the cache info for one specific size of a texture:
struct graphicstexturemanaged;
struct graphicstexturescaled {
    int locked; // if this is 1, do not access any other fields except width
      // and height! the entry is currently processed from another thread
      // e.g. for scaling, disk caching or other
    int writelock;  // if this is 1, you may read the pixel data and use
      // everything read-only, but you must not change anything or obtain
      // a full lock (locked)
    struct graphicstexture* gt;  // NULL if not loaded or not in GPU memory
    void* pixels; // not NULL if texture is in regular memory
    char* diskcachepath;  // path to raw disk cache file or NULL
    size_t width, height;  // width/height of this particular scaled entry
    struct graphicstexturemanaged* parent;
};

// A managed texture entry containing all the different sized cached versions:
struct graphicstexturemanaged {
    char* path;  // original texture path this represents
    struct graphicstexturescaled* scalelist;  // one dimensional array
    int scalelistcount;  // count of scalelist array items
    int origscale;  // array index of scalelist of item scaled in original size
    int beingInitiallyLoaded;  // the texture is just being initiially loaded
        // from disk (= wait until loading is complete)
    int failedToLoad;  // the texture failed to load (e.g. file not found)
    int handedOutLast;  // the scaled index of the last handed out size

    // usage time stamps:
    time_t lastUsage[USING_AT_COUNT];

    // initialise to zeros and then don't touch:
    struct graphicstexturemanaged* next;
    struct graphicstexturemanaged* hashbucketnext;

    // original texture size if known (otherwise zero):
    size_t width,height;
};

// Find a texture by doing a hash map lookup:
struct graphicstexturemanaged* graphicstexturelist_GetTextureByName
(const char* name);

// Add a texture to the hash map:
void graphicstexturelist_AddTextureToHashmap(
struct graphicstexturemanaged* gt);

// Remove a texture from the hash map:
void graphicstexturelist_RemoveTextureFromHashmap(
struct graphicstexturemanaged* gt);

// get all sizes of a specific graphics texture out of GPU memory:
void graphicstexturelist_TransferTextureFromHW(
struct graphicstexturemanaged* gt);

// invalidate a texture's scaled versions in GPU memory:
void graphicstexturelist_InvalidateTextureInHW(
struct graphicstexturemanaged* gt);

// get all textures out of GPU memory:
void graphicstexturelist_TransferTexturesFromHW(void);

// invalidate all textures' copies in GPU memory:
void graphicstexturelist_InvalidateHWTextures(void);

// get previous texture in texture list:
struct graphicstexturemanaged* graphicstexturelist_GetPreviousTexture(
struct graphicstexturemanaged* gt);

// add a new empty graphics texture with the given file path
// set to the list and return it:
struct graphicstexturemanaged* graphicstexturelist_AddTextureToList(
const char* path);

// Remove a texture from the linear list (it might still be in the hash map):
// You need to specify the previous list entry, which you can
// obtain through graphicstexturelist_GetPreviousTexture.
void graphicstexturelist_RemoveTextureFromList(
struct graphicstexturemanaged* gt, struct graphicstexturemanaged* prev);

// Destroy a texture. It will be removed from the linear list and the
// hash map, then all its disk cache entries of the graphicstexturescaled
// list will be diskcache_Delete()'d, the textures will be
// graphicstexture_Destroy()'d, and the char* struct members will be free'd.
void graphicstexturelist_DestroyTexture(struct graphicstexturemanaged* gt);

// Calls graphicstexturelist_DestroyTexture on all textures.
void graphicstexturelist_FreeAllTextures(void);

// Do something with all textures:
void graphicstexturelist_doForAllTextures(
int (*callback)(struct graphicstexturemanaged* texture,
struct graphicstexturemanaged* previoustexture, void* userdata),
void* userdata);

#endif  // USE_GRAPHICS

#endif  // BLITWIZARD_GRAPHICSTEXTURELIST_H_

