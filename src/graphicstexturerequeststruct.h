
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

#ifndef BLITWIZARD_GRAPHISTEXTUREREQUESTSTRUCT_H_
#define BLITWIZARD_GRAPHISTEXTUREREQUESTSTRUCT_H_

#include "graphicstexturelist.h"

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
    void (*textureHandlingDoneCallback)(
        struct texturerequesthandle* request,
        void* userdata);
    void* userdata;

    // remember if we already handed a texture to this request:
    struct graphicstexture* handedTexture;
    struct graphicstexturescaled* handedTextureScaledEntry;

    // remember whether we already issued the dimensions callback:
    int textureDimensionInfoCallbackIssued;

    // remember whether we issued a texture handling callback:
    int textureHandlingDoneIssued;

    // if the request was cancelled, the callbacks
    // are gone and must not be used anymore:
    int canceled;

    // prev, next for regular request list:
    struct texturerequesthandle* prev, *next;

    // prev, next for unhandled request list:
    struct texturerequesthandle* unhandledPrev, *unhandledNext;
};

#endif  // BLITWZARD_GRAPHISTEXTUREREQUESTSTRUCT_H_

